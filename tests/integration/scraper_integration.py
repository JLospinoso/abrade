#!/usr/bin/env python3

from __future__ import annotations

import argparse
import contextlib
import http.server
import os
import select
import shutil
import socket
import socketserver
import ssl
import subprocess
import tempfile
import threading
import time
from pathlib import Path


OPENSSL_CONFIG = """[req]
distinguished_name = dn
x509_extensions = v3_req
prompt = no

[dn]
CN = 127.0.0.1

[v3_req]
subjectAltName = IP:127.0.0.1,DNS:localhost
"""


def resolve_openssl(explicit_path: str | None) -> str:
  if explicit_path:
    return explicit_path
  from_environment = os.environ.get("ABRADE_TEST_OPENSSL")
  if from_environment:
    return from_environment
  from_path = shutil.which("openssl")
  if from_path:
    return from_path
  raise AssertionError("openssl executable is required for TLS integration tests")


def generate_test_certificate(work_dir: Path, openssl: str) -> tuple[Path, Path]:
  cert_file = work_dir / "server.crt"
  key_file = work_dir / "server.key"
  config_file = work_dir / "openssl.cnf"
  config_file.write_text(OPENSSL_CONFIG, encoding="utf-8")
  command = [
    openssl,
    "req",
    "-x509",
    "-newkey",
    "rsa:2048",
    "-nodes",
    "-keyout",
    str(key_file),
    "-out",
    str(cert_file),
    "-days",
    "1",
    "-config",
    str(config_file),
    "-sha256",
  ]
  result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=20, check=False)
  if result.returncode != 0:
    raise AssertionError(
      f"openssl failed to generate TLS fixture certificate\ncommand={command}\nstdout={result.stdout}\nstderr={result.stderr}"
    )
  return cert_file, key_file


class FixtureHandler(http.server.BaseHTTPRequestHandler):
  def do_HEAD(self) -> None:
    if self.path in {"/found", "/secure", "/real-result", "/shell"}:
      self.send_response(200)
      self.send_header("Content-Length", "0")
      self.end_headers()
    else:
      self.send_response(404)
      self.send_header("Content-Length", "0")
      self.end_headers()

  def do_GET(self) -> None:
    if self.path == "/disconnect":
      with contextlib.suppress(OSError):
        self.connection.shutdown(socket.SHUT_RDWR)
      self.connection.close()
      return
    if self.path == "/slow":
      time.sleep(1)
    if self.path == "/redirect-relative":
      self.send_response(302)
      self.send_header("Location", "/found")
      self.send_header("Content-Length", "0")
      self.end_headers()
      return
    if self.path == "/redirect-cross-scheme":
      host = self.headers.get("Host", "127.0.0.1")
      self.send_response(302)
      self.send_header("Location", f"https://{host}/found")
      self.send_header("Content-Length", "0")
      self.end_headers()
      return
    if self.path == "/redirect-cross-authority":
      self.send_response(302)
      self.send_header("Location", "http://example.invalid/found")
      self.send_header("Content-Length", "0")
      self.end_headers()
      return

    routes = {
      "/found": (200, b"FOUND BODY\n"),
      "/screen": (200, b"SCREENED BODY\n"),
      "/shell": (200, b"<html><title>event shell</title>No results here</html>\n"),
      "/real-result": (200, b"HCRA RESULTS\nRace 12\nCrew: Example Canoe Club\n"),
      "/slow": (200, b"SLOW BODY\n"),
      "/secure": (200, b"SECURE BODY\n"),
    }
    status, body = routes.get(self.path, (404, b"missing\n"))
    self.send_response(status)
    self.send_header("Content-Type", "text/plain")
    self.send_header("Content-Length", str(len(body)))
    self.end_headers()
    self.wfile.write(body)

  def log_message(self, fmt: str, *args: object) -> None:
    return


class FixtureServer:
  def __init__(
    self,
    tls: bool,
    work_dir: Path,
    openssl: str | None = None,
    required_sni: str | None = None,
  ) -> None:
    self.tls = tls
    self.work_dir = work_dir
    self.observed_server_names: list[str | None] = []
    self.server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), FixtureHandler)
    if tls:
      cert_file, key_file = generate_test_certificate(work_dir, resolve_openssl(openssl))
      context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
      context.load_cert_chain(cert_file, key_file)
      if required_sni:
        def servername_callback(_sock: ssl.SSLSocket, server_name: str | None, _context: ssl.SSLContext) -> None:
          self.observed_server_names.append(server_name)
          if server_name != required_sni:
            raise ssl.SSLError(f"expected SNI {required_sni}, got {server_name}")

        context.set_servername_callback(servername_callback)
      self.server.socket = context.wrap_socket(self.server.socket, server_side=True)
    self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)

  @property
  def authority(self) -> str:
    host, port = self.server.server_address[:2]
    return f"{host}:{port}"

  @property
  def dns_authority(self) -> str:
    _host, port = self.server.server_address[:2]
    return f"localhost:{port}"

  def __enter__(self) -> "FixtureServer":
    self.thread.start()
    return self

  def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
    self.server.shutdown()
    self.server.server_close()
    self.thread.join(timeout=5)


def recv_exact(sock: socket.socket, size: int) -> bytes:
  chunks: list[bytes] = []
  remaining = size
  while remaining:
    chunk = sock.recv(remaining)
    if not chunk:
      raise ConnectionError("unexpected EOF")
    chunks.append(chunk)
    remaining -= len(chunk)
  return b"".join(chunks)


class SocksProxyHandler(socketserver.BaseRequestHandler):
  def handle(self) -> None:
    request: socket.socket = self.request
    version, method_count = recv_exact(request, 2)
    methods = recv_exact(request, method_count)
    if version != 5 or 0 not in methods:
      request.sendall(b"\x05\xff")
      return
    request.sendall(b"\x05\x00")

    version, command, _reserved, address_type = recv_exact(request, 4)
    if version != 5 or command != 1:
      request.sendall(b"\x05\x07\x00\x01\x00\x00\x00\x00\x00\x00")
      return
    if address_type == 1:
      host = socket.inet_ntoa(recv_exact(request, 4))
    elif address_type == 3:
      length = recv_exact(request, 1)[0]
      host = recv_exact(request, length).decode("ascii")
    else:
      request.sendall(b"\x05\x08\x00\x01\x00\x00\x00\x00\x00\x00")
      return
    port = int.from_bytes(recv_exact(request, 2), "big")

    with socket.create_connection((host, port), timeout=5) as upstream:
      request.sendall(b"\x05\x00\x00\x01\x7f\x00\x00\x01\x00\x00")
      while True:
        readable, _, _ = select.select([request, upstream], [], [], 5)
        if not readable:
          return
        for source in readable:
          target = upstream if source is request else request
          data = source.recv(65536)
          if not data:
            return
          target.sendall(data)


class SocksProxyServer:
  def __init__(self) -> None:
    class ThreadingTcpServer(socketserver.ThreadingTCPServer):
      allow_reuse_address = True

    self.server = ThreadingTcpServer(("127.0.0.1", 0), SocksProxyHandler)
    self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)

  @property
  def authority(self) -> str:
    host, port = self.server.server_address[:2]
    return f"{host}:{port}"

  def __enter__(self) -> "SocksProxyServer":
    self.thread.start()
    return self

  def __exit__(self, exc_type: object, exc: object, tb: object) -> None:
    self.server.shutdown()
    self.server.server_close()
    self.thread.join(timeout=5)


def run_abrade(
  exe: Path,
  cwd: Path,
  args: list[str],
  stdin: str | None = None,
  env: dict[str, str] | None = None,
  expected_returncode: int = 0,
) -> subprocess.CompletedProcess[str]:
  command = [str(exe), *args, "--init", "1", "--sint", "1000"]
  environment = os.environ.copy()
  if env:
    environment.update(env)
  result = subprocess.run(
    command,
    cwd=cwd,
    env=environment,
    input=stdin,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    timeout=30,
    check=False,
  )
  if result.returncode != expected_returncode:
    raise AssertionError(
      f"abrade returned {result.returncode}, expected {expected_returncode}\n"
      f"command={command}\nstdout={result.stdout}\nstderr={result.stderr}"
    )
  return result


def read_text(path: Path) -> str:
  return path.read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
  if not condition:
    raise AssertionError(message)


def test_head_found(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "head-found.txt"
  err = tmp / "head-found.err"
  run_abrade(exe, tmp, [server.authority, "/found", "--out", str(out), "--err", str(err)])
  require(read_text(out).splitlines() == ["/found"], "HEAD should record only found resources")
  require(not err.exists() or read_text(err) == "", "HEAD found path should not write errors")


def test_stdin_head_filters_missing(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "stdin-found.txt"
  err = tmp / "stdin-found.err"
  run_abrade(
    exe,
    tmp,
    [server.authority, "--stdin", "--out", str(out), "--err", str(err)],
    stdin="/found\n/missing\n",
  )
  require(read_text(out).splitlines() == ["/found"], "stdin HEAD should omit 404 resources")
  require(not err.exists() or read_text(err) == "", "stdin HEAD should not write errors for 404 responses")


def test_get_contents(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "contents"
  err = tmp / "contents.err"
  run_abrade(exe, tmp, [server.authority, "/found", "--contents", "--out", str(out_dir), "--err", str(err)])
  output = out_dir / "_found"
  require(output.exists(), "GET contents should write a file for 2xx responses")
  output_text = read_text(output)
  require(output_text == "FOUND BODY\n", "GET contents file should contain only the response body")
  require("HTTP/" not in output_text, "GET contents file should not contain a status line")
  require("Content-Type" not in output_text, "GET contents file should not contain response headers")


def test_screen_filters_contents(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "screened"
  err = tmp / "screened.err"
  run_abrade(
    exe,
    tmp,
    [server.authority, "/screen", "--contents", "--screen", "SCREENED", "--out", str(out_dir), "--err", str(err)],
  )
  require(not (out_dir / "_screen").exists(), "screened 2xx contents should not be written")


def test_body_filters_distinguish_shell_200_pages(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "filtered-shell"
  err = tmp / "filtered-shell.err"
  result = run_abrade(
    exe,
    tmp,
    [
      server.authority,
      "--stdin",
      "--contents",
      "--require",
      "HCRA RESULTS",
      "--reject",
      "event shell",
      "--out",
      str(out_dir),
      "--err",
      str(err),
    ],
    stdin="/shell\n/real-result\n",
  )
  require((out_dir / "_real-result").exists(), "required real body should be written")
  require(not (out_dir / "_shell").exists(), "200 shell body should be filtered")
  require("filtered=1" in result.stdout, "summary should count filtered shell pages")


def test_filter_option_errors_return_2(exe: Path, tmp: Path, server: FixtureServer) -> None:
  err = tmp / "invalid-filter.err"
  result = run_abrade(
    exe,
    tmp,
    [server.authority, "/found", "--require", "FOUND", "--err", str(err)],
    expected_returncode=2,
  )
  require("content filters require --contents" in result.stderr, "filter option error should be explicit")


def test_error_log_records_transport_failure(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "disconnect"
  err = tmp / "disconnect.err"
  result = run_abrade(
    exe,
    tmp,
    [server.authority, "/disconnect", "--contents", "--out", str(out_dir), "--err", str(err)],
    expected_returncode=1,
  )
  require(err.exists(), "transport failure should create an error log")
  err_text = read_text(err)
  require("/disconnect" in err_text, "transport failure should identify the candidate URI")
  require("errors=1" in result.stdout, "summary should count transport failures")


def test_timeout_records_error(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "timeout"
  err = tmp / "timeout.err"
  run_abrade(
    exe,
    tmp,
    [server.authority, "/slow", "--contents", "--out", str(out_dir), "--err", str(err)],
    env={"ABRADE_NETWORK_TIMEOUT_MS": "100"},
    expected_returncode=1,
  )
  require(err.exists(), "timeout should create an error log")
  err_text = read_text(err)
  require("/slow" in err_text, "timeout should identify the candidate URI")
  require("timeout" in err_text, "timeout should identify the timeout failure")


def test_socks_proxy_head(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "proxy-head.txt"
  err = tmp / "proxy-head.err"
  with SocksProxyServer() as proxy:
    run_abrade(
      exe,
      tmp,
      [server.authority, "/found", "--proxy", proxy.authority, "--out", str(out), "--err", str(err)],
    )
  require(read_text(out).splitlines() == ["/found"], "SOCKS proxy should forward HEAD requests")
  require(not err.exists() or read_text(err) == "", "SOCKS proxy HEAD should not write errors")


def test_socks_proxy_get_contents(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "proxy-contents"
  err = tmp / "proxy-contents.err"
  with SocksProxyServer() as proxy:
    run_abrade(
      exe,
      tmp,
      [server.authority, "/found", "--proxy", proxy.authority, "--contents", "--out", str(out_dir), "--err", str(err)],
    )
  output = out_dir / "_found"
  require(output.exists(), "SOCKS proxy should forward GET requests")
  require("FOUND BODY" in read_text(output), "SOCKS proxy GET should write response body")


def test_tls_no_verify(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "tls-head.txt"
  err = tmp / "tls-head.err"
  run_abrade(exe, tmp, [server.authority, "/secure", "--tls", "--out", str(out), "--err", str(err)])
  require(read_text(out).splitlines() == ["/secure"], "TLS without verification should accept test cert")


def test_tls_get_contents(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "tls-contents"
  err = tmp / "tls-contents.err"
  run_abrade(exe, tmp, [server.authority, "/secure", "--tls", "--contents", "--out", str(out_dir), "--err", str(err)])
  output = out_dir / "_secure"
  require(output.exists(), "TLS GET should write response body")
  require(read_text(output) == "SECURE BODY\n", "TLS GET should write body-only output")


def test_tls_verify_rejects_self_signed(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "tls-verify.txt"
  err = tmp / "tls-verify.err"
  result = run_abrade(
    exe,
    tmp,
    [server.authority, "/secure", "--tls", "--verify", "--out", str(out), "--err", str(err)],
    expected_returncode=1,
  )
  combined_output = result.stdout + result.stderr
  if err.exists():
    err_text = read_text(err)
    require("/secure" in err_text, "TLS verify failure should identify the candidate URI")
    combined_output += err_text
  require("ssl" in combined_output.lower(), "TLS verify failure should report an SSL error")
  require(not out.exists() or read_text(out) == "", "TLS verify failure should not record the resource as found")


def test_socks_proxy_tls_get_contents(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "proxy-tls-contents"
  err = tmp / "proxy-tls-contents.err"
  with SocksProxyServer() as proxy:
    run_abrade(
      exe,
      tmp,
      [server.authority, "/secure", "--tls", "--proxy", proxy.authority, "--contents", "--out", str(out_dir), "--err", str(err)],
    )
  output = out_dir / "_secure"
  require(output.exists(), "SOCKS proxy should tunnel TLS GET requests")
  require("SECURE BODY" in read_text(output), "proxied TLS GET should write response body")


def test_tls_sni_dns_virtual_host(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out = tmp / "tls-sni-head.txt"
  err = tmp / "tls-sni-head.err"
  run_abrade(exe, tmp, [server.dns_authority, "/secure", "--tls", "--out", str(out), "--err", str(err)])
  require(read_text(out).splitlines() == ["/secure"], "TLS DNS virtual host should succeed with SNI")
  require("localhost" in server.observed_server_names, "TLS fixture should observe localhost SNI")


def test_relative_redirect_follow(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "redirect-relative"
  err = tmp / "redirect-relative.err"
  result = run_abrade(
    exe,
    tmp,
    [
      server.authority,
      "/redirect-relative",
      "--contents",
      "--follow-redirects",
      "--out",
      str(out_dir),
      "--err",
      str(err),
    ],
  )
  output = out_dir / "_found"
  require(output.exists(), "same-origin relative redirect should write final response body")
  require(read_text(output) == "FOUND BODY\n", "relative redirect should persist final body only")
  require("attempted=2" in result.stdout, "summary should include original and redirected attempts")


def test_cross_scheme_redirect_not_followed(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "redirect-cross-scheme"
  err = tmp / "redirect-cross-scheme.err"
  result = run_abrade(
    exe,
    tmp,
    [
      server.authority,
      "/redirect-cross-scheme",
      "--contents",
      "--follow-redirects",
      "--out",
      str(out_dir),
      "--err",
      str(err),
    ],
  )
  require(not (out_dir / "_found").exists(), "cross-scheme redirect should not be followed")
  require("non-2xx=1" in result.stdout, "summary should count the non-followed 3xx response")


def test_cross_authority_redirect_not_followed(exe: Path, tmp: Path, server: FixtureServer) -> None:
  out_dir = tmp / "redirect-cross-authority"
  err = tmp / "redirect-cross-authority.err"
  result = run_abrade(
    exe,
    tmp,
    [
      server.authority,
      "/redirect-cross-authority",
      "--contents",
      "--follow-redirects",
      "--out",
      str(out_dir),
      "--err",
      str(err),
    ],
  )
  require(not (out_dir / "_found").exists(), "cross-authority redirect should not be followed")
  require("non-2xx=1" in result.stdout, "summary should count the non-followed 3xx response")


def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument("abrade", type=Path)
  parser.add_argument("--openssl")
  args = parser.parse_args()

  exe = args.abrade.resolve()
  if os.name == "nt" and exe.suffix.lower() != ".exe":
    exe = exe.with_suffix(".exe")
  require(exe.exists(), f"abrade executable does not exist: {exe}")

  with tempfile.TemporaryDirectory(prefix="abrade-integration-") as tmp_raw:
    tmp = Path(tmp_raw)
    with FixtureServer(tls=False, work_dir=tmp) as server:
      test_head_found(exe, tmp, server)
      test_stdin_head_filters_missing(exe, tmp, server)
      test_get_contents(exe, tmp, server)
      test_screen_filters_contents(exe, tmp, server)
      test_body_filters_distinguish_shell_200_pages(exe, tmp, server)
      test_filter_option_errors_return_2(exe, tmp, server)
      test_error_log_records_transport_failure(exe, tmp, server)
      test_timeout_records_error(exe, tmp, server)
      test_socks_proxy_head(exe, tmp, server)
      test_socks_proxy_get_contents(exe, tmp, server)
      test_relative_redirect_follow(exe, tmp, server)
      test_cross_scheme_redirect_not_followed(exe, tmp, server)
      test_cross_authority_redirect_not_followed(exe, tmp, server)

    tls_dir = tmp / "tls"
    tls_dir.mkdir()
    with FixtureServer(tls=True, work_dir=tls_dir, openssl=args.openssl) as server:
      test_tls_no_verify(exe, tmp, server)
      test_tls_get_contents(exe, tmp, server)
      test_tls_verify_rejects_self_signed(exe, tmp, server)
      test_socks_proxy_tls_get_contents(exe, tmp, server)

    sni_dir = tmp / "tls-sni"
    sni_dir.mkdir()
    with FixtureServer(tls=True, work_dir=sni_dir, openssl=args.openssl, required_sni="localhost") as server:
      test_tls_sni_dns_virtual_host(exe, tmp, server)


if __name__ == "__main__":
  main()

# Networking Behavior

Abrade's networking path is intentionally small: generate a request target,
establish a stream, write one HTTP request, read one response, record output, and
move to the next candidate. When redirect following is enabled, each followed
redirect creates another same-origin request attempt for that candidate.

## Default Method: HEAD

Default mode sends `HEAD` requests. Abrade reads response headers and treats any
2xx status code as a found resource. Found candidates are appended to the output
file.

Use this mode when you only need existence checks.

## Fetching Contents With GET

`--contents` switches to `GET`, reads the response body, and writes accepted 2xx
bodies to the output directory. The saved file is body-only; it does not include
the HTTP status line or headers.

```sh
abrade example.com '/items/{1:100}' --contents --out example-items
```

In verbose contents mode, Abrade prints response bodies for diagnostics. Verbose
mode does not change which responses are written.

## TLS

`--tls` uses HTTPS. The default port becomes 443 when the host does not include a
port.

```sh
abrade example.com '/items/{1:100}' --tls
```

## TLS Verification

`--verify` enables TLS peer verification and also enables TLS. Verification uses
the platform default trust paths and checks the certificate against the target
DNS host name.

```sh
abrade example.com '/items/{1:100}' --verify
```

Abrade sends SNI for DNS host names in TLS mode, including when verification is
disabled. Without `--verify`, Abrade performs TLS without peer verification. This
may be useful for local diagnostics but is not a strong authenticity check.

SNI is sent for DNS-name targets so TLS virtual hosts can select the right
certificate and application route. IP-address targets do not receive DNS-name
SNI.

## SOCKS5 Proxy

`--proxy HOST:PORT` connects to a SOCKS5 proxy before opening the target stream.
If no proxy port is supplied internally, the parser uses the SOCKS5 default port
1080.

```sh
abrade example.com '/items/{1:100}' --proxy 127.0.0.1:1080
```

Abrade supports SOCKS5 no-authentication negotiation. It does not currently
support username/password proxy authentication.

## Tor Shortcut

`--tor` is a convenience shortcut for the common local Tor SOCKS5 listener:

```sh
abrade example.com '/items/{1:100}' --tor
```

It sets the proxy to `127.0.0.1:9050` only when `--proxy` was not already
provided. This option configures a local proxy route; it is not a promise of
anonymity, legal authorization, or operational safety.

## Timeouts

These operations use the same per-operation timeout:

- DNS resolution.
- TCP connect.
- SOCKS5 authentication and connect negotiation.
- TLS handshake.
- HTTP request write.
- HTTP response read.

The default is 30000 ms. Override it with:

```sh
ABRADE_NETWORK_TIMEOUT_MS=5000 abrade example.com '/items/{1:100}' --found
```

Invalid, empty, zero, or too-large values fall back to the default.

## Adaptive Concurrency

Abrade starts with `--init` active request coroutines. Without `--optimize`, the
fixed controller keeps that recommendation constant.

With `--optimize`, the adaptive controller samples completion velocity and
adjusts its recommended coroutine count within `--min` and `--max`.

```sh
abrade example.com '/items/{1:10000}' --optimize --init 100 --min 10 --max 500
```

Adaptive concurrency is a throughput tool, not a safety mechanism. Bound it
explicitly and start conservatively.

## Filtering Response Bodies

Body filters apply after a successful response body is read. Required filters
must match before a body is written; rejected filters skip matching bodies.

```sh
abrade example.com '/items/{1:100}' --contents --require 'Item title' --reject 'not found'
```

Use this for services that return a reusable shell or error page with HTTP 200.
`--screen TEXT` remains an alias for `--reject TEXT`.

## Redirect Handling

Redirect following is disabled by default. Enable it with:

```sh
abrade example.com '/items/{1:100}' --contents --follow-redirects --max-redirects 5
```

Abrade follows relative redirects and absolute redirects that keep the original
scheme and exact host authority. It intentionally does not switch scheme,
authority, or transport target during this tranche. Cross-scheme and
cross-authority redirects remain non-followed 3xx responses.

## Error Handling

Per-candidate exceptions are written to the configured error log and do not stop
the scrape. Verbose mode also prints exception text to stderr.

The process returns `1` after the run if any candidate recorded a
transport/runtime error. Option validation errors return `2`. Completed runs
with only HTTP non-2xx responses still return `0` because the transport work
finished successfully.

`--sensitive` makes Abrade report teardown errors that are usually ignored after
response processing has completed. Use it for protocol diagnostics, not ordinary
discovery runs.

## Responsible Use

Networking options can produce substantial traffic quickly. Read
[responsible-use.md](responsible-use.md) before running Abrade against anything
other than a local fixture or explicitly authorized target.

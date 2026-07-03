#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import os
import re
import tarfile
import zipfile
from pathlib import Path


TAG_PATTERN = re.compile(r"^v(?P<version>\d+\.\d+\.\d+)$")
PROJECT_VERSION_PATTERN = re.compile(r"project\s*\(\s*abrade\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s+", re.I)


def project_version(source_dir: Path) -> str:
  cmake = (source_dir / "CMakeLists.txt").read_text(encoding="utf-8")
  match = PROJECT_VERSION_PATTERN.search(cmake)
  if not match:
    raise SystemExit("Could not find Abrade project version in CMakeLists.txt")
  return match.group(1)


def tag_version(tag: str) -> str:
  match = TAG_PATTERN.fullmatch(tag)
  if not match:
    raise SystemExit(f"Release tag must match vMAJOR.MINOR.PATCH: {tag}")
  return match.group("version")


def platform_id() -> str:
  runner_os = os.environ.get("RUNNER_OS", os.uname().sysname if hasattr(os, "uname") else "")
  runner_arch = os.environ.get("RUNNER_ARCH", os.uname().machine if hasattr(os, "uname") else "")
  normalized = {
    ("Linux", "X64"): "linux-x64",
    ("Linux", "ARM64"): "linux-arm64",
    ("macOS", "X64"): "macos-x64",
    ("macOS", "ARM64"): "macos-arm64",
    ("Windows", "X64"): "windows-x64",
    ("Windows", "ARM64"): "windows-arm64",
  }.get((runner_os, runner_arch))
  if normalized:
    return normalized

  os_name = runner_os.lower().replace("darwin", "macos")
  arch = runner_arch.lower().replace("amd64", "x64").replace("x86_64", "x64").replace("aarch64", "arm64")
  return f"{os_name}-{arch}"


def write_github_outputs(path: Path, values: dict[str, str]) -> None:
  with path.open("a", encoding="utf-8") as output:
    for key, value in values.items():
      output.write(f"{key}={value}\n")


def metadata(args: argparse.Namespace) -> None:
  source_dir = args.source_dir.resolve()
  version = tag_version(args.tag)
  cmake_version = project_version(source_dir)
  if version != cmake_version:
    raise SystemExit(f"Release tag {args.tag} does not match CMake project version {cmake_version}")

  platform = platform_id()
  archive_ext = "zip" if platform.startswith("windows-") else "tar.gz"
  artifact_base = f"abrade-{version}-{platform}"
  values = {
    "version": version,
    "tag": args.tag,
    "platform": platform,
    "artifact_base": artifact_base,
    "archive_name": f"{artifact_base}.{archive_ext}",
  }
  if args.github_output:
    write_github_outputs(args.github_output, values)
  for key, value in values.items():
    print(f"{key}={value}")


def add_zip_tree(archive: zipfile.ZipFile, root: Path) -> None:
  for path in sorted(root.rglob("*")):
    if path.is_file():
      archive.write(path, path.relative_to(root.parent).as_posix())


def package(args: argparse.Namespace) -> None:
  stage = args.stage.resolve()
  dist = args.dist.resolve()
  dist.mkdir(parents=True, exist_ok=True)
  if not stage.is_dir():
    raise SystemExit(f"Stage directory does not exist: {stage}")

  if args.archive_name.endswith(".zip"):
    archive_path = dist / args.archive_name
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
      add_zip_tree(archive, stage)
  elif args.archive_name.endswith(".tar.gz"):
    archive_path = dist / args.archive_name
    with tarfile.open(archive_path, "w:gz") as archive:
      archive.add(stage, arcname=stage.name)
  else:
    raise SystemExit(f"Unsupported archive extension: {args.archive_name}")

  print(archive_path)


def checksums(args: argparse.Namespace) -> None:
  dist = args.dist.resolve()
  archives = sorted(
    path for path in dist.iterdir()
    if path.is_file() and (path.name.endswith(".tar.gz") or path.name.endswith(".zip"))
  )
  if not archives:
    raise SystemExit(f"No release archives found in {dist}")

  lines: list[str] = []
  for archive in archives:
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    lines.append(f"{digest}  {archive.name}")
  args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
  print(args.output)


def main() -> None:
  parser = argparse.ArgumentParser()
  subcommands = parser.add_subparsers(dest="command", required=True)

  metadata_parser = subcommands.add_parser("metadata")
  metadata_parser.add_argument("--tag", required=True)
  metadata_parser.add_argument("--source-dir", type=Path, default=Path.cwd())
  metadata_parser.add_argument("--github-output", type=Path)
  metadata_parser.set_defaults(func=metadata)

  package_parser = subcommands.add_parser("package")
  package_parser.add_argument("--stage", type=Path, required=True)
  package_parser.add_argument("--dist", type=Path, required=True)
  package_parser.add_argument("--archive-name", required=True)
  package_parser.set_defaults(func=package)

  checksums_parser = subcommands.add_parser("checksums")
  checksums_parser.add_argument("--dist", type=Path, required=True)
  checksums_parser.add_argument("--output", type=Path, required=True)
  checksums_parser.set_defaults(func=checksums)

  args = parser.parse_args()
  args.func(args)


if __name__ == "__main__":
  main()

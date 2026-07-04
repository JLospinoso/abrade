![Abrade](img/Abrade.png)

# Abrade

[![Build](https://github.com/JLospinoso/abrade/actions/workflows/main.yml/badge.svg)](https://github.com/JLospinoso/abrade/actions/workflows/main.yml)
[![Release](https://github.com/JLospinoso/abrade/actions/workflows/release.yml/badge.svg)](https://github.com/JLospinoso/abrade/actions/workflows/release.yml)

Abrade is a C++23 command-line crawler for probing URL patterns quickly. It
generates candidate resource paths, performs concurrent HTTP `HEAD` requests by
default, can fetch response bodies with `GET`, and supports TLS, SOCKS5 proxies,
Tor's default local proxy, stdin-fed paths, body filters, conservative redirects,
and dry-run generation.

Abrade is meant for authorized resource discovery, migration checks, and
repeatable diagnostics against systems you own or are allowed to test. Do not
run it against third-party services without permission.

The original project write-up is
[Abrade, a high-throughput web-resource crawler](https://lospino.so/cpp/developing/software/2017/09/15/abrade-web-scraper.html).

## Install

Current binaries are published on the
[GitHub Releases](https://github.com/JLospinoso/abrade/releases) page.

Download the archive for your platform, verify it against `SHA256SUMS`, and put
the executable on your `PATH`.

```sh
# Linux
sha256sum -c SHA256SUMS --ignore-missing
tar -xzf abrade-VERSION-linux-x64.tar.gz

# macOS
shasum -a 256 -c SHA256SUMS
tar -xzf abrade-VERSION-macos-arm64.tar.gz
```

Windows PowerShell:

```powershell
Get-FileHash .\abrade-VERSION-windows-x64.zip -Algorithm SHA256
Expand-Archive .\abrade-VERSION-windows-x64.zip
```

macOS users may need to approve the downloaded executable in System Settings
because release artifacts are not notarized.

## Quick Start

Print generated URLs without making network requests:

```sh
abrade example.com '/items/{1:3}' --test
```

Probe generated paths with `HEAD` and append discovered 2xx responses to the
default output file, `example.com`:

```sh
abrade example.com '/items/{1:100}' --found
```

Fetch response bodies with `GET` and write successful bodies to a directory:

```sh
abrade example.com '/items/{1:100}' --contents --out example-items
```

Filter successful response bodies before writing them:

```sh
abrade example.com '/items/{1:100}' --contents --require 'Item title' --reject 'not found'
```

Read paths from stdin instead of generating them from a pattern:

```sh
printf '/a\n/b\n' | abrade example.com --stdin --test
```

Use TLS, verify the peer certificate, and route through a SOCKS5 proxy:

```sh
abrade example.com '/private/{001:250}' --tls --verify --proxy 127.0.0.1:1080
```

## Documentation

The documentation site entry point is [docs/index.md](docs/index.md).

- [Usage guide](docs/usage.md): CLI modes, outputs, concurrency, TLS, proxies,
  timeouts, and operational safety.
- [CLI reference](docs/cli.md): invocation forms, option semantics, environment
  variables, summaries, and exit statuses.
- [Pattern reference](docs/patterns.md): explicit ranges, implicit character
  domains, leading zeros, telescoping, continuation, and cardinality.
- [Networking behavior](docs/networking.md): HTTP methods, TLS verification,
  redirects, SOCKS5/Tor routing, timeouts, and adaptive concurrency.
- [Responsible use](docs/responsible-use.md): safety constraints for authorized
  use and local artifacts.
- [Build and development](docs/building.md): vcpkg, CMake presets, quality
  gates, coverage, CLion setup, and CI workflow expectations.
- [Source API map](docs/api.md): the private C++ core, current extension points,
  and source documentation conventions.
- [Project layout](docs/PROJECT_LAYOUT.md): canonical files, namespaces, and
  CMake targets.
- [Release process](docs/RELEASING.md): tag-driven GitHub-native release flow.
- [Project history](docs/history.md): legacy v0.1/v0.2 notes and old binary
  checksums.

## Build From Source

Abrade uses vcpkg manifest mode and CMake presets. Install CMake 3.24 or newer,
Ninja, a C++23 compiler, and vcpkg. Set `VCPKG_ROOT` to your vcpkg checkout.

```sh
git clone git@github.com:JLospinoso/abrade.git
cd abrade
export VCPKG_ROOT="$HOME/src/vcpkg"

cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

The CI-equivalent local path is:

```sh
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
./build/ci/abrade --help
./build/ci/abrade example.com '/items/{1:3}' --test
printf '/a\n/b\n' | ./build/ci/abrade example.com --stdin --test
```

For substantial local work, run the quality preset and static analysis:

```sh
cmake --preset quality
cmake --build --preset quality
ctest --preset quality
cmake --build --preset format-check
cmake --build --preset quality --target abrade_clang_tidy
cmake --build --preset quality --target abrade_cppcheck
```

See [docs/building.md](docs/building.md) for platform notes and optional
coverage/sanitizer presets.

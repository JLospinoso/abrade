# Abrade Documentation

Abrade is a C++23 CLI for probing generated web-resource paths at high
concurrency. It is optimized for a narrow job: take a host plus a path pattern,
generate candidate URLs, and record which resources exist or what their response
bodies contain.

Use Abrade only against systems you own or have permission to test. It can create
large request volumes quickly; start with `--test`, bound the pattern space, and
choose conservative concurrency before running against any live service.

## Start Here

1. Install a release binary from
   [GitHub Releases](https://github.com/JLospinoso/abrade/releases), or build
   from source with [building.md](building.md).
2. Dry-run the generated URL set:

   ```sh
   abrade example.com '/items/{1:3}' --test
   ```

3. Probe with `HEAD` when you only need to know whether a resource exists:

   ```sh
   abrade example.com '/items/{1:100}' --found
   ```

4. Switch to `--contents` only when response bodies are needed:

   ```sh
   abrade example.com '/items/{1:100}' --contents --out example-items
   ```

## Documentation Map

- [Usage guide](usage.md): command modes, output files, TLS, proxies, stdin,
  timeouts, and concurrency controls.
- [CLI reference](cli.md): invocation forms, option semantics, environment
  variables, summaries, and exit statuses.
- [Pattern reference](patterns.md): all brace-pattern forms and how Abrade counts
  generated candidates.
- [Networking behavior](networking.md): HTTP methods, TLS verification,
  SOCKS5/Tor routing, timeouts, and adaptive concurrency.
- [Responsible use](responsible-use.md): safety constraints for authorized use,
  generated request volume, and scrape artifacts.
- [Build and development](building.md): local builds, CMake presets, vcpkg,
  quality checks, coverage, CLion, and CI.
- [Source API map](api.md): documented internal API surface and source
  documentation conventions.
- [Project layout](PROJECT_LAYOUT.md): canonical directories, target names, and
  source organization rules.
- [Release process](RELEASING.md): tag-driven release rehearsals and publishing.
- [Project history](history.md): legacy v0.1/v0.2 notes and historical checksums.

## External Reference

The original design and pattern syntax article is
[Abrade, a high-throughput web-resource crawler](https://lospino.so/cpp/developing/software/2017/09/15/abrade-web-scraper.html).
The current documentation in this directory is authoritative for the modern
build and release workflow.

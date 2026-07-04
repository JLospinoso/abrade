# Project History

Abrade began as a high-throughput web-resource crawler built around
asynchronous I/O, TLS, SOCKS5 proxy support, and a compact URL pattern grammar.
The original 2017 write-up remains useful for historical motivation and pattern
examples:

[Abrade, a high-throughput web-resource crawler](https://lospino.so/cpp/developing/software/2017/09/15/abrade-web-scraper.html)

Current build, install, and release instructions live in this repository's
README and `docs/` directory.

## v0.2 Notes

Abrade v0.2 added stdin-driven candidate paths:

```sh
echo /anything/a/b/c?d=123 | abrade httpbin.org --stdin --contents --verbose
```

When using `--stdin`, omit the positional pattern argument.

v0.2 also added `--screen` for filtering known error landing pages that return
HTTP 200. In `--contents` mode, matching bodies are not written to disk.

Network resolve, connect, TLS handshake, proxy negotiation, request write, and
response read operations use a 30 second timeout by default.

## Historical Binaries

Older pre-GitHub-Releases binaries were distributed outside the modern release
workflow. These checksums are kept only for provenance; use
[GitHub Releases](https://github.com/JLospinoso/abrade/releases) for current
downloads.

- v0.2.0 Linux ELF: SHA-256 `89df60eebcf1c8f224fed98b89ee403b45022c86181a12e84cba8abc5d56ca07`
- v0.2.0 Windows EXE: SHA-256 `b574aa1d8e37f9f0a867ed4d890d5b3d152388f0f4e3d9c9c4223d7804d1be4b`
- v0.1.0 Linux ELF: SHA-256 `1b8adf0fe8b7db252c4f84398bf5980f0a0c57a7592cd529ac6558b57407f238`
- v0.1.0 Windows EXE: SHA-256 `f98ca3a68fbdcc7dde3f7db868b24d8a0b328d3c05732aa1d81b5a70b0531f31`

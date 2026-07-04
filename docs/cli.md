# CLI Reference

This page is the canonical option reference for the current executable. Check it
against `abrade --help` whenever parser behavior changes.

## Invocation Forms

Generate candidate paths from a pattern:

```sh
abrade HOST PATTERN [options]
```

Read candidate paths from standard input:

```sh
abrade HOST --stdin [options]
```

Preview generated or stdin-fed URLs without network requests:

```sh
abrade HOST PATTERN --test [options]
abrade HOST --stdin --test [options]
```

## Positional Values

| Value | Meaning |
| --- | --- |
| `HOST` | Target host authority, optionally including a port. Examples: `example.com`, `example.com:8080`, `[::1]:8443`. |
| `PATTERN` | URI target pattern. Defaults to `/` when omitted by the parser, but normal pattern mode should pass it explicitly. Omit it when using `--stdin`. |

The scheme is not part of `HOST`. Abrade uses HTTP by default and HTTPS when
`--tls` or `--verify` is active.

## Request Mode

| Option | Meaning |
| --- | --- |
| default | Send `HEAD`, record 2xx candidate paths. |
| `--contents`, `-c` | Send `GET`, read response bodies, and write accepted 2xx bodies to an output directory. |

`--contents` files contain the response body only. They do not include HTTP
status lines or response headers. `--verbose` prints diagnostic request and
response details, but it does not change which responses are persisted.

## Input and Preview

| Option | Meaning |
| --- | --- |
| `--stdin`, `-d` | Read one request target per line from stdin. |
| `--test` | Print generated URLs and exit before any network request. |

Use `--test` as the first step for every new pattern:

```sh
abrade example.com '/items/{1:3}' --test
```

## Output

| Option | Meaning |
| --- | --- |
| `--out PATH` | Output file for `HEAD`, output directory for `--contents`. Default is `HOST` for `HEAD` and `HOST-contents` for `--contents`. |
| `--err PATH` | Append-only error log. Default is `HOST-err.log`. |
| `--found`, `-f` | Print successful 2xx candidates. |
| `--verbose`, `-v` | Print detailed request/response output and imply `--found`. |

## Body Filters

| Option | Meaning |
| --- | --- |
| `--require TEXT` | Require a literal body substring before writing. Repeatable. |
| `--reject TEXT` | Reject bodies containing a literal substring. Repeatable. |
| `--require-regex PATTERN` | Require a Boost.Regex body match before writing. Repeatable. |
| `--reject-regex PATTERN` | Reject bodies matching a Boost.Regex pattern. Repeatable. |
| `--screen TEXT` | Backward-compatible alias for `--reject TEXT`. |

Body filters are valid only with `--contents`, because they inspect `GET`
response bodies. They are useful for applications that return generic 200-level
shell pages for missing records.

## Redirects

| Option | Meaning |
| --- | --- |
| `--follow-redirects` | Follow redirects only when the target remains on the same scheme and host authority. |
| `--max-redirects N` | Maximum redirect hops per generated candidate. Default is `5`; valid only with `--follow-redirects`. |

Redirect following is off by default. Abrade follows relative redirects and
absolute redirects that keep the same scheme and exact host authority. Cross
scheme or cross authority redirects are left as non-followed 3xx responses.

## TLS and Proxies

| Option | Meaning |
| --- | --- |
| `--tls`, `-t` | Use HTTPS. Peer verification remains disabled unless `--verify` is set. |
| `--verify`, `-r` | Verify the TLS peer certificate and DNS host name, and enable HTTPS. |
| `--proxy HOST:PORT` | Route through a SOCKS5 proxy. |
| `--tor`, `-o` | Use `127.0.0.1:9050` as the SOCKS5 proxy unless `--proxy` is already set. |
| `--sensitive`, `-s` | Surface teardown errors that are normally ignored. |

Abrade currently supports SOCKS5 no-authentication negotiation.

In TLS mode, Abrade sends SNI for DNS host names. `--verify` adds platform
CA-chain validation and DNS host-name verification.

## Pattern Options

| Option | Meaning |
| --- | --- |
| `--leadzero`, `-l` | Preserve leading zero-domain positions in implicit ranges. |
| `--telescoping`, `-e` | Expand implicit ranges by suffix length, shortest to longest. |

See [patterns.md](patterns.md) for the pattern grammar.

## Concurrency

| Option | Default | Meaning |
| --- | --- | --- |
| `--init`, `-i` | `1000` | Initial concurrent request count. |
| `--optimize`, `-p` | off | Enable adaptive concurrency. |
| `--min` | `1` | Minimum adaptive concurrency. |
| `--max` | `25000` | Maximum adaptive concurrency. |
| `--ssize` | `50` | Adaptive velocity sliding-window size. |
| `--sint` | `1000` | Completion interval between adaptive samples. |

Start lower than the defaults when testing a target for the first time.

## HTTP Headers

| Option | Meaning |
| --- | --- |
| `--agent VALUE` | User-Agent header. Defaults to a Firefox 47-style value retained for compatibility. |

## Help

```sh
abrade --help
```

## Environment

| Variable | Meaning |
| --- | --- |
| `ABRADE_NETWORK_TIMEOUT_MS` | Per-operation network timeout in milliseconds. Invalid values fall back to 30000 ms. |

## Run Summary

Network runs print a final summary with attempted requests, 2xx responses,
non-2xx responses, filtered bodies, runtime errors, bytes written, elapsed
seconds, requests per second, and MiB per second.

## Exit Status

| Code | Meaning |
| --- | --- |
| `0` | Help, `--test`, or a completed run with no transport/runtime errors. |
| `1` | A completed run recorded at least one transport/runtime error, or an unexpected top-level runtime failure occurred. |
| `2` | Option or parser validation failed before the scrape began. |

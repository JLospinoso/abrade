# Usage Guide

Abrade takes a host and a candidate path source, then runs many asynchronous HTTP
requests. The default query is `HEAD`, which records candidate paths that return
2xx status codes. `--contents` switches to `GET` and writes response bodies.

## Safety First

- Use Abrade only on systems you own or have explicit authorization to test.
- Run `--test` first to inspect the generated URL set.
- Check the generated cardinality before running a network scrape. Abrade prints
  an exact count when it fits in `size_t` and a log cardinality otherwise.
- Start with lower concurrency when testing a new target.
- Prefer `HEAD` unless you need response bodies.

## Basic Shape

```sh
abrade HOST PATTERN [options]
```

`HOST` is an authority such as `example.com`, `example.com:8080`, `::1`, or
`[::1]:8443`. The URI scheme is selected by options: `http://` by default and
`https://` with `--tls` or `--verify`.

`PATTERN` is a path or path-plus-query template. Pattern syntax is documented in
[patterns.md](patterns.md).

```sh
abrade example.com '/items/{1:3}' --test
```

produces:

```text
http://example.com/items/1
http://example.com/items/2
http://example.com/items/3
```

## Request Modes

### HEAD Probes

Default mode sends `HEAD` requests and appends successful candidates to an output
file. The default output path is the host name.

```sh
abrade example.com '/items/{1:100}' --found
```

Use `--out` to choose a different output file:

```sh
abrade example.com '/items/{1:100}' --out found-items.txt
```

`--found` prints successful 2xx responses as they are found. `--verbose` prints
all status codes and implies `--found`.

### GET Contents

`--contents` sends `GET` requests and writes accepted 2xx response bodies to an
output directory. The default directory is `HOST-contents`. Output files contain
the response body only, with no HTTP status line or response headers.

```sh
abrade example.com '/items/{1:100}' --contents --out example-items
```

Filenames are derived from candidate paths with characters outside
`[A-Za-z0-9.-]` replaced by `_`.

When `--verbose` is also set, Abrade prints each response body for diagnostics.
Verbose mode does not change which bodies are written.

### Filter Known Shell or Error Pages

Some applications return HTTP 200 for a generic error page, shell page, or
empty result. In `--contents` mode, body filters let you require evidence of a
real page and reject known placeholders:

```sh
abrade example.com '/items/{1:100}' --contents --require 'Item title' --reject 'not found'
```

Literal filters are repeatable with `--require TEXT` and `--reject TEXT`. Regex
filters are repeatable with `--require-regex PATTERN` and `--reject-regex
PATTERN`. `--screen TEXT` remains a backward-compatible alias for `--reject
TEXT`.

The HCRA results sample that prompted this behavior had many HTTP 200 pages that
were not useful records: some were no-event pages, blank shells, cancellations,
or headers without row data. Treat 2xx status as a transport signal, not proof
that the page contains the domain record you wanted.

### Redirects

Redirect following is opt-in:

```sh
abrade example.com '/items/{1:100}' --contents --follow-redirects --max-redirects 5
```

Abrade follows relative redirects and absolute redirects that stay on the same
scheme and host authority. It does not follow cross-scheme or cross-authority
redirects in this tranche; those remain recorded as non-2xx responses.

### Stdin Input

Use `--stdin` when another tool owns candidate generation. Omit the positional
pattern; Abrade reads one path per line.

```sh
printf '/a\n/b\n' | abrade example.com --stdin --test
```

Stdin values are used as request targets exactly as provided. Include the leading
slash when the server expects an absolute path.

## TLS and Proxies

`--tls` uses HTTPS with peer verification disabled by default.

```sh
abrade example.com '/items/{1:100}' --tls
```

`--verify` enables TLS peer verification and also enables TLS:

```sh
abrade example.com '/items/{1:100}' --verify
```

`--proxy HOST:PORT` routes requests through a SOCKS5 proxy. `--tor` is a shortcut
for `--proxy 127.0.0.1:9050` unless a proxy was already supplied.

```sh
abrade example.com '/items/{1:100}' --tls --proxy 127.0.0.1:1080
abrade example.com '/items/{1:100}' --tls --tor
```

Abrade currently supports SOCKS5 "no authentication" proxy negotiation.

## Timeouts

DNS resolution, TCP connect, TLS handshake, proxy negotiation, request write, and
response read operations use the same per-operation timeout. The default is 30
seconds.

Set `ABRADE_NETWORK_TIMEOUT_MS` to override it for diagnostics and tests:

```sh
ABRADE_NETWORK_TIMEOUT_MS=5000 abrade example.com '/items/{1:100}' --found
```

Invalid, empty, zero, or too-large values fall back to the default timeout.

## Concurrency

Abrade starts with `--init` concurrent request coroutines. The default is 1000.
Use lower values for first contact with a target or for local loopback tests.

```sh
abrade example.com '/items/{1:1000}' --init 50 --found
```

`--optimize` enables adaptive concurrency. The controller samples completion
velocity and adjusts the recommended coroutine count between `--min` and `--max`.

```sh
abrade example.com '/items/{1:10000}' --optimize --init 100 --min 10 --max 500
```

Controller tuning options:

- `--init`: initial concurrent request count, default `1000`.
- `--min`: minimum adaptive concurrency, default `1`.
- `--max`: maximum adaptive concurrency, default `25000`.
- `--ssize`: adaptive velocity window size, default `50`.
- `--sint`: completion sampling interval, default `1000`.

## Output and Errors

Default output paths:

- `HEAD`: file named `HOST`.
- `GET --contents`: directory named `HOST-contents`.
- errors: append-only file named `HOST-err.log`.

Use `--out` and `--err` to set explicit paths:

```sh
abrade example.com '/items/{1:100}' --out found.txt --err scrape-errors.log
```

Every network run prints a final summary with attempted requests, 2xx responses,
non-2xx responses, filtered bodies, transport/runtime errors, bytes written,
elapsed seconds, requests per second, and MiB per second. Abrade returns `1`
after a completed run if any candidate recorded a transport/runtime error.
Parser and option validation errors return `2`. Help, `--test`, and completed
runs without transport/runtime errors return `0`.

`--sensitive` makes Abrade report TCP and TLS teardown errors that are normally
ignored when the response work has already completed. This is useful for protocol
diagnostics and too noisy for most discovery runs.

## Full Help

Run:

```sh
abrade --help
```

The help text is generated from the same parser used by the executable and is
the quickest way to check option spelling for the installed binary.

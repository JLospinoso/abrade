# Source API Map

Abrade is maintained as a CLI product with a private C++ core. The headers under
`src/abrade/` are documented so maintainers can extend the scraper safely, but
they are not installed as a supported public library API.

## Namespaces and Include Style

Production symbols live in `namespace abrade`. Project headers are included with
the `abrade/` prefix:

```cpp
#include <abrade/generator.hpp>
```

The command-line entry point is `src/cli/main.cpp`. CLI-only orchestration should
stay there unless it grows enough to justify a dedicated `src/cli/` header and
implementation pair.

## Core Components

### Candidate Generation

Files:

- `src/abrade/generator.hpp`
- `src/abrade/generator.cpp`
- `src/abrade/candidate.hpp`

`Generator` is the abstract source of candidate URI paths. `StdinGenerator`
reads one path per line from standard input. `UriGenerator` parses Abrade brace
patterns and emits paths in deterministic order.

`Range` and its concrete implementations own the counter domains used by
`UriGenerator`:

- `ExplicitRange` for inclusive decimal `{START:END}` ranges.
- `ImplicitRange` for character-domain patterns such as `{ddd}` or `{hh}`.
- `TelescopingRange` for suffix-width expansion when `--telescoping` is enabled.
- `ContinuationRange` for `{}` mirrors of the previous range.

`Candidate` is the request-level object passed from generation into request
writing and query execution. Today it carries a URI path plus reserved extension
points for request headers and body contents.

### Options and CLI Wiring

Files:

- `src/abrade/options.hpp`
- `src/abrade/options.cpp`
- `src/cli/main.cpp`

`Options` is the single parsed representation of command-line input. It owns the
generated help text, applies derived settings such as `--verify` implying TLS and
`--tor` setting the default SOCKS5 proxy, and validates numeric concurrency
settings.

`main.cpp` keeps product wiring explicit: build a generator, choose HEAD or GET,
choose direct or proxied transport, choose a fixed or adaptive controller, then
run `Scraper`. It also owns the process-level exit-code mapping: option errors
return `2`, runtime/transport errors return `1`, and successful/help/test paths
return `0`.

### Network Transport

Files:

- `src/abrade/endpoint.hpp`
- `src/abrade/connection.hpp`
- `src/abrade/network_timeout.hpp`
- `src/abrade/exception.hpp`

`parse_host_endpoint` normalizes host authorities and default ports. It accepts
plain host names, `host:port`, unbracketed IPv6 literals without explicit ports,
and bracketed IPv6 with ports.

Connection policy classes establish streams:

- `PlaintextConnection`: direct TCP.
- `TlsConnection`: direct TCP plus TLS handshake.
- `ProxiedConnection`: SOCKS5 plus plaintext target stream.
- `ProxiedTlsConnection`: SOCKS5 plus TLS handshake.

`Connection<Stream>` owns cleanup behavior around an established stream. The
transport classes use `network_timeout.hpp` helpers to apply the same
per-operation timeout to DNS, connect, proxy negotiation, TLS handshake, writes,
and reads.

### Requests, Queries, and Actions

Files:

- `src/abrade/writer.hpp`
- `src/abrade/query.hpp`
- `src/abrade/action.hpp`
- `src/abrade/content_filter.hpp`
- `src/abrade/redirect_policy.hpp`
- `src/abrade/http_status.hpp`

`RequestWriter` clones an immutable request template, applies the query method,
sets the generated target URI, and asynchronously writes the HTTP request.

`HeadQuery` reads headers and passes status codes to `HeadAction`. `GetQuery`
reads full response bodies and passes status plus body contents to `GetAction`.
Both query types can return a redirect target when `RedirectPolicy` allows a
same-scheme, same-authority redirect.

Actions own product output behavior:

- `HeadAction` appends successful candidates to a file and optionally prints all
  status codes.
- `GetAction` writes accepted 2xx body-only contents to an output directory,
  applies `ContentFilters`, reports filtered bodies and bytes written, and keeps
  verbose output diagnostic-only.

### Scraper Runtime

Files:

- `src/abrade/controller.hpp`
- `src/abrade/controller.cpp`
- `src/abrade/run_stats.hpp`
- `src/abrade/scraper.hpp`
- `src/abrade/scraper_runtime.hpp`

`Controller` decides how many request coroutines should be active. The fixed
controller keeps a constant recommendation; the adaptive controller samples
completion velocity and adjusts its recommendation within configured bounds.

`RunStats` aggregates attempted requests, status classes, filtered bodies,
transport/runtime errors, bytes written, elapsed time, and throughput metrics for
the final run summary.

`Scraper` coordinates generation, connection setup, request writing, optional
redirect follow-up requests, query execution, completion accounting, run stats,
and error logging. It is templated over the generator, query, connection policy,
request writer, and error log so production logic can be unit-tested without
introducing a public library boundary.

`FileErrorLog` preserves candidate context for exceptions in an append-only
error file.

## Documentation Standard

Document headers when a maintainer needs to know:

- what role a type plays in the CLI workflow;
- what invariants or ownership rules callers must respect;
- what an option or flag changes downstream;
- whether behavior is product output, test-only support, or a future extension
  point;
- what errors or exceptional cases are intentionally surfaced.

Prefer concise `///` comments on declarations over implementation narration.
Implementation comments should explain non-obvious protocol, concurrency, or
overflow behavior, not restate ordinary C++.

## Extension Guidance

- Add pattern syntax in `UriGenerator` and cover it in `tests/unit/`.
- Add request-level behavior through `Candidate`, `RequestWriter`, and the query
  or action layer rather than directly in `Scraper`.
- Add transport variants as connection policies with the same coroutine
  `connect(sock, yield)` shape.
- Keep installed headers out of releases until a dedicated library API issue
  defines support, compatibility, and documentation requirements.

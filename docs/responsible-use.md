# Responsible Use

Abrade is a high-concurrency crawler. Treat it as a tool for authorized
diagnostics, not as a way to bypass another service's controls.

## Rules

- Use Abrade only against systems you own or have explicit permission to test.
- Respect service terms, rate limits, access controls, and applicable law.
- Do not use Abrade to bypass authentication, authorization, robots rules,
  account limits, paywalls, or rate limits.
- Do not aim Abrade at third-party services just because examples are easy to
  write.
- Prefer local fixtures, loopback integration tests, or neutral test services for
  examples and validation.

## Before Any Network Run

1. Preview the generated URL set with `--test`.
2. Confirm the cardinality is intentional.
3. Start with conservative concurrency, for example `--init 10`.
4. Prefer default `HEAD` probes over `--contents` unless response bodies are
   required.
5. Set an explicit timeout if the target is slow or unstable.
6. Stop immediately if the target shows distress, unexpected traffic volume, or
   authorization ambiguity.

Example preview:

```sh
abrade example.com '/items/{1:3}' --test
```

Example low-concurrency run:

```sh
abrade example.com '/items/{1:100}' --init 10 --found
```

## Sensitive Outputs

`--contents` writes response bodies to disk. Those bodies may contain private,
regulated, or user-owned data. Choose output paths deliberately, avoid committing
scrape output, and clean up local artifacts when they are no longer needed.

Error logs can also contain request targets and exception details. Treat them as
operational artifacts, not public documentation.

## Tor and Proxies

`--proxy` and `--tor` change routing. They do not grant permission, guarantee
anonymity, or make an unsafe run safe. Use them only when they are part of an
authorized diagnostic setup.

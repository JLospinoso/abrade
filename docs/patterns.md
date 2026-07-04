# Pattern Reference

Abrade patterns describe URI paths. Literal text is copied into every generated
candidate, and brace forms create ranges.

Always inspect a new pattern with `--test` before making network requests:

```sh
abrade example.com '/items/{1:3}' --test
```

## Explicit Numeric Ranges

Use `{START:END}` for an inclusive decimal range:

```sh
abrade example.com '/items/{1:3}' --test
```

produces:

```text
http://example.com/items/1
http://example.com/items/2
http://example.com/items/3
```

`END` must be greater than or equal to `START`. Both sides must parse as
non-negative `size_t` values.

## Implicit Character Ranges

Implicit ranges use one or more domain symbols. Each symbol contributes one
position to the generated value.

| Symbol | Domain |
| --- | --- |
| `o` | `01234567` |
| `d` | `0123456789` |
| `h` | `0123456789abcdef` |
| `H` | `0123456789ABCDEF` |
| `a` | `abcdefghijklmnopqrstuvwxyz` |
| `A` | `ABCDEFGHIJKLMNOPQRSTUVWXYZ` |
| `n` | `0123456789abcdefghijklmnopqrstuvwxyz` |
| `N` | `0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ` |
| `b` | `0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz` |

Example:

```sh
abrade example.com '/hex/{hh}' --test
```

generates 256 candidates, from `/hex/00` through `/hex/ff`.

Mixed symbols are allowed:

```sh
abrade example.com '/file/{AAdd}' --test
```

The rightmost position increments fastest, the same way an odometer rolls over.

## Leading Zeros

Implicit ranges suppress leading zero-domain positions by default, except for the
last position. For example, `{ddd}` without `--leadzero` begins:

```text
0
1
2
...
9
10
```

Use `--leadzero` to preserve the full implicit width:

```sh
abrade example.com '/items/{ddd}' --leadzero --test
```

begins:

```text
http://example.com/items/000
http://example.com/items/001
http://example.com/items/002
```

Explicit numeric ranges are emitted as ordinary decimal numbers and are not
zero-padded by `--leadzero`.

## Telescoping

`--telescoping` expands implicit patterns by increasing suffix length. A pattern
such as `{ddd}` is treated as the union of `{d}`, `{dd}`, and `{ddd}`.

```sh
abrade example.com '/items/{ddd}' --telescoping --test
```

This is useful for unknown identifier widths. Combine it with `--leadzero` if
the target path requires fixed-width values at each suffix length.

## Continuation

An empty brace pair `{}` mirrors the current value of the immediately preceding
range. It does not add cardinality by itself.

```sh
abrade example.com '/album/{1:3}/photo/{}' --test
```

produces:

```text
http://example.com/album/1/photo/1
http://example.com/album/2/photo/2
http://example.com/album/3/photo/3
```

A pattern cannot start with `{}` because there is no previous range to mirror.

## Multiple Ranges

Multiple ranges form a Cartesian product. The rightmost range changes fastest.

```sh
abrade example.com '/a/{1:2}/b/{A}' --test
```

begins:

```text
http://example.com/a/1/b/A
http://example.com/a/1/b/B
http://example.com/a/1/b/C
```

and continues through all uppercase letters before moving to `/a/2/...`.

## Cardinality

Abrade prints the generated set size before a network run when it can represent
the exact value as `size_t`. Very large patterns print a natural-log cardinality
instead.

Use this output to catch accidental explosions before they become live request
volume:

```sh
abrade example.com '/items/{bbbbbb}' --test
```

The original pattern article has more historical examples:
[Abrade, a high-throughput web-resource crawler](https://lospino.so/cpp/developing/software/2017/09/15/abrade-web-scraper.html).

# Project Layout

Abrade is organized as a command-line application with a private C++ core.

```text
src/
  abrade/       Private production headers and implementations in namespace abrade.
  cli/          Command-line entry point and CLI orchestration.
tests/
  unit/         Catch2 unit tests for core behavior.
  integration/ Process-level and loopback integration tests.
cmake/          Reusable CMake modules for compiler options, tests, and analysis.
tools/          Maintainer automation that is not part of the product binary.
docs/           Project and maintainer documentation.
```

`docs/index.md` is the documentation site entry point. The README is the project
front door; deeper operational and maintainer material belongs under `docs/`.

## Source Rules

- Production headers are private project headers under `src/abrade/`.
- Use `.hpp` for headers and `.cpp` for implementation files.
- Include production headers with the `abrade/` prefix, for example:

```cpp
#include <abrade/generator.hpp>
```

- Put production symbols in `namespace abrade`.
- Keep CLI-only helpers in `src/cli/main.cpp`, either in an anonymous namespace or in a future
  `abrade::cli` namespace if they grow enough to justify separate files.
- Document maintainer-facing types and functions in headers with concise `///`
  comments. Do not introduce installed public headers unless a dedicated library
  API issue defines that contract.

## CMake Targets

- `abrade_core` is the private static library containing reusable scraper logic.
- `abrade_cli` is the executable target and emits the `abrade` binary.
- `abrade_unit_tests` is the Catch2 test binary.
- `abrade_format`, `abrade_format_check`, `abrade_clang_tidy`, and `abrade_cppcheck` are optional quality targets when the
  corresponding tools are installed.

This keeps the public developer interface stable while allowing internal structure to evolve.

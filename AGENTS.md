# Abrade Agent Instructions

- Treat Abrade as a CLI product, not a public C++ library. Do not add installed public headers
  unless a dedicated library API issue explicitly calls for it.
- Keep production code under `src/abrade/` in `namespace abrade`. Keep CLI-only orchestration under
  `src/cli/`.
- Use lower-snake-case file names. C++ headers use `.hpp`; implementation and tests use `.cpp`.
- Include project headers with the canonical prefix, for example `#include <abrade/options.hpp>`.
- Keep the internal CMake library target named `abrade_core`. Keep the executable target named
  `abrade_cli` with output name `abrade`.
- Treat `docs/index.md` as the documentation site hub and Markdown under `docs/` as canonical
  documentation. Do not add a docs generator without explicit maintainer approval.
- Keep CLI docs synchronized with `abrade --help`; when they disagree, inspect and fix the parser,
  generated help, or docs in the same change.
- Prefer `--test` examples in documentation so examples preview URL generation before network use.
  Do not add examples that target real third-party services except neutral test services or local
  fixtures.
- Add behavior coverage in `tests/unit/` for core logic and `tests/integration/` for process-level
  smoke or loopback tests.
- Run the quality preset before handing off substantial changes:
  `cmake --preset quality`, `cmake --build --preset quality`, `ctest --preset quality`.
- Run `cmake --build --preset quality --target abrade_clang_tidy` when changing C++ source.

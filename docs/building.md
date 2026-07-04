# Build and Development

Abrade uses vcpkg manifest mode, CMake presets, Ninja, Catch2 v3, CTest, and
strict optional static-analysis targets. The C++ baseline is C++23.

## Prerequisites

- CMake 3.24 or newer.
- Ninja.
- A C++23-capable compiler.
- vcpkg.
- Python 3 for release tooling and integration tests.

Set `VCPKG_ROOT` to your vcpkg checkout:

```sh
export VCPKG_ROOT="$HOME/src/vcpkg"
```

Windows PowerShell:

```powershell
$env:VCPKG_ROOT = "C:\src\vcpkg"
```

Abrade does not vendor vcpkg as a submodule. CI pins the vcpkg commit in the
workflow environment.

## Developer Build

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

The executable is written to `build/dev/abrade` on Unix-like hosts and
`build/dev/abrade.exe` on Windows.

## CI-Equivalent Build

Use the `ci` preset when you want the same release-mode build used by the
required hosted GitHub Actions check:

```sh
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
./build/ci/abrade --help
./build/ci/abrade example.com '/items/{1:3}' --test
printf '/a\n/b\n' | ./build/ci/abrade example.com --stdin --test
```

`ctest --preset ci` includes unit tests and a loopback scraper integration test.

## Quality Build

For substantial changes, use the quality preset:

```sh
cmake --preset quality
cmake --build --preset quality
ctest --preset quality
```

The quality preset enables project warnings and treats them as errors. It also
exports a compile database for static-analysis targets.

Run clang-format before handoff:

```sh
cmake --build --preset format
cmake --build --preset format-check
```

Run clang-tidy when C++ source changes:

```sh
cmake --build --preset quality --target abrade_clang_tidy
```

Run cppcheck when available:

```sh
cmake --build --preset quality --target abrade_cppcheck
```

The shorthand build presets are also available:

```sh
cmake --build --preset format-check
cmake --build --preset clang-tidy
cmake --build --preset cppcheck
```

## Coverage

Coverage is available on non-Windows hosts:

```sh
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage
bash tools/quality/llvm-coverage-report.sh
```

The report script uses LLVM coverage tooling and prints a line coverage report.

## Sanitizers

AddressSanitizer and UndefinedBehaviorSanitizer are available on non-Windows
hosts:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

## CLion

CLion should use the repository presets directly:

1. Open `/Users/jalospinoso/abrade` as the project root.
2. Enable CMake presets if CLion does not detect them automatically.
3. Set or inherit `VCPKG_ROOT` in the CLion environment.
4. Use `dev` for normal editing, `quality` before handoff, and `asan` for
   memory diagnostics on non-Windows hosts.
5. Keep the build directory inside `build/<preset>` so command-line and IDE
   builds share the same layout.

The canonical targets are:

- `abrade_core`: private static library for production logic.
- `abrade_cli`: executable target with output name `abrade`.
- `abrade_unit_tests`: Catch2 unit test binary.
- `abrade_format` and `abrade_format_check`: optional clang-format targets.
- `abrade_clang_tidy`: optional clang-tidy target.
- `abrade_cppcheck`: optional cppcheck target.

## GitHub Actions

The required protected check is named `Build (GitHub)`. It runs on GitHub's
hosted Ubuntu runner for pushes and pull requests. A manual hosted validation
workflow matrix is available through `workflow_dispatch` for selective Ubuntu,
macOS, and Windows validation.

The release workflow is tag-driven and documented in [RELEASING.md](RELEASING.md).

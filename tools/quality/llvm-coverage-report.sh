#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build/coverage}"
profdata="${build_dir}/abrade.profdata"

resolve_tool() {
  local name="$1"
  if command -v "${name}" >/dev/null 2>&1; then
    command -v "${name}"
    return
  fi
  if command -v xcrun >/dev/null 2>&1; then
    xcrun --find "${name}"
    return
  fi
  return 1
}

llvm_profdata="${LLVM_PROFDATA:-$(resolve_tool llvm-profdata)}"
llvm_cov="${LLVM_COV:-$(resolve_tool llvm-cov)}"

shopt -s nullglob
profiles=("${build_dir}"/*.profraw)
if [[ ${#profiles[@]} -eq 0 ]]; then
  echo "No .profraw files found in ${build_dir}. Run ctest --preset coverage first." >&2
  exit 1
fi

"${llvm_profdata}" merge -sparse "${profiles[@]}" -o "${profdata}"
"${llvm_cov}" report \
  "${build_dir}/abrade_test" \
  -object="${build_dir}/abrade" \
  -instr-profile="${profdata}" \
  -ignore-filename-regex='(vcpkg_installed|CMakeFiles|/build/)'

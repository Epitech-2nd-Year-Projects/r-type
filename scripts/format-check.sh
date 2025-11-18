#!/bin/bash
set -euo pipefail

if [[ -n "${LLVM_BINDIR:-}" ]]; then
  export PATH="${LLVM_BINDIR}:${PATH}"
fi

echo "clang-format available at: $(command -v clang-format)"
clang-format --version

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -p windows -a x64 -m release --toolchain=clang
fi

xmake format --dry-run

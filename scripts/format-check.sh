#!/bin/bash
set -euo pipefail

if [[ -n "${LLVM_BINDIR:-}" ]]; then
  export PATH="${LLVM_BINDIR}:${PATH}"
fi

echo "clang-format available at: $(command -v clang-format)"
clang-format --version
xmake format --dry-run

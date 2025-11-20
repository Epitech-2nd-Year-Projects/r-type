#!/bin/bash
set -euo pipefail

if [[ -n "${LLVM_BINDIR:-}" ]]; then
  export PATH="${LLVM_BINDIR}:${PATH}"
fi

echo "clang-format available at: $(command -v clang-format)"
clang-format --version

if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" ]]; then
  sudo apt-get update
  sudo apt-get install -y --no-install-recommends \
    libgl1-mesa-dev \
    libglu1-mesa-dev
fi

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -y -p windows -a x64 -m release
fi

xmake -y format --dry-run

#!/bin/bash
set -euo pipefail

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -p windows -a x64 -m release --toolchain=clang-cl --ar=llvm-ar --yes
fi

xmake build --yes

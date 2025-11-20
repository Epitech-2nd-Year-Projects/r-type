#!/bin/bash
set -euo pipefail

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -y -p windows -a x64 -m release --toolchain=clang --ar=llvm-ar
fi

xmake -y

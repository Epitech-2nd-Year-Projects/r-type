#!/bin/bash
set -euo pipefail

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -p windows -a x64 -m release --toolchain=clang
fi

xmake test --yes -v

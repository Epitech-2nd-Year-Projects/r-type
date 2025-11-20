#!/bin/bash
set -euo pipefail

if [[ "${CI:-}" == "true" && "$(uname -s)" == "Linux" ]]; then
  sudo apt-get update
  sudo apt-get install -y --no-install-recommends \
    libgl1-mesa-dev \
    libglu1-mesa-dev
fi

if [[ "${OS:-}" == "Windows_NT" ]]; then
  xmake f -y -p windows -a x64 -m release
fi

xmake -y test

#!/usr/bin/env bash
set -euo pipefail

BUILD_TYPE="${1:-Release}"

echo "==> Configuring Md-101 (${BUILD_TYPE}) by LU5 (https://lu5.art.br) on Linux..."
cmake -B build -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

echo "==> Compiling Md-101 VST3 & Standalone..."
cmake --build build --parallel "$(nproc)"

echo "==> Build succeeded! (Md-101 by LU5 - https://lu5.art.br)"
echo "    VST3 Plugin : build/Md101_artefacts/${BUILD_TYPE}/VST3/Md-101.vst3"
echo "    Standalone  : build/Md101_artefacts/${BUILD_TYPE}/Standalone/Md-101"
echo "    Install VST3: mkdir -p ~/.vst3 && cp -r build/Md101_artefacts/${BUILD_TYPE}/VST3/Md-101.vst3 ~/.vst3/"

#!/usr/bin/env bash
set -euo pipefail

# NXDev Build Script
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "==> Configuring NXDev in ${BUILD_DIR}..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

echo "==> Building NXDev targets..."
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"

echo "==> Running NXDev test suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> Build and tests completed successfully!"

#!/usr/bin/env bash
set -euo pipefail

# NXDev Build Script
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "==> Configuring NXDev in ${BUILD_DIR}..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

# Calculate safe parallelism respecting host/WSL memory
MEM_AVAIL_KB=$(grep MemAvailable /proc/meminfo 2>/dev/null | awk '{print $2}' || echo "4194304")
CPUS=$(nproc 2>/dev/null || echo 2)
MEM_JOBS=$(( MEM_AVAIL_KB / 1572864 ))
[[ ${MEM_JOBS} -lt 1 ]] && MEM_JOBS=1
SAFE_JOBS=${CPUS}
[[ ${MEM_JOBS} -lt ${SAFE_JOBS} ]] && SAFE_JOBS=${MEM_JOBS}
[[ ${SAFE_JOBS} -lt 1 ]] && SAFE_JOBS=1

echo "==> Building NXDev targets (including host tools and hacbrewpack, jobs=${SAFE_JOBS})..."
cmake --build "${BUILD_DIR}" --parallel "${SAFE_JOBS}"

echo "==> Running NXDev test suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> Build and tests completed successfully!"

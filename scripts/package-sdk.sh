#!/usr/bin/env bash
set -euo pipefail

# NXDevSDK Packaging Script
# Generates NXDevSDK.zip containing install.sh and SDK.zip

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_DIR="${1:-${REPO_ROOT}/dist/sdk}"

echo "=== Building NXDevSDK Distribution Archive ==="
echo "Repository Root: ${REPO_ROOT}"
echo "Output Directory: ${OUTPUT_DIR}"

mkdir -p "${OUTPUT_DIR}"

# Use nxdev sdk package command
if [[ -x "${REPO_ROOT}/build/bin/nxdev" ]]; then
    "${REPO_ROOT}/build/bin/nxdev" sdk package --output "${OUTPUT_DIR}"
elif [[ -x "${REPO_ROOT}/build/cli/nxdev" ]]; then
    "${REPO_ROOT}/build/cli/nxdev" sdk package --output "${OUTPUT_DIR}"
elif command -v nxdev >/dev/null 2>&1; then
    nxdev sdk package --output "${OUTPUT_DIR}"
else
    echo "Building host CLI first..."
    cmake -S "${REPO_ROOT}" -B "${REPO_ROOT}/build" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${REPO_ROOT}/build" --target nxdev -j"$(nproc)"
    if [[ -x "${REPO_ROOT}/build/bin/nxdev" ]]; then
        "${REPO_ROOT}/build/bin/nxdev" sdk package --output "${OUTPUT_DIR}"
    else
        "${REPO_ROOT}/build/cli/nxdev" sdk package --output "${OUTPUT_DIR}"
    fi
fi

cd "${OUTPUT_DIR}"
if [[ -f "NXDevSDK.zip" ]]; then
    sha256sum NXDevSDK.zip > NXDevSDK.zip.sha256
    echo "✓ Generated SHA256 checksum: $(cat NXDevSDK.zip.sha256)"
fi

echo "✓ NXDevSDK Packaging Complete!"

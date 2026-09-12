#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if ! command -v clang-format &> /dev/null; then
    echo "clang-format not found. Please install clang-format to run code formatting."
    exit 1
fi

echo "==> Formatting C/C++ source and header files..."
find "${ROOT_DIR}/sdk" "${ROOT_DIR}/cli" "${ROOT_DIR}/pack" "${ROOT_DIR}/manifest" "${ROOT_DIR}/examples" "${ROOT_DIR}/tests" \
    -type f \( -name "*.hpp" -o -name "*.cpp" -o -name "*.h" -o -name "*.c" \) \
    -exec clang-format -i {} +

echo "==> Done formatting."

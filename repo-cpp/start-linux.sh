#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build}"
TARGET="partycje_pudelka_cpp"

if ! command -v cmake >/dev/null 2>&1; then
  echo "[ERROR] Nie znaleziono cmake."
  exit 1
fi

echo "[INFO] Konfiguracja CMake..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR"

echo "[INFO] Budowanie aplikacji..."
cmake --build "$BUILD_DIR" -j

APP_PATH="$BUILD_DIR/$TARGET"
if [ ! -x "$APP_PATH" ]; then
  echo "[ERROR] Nie znaleziono pliku wykonywalnego: $APP_PATH"
  exit 1
fi

echo "[INFO] Uruchamianie: $APP_PATH"
exec "$APP_PATH"

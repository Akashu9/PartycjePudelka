#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-8080}"

if ! [[ "$PORT" =~ ^[0-9]+$ ]]; then
  echo "[ERROR] Port musi byc liczba calkowita (np. 8080)."
  echo "Uzycie: ./start-linux.sh [port]"
  exit 1
fi

if (( PORT < 1 || PORT > 65535 )); then
  echo "[ERROR] Port musi byc w zakresie 1-65535."
  exit 1
fi

if command -v python3 >/dev/null 2>&1; then
  PYTHON_CMD="python3"
elif command -v python >/dev/null 2>&1; then
  PYTHON_CMD="python"
else
  echo "[ERROR] Nie znaleziono interpretera Python (python3/python)."
  exit 1
fi

cd "$SCRIPT_DIR"

echo "Uruchamiam serwer w: $SCRIPT_DIR"
echo "Adres: http://localhost:$PORT"

auto_open="${AUTO_OPEN_BROWSER:-1}"
if [[ "$auto_open" == "1" ]] && command -v xdg-open >/dev/null 2>&1; then
  (sleep 1; xdg-open "http://localhost:$PORT" >/dev/null 2>&1 || true) &
fi

exec "$PYTHON_CMD" -m http.server "$PORT"

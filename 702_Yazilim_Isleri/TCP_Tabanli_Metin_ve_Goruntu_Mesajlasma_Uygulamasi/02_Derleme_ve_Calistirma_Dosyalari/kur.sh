#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export COLORTERM=truecolor

missing=0
for tool in g++ make tmux chafa ss timeout; do
    command -v "$tool" >/dev/null 2>&1 || missing=1
done
if [ "$missing" -eq 1 ]; then
    echo "[i] Derleme ve terminal araçları kuruluyor..."
    admin=()
    if [ "$(id -u)" -ne 0 ]; then admin=(sudo); fi
    "${admin[@]}" apt-get update
    "${admin[@]}" apt-get install -y build-essential tmux chafa iproute2 coreutils
fi
for tool in g++ make tmux chafa ss timeout; do
    command -v "$tool" >/dev/null 2>&1 || { echo "[X] Eksik araç: $tool" >&2; exit 1; }
done
echo "[✓] Gerekli araçlar hazır."

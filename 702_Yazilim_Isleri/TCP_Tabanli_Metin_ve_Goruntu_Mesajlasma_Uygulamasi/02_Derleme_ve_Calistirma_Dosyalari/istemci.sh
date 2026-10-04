#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export COLORTERM=truecolor

for attempt in $(seq 1 80); do
    if ss -H -ltn 'sport = :5000' | grep -q .; then
        exec "$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/client"
    fi
    sleep 0.1
done
echo "[X] Sunucu 5000/TCP portunda başlamadı." >&2
exit 1

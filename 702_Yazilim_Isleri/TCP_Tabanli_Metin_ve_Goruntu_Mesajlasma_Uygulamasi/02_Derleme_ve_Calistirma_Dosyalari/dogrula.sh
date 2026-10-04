#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export COLORTERM=truecolor

bash "$SCRIPT_DIR/kur.sh"
for script in "$SCRIPT_DIR"/*.sh; do bash -n "$script"; done
make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR" CXXFLAGS='-Wall -Wextra -Wpedantic -Werror -std=c++17 -pthread'
python3 "$SCRIPT_DIR/../03_Test_ve_Dogrulama_Calismalari/dogrula.py"

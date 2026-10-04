#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "[1/3] Sunucu ve istemci temiz derleniyor..."
make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR"

echo "[2/3] C++ protokol testleri derleniyor..."
g++ -Wall -Wextra -Wpedantic -Werror -std=c++17 -pthread \
    -I"$BASE_DIR/01_Kaynak_Kod_Dosyalari" \
    "$BASE_DIR/03_Test_ve_Dogrulama_Calismalari/protocol_test.cpp" \
    "$BASE_DIR/01_Kaynak_Kod_Dosyalari/protocol.cpp" \
    -o "$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/protocol_test"

echo "[3/3] C++ dogrulama calistiriliyor..."
"$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/protocol_test"

echo "[✓] Tum C++ testleri ve dogrulama basariyla tamamlandi."
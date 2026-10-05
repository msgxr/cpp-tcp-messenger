#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "[1/4] Sunucu ve istemci temiz derleniyor..."
make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR" CXXFLAGS="-Wall -Wextra -Wpedantic -Werror -std=c++17 -pthread"

echo "[2/4] C++ protokol testleri derleniyor..."
g++ -Wall -Wextra -Wpedantic -Werror -std=c++17 -pthread \
    -I"$BASE_DIR/01_Kaynak_Kod_Dosyalari" \
    "$BASE_DIR/03_Test_ve_Dogrulama_Calismalari/protocol_test.cpp" \
    "$BASE_DIR/01_Kaynak_Kod_Dosyalari/protocol.cpp" \
    -o "$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/protocol_test"

echo "[3/4] C++ protokol testi çalıştırılıyor..."
"$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/protocol_test"

echo "[4/4] C++ çoklu istemci entegrasyon testi derlenip çalıştırılıyor..."
g++ -Wall -Wextra -Wpedantic -Werror -std=c++17 -pthread \
    -I"$BASE_DIR/01_Kaynak_Kod_Dosyalari" \
    "$BASE_DIR/03_Test_ve_Dogrulama_Calismalari/integration_test.cpp" \
    "$BASE_DIR/01_Kaynak_Kod_Dosyalari/protocol.cpp" \
    -o "$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/integration_test"
"$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/integration_test" \
    "$SCRIPT_DIR/Derlenmis_Uygulama_Dosyalari/server"

echo "[✓] Tüm testler ve doğrulama başarıyla tamamlandı."

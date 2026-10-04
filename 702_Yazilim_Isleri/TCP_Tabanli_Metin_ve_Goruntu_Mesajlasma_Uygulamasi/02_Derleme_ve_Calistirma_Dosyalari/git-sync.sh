#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

if [ ! -r README.md ] || [ ! -r .gitignore ]; then
    echo "[X] README.md veya .gitignore disk üzerinde okunamıyor; önce dosyaları kaydedin." >&2
    exit 1
fi

git update-index --refresh
git diff --check
if [ -z "$(git status --porcelain)" ]; then
    echo "[i] Kaydedilmiş değişiklik yok. Editördeki dosyayı önce Ctrl+S ile kaydedin." >&2
    exit 1
fi

# WSL içindeki Linux GCM ile Windows GCM'nin aynı anda devreye girmesini önle.
windows_gcm="/mnt/c/Program Files/Git/mingw64/bin/git-credential-manager.exe"
if [ -x "$windows_gcm" ]; then
    git config --local --unset-all credential.helper 2>/dev/null || true
    git config --local credential.helper "$windows_gcm"
fi

message="${1:-WSL senkronizasyonu}"
git add -A
git commit -m "$message" -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
git push origin HEAD

#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

need_install=0
command -v chafa >/dev/null 2>&1 || need_install=1
command -v tmux >/dev/null 2>&1 || need_install=1
if [ "$need_install" -eq 1 ]; then
  echo "[i] Gerekli terminal araçları kuruluyor; sudo parolası istenebilir..."
  sudo apt update
  sudo apt install -y chafa tmux
fi

mkdir -p "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar"
make -C "$SCRIPT_DIR" clean
make -C "$SCRIPT_DIR"

tmux kill-session -t tcpmsg 2>/dev/null || true
tmux new-session -d -s tcpmsg -n mesajlasma "$SCRIPT_DIR/sunucu.sh"
tmux split-window -h -t tcpmsg:mesajlasma "$SCRIPT_DIR/istemci.sh"
tmux select-layout -t tcpmsg:mesajlasma even-horizontal
tmux set-option -t tcpmsg mouse on
tmux set-option -t tcpmsg status-style "bg=default,fg=cyan"
tmux set-option -t tcpmsg status-left "[ TCP MESSENGER ] "
tmux attach -t tcpmsg

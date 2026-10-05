#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export COLORTERM=truecolor

bash "$SCRIPT_DIR/kur.sh"
make -C "$SCRIPT_DIR"
mkdir -p "702_Yazilim_Isleri/TCP_Tabanli_Metin_ve_Goruntu_Mesajlasma_Uygulamasi/04_Uygulama_Ciktilari/Alinan_Dosyalar"
session="${TCPMSG_SESSION:-tcpmsg}"
tmux_cmd=(tmux -u)
if [ -n "${TCPMSG_SOCKET:-}" ]; then tmux_cmd+=(-L "$TCPMSG_SOCKET"); fi
"${tmux_cmd[@]}" kill-session -t "$session" 2>/dev/null || true
for attempt in $(seq 1 40); do
    if ! ss -H -ltn 'sport = :5000' | grep -q .; then break; fi
    sleep 0.1
done
if ss -H -ltn 'sport = :5000' | grep -q .; then
    echo "[X] 5000/TCP portu dolu. Açık sunucuyu Ctrl+C ile kapatın." >&2
    ss -ltnp 'sport = :5000' >&2
    exit 1
fi
server_pane=$("${tmux_cmd[@]}" new-session -d -s "$session" -n mesajlasma -c "$PROJECT_ROOT" -P -F '#{pane_id}' bash "$SCRIPT_DIR/sunucu.sh")
client_pane=$("${tmux_cmd[@]}" split-window -h -t "$server_pane" -c "$PROJECT_ROOT" -P -F '#{pane_id}' bash "$SCRIPT_DIR/istemci.sh")
"${tmux_cmd[@]}" split-window -v -t "$client_pane" -c "$PROJECT_ROOT" bash "$SCRIPT_DIR/istemci.sh"
"${tmux_cmd[@]}" select-layout -t "$server_pane" tiled >/dev/null
"${tmux_cmd[@]}" set-option -t "$session" mouse on
"${tmux_cmd[@]}" set-option -t "$session" status-style 'bg=default,fg=cyan'
"${tmux_cmd[@]}" set-option -t "$session" status-left '[ TCP MESSENGER ] '
"${tmux_cmd[@]}" select-pane -t "$client_pane"
if [ "${1:-}" = '--ayrik' ]; then
    echo "[✓] Sunucu ve iki istemci başlatıldı: $session"
else
    "${tmux_cmd[@]}" attach-session -t "$session"
fi

#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"
export LANG=C.UTF-8
export LC_ALL=C.UTF-8
export COLORTERM=truecolor

client_count=5
detached=0
for argument in "$@"; do
    if [ "$argument" = "--ayrik" ]; then
        detached=1
    elif [[ "$argument" =~ ^[2-5]$ ]]; then
        client_count="$argument"
    else
        echo "[X] Kullanım: calistir.sh [2-5] [--ayrik]" >&2
        exit 1
    fi
done

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
for attempt in $(seq 1 40); do
    if ss -H -ltn 'sport = :5000' | grep -q .; then break; fi
    sleep 0.1
done
client_panes=()
for number in $(seq 1 "$client_count"); do
    pane=$("${tmux_cmd[@]}" split-window -h -t "$server_pane" -c "$PROJECT_ROOT" -P -F '#{pane_id}' env TCPMSG_EXPECTED_CLIENTS="$client_count" bash "$SCRIPT_DIR/istemci.sh")
    client_panes+=("$pane")
done
"${tmux_cmd[@]}" select-pane -t "$server_pane"
if [ "$client_count" -le 3 ]; then
    "${tmux_cmd[@]}" set-window-option -t "$session" main-pane-height 9
    "${tmux_cmd[@]}" select-layout -t "$session" main-horizontal >/dev/null
else
    "${tmux_cmd[@]}" select-layout -t "$session" tiled >/dev/null
fi
"${tmux_cmd[@]}" select-pane -t "$server_pane" -T 'SUNUCU (BAĞLANTILAR)'
for index in "${!client_panes[@]}"; do
    number=$((index + 1))
    "${tmux_cmd[@]}" select-pane -t "${client_panes[$index]}" -T "İSTEMCİ $number - BURAYA MESAJ YAZIN"
done
"${tmux_cmd[@]}" set-option -t "$session" mouse on
"${tmux_cmd[@]}" set-option -t "$session" pane-border-status top
"${tmux_cmd[@]}" set-option -t "$session" pane-border-format '#[fg=cyan,bold] #{pane_title} #[default]'
"${tmux_cmd[@]}" set-option -t "$session" status off
"${tmux_cmd[@]}" select-pane -t "${client_panes[0]}"
if [ "$detached" -eq 1 ]; then
    echo "[✓] Sunucu ve $client_count istemci başlatıldı: $session"
else
    "${tmux_cmd[@]}" attach-session -t "$session"
fi

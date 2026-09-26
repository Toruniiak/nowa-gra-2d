#!/usr/bin/env bash
# Serwer gry uruchamiany na Windowsie przez WSL (wywołuje go graj.bat).
# Serwer używa gniazd POSIX, więc nie działa bezpośrednio na Windowsie.
#   serwer_wsl.sh build        — biblioteki (za 1. razem), kompilacja, certyfikat
#   serwer_wsl.sh run [port]   — uruchamia serwer (okno z logiem)
#   serwer_wsl.sh stop         — łagodnie zatrzymuje (zapis pozycji graczy)
# Build, baza i log leżą w ~/.nowa-gra-2d po stronie Linuksa: szybciej niż na
# dysku C:, a SQLite na dyskach Windowsa widzianych z WSL miewa problemy z
# blokadami pliku.
set -euo pipefail

MODE="${1:-}"
PORT="${2:-7777}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
STATE="$HOME/.nowa-gra-2d"
PIDFILE="$STATE/server.pid"
mkdir -p "$STATE"

case "$MODE" in
  build)
    need=()
    for c in cmake g++ pkg-config openssl; do command -v "$c" >/dev/null || need+=("$c"); done
    pkg-config --exists openssl 2>/dev/null || need+=(libssl-dev)
    pkg-config --exists sqlite3 2>/dev/null || need+=(libsqlite3-dev)
    [ -f /usr/include/nlohmann/json.hpp ] || need+=(nlohmann-json3-dev)
    if [ ${#need[@]} -gt 0 ]; then
      echo "=== Doinstalowuję w WSL: ${need[*]} (może zapytać o hasło do Linuksa)"
      sudo apt-get update && sudo apt-get install -y "${need[@]}"
    fi
    echo "=== Buduję serwer..."
    cmake -S "$REPO/server" -B "$STATE/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
    cmake --build "$STATE/build" -j >/dev/null
    if [ ! -f "$REPO/server/certs/server.crt" ] || [ ! -f "$REPO/server/certs/server.key" ]; then
      echo "=== Tworzę lokalny certyfikat szyfrowania..."
      mkdir -p "$REPO/server/certs"
      openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -subj "/CN=localhost" \
        -keyout "$REPO/server/certs/server.key" -out "$REPO/server/certs/server.crt" >/dev/null 2>&1
    fi
    ;;
  run)
    cd "$REPO/server"
    "$STATE/build/server" "$PORT" "$STATE/game.db" certs/server.crt certs/server.key ../client/data \
      > "$STATE/server.log" 2>&1 &
    SP=$!
    echo "$SP" > "$PIDFILE"
    trap 'kill -TERM "$SP" 2>/dev/null; wait "$SP" 2>/dev/null; rm -f "$PIDFILE"' EXIT HUP INT TERM
    echo "Serwer gry (port $PORT). To okno zamknie się samo po wyjściu z gry."
    tail -n +1 -f "$STATE/server.log" --pid="$SP"
    ;;
  stop)
    if [ -f "$PIDFILE" ]; then
      P="$(cat "$PIDFILE")"
      if ps -p "$P" -o command= 2>/dev/null | grep -q "build/server"; then
        kill -TERM "$P" 2>/dev/null || true
        for _ in $(seq 1 50); do kill -0 "$P" 2>/dev/null || break; sleep 0.1; done
      fi
      rm -f "$PIDFILE"
    fi
    rm -f "$STATE/server.log"  # so 'ready' can't see a previous run's log
    ;;
  ready)
    grep -q "Server listening" "$STATE/server.log" 2>/dev/null
    ;;
  *)
    echo "Użycie: $0 build | run [port] | stop | ready" >&2
    exit 2
    ;;
esac

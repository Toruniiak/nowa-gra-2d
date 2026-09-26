#!/usr/bin/env bash
# Uruchamia grę do testów — zawsze w NAJNOWSZEJ wersji z GitHuba.
# Linux (Debian/Ubuntu) i macOS. Skopiuj ten plik na Pulpit i uruchamiaj.
#
# Co robi przy każdym uruchomieniu:
#   1. pobiera najnowszy kod (za pierwszym razem klonuje repo),
#   2. buduje serwer (za pierwszym razem doinstalowuje biblioteki),
#   3. uruchamia serwer w tle,
#   4. pobiera Godot 4.3, jeśli go nie ma,
#   5. uruchamia grę; po jej zamknięciu łagodnie zatrzymuje serwer.
#
# Ustawienia (opcjonalne zmienne środowiskowe):
#   NOWA_GRA_DIR   gdzie trzymać grę   (domyślnie ~/nowa-gra-2d)
#   NOWA_GRA_PORT  port serwera        (domyślnie 7777)
set -euo pipefail

REPO_URL="https://github.com/Toruniiak/nowa-gra-2d.git"
GAME_DIR="${NOWA_GRA_DIR:-$HOME/nowa-gra-2d}"
PORT="${NOWA_GRA_PORT:-7777}"
TOOLS_DIR="$HOME/.nowa-gra-2d-tools"
GODOT_VER="4.3-stable"

say() { printf '\n\033[1;32m==> %s\033[0m\n' "$*"; }
die() { printf '\n\033[1;31mBŁĄD: %s\033[0m\n' "$*" >&2; read -r -p "Naciśnij Enter, aby zamknąć..." _ || true; exit 1; }

OS="$(uname -s)"

# ---------------------------------------------------------------- 1. kod
command -v git >/dev/null || die "Brak programu git. Ubuntu: sudo apt install git   macOS: xcode-select --install"
if [ -d "$GAME_DIR/.git" ]; then
  say "Pobieram najnowszą wersję gry..."
  if ! git -C "$GAME_DIR" pull --ff-only; then
    echo "Nie udało się pobrać aktualizacji (brak internetu albo lokalne zmiany) — uruchamiam wersję, która już jest."
  fi
else
  say "Pierwsze uruchomienie: pobieram grę do $GAME_DIR (GitHub może poprosić o zalogowanie)..."
  git clone "$REPO_URL" "$GAME_DIR" || die "Nie udało się sklonować repozytorium."
fi
cd "$GAME_DIR"
echo "Wersja: $(git log -1 --format='%h %ad %s' --date=format:'%Y-%m-%d %H:%M')"

# ---------------------------------------------------------------- 2. biblioteki + build serwera
need_pkgs=()
command -v cmake >/dev/null || need_pkgs+=(cmake)
command -v pkg-config >/dev/null || need_pkgs+=(pkg-config)
command -v openssl >/dev/null || need_pkgs+=(openssl)
if [ "$OS" = "Linux" ]; then
  command -v g++ >/dev/null || need_pkgs+=(g++)
  pkg-config --exists openssl 2>/dev/null || need_pkgs+=(libssl-dev)
  pkg-config --exists sqlite3 2>/dev/null || need_pkgs+=(libsqlite3-dev)
  [ -f /usr/include/nlohmann/json.hpp ] || need_pkgs+=(nlohmann-json3-dev)
  if [ ${#need_pkgs[@]} -gt 0 ]; then
    say "Doinstalowuję potrzebne biblioteki: ${need_pkgs[*]} (może zapytać o hasło)"
    sudo apt-get update && sudo apt-get install -y "${need_pkgs[@]}" || die "Instalacja bibliotek nie powiodła się."
  fi
  CMAKE_EXTRA=()
elif [ "$OS" = "Darwin" ]; then
  command -v brew >/dev/null || die "Brak Homebrew — zainstaluj ze strony https://brew.sh i uruchom ponownie."
  for f in cmake pkg-config openssl@3 sqlite nlohmann-json; do
    brew list "$f" >/dev/null 2>&1 || need_pkgs+=("$f")
  done
  if [ ${#need_pkgs[@]} -gt 0 ]; then
    say "Doinstalowuję: ${need_pkgs[*]}"
    brew install "${need_pkgs[@]}" || die "Instalacja przez Homebrew nie powiodła się."
  fi
  CMAKE_EXTRA=(-DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)")
  export PKG_CONFIG_PATH="$(brew --prefix sqlite)/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
else
  die "Nieobsługiwany system: $OS (na Windowsie użyj graj.bat)."
fi

say "Buduję serwer..."
cmake -S server -B server/build -DCMAKE_BUILD_TYPE=Release "${CMAKE_EXTRA[@]}" >/dev/null || die "cmake nie skonfigurował serwera."
cmake --build server/build -j >/dev/null || die "Kompilacja serwera nie powiodła się (szczegóły: cmake --build server/build)."

# ---------------------------------------------------------------- certyfikat TLS (lokalny, testowy)
if [ ! -f server/certs/server.crt ] || [ ! -f server/certs/server.key ]; then
  say "Tworzę lokalny certyfikat szyfrowania (tylko na ten komputer)..."
  mkdir -p server/certs
  openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -subj "/CN=localhost" \
    -keyout server/certs/server.key -out server/certs/server.crt >/dev/null 2>&1 || die "Nie udało się utworzyć certyfikatu."
fi
mkdir -p client/certs
cp server/certs/server.crt client/certs/dev_server.crt

# ---------------------------------------------------------------- Godot
GODOT=""
if command -v godot4 >/dev/null && godot4 --version 2>/dev/null | grep -q '^4\.3\.'; then
  GODOT="$(command -v godot4)"
fi
if [ -z "$GODOT" ]; then
  mkdir -p "$TOOLS_DIR"
  if [ "$OS" = "Linux" ]; then
    GODOT="$TOOLS_DIR/Godot_v${GODOT_VER}_linux.x86_64"
    ZIP="Godot_v${GODOT_VER}_linux.x86_64.zip"
  else
    GODOT="$TOOLS_DIR/Godot.app/Contents/MacOS/Godot"
    ZIP="Godot_v${GODOT_VER}_macos.universal.zip"
  fi
  if [ ! -x "$GODOT" ]; then
    say "Pobieram Godot 4.3 (jednorazowo, ok. 50-150 MB)..."
    curl -fL --progress-bar -o "$TOOLS_DIR/$ZIP" \
      "https://github.com/godotengine/godot/releases/download/${GODOT_VER}/$ZIP" || die "Nie udało się pobrać Godota."
    (cd "$TOOLS_DIR" && unzip -q -o "$ZIP" && rm -f "$ZIP")
    chmod +x "$GODOT" 2>/dev/null || true
  fi
fi

# ---------------------------------------------------------------- 3. serwer
PIDFILE="$GAME_DIR/server/server.pid"
if [ -f "$PIDFILE" ]; then
  OLD_PID="$(cat "$PIDFILE")"
  # Only if that PID is still OUR server — after a crash/reboot the number
  # may belong to an unrelated program by now.
  if ps -p "$OLD_PID" -o command= 2>/dev/null | grep -q "build/server"; then
    say "Zatrzymuję serwer z poprzedniego uruchomienia..."
    kill -TERM "$OLD_PID" 2>/dev/null || true
    sleep 1
  fi
  rm -f "$PIDFILE"
fi
say "Uruchamiam serwer na porcie $PORT (log: server/server.log)..."
(cd server && exec ./build/server "$PORT" game.db certs/server.crt certs/server.key ../client/data) \
  > server/server.log 2>&1 &
SERVER_PID=$!
echo "$SERVER_PID" > "$PIDFILE"
stop_server() {
  if kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Zatrzymuję serwer (zapisuje pozycje graczy)..."
    kill -TERM "$SERVER_PID" 2>/dev/null || true
    wait "$SERVER_PID" 2>/dev/null || true
  fi
  rm -f "$PIDFILE"
}
trap stop_server EXIT INT TERM

for _ in $(seq 1 50); do
  grep -q "Server listening" server/server.log 2>/dev/null && break
  kill -0 "$SERVER_PID" 2>/dev/null || die "Serwer się nie uruchomił:\n$(tail -5 server/server.log)"
  sleep 0.2
done
grep -q "Server listening" server/server.log || die "Serwer nie odpowiada:\n$(tail -5 server/server.log)"

# ---------------------------------------------------------------- 4. gra
say "Przygotowuję grafikę gry..."
"$GODOT" --headless --path client --import >/dev/null 2>&1 || true
say "Uruchamiam grę. Zamknij okno gry, aby zakończyć."
"$GODOT" --path client --resolution 450x800 -- --server-port="$PORT"

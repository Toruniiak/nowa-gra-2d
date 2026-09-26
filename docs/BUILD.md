# BUILD.md

## Serwer (C++)

Wymagania: kompilator C++20 (g++/clang++), CMake ≥ 3.20, pkg-config,
**OpenSSL 3** i **SQLite 3** (Ubuntu/Debian: `apt install libssl-dev
libsqlite3-dev pkg-config`) — uzasadnienie w TECH_STACK.md.

```bash
cd server
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # albo Release
cmake --build build                            # musi przejść bez ostrzeżeń
```

Certyfikat TLS dla serwera dev (self-signed, CN=localhost). **Nigdy nie
commituj** — `server/certs/` jest w `.gitignore`:

```bash
mkdir -p server/certs
openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -subj "/CN=localhost" \
  -keyout server/certs/server.key -out server/certs/server.crt
```

Uruchomienie (z katalogu `server/`, domyślne ścieżki są względne):

```bash
./build/server [port] [db_path] [cert_path] [key_path] [data_dir]
# domyślnie: 7777 game.db certs/server.crt certs/server.key ../client/data
```

Serwer wczytuje mapę z `data_dir` (`tiles.json` + `maps/start.json`) — tę
samą, którą rysuje klient; błędna mapa = serwer nie startuje (z komunikatem).
Zależność: `apt install nlohmann-json3-dev`.

Baza SQLite tworzy się sama przy pierwszym starcie. `Ctrl+C`/`SIGTERM` =
łagodne zamknięcie: serwer zapisuje pozycje wszystkich graczy online.

## Klient (Godot)

Wymaga Godot 4.3. Klient **zawsze weryfikuje** certyfikat serwera — dla
serwera dev skopiuj jego publiczny certyfikat do klienta (trafia też do APK;
`client/certs/` jest w `.gitignore`, bo każdy generuje własny):

```bash
mkdir -p client/certs && cp server/certs/server.crt client/certs/dev_server.crt
```

```bash
godot4 --path client --editor                 # edytor (wymaga GUI)
godot4 --headless --path client --import      # walidacja bez GUI
godot4 --headless --path client --check-only --script res://scripts/world.gd   # parse-check skryptu
```

Argumenty dev (po `--`, patrz `client/scripts/world.gd`): `--server-host=`,
`--server-port=`, `--tls-cert=<plik PEM>`, `--tls-cn=<oczekiwana nazwa>`,
oraz automatyczne logowanie do testów: `--user= --password= --character=
[--register]`. `--password=` jest widoczne dla innych procesów — tylko dev.

Na telefonie: wpisz adres PC w sieci lokalnej (np. `192.168.1.10:7777`) w
polu adresu na ekranie logowania i naciśnij "Połącz ponownie". Certyfikat
dev ma CN=localhost i klient sprawdza właśnie tę nazwę — to działa także
przy łączeniu po IP.

## Testy end-to-end

Serwer + protokół (34 asercje: konta, postacie, anty-cheat, persystencja,
bezpieczeństwo — patrz nagłówek skryptu):

```bash
cd server && ./build/server 7788 test.db certs/server.crt certs/server.key &
cd .. && python3 tools/test_client.py 7788 server/test.db
```

Prawdziwy klient Godot przeciw serwerowi (headless, auto-logowanie):

```bash
godot4 --headless --path client --max-fps 60 --quit-after 240 -- \
  --server-port=7788 --register --user=test_user --password="haslo123" --character="Bohater"
# oczekiwane: "Entered world as entity N" i "Local entity N spawned at (x, y)"
```

Chodzenie w prawdziwym kliencie (trzyma "w prawo", sprawdza kroki
potwierdzone przez serwer i płynność animacji): `tools/godot_walk_test.gd`.

Grafika: `python3 tools/art/gen_tileset.py` odtwarza wszystkie arkusze w
`client/assets/`.

Ścieżka "zły adres → wpisanie poprawnego → Połącz ponownie" na prawdziwej
scenie: `tools/godot_reconnect_test.gd` (instrukcja w nagłówku pliku).

Zrzut ekranu prawdziwego klienta bez monitora (Xvfb + programowy OpenGL):
`tools/godot_screenshot.gd` (instrukcja w nagłówku pliku).

Uwaga dla skryptów testowych w Pythonie: czytaj gniazdo TLS nieblokująco
(`select`), nie blokującym `recv()` z timeoutem — przy TLS 1.3 potrafi on
przespać cały timeout mimo że odpowiedź już przyszła (patrz komentarz w
`tools/test_client.py`).

## Android (`.apk`)

Zweryfikowane end-to-end: 2026-09-26. Wymaga Android SDK (cmdline-tools,
`platform-tools`, `build-tools;34.0.0`, `platforms;android-34`), Godot 4.3
(edytor + export templates) i **JDK 17** (nie 21 — Godot 4.3 wymaga Java 17
dla builda Gradle, inaczej `Invalid Java version 21` przy `build.gradle`).

Instalacja SDK (headless, `sdkmanager` z cmdline-tools):

```bash
sdkmanager --sdk_root=$ANDROID_SDK_ROOT --licenses
sdkmanager --sdk_root=$ANDROID_SDK_ROOT "platform-tools" "build-tools;34.0.0" "platforms;android-34"
```

Godot 4.3 (edytor headless + export templates) — pobierane z GitHub Releases
(`godotengine/godot`, tag `4.3-stable`), nie z tuxfamily (stara ścieżka
`downloads.tuxfamily.org/godotengine/4.3/...` zwraca 404 dla tej wersji).
Export templates rozpakować do
`~/.local/share/godot/export_templates/4.3.stable/`.

Wymagane ustawienie projektu (bez tego `has_valid_project_configuration`
zwraca `valid=false` z **pustym** komunikatem błędu — realny, potwierdzony
brak komunikatu w Godot 4.3 dla tego konkretnego przypadku, patrz
`editor/import/resource_importer_texture_settings.cpp`,
`should_import_etc2_astc()`):

```ini
; client/project.godot, [rendering]
textures/vram_compression/import_etc2_astc=true
```

`godot4 --headless ... --install-android-build-template` **wisi bez końca**
w tym środowisku (headless, brak GUI) — używa wewnętrznie `ProgressDialog`,
który nigdy nie sygnalizuje zakończenia bez działającego display servera.
Obejście: odtworzyć ręcznie to, co robi silnik (identyczne z
`ExportTemplateManager::install_android_template_from_file`, patrz
`editor/export/export_template_manager.cpp`):

```bash
mkdir -p client/android/build
echo "4.3.stable" > client/android/.build_version   # musi zgadzać się z VERSION_FULL_CONFIG
: > client/android/build/.gdignore
cd client/android/build && unzip -q .../export_templates/4.3.stable/android_source.zip
```

Eksport (preset "Android" musi istnieć w `client/export_presets.cfg`,
`gradle_build/use_gradle_build=true` — legacy prebuilt-APK build bez Gradle
nie działa w 4.3, zwraca ten sam "pusty" błąd konfiguracji):

```bash
export ANDROID_HOME=/opt/android-sdk   # ścieżka SDK musi być też w Editor Settings
godot4 --headless --path client --export-debug "Android" client/builds/android/nowa-gra-2d-debug.apk
```

Preset ma `include_filter="certs/*.crt"` — przypięty certyfikat dev
(`client/certs/dev_server.crt`, patrz sekcja Klient) trafia do APK jako
`assets/certs/dev_server.crt`. Bez niego APK połączy się tylko z serwerem
z certyfikatem od publicznego CA.

Weryfikacja zawartości `.apk` (manifest, podpis, integralność archiwum —
patrz PROJECT_STATE.md dla pełnego wyniku):

```bash
$ANDROID_HOME/build-tools/34.0.0/aapt dump badging plik.apk
$ANDROID_HOME/build-tools/34.0.0/apksigner verify --verbose plik.apk
unzip -t plik.apk
```

**Nieukończone:** instalacja na fizycznym urządzeniu/emulatorze (ten
kontener nie ma `/dev/kvm` ani GUI — patrz PROJECT_STATE.md). Android
SDK/JDK/Godot binary żyją w kontenerze (`/opt/android-sdk`, `/opt/godot`,
`/usr/lib/jvm/java-17-openjdk-amd64`), **nie** w repo — po restarcie
kontenera trzeba je zainstalować od nowa; kroki powyżej są tego pełnym
zapisem.

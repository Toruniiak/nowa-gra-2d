# BUILD.md

## Serwer (C++)

```bash
cd server
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # albo Release
cmake --build build
./build/server [port]     # domyślny port: 7777
```

Wymagania: kompilator C++20 (g++/clang++), CMake ≥ 3.20. Brak zewnętrznych
zależności (patrz TECH_STACK.md) — tylko POSIX sockets.

## Klient (Godot)

Wymaga Godot 4.3 (edytor lub headless binary).

```bash
godot4 --path client --editor        # otwiera edytor (wymaga GUI)
godot4 --headless --path client --import   # walidacja bez GUI (CI-friendly)
godot4 --headless --path client --quit-after 60 -- --server-host=127.0.0.1 --server-port=7777
```

Domyślny host/port: `127.0.0.1:7777` — zmienialny w `World.tscn` (export
vars `server_host`/`server_port`) albo argumentami `--server-host=`/
`--server-port=` (patrz `client/scripts/world.gd`).

## Test end-to-end (bez GUI, weryfikuje protokół)

```bash
./server/build/server 7788 &
python3 tools/test_client.py 7788
```

`tools/test_client.py` to skrypt diagnostyczny (nie część gry) symulujący
klienta — wysyła normalny ruch, próbę "cheatowania" (za duży skok) i
zniekształcone dane, sprawdzając że serwer poprawnie waliduje/nie crashuje.

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

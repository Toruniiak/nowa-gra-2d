# PROJECT_STATE.md

**Ostatnia aktualizacja:** 2026-09-26 (sesja Claude Code)

Ten plik ma pozwolić kontynuować projekt w nowej sesji bez utraty kontekstu.
Czytaj go pierwszy.

## Co działa (zweryfikowane, nie zadeklarowane)

- **Serwer C++** (`server/`): buduje się bez błędów/ostrzeżeń
  (`-Wall -Wextra -Wpedantic`). Nasłuchuje TCP, przyjmuje wielu klientów,
  waliduje ruch (anty-cheat na pojedynczy pakiet — patrz KNOWN_ISSUES.md),
  rozsyła pozycje, obsługuje disconnect.
- **Klient Godot** (`client/`): projekt importuje się bez błędów headless
  (`godot4 --headless --path client --import`, exit 0). Scena `World.tscn`
  łączy się z serwerem, odbiera `WELCOME`, potwierdzone w logu:
  `Connected to server as entity 1`.
- **Test end-to-end:** `tools/test_client.py` — dwóch klientów, normalny
  ruch, próba cheatowania (klamrowana poprawnie: `MOVE 500 0` po `MOVE 3 0`
  dało pozycję `9`, nie `503`), zniekształcone dane nie crashują serwera,
  `LEAVE` rozsyłany po rozłączeniu.

- **Sterowanie padem** (Bluetooth/wired): ruch działa z padem — zweryfikowane
  symulacją zdarzenia `InputEventJoypadMotion`, ta sama ścieżka kodu co
  klawiatura/dotyk (`Input.get_vector`, `client/scripts/world.gd`).
  Wykrywanie connect/disconnect: `client/scripts/gamepad_status.gd`. Nie
  testowane na fizycznym padzie (brak sprzętu w kontenerze).
- **Eksport Android (Phase 2) — pierwszy `.apk` zbudowany i zweryfikowany**
  (2026-09-26): Android SDK (cmdline-tools + `platform-tools` +
  `build-tools;34.0.0` + `platforms;android-34`), Godot 4.3.stable
  (headless editor + export templates z GitHub Releases) i JDK 17
  zainstalowane w kontenerze. Build debug APK przez Gradle (`--export-debug`)
  zakończony exit 0, potwierdzony **dwukrotnie** (build od zera, identyczny
  rozmiar 74 849 033 B — powtarzalny, nie przypadek). Weryfikacja `.apk`:
  `aapt dump badging` (package `com.novagra2d.client`, targetSdk 34,
  minSdk 24, permissions `INTERNET`+`ACCESS_NETWORK_STATE` — zgodne z tym,
  co klient faktycznie potrzebuje, nic więcej), `unzip -t` (integralność
  archiwum OK), `apksigner verify` (podpisany debug-keystore, v2 scheme OK),
  zawartość `assets/` potwierdza spakowanie realnych scen/skryptów klienta
  (`World.tscn`, `PlayerEntity.tscn`, `world.gdc`, `net_client.gdc`, itd.).
  Szczegóły i pełna procedura: `docs/BUILD.md`, sekcja "Android (`.apk`)".
  **Nieukończone:** instalacja/test na fizycznym urządzeniu lub emulatorze —
  ten kontener nie ma `/dev/kvm` ani GUI, fizycznie niemożliwe do
  zweryfikowania tutaj. To wymaga maszyny użytkownika (patrz "Następny krok").

## Co NIE działa / nie istnieje jeszcze

- `.apk` zbudowany, ale **nigdy nie zainstalowany/uruchomiony na realnym
  urządzeniu lub emulatorze** — brak takiej możliwości w tym kontenerze.
- Brak jakiejkolwiek grafiki poza placeholderami (kolorowe prostokąty).
- Brak kont/logowania/bazy danych/persystencji.
- Brak combat, itemów, questów, NPC, ekonomii, PvP, guildii — wszystko od
  Phase 6 wzwyż w ROADMAP.md.

## Środowisko budowania Android — efemeryczne, nie w repo

Android SDK (`/opt/android-sdk`), Godot binary (`/opt/godot/godot4`),
export templates (`~/.local/share/godot/export_templates/4.3.stable/`) i
JDK 17 (`/usr/lib/jvm/java-17-openjdk-amd64`, zainstalowany przez
`apt-get install openjdk-17-jdk-headless` — potrzebny **obok** domyślnego
JDK 21, bo Godot 4.3 wymaga dokładnie Java 17 dla Gradle) żyją **tylko w
tym kontenerze**, nie w repozytorium. Po restarcie/nowej sesji kontener
jest czyszczony — trzeba to zainstalować od nowa. `docs/BUILD.md` ma pełną,
zweryfikowaną procedurę (włącznie z dwoma realnymi obejściami błędów
Godota opisanymi tam szczegółowo: headless `--install-android-build-template`
wisi bez końca, i "pusty" komunikat błędu eksportu przy braku
`import_etc2_astc`). Repo zawiera trwałe efekty tej pracy:
`client/export_presets.cfg` (nowy, wcześniej brakował) i
`client/project.godot` (dodane `textures/vram_compression/import_etc2_astc=true`).

## Sieć — odblokowana

`dl.google.com` (Android SDK), GitHub Releases (Godot binary/templates),
`services.gradle.org`, `maven.google.com`, `repo.maven.apache.org`,
`plugins.gradle.org` — wszystkie dostępne w tej sesji (środowisko
"AZONERA MMORPG 2D", zweryfikowane realnym `curl`, 2026-09-26). Wcześniejsza
blokada `dl.google.com` (opisana w poprzednich wersjach tego pliku) została
usunięta przez użytkownika w ustawieniach Network access środowiska.

## Repozytorium GitHub

**Istnieje i jest aktywnie używane:** `Toruniiak/nowa-gra-2d` (branch
`main`). Kod jest wypychany na bieżąco z tej sesji.

## Decyzje techniczne podjęte (i dlaczego)

- Serwer: C++20, własny, bez frameworka — wybrane przez użytkownika,
  analogicznie do Canary/Azonery (inny, niezwiązany projekt).
- Klient: Godot 4.3, nie "silnik od zera" w C++ — bo napisanie własnego
  renderera 2D + dotykowego UI + eksportu Android w czystym NDK to
  osobny, wielomiesięczny projekt. Serwer (logika, autorytatywność)
  zostaje w 100% własny — Godot to tylko warstwa prezentacji. Patrz
  TECH_STACK.md.
- Protokół: tekstowy TCP na start (czytelny do debugowania), do rewizji
  binarnej/UDP przy realnym obciążeniu — patrz NETWORKING.md.
- To NIE jest ten sam projekt co Azonera (serwer Canary/OTServBR + strona
  `strona`). Zero współdzielonego kodu, danych, assetów.

## Następny krok

1. **Użytkownik:** zainstalować `.apk` (`client/builds/android/nowa-gra-2d-debug.apk`,
   przekazany jako plik do pobrania) na realnym telefonie Android lub w
   emulatorze na swoim komputerze — sprawdzić, że się instaluje, odpala,
   i (mając zbudowany `server/build/server` lokalnie) łączy się z serwerem.
   Bez tego Phase 2 nie jest w 100% zamknięta zgodnie z ROADMAP.md ("Test na
   prawdziwym urządzeniu: po stronie użytkownika").
2. Po potwierdzeniu działania na urządzeniu: odhaczyć Phase 2 w ROADMAP.md.
3. Dopiero po tym: Phase 4 (Login & Characters) — baza danych, konta.

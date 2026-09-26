# CHANGELOG.md

## 2026-09-26

- Audyt środowiska (Phase 0): brak Android SDK/NDK, brak `/dev/kvm`,
  `dl.google.com` zablokowany przez politykę sieciową kontenera.
- Ustalono stos: serwer C++20/CMake (autorytatywny), klient Godot 4.3
  (Android + desktop).
- Utworzono szkielet monorepo: `server/`, `client/`, `docs/`.
- Zaimplementowano i **zweryfikowano end-to-end** minimalny protokół
  sieciowy: `WELCOME`/`MOVE`/`POS`/`LEAVE`, z walidacją ruchu po stronie
  serwera (klamrowanie do `kMaxMovePerTick`). Test: realny serwer + realny
  headless klient Godot + skrypt symulujący próbę cheatowania.
- Dodano pełną dokumentację projektu (ten plik i pliki w `docs/`).
- Nieukończone: instalacja Android SDK (zablokowana sieciowo), pierwszy
  `.apk`.

## 2026-09-26 (2)

- Dodano obsługę pada (Bluetooth na Androidzie, wired na desktopie).
  Zweryfikowano: `InputMap` domyślnie binduje `ui_left/right/up/down` do
  D-pada i lewego analoga (dump `InputMap.action_get_events`); symulacja
  `InputEventJoypadMotion` w headless Godocie potwierdziła, że
  `Input.get_vector(...)` — czyli ta sama funkcja, którą już używał
  `world.gd` dla klawiatury — reaguje identycznie na pada. Brak dodatkowego
  kodu specyficznego dla gamepada w ścieżce ruchu.
- Dodano `client/scripts/gamepad_status.gd`: wykrywanie połączenia/
  rozłączenia pada w czasie rzeczywistym (`Input.joy_connection_changed`),
  fundament pod przyszły wskaźnik w HUD.
- Nieukończone: przypisanie przycisków akcji pada (nie istnieje jeszcze
  system akcji), test na fizycznym padzie Bluetooth (wymaga urządzenia
  użytkownika).

## 2026-09-26 (3)

- Ponowny audyt sieci w środowisku "AZONERA MMORPG 2D" (nowa sesja):
  zweryfikowano realnym `curl` (nie deklaracją), że `dl.google.com` wciąż
  zwraca `403` na `CONNECT` przez proxy środowiska — ta sama blokada co
  w poprzedniej sesji, mimo zmiany środowiska. Nie podjęto żadnej próby
  obejścia (zgodnie z zasadą projektu — blokada polityki proxy nie jest
  czymś do obchodzenia z bash).
- Faza 2 (Android Client) pozostaje zablokowana. Nie zainstalowano
  Android SDK, nie skonfigurowano eksportu, nie zbudowano `.apk`.
  Nieukończone.

## 2026-09-26 (4)

- Sieć odblokowana przez użytkownika (środowisko "AZONERA MMORPG 2D") —
  zweryfikowano realnym `curl`: `dl.google.com` i pozostałe hosty potrzebne
  do Android/Gradle są dostępne.
- **Phase 2 (Android Client) — pierwszy `.apk` zbudowany i zweryfikowany.**
  Zainstalowano w kontenerze: Android SDK cmdline-tools + `platform-tools`
  + `build-tools;34.0.0` + `platforms;android-34`; Godot 4.3.stable
  (headless editor + export templates, z GitHub Releases —
  `downloads.tuxfamily.org/godotengine/4.3/...` zwraca 404 dla tej wersji);
  JDK 17 (`openjdk-17-jdk-headless`, obok istniejącego JDK 21).
- Napotkano i naprawiono dwa realne błędy/pułapki Godota 4.3 (opisane
  ze źródłem w `docs/BUILD.md`):
  1. `godot4 --headless --install-android-build-template` wisi bez końca w
     środowisku bez GUI (`ProgressDialog` nigdy nie sygnalizuje końca) —
     odtworzono ręcznie identyczny efekt (`.build_version`, `.gdignore`,
     rozpakowanie `android_source.zip`), zweryfikowane przeciwko źródłu
     silnika (`export_template_manager.cpp`).
  2. Eksport failuje z **pustym** komunikatem błędu, gdy projekt nie ma
     `rendering/textures/vram_compression/import_etc2_astc=true` — dodano
     to ustawienie do `client/project.godot` (wymagane i poprawne dla
     eksportu mobilnego, nie obejście).
  3. Gradle build Androida w Godot 4.3 wymaga dokładnie JDK 17 (nie 21) —
     skonfigurowano osobny JDK 17 w Editor Settings.
- Dodano `client/export_presets.cfg` (preset "Android", Gradle build,
  `arm64-v8a`, permissions ograniczone do `INTERNET`+`ACCESS_NETWORK_STATE`
  — dokładnie to, czego klient używa, nic więcej). Usunięto go z
  `.gitignore` — to realna, przenośna konfiguracja projektu (bez lokalnych
  ścieżek keystore), wartość do zachowania w repo.
- Weryfikacja `.apk` (`client/builds/android/nowa-gra-2d-debug.apk`,
  74 849 033 B, powtarzalny rozmiar w dwóch niezależnych buildach):
  `aapt dump badging` — package `com.novagra2d.client`, minSdk 24,
  targetSdk 34, `native-code: arm64-v8a`; `apksigner verify` — podpisany
  (v2 scheme); `unzip -t` — brak błędów integralności; zawartość `assets/`
  potwierdza obecność realnych scen/skryptów (`World.tscn`,
  `PlayerEntity.tscn`, `world.gdc`, `net_client.gdc`, `player_entity.gdc`,
  `gamepad_status.gdc`).
- Regresja: `godot4 --headless --path client --import` (exit 0), build
  serwera C++ bez ostrzeżeń, `tools/test_client.py` (protokół end-to-end)
  — wszystko przechodzi bez zmian po modyfikacjach klienta.
- **Nieukończone:** instalacja/test `.apk` na fizycznym urządzeniu Android
  lub emulatorze — fizycznie niemożliwe w tym kontenerze (brak `/dev/kvm`,
  brak GUI). Wymaga maszyny użytkownika. Android SDK/JDK/Godot binary nie
  są częścią repo (żyją w efemerycznym kontenerze) — `docs/BUILD.md` ma
  pełną procedurę do powtórzenia w nowej sesji.

## 2026-09-26 (5) — Phase 4: Login & Characters

- **Serwer:** TLS 1.2+ (OpenSSL) na wszystkich połączeniach; konta
  (`REGISTER`/`LOGIN`, scrypt + sól, porównanie w stałym czasie);
  postacie (`CHAR_LIST`/`CHAR_CREATE`/`CHAR_SELECT`) z kontrolą własności;
  persystencja w SQLite (nowe `server/src/db/`, `server/src/auth/`,
  `server/src/net/tls.*`); snapshot świata przy wejściu; zapis pozycji przy
  rozłączeniu i przy łagodnym zamknięciu (SIGINT/SIGTERM).
- **Bezpieczeństwo (znalezione przeglądem i poprawione przed commitem):**
  stan świata (`POS`/`LEAVE`) szedł do każdego połączenia TLS, także bez
  konta → teraz tylko do graczy w świecie; throttling uwierzytelniania
  (1 próba/s, 5 porażek → rozłączenie), bo scrypt blokuje jednowątkową pętlę
  gry; limity bufora wejścia (1024 B) i wyjścia (64 KB); 60 s na zalogowanie
  (slowloris); odrzucanie deskryptorów ≥ `FD_SETSIZE` (zapis poza tablicą w
  `select()`); `REGISTER` sprawdza zajętość loginu przed liczeniem scrypt.
- **Realne błędy znalezione testami i naprawione:**
  1. `SIGPIPE` przy zapisie do zamkniętego gniazda zabijał cały serwer.
  2. Nieczyszczona kolejka błędów OpenSSL (`ERR_clear_error`): realny błąd
     jednego połączenia sprawiał, że `SSL_get_error()` dla *innych*,
     zdrowych połączeń zwracał `SSL_ERROR_SSL` → kaskadowe rozłączanie.
  3. Self-move-assignment w `removeClient()` przy usuwaniu ostatniego
     elementu wektora (zabezpieczone; okazało się nie być przyczyną nr 2,
     ale jest niebezpieczne dla `std::string`).
- **Fałszywe tropy (opisane, żeby nie powtarzać):** "zawieszanie" testów to
  blokujący `recv()` z timeoutem w Pythonie przy TLS 1.3 (realne czasy
  serwera: REGISTER ~50 ms, reszta < 2 ms); dziwne kody wyjścia 144 to
  `pkill -f "build/server"` zabijający własną powłokę (wzorzec pasował do
  jej linii poleceń).
- **Klient Godot:** `net_client.gd` przepisany na TLS (StreamPeerTLS) z
  zawsze weryfikowanym certyfikatem — tryb bez weryfikacji usunięty
  (`client_unsafe()` i tak nie działał w 4.3); nowy ekran logowania/
  rejestracji/wyboru postaci (`login_ui.gd`) + pole adresu serwera (bez
  tego APK na telefonie łączyłby się z samym sobą — dotyczyło też APK z
  Phase 2); auto-logowanie z linii poleceń do testów.
- **Testy:** `tools/test_client.py` przepisany (34 asercje, zweryfikowany
  TLS); nowe harnessy `tools/godot_reconnect_test.gd`,
  `tools/godot_screenshot.gd`. Pierwsze zrzuty ekranu gry (Xvfb).
- **Decyzja użytkownika:** gra 2D z pochyloną kamerą (izometria w stylu
  Diablo 2) — zapisane w GAME_DESIGN.md, realizacja w Phase 5.
- **Nieukończone:** żadna część nie była testowana na telefonie ani przez
  człowieka; znane ograniczenia w KNOWN_ISSUES.md.

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

## Co NIE działa / nie istnieje jeszcze

- Brak `.apk`. Android SDK/NDK nie zainstalowane w tym kontenerze.
- Brak jakiejkolwiek grafiki poza placeholderami (kolorowe prostokąty).
- Brak kont/logowania/bazy danych/persystencji.
- Brak combat, itemów, questów, NPC, ekonomii, PvP, guildii — wszystko od
  Phase 6 wzwyż w ROADMAP.md.
- Brak testu na prawdziwym urządzeniu Android (i nie będzie możliwy z tego
  kontenera — brak `/dev/kvm`, brak GUI).

## Bloker aktywny

**Sieć:** `dl.google.com` (i Google Maven) były zablokowane przez politykę
sieciową tego środowiska w momencie audytu — Gradle/Android Gradle Plugin
tego potrzebuje. Użytkownik zadeklarował, że zmieni ustawienia środowiska
(menu sesji → Edit → Network access → dodać `dl.google.com`). **Do
sprawdzenia na początku następnej sesji**, jeśli nie zostało potwierdzone w
tej.

## Repozytorium GitHub

**Nie istnieje jeszcze.** Plan: `Toruniiak/nowa-gra-2d`, prywatne. Próba
utworzenia przez API nie powiodła się (`403 Resource not accessible by
integration` — apka Claude GitHub App nie ma uprawnienia do tworzenia
nowych repozytoriów, tylko do pracy na istniejących). Użytkownik ma
utworzyć repo ręcznie na https://github.com/new. Cały dotychczasowy kod
istnieje **tylko lokalnie w tej sesji** (scratchpad) — do wypchnięcia po
utworzeniu repo.

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

1. Użytkownik: utworzyć repo `Toruniiak/nowa-gra-2d` (prywatne) + potwierdzić
   zmianę ustawień sieci (dl.google.com).
2. Po repo: `add_repo`, pierwszy push całego dotychczasowego stanu.
3. Po sieci: instalacja Android SDK/NDK (`sdkmanager`), konfiguracja eksportu
   Godota, pierwszy `.apk`, weryfikacja zawartości (manifest/assety) —
   Phase 2 z ROADMAP.md.
4. Dopiero po tym: Phase 4 (Login & Characters) — baza danych, konta.

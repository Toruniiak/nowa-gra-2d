# PROJECT_STATE.md

**Ostatnia aktualizacja:** 2026-09-26 (sesja Claude Code)

Ten plik ma pozwolić kontynuować projekt w nowej sesji bez utraty kontekstu.
Czytaj go pierwszy.

## Co działa (zweryfikowane, nie zadeklarowane)

- **Serwer C++** (`server/`): buduje się bez ostrzeżeń (`-Wall -Wextra
  -Wpedantic`), zależności: OpenSSL 3 + SQLite 3 (TECH_STACK.md).
  - TLS 1.2+ na każdym połączeniu; klient bez TLS nie dostaje odpowiedzi.
  - Konta (`REGISTER`/`LOGIN`): hasła wyłącznie jako scrypt + sól,
    porównanie w stałym czasie; throttling prób; 60 s na zalogowanie.
  - Postacie (`CHAR_LIST`/`CHAR_CREATE`/`CHAR_SELECT`) z kontrolą
    własności; pozycja w SQLite, zapisywana przy rozłączeniu i przy łagodnym
    zamknięciu serwera (SIGINT/SIGTERM); snapshot świata przy wejściu.
  - Ruch: intencje `MOVE` przycinane do `kMaxMovePerTick`; stan świata
    wysyłany tylko graczom w świecie.
  - Protokół i maszyna stanów: NETWORKING.md.
- **Testy serwera:** `tools/test_client.py` — 34 asercje przez
  zweryfikowany TLS: konta, postacie, anty-cheat, reconnect z persystencją,
  snapshot, oraz bezpieczeństwo (klient bez TLS, komendy przed logowaniem,
  cudza postać, podglądanie bez konta, burst logowań → throttling +
  rozłączenie, za długa linia, brak hasła jawnym tekstem w pliku bazy).
  3 kolejne przebiegi na jednym serwerze: 0 błędów. Osobno sprawdzone:
  limit 60 s na zalogowanie, zapis przy `SIGTERM` + restart (pozycja
  (7, 2) przetrwała).
- **Klient Godot** (`client/`): import headless bez błędów, wszystkie
  skrypty przechodzą `--check-only`. Łączy się przez TLS z **zawsze
  weryfikowanym** certyfikatem (przypięty dev cert albo systemowe CA),
  ekran logowania/rejestracji/wyboru i tworzenia postaci
  (`scripts/login_ui.gd`), pole adresu serwera + "Połącz ponownie".
  Zweryfikowane prawdziwym klientem przeciw prawdziwemu serwerowi:
  rejestracja → postać → świat; persystencja (postać przesunięta innym
  klientem pojawiła się w (5, 0)); zmiana adresu + ponowne połączenie
  (`tools/godot_reconnect_test.gd`); 3 testy negatywne TLS (zły cert, brak
  certu, zła nazwa hosta) — połączenie odrzucone.
- **Zrzuty ekranu** prawdziwego klienta przez Xvfb (`tools/godot_screenshot.gd`)
  — pokazują ekran logowania i dwie postacie w świecie (kwadraty-placeholdery).
- **Android:** debug APK budowany i weryfikowany (manifest, podpis v2,
  integralność); zawiera ekran logowania i przypięty certyfikat
  (`assets/certs/dev_server.crt`). Procedura: BUILD.md.
- **Sterowanie padem:** wspólna ścieżka `Input.get_vector` (symulacja
  zdarzenia, nie fizyczny pad).

- **Phase 5 (w toku) — mapa i ruch jak w Tibii:** mapa 40×30
  (`client/data/maps/start.json`: wioska z domem, jezioro, drogi, ruiny, las
  na granicy) z grafiką z generatora `tools/art/gen_tileset.py` (styl wg CC0
  wzoru Summer Engine, rysowana od zera). Serwer wczytuje tę samą mapę,
  pozycje są na kaflach, ruch `STEP N/E/S/W` z czasem kroku 250 ms, kolejką
  jednego kroku (spam nie przyspiesza), kolizjami z mapą i między graczami.
  Zweryfikowane: `tools/test_client.py` (m.in. blokowanie przez gracza,
  limit prędkości, zatrzymanie na brzegu jeziora wg mapy, zapis kafla),
  3 uszkodzone mapy → serwer odmawia startu z komunikatem, prawdziwy klient
  Godot chodzi (`tools/godot_walk_test.gd`: 5 kroków w 1,2 s, płynna
  animacja), zrzut ekranu przez Xvfb, APK zawiera mapę i grafikę.
- **Odpalacz do testów (`tools/launcher/`)** — pobiera najnowszą wersję,
  buduje, startuje serwer i grę (BUILD.md). `graj.sh` przetestowany na
  Linuksie; `graj.bat` (Windows + WSL) **nieprzetestowany** — pierwszy test
  po stronie użytkownika.

## Co NIE działa / nie istnieje jeszcze

- **Nic nie było uruchomione na telefonie ani użyte przez człowieka** —
  ani APK, ani ekran logowania. Brak `/dev/kvm` w kontenerze.
- Brak potworów, przedmiotów, wielu map/pięter; grafika to pierwsza wersja z generatora.
- Brak combat, statystyk, itemów, questów, NPC, ekonomii, PvP, guildii.
- Znane problemy bezpieczeństwa/skali (m.in. scrypt blokuje wątek gry,
  KNOWN_ISSUES.md.

## Decyzja projektowa od użytkownika: kamera

**Styl i mechanika klasycznej Tibii** (siatka 32×32, rzut skośny, ruch po
kratkach) — ostateczna decyzja użytkownika, zastąpiła wcześniejszą notatkę o
izometrii. Wymaga zmian także na serwerze (pozycje na kaflach, serwer zna
mapę) — szczegóły: GAME_DESIGN.md, "Kamera, perspektywa i ruch".

## Środowisko (efemeryczne — kontener)

Android SDK (`/opt/android-sdk`), JDK 17, Godot 4.3 + export templates,
certyfikat dev (`server/certs/`, `client/certs/`) i bazy `.db` istnieją
tylko w tym kontenerze — nie w repo. Nowa sesja: odtworzyć wg BUILD.md.
Sieć: `dl.google.com`, GitHub Releases, Maven/Gradle dostępne
(zweryfikowane 2026-09-26).

## Repozytorium GitHub

`Toruniiak/nowa-gra-2d`, branch `main`.

## Decyzje techniczne podjęte (i dlaczego)

- Serwer: C++20, własny, autorytatywny. Klient: Godot 4.3 (tylko
  prezentacja) — TECH_STACK.md.
- Baza: SQLite; transport: TLS (OpenSSL); hasła: scrypt — TECH_STACK.md,
  ARCHITECTURE.md.
- Protokół tekstowy TCP+TLS na start, do rewizji — NETWORKING.md.
- To NIE jest projekt Azonera. Zero współdzielonego kodu, danych, assetów.

## Następny krok

1. **Użytkownik:** zainstalować APK na telefonie, uruchomić serwer na PC
   (BUILD.md), wpisać adres PC na ekranie logowania, założyć konto i
   postać, poruszać się. Zgłosić, co nie działa / co jest niewygodne.
2. **Phase 5 — First World** w stylu Tibii: ruch po kratkach (serwer +
   protokół), mapa jako dane wspólne serwera i klienta, kamera za graczem,
   pierwszy zestaw grafik (wybór źródeł: ASSET_PIPELINE.md). Przed startem ustalić z użytkownikiem otwarte
   punkty z GAME_DESIGN.md (rozmiar kafla, 8 vs 4 kierunki, styl grafiki).
3. Przed jakimkolwiek publicznym serwerem: KNOWN_ISSUES.md — scrypt w
   wątku roboczym, limit `MOVE`, prawdziwy certyfikat, backupy bazy.

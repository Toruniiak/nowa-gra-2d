# TECH_STACK.md

Stan faktyczny (nie plan, nie życzenia) — aktualizuj przy każdej zmianie stosu.

## Serwer (autorytatywny)

- **Język:** C++20
- **Build:** CMake ≥ 3.20
- **Sieć:** POSIX sockets (TCP) + **TLS 1.2+ (OpenSSL)**, pętla `select()`,
  jeden wątek. Do rozważenia przy skalowaniu: poll/epoll, ASIO albo ENet (UDP)
  dla ruchu w czasie rzeczywistym — decyzja odłożona, patrz NETWORKING.md.
- **Baza danych:** **SQLite 3** (zdecydowane w Phase 4, 2026-09-26). Plik
  osadzony w procesie serwera, bez osobnej usługi do uruchamiania/zabezpieczania
  na tym etapie; wolumen danych (konta + pozycje postaci) nie uzasadnia
  osobnego procesu DB. Do rewizji (np. PostgreSQL) dopiero przy realnej
  potrzebie współbieżności/skali — nie wcześniej.
- **Hasła:** scrypt (N=16384, r=8, p=1, sól 16 B/konto) z OpenSSL —
  ~50 ms/hash zmierzone w tym kontenerze. Argon2id (pierwszy wybór OWASP)
  odrzucony tylko dlatego, że wymagałby drugiej biblioteki krypto.

## Klient

- **Silnik:** Godot 4.3 (GDScript na start; C#/GDExtension do rozważenia, gdy
  wydajność tego wymaga — nie na starcie).
- **Powód wyboru Godot, nie "silnik od zera":** napisanie własnego renderera 2D +
  systemu animacji + dotykowego UI + eksportu Android w czystym C++/NDK to
  osobny, wielomiesięczny projekt bez żadnej gwarancji, że da coś grywalnego
  szybciej niż Godot. Godot daje gotowy renderer 2D, system animacji (Animation-
  Player/AnimationTree), UI (Control nodes ze skalowaniem pod różne ekrany) i
  natywny eksport na Android — a serwer (logika gry, autorytatywność, anty-cheat)
  **zostaje w 100% własny, w C++**. To nie jest "silnik MMO z pudełka" — Godot
  nie ma wbudowanego multiplayera pod nasz przypadek (własny protokół, własna
  autorytatywność); to tylko warstwa prezentacji.
- **Platforma docelowa:** Android (eksport APK). Desktop (Linux) jako platforma
  deweloperska do szybkiej iteracji — to samo źródło Godota eksportuje na obie.

## Środowisko deweloperskie (ten kontener)

- Ubuntu 24.04, brak `/dev/kvm` → **brak emulatora Androida w tym środowisku**.
  Test na urządzeniu/emulatorze wymaga maszyny użytkownika.
- Android SDK, JDK 17, Godot 4.3 + export templates: instalowane w kontenerze
  (efemeryczne, nie w repo) — procedura w BUILD.md.
- Renderowanie klienta bez monitora działa przez `xvfb-run` + Mesa (programowy
  OpenGL) — używane do zrzutów ekranu (`tools/godot_screenshot.gd`). To nie
  zastępuje testu na telefonie.

## Zależności zewnętrzne

Każda nowa zależność wymaga wpisu tutaj z uzasadnieniem — nie dodawaj
bibliotek "na wszelki wypadek".

| Zależność | Gdzie | Od kiedy | Dlaczego |
|---|---|---|---|
| Godot Engine 4.3 | klient | Phase 1 | patrz wyżej |
| OpenSSL 3 (`libssl-dev`) | serwer | Phase 4 | szyfrowany transport (TLS) wymagany przed pierwszym kontem (CLAUDE.md, KNOWN_ISSUES.md) + scrypt do haseł. Własna kryptografia wykluczona. |
| SQLite 3 (`libsqlite3-dev`) | serwer | Phase 4 | persystencja kont i postaci, patrz wyżej |

| nlohmann/json 3 (`nlohmann-json3-dev`) | serwer | Phase 5 | odczyt mapy i definicji kafli (`client/data/*.json`) — ten sam format co klient. Sam nagłówek, licencja MIT. Własny parser JSON odrzucony (ryzyko błędów w kodzie czytającym dane). |

Testowane wersje (ten kontener): OpenSSL 3.0.13, SQLite 3.45.1, nlohmann/json 3.11.3.

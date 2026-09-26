# TECH_STACK.md

Stan faktyczny (nie plan, nie życzenia) — aktualizuj przy każdej zmianie stosu.

## Serwer (autorytatywny)

- **Język:** C++20
- **Build:** CMake ≥ 3.20
- **Sieć:** POSIX sockets (TCP), bez zewnętrznych bibliotek na starcie — świadomie,
  żeby nie ciągnąć zależności przed ustaleniem realnych wymagań (liczba graczy,
  częstotliwość tick). Do rozważenia przy skalowaniu: ASIO/Boost.Asio albo ENet
  (UDP) dla ruchu w czasie rzeczywistym — decyzja odłożona, patrz NETWORKING.md.
- **Baza danych:** niezdecydowana. Kandydaci: PostgreSQL (transakcje, dojrzałe
  narzędzia) lub SQLite na etapie prototypu (zero-config). Decyzja przy Phase 4
  (Login & Characters), nie wcześniej.

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
- Android SDK/NDK: **nie zainstalowane** — `dl.google.com` był zablokowany
  przez politykę sieciową w momencie audytu (2026-09-26). Wymaga zmiany
  ustawień środowiska przed pierwszym buildem `.apk`. Stan aktualny: patrz
  PROJECT_STATE.md.
- Godot 4.3 (headless, linux.x86_64) zainstalowany lokalnie w kontenerze do
  walidacji projektu klienta (import, brak błędów skryptów). Nie jest to
  narzędzie GUI — nie renderuje obrazu w tym środowisku.

## Zależności zewnętrzne

Brak na dzień pisania tego pliku, poza samym Godot Engine i standardową
biblioteką C++. Każda nowa zależność (biblioteka sieciowa, ORM, biblioteka
kryptograficzna) wymaga wpisu tutaj z uzasadnieniem — nie dodawaj bibliotek
"na wszelki wypadek".

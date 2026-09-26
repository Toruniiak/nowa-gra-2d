# CLAUDE.md — nowa-gra-2d

## Ważne: to NIE jest Azonera

Ten projekt jest celowo i całkowicie odseparowany od Azonery (serwer Canary/
OTServBR, strona `strona`, klient `Azonera-OTClient`, `azonera-wiki` itd.).
Nie kopiuj tu kodu, schematów bazy, assetów ani konfiguracji z żadnego
repozytorium Azonery bez wyraźnej prośby. Żadne zmiany w tym repo nie mają
dotykać repozytoriów Azonery, i odwrotnie.

## Co to jest

2D MMORPG PvP na Androida, open world, inspirowane Tibią/klasycznymi
MMORPG — własny kod i assety. Pełny opis systemów: `docs/GAME_DESIGN.md`.
Aktualny, szczery stan projektu: **`docs/PROJECT_STATE.md` — czytaj
pierwsze w każdej nowej sesji.**

## Stos technologiczny

- **Serwer** (`server/`): C++20, CMake, autorytatywny, POSIX sockets (na
  razie, bez zewnętrznych bibliotek).
- **Klient** (`client/`): Godot 4.3, GDScript, eksport Android + Linux.
- Szczegóły i uzasadnienia: `docs/TECH_STACK.md`.

## Struktura

```
server/       kod serwera C++ (net/, world/, docelowo: entity/, combat/, db/, data/)
client/       projekt Godot (scenes/, scripts/)
docs/         cała dokumentacja projektowa (patrz README.md dla spisu)
tools/        skrypty diagnostyczne (np. test_client.py) — NIE część gry
```

## Zasada nadrzędna

Serwer jest jedynym źródłem prawdy. Klient wysyła tylko intencje (`MOVE`,
później `ATTACK`/`USE_ITEM`/itd.) — nigdy wynik. Żaden nowy typ pakietu
klient→serwer nie może zakładać, że klient podaje poprawną wartość. Patrz
`docs/ARCHITECTURE.md` i `docs/NETWORKING.md`.

## Zasady pracy

- To greenfield na poziomie treści (lore, grafika, konkretne itemy/questy),
  ale **nie** na poziomie architektury sieci/serwera od Phase 3 wzwyż —
  traktuj istniejący kod (`server/src/net/`, `server/src/world/`,
  `client/scripts/net_client.gd`) jak każdy inny działający system: audytuj
  przed zmianą, nie przepisuj bez powodu.
- Nie twórz fikcyjnych plików, systemów, bibliotek ani danych "na zapas" —
  dodawaj tylko to, co jest potrzebne do aktualnie realizowanej fazy
  (`docs/ROADMAP.md`).
- Przed uznaniem czegokolwiek za "gotowe": build musi przejść bez ostrzeżeń
  (`-Wall -Wextra -Wpedantic` dla serwera; `godot4 --headless --path client
  --import` bez błędów dla klienta), a zmiany sieciowe — przetestowane
  end-to-end (patrz `docs/BUILD.md`, sekcja "Test end-to-end"), nie tylko
  "skompilowało się".
- Nigdy nie deklaruj ukończenia fazy/milestone'u, jeśli wymagania nie są
  faktycznie spełnione (patrz `docs/PROJECT_STATE.md` — musi zawsze
  odzwierciedlać prawdę, nie plan).

## Bezpieczeństwo

- Serwer nigdy nie ufa danym z klienta: pozycja, HP, przedmioty, złoto,
  EXP, poziom — wszystko liczone i walidowane po stronie serwera.
- Żadnych sekretów, tokenów, kluczy w kodzie ani w historii commitów.
- Gdy powstanie warstwa kont (Phase 4): hasła tylko hashowane, porównania
  tokenów/hashy odporne na timing attack, szyfrowany transport przed
  pierwszym realnym kontem — patrz `docs/KNOWN_ISSUES.md`.

## Ograniczenia tego środowiska (do sprawdzenia na nowo w nowej sesji — mogą się zmienić)

- Brak `/dev/kvm` → brak emulatora Androida w tym kontenerze. Test na
  urządzeniu wymaga maszyny użytkownika.
- Android SDK/NDK nie zainstalowane; `dl.google.com` bywał blokowany przez
  politykę sieciową — sprawdź `docs/PROJECT_STATE.md` co do aktualnego
  stanu przed założeniem, że trzeba to powtórzyć.

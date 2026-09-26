# nowa-gra-2d

2D MMORPG PvP na Androida, open world, inspirowane klasycznymi MMORPG (w
szczególności Tibią) — **własny kod, własne assety, własna nazwa i tożsamość
wizualna, docelowo**. Zero powiązań z projektem Azonera (inne repo, inny
serwer, inny klient).

## Stos

- **Serwer** (`server/`) — C++20, CMake, autorytatywny, POSIX sockets.
- **Klient** (`client/`) — Godot 4.3, eksport Android + desktop.

Uzasadnienie wyboru i pełny stan faktyczny: `docs/TECH_STACK.md`.

## Status

Wczesna faza (Phase 1-3 z `docs/ROADMAP.md`): fundament sieciowy działa i
jest zweryfikowany end-to-end (serwer + headless klient Godot + test
anty-cheatu). Brak jeszcze: grafiki (same placeholdery), kont, combat,
`.apk`. Pełny, szczery stan: `docs/PROJECT_STATE.md` — **czytaj to pierwsze
w nowej sesji.**

## Build

Patrz `docs/BUILD.md`.

## Dokumentacja

| Plik | Zawartość |
|---|---|
| `docs/PROJECT_STATE.md` | Stan na żywo — czytaj pierwsze |
| `docs/ARCHITECTURE.md` | Struktura kodu, zasada autorytatywności serwera |
| `docs/TECH_STACK.md` | Stos technologiczny i uzasadnienia |
| `docs/NETWORKING.md` | Protokół sieciowy |
| `docs/GAME_DESIGN.md` | Fundamenty systemowe gry |
| `docs/ASSET_PIPELINE.md` | Pipeline grafiki |
| `docs/ROADMAP.md` | Fazy długoterminowe |
| `docs/BUILD.md` | Jak zbudować/uruchomić |
| `docs/KNOWN_ISSUES.md` | Znane ograniczenia (szczerze) |
| `docs/CHANGELOG.md` | Historia zmian |

Zasady pracy dla Claude Code: `CLAUDE.md`.

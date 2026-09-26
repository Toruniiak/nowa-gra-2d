# ROADMAP.md

Fazy długoterminowe. Kolejność ma znaczenie: każda faza zakłada, że poprzednia
działa i jest przetestowana — nie przeskakujemy do combat/ekonomii bez
działającej sieci i kont.

- [x] **Phase 0 — Audit & Architecture.** Audyt środowiska, wybór stosu
      (C++ serwer / Godot klient), dokumentacja. *Zrobione: 2026-09-26.*
- [x] **Phase 1 — Core Engine (fundament).** Build C++ (CMake) + projekt
      Godot, oba się budują/importują bez błędów. *Zrobione.*
- [x] **Phase 3 — Networking (minimalny szkielet).** Protokół TCP
      `WELCOME/MOVE/POS/LEAVE`, walidacja ruchu po stronie serwera
      (anty-cheat na pojedynczy pakiet), zweryfikowane end-to-end (realny
      serwer + realny headless klient Godot + skrypt testowy z próbą
      cheatowania). *Zrobione — ale patrz KNOWN_ISSUES.md: brak rate-limitu
      pakietów, brak szyfrowania, format tekstowy do rewizji.*
- [ ] **Phase 2 — Android Client (eksport).** Instalacja Android SDK/NDK w
      środowisku (blokada: `dl.google.com`, patrz PROJECT_STATE.md),
      konfiguracja eksportu Godota, pierwszy `.apk`, weryfikacja zawartości
      (manifest, assety) — **bez** testu na urządzeniu w tym środowisku
      (brak KVM/GUI, patrz TECH_STACK.md). Test na prawdziwym urządzeniu:
      po stronie użytkownika.
- [ ] **Phase 4 — Login & Characters.** Rejestracja/logowanie, wybór/
      tworzenie postaci, persystencja (baza danych — decyzja SQL na tym
      etapie), reconnect.
- [ ] **Phase 5 — First World.** Pierwsza prawdziwa mapa (nie placeholder),
      system modułowy map (patrz GAME_DESIGN.md), tile-based, dekoracje.
- [ ] **Phase 6 — Character Systems.** HP/Mana/EXP/level/statystyki,
      profesje jako dane (nie hardkodowana logika).
- [ ] **Phase 7 — Combat.** PvE/PvP, melee/ranged/magic, cooldown, obrażenia,
      pancerz/odporności, efekty statusowe.
- [ ] **Phase 8 — Monsters.** System AI, dane potworów, spawn.
- [ ] **Phase 9 — NPC & Quests.** NPC data-driven (dialog/sklep/quest/
      teleport/bank), system questów (kill/fetch/delivery/dialog/chain).
- [ ] **Phase 10 — Items & Inventory.** Itemy jako dane, ekwipunek, depot.
- [ ] **Phase 11 — PvP.** Obszary PvP/bezpieczne, reguły walki graczy.
- [ ] **Phase 12 — Economy.** Złoto, sklepy NPC, handel gracz-gracz.
- [ ] **Phase 13 — Guilds.**
- [ ] **Phase 14 — Expanded Open World.** Kolejne kontynenty/dungeony/bossy.
- [ ] **Phase 15 — Optimization.** Rewizja protokołu (TCP→UDP dla ruchu?),
      profiling na słabszych telefonach, draw calls, pamięć.
- [ ] **Phase 16 — Security.** Rate limiting, packet validation, logging
      podejrzanych akcji, szyfrowanie transportu.
- [ ] **Phase 17 — Closed Testing.**
- [ ] **Phase 18 — Release Candidate.**
- [ ] **Phase 19 — Production.**

## Aktualna faza

**Phase 2 (Android Client)** — zablokowana na decyzji użytkownika o
ustawieniach sieci środowiska. Patrz PROJECT_STATE.md dla stanu na żywo.

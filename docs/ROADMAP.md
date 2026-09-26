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
      `MOVE`, format tekstowy do rewizji. (Szyfrowanie: dodane w Phase 4.)*
- [x] **Phase 2 — Android Client (eksport).** Instalacja Android SDK,
      konfiguracja eksportu Godota, pierwszy `.apk`, weryfikacja zawartości
      (manifest, podpis, integralność, assety) — *zrobione i zweryfikowane:
      2026-09-26, patrz PROJECT_STATE.md i CHANGELOG.md.* **Bez** testu na
      urządzeniu w tym środowisku (brak KVM/GUI, patrz TECH_STACK.md) —
      test na prawdziwym urządzeniu zostaje po stronie użytkownika, zgodnie
      z zakresem tej fazy zdefiniowanym wyżej.
- [x] **Phase 4 — Login & Characters.** Rejestracja/logowanie, wybór/
      tworzenie postaci, persystencja (SQLite), reconnect, szyfrowany
      transport (TLS) — *zrobione i zweryfikowane automatycznie: 2026-09-26*
      (34 asercje protokołu/bezpieczeństwa + prawdziwy klient Godot). **Nie
      przetestowane jeszcze przez człowieka ani na telefonie** — patrz
      PROJECT_STATE.md i KNOWN_ISSUES.md (m.in. scrypt w wątku gry).
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

**Phase 5 (First World)** — jeszcze nie zaczęta. Phase 2 i 4 zamknięte w
zakresie możliwym z tego środowiska; test na telefonie i ręczny test ekranu
logowania zostają po stronie użytkownika (PROJECT_STATE.md).

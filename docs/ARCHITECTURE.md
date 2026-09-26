# ARCHITECTURE.md

## Zasada nadrzędna

Serwer jest **jedynym źródłem prawdy**. Klient nigdy nie decyduje o wyniku
żadnej akcji (ruch, walka, ekonomia, przedmioty) — wysyła intencję, serwer
waliduje i rozsyła stan. Cała reszta architektury wynika z tej zasady.

## Układ repozytorium (monorepo)

```
server/   C++20, CMake — logika gry, sieć, baza danych, autorytatywność
client/   Godot 4.3 — rendering, UI, input, eksport Android/desktop
docs/     ta dokumentacja
```

Serwer i klient to odrębne procesy komunikujące się wyłącznie przez protokół
sieciowy zdefiniowany w `NETWORKING.md`. Klient nie linkuje kodu serwera i
odwrotnie — jedyna "współdzielona wiedza" to specyfikacja protokołu (docelowo:
wspólny plik/dokument definiujący typy pakietów, żeby uniknąć rozjazdu).

## Serwer — struktura docelowa (rozszerzana etapami, nie budowana z góry)

```
server/src/
  main.cpp           punkt wejścia, inicjalizacja
  net/                warstwa sieciowa (accept, recv/send, serializacja pakietów)
  world/               stan świata: encje, pozycje, tick loop
  entity/               gracze, potwory, NPC — wspólny bazowy typ encji
  combat/               (Phase 7) system walki, oddzielony od world/, żeby
                        dodawanie mechanik nie wymagało zmian w pętli świata
  db/                   (Phase 4+) warstwa dostępu do bazy — konta, postacie
  data/                  (Phase 9+) definicje itemów/potworów/questów jako dane
                        (JSON/podobny format), NIE hardkodowane w kodzie C++
```

Każdy nowy system (quest, item, profesja) ma być **danymi** ładowanymi przez
istniejący, generyczny mechanizm — nie kolejnym `if`/`switch` rozrastającym
się w jednym pliku. To jest bezpośrednia konsekwencja wymogu "wieloletni
rozwój bez przepisywania architektury".

## Klient — struktura docelowa

```
client/
  project.godot
  scenes/     World.tscn (świat), Player.tscn, UI/ (HUD, inventory, itd.)
  scripts/    logika GDScript per-scena
  net/        klient sieciowy (StreamPeerTCP + parser tego samego protokołu
              co serwer)
  assets/     sprite'y, tilesety, dźwięki — patrz ASSET_PIPELINE.md
```

## Tick / game loop (serwer)

Serwer działa na pętli o ustalonym ticku (docelowo konfigurowalne, start: coś
w okolicy 10-20 Hz — dokładna wartość do ustalenia empirycznie przy
pierwszych testach opóźnień, nie zgadywana z góry). Każdy tick: odbierz
wejście od klientów → zwaliduj → zaktualizuj stan świata → rozeslij delty.

## Decyzje jeszcze nieподjęte (nie zgaduj, nie hardkoduj)

- Format zapisu danych świata/graczy (baza SQL vs. pliki) — Phase 4.
- Format definicji itemów/potworów/questów (JSON? własny DSL?) — Phase 9-10.
- TCP vs UDP dla ruchu w czasie rzeczywistym — patrz NETWORKING.md.

# GAME_DESIGN.md

## Status

Ten dokument opisuje **fundamenty systemowe**, nie finalny lore/setting gry.
Nazwa gry, świata, ras, historia — **nieustalone i celowo nie wymyślone
tutaj**. To decyzje kreatywne, które powinny wyjść od Ciebie (albo wspólnie),
nie zostać wygenerowane jako wypełniacz. Poniżej: co system MA umożliwiać,
nie konkretna treść.

## Zasada oryginalności

Inspiracja mechaniką gatunku (Tibia i klasyczne MMORPG) — bez kopiowania
sprite'ów, map, nazw chronionych, kodu. Każdy nowy asset/nazwa/system
przechodzi tę kontrolę przy tworzeniu, nie po fakcie.

## Kamera i perspektywa (decyzja użytkownika, 2026-09-26)

**Gra jest 2D z pochyloną kamerą — widok izometryczny w stylu Diablo 2**
(użytkownik podał jako przykład serię Diablo). Nie widok prosto z góry jak
klasyczna Tibia, i nie pełne 3D jak Diablo 3/4.

Konsekwencje techniczne (ustalone, do realizacji w Phase 5):

- **Serwer bez zmian.** Świat logicznie pozostaje płaską siatką (x, y);
  pozycje, kolizje, zasięg i walidacja ruchu liczone są na tej płaszczyźnie.
  Pochylenie to wyłącznie sposób rysowania — transformacja świat → ekran
  po stronie klienta. Protokół się nie zmienia.
- **Klient:** Godot 4.3 ma natywny tryb izometryczny (`TileMapLayer` z
  `TileSet.tile_shape = ISOMETRIC`) i sortowanie po głębokości
  (`y_sort_enabled`), więc postacie zasłaniają się poprawnie za drzewami/
  budynkami. Kamera (`Camera2D`) podąża za graczem — dziś jej nie ma
  (KNOWN_ISSUES.md).
- **Sterowanie:** "góra" na joysticku/padzie musi odpowiadać "górze ekranu",
  więc wektor wejścia jest obracany z przestrzeni ekranu do przestrzeni
  świata przed wysłaniem `MOVE`.
- **Grafika:** sprite'y rysowane pod kątem izometrycznym, zwykle w **8
  kierunkach** (nie 4) — to znacząco zwiększa liczbę klatek do przygotowania
  per animacja. Kafle w proporcji 2:1 (np. 64×32 px).

Otwarte (do decyzji przy Phase 5, nie zgadywane tutaj): dokładny kąt/
proporcja kafli, rozmiar kafla i postaci w pikselach, 8 vs 4 kierunki
animacji na start, ręcznie rysowane vs. renderowane z modeli 3D sprite'y.

## Postać gracza (fundament, patrz Phase 6)

Statystyki: HP, Mana, stamina, poziom, EXP, atrybuty podstawowe. Wygląd:
reaguje na wyposażenie (broń/pancerz/hełm/buty/tarcza) przez warstwy
sprite'ów, **nie** przez zmianę koloru jednej postaci (patrz
ASSET_PIPELINE.md — layered sprite system, do zaprojektowania przy Phase 6,
nie przy pierwszym movement-prototype).

## Profesje (fundament, patrz Phase 6)

Architektura: profesja = dane (statystyki bazowe, dostępne umiejętności,
modyfikatory), nie gałąź `if/switch` w kodzie combat. Przykładowe archetypy
do rozważenia (nazwy robocze, do potwierdzenia/zmiany): Knight (melee/tank),
Paladin (melee/support), Sorcerer (magia ofensywna), Druid (magia
wsparcia/natura). Rozszerzalność o kolejne klasy musi nie wymagać zmian w
silniku combat — tylko nowego wpisu danych.

## Świat (fundament, patrz Phase 5, 14)

Open world, modułowy podział na mapy/regiony (nie jeden gigantyczny plik —
patrz ARCHITECTURE.md). Typy terenu do pokrycia w miarę rozwoju: miasta,
wioski, lasy, góry, jaskinie, ruiny, dungeony, obszary PvP vs. bezpieczne,
boss areas. Teren ma wyglądać "zaprojektowany", nie proceduralnie losowy —
wymaga realnej pracy level-designerskiej przy Phase 5, nie automatycznego
generatora.

## Combat (fundament, patrz Phase 7)

PvE + PvP, melee/ranged/magia, cooldown, obrażenia/pancerz/odporności,
critical hits, efekty statusowe, leczenie/mana. System danych obrażeń
oddzielony od pętli świata (patrz ARCHITECTURE.md), żeby dodanie nowej
mechaniki (np. nowy status effect) nie wymagało przebudowy `world/`.

## Itemy / Questy / NPC (fundament, patrz Phase 9-10)

Wszystkie trzy systemy: **data-driven**. Nowy item/quest/NPC = nowy wpis
danych (format do ustalenia przy odpowiedniej fazie — JSON jest naturalnym
kandydatem, ale nie decydujemy przedwcześnie), nie nowy kod. Item ma dane
(typ, staty, wymagania), nie osobną klasę C++ na każdy przedmiot.

## UI / Sterowanie (fundament, patrz Phase 2+)

Mobilny HUD: HP/Mana/EXP/level, minimapa, inventory, equipment, skills,
spellbook, quest log, chat, battle list, target, action buttons. Sterowanie:
wirtualny joystick lub tap-to-move (do zdecydowania empirycznie — nie
teoretycznie — przy pierwszym testowalnym buildzie na Androida), przyciski
akcji, drag & drop dla ekwipunku. Godot's `Control` nodes ze skalowaniem pod
różne rozdzielczości — patrz TECH_STACK.md.

**Pad Bluetooth — zaimplementowane i zweryfikowane (nie plan).** Android
zgłasza sparowany pad Bluetooth (HID) do Godota jako zwykły joypad — bez
dodatkowych uprawnień/kodu parowania po stronie aplikacji. Ruch gracza jest
zbindowany do akcji silnikowych `ui_left/right/up/down`, które **domyślnie**
(zweryfikowane przez `InputMap.action_get_events` w tej sesji, nie z pamięci)
zawierają: strzałki klawiatury, D-pad (button_index 11-14) i lewy analog
(axis 0/1). Efekt: klawiatura (dev), D-pad/analog pada i — po dodaniu w
przyszłej fazie UI — wirtualny joystick dotykowy wszystkie sterują tą samą
postacią przez jedną, wspólną ścieżkę kodu (`Input.get_vector(...)` w
`client/scripts/world.gd`), bez odrębnej logiki "gamepad mode". To jest
świadoma decyzja architektoniczna: jedno źródło intencji ruchu, trzy metody
wejścia.

Dodatkowo: `client/scripts/gamepad_status.gd` wykrywa podłączenie/
odłączenie pada w czasie rzeczywistym (`Input.joy_connection_changed`) —
przydatne przy niestabilnych połączeniach Bluetooth i jako fundament pod
przyszły wskaźnik "pad podłączony" w HUD. Nie zaimplementowano jeszcze:
przypisania przycisków akcji (atak/użycie itemu) do pada — nie ma jeszcze
systemu akcji do zbindowania (patrz Phase 7). Nie zaimplementowano deadzone
tuningu ponad domyślny silnika ani wibracji (rumble) — do rozważenia, gdy
będzie czym testować na realnym padzie.

## Co NIE jest jeszcze zaprojektowane (nie zgaduj)

Nazwa gry i świata, lore/historia, konkretne rasy/frakcje, dokładne wzory
matematyczne obrażeń/EXP, dokładna lista pierwszych questów/potworów/map.

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

## Kamera, perspektywa i ruch (decyzja użytkownika, 2026-09-26 — ostateczna)

**Styl i mechanika klasycznej Tibii:** kwadratowa siatka **32×32 px**, rzut
skośny/oblique (wysokie obiekty i ściany "wystają" w górę-lewo), **ruch
krokami po kratkach**. Zastępuje wcześniejszą, tymczasową notatkę o
izometrii w stylu Diablo 2 (użytkownik ją wycofał tego samego dnia).

Konsekwencje techniczne (do realizacji w Phase 5):

- **Serwer:** pozycje stają się całkowitymi współrzędnymi kafli (x, y, i
  docelowo piętro z). Klient wysyła intencję kroku w kierunku (N/E/S/W +
  skosy), serwer sprawdza **przechodniość kafla** (serwer musi znać mapę!)
  i czas od poprzedniego kroku (szybkość postaci). To zastępuje obecne
  ciągłe `MOVE dx dy` — zmiana protokołu.
- **Mapa jako dane wspólne dla serwera i klienta:** klient bierze z niej
  grafikę, serwer tylko flagi (przechodni / blokuje / itp.). Jedno źródło
  prawdy — format do ustalenia w Phase 5 (propozycja: JSON).
- **Klient:** `Camera2D` podąża za graczem; płynna animacja kroku między
  kaflami (serwer podaje kafel docelowy, klient animuje przejście);
  sortowanie rysowania Tibia-style (wiersz po wierszu, obiekty wyższe niż
  32 px rysowane z przesunięciem w górę-lewo).
- **Grafika:** 4 kierunki postaci (N/E/S/W) jak w Tibii.
- **Nie wolno** używać grafik z Tibii (własność CipSoft) — tylko oryginalne
  lub darmowe na licencji pozwalającej na użycie komercyjne (ASSET_PIPELINE.md).

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

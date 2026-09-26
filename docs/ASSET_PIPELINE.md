# ASSET_PIPELINE.md

## Stan faktyczny (Phase 5, 2026-09-26)

- **Grafika gry:** `client/assets/tilesets/` (teren 32×32, ściany 32×48,
  obiekty w komórkach 64×64) i `client/assets/sprites/` (postacie: klatki
  32×48, rzędy dół/lewo/prawo/góra, kolumny: stoi / krok A / krok B).
  Wszystko **generowane kodem** przez `tools/art/gen_tileset.py` — po zmianie
  generatora: `python3 tools/art/gen_tileset.py`. Pochodzenie i licencje:
  `client/assets/CREDITS.md`.
- **Wzór stylu:** CC0 "Tibia-style RPG Tileset" (Summer Engine) — tylko
  palety i faktury; to jeden obraz AI, nie gotowe kafle, więc nie jest w grze.
- **Definicje:** `client/data/tiles.json` — każdy kafel/obiekt: arkusz,
  komórka, `walkable`, opcjonalnie animacja (`frames`, `fps`) lub autotile
  ścian (`top`/`front`). **Serwer czyta z niego tylko `walkable`**, klient
  grafikę — jedno źródło prawdy.
- **Mapy:** `client/data/maps/*.json` — warstwy `ground` i `objects` jako
  wiersze jednoznakowych kodów + `legend`, `spawn`. Edytowalne ręcznie.
- **Zasada rysowania:** spód-środek każdej grafiki na spodzie-środku kafla;
  wyższe grafiki (ściany, drzewa, postacie) wystają w górę — rzut skośny.
  Obiekty i postacie są sortowane po Y (postać za drzewem jest zasłaniana).
- Klatki animacji kafla muszą leżeć obok siebie w jednym rzędzie atlasu
  (wymóg `TileSetAtlasSource` w Godocie).

## Docelowy pipeline (do zbudowania, nie istnieje jeszcze)

1. **Sprite'y postaci:** warstwowy system (baza postaci + nakładki
   broń/pancerz/hełm/buty/tarcza), sprite sheet per animacja (idle, walk,
   atak, obrażenia, śmierć) — **4 kierunki, siatka 32×32, rzut skośny jak w Tibii** (GAME_DESIGN.md). Rozdzielczość i format do ustalenia
   przy Phase 6 na podstawie rzeczywistych testów wydajności na Androidzie.
2. **Potwory:** własny sprite + animacje (idle/ruch/atak/obrażenia/śmierć)
   per typ, nie reused/przeskalowany jeden model.
3. **Tileset świata:** kafle 32×32 (rzut skośny) + dekoracje (drzewa, kamienie, woda,
   budynki) zaprojektowany tak, by unikać widocznej powtarzalności — patrz
   GAME_DESIGN.md.
4. **UI:** ikony akcji, ramki, HUD — jako `Control`-based UI w Godocie.

## Narzędzia ocenione w tym środowisku

- **Generowanie grafiki AI:** w tej sesji dostępne jest narzędzie do
  generowania obrazów (Higgsfield). Może być użyte do concept art i
  pierwszych wersji sprite'ów — **każdy wygenerowany asset wymaga ręcznej
  kontroli jakości i spójności stylu** przed wejściem do gry (patrz reguła
  w GAME_DESIGN.md o oryginalności). Nie zostało jeszcze użyte — do
  zastosowania przy konkretnej potrzebie (np. "potrzebuję 5 wariantów
  potwora X"), nie z góry, "na zapas".
- **Godot 4.3** (zainstalowany lokalnie w tym kontenerze, headless) — ma
  wbudowany import pipeline dla PNG/sprite sheetów, `AnimatedSprite2D`/
  `AnimationPlayer`. Nie wymaga zewnętrznego narzędzia do samego importu.
- Zewnętrzne edytory sprite'ów (Aseprite, itp.) — nie zainstalowane w tym
  kontenerze (brak GUI); praca graficzna wymaga narzędzi lokalnych
  użytkownika albo generatorów AI używanych z tej sesji.

## Zasada

Każdy asset przed wejściem do repo: nazwany sensownie, w odpowiednim
katalogu (`client/assets/...` — katalog do utworzenia przy pierwszym realnym
asseecie, nie teraz "na zapas"), zoptymalizowany pod Android (rozmiar
tekstury, atlas), i przechodzi kontrolę z GAME_DESIGN.md/pkt "Zasada
oryginalności".

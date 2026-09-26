# ASSET_PIPELINE.md

## Stan faktyczny

Aktualnie w projekcie istnieją wyłącznie **techniczne placeholdery**:
kolorowe prostokąty (`client/scripts/player_entity.gd`) reprezentujące
encje, i placeholder SVG jako ikona projektu. To jest zgodne z zasadą "nie
udawaj że coś jest gotowe" — te placeholdery są jawnie oznaczone w kodzie i
**nie wolno** ich pokazywać jako finalnej grafiki w żadnym publicznym
milestone.

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

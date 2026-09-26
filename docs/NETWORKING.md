# NETWORKING.md

## Stan faktyczny (co jest zaimplementowane i zweryfikowane)

- Serwer: TCP, POSIX sockets, `server/src/net/`. Nasłuchuje na porcie
  (domyślnie `7777`, konfigurowalny przez argument/env — patrz kod).
  Protokół tekstowy, linia = jedna komenda, `\n`-terminowana (prosty do
  debugowania na tym etapie; do rewizji przy realnym obciążeniu — patrz
  "Do zdecydowania" poniżej).
- Komendy klient→serwer:
  - `MOVE <dx> <dy>` — intencja ruchu. Serwer **waliduje** (odległość na tick
    nie może przekroczyć maksymalnej prędkości) i tylko wtedy aktualizuje
    pozycję. To pierwszy, minimalny element anty-cheatu: **serwer nigdy nie
    przyjmuje pozycji od klienta, tylko intencję ruchu**.
- Komendy serwer→klient:
  - `WELCOME <entity_id>` — przydzielony identyfikator encji po połączeniu.
  - `POS <entity_id> <x> <y>` — stan pozycji encji, rozsyłany do wszystkich
    klientów po każdym ticku, w którym coś się zmieniło.
  - `LEAVE <entity_id>` — encja rozłączona.
- Zweryfikowane (patrz PROJECT_STATE.md): serwer akceptuje wielu klientów
  równocześnie, waliduje ruch, rozsyła pozycje. Testowane skryptem symulującym
  klienta (nie przez Godot — patrz ograniczenia środowiska w TECH_STACK.md).
- Klient (Godot): `client/net/` łączy się przez `StreamPeerTCP`, wysyła `MOVE`,
  odbiera i stosuje `POS`/`WELCOME`/`LEAVE`.

## Do zdecydowania (nie zgaduj, eskaluj przy realnej potrzebie)

- **TCP vs UDP:** TCP wybrany na start dla prostoty i pewności dostarczenia
  podczas budowy fundamentu. Przy realnym ruchu wielu graczy TCP head-of-line
  blocking może być problemem — do zmiany na UDP (+ własna warstwa
  potwierdzeń dla akcji krytycznych jak combat) w Phase 15 (Optymalizacja),
  na podstawie rzeczywistych pomiarów, nie przed nimi.
- **Format pakietów:** tekstowy protokół jest czytelny i łatwy do debugowania
  teraz, ale nieefektywny (parsing stringów) i podatny na błędy formatu przy
  rozroście liczby komend. Do rewizji na binarny/TLV przy dodawaniu combat/
  inventory (Phase 7+), gdy liczba typów pakietów wzrośnie.
- **Szyfrowanie transportu:** brak. Do dodania przed jakimkolwiek testem z
  realnymi kontami/hasłami (Phase 4) — TLS albo własny handshake, decyzja
  wtedy.
- **Reconnect/disconnect handling:** obecnie rozłączenie = usunięcie encji.
  Brak zachowania stanu sesji na reconnect — do Phase 4.

## Zasada anty-cheat (patrz też KNOWN_ISSUES.md)

Klient wysyła tylko **intencje** (`MOVE dx dy`, później `ATTACK target_id`,
`USE_ITEM item_id`, itd.) — nigdy wynik. Serwer jest jedynym miejscem, które
zapisuje pozycję/HP/przedmioty/złoto. Każdy nowy typ pakietu klient→serwer
musi być projektowany z tą zasadą, nie jako wyjątek.

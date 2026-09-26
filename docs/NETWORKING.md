# NETWORKING.md

## Stan faktyczny (co jest zaimplementowane i zweryfikowane)

- Serwer: TCP + **TLS 1.2+ (OpenSSL)**, `server/src/net/`. Domyślny port
  `7777` (argument — patrz BUILD.md). Klient bez TLS nie dostaje żadnej
  odpowiedzi protokołu (połączenie zamykane przy nieudanym handshake'u).
- Protokół tekstowy, linia = jedna komenda, `\n`-terminowana, max 1024 B na
  niedokończoną linię (dłuższa → rozłączenie).
- Weryfikacja: `tools/test_client.py` (34 asercje, w tym bezpieczeństwo) +
  prawdziwy klient Godot (patrz PROJECT_STATE.md).

## Maszyna stanów połączenia

```
TLS handshake ──► NIEZALOGOWANY ──REGISTER/LOGIN ok──► ZALOGOWANY ──CHAR_SELECT ok──► W ŚWIECIE
                    │  (60 s na zalogowanie,                │                            │
                    │   inaczej rozłączenie)                 │ CHAR_LIST / CHAR_CREATE    │ STEP
```

Każdy handler sam sprawdza stan — komenda spoza swojego stanu (np. `STEP`
przed wyborem postaci, `CHAR_LIST` przed logowaniem) jest **ignorowana bez
odpowiedzi**, nie powoduje błędu. Zmiana postaci = nowe połączenie.

## Komendy klient → serwer

| Komenda | Stan | Uwagi |
|---|---|---|
| `REGISTER <login> <hasło>` | niezalogowany | login 3-32 znaki `[A-Za-z0-9_]`, hasło 6-128 znaków (reszta linii — może mieć spacje). Sukces = od razu zalogowany. |
| `LOGIN <login> <hasło>` | niezalogowany | |
| `CHAR_LIST` | zalogowany | |
| `CHAR_CREATE <nazwa>` | zalogowany | 3-20 znaków, litery/cyfry/spacje (nie na brzegach); nazwy globalnie unikalne |
| `CHAR_SELECT <id>` | zalogowany | serwer sprawdza, że postać należy do konta |
| `STEP <N\|E\|S\|W\|NE\|SE\|SW\|NW>` | w świecie | intencja kroku o 1 kafel (8 kierunków; dokładnie jeden token — `STEP N E` jest ignorowany). Serwer trzyma **najwyżej jeden** oczekujący krok (kolejne go zastępują) i wykonuje go, gdy minie czas poprzedniego kroku, cel jest przechodni wg mapy i nie stoi na nim inna postać. Wejście w przeszkodę tylko obraca postać. |

## Komendy serwer → klient

| Komenda | Znaczenie |
|---|---|
| `AUTH_OK` | zalogowano / zarejestrowano |
| `AUTH_FAIL <powód>` | `invalid_input`, `username_taken`, `bad_credentials`, `rate_limited`, `server_error` |
| `CHARS <id>:<nazwa> ...` | lista postaci konta (pusta = samo `CHARS`). Nazwy mogą mieć spacje, ale nigdy `:` — klient skleja tokeny bez prefiksu `<id>:` z poprzednią nazwą. |
| `CHAR_CREATED <id> <nazwa>` / `CHAR_CREATE_FAIL <powód>` | `invalid_name`, `name_taken` |
| `CHAR_SELECT_FAIL <powód>` | `invalid_id`, `not_found` (to samo dla "nie istnieje" i "cudza postać") |
| `WELCOME <entity_id> <step_ms> <diagonal_step_ms>` | wejście do świata + czas kroku prostego (dziś 250 ms) i po skosie (354 ms = 250·√2); klient animuje krok w tym czasie; zaraz po nim snapshot `POS` wszystkich obecnych encji |
| `POS <entity_id> <x> <y> <kierunek>` (jeden z 8 jak w `STEP`) | kafel (liczby całkowite) i kierunek, w który patrzy postać; wysyłane, gdy się zmieniły |
| `LEAVE <entity_id>` | encja opuściła świat |

`POS`/`LEAVE` trafiają **wyłącznie do połączeń w świecie** — nie do
niezalogowanych ani zalogowanych bez wybranej postaci (inaczej każdy mógłby
śledzić ruchy graczy bez konta).

`entity_id` jest ulotny (per sesja w świecie); trwałą tożsamością jest
`character id` z bazy.

## Bezpieczeństwo protokołu

- **TLS:** serwer ładuje certyfikat + klucz PEM (BUILD.md). Klient Godot
  **zawsze weryfikuje** serwer: przypięty certyfikat (`res://certs/dev_server.crt`)
  albo systemowe CA — nie ma trybu "bez weryfikacji" (patrz KNOWN_ISSUES.md).
- **Hasła:** tylko scrypt + sól w bazie, porównanie `CRYPTO_memcmp` (stały czas).
- **Throttling uwierzytelniania (per połączenie):** max 1 próba
  `REGISTER`/`LOGIN` na sekundę (nadmiar → `rate_limited` bez liczenia scrypt),
  5 nieudanych prób → rozłączenie.
- **Limity zasobów:** 1024 B na niedokończoną linię, 64 KB bufora wyjściowego
  (klient, który nie czyta, jest rozłączany), 60 s na zalogowanie, deskryptory
  ≥ `FD_SETSIZE` odrzucane (ograniczenie `select()`).

## Ruch po kratkach (Phase 5)

Pozycje są całkowitymi współrzędnymi kafli mapy (`client/data/maps/start.json`,
40×30). Serwer wczytuje tę samą mapę co klient i sam decyduje o
przechodniości. Nowa postać (i każda zapisana pozycja, która stała się
nieprawidłowa) trafia na najbliższy wolny kafel od punktu startowego mapy.
Jedna postać na kafel. **8 kierunków** (decyzja użytkownika). Reguły skosu:
krok po skosie trwa √2 dłużej (354 ms) — skos nie jest skrótem; **nie
można ścinać rogów**: oba kafle obok skosu muszą być przechodnie wg mapy
(ściana, drzewo, woda blokują), ale stojąca tam inna postać nie blokuje
(można obejść gracza po skosie). Obie reguły liczy serwer
(`World::canStep`).

## Reconnect — zakres

"Reconnect" = gracz łączy się ponownie, loguje i wybiera tę samą postać;
serwer odtwarza ją w zapisanej pozycji. Pozycja zapisywana przy rozłączeniu
i przy łagodnym zamknięciu serwera (SIGINT/SIGTERM). **Nie** ma wznawiania
przerwanej sesji bez ponownego logowania (tokeny sesji) — świadomie, do
rozważenia przy realnej potrzebie (np. częste zrywanie połączeń na mobile).

## Do zdecydowania (nie zgaduj, eskaluj przy realnej potrzebie)

- **TCP vs UDP:** TCP na start. Head-of-line blocking przy wielu graczach — do
  rewizji w Phase 15 na podstawie pomiarów.
- **Format pakietów:** tekstowy — czytelny, ale kruchy (np. lista `CHARS` z
  nazwami ze spacjami). Do rewizji na binarny/TLV przy Phase 7+.
- **`select()` → poll/epoll:** limit 1024 deskryptorów i O(n) na tick.
- **Tokeny sesji do szybkiego reconnectu** — patrz wyżej.

## Zasada anty-cheat (patrz też KNOWN_ISSUES.md)

Klient wysyła tylko **intencje** (`STEP dir`, `CHAR_SELECT id`, później
`ATTACK target_id`, itd.) — nigdy wynik. Serwer jest jedynym miejscem, które
zapisuje pozycję/HP/przedmioty/złoto, i nigdy nie ufa identyfikatorom od
klienta bez sprawdzenia własności (np. `CHAR_SELECT` cudzej postaci).

# KNOWN_ISSUES.md

Szczere ograniczenia obecnego stanu — nie naprawiać "przy okazji" bez
związanego zadania; naprawiać w fazie, do której należą (patrz ROADMAP.md).

## Bezpieczeństwo / anty-cheat

- **Scrypt blokuje pętlę gry.** Serwer jest jednowątkowy, a każde
  `REGISTER`/`LOGIN` liczy scrypt ~50 ms — w tym czasie nikt nie dostaje
  ticków. Throttling per połączenie (1 próba/s, 5 porażek → rozłączenie)
  ogranicza jedno połączenie, ale **wiele równoległych połączeń nadal może
  spowolnić serwer dla wszystkich**. Właściwa naprawa: hashowanie w wątku
  roboczym + globalny limit — przed jakimkolwiek publicznym serwerem.
- **Istnienie loginu da się ustalić.** `REGISTER` odpowiada
  `username_taken` (standardowy UX w MMO), a `LOGIN` dla nieistniejącego
  konta odpowiada szybciej (nie liczy scrypt). Zaakceptowane — obrona przed
  brute-force opiera się na throttlingu, nie na ukrywaniu loginów.
- **Nieograniczone tworzenie kont i postaci.** Brak limitu kont z jednego
  źródła i limitu postaci na konto — to decyzje projektowe (ile postaci na
  konto?) + anty-abuse (Phase 16). Nie wymyślono tu wartości.
- **Certyfikat dev jest self-signed (CN=localhost).** Klient go przypina, więc
  połączenie jest zweryfikowane — ale każdy deweloper generuje własny i musi
  go skopiować do klienta przed eksportem (BUILD.md). Publiczny serwer
  wymaga prawdziwej domeny + certyfikatu z CA (klient wtedy weryfikuje przez
  systemowe CA, bez przypinania).
- **Obserwacja, nie diagnoza:** w Godot 4.3.stable `TLSOptions.client_unsafe()`
  i tak przerywał handshake z naszym serwerem (`x509_verify_cert -0x2700`).
  Nie ustalono przyczyny (podejrzenie: obsługa opcjonalnej weryfikacji w
  TLS 1.3 w dołączonym mbedTLS). Bez znaczenia w praktyce — klient celowo
  nie ma trybu bez weryfikacji.
- **Hasło w argumentach linii poleceń** (`--password=` w kliencie) jest
  widoczne dla innych procesów — wyłącznie do testów/dev, nigdy dla
  prawdziwego konta.

## Persystencja

- Pozycja zapisywana tylko przy rozłączeniu gracza i przy łagodnym zamknięciu
  serwera (SIGINT/SIGTERM). **Crash lub `kill -9` = utrata ruchu od
  zalogowania** dla graczy online. Okresowy zapis — gdy pojawi się więcej
  stanu do zapisywania (Phase 6+).
- Brak migracji schematu — `CREATE TABLE IF NOT EXISTS`. Pierwsza zmiana
  schematu musi wprowadzić wersjonowanie (np. `PRAGMA user_version`) +
  backup przed migracją.
- Brak backupów bazy (to dev). Wymagane przed jakimikolwiek prawdziwymi
  danymi graczy.

## Sieć

- **`select()`**: max 1024 deskryptorów (wyższe są odrzucane, nie psują
  pamięci), O(n) na tick, jeden `accept()` na tick. Do zmiany na poll/epoll
  przy realnej liczbie graczy.
- **Format tekstowy protokołu** — do rewizji (binarny/TLV) przy combat/
  inventory. Już teraz kruchy w miejscu listy `CHARS` (patrz NETWORKING.md).
- `WANT_WRITE` przy TLS nie jest śledzony osobnym zestawem `select()` — zapis
  jest ponawiany w następnym ticku (max ~50 ms opóźnienia). Wystarczające przy
  obecnym ruchu.
- Serwer loguje przez `std::cout` (buforowany przy przekierowaniu do pliku) —
  kosmetyczne.
- Brak interpolacji ruchu po stronie klienta — przy realnym jitterze ruch
  innych graczy będzie "szarpał". Do dodania, gdy zaobserwowane.
- Brak tokenów sesji: po zerwaniu połączenia trzeba zalogować się ponownie
  (patrz NETWORKING.md, "Reconnect — zakres").

## Mapa / grafika

- Przejścia terenu są kanciaste (np. brzeg jeziora schodkami po kaflach) —
  brak kafli przejściowych/autotilingu terenu. Do dodania w generatorze
  grafiki.
- Ruiny: ciemna podłoga i ciemne ściany mają mały kontrast — do poprawy
  palety.
- Jedna mapa (`start.json`), brak pięter (z) i przejść między mapami —
  "system modułowy map" z ROADMAP (Phase 5) jeszcze nie istnieje.
- Ruch po skosie pokazuje sprite boczny (arkusz ma 4 kierunki). Osobne
  klatki skosu — jeśli grafik uzna, że są potrzebne.
- Na telefonie nie ma jeszcze wirtualnego joysticka — 8 kierunków działa z
  klawiatury (dwa klawisze naraz) i z gałki pada; przyciski dotykowe to
  osobne zadanie UI.
- Tryb ekranu to portret (720×1280) — przy widoku 15 kafli wszerz widać ~26
  w pionie. Czy gra ma być w poziomie (jak Tibia na PC) — do decyzji.

## Klient / UI
- Ekran logowania nie był jeszcze używany przez człowieka ani na telefonie —
  sprawdzony tylko automatycznie (sterowanie z harnessu) i wizualnie
  (zrzut ekranu przez Xvfb). Brak m.in. marginesów panelu, zapamiętywania
  adresu serwera i loginu między uruchomieniami.
- Adres serwera domyślnie `127.0.0.1` — na telefonie trzeba wpisać adres PC w
  sieci lokalnej i nacisnąć "Połącz ponownie" (pierwsza próba połączenia
  z 127.0.0.1 na telefonie się nie uda, co odsłania ten przycisk).

## Sterowanie padem

- Ruch działa z padem przez domyślne bindowanie `ui_left/right/up/down` —
  zweryfikowane symulacją zdarzenia, nie fizycznym padem. Test na sprzęcie:
  po stronie użytkownika.
- Brak: przycisków akcji (nie ma jeszcze systemu akcji, Phase 7), tuningu
  deadzone, wibracji.

## Grafika

- Grafika jest pierwszą wersją z generatora (`tools/art/gen_tileset.py`):
  jeden wygląd postaci (niebieski = Ty, czerwony = inni), brak potworów,
  animacji ataku i przedmiotów. Dobry grafik zrobi to lepiej — generator
  jest punktem startowym, nie wersją finalną.

## Android

- APK budowany i weryfikowany (manifest, podpis, zawartość), ale **nigdy nie
  uruchomiony na urządzeniu** — brak `/dev/kvm` w kontenerze. Test po stronie
  użytkownika.

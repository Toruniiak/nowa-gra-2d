# KNOWN_ISSUES.md

Szczere ograniczenia obecnego stanu — nie naprawiać "przy okazji" bez
związanego zadania; naprawiać w fazie, do której należą (patrz ROADMAP.md).

## Bezpieczeństwo / anty-cheat

- **Brak rate-limitingu pakietów.** Serwer waliduje wielkość *pojedynczego*
  ruchu (`kMaxMovePerTick`), ale nie ogranicza, jak często klient może
  wysyłać `MOVE`. Złośliwy klient wysyłający pakiety szybciej niż tick
  serwera może realnie ruszać się szybciej niż zamierzona prędkość. Do
  naprawy w Phase 16 (Security) — albo wcześniej, jeśli okaże się problemem
  przy pierwszych testach wieloosobowych.
- **Brak szyfrowania transportu.** Zwykły TCP, plaintext. Akceptowalne teraz
  (brak kont/haseł w systemie), ale musi być rozwiązane przed Phase 4
  (Login) — nie później.
- **Brak uwierzytelniania połączeń.** Każdy, kto się połączy, dostaje
  encję. Do zmiany razem z systemem kont.

## Sieć

- **Format tekstowy protokołu** jest nieefektywny i będzie wymagał rewizji
  (binarny/TLV) przy większej liczbie typów pakietów (combat, inventory).
- Serwer loguje przez `std::cout`, który jest w pełni buforowany przy
  przekierowaniu do pliku — logi mogą się pojawić z opóźnieniem/dopiero po
  zamknięciu procesu. Kosmetyczne, ale warto dodać `std::endl`/flush albo
  `std::ios::sync_with_stdio` przy pierwszym realnym debugowaniu produkcyjnym.
- Brak interpolacji ruchu po stronie klienta — pozycje są ustawiane
  bezpośrednio (`set_server_position`). Przy prawdziwym jitterze sieciowym
  (nie loopback) ruch innych graczy będzie się "szarpał". Do dodania, gdy
  realnie zaobserwowane, nie przed.

## Sterowanie padem

- Ruch działa z padem (D-pad/lewy analog) przez domyślne bindowanie
  silnika do `ui_left/right/up/down` — zweryfikowane symulacją zdarzenia
  `InputEventJoypadMotion` w tej sesji (nie testem na fizycznym padzie
  Bluetooth, którego tu nie ma). Test na prawdziwym sprzęcie: po stronie
  użytkownika.
- Nie zaimplementowano: przypisania przycisków pada do akcji (atak/użycie
  itemu) — nie istnieje jeszcze system akcji do zbindowania (Phase 7).
  Nie zaimplementowano: tuningu deadzone ponad wartość domyślną silnika,
  rumble/vibration feedback, UI wyboru "touch vs. gamepad" (na razie działają
  jednocześnie, bez konfliktu, bo to jedna ścieżka `Input.get_vector`).

## Grafika

- Wszystkie encje renderowane jako kolorowe prostokąty. **Placeholder — patrz
  ASSET_PIPELINE.md.** Nie pokazywać jako finalnej grafiki.

## Android

- Android SDK/NDK nie zainstalowane w tym kontenerze — `dl.google.com`
  zablokowany na dzień 2026-09-26. Patrz PROJECT_STATE.md.
- Brak `/dev/kvm` w tym kontenerze → nawet po instalacji SDK, **emulator
  Androida nie zadziała tutaj**. Test na urządzeniu wymaga maszyny
  użytkownika.

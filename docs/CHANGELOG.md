# CHANGELOG.md

## 2026-09-26

- Audyt środowiska (Phase 0): brak Android SDK/NDK, brak `/dev/kvm`,
  `dl.google.com` zablokowany przez politykę sieciową kontenera.
- Ustalono stos: serwer C++20/CMake (autorytatywny), klient Godot 4.3
  (Android + desktop).
- Utworzono szkielet monorepo: `server/`, `client/`, `docs/`.
- Zaimplementowano i **zweryfikowano end-to-end** minimalny protokół
  sieciowy: `WELCOME`/`MOVE`/`POS`/`LEAVE`, z walidacją ruchu po stronie
  serwera (klamrowanie do `kMaxMovePerTick`). Test: realny serwer + realny
  headless klient Godot + skrypt symulujący próbę cheatowania.
- Dodano pełną dokumentację projektu (ten plik i pliki w `docs/`).
- Nieukończone: instalacja Android SDK (zablokowana sieciowo), pierwszy
  `.apk`.

## 2026-09-26 (2)

- Dodano obsługę pada (Bluetooth na Androidzie, wired na desktopie).
  Zweryfikowano: `InputMap` domyślnie binduje `ui_left/right/up/down` do
  D-pada i lewego analoga (dump `InputMap.action_get_events`); symulacja
  `InputEventJoypadMotion` w headless Godocie potwierdziła, że
  `Input.get_vector(...)` — czyli ta sama funkcja, którą już używał
  `world.gd` dla klawiatury — reaguje identycznie na pada. Brak dodatkowego
  kodu specyficznego dla gamepada w ścieżce ruchu.
- Dodano `client/scripts/gamepad_status.gd`: wykrywanie połączenia/
  rozłączenia pada w czasie rzeczywistym (`Input.joy_connection_changed`),
  fundament pod przyszły wskaźnik w HUD.
- Nieukończone: przypisanie przycisków akcji pada (nie istnieje jeszcze
  system akcji), test na fizycznym padzie Bluetooth (wymaga urządzenia
  użytkownika).

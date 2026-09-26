# BUILD.md

## Serwer (C++)

```bash
cd server
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug   # albo Release
cmake --build build
./build/server [port]     # domyślny port: 7777
```

Wymagania: kompilator C++20 (g++/clang++), CMake ≥ 3.20. Brak zewnętrznych
zależności (patrz TECH_STACK.md) — tylko POSIX sockets.

## Klient (Godot)

Wymaga Godot 4.3 (edytor lub headless binary).

```bash
godot4 --path client --editor        # otwiera edytor (wymaga GUI)
godot4 --headless --path client --import   # walidacja bez GUI (CI-friendly)
godot4 --headless --path client --quit-after 60 -- --server-host=127.0.0.1 --server-port=7777
```

Domyślny host/port: `127.0.0.1:7777` — zmienialny w `World.tscn` (export
vars `server_host`/`server_port`) albo argumentami `--server-host=`/
`--server-port=` (patrz `client/scripts/world.gd`).

## Test end-to-end (bez GUI, weryfikuje protokół)

```bash
./server/build/server 7788 &
python3 tools/test_client.py 7788
```

`tools/test_client.py` to skrypt diagnostyczny (nie część gry) symulujący
klienta — wysyła normalny ruch, próbę "cheatowania" (za duży skok) i
zniekształcone dane, sprawdzając że serwer poprawnie waliduje/nie crashuje.

## Android (`.apk`) — NIEUKOŃCZONE

Wymaga Android SDK + NDK + eksport template'ów Godota. W tym środowisku
(kontener chmurowy) `dl.google.com` był zablokowany politykę sieciową w
momencie pisania tego pliku — patrz PROJECT_STATE.md dla aktualnego stanu.
Instrukcja zostanie uzupełniona, gdy pierwszy build faktycznie się powiedzie
— nie wcześniej (żeby nie opisywać kroków, które nie zostały zweryfikowane).

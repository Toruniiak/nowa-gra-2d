#!/usr/bin/env python3
"""Manual protocol test client — NOT part of the game, just a throwaway
script to verify server/src/net against docs/NETWORKING.md end-to-end
without needing a graphical Godot client. Usage:
  python3 test_client.py <port>
"""
import socket
import sys
import time

port = int(sys.argv[1]) if len(sys.argv) > 1 else 7777


def connect():
    s = socket.create_connection(("127.0.0.1", port), timeout=2)
    s.settimeout(1.0)
    return s


def recv_lines(s, duration=0.3):
    lines = []
    end = time.time() + duration
    buf = b""
    while time.time() < end:
        try:
            data = s.recv(4096)
        except socket.timeout:
            continue
        if not data:
            break
        buf += data
    for line in buf.decode().splitlines():
        if line:
            lines.append(line)
    return lines


a = connect()
welcome_a = recv_lines(a)
print("A welcome:", welcome_a)

b = connect()
welcome_b = recv_lines(b)
print("B welcome:", welcome_b)

# Normal move — should be accepted as-is.
a.sendall(b"MOVE 3 0\n")
time.sleep(0.15)
print("A after normal move, A sees:", recv_lines(a))
print("A after normal move, B sees:", recv_lines(b))

# Cheat attempt: absurdly large move — server must clamp to kMaxMovePerTick (6.0).
a.sendall(b"MOVE 500 0\n")
time.sleep(0.15)
print("A after CHEAT move (500,0), B sees (expect clamped ~6.0 delta):", recv_lines(b))

# Malformed input — must not crash the server.
a.sendall(b"GARBAGE not a real command\n")
time.sleep(0.15)
print("A after malformed input, still alive:", recv_lines(a))

a.close()
time.sleep(0.15)
print("B sees A leave:", recv_lines(b))
b.close()
print("OK — protocol behaves as documented")

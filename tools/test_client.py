#!/usr/bin/env python3
"""Manual protocol test client — NOT part of the game, just a throwaway
script to verify server/src/net against docs/NETWORKING.md end-to-end
without needing a graphical Godot client. Usage:
  python3 test_client.py <port> [db_path] [cert_path]

cert_path (default server/certs/server.crt) is pinned: the connection is
verified against it, exactly like the Godot client does.

Covers: TLS handshake, register/login (incl. rejected duplicate username and
wrong password), character create/select, movement + anti-cheat clamping,
malformed input, disconnect/reconnect with persisted position, world
snapshot on join, and the negative/security cases: a character cannot be
selected by an account that doesn't own it, world state is not sent to
unauthenticated connections, commands before login are ignored, auth is
rate-limited and repeated failures disconnect, oversized lines disconnect,
plaintext (non-TLS) clients are rejected, and — if db_path is given — no
plaintext password is stored in the database.
"""
import select
import socket
import sqlite3
import ssl
import sys
import time
import uuid

port = int(sys.argv[1]) if len(sys.argv) > 1 else 7777
db_path = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] else None
cert_path = sys.argv[3] if len(sys.argv) > 3 else "server/certs/server.crt"

# Verified TLS, pinned to the dev server's self-signed certificate — the
# same policy as the Godot client (it has no "skip verification" mode).
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ctx.load_verify_locations(cert_path)


def connect():
    raw = socket.create_connection(("127.0.0.1", port), timeout=2)
    s = ctx.wrap_socket(raw, server_hostname="localhost")
    s.settimeout(1.0)
    return s


def recv_lines(s, duration=0.3):
    # Non-blocking + select(), not a blocking recv() with a socket timeout:
    # under TLS 1.3, SSL_read can consume post-handshake NewSessionTicket
    # records, report "want read" while the kernel socket is already
    # drained, and a blocking recv() then sleeps out its whole timeout even
    # though the server replied in ~1 ms. Measured server round-trips:
    # REGISTER ~50 ms (scrypt), CHAR_LIST/CHAR_CREATE < 2 ms.
    s.setblocking(False)
    buf = b""
    end = time.time() + duration
    while time.time() < end:
        select.select([s], [], [], 0.01)
        try:
            data = s.recv(4096)
        except ssl.SSLWantReadError:
            continue
        except (ConnectionResetError, ssl.SSLError, OSError):
            break
        if not data:
            break
        buf += data
    s.settimeout(1.0)
    return [line for line in buf.decode().splitlines() if line]


def is_closed_by_server(s, wait=0.5):
    s.setblocking(False)
    end = time.time() + wait
    while time.time() < end:
        select.select([s], [], [], 0.01)
        try:
            if s.recv(4096) == b"":
                return True
        except ssl.SSLWantReadError:
            continue
        except (ConnectionResetError, BrokenPipeError, ssl.SSLError, OSError):
            return True
    return False


def check(label, ok, detail=""):
    print(f"{'OK ' if ok else 'FAIL'} {label}: {detail}")
    if not ok:
        sys.exit(1)


def expect(label, lines, needle):
    check(label, any(needle in line for line in lines), lines)


def expect_none(label, lines, needle):
    check(label, not any(needle in line for line in lines), lines)


# Unique per run so repeated test invocations against a persistent
# database don't collide on username/character-name uniqueness.
run_id = uuid.uuid4().hex[:8]
user_a, user_b = f"alice_{run_id}", f"bob_{run_id}"
password = "correct horse battery"
char_a, char_b = f"CharA{run_id}"[:20], f"CharB{run_id}"[:20]

# --- Plaintext (non-TLS) client must be rejected, and must not hurt the server ---
plain = socket.create_connection(("127.0.0.1", port), timeout=2)
plain.sendall(b"LOGIN someone whatever\n")
plain.settimeout(1.0)
try:
    reply = plain.recv(4096)
except (socket.timeout, ConnectionResetError):
    reply = b""
plain.close()
check("plaintext client gets no protocol reply", b"AUTH" not in reply, reply[:60])

# --- Registration ---
a = connect()
b = connect()
a.sendall(f"REGISTER {user_a} {password}\n".encode())
expect("A register", recv_lines(a), "AUTH_OK")

# Duplicate-username check needs a *fresh* connection: an already-registered
# connection silently ignores a second REGISTER (see server/src/net/server.cpp
# handleRegister — "already authenticated on this connection" is a no-op).
dup = connect()
dup.sendall(f"REGISTER {user_a} {password}\n".encode())
expect("duplicate register rejected", recv_lines(dup), "AUTH_FAIL username_taken")
dup.close()

inj = connect()
inj.sendall(f"REGISTER x';DROP_TABLE_accounts;-- {password}\n".encode())
expect("injection-shaped username rejected", recv_lines(inj), "AUTH_FAIL invalid_input")
inj.close()

b.sendall(f"REGISTER {user_b} {password}\n".encode())
expect("B register", recv_lines(b), "AUTH_OK")
a.close()
b.close()

# --- Commands before login are ignored (no reply, no world entry) ---
pre = connect()
pre.sendall(b"MOVE 5 5\nCHAR_LIST\nCHAR_SELECT 1\n")
check("commands before login ignored", recv_lines(pre) == [], "no reply")
pre.close()

# --- Auth throttling: burst of bad logins -> 1 real check, then rate-limited, then disconnect ---
brute = connect()
brute.sendall((f"LOGIN {user_a} wrong-password\n" * 5).encode())
brute_lines = recv_lines(brute)
check("burst: exactly one real password check",
      sum("bad_credentials" in l for l in brute_lines) == 1, brute_lines)
check("burst: remaining attempts rate-limited",
      sum("rate_limited" in l for l in brute_lines) == 4, brute_lines)
check("burst: connection dropped after 5 failures", is_closed_by_server(brute))
brute.close()

# --- Oversized line without newline -> disconnect ---
big = connect()
big.sendall(b"A" * 2000)
check("oversized line drops the connection", is_closed_by_server(big, wait=1.0))
big.close()

# --- Login ---
a = connect()
a.sendall(f"LOGIN {user_a} wrong-password\n".encode())
expect("A login with wrong password rejected", recv_lines(a), "AUTH_FAIL bad_credentials")
a.close()

a = connect()
b = connect()
a.sendall(f"LOGIN {user_a} {password}\n".encode())
expect("A login", recv_lines(a), "AUTH_OK")
b.sendall(f"LOGIN {user_b} {password}\n".encode())
expect("B login", recv_lines(b), "AUTH_OK")

# --- Characters ---
a.sendall(b"CHAR_LIST\n")
expect("A char list empty", recv_lines(a), "CHARS")

a.sendall(f"CHAR_CREATE {char_a}\n".encode())
create_a = recv_lines(a)
expect("A char create", create_a, "CHAR_CREATED")
char_a_id = create_a[0].split()[1]

b.sendall(f"CHAR_CREATE {char_b}\n".encode())
create_b = recv_lines(b)
expect("B char create", create_b, "CHAR_CREATED")
char_b_id = create_b[0].split()[1]

a.sendall(f"CHAR_SELECT {char_b_id}\n".encode())
expect("A cannot select B's character", recv_lines(a), "CHAR_SELECT_FAIL")

a.sendall(f"CHAR_SELECT {char_a_id}\n".encode())
welcome_a = recv_lines(a)
expect("A char select", welcome_a, "WELCOME")
entity_a = next(l for l in welcome_a if l.startswith("WELCOME")).split()[1]

# B is logged in but hasn't selected a character: it must NOT see world state.
a.sendall(b"MOVE 1 0\n")
time.sleep(0.15)
recv_lines(a)
expect_none("logged-in but not playing: no world state", recv_lines(b), "POS")

# Unauthenticated observer (TLS only, no login) must not see world state either.
spy = connect()
a.sendall(b"MOVE 2 0\n")
time.sleep(0.15)
expect_none("unauthenticated observer: no world state", recv_lines(spy), "POS")
spy.close()

b.sendall(f"CHAR_SELECT {char_b_id}\n".encode())
welcome_b = recv_lines(b)
expect("B char select", welcome_b, "WELCOME")
entity_b = next(l for l in welcome_b if l.startswith("WELCOME")).split()[1]
# A is standing still at (3, 0): B must learn about it from the join snapshot.
expect("B join snapshot includes idle A", welcome_b, f"POS {entity_a} 3")

# --- Movement + anti-cheat (same assertions as before TLS/accounts existed) ---
recv_lines(a)
a.sendall(b"MOVE 3 0\n")
time.sleep(0.15)
expect("normal move seen by B", recv_lines(b), f"POS {entity_a} 6")

a.sendall(b"MOVE 500 0\n")
time.sleep(0.15)
expect("cheat move (500,0) clamped to +6", recv_lines(b), f"POS {entity_a} 12")

a.sendall(b"GARBAGE not a real command\n")
time.sleep(0.15)
a.sendall(b"MOVE 0 1\n")
time.sleep(0.15)
expect("A still alive after malformed input", recv_lines(b), f"POS {entity_a} 12")

a.close()
time.sleep(0.15)
expect("B sees A leave", recv_lines(b), f"LEAVE {entity_a}")

# --- Reconnect: position persisted, and idle B visible via snapshot ---
a2 = connect()
a2.sendall(f"LOGIN {user_a} {password}\n".encode())
expect("A reconnect login", recv_lines(a2), "AUTH_OK")
a2.sendall(f"CHAR_SELECT {char_a_id}\n".encode())
reconnect_lines = recv_lines(a2)
expect("A reconnect char select", reconnect_lines, "WELCOME")
# 0,0 -> +1 -> +2 -> +3 -> clamped +6 -> +(0,1) = (12, 1)
expect("A reconnect position persisted at (12, 1)", reconnect_lines, " 12.000000 1.000000")
expect("A reconnect snapshot includes idle B", reconnect_lines, f"POS {entity_b} 0")
a2.close()
b.close()

# --- At rest: no plaintext password in the database ---
if db_path:
    time.sleep(0.2)
    con = sqlite3.connect(db_path)
    row = con.execute(
        "SELECT length(password_hash), length(password_salt) FROM accounts WHERE username=?",
        (user_a,)).fetchone()
    salts = con.execute(
        "SELECT password_salt FROM accounts WHERE username IN (?, ?)", (user_a, user_b)).fetchall()
    con.close()
    check("stored hash is 32 B, salt 16 B", row == (32, 16), row)
    check("same password -> different salts", len({s[0] for s in salts}) == 2, "")
    with open(db_path, "rb") as f:
        raw_db = f.read()
    check("plaintext password absent from DB file", password.encode() not in raw_db, "")

print("OK — protocol (TLS + accounts + characters + anti-cheat + security checks) behaves as documented")

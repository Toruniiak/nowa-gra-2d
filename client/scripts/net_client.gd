extends Node
## Network client for the line-based TLS protocol documented in
## docs/NETWORKING.md. This is the ONLY place in the client that talks to
## the server socket — every other script reacts to the signals below.
##
## The client never decides game state on its own: it sends intents
## (login, character choice, movement) and applies whatever the server
## sends back. See docs/ARCHITECTURE.md, "Zasada nadrzędna".

signal secure_connection_ready
signal connection_failed(reason: String)
signal auth_result(ok: bool, reason: String)
signal characters_listed(characters: Array)  # Array of {id: int, name: String}
signal character_created(id: int, name: String)
signal character_create_failed(reason: String)
signal character_select_failed(reason: String)
signal entered_world(local_entity_id: int, step_duration: float)
signal entity_position_updated(entity_id: int, tile: Vector2i, facing: String)
signal entity_left(entity_id: int)

enum State { DISCONNECTED, TCP_CONNECTING, TLS_HANDSHAKING, READY }

var _tcp := StreamPeerTCP.new()
var _tls := StreamPeerTLS.new()
var _state := State.DISCONNECTED
var _local_entity_id := -1
## From WELCOME, valid once entered_world fired. A diagonal step takes longer
## (it covers sqrt(2) tiles); the server decides both values.
var diagonal_step_duration := 0.354
var _inbuf := PackedByteArray()
var _host := ""
var _tls_common_name := ""
var _tls_options: TLSOptions


## The server certificate is always verified — there is deliberately no
## "skip verification" mode (in Godot 4.3.stable TLSOptions.client_unsafe()
## also failed the handshake against our server anyway; see
## docs/KNOWN_ISSUES.md). trusted_cert_path: PEM certificate to pin (the
## self-signed dev server). If empty or missing, the server must present a
## certificate from a CA in Godot's bundled root store — i.e. a real server
## with a real domain certificate.
func connect_to_server(host: String, port: int, trusted_cert_path := "",
		tls_common_name := "localhost") -> void:
	# Fresh peers on every (re)connect: a StreamPeerTLS that already failed
	# or was disconnected is not reused.
	_tcp = StreamPeerTCP.new()
	_tls = StreamPeerTLS.new()
	_inbuf = PackedByteArray()
	_local_entity_id = -1
	_host = host
	_tls_common_name = tls_common_name
	if trusted_cert_path.is_empty() or not FileAccess.file_exists(trusted_cert_path):
		if not trusted_cert_path.is_empty():
			push_warning("net_client: pinned certificate %s not found — verifying against system CAs (a self-signed dev server will be rejected; see docs/BUILD.md)" % trusted_cert_path)
		_tls_options = TLSOptions.client()
	else:
		var cert := X509Certificate.new()
		if cert.load(trusted_cert_path) != OK:
			_fail("cannot load trusted certificate: %s" % trusted_cert_path)
			return
		_tls_options = TLSOptions.client(cert)

	var err := _tcp.connect_to_host(host, port)
	if err != OK:
		_fail("connect_to_host failed: %s" % err)
		return
	_state = State.TCP_CONNECTING


func _process(_delta: float) -> void:
	match _state:
		State.TCP_CONNECTING:
			_tcp.poll()
			var tcp_status := _tcp.get_status()
			if tcp_status == StreamPeerTCP.STATUS_CONNECTED:
				var err := _tls.connect_to_stream(_tcp, _tls_common_name, _tls_options)
				if err != OK:
					_fail("TLS connect_to_stream failed: %s" % err)
					return
				_state = State.TLS_HANDSHAKING
			elif tcp_status == StreamPeerTCP.STATUS_ERROR or tcp_status == StreamPeerTCP.STATUS_NONE:
				_fail("TCP connection to %s failed" % _host)
		State.TLS_HANDSHAKING:
			_tls.poll()
			match _tls.get_status():
				StreamPeerTLS.STATUS_CONNECTED:
					_state = State.READY
					secure_connection_ready.emit()
				StreamPeerTLS.STATUS_HANDSHAKING:
					pass
				StreamPeerTLS.STATUS_ERROR_HOSTNAME_MISMATCH:
					_fail("TLS certificate hostname mismatch")
				_:
					_fail("TLS handshake failed")
		State.READY:
			_tls.poll()
			if _tls.get_status() != StreamPeerTLS.STATUS_CONNECTED:
				_fail("connection closed by server")
				return
			_read_available()


func _read_available() -> void:
	var available := _tls.get_available_bytes()
	if available <= 0:
		return
	var result: Array = _tls.get_partial_data(available)
	if result[0] != OK:
		return
	_inbuf.append_array(result[1])
	# Decode only complete lines, so a multi-byte UTF-8 character split
	# across two TLS reads is never decoded half-way.
	while true:
		var newline := _inbuf.find(10)
		if newline == -1:
			break
		var line := _inbuf.slice(0, newline).get_string_from_utf8().strip_edges()
		_inbuf = _inbuf.slice(newline + 1)
		_handle_line(line)


func _send(line: String) -> void:
	if _state != State.READY:
		return
	_tls.put_data((line + "\n").to_utf8_buffer())


func _fail(reason: String) -> void:
	push_warning("net_client: %s" % reason)
	_state = State.DISCONNECTED
	_tls.disconnect_from_stream()
	_tcp.disconnect_from_host()
	connection_failed.emit(reason)


func send_register(username: String, password: String) -> void:
	_send("REGISTER %s %s" % [username, password])


func send_login(username: String, password: String) -> void:
	_send("LOGIN %s %s" % [username, password])


func request_character_list() -> void:
	_send("CHAR_LIST")


func send_character_create(character_name: String) -> void:
	_send("CHAR_CREATE %s" % character_name)


func send_character_select(character_id: int) -> void:
	_send("CHAR_SELECT %d" % character_id)


## dir: "N", "E", "S", "W", "NE", "SE", "SW" or "NW". Only an intent — the server decides if, when
## and where the character moves (walls, other players, step duration).
func send_step(dir: String) -> void:
	if _local_entity_id == -1:
		return
	_send("STEP %s" % dir)


func _handle_line(line: String) -> void:
	if line.is_empty():
		return
	var parts := line.split(" ")
	var cmd := parts[0]

	match cmd:
		"AUTH_OK":
			auth_result.emit(true, "")
		"AUTH_FAIL":
			auth_result.emit(false, parts[1] if parts.size() > 1 else "")
		"CHARS":
			var characters: Array = []
			for i in range(1, parts.size()):
				# Format "<id>:<name>"; names may contain spaces, which split()
				# would have broken up — re-join tokens that lack an "<id>:".
				var entry := parts[i]
				var sep := entry.find(":")
				if sep > 0 and entry.substr(0, sep).is_valid_int():
					characters.append({"id": int(entry.substr(0, sep)), "name": entry.substr(sep + 1)})
				elif not characters.is_empty():
					characters[-1]["name"] += " " + entry
			characters_listed.emit(characters)
		"CHAR_CREATED":
			character_created.emit(int(parts[1]), " ".join(parts.slice(2)))
		"CHAR_CREATE_FAIL":
			character_create_failed.emit(parts[1] if parts.size() > 1 else "")
		"CHAR_SELECT_FAIL":
			character_select_failed.emit(parts[1] if parts.size() > 1 else "")
		"WELCOME":
			_local_entity_id = int(parts[1])
			var step_ms := int(parts[2]) if parts.size() > 2 else 250
			var diag_ms := int(parts[3]) if parts.size() > 3 else roundi(step_ms * sqrt(2.0))
			diagonal_step_duration = diag_ms / 1000.0
			entered_world.emit(_local_entity_id, step_ms / 1000.0)
		"POS":
			if parts.size() >= 5:
				entity_position_updated.emit(int(parts[1]), Vector2i(int(parts[2]), int(parts[3])), parts[4])
		"LEAVE":
			entity_left.emit(int(parts[1]))
		_:
			push_warning("net_client: unrecognized line from server: %s" % line)


func local_entity_id() -> int:
	return _local_entity_id

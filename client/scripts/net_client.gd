extends Node
## Network client for the line-based TCP protocol documented in
## docs/NETWORKING.md. This is the ONLY place in the client that talks to
## the server socket — every other script reacts to the signals below.
##
## The client never decides game state on its own: it sends intents
## (move_intent) and applies whatever the server broadcasts back. See
## docs/ARCHITECTURE.md, "Zasada nadrzędna".

signal connected_to_server(local_entity_id: int)
signal entity_position_updated(entity_id: int, x: float, y: float)
signal entity_left(entity_id: int)
signal connection_failed

var _peer := StreamPeerTCP.new()
var _connected := false
var _local_entity_id := -1
var _line_buffer := ""

func connect_to_server(host: String, port: int) -> void:
	var err := _peer.connect_to_host(host, port)
	if err != OK:
		push_error("net_client: connect_to_host failed: %s" % err)
		connection_failed.emit()

func _process(_delta: float) -> void:
	_peer.poll()
	var status := _peer.get_status()

	if status == StreamPeerTCP.STATUS_CONNECTED and not _connected:
		_connected = true

	if status != StreamPeerTCP.STATUS_CONNECTED:
		if _connected:
			_connected = false
		return

	var available := _peer.get_available_bytes()
	if available > 0:
		var chunk := _peer.get_utf8_string(available)
		_line_buffer += chunk
		while true:
			var newline_pos := _line_buffer.find("\n")
			if newline_pos == -1:
				break
			var line := _line_buffer.substr(0, newline_pos)
			_line_buffer = _line_buffer.substr(newline_pos + 1)
			_handle_line(line.strip_edges())

func send_move_intent(dx: float, dy: float) -> void:
	if not _connected:
		return
	_peer.put_data(("MOVE %f %f\n" % [dx, dy]).to_utf8_buffer())

func _handle_line(line: String) -> void:
	if line.is_empty():
		return
	var parts := line.split(" ")
	var cmd := parts[0]

	match cmd:
		"WELCOME":
			_local_entity_id = int(parts[1])
			connected_to_server.emit(_local_entity_id)
		"POS":
			var entity_id := int(parts[1])
			var x := float(parts[2])
			var y := float(parts[3])
			entity_position_updated.emit(entity_id, x, y)
		"LEAVE":
			entity_left.emit(int(parts[1]))
		_:
			push_warning("net_client: unrecognized line from server: %s" % line)

func local_entity_id() -> int:
	return _local_entity_id

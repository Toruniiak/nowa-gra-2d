extends Node2D
## Root scene: owns the NetClient and the login screen, spawns/updates
## entities on server broadcasts, and sends the local player's movement
## intent. Contains no game rules — it only relays. See
## docs/ARCHITECTURE.md.

@export var server_host := "127.0.0.1"
@export var server_port := 7777
## PEM certificate to pin (the self-signed dev server's public certificate,
## copied here per docs/BUILD.md so it is bundled into exports). If the file
## is missing, NetClient verifies against system CAs instead — it never
## connects unverified.
@export var tls_trusted_cert := "res://certs/dev_server.crt"
@export var tls_common_name := "localhost"

const PLAYER_ENTITY_SCENE := preload("res://scenes/PlayerEntity.tscn")
const PLAYER_SPEED := 120.0  # px/sec
# Matches the server tick in server/src/main.cpp (kTickIntervalMs). Sending
# faster than this wouldn't move the player faster (see KNOWN_ISSUES.md for
# why that's not yet enforced server-side either) but would just waste
# bandwidth, so the client throttles to the same rate.
const SEND_INTERVAL := 0.05

var _entities: Dictionary = {}
var _local_id := -1
var _send_accum := 0.0

@onready var _net := $NetClient
@onready var _login_ui := $LoginUI


func _ready() -> void:
	# Dev/test-only overrides. --password on a command line is visible to
	# other local processes — never use it for a real account.
	var auto_user := ""
	var auto_password := ""
	var auto_register := false
	var auto_character := ""
	for arg in OS.get_cmdline_user_args():
		var value: String = arg.split("=", true, 1)[1] if "=" in arg else ""
		if arg.begins_with("--server-port="):
			server_port = int(value)
		elif arg.begins_with("--server-host="):
			server_host = value
		elif arg.begins_with("--tls-cert="):
			tls_trusted_cert = value
		elif arg.begins_with("--tls-cn="):
			tls_common_name = value
		elif arg.begins_with("--user="):
			auto_user = value
		elif arg.begins_with("--password="):
			auto_password = value
		elif arg == "--register":
			auto_register = true
		elif arg.begins_with("--character="):
			auto_character = value

	_net.entered_world.connect(_on_entered_world)
	_net.entity_position_updated.connect(_on_position_updated)
	_net.entity_left.connect(_on_entity_left)
	_net.connection_failed.connect(_on_connection_failed)
	_login_ui.setup(_net)
	_login_ui.set_server_address(server_host, server_port)
	_login_ui.reconnect_requested.connect(_on_reconnect_requested)

	if not auto_user.is_empty() and not auto_character.is_empty():
		_login_ui.auto_login(auto_user, auto_password, auto_register, auto_character)
	_connect()


func _connect() -> void:
	_net.connect_to_server(server_host, server_port, tls_trusted_cert, tls_common_name)


func _on_reconnect_requested(host: String, port: int) -> void:
	server_host = host
	server_port = port
	_connect()


func _on_entered_world(local_entity_id: int) -> void:
	_local_id = local_entity_id
	print("Entered world as entity %d" % local_entity_id)


func _on_connection_failed(_reason: String) -> void:
	for node in _entities.values():
		node.queue_free()
	_entities.clear()
	_local_id = -1


func _on_position_updated(entity_id: int, x: float, y: float) -> void:
	var node: Node2D = _entities.get(entity_id)
	if node == null:
		node = PLAYER_ENTITY_SCENE.instantiate()
		node.is_local = (entity_id == _local_id)
		add_child(node)
		_entities[entity_id] = node
		if node.is_local:
			print("Local entity %d spawned at (%s, %s)" % [entity_id, x, y])
	node.set_server_position(x, y)


func _on_entity_left(entity_id: int) -> void:
	var node: Node2D = _entities.get(entity_id)
	if node:
		node.queue_free()
		_entities.erase(entity_id)


func _process(delta: float) -> void:
	if _local_id == -1:
		return
	_send_accum += delta
	if _send_accum < SEND_INTERVAL:
		return
	_send_accum = 0.0

	var input_vec := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	if input_vec == Vector2.ZERO:
		return
	var move := input_vec * PLAYER_SPEED * SEND_INTERVAL
	_net.send_move_intent(move.x, move.y)

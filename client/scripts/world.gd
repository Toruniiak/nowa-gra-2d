extends Node2D
## Root scene: owns the NetClient, the map, the login screen and the camera;
## spawns/updates creatures from server broadcasts and turns the player's
## input into STEP intents. Contains no game rules — the server decides every
## move. See docs/ARCHITECTURE.md.

@export var server_host := "127.0.0.1"
@export var server_port := 7777
## PEM certificate to pin (the self-signed dev server's public certificate,
## copied here per docs/BUILD.md so it is bundled into exports). If the file
## is missing, NetClient verifies against system CAs instead — it never
## connects unverified.
@export var tls_trusted_cert := "res://certs/dev_server.crt"
@export var tls_common_name := "localhost"
## How many tiles fit across the screen (classic view is 15 wide).
@export var view_tiles_wide := 15.0

const PLAYER_ENTITY_SCENE := preload("res://scenes/PlayerEntity.tscn")
const MAP_PATH := "res://data/maps/start.json"

var _entities: Dictionary = {}
var _local_id := -1
var _step_duration := 0.25
var _diagonal_step_duration := 0.354
var _next_step_at := 0.0

@onready var _net := $NetClient
@onready var _login_ui := $LoginUI
@onready var _map := $Map
@onready var _camera: Camera2D = $Camera


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

	if not _map.load_map(MAP_PATH):
		push_error("world: map failed to load")
	_setup_camera()

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


func _setup_camera() -> void:
	var view_w := get_viewport_rect().size.x
	var z := view_w / (view_tiles_wide * GameMapScript.TILE)
	_camera.zoom = Vector2(z, z)
	_camera.limit_left = 0
	_camera.limit_top = 0
	_camera.limit_right = _map.width * GameMapScript.TILE
	_camera.limit_bottom = _map.height * GameMapScript.TILE
	_camera.position = Vector2(_map.spawn) * GameMapScript.TILE


const GameMapScript := preload("res://scripts/game_map.gd")


func _connect() -> void:
	_net.connect_to_server(server_host, server_port, tls_trusted_cert, tls_common_name)


func _on_reconnect_requested(host: String, port: int) -> void:
	server_host = host
	server_port = port
	_connect()


func _on_entered_world(local_entity_id: int, step_duration: float) -> void:
	_local_id = local_entity_id
	_step_duration = step_duration
	_diagonal_step_duration = _net.diagonal_step_duration
	print("Entered world as entity %d (step %d ms, diagonal %d ms)" % [
		local_entity_id, int(step_duration * 1000), int(_diagonal_step_duration * 1000)])


func _on_connection_failed(_reason: String) -> void:
	for node in _entities.values():
		node.queue_free()
	_entities.clear()
	_local_id = -1


func _on_position_updated(entity_id: int, tile: Vector2i, facing: String) -> void:
	var node = _entities.get(entity_id)
	if node == null:
		node = PLAYER_ENTITY_SCENE.instantiate()
		node.is_local = (entity_id == _local_id)
		node.step_duration = _step_duration
		node.diagonal_step_duration = _diagonal_step_duration
		_map.sorted_layer().add_child(node)
		_entities[entity_id] = node
		if node.is_local:
			print("Local entity %d spawned at (%d, %d)" % [entity_id, tile.x, tile.y])
	node.set_tile_position(tile, facing)


func _on_entity_left(entity_id: int) -> void:
	var node = _entities.get(entity_id)
	if node:
		node.queue_free()
		_entities.erase(entity_id)


func _process(_delta: float) -> void:
	var local = _entities.get(_local_id)
	if local:
		_camera.position = local.position - Vector2(0, GameMapScript.TILE / 2.0)
	if _local_id == -1:
		return
	var dir := _input_direction()
	if dir.is_empty():
		return
	# Keep one step in flight: the server holds a single queued step, so
	# sending again shortly before the current one ends gives continuous
	# walking without the client ever deciding where it actually is.
	var now := Time.get_ticks_msec() / 1000.0
	if now < _next_step_at:
		return
	_next_step_at = now + (_diagonal_step_duration if dir.length() == 2 else _step_duration) * 0.8
	_net.send_step(dir)


## Directions by 45-degree sector, starting at +x (east) and turning towards
## +y (screen down = south).
const DIRS_8: Array[String] = ["E", "SE", "S", "SW", "W", "NW", "N", "NE"]


## Keyboard, gamepad and (later) on-screen joystick all feed the same
## ui_* actions. 8 directions: the stick/keys vector is snapped to the
## nearest 45-degree sector (two keys at once = diagonal).
func _input_direction() -> String:
	var v := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	if v.length() < 0.3:
		return ""
	var sector := wrapi(roundi(v.angle() / (PI / 4.0)), 0, 8)
	return DIRS_8[sector]

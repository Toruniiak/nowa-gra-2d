extends Node2D
## Root scene: owns the NetClient, spawns/updates entities on server
## broadcasts, and sends the local player's movement intent. Contains no
## game rules — it only relays. See docs/ARCHITECTURE.md.

@export var server_host := "127.0.0.1"
@export var server_port := 7777

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

func _ready() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--server-port="):
			server_port = int(arg.split("=")[1])
		elif arg.begins_with("--server-host="):
			server_host = arg.split("=")[1]

	_net.connected_to_server.connect(_on_connected)
	_net.entity_position_updated.connect(_on_position_updated)
	_net.entity_left.connect(_on_entity_left)
	_net.connect_to_server(server_host, server_port)

func _on_connected(local_entity_id: int) -> void:
	_local_id = local_entity_id
	print("Connected to server as entity %d" % local_entity_id)

func _on_position_updated(entity_id: int, x: float, y: float) -> void:
	var node: Node2D = _entities.get(entity_id)
	if node == null:
		node = PLAYER_ENTITY_SCENE.instantiate()
		node.is_local = (entity_id == _local_id)
		add_child(node)
		_entities[entity_id] = node
	node.set_server_position(x, y)

func _on_entity_left(entity_id: int) -> void:
	var node: Node2D = _entities.get(entity_id)
	if node:
		node.queue_free()
		_entities.erase(entity_id)

func _process(delta: float) -> void:
	_send_accum += delta
	if _send_accum < SEND_INTERVAL:
		return
	_send_accum = 0.0

	var input_vec := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	if input_vec == Vector2.ZERO:
		return
	var move := input_vec * PLAYER_SPEED * SEND_INTERVAL
	_net.send_move_intent(move.x, move.y)

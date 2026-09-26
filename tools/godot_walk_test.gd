extends SceneTree
## Diagnostic harness — NOT part of the game. Drives the real World.tscn:
## holds "right" for HOLD_SEC and checks that (1) the server confirmed
## several steps at the expected pace, and (2) the sprite glides between
## tiles (intermediate positions seen) instead of teleporting. Usage:
##   godot4 --headless --path client --script ../tools/godot_walk_test.gd -- \
##     --server-port=... --user=U --password=P --character=C [--register]

const HOLD_SEC := 1.2

var _world: Node
var _entered := false
var _t := 0.0
var _start_tile := Vector2i(-1, -1)
var _last_tile := Vector2i(-1, -1)
var _confirmed_steps := 0
var _between_positions := 0


func _initialize() -> void:
	_world = load("res://scenes/World.tscn").instantiate()
	root.add_child(_world)
	var net: Node = _world.get_node("NetClient")
	net.entered_world.connect(func(_id: int, _s: float) -> void: _entered = true)
	net.entity_position_updated.connect(_on_pos)


func _on_pos(id: int, tile: Vector2i, _facing: String) -> void:
	if id != _world._local_id:
		return
	if _start_tile == Vector2i(-1, -1):
		_start_tile = tile
	elif tile != _last_tile:
		_confirmed_steps += 1
	_last_tile = tile


func _process(delta: float) -> bool:
	if not _entered or _start_tile == Vector2i(-1, -1):
		_t += delta
		if _t > 15.0:
			print("walk: FAIL — never entered the world")
			quit(1)
		return false
	_t += delta
	if _t < 15.0:
		_t = 15.0  # start the walk clock
	var walk_t := _t - 15.0
	if walk_t < HOLD_SEC:
		Input.action_press("ui_right")
	else:
		Input.action_release("ui_right")
	var local = _world._entities.get(_world._local_id)
	if local:
		var frac := fposmod(local.position.x - 16.0, 32.0)
		if frac > 4.0 and frac < 28.0:
			_between_positions += 1
	if walk_t > HOLD_SEC + 0.6:
		var dx := _last_tile.x - _start_tile.x
		print("walk: start %s end %s, server-confirmed steps %d, in-between frames %d"
				% [_start_tile, _last_tile, _confirmed_steps, _between_positions])
		var ok := dx >= 3 and dx <= 6 and _last_tile.y == _start_tile.y and _between_positions > 10
		print("walk: %s" % ("PASS" if ok else "FAIL"))
		quit(0 if ok else 1)
	return false

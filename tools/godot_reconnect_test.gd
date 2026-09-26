extends SceneTree
## Diagnostic harness — NOT part of the game. Drives the real World.tscn to
## check the login screen's "server address + Połącz ponownie" path: the
## client starts against a dead port, the connection fails, the harness
## types the real address into the UI and presses reconnect, and the
## auto-login flow must then reach the world. Usage (from repo root):
##   godot4 --headless --path client --script ../tools/godot_reconnect_test.gd -- \
##     --server-port=1 --tls-cert=<abs path> --user=U --password=P --character=C \
##     --real-address=127.0.0.1:7800

const TIMEOUT_SEC := 15.0

var _world: Node
var _elapsed := 0.0
var _pressed := false
var _real_address := ""


func _initialize() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--real-address="):
			_real_address = arg.split("=", true, 1)[1]
	_world = load("res://scenes/World.tscn").instantiate()
	root.add_child(_world)
	_world.get_node("NetClient").entered_world.connect(_on_entered_world)


func _process(delta: float) -> bool:
	_elapsed += delta
	var ui: Node = _world.get_node("LoginUI")
	if not _pressed and ui._reconnect_button.visible:
		print("harness: initial connection failed as expected, status: %s" % ui._status.text)
		ui._server_address.text = _real_address
		ui._on_reconnect_pressed()
		_pressed = true
	if _elapsed > TIMEOUT_SEC:
		print("harness: FAIL — did not enter the world within %ds" % TIMEOUT_SEC)
		quit(1)
	return false


func _on_entered_world(entity_id: int) -> void:
	print("harness: PASS — entered world as entity %d after reconnect=%s" % [entity_id, _pressed])
	quit(0 if _pressed else 1)

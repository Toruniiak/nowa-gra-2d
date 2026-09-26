extends SceneTree
## Diagnostic harness — NOT part of the game. Runs the real World.tscn with
## rendering (needs a display, e.g. xvfb-run) and saves a PNG of the
## viewport. Usage (from repo root):
##   xvfb-run -s "-screen 0 1280x1400x24" godot4 --path client \
##     --rendering-driver opengl3 --script "$PWD"/tools/godot_screenshot.gd -- \
##     --shot=/abs/out.png [--wait-world] [world.gd args: --server-port=... --user=...]
## Without --wait-world the shot is taken on the login screen once the TLS
## connection is up; with it, after the local character entered the world.

const SETTLE_FRAMES := 30
const TIMEOUT_SEC := 20.0

var _shot_path := ""
var _wait_world := false
var _countdown := -1
var _elapsed := 0.0


func _initialize() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--shot="):
			_shot_path = arg.split("=", true, 1)[1]
		elif arg == "--wait-world":
			_wait_world = true
	var world: Node = load("res://scenes/World.tscn").instantiate()
	root.add_child(world)
	var net: Node = world.get_node("NetClient")
	if _wait_world:
		net.entered_world.connect(func(_id: int, _step: float) -> void: _countdown = SETTLE_FRAMES)
	else:
		net.secure_connection_ready.connect(func() -> void: _countdown = SETTLE_FRAMES)


func _process(delta: float) -> bool:
	_elapsed += delta
	if _countdown > 0:
		_countdown -= 1
	elif _countdown == 0:
		var err := root.get_texture().get_image().save_png(_shot_path)
		print("screenshot: %s (%s)" % [_shot_path, error_string(err)])
		quit(0 if err == OK else 1)
	if _elapsed > TIMEOUT_SEC:
		print("screenshot: FAIL — timed out")
		quit(1)
	return false

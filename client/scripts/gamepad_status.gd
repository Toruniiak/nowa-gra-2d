extends Node
## Tracks connected gamepads — wired or Bluetooth-paired (Android reports a
## paired Bluetooth HID controller to Godot as an ordinary joypad; no extra
## permission or pairing code is needed on the app side, see
## docs/GAME_DESIGN.md "Sterowanie"). Responsible only for detecting
## connect/disconnect and reporting it — movement itself is NOT bound here,
## it already works via the engine's default ui_left/right/up/down bindings
## (verified: InputMap.action_get_events includes InputEventJoypadButton and
## InputEventJoypadMotion for those actions out of the box).
##
## Future HUD (Phase 10+ UI work) can listen to these signals to show a
## "controller connected" indicator instead of/alongside the touch joystick.

signal gamepad_connected(device_id: int, device_name: String)
signal gamepad_disconnected(device_id: int)

func _ready() -> void:
	Input.joy_connection_changed.connect(_on_joy_connection_changed)
	for id in Input.get_connected_joypads():
		_report_connected(id)

func _on_joy_connection_changed(device_id: int, connected: bool) -> void:
	if connected:
		_report_connected(device_id)
	else:
		print("Gamepad disconnected: id=%d" % device_id)
		gamepad_disconnected.emit(device_id)

func _report_connected(device_id: int) -> void:
	var device_name := Input.get_joy_name(device_id)
	print("Gamepad connected: id=%d name=%s" % [device_id, device_name])
	gamepad_connected.emit(device_id, device_name)

func has_active_gamepad() -> bool:
	return Input.get_connected_joypads().size() > 0

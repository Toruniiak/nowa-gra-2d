extends Node2D
## Represents one entity (any connected player, for now — monsters/NPCs will
## get their own script once Phase 8/9 exist, see docs/ROADMAP.md).
##
## VISUAL PLACEHOLDER: a plain colored rectangle. This is intentional and
## explicitly allowed at this stage (see docs/ASSET_PIPELINE.md) — it exists
## to prove movement/networking works, NOT as final art. Must be replaced
## with a real sprite + animation before any playable milestone.

@export var is_local := false

func _ready() -> void:
	$Body.color = Color(0.3, 0.6, 1.0) if is_local else Color(0.85, 0.25, 0.25)

func set_server_position(x: float, y: float) -> void:
	# Direct assignment for now — no interpolation. Once real network jitter
	# shows up (multiple real clients, not the loopback test), revisit with
	# smoothing. Don't add interpolation speculatively before it's needed.
	position = Vector2(x, y)

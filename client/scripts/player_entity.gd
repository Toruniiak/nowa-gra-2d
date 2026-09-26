extends Node2D
## One creature on the tile grid (any connected player, for now — monsters
## get their own sprites in Phase 8). Position comes only from the server
## (tile + facing); this script just animates the step between two tiles over
## the server's step duration and plays the walk cycle.
##
## Sprite sheet layout (tools/art/gen_tileset.py): 32x48 frames, rows =
## down/left/right/up, cols = idle/step A/step B; drawn bottom-centre on the
## tile like everything else (see game_map.gd).

const FRAME := Vector2i(32, 48)
const ROW := {"S": 0, "W": 1, "E": 2, "N": 3}

@export var is_local := false

var tile := Vector2i(-1, -1)
var step_duration := 0.25

var _facing := "S"
var _tween: Tween
var _anim_time := 0.0
var _moving := false

@onready var _sprite: Sprite2D = $Sprite


func _ready() -> void:
	var sheet := "res://assets/sprites/player_blue.png" if is_local else "res://assets/sprites/player_red.png"
	var atlas := AtlasTexture.new()
	atlas.atlas = load(sheet)
	_sprite.texture = atlas
	_sprite.centered = false
	_sprite.offset = Vector2(-FRAME.x / 2.0, -FRAME.y)
	_set_frame(0)


func set_tile_position(new_tile: Vector2i, facing: String) -> void:
	_facing = facing if ROW.has(facing) else "S"
	var target := Vector2(new_tile.x * 32 + 16, new_tile.y * 32 + 32)
	var distance := absi(new_tile.x - tile.x) + absi(new_tile.y - tile.y)
	var first := tile == Vector2i(-1, -1)
	tile = new_tile
	if _tween:
		_tween.kill()
	if first or distance != 1:
		position = target  # spawn, relocation or catching up: no slide
		_moving = false
		_set_frame(0)
		return
	_moving = true
	_tween = create_tween()
	_tween.tween_property(self, "position", target, step_duration)
	_tween.finished.connect(func() -> void:
		_moving = false
		_set_frame(0))


func is_moving() -> bool:
	return _moving


func _process(delta: float) -> void:
	if not _moving:
		_set_frame(0)
		return
	_anim_time += delta
	# two step frames per tile, alternating A/B
	_set_frame(1 + int(_anim_time / (step_duration / 2.0)) % 2)


func _set_frame(col: int) -> void:
	(_sprite.texture as AtlasTexture).region = Rect2(
		Vector2(col * FRAME.x, ROW[_facing] * FRAME.y), Vector2(FRAME))

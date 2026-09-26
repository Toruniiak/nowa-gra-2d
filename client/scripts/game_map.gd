extends Node2D
## Renders a map from res://data (the same files the server loads for its
## walkability rules — see docs/ASSET_PIPELINE.md). Ground goes into a
## TileMapLayer (fast, supports animated water); objects that can be taller
## than a tile (walls, trees) become sprites under a y-sorted node shared with
## creatures, so a player standing "behind" a tree is correctly overlapped.
##
## Drawing rule for every sprite: its bottom-centre sits on the tile's
## bottom-centre, so taller art rises into the tile(s) above (oblique look).

const TILES_PATH := "res://data/tiles.json"
const TILE := 32

var width := 0
var height := 0
var spawn := Vector2i.ZERO

var _defs: Dictionary
var _sheets := {}  # sheet name -> {texture, cell}

@onready var _ground: TileMapLayer = $Ground
@onready var _sorted: Node2D = $Sorted


## Returns the y-sorted node creatures should be added to.
func sorted_layer() -> Node2D:
	return _sorted


func load_map(map_path: String) -> bool:
	_defs = _read_json(TILES_PATH)
	var map: Dictionary = _read_json(map_path)
	if _defs.is_empty() or map.is_empty():
		return false
	for sheet_name in _defs["sheets"]:
		var s: Dictionary = _defs["sheets"][sheet_name]
		_sheets[sheet_name] = {"texture": load(s["path"]), "cell": Vector2i(s["cell"][0], s["cell"][1])}

	width = int(map["width"])
	height = int(map["height"])
	spawn = Vector2i(int(map["spawn"][0]), int(map["spawn"][1]))
	_build_ground(map)
	_build_objects(map)
	return true


func _build_ground(map: Dictionary) -> void:
	var tileset := TileSet.new()
	tileset.tile_size = Vector2i(TILE, TILE)
	var source := TileSetAtlasSource.new()
	source.texture = _sheets["terrain"]["texture"]
	source.texture_region_size = Vector2i(TILE, TILE)
	var source_id := tileset.add_source(source)

	var atlas_of := {}  # ground id -> atlas coords
	for id in _defs["ground"]:
		var d: Dictionary = _defs["ground"][id]
		var coords: Vector2i
		if d.has("frames"):
			coords = Vector2i(d["frames"][0][0], d["frames"][0][1])
			source.create_tile(coords)
			source.set_tile_animation_columns(coords, d["frames"].size())
			source.set_tile_animation_frames_count(coords, d["frames"].size())
			for i in d["frames"].size():
				source.set_tile_animation_frame_duration(coords, i, 1.0 / float(d.get("fps", 3)))
		else:
			coords = Vector2i(d["cell"][0], d["cell"][1])
			source.create_tile(coords)
		atlas_of[id] = coords
	_ground.tile_set = tileset

	var legend: Dictionary = map["legend"]["ground"]
	for y in height:
		var row: String = map["ground"][y]
		for x in width:
			_ground.set_cell(Vector2i(x, y), source_id, atlas_of[legend[row[x]]])


func _build_objects(map: Dictionary) -> void:
	var legend: Dictionary = map["legend"]["objects"]
	for y in height:
		var row: String = map["objects"][y]
		for x in width:
			var id = legend[row[x]]
			if id == null:
				continue
			var d: Dictionary = _defs["objects"][id]
			var cell: Vector2i
			if d.has("autotile"):
				# Front face only where the tile below is not a wall, otherwise
				# the wall below covers it and only the top should show.
				var below_is_wall := false
				if y + 1 < height:
					var below = legend[String(map["objects"][y + 1])[x]]
					below_is_wall = below != null and _defs["objects"][below].has("autotile")
				var key := "top" if below_is_wall else "front"
				cell = Vector2i(d["autotile"][key][0], d["autotile"][key][1])
			else:
				cell = Vector2i(d["cell"][0], d["cell"][1])
			_sorted.add_child(make_sprite(d["sheet"], cell, Vector2i(x, y)))


## A sprite for one sheet cell, anchored bottom-centre on tile (x, y).
func make_sprite(sheet_name: String, cell: Vector2i, tile: Vector2i) -> Sprite2D:
	var sheet: Dictionary = _sheets[sheet_name]
	var size: Vector2i = sheet["cell"]
	var atlas := AtlasTexture.new()
	atlas.atlas = sheet["texture"]
	atlas.region = Rect2(Vector2(cell * size), Vector2(size))
	var sprite := Sprite2D.new()
	sprite.texture = atlas
	sprite.centered = false
	sprite.offset = Vector2(-size.x / 2.0, -size.y)
	sprite.position = tile_bottom_centre(tile)
	return sprite


static func tile_bottom_centre(tile: Vector2i) -> Vector2:
	return Vector2(tile.x * TILE + TILE / 2.0, tile.y * TILE + TILE)


func _read_json(path: String) -> Dictionary:
	var text := FileAccess.get_file_as_string(path)
	if text.is_empty():
		push_error("game_map: cannot read %s" % path)
		return {}
	var parsed = JSON.parse_string(text)
	if typeof(parsed) != TYPE_DICTIONARY:
		push_error("game_map: invalid JSON in %s" % path)
		return {}
	return parsed

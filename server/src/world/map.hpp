#pragma once

#include <string>
#include <vector>

namespace game {

struct TilePos {
  int x = 0;
  int y = 0;
  bool operator==(const TilePos&) const = default;
};

// Server-side view of a map: only what the rules need (bounds, which tiles
// can be stood on, where new characters appear). Graphics live in the same
// files but are the client's business. Data format: client/data/tiles.json +
// client/data/maps/*.json, see docs/ASSET_PIPELINE.md.
class GameMap {
 public:
  // Throws std::runtime_error with a precise message on any inconsistency
  // (bad dimensions, unknown codes, unknown tile ids, unwalkable spawn) — a
  // broken map must stop the server at startup, not surface as odd gameplay.
  GameMap(const std::string& tilesPath, const std::string& mapPath);

  int width() const { return width_; }
  int height() const { return height_; }
  TilePos spawn() const { return spawn_; }
  bool inBounds(TilePos p) const { return p.x >= 0 && p.y >= 0 && p.x < width_ && p.y < height_; }
  bool walkable(TilePos p) const {
    return inBounds(p) && walkable_[static_cast<size_t>(p.y) * width_ + p.x];
  }

 private:
  int width_ = 0;
  int height_ = 0;
  TilePos spawn_;
  std::vector<bool> walkable_;
};

}  // namespace game

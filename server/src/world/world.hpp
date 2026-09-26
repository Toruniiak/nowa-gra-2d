#pragma once

#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

#include "world/map.hpp"

namespace game {

using EntityId = int;
using Clock = std::chrono::steady_clock;

enum class Dir { North, East, South, West };

// Time one step takes. The client animates each step over the same duration
// (sent in WELCOME), so movement looks continuous while staying on the grid.
constexpr auto kStepDuration = std::chrono::milliseconds(250);

struct Entity {
  TilePos pos;
  Dir facing = Dir::South;
  Clock::time_point nextStepAt{};
  std::optional<Dir> queued;  // at most ONE pending step — see queueStep()
};

// Authoritative world state on a tile grid. The client only ever asks to
// step in a direction; whether, when and where it moves is decided here
// (walkability from the map, one creature per tile, fixed step duration).
class World {
 public:
  explicit World(const GameMap& map) : map_(map) {}

  // Places a new entity at `preferred` if it is walkable and free, otherwise
  // at the nearest free tile to the map spawn (e.g. a brand-new character, or
  // a saved position that is no longer valid after a map change).
  EntityId addEntity(TilePos preferred);
  void removeEntity(EntityId id) { entities_.erase(id); }

  // Replaces any pending step instead of queueing more: however fast a client
  // sends STEP, it can't move faster than one tile per kStepDuration.
  void queueStep(EntityId id, Dir dir);

  // Executes pending steps whose time has come. Call every tick and right
  // after queueStep() (so an idle entity reacts without waiting for a tick).
  void update(Clock::time_point now);

  const std::unordered_map<EntityId, Entity>& entities() const { return entities_; }
  const std::vector<EntityId>& dirty() const { return dirty_; }
  void clearDirty() { dirty_.clear(); }
  const GameMap& map() const { return map_; }

 private:
  bool occupied(TilePos p) const;
  bool free(TilePos p) const { return map_.walkable(p) && !occupied(p); }
  TilePos nearestFree(TilePos from) const;
  void markDirty(EntityId id);

  const GameMap& map_;
  std::unordered_map<EntityId, Entity> entities_;
  std::vector<EntityId> dirty_;
  EntityId nextId_ = 1;
};

char dirToChar(Dir d);
std::optional<Dir> dirFromChar(char c);

}  // namespace game

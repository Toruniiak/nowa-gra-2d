#pragma once

#include <cmath>
#include <unordered_map>
#include <vector>

namespace game {

using EntityId = int;

struct Entity {
  float x = 0.0f;
  float y = 0.0f;
};

// Maximum distance an entity may move in a single tick. This is the
// server-side anti-cheat boundary for movement: the client only ever sends
// an intended delta, and the server clamps it. See docs/NETWORKING.md.
constexpr float kMaxMovePerTick = 6.0f;

// Authoritative world state. Nothing in here trusts client-provided
// positions — only client-provided *intentions*, which are validated here.
class World {
 public:
  EntityId addEntity() {
    const EntityId id = nextId_++;
    entities_[id] = Entity{};
    dirty_.push_back(id);
    return id;
  }

  void removeEntity(EntityId id) { entities_.erase(id); }

  // Applies a movement intent (dx, dy), clamping it to the maximum allowed
  // distance per tick before updating position. Returns true if the
  // resulting position changed (i.e. is worth broadcasting).
  bool applyMove(EntityId id, float dx, float dy) {
    auto it = entities_.find(id);
    if (it == entities_.end()) return false;

    const float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > kMaxMovePerTick && dist > 0.0f) {
      const float scale = kMaxMovePerTick / dist;
      dx *= scale;
      dy *= scale;
    }
    if (dx == 0.0f && dy == 0.0f) return false;

    it->second.x += dx;
    it->second.y += dy;
    dirty_.push_back(id);
    return true;
  }

  const std::unordered_map<EntityId, Entity>& entities() const {
    return entities_;
  }

  // Entities whose state changed since the last call to clearDirty().
  const std::vector<EntityId>& dirty() const { return dirty_; }
  void clearDirty() { dirty_.clear(); }

 private:
  std::unordered_map<EntityId, Entity> entities_;
  std::vector<EntityId> dirty_;
  EntityId nextId_ = 1;
};

}  // namespace game

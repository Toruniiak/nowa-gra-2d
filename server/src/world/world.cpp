#include "world/world.hpp"

#include <algorithm>
#include <deque>

namespace game {

namespace {

TilePos offset(TilePos p, Dir d) {
  switch (d) {
    case Dir::North: return {p.x, p.y - 1};
    case Dir::East:  return {p.x + 1, p.y};
    case Dir::South: return {p.x, p.y + 1};
    case Dir::West:  return {p.x - 1, p.y};
  }
  return p;
}

}  // namespace

char dirToChar(Dir d) {
  switch (d) {
    case Dir::North: return 'N';
    case Dir::East:  return 'E';
    case Dir::South: return 'S';
    case Dir::West:  return 'W';
  }
  return 'S';
}

std::optional<Dir> dirFromChar(char c) {
  switch (c) {
    case 'N': return Dir::North;
    case 'E': return Dir::East;
    case 'S': return Dir::South;
    case 'W': return Dir::West;
    default:  return std::nullopt;
  }
}

EntityId World::addEntity(TilePos preferred) {
  const EntityId id = nextId_++;
  Entity e;
  e.pos = free(preferred) ? preferred : nearestFree(map_.spawn());
  entities_[id] = e;
  markDirty(id);
  return id;
}

void World::queueStep(EntityId id, Dir dir) {
  auto it = entities_.find(id);
  if (it != entities_.end()) it->second.queued = dir;
}

void World::update(Clock::time_point now) {
  for (auto& [id, e] : entities_) {
    if (!e.queued || now < e.nextStepAt) continue;
    const Dir dir = *e.queued;
    e.queued.reset();
    const TilePos target = offset(e.pos, dir);
    if (free(target)) {
      e.pos = target;
      e.facing = dir;
      e.nextStepAt = now + kStepDuration;
      markDirty(id);
    } else if (e.facing != dir) {
      e.facing = dir;  // bumping into something just turns you, costs no time
      markDirty(id);
    }
  }
}

bool World::occupied(TilePos p) const {
  return std::any_of(entities_.begin(), entities_.end(),
                     [&](const auto& kv) { return kv.second.pos == p; });
}

TilePos World::nearestFree(TilePos from) const {
  // BFS over the map; bounded by map size. If literally every tile is taken
  // the spawn is returned (stacking is harmless and practically unreachable).
  std::vector<bool> seen(static_cast<size_t>(map_.width()) * map_.height(), false);
  std::deque<TilePos> q{from};
  while (!q.empty()) {
    const TilePos p = q.front();
    q.pop_front();
    if (!map_.inBounds(p)) continue;
    auto idx = static_cast<size_t>(p.y) * map_.width() + p.x;
    if (seen[idx]) continue;
    seen[idx] = true;
    if (free(p)) return p;
    for (Dir d : {Dir::North, Dir::East, Dir::South, Dir::West}) q.push_back(offset(p, d));
  }
  return map_.spawn();
}

void World::markDirty(EntityId id) {
  if (std::find(dirty_.begin(), dirty_.end(), id) == dirty_.end()) dirty_.push_back(id);
}

}  // namespace game

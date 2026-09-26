#include "world/world.hpp"

#include <algorithm>
#include <deque>
#include <utility>

namespace game {

namespace {

TilePos offset(TilePos p, Dir d) {
  switch (d) {
    case Dir::North: return {p.x, p.y - 1};
    case Dir::East:  return {p.x + 1, p.y};
    case Dir::South: return {p.x, p.y + 1};
    case Dir::West:  return {p.x - 1, p.y};
    case Dir::NorthEast: return {p.x + 1, p.y - 1};
    case Dir::SouthEast: return {p.x + 1, p.y + 1};
    case Dir::SouthWest: return {p.x - 1, p.y + 1};
    case Dir::NorthWest: return {p.x - 1, p.y - 1};
  }
  return p;
}

constexpr std::pair<Dir, std::string_view> kDirNames[] = {
    {Dir::North, "N"},      {Dir::East, "E"},       {Dir::South, "S"},      {Dir::West, "W"},
    {Dir::NorthEast, "NE"}, {Dir::SouthEast, "SE"}, {Dir::SouthWest, "SW"}, {Dir::NorthWest, "NW"},
};

}  // namespace

bool isDiagonal(Dir d) {
  return d == Dir::NorthEast || d == Dir::SouthEast || d == Dir::SouthWest || d == Dir::NorthWest;
}

std::string dirToString(Dir d) {
  for (const auto& [dir, name] : kDirNames) {
    if (dir == d) return std::string(name);
  }
  return "S";
}

std::optional<Dir> dirFromString(std::string_view s) {
  for (const auto& [dir, name] : kDirNames) {
    if (name == s) return dir;
  }
  return std::nullopt;
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
    if (canStep(e.pos, dir)) {
      e.pos = offset(e.pos, dir);
      e.facing = dir;
      e.nextStepAt = now + (isDiagonal(dir) ? kDiagonalStepDuration : kStepDuration);
      markDirty(id);
    } else if (e.facing != dir) {
      e.facing = dir;  // bumping into something just turns you, costs no time
      markDirty(id);
    }
  }
}

bool World::canStep(TilePos from, Dir dir) const {
  const TilePos target = offset(from, dir);
  if (!free(target)) return false;
  if (!isDiagonal(dir)) return true;
  // No cutting corners: both tiles beside the diagonal must be walkable
  // terrain, otherwise you would slip through the corner of a wall, tree or
  // shoreline. Other creatures there do NOT block (you can step around a
  // player diagonally) — only the map does.
  return map_.walkable({target.x, from.y}) && map_.walkable({from.x, target.y});
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

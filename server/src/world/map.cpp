#include "world/map.hpp"

#include <fstream>
#include <map>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace game {

namespace {

nlohmann::json readJson(const std::string& path) {
  std::ifstream in(path);
  if (!in) throw std::runtime_error("Cannot open '" + path + "'");
  try {
    return nlohmann::json::parse(in);
  } catch (const nlohmann::json::parse_error& e) {
    throw std::runtime_error("Invalid JSON in '" + path + "': " + e.what());
  }
}

// code char -> walkable, for one layer. A null legend entry means "nothing
// here" (only allowed for objects) and counts as walkable.
std::map<char, bool> layerLegend(const nlohmann::json& legend, const nlohmann::json& defs,
                                 const std::string& layer, const std::string& mapPath) {
  std::map<char, bool> out;
  for (const auto& [code, id] : legend.items()) {
    if (code.size() != 1) {
      throw std::runtime_error(mapPath + ": legend." + layer + " key '" + code + "' must be one character");
    }
    if (id.is_null()) {
      out[code[0]] = true;
      continue;
    }
    const std::string name = id.get<std::string>();
    if (!defs.contains(name)) {
      throw std::runtime_error(mapPath + ": " + layer + " '" + name + "' is not defined in tiles.json");
    }
    out[code[0]] = defs.at(name).value("walkable", false);
  }
  return out;
}

}  // namespace

GameMap::GameMap(const std::string& tilesPath, const std::string& mapPath) {
  const auto tiles = readJson(tilesPath);
  const auto map = readJson(mapPath);

  try {
    width_ = map.at("width").get<int>();
    height_ = map.at("height").get<int>();
    const auto ground = layerLegend(map.at("legend").at("ground"), tiles.at("ground"), "ground", mapPath);
    const auto objects = layerLegend(map.at("legend").at("objects"), tiles.at("objects"), "objects", mapPath);
    const auto& groundRows = map.at("ground");
    const auto& objectRows = map.at("objects");

    if (width_ <= 0 || height_ <= 0 || groundRows.size() != static_cast<size_t>(height_) ||
        objectRows.size() != static_cast<size_t>(height_)) {
      throw std::runtime_error(mapPath + ": layer row count does not match height");
    }

    walkable_.assign(static_cast<size_t>(width_) * height_, false);
    for (int y = 0; y < height_; ++y) {
      const std::string g = groundRows[y].get<std::string>();
      const std::string o = objectRows[y].get<std::string>();
      if (g.size() != static_cast<size_t>(width_) || o.size() != static_cast<size_t>(width_)) {
        throw std::runtime_error(mapPath + ": row " + std::to_string(y) + " does not match width");
      }
      for (int x = 0; x < width_; ++x) {
        const auto gi = ground.find(g[x]);
        const auto oi = objects.find(o[x]);
        if (gi == ground.end() || oi == objects.end()) {
          throw std::runtime_error(mapPath + ": unknown code at (" + std::to_string(x) + ", " +
                                   std::to_string(y) + ")");
        }
        walkable_[static_cast<size_t>(y) * width_ + x] = gi->second && oi->second;
      }
    }

    const auto& s = map.at("spawn");
    spawn_ = {s.at(0).get<int>(), s.at(1).get<int>()};
  } catch (const nlohmann::json::exception& e) {
    throw std::runtime_error(mapPath + ": missing or wrongly typed field: " + e.what());
  }

  if (!walkable(spawn_)) {
    throw std::runtime_error(mapPath + ": spawn tile is not walkable");
  }
}

}  // namespace game

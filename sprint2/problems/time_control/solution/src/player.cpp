#include "player.h"
#include <random>
#include <sstream>

namespace model {

static std::string GenerateToken() {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<int> dist(0, 15);

    std::ostringstream oss;
    for (int i = 0; i < 32; ++i)
        oss << std::hex << dist(gen);

    return oss.str();
}

Player& PlayerManager::AddPlayer(std::string name, const Map::Id& map_id) {
    Token token = GenerateToken();
    players_.emplace_back(next_id_++, std::move(name), map_id, token);
    Player& p = players_.back();
    token_to_player_[p.GetToken()] = &p;
    
    const Map* map = game_.FindMap(map_id);
    if (map) {
        const auto& roads = map->GetRoads();
        if (!roads.empty()) {
            const auto& first_road = roads[0];
            Point start = first_road.GetStart();
            
            if (first_road.IsVertical()) {
                // Для вертикальной дороги: ось по X, игрок смещен на +0.4 по X
                p.SetPosition({static_cast<double>(start.x) + 0.4, 
                               static_cast<double>(start.y)});
            } else if (first_road.IsHorizontal()) {
                // Для горизонтальной дороги: ось по Y, игрок смещен на +0.4 по Y
                p.SetPosition({static_cast<double>(start.x), 
                               static_cast<double>(start.y) + 0.4});
            } else {
                p.SetPosition({0.4, 0.4});
            }
        } else {
            p.SetPosition({0.4, 0.4});
        }
    } else {
        p.SetPosition({0.4, 0.4});
    }
    
    p.SetSpeed({0.0, 0.0});
    p.SetDirection(Direction::NORTH);
    return p;
}

Player* PlayerManager::FindByToken(const Token& token) {
    if (auto it = token_to_player_.find(token); it != token_to_player_.end())
        return it->second;
    return nullptr;
}

std::vector<Player*> PlayerManager::GetPlayersByMap(const Map::Id& map_id) {
    std::vector<Player*> result;

    for (auto& p : players_) {
        if (p.GetMapId() == map_id)
            result.push_back(&p);
    }

    return result;
}

} // namespace model
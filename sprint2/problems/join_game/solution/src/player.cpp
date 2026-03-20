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
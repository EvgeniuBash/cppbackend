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

Player& PlayerManager::AddPlayer(std::string name, const Map::Id& map_id, Position start_pos) {
    Token token = GenerateToken();
    players_.emplace_back(next_id_++, std::move(name), map_id, token);
    Player& p = players_.back();
    auto now = Player::Clock::now();
    token_to_player_[p.GetToken()] = &p;

    p.SetPosition(start_pos);
    p.SetSpeed({0.0, 0.0});
    p.SetDirection(Direction::NORTH);
    p.SetJoinTime(now);
    p.SetLastMoveTime(now);
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

std::vector<Player*> PlayerManager::GetAllPlayers() {
    std::vector<Player*> result;
    result.reserve(players_.size());

    for (auto& player : players_) {
        result.push_back(&player);
    }

    return result;
}

std::vector<Player*> PlayerManager::RemoveInactive(std::chrono::steady_clock::duration max_idle) {
    std::vector<Player*> retired;

    auto now = Player::Clock::now();

    for (auto& p : players_) {
        auto idle = std::chrono::duration_cast<std::chrono::seconds>(
            now - p.GetLastMoveTime()
        );

        if (idle >= max_idle) {
            retired.push_back(&p);
        }
    }

    for (auto* p : retired) {
        token_to_player_.erase(p->GetToken());
    }

    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
            [&](const Player& p) {
                return std::find_if(retired.begin(), retired.end(),
                    [&](Player* rp) { return rp->GetId() == p.GetId(); }) != retired.end();
            }),
        players_.end()
    );

    return retired;
}

} // namespace model
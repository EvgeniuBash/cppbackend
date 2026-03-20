#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "model.h"

namespace model {

using PlayerId = uint32_t;
using Token = std::string;

class Player {
public:
    Player(PlayerId id, std::string name, Map::Id map_id, Token token)
        : id_(id), name_(std::move(name)), map_id_(map_id), token_(std::move(token)) {}

    PlayerId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const Map::Id& GetMapId() const { return map_id_; }
    const Token& GetToken() const { return token_; }

private:
    PlayerId id_;
    std::string name_;
    Map::Id map_id_;
    Token token_;
};

class PlayerManager {
public:
    Player& AddPlayer(std::string name, const Map::Id& map_id);

    Player* FindByToken(const Token& token);

    std::vector<Player*> GetPlayersByMap(const Map::Id& map_id);

private:
    PlayerId next_id_ = 0;

    std::vector<Player> players_;
    std::unordered_map<Token, Player*> token_to_player_;
};

} // namespace model
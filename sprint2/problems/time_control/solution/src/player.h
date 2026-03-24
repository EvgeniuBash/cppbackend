#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include "model.h"

namespace model {

struct Position {
    double x = 0.0;
    double y = 0.0;
};

struct Speed {
    double vx = 0.0;
    double vy = 0.0;
};

enum class Direction {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

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
    const Position& GetPosition() const { return pos_; }
    const Speed& GetSpeed() const { return speed_; }
    Direction GetDirection() const { return dir_; }
    void SetPosition(Position pos) { pos_ = pos; }
    void SetSpeed(Speed speed) { speed_ = speed; }
    void SetDirection(Direction dir) { dir_ = dir; }

private:
    PlayerId id_;
    std::string name_;
    Map::Id map_id_;
    Token token_;
    Position pos_;
    Speed speed_;
    Direction dir_ = Direction::NORTH;
};

class PlayerManager {
public:

    explicit PlayerManager(const model::Game& game) : game_(game) {}
    Player& AddPlayer(std::string name, const Map::Id& map_id);

    Player* FindByToken(const Token& token);

    std::vector<Player*> GetPlayersByMap(const Map::Id& map_id);

private:
    PlayerId next_id_ = 0;
    const Game& game_;

    std::deque<Player> players_;
    std::unordered_map<Token, Player*> token_to_player_;
};

} // namespace model
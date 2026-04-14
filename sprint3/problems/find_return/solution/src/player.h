#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include "model.h"

namespace model {

struct Speed {
    double vx = 0.0;
    double vy = 0.0;
};

struct BagItem {
    int id;
    size_t type;
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
        : id_(id), name_(std::move(name)), map_id_(map_id), token_(std::move(token)) 
        {prev_position_ = pos_;}

    PlayerId GetId() const { return id_; }
    const std::string& GetName() const { return name_; }
    const Map::Id& GetMapId() const { return map_id_; }
    const Token& GetToken() const { return token_; }
    const Position& GetPosition() const { return pos_; }
    const Speed& GetSpeed() const { return speed_; }
    const std::vector<BagItem>& GetBag() const { return bag_; }
    Direction GetDirection() const { return dir_; }
    Position GetPrevPosition() const { return prev_position_; }

    void SetPosition(Position pos) { pos_ = pos; }
    void SetSpeed(Speed speed) { speed_ = speed; }
    void SetDirection(Direction dir) { dir_ = dir; }
    void AddToBag(BagItem item) { bag_.push_back(item); }
    void ClearBag() { bag_.clear(); }
    void SetPrevPosition(Position p) { prev_position_ = p; }
    size_t GetBagSize() const { return bag_.size(); }

private:
    PlayerId id_;
    std::string name_;
    Map::Id map_id_;
    Token token_;
    Position pos_;
    Position prev_position_;
    Speed speed_;
    std::vector<BagItem> bag_;
    Direction dir_ = Direction::NORTH;
};

class PlayerManager {
public:
    Player& AddPlayer(std::string name, const Map::Id& map_id, Position start_pos);

    Player* FindByToken(const Token& token);

    std::vector<Player*> GetPlayersByMap(const Map::Id& map_id);
    std::vector<Player*> GetAllPlayers();

private:
    PlayerId next_id_ = 0;
    std::deque<Player> players_;
    std::unordered_map<Token, Player*> token_to_player_;
};

} // namespace model
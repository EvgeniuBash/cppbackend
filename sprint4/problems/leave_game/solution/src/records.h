#pragma once

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdexcept>

namespace records {

struct Record {
    std::string name;
    int score;
    double play_time;
};

class Repository {
public:
    explicit Repository(std::string db_url)
        : db_url_(std::move(db_url)) {
        Init();
    }

    void Save(const std::string& name, int score, double play_time) {
        pqxx::connection conn{db_url_};
        pqxx::work tx{conn};

        tx.exec_params(
            "INSERT INTO retired_players (name, score, play_time) "
            "VALUES ($1, $2, $3)",
            name, score, play_time
        );

        tx.commit();
    }

    std::vector<Record> Get(size_t start, size_t max_items) {
        pqxx::connection conn{db_url_};
        pqxx::read_transaction tx{conn};

        auto result = tx.exec_params(
            "SELECT name, score, play_time "
            "FROM retired_players "
            "ORDER BY score DESC, play_time ASC, name ASC "
            "OFFSET $1 LIMIT $2",
            start,
            max_items
        );

        std::vector<Record> records;
        for (const auto& row : result) {
            records.push_back({
                row["name"].as<std::string>(),
                row["score"].as<int>(),
                row["play_time"].as<double>()
            });
        }

        return records;
    }

    void SaveMany(const std::vector<Record>& records) {
    if (records.empty()) {
        return;
    }

    pqxx::connection conn{db_url_};
    pqxx::work tx{conn};

    for (const auto& r : records) {
        tx.exec_params(
            "INSERT INTO retired_players (name, score, play_time) "
            "VALUES ($1, $2, $3)",
            r.name,
            r.score,
            r.play_time
        );
    }

    tx.commit();
}

private:
    void Init() {
        pqxx::connection conn{db_url_};
        pqxx::work tx{conn};

        tx.exec(
            "CREATE TABLE IF NOT EXISTS retired_players ("
            "id SERIAL PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "score INTEGER NOT NULL,"
            "play_time DOUBLE PRECISION NOT NULL"
            ")"
        );

        tx.exec(
            "CREATE INDEX IF NOT EXISTS retired_players_order_idx "
            "ON retired_players (score DESC, play_time ASC, name ASC)"
        );

        tx.commit();
    }

    std::string db_url_;
};

inline std::string GetDbUrlFromEnv() {
    const char* url = std::getenv("GAME_DB_URL");
    if (!url) {
        throw std::runtime_error("GAME_DB_URL is not set");
    }
    return url;
}

} // namespace records
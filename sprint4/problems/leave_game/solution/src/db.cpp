#include "db.h"

Database::Database(const std::string& conn_str)
    : conn_str_(conn_str) {}

void Database::Init() {
    pqxx::connection conn(conn_str_);
    pqxx::work tx(conn);

    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id SERIAL PRIMARY KEY,
            name TEXT NOT NULL,
            score INT NOT NULL,
            play_time DOUBLE PRECISION NOT NULL
        );
    )");

    tx.exec(R"(
        CREATE INDEX IF NOT EXISTS idx_score_playtime_name
        ON retired_players (score DESC, play_time ASC, name ASC);
    )");

    tx.commit();
}

void Database::AddRecord(const std::string& name, int score, double play_time) {
    pqxx::connection conn(conn_str_);
    pqxx::work tx(conn);

    tx.exec_params(
        "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3)",
        name, score, play_time
    );

    tx.commit();
}

pqxx::result Database::GetRecords(int start, int maxItems) {
    pqxx::connection conn(conn_str_);
    pqxx::work tx(conn);

    return tx.exec_params(
        "SELECT name, score, play_time FROM retired_players "
        "ORDER BY score DESC, play_time ASC, name ASC "
        "OFFSET $1 LIMIT $2",
        start, maxItems
    );
}
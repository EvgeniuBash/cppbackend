#pragma once
#include <pqxx/pqxx>
#include <string>

class Database {
public:
    explicit Database(const std::string& conn_str);

    void Init();

    void AddRecord(const std::string& name, int score, double play_time);

    pqxx::result GetRecords(int start, int maxItems);

private:
    std::string conn_str_;
};
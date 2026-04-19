#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Connection string required\n";
        return 1;
    }

    std::string conn_str = argv[1];

    try {
        pqxx::connection conn(conn_str);

        {
            pqxx::work txn(conn);
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS books (
                    id SERIAL PRIMARY KEY,
                    title VARCHAR(100) NOT NULL,
                    author VARCHAR(100) NOT NULL,
                    year INTEGER NOT NULL,
                    ISBN CHAR(13) UNIQUE
                )
            )");
            txn.commit();
        }

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;

            json request = json::parse(line);
            std::string action = request["action"];

            if (action == "exit") {
                break;
            }

            if (action == "add_book") {
                auto payload = request["payload"];

                try {
                    pqxx::work txn(conn);

                    if (payload["ISBN"].is_null()) {
                        txn.exec_params(
                            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)",
                            payload["title"].get<std::string>(),
                            payload["author"].get<std::string>(),
                            payload["year"].get<int>()
                        );
                    } else {
                        txn.exec_params(
                            "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)",
                            payload["title"].get<std::string>(),
                            payload["author"].get<std::string>(),
                            payload["year"].get<int>(),
                            payload["ISBN"].get<std::string>()
                        );
                    }

                    txn.commit();

                    std::cout << json({{"result", true}}) << std::endl;

                } catch (const pqxx::sql_error&) {
                    std::cout << json({{"result", false}}) << std::endl;
                }
            }

            if (action == "all_books") {
                pqxx::read_transaction txn(conn);

                pqxx::result res = txn.exec(R"(
                    SELECT id, title, author, year, ISBN
                    FROM books
                    ORDER BY year DESC, title ASC, author ASC, ISBN ASC
                )");

                json result = json::array();

                for (const auto& row : res) {
                    json book;
                    book["id"] = row["id"].as<int>();
                    book["title"] = row["title"].as<std::string>();
                    book["author"] = row["author"].as<std::string>();
                    book["year"] = row["year"].as<int>();

                    if (row["ISBN"].is_null()) {
                        book["ISBN"] = nullptr;
                    } else {
                        book["ISBN"] = row["ISBN"].as<std::string>();
                    }

                    result.push_back(book);
                }

                std::cout << result << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
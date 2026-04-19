#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/pqxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)}
    , authors_{connection_}
    , books_{connection_}
{
    pqxx::work work{connection_};

    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
    name varchar(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title varchar(100) NOT NULL,
    publication_year INTEGER NOT NULL
);
)"_zv);

    work.commit();
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year)
VALUES ($1, $2, $3, $4);
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPubYear()
    );
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    pqxx::work work{connection_};
    auto res = work.exec("SELECT id, author_id, title, publication_year FROM books ORDER BY title;");

    std::vector<domain::Book> result;
    for (const auto& row : res) {
        result.emplace_back(
            domain::BookId::FromString(row[0].c_str()),
            domain::AuthorId::FromString(row[1].c_str()),
            row[2].c_str(),
            row[3].as<int>());
    }
    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthor(const domain::AuthorId& author_id) const {
    pqxx::work work{connection_};
    auto res = work.exec_params(
        "SELECT id, author_id, title, publication_year FROM books WHERE author_id=$1 ORDER BY publication_year, title;",
        author_id.ToString());

    std::vector<domain::Book> result;
    for (const auto& row : res) {
        result.emplace_back(
            domain::BookId::FromString(row[0].c_str()),
            domain::AuthorId::FromString(row[1].c_str()),
            row[2].c_str(),
            row[3].as<int>());
    }
    return result;
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    pqxx::work work{connection_};
    auto res = work.exec("SELECT id, name FROM authors ORDER BY name;");

    std::vector<domain::Author> result;
    for (const auto& row : res) {
        result.emplace_back(
            domain::AuthorId::FromString(row[0].c_str()),
            row[1].c_str());
    }
    return result;
}

}  // namespace postgres
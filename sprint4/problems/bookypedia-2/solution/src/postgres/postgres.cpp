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

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID REFERENCES books(id) ON DELETE CASCADE,
    tag VARCHAR(30) NOT NULL
);
)");

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

void SaveTags(const domain::BookId& book_id,
              const std::vector<std::string>& tags) {
    pqxx::work work{connection_};

    for (const auto& tag : tags) {
        work.exec_params(
            "INSERT INTO book_tags (book_id, tag) VALUES ($1, $2)",
            book_id.ToString(), tag);
    }

    work.commit();
}

std::vector<std::string> GetTags(const domain::BookId& book_id) const {
    pqxx::work work{connection_};

    auto res = work.exec_params(
        "SELECT tag FROM book_tags WHERE book_id = $1 ORDER BY tag",
        book_id.ToString());

    std::vector<std::string> tags;

    for (const auto& row : res) {
        tags.push_back(row[0].c_str());
    }

    return tags;
}

void BookRepositoryImpl::Delete(const domain::BookId& id) {
    pqxx::work work{connection_};

    auto res = work.exec_params(
        "DELETE FROM books WHERE id=$1",
        id.ToString());

    if (res.affected_rows() == 0) {
        throw std::runtime_error("not found");
    }

    work.commit();
}

void AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    pqxx::work work{connection_};

    auto res = work.exec_params(
        "DELETE FROM authors WHERE id=$1",
        id.ToString());

    if (res.affected_rows() == 0) {
        throw std::runtime_error("not found");
    }

    work.commit();
}

void BookRepositoryImpl::Update(const domain::Book& book) {
    pqxx::work work{connection_};

    work.exec_params(
        "UPDATE books SET title=$1, publication_year=$2 WHERE id=$3",
        book.GetTitle(),
        book.GetPublicationYear(),
        book.GetId().ToString());

    work.commit();
}

std::vector<BookInfo> BookRepositoryImpl::GetAllWithAuthors() const {
    pqxx::work work{connection_};

    auto res = work.exec(R"(
        SELECT b.id, b.title, a.name, b.publication_year
        FROM books b
        JOIN authors a ON b.author_id = a.id
        ORDER BY b.title, a.name, b.publication_year
    )");

    std::vector<BookInfo> result;

    for (const auto& row : res) {
        result.push_back({
            domain::BookId::FromString(row[0].c_str()),
            row[1].c_str(),
            row[2].c_str(),
            row[3].as<int>()
        });
    }

    return result;
}

}  // namespace postgres
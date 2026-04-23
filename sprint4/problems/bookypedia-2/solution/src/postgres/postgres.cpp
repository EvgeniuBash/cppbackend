#include "postgres.h"

#include <pqxx/pqxx>
#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};

    work.exec_params(
        R"(
INSERT INTO authors (id, name)
VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name = EXCLUDED.name;
)"_zv,
        author.GetId().ToString(),
        author.GetName());

    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() const {
    pqxx::read_transaction work{connection_};
    auto res = work.exec(R"(
SELECT id, name
FROM authors
ORDER BY name;
)"_zv);

    std::vector<domain::Author> result;
    result.reserve(res.size());

    for (const auto& row : res) {
        result.emplace_back(
            domain::AuthorId::FromString(row["id"].c_str()),
            row["name"].c_str()
        );
    }

    return result;
}

bool AuthorRepositoryImpl::Delete(const domain::AuthorId& id) {
    pqxx::work work{connection_};

    work.exec_params(
        R"(
DELETE FROM book_tags
WHERE book_id IN (
    SELECT id FROM books WHERE author_id = $1
);
)"_zv,
        id.ToString()
    );

    work.exec_params(
        R"(
DELETE FROM books
WHERE author_id = $1;
)"_zv,
        id.ToString()
    );

    auto res = work.exec_params(
        R"(
DELETE FROM authors
WHERE id = $1;
)"_zv,
        id.ToString()
    );

    if (res.affected_rows() != 1) {
        return false;
    }

    work.commit();
    return true;
}

bool AuthorRepositoryImpl::Edit(const domain::AuthorId& id, const std::string& new_name) {
    pqxx::work work{connection_};

    auto res = work.exec_params(
        R"(
UPDATE authors
SET name = $2
WHERE id = $1;
)"_zv,
        id.ToString(),
        new_name
    );

    if (res.affected_rows() != 1) {
        return false;
    }

    work.commit();
    return true;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{connection_};

    work.exec_params(
        R"(
INSERT INTO books (id, author_id, title, publication_year)
VALUES ($1, $2, $3, $4)
ON CONFLICT (id) DO UPDATE
SET author_id = EXCLUDED.author_id,
    title = EXCLUDED.title,
    publication_year = EXCLUDED.publication_year;
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPubYear()
    );

    work.exec_params(
        R"(
DELETE FROM book_tags
WHERE book_id = $1;
)"_zv,
        book.GetId().ToString()
    );

    for (const auto& tag : book.GetTags()) {
        work.exec_params(
            R"(
INSERT INTO book_tags (book_id, tag)
VALUES ($1, $2);
)"_zv,
            book.GetId().ToString(),
            tag
        );
    }

    work.commit();
}

bool BookRepositoryImpl::Update(const domain::Book& book) {
    pqxx::work work{connection_};

    auto res = work.exec_params(
        R"(
UPDATE books
SET author_id = $2,
    title = $3,
    publication_year = $4
WHERE id = $1;
)"_zv,
        book.GetId().ToString(),
        book.GetAuthorId().ToString(),
        book.GetTitle(),
        book.GetPubYear()
    );

    if (res.affected_rows() != 1) {
        return false;
    }

    work.exec_params(
        R"(
DELETE FROM book_tags
WHERE book_id = $1;
)"_zv,
        book.GetId().ToString()
    );

    for (const auto& tag : book.GetTags()) {
        work.exec_params(
            R"(
INSERT INTO book_tags (book_id, tag)
VALUES ($1, $2);
)"_zv,
            book.GetId().ToString(),
            tag
        );
    }

    work.commit();
    return true;
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() const {
    pqxx::read_transaction work{connection_};

    auto res = work.exec(
        R"(
SELECT id, author_id, title, publication_year
FROM books
ORDER BY title, publication_year, id;
)"_zv
    );

    std::vector<domain::Book> result;
    result.reserve(res.size());

    for (const auto& row : res) {
        auto tag_res = work.exec_params(
            R"(
SELECT tag
FROM book_tags
WHERE book_id = $1
ORDER BY tag;
)"_zv,
            row["id"].c_str()
        );

        std::vector<std::string> tags;
        tags.reserve(tag_res.size());

        for (const auto& t : tag_res) {
            tags.push_back(t["tag"].c_str());
        }

        result.emplace_back(
            domain::BookId::FromString(row["id"].c_str()),
            domain::AuthorId::FromString(row["author_id"].c_str()),
            row["title"].c_str(),
            row["publication_year"].as<int>(),
            tags
        );
    }

    return result;
}

std::vector<domain::Book> BookRepositoryImpl::GetByAuthor(const domain::AuthorId& author_id) const {
    pqxx::read_transaction work{connection_};

    auto res = work.exec_params(
        R"(
SELECT id, author_id, title, publication_year
FROM books
WHERE author_id = $1
ORDER BY publication_year, title;
)"_zv,
        author_id.ToString()
    );

    std::vector<domain::Book> result;
    result.reserve(res.size());

    for (const auto& row : res) {
        auto tag_res = work.exec_params(
            R"(
SELECT tag
FROM book_tags
WHERE book_id = $1
ORDER BY tag;
)"_zv,
            row["id"].c_str()
        );

        std::vector<std::string> tags;
        tags.reserve(tag_res.size());

        for (const auto& t : tag_res) {
            tags.push_back(t["tag"].c_str());
        }

        result.emplace_back(
            domain::BookId::FromString(row["id"].c_str()),
            domain::AuthorId::FromString(row["author_id"].c_str()),
            row["title"].c_str(),
            row["publication_year"].as<int>(),
            tags
        );
    }

    return result;
}

bool BookRepositoryImpl::Delete(const domain::BookId& id) {
    pqxx::work work{connection_};

    work.exec_params(
        R"(
DELETE FROM book_tags
WHERE book_id = $1;
)"_zv,
        id.ToString()
    );

    auto res = work.exec_params(
        R"(
DELETE FROM books
WHERE id = $1;
)"_zv,
        id.ToString()
    );

    if (res.affected_rows() != 1) {
        return false;
    }

    work.commit();
    return true;
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)}
    , authors_{connection_}
    , books_{connection_} {
    pqxx::work work{connection_};

    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID PRIMARY KEY,
    author_id UUID NOT NULL REFERENCES authors(id),
    title VARCHAR(100) NOT NULL,
    publication_year INTEGER NOT NULL
);
)"_zv);

    work.exec(R"(
CREATE TABLE IF NOT EXISTS book_tags (
    book_id UUID NOT NULL REFERENCES books(id),
    tag VARCHAR(30) NOT NULL
);
)"_zv);

    work.commit();
}

}  // namespace postgres
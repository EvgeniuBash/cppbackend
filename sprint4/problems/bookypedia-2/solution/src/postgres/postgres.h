#pragma once

#include <pqxx/connection>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& conn)
        : connection_(conn) {
    }

    void Save(const domain::Author& author) override;
    std::vector<domain::Author> GetAll() const override;
    bool Delete(const domain::AuthorId& id) override;
    bool Edit(const domain::AuthorId& id, const std::string& new_name) override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& conn)
        : connection_(conn) {
    }

    void Save(const domain::Book& book) override;
    bool Update(const domain::Book& book) override;
    std::vector<domain::Book> GetAll() const override;
    std::vector<domain::Book> GetByAuthor(const domain::AuthorId& author_id) const override;
    bool Delete(const domain::BookId& id) override;

private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{connection_};
};

}  // namespace postgres
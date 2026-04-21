#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& conn)
        : connection_(conn) {}

    void Save(const domain::Author& author) override;
    void Delete(const domain::AuthorId& id) override;
    std::vector<domain::Author> GetAll() const override;

private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& conn)
        : connection_(conn) {}

    void Save(const domain::Book& book) override;
    void Delete(const domain::BookId& id) override;

    std::vector<domain::Book> GetAll() const override;
    std::vector<domain::Book> GetByAuthor(const domain::AuthorId& author_id) const override;

    void SaveTags(const domain::BookId& book_id,
              const std::vector<std::string>& tags);

    std::vector<std::string> GetTags(const domain::BookId& book_id) const;

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
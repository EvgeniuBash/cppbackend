#pragma once

#include "../domain/author.h"
#include "../domain/book.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    // Старый конструктор оставляем ради локальных unit-тестов
    explicit UseCasesImpl(domain::AuthorRepository& authors)
        : authors_(authors)
        , books_(dummy_books_) {
    }

    UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books);

    void AddAuthor(const std::string& name) override;
    bool EditAuthor(const std::string& id, const std::string& new_name) override;
    bool DeleteAuthor(const std::string& id) override;

    void AddBook(int year,
                 const std::string& title,
                 const std::string& author_name,
                 const std::vector<std::string>& tags) override;

    bool DeleteBook(const std::string& id) override;

    bool EditBook(const std::string& id,
                  const std::string& title,
                  int year,
                  const std::vector<std::string>& tags) override;

    std::vector<std::pair<std::string, std::string>> GetAuthors() const override;
    std::vector<std::pair<std::string, int>> GetBooks() const override;
    std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) const override;
    std::vector<BookInfo> GetBooksWithAuthors() const override;
    std::optional<BookInfo> GetBook(const std::string& title) const override;

private:
    domain::AuthorRepository& authors_;

    class DummyBookRepo : public domain::BookRepository {
    public:
        void Save(const domain::Book&) override {
        }

        bool Update(const domain::Book&) override {
            return false;
        }

        std::vector<domain::Book> GetAll() const override {
            return {};
        }

        std::vector<domain::Book> GetByAuthor(const domain::AuthorId&) const override {
            return {};
        }

        bool Delete(const domain::BookId&) override {
            return false;
        }
    } dummy_books_;

    domain::BookRepository& books_;
};

}  // namespace app

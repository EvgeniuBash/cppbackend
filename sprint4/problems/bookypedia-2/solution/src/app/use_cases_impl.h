#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors)
    : authors_(authors), books_(dummy_books_) {}

    explicit UseCasesImpl(domain::AuthorRepository& authors,
                          domain::BookRepository& books);

    void AddAuthor(const std::string& name) override;
    void DeleteBook(const domain::BookId& id) override;
    void DeleteAuthor(const domain::AuthorId& id) override;
    void EditAuthor(const domain::AuthorId& id, const std::string& new_name) override;

    void AddBook(int year,
                 const std::string& title,
                 const std::string& author_name,
                 const std::vector<std::string>& tags) override;

    std::vector<std::pair<std::string, std::string>> GetAuthors() const override;
    std::vector<std::pair<std::string, int>> GetBooks() const override;
    std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) const override;
    std::vector<BookInfo> GetBooksWithAuthors() const override;
    std::optional<BookInfo> GetBook(const std::string& title) const override;
    
    void EditBook(const std::string& id,
              const std::string& new_title,
              int new_year,
              const std::vector<std::string>& new_tags) override;
private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;

    class DummyBookRepo : public domain::BookRepository {
    public:
        void Save(const domain::Book&) override {}
        std::vector<domain::Book> GetAll() const override { return {}; }
        std::vector<domain::Book> GetByAuthor(const domain::AuthorId&) const override { return {}; }
        void Delete(const domain::BookId&) override {}
    } dummy_books_;
};

}  // namespace app

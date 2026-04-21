#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors,
                          domain::BookRepository& books);

    explicit UseCasesImpl(domain::AuthorRepository& authors);

    void AddAuthor(const std::string& name) override;
    void DeleteBook(const domain::BookId& id) override;
    void DeleteAuthor(const domain::AuthorId& id) override;

    void AddBook(int year,
                          const std::string& title,
                          const std::string& author_name,
                          const std::vector<std::string>& tags) override;

    std::vector<std::pair<std::string, std::string>> GetAuthors() const override;

    std::vector<std::pair<std::string, int>> GetBooks() const override;

    std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string& author_id) const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository* books_ = nullptr;;
};

}  // namespace app

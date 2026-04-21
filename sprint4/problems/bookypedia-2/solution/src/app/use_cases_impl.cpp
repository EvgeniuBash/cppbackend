#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(domain::AuthorRepository& authors,
                           domain::BookRepository& books)
    : authors_(authors)
    , books_(books) {}


void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(int year,
                          const std::string& title,
                          const std::string& author_name,
                          const std::vector<std::string>& tags) {

    domain::Author author{
        domain::AuthorId::New(),
        author_name
    };

    authors_.Save(author);

    domain::Book book(
        domain::BookId::New(),
        author.GetId(),
        title,
        year
    );

    books_.Save(book);
}

void UseCasesImpl::DeleteBook(const domain::BookId& id) {
    books_.Delete(id);
}

void UseCasesImpl::DeleteAuthor(const domain::AuthorId& id) {
    authors_.Delete(id);
}

std::vector<std::pair<std::string, std::string>> UseCasesImpl::GetAuthors() const {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& a : authors_.GetAll()) {
        result.emplace_back(a.GetId().ToString(), a.GetName());
    }
    return result;
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetBooks() const {
    std::vector<std::pair<std::string, int>> result;
    for (const auto& b : books_.GetAll()) {
        result.emplace_back(b.GetTitle(), b.GetPubYear());
    }
    return result;
}

std::vector<std::pair<std::string, int>> UseCasesImpl::GetAuthorBooks(const std::string& author_id) const {
    std::vector<std::pair<std::string, int>> result;
    for (const auto& b : books_.GetByAuthor(domain::AuthorId::FromString(author_id))) {
        result.emplace_back(b.GetTitle(), b.GetPubYear());
    }
    return result;
}

}  // namespace app

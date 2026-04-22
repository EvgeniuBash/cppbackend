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

void UseCasesImpl::DeleteBook(const domain::BookId&) {
}

void UseCasesImpl::DeleteAuthor(const domain::AuthorId&) {
}

void UseCasesImpl::AddBook(int year,
                          const std::string& title,
                          const std::string& author_name,
                          const std::vector<std::string>&) {

    // ищем автора
    domain::AuthorId author_id;

    for (const auto& a : authors_.GetAll()) {
        if (a.GetName() == author_name) {
            author_id = a.GetId();
            break;
        }
    }

    if (author_id == domain::AuthorId{}) {
        domain::Author new_author{domain::AuthorId::New(), author_name};
        author_id = new_author.GetId();
        authors_.Save(new_author);
    }

    domain::Book book(
        domain::BookId::New(),
        author_id,
        title,
        year
    );

    books_.Save(book);
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

std::vector<app::BookInfo> UseCasesImpl::GetBooksWithAuthors() const {
    std::vector<BookInfo> result;

    const auto authors = authors_.GetAll();

    for (const auto& b : books_.GetAll()) {
        std::string author_name;

        for (const auto& a : authors) {
            if (a.GetId() == b.GetAuthorId()) {
                author_name = a.GetName();
                break;
            }
        }

        result.push_back({
            b.GetId().ToString(),
            b.GetTitle(),
            author_name,
            b.GetPubYear()
        });
    }

    return result;
}

std::optional<BookInfo> UseCasesImpl::GetBook(const std::string& title) const {
    for (const auto& b : books_.GetAll()) {
        if (b.GetTitle() == title) {
            for (const auto& a : authors_.GetAll()) {
                if (a.GetId() == b.GetAuthorId()) {
                    return BookInfo{
                        b.GetId().ToString(),
                        b.GetTitle(),
                        a.GetName(),
                        b.GetPubYear()
                    };
                }
            }
        }
    }
    return std::nullopt;
}

}  // namespace app

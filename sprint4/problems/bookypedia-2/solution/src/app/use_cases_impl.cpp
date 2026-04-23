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
                          const std::vector<std::string>& tags) {

    // 1. Ищем автора
    auto authors = authors_.GetAll();

    auto it = std::find_if(authors.begin(), authors.end(),
        [&](const domain::Author& a) {
            return a.GetName() == author_name;
        });

    domain::AuthorId author_id;

    if (it != authors.end()) {
        // автор найден
        author_id = it->GetId();
    } else {
        // создаём нового
        domain::Author new_author{domain::AuthorId::New(), author_name};
        authors_.Save(new_author);
        author_id = new_author.GetId();
    }

    // 2. Создаём книгу
    domain::Book book(
        domain::BookId::New(),
        author_id,
        title,
        year
    );

    // ❗ если в Book есть теги — обязательно добавь это:
    // book.SetTags(tags);

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
    std::vector<app::BookInfo> result;

    auto authors = authors_.GetAll();

    for (const auto& b : books_.GetAll()) {
        auto it = std::find_if(authors.begin(), authors.end(),
            [&](const domain::Author& a) {
                return a.GetId() == b.GetAuthorId();
            });

        std::string author_name = (it != authors.end()) ? it->GetName() : "Unknown";

        result.push_back({
            b.GetId().ToString(),
            b.GetTitle(),
            author_name,
            b.GetPubYear(),
            b.GetTags()
        });
    }

    return result;
}

std::optional<app::BookInfo> UseCasesImpl::GetBook(const std::string& title) const {
    for (const auto& b : books_.GetAll()) {
        if (b.GetTitle() == title) {
            for (const auto& a : authors_.GetAll()) {
                if (a.GetId() == b.GetAuthorId()) {
                    return app::BookInfo{
                        b.GetId().ToString(),
                        b.GetTitle(),
                        a.GetName(),
                        b.GetPubYear(),
                        b.GetTags()
                    };
                }
            }
        }
    }
    return std::nullopt;
}

}  // namespace app

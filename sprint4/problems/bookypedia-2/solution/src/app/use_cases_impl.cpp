#include "use_cases_impl.h"

#include <algorithm>
#include <tuple>

#include <boost/algorithm/string/trim.hpp>

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(domain::AuthorRepository& authors,
                           domain::BookRepository& books)
    : authors_(authors)
    , books_(books) {
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

bool UseCasesImpl::EditAuthor(const std::string& id, const std::string& new_name) {
    std::string trimmed = new_name;
    boost::algorithm::trim(trimmed);
    if (trimmed.empty()) {
        return false;
    }

    return authors_.Edit(AuthorId::FromString(id), trimmed);
}

bool UseCasesImpl::DeleteAuthor(const std::string& id) {
    return authors_.Delete(AuthorId::FromString(id));
}

void UseCasesImpl::AddBook(int year,
                           const std::string& title,
                           const std::string& author_name,
                           const std::vector<std::string>& tags) {
    domain::AuthorId author_id;

    for (const auto& author : authors_.GetAll()) {
        if (author.GetName() == author_name) {
            author_id = author.GetId();
            break;
        }
    }

    if (author_id == domain::AuthorId{}) {
        domain::Author new_author{domain::AuthorId::New(), author_name};
        author_id = new_author.GetId();
        authors_.Save(new_author);
    }

    books_.Save(domain::Book(
        domain::BookId::New(),
        author_id,
        title,
        year,
        tags
    ));
}

bool UseCasesImpl::DeleteBook(const std::string& id) {
    return books_.Delete(BookId::FromString(id));
}

bool UseCasesImpl::EditBook(const std::string& id,
                            const std::string& title,
                            int year,
                            const std::vector<std::string>& tags) {
    auto books = books_.GetAll();

    auto it = std::find_if(books.begin(), books.end(),
        [&](const domain::Book& b) {
            return b.GetId().ToString() == id;
        });

    if (it == books.end()) {
        return false;
    }

    std::string new_title = title;
    boost::algorithm::trim(new_title);

    domain::Book updated(
        it->GetId(),
        it->GetAuthorId(),
        new_title.empty() ? it->GetTitle() : new_title,
        year == 0 ? it->GetPubYear() : year,
        tags
    );

    return books_.Update(updated);
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

std::vector<BookInfo> UseCasesImpl::GetBooksWithAuthors() const {
    std::vector<BookInfo> result;
    const auto authors = authors_.GetAll();

    for (const auto& b : books_.GetAll()) {
        auto it = std::find_if(authors.begin(), authors.end(),
            [&](const domain::Author& a) {
                return a.GetId() == b.GetAuthorId();
            });

        std::string author_name = (it != authors.end()) ? it->GetName() : "";

        result.push_back({
            b.GetId().ToString(),
            b.GetTitle(),
            author_name,
            b.GetPubYear(),
            b.GetTags()
        });
    }

    std::sort(result.begin(), result.end(),
        [](const BookInfo& lhs, const BookInfo& rhs) {
            return std::tie(lhs.title, lhs.author, lhs.publication_year)
                < std::tie(rhs.title, rhs.author, rhs.publication_year);
        });

    return result;
}

std::optional<BookInfo> UseCasesImpl::GetBook(const std::string& title) const {
    const auto books = GetBooksWithAuthors();

    auto it = std::find_if(books.begin(), books.end(),
        [&](const BookInfo& book) {
            return book.title == title;
        });

    if (it == books.end()) {
        return std::nullopt;
    }

    return *it;
}

}  // namespace app

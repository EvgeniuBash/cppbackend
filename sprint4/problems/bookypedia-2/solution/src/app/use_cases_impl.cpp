#include "use_cases_impl.h"
#include <boost/algorithm/string.hpp>

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

void UseCasesImpl::DeleteAuthor(const domain::AuthorId&) {
}

void UseCasesImpl::DeleteBook(const domain::BookId& id) {
    books_.Delete(id);
}

void UseCasesImpl::AddBook(int year,
                          const std::string& title,
                          const std::string& author_name,
                          const std::vector<std::string>& tags) {

    auto authors = authors_.GetAll();

    auto it = std::find_if(authors.begin(), authors.end(),
        [&](const domain::Author& a) {
            return a.GetName() == author_name;
        });

    domain::AuthorId author_id;

    if (it != authors.end()) {
        author_id = it->GetId();
    } else {
        domain::Author new_author{domain::AuthorId::New(), author_name};
        authors_.Save(new_author);
        author_id = new_author.GetId();
    }

    domain::Book book(
        domain::BookId::New(),
        author_id,
        title,
        year,
        tags
    );

    books_.Save(book);
}

void UseCasesImpl::EditBook(const std::string& id,
                           const std::string& title,
                           int year,
                           const std::vector<std::string>& tags) {
    auto books = books_.GetAll();

    auto it = std::find_if(books.begin(), books.end(),
        [&](const domain::Book& b) {
            return b.GetId().ToString() == id;
        });

    if (it == books.end()) {
        return;
    }

    std::string new_title = title;
    boost::algorithm::trim(new_title);

    domain::Book updated(
        it->GetId(),
        it->GetAuthorId(),
        new_title.empty() ? it->GetTitle() : new_title,
        year == 0 ? it->GetPubYear() : year,
        tags.empty() ? it->GetTags() : tags
    );

    books_.Save(updated);
}

void UseCasesImpl::EditAuthor(const domain::AuthorId& id, const std::string& new_name) {
    // Находим автора
    auto authors = authors_.GetAll();
    auto it = std::find_if(authors.begin(), authors.end(),
        [&](const domain::Author& a) { return a.GetId() == id; });
    
    if (it != authors.end()) {
        // Создаём автора с тем же ID и новым именем
        domain::Author updated_author(id, new_name);
        authors_.Save(updated_author);
    }
}

void UseCasesImpl::EditBook(const domain::BookId& id,
                           const std::string& new_title,
                           int new_year,
                           const std::vector<std::string>& new_tags) {
    auto books = books_.GetAll();
    
    auto it = std::find_if(books.begin(), books.end(),
        [&](const domain::Book& b) { return b.GetId() == id; });
    
    if (it != books.end()) {
        // Если значения пустые, оставляем старые
        std::string title = new_title.empty() ? it->GetTitle() : new_title;
        int year = (new_year == 0) ? it->GetPubYear() : new_year;
        const auto& tags = new_tags.empty() ? it->GetTags() : new_tags;
        
        domain::Book updated_book(id, it->GetAuthorId(), title, year, tags);
        books_.Save(updated_book);
    }
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

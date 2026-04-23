#pragma once

#include <string>
#include <vector>
#include <optional>

#include "../domain/book.h"
#include "../domain/author.h"

namespace app {

struct BookInfo {
    std::string id;
    std::string title;
    std::string author;
    int publication_year;
    std::vector<std::string> tags;
};

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual bool EditAuthor(const std::string& id, const std::string& new_name) = 0;
    virtual bool DeleteAuthor(const std::string& id) = 0;
    virtual void AddBook(int year,
                     const std::string& title,
                     const std::string& author_id,
                     const std::vector<std::string>& tags) = 0;

    virtual bool DeleteBook(const std::string& id) = 0;

    virtual void EditBook(const std::string& id,
                         const std::string& title,
                         int year,
                         const std::vector<std::string>& tags) = 0;

    virtual std::vector<std::pair<std::string, std::string>> GetAuthors() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetBooks() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string&) const = 0;
    virtual std::vector<BookInfo> GetBooksWithAuthors() const = 0;
    virtual std::optional<BookInfo> GetBook(const std::string& title) const = 0;
    virtual std::optional<BookInfo> GetBookById(const std::string& id) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app

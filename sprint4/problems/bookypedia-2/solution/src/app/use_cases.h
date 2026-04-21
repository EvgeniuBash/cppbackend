#pragma once

#include <string>
#include <vector>

namespace app {

struct BookInfo {
    std::string id;
    std::string title;
    std::string author;
    int publication_year;
};

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(int year,
                     const std::string& title,
                     const std::string& author_id,
                     const std::vector<std::string>& tags) = 0;

    virtual std::vector<std::pair<std::string, std::string>> GetAuthors() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetBooks() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string&) const = 0;
    virtual std::vector<BookInfo> GetBooksWithAuthors() const = 0;
    virtual void DeleteBook(const domain::BookId& id) = 0;
    virtual void DeleteAuthor(const domain::AuthorId& id) = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app

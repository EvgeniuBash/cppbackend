#pragma once

#include <string>
#include <vector>

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(int pub_year, const std::string& title, const std::string& author_id) = 0;

    virtual std::vector<std::pair<std::string, std::string>> GetAuthors() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetBooks() const = 0;
    virtual std::vector<std::pair<std::string, int>> GetAuthorBooks(const std::string&) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app

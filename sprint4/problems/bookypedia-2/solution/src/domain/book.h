#pragma once
#include <string>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {

namespace detail {
struct BookTag {};
}

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId id, AuthorId author_id, std::string title, int pub_year, std::vector<std::string> tags)
        : id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , pub_year_(pub_year)
        , tags_(std::move(tags)) {
    }

    const BookId& GetId() const noexcept { return id_; }
    const AuthorId& GetAuthorId() const noexcept { return author_id_; }
    const std::string& GetTitle() const noexcept { return title_; }
    int GetPubYear() const noexcept { return pub_year_; }
    const std::vector<std::string>& GetTags() const { return tags_; }

private:
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int pub_year_;
    std::vector<std::string> tags_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;
    virtual bool Update(const Book& book) = 0;
    virtual std::vector<Book> GetAll() const = 0;
    virtual std::vector<Book> GetByAuthor(const AuthorId& author_id) const = 0;
    virtual bool Delete(const BookId& id) = 0;

protected:
    ~BookRepository() = default;
};

}  // namespace domain
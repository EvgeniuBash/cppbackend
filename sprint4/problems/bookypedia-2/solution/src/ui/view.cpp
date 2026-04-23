#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <sstream>
#include <cassert>
#include <iostream>

#include "../app/use_cases.h"
#include "../menu/menu.h"
#include "../util/tagged.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {

    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s,
        std::bind(&View::AddAuthor, this, ph::_1));

    menu_.AddAction("AddBook"s, "", "Adds book"s,
        std::bind(&View::AddBook, this, ph::_1));

    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s,
        std::bind(&View::ShowAuthors, this));

    menu_.AddAction("ShowBooks"s, {}, "Show books"s,
        std::bind(&View::ShowBooks, this));

    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
        std::bind(&View::ShowAuthorBooks, this));

    menu_.AddAction("ShowBook"s, "title"s, "Show book"s,
        std::bind(&View::ShowBook, this, ph::_1));

    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s,
        std::bind(&View::DeleteBook, this, ph::_1));

    menu_.AddAction("EditBook"s, "", "Edit book"s,
        std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        if (name.empty()) {
            output_ << "Failed to add author" << std::endl;
            return true;
        }

        use_cases_.AddAuthor(name);

    } catch (...) {
        output_ << "Failed to add author" << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        std::string author;
        std::getline(cmd_input, author);
        boost::algorithm::trim(author);

        std::string year_str;
        std::getline(cmd_input, year_str);
        int year = std::stoi(year_str);

        std::string tags_line;
        std::getline(cmd_input, tags_line);

        std::vector<std::string> tags;
        std::stringstream ss(tags_line);
        std::string tag;

        while (std::getline(ss, tag, ',')) {
            boost::algorithm::trim(tag);
            if (!tag.empty()) {
                tags.push_back(tag);
            }
        }

        use_cases_.AddBook(year, title, author, tags);

    } catch (...) {
        output_ << "Failed to add book" << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    const auto books = use_cases_.GetBooksWithAuthors();

    int i = 1;
    for (const auto& b : books) {
        output_ << i++ << " "
                << b.title << " by "
                << b.author << ", "
                << b.publication_year << std::endl;
    }

    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    auto books = use_cases_.GetBooksWithAuthors();

    std::vector<app::BookInfo> found;

    for (const auto& b : books) {
        if (title.empty() || b.title == title) {
            found.push_back(b);
        }
    }

    if (found.empty()) {
        return true;
    }

    if (found.size() > 1) {
        for (size_t i = 0; i < found.size(); ++i) {
            output_ << i + 1 << " "
                    << found[i].title << " by "
                    << found[i].author << ", "
                    << found[i].publication_year << std::endl;
        }

        output_ << "Enter the book # or empty line to cancel:" << std::endl;

        std::string line;
        if (!std::getline(input_, line) || line.empty()) {
            return true;
        }

        int idx = std::stoi(line) - 1;

        if (idx < 0 || idx >= (int)found.size()) {
            return true;
        }

        auto& book = found[idx];

        output_ << "Title: " << book.title << std::endl;
        output_ << "Author: " << book.author << std::endl;
        output_ << "Publication year: " << book.publication_year << std::endl;

        if (!book.tags.empty()) {
            auto tags = book.tags;
            std::sort(tags.begin(), tags.end());

            output_ << "Tags: ";
            for (size_t i = 0; i < tags.size(); ++i) {
                if (i) output_ << ", ";
                output_ << tags[i];
            }
            output_ << std::endl;
        }

        return true;
    }

    auto& book = found[0];

    output_ << "Title: " << book.title << std::endl;
    output_ << "Author: " << book.author << std::endl;
    output_ << "Publication year: " << book.publication_year << std::endl;

    if (!book.tags.empty()) {
        auto tags = book.tags;
        std::sort(tags.begin(), tags.end());

        output_ << "Tags: ";
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i) output_ << ", ";
            output_ << tags[i];
        }
        output_ << std::endl;
    }

    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    auto books = use_cases_.GetBooksWithAuthors();

    std::vector<app::BookInfo> found;

    for (const auto& b : books) {
        if (title.empty() || b.title == title) {
            found.push_back(b);
        }
    }

    if (found.empty()) {
        output_ << "Failed to delete book" << std::endl;
        return true;
    }

    for (size_t i = 0; i < found.size(); ++i) {
        output_ << i + 1 << " "
                << found[i].title << " by "
                << found[i].author << ", "
                << found[i].publication_year << std::endl;
    }

    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string line;
    if (!std::getline(input_, line) || line.empty()) {
        return true;
    }

    int idx = std::stoi(line) - 1;

    if (idx < 0 || idx >= (int)found.size()) {
        output_ << "Failed to delete book" << std::endl;
        return true;
    }

    use_cases_.DeleteBook(domain::BookId::FromString(found[idx].id));
    return true;
}

bool View::EditBook(std::istream& input) const {
    std::string title;
    std::getline(input, title);
    boost::algorithm::trim(title);

    auto book = use_cases_.GetBook(title);
    if (!book) {
        output_ << "Book not found" << std::endl;
        return true;
    }

    std::string new_title;
    std::getline(input, new_title);
    boost::algorithm::trim(new_title);

    std::string year_str;
    std::getline(input, year_str);
    int year = year_str.empty() ? 0 : std::stoi(year_str);

    std::string tags_line;
    std::getline(input, tags_line);

    std::vector<std::string> tags;
    std::stringstream ss(tags_line);
    std::string tag;

    while (std::getline(ss, tag, ',')) {
        boost::algorithm::trim(tag);
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }

    use_cases_.EditBook(book->id, new_title, year, tags);
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (...) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int idx = std::stoi(str) - 1;
    if (idx < 0 || idx >= (int)authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> result;
    for (auto& [id, name] : use_cases_.GetAuthors()) {
        result.push_back({id, name});
    }
    return result;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> result;
    for (auto& [title, year] : use_cases_.GetBooks()) {
        result.push_back({title, year});
    }
    return result;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> result;
    for (auto& [title, year] : use_cases_.GetAuthorBooks(author_id)) {
        result.push_back({title, year});
    }
    return result;
}

}  // namespace ui

#include "view.h"

#include <boost/algorithm/string/trim.hpp>
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
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("ShowBook"s, "title"s, "Show book"s,
                std::bind(&View::ShowBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        if (name.empty()) {
            output_ << "Failed to add author"sv << std::endl;
            return true;
        }

        use_cases_.AddAuthor(std::move(name));

    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);

        std::string author_name;
        std::getline(cmd_input, author_name);

        std::string year_str;
        std::getline(cmd_input, year_str);
        int year = std::stoi(year_str);

        std::string tags_input;
        std::getline(cmd_input, tags_input);

        auto tags = util::NormalizeTags(tags_input);

        use_cases_.AddBook(year, title, author_name, tags);
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

    if (books.empty()) {
        output_ << "No books found" << std::endl;
        return true;
    }

    int i = 1;
    for (const auto& b : books) {
        output_ << i++ << " "
                << b.title << " by "
                << b.author << ", "
                << b.publication_year << std::endl;
    }

    return true;
}

bool View::ShowBook(std::istream& input) const {
    std::string title;
    std::getline(input, title);
    boost::algorithm::trim(title);

    const auto book = use_cases_.GetBook(title);

    if (!book) {
    return true;
    }

    output_ << "Title: " << book->title << std::endl;
    output_ << "Author: " << book->author << std::endl;
    output_ << "Publication year: " << book->year << std::endl;

    if (!book->tags.empty()) {
        output_ << "Tags: ";
        for (size_t i = 0; i < book->tags.size(); ++i) {
            if (i > 0) output_ << ", ";
            output_ << book->tags[i];
        }
        output_ << std::endl;
    }

    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    for (const auto& b : use_cases_.GetBooksWithAuthors()) {
        if (b.title == title) {
            use_cases_.DeleteBook(domain::BookId::FromString(b.id));
            return true;
        }
    }

    output_ << "Book not found" << std::endl;
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    auto author_id = SelectAuthor();
    if (not author_id.has_value())
        return std::nullopt;
    else {
        params.author_id = author_id.value();
        return params;
    }
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

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
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

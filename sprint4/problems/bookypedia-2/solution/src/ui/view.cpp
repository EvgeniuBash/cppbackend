#include "view.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string/trim.hpp>

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

namespace {

void DrainPendingLine(std::istream& input) {
    if (input.rdbuf()->in_avail() > 0) {
        std::string dummy;
        std::getline(input, dummy);
    }
}

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (const auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

void PrintBooksFull(std::ostream& out, const std::vector<app::BookInfo>& books) {
    int i = 1;
    for (const auto& b : books) {
        out << i++ << " " << b.title << " by " << b.author << ", " << b.publication_year << std::endl;
    }
}

std::vector<app::BookInfo> FilterBooksByTitle(const std::vector<app::BookInfo>& books,
                                              const std::string& title) {
    std::vector<app::BookInfo> result;
    for (const auto& book : books) {
        if (book.title == title) {
            result.push_back(book);
        }
    }
    return result;
}

std::optional<app::BookInfo> SelectBookFromList(const std::vector<app::BookInfo>& books,
                                                std::istream& input,
                                                std::ostream& output) {
    if (books.empty()) {
        return std::nullopt;
    }

    PrintBooksFull(output, books);
    output << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input, str) || str.empty()) {
        return std::nullopt;
    }

    int idx = std::stoi(str) - 1;
    if (idx < 0 || idx >= static_cast<int>(books.size())) {
        throw std::runtime_error("Invalid book num");
    }

    return books[idx];
}

std::string JoinTagsSorted(std::vector<std::string> tags) {
    std::sort(tags.begin(), tags.end());

    std::ostringstream out;
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i) {
            out << ", ";
        }
        out << tags[i];
    }
    return out.str();
}

}  // namespace

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

    menu_.AddAction("DeleteAuthor"s, "name"s, "Delete author"s,
        std::bind(&View::DeleteAuthor, this, ph::_1));

    menu_.AddAction("EditBook"s, "title"s, "Edit book"s,
        std::bind(&View::EditBook, this, ph::_1));

    menu_.AddAction("EditAuthor"s, "name"s, "Edit author"s,
        std::bind(&View::EditAuthor, this, ph::_1));
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
        int year = 0;
        if (!(cmd_input >> year)) {
            output_ << "Failed to add book" << std::endl;
            return true;
        }

        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        if (title.empty()) {
            output_ << "Failed to add book" << std::endl;
            return true;
        }

        output_ << "Enter author name or empty line to select from list:" << std::endl;

        std::string author_name;
        std::getline(input_, author_name);
        boost::algorithm::trim(author_name);

        const auto authors = GetAuthors();

        if (author_name.empty()) {
            if (authors.empty()) {
                output_ << "Failed to add book" << std::endl;
                return true;
            }

            output_ << "Select author:" << std::endl;
            PrintVector(output_, authors);
            output_ << "Enter author # or empty line to cancel" << std::endl;

            std::string str;
            if (!std::getline(input_, str) || str.empty()) {
                output_ << "Enter tags (comma separated):" << std::endl;
                std::string dummy_tags;
                std::getline(input_, dummy_tags);

                output_ << "Failed to add book" << std::endl;
                return true;
            }

            int idx = std::stoi(str) - 1;
            if (idx < 0 || idx >= static_cast<int>(authors.size())) {
                output_ << "Enter tags (comma separated):" << std::endl;
                std::string dummy_tags;
                std::getline(input_, dummy_tags);

                output_ << "Failed to add book" << std::endl;
                return true;
            }

            author_name = authors[idx].name;
        } else {
            const auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) {
                    return a.name == author_name;
                });

            if (it == authors.end()) {
                output_ << "No author found. Do you want to add " << author_name << " (y/n)?" << std::endl;

                std::string answer;
                std::getline(input_, answer);
                boost::algorithm::trim(answer);

                if (answer != "y" && answer != "Y") {
                    output_ << "Failed to add book" << std::endl;
                    return true;
                }

                use_cases_.AddAuthor(author_name);
            }
        }

        output_ << "Enter tags (comma separated):" << std::endl;
        std::string tags_line;
        std::getline(input_, tags_line);

        use_cases_.AddBook(year, title, author_name, util::NormalizeTags(tags_line));
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
    PrintBooksFull(output_, books);
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        const auto all_books = use_cases_.GetBooksWithAuthors();
        if (all_books.empty()) {
            return true;
        }

        std::optional<app::BookInfo> selected;

        if (title.empty()) {
            selected = SelectBookFromList(all_books, input_, output_);
            if (!selected) {
                return true;
            }
        } else {
            auto matched = FilterBooksByTitle(all_books, title);

            if (matched.empty()) {
                return true;
            }

            if (matched.size() == 1) {
                selected = matched.front();
            } else {
                selected = SelectBookFromList(matched, input_, output_);
                if (!selected) {
                    return true;
                }
            }
        }

        output_ << "Title: " << selected->title << std::endl;
        output_ << "Author: " << selected->author << std::endl;
        output_ << "Publication year: " << selected->publication_year << std::endl;

        if (!selected->tags.empty()) {
            output_ << "Tags: " << JoinTagsSorted(selected->tags) << std::endl;
        }
    } catch (...) {
    }

    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        const auto all_books = use_cases_.GetBooksWithAuthors();
        std::vector<app::BookInfo> candidates;

        if (title.empty()) {
            candidates = all_books;
        } else {
            candidates = FilterBooksByTitle(all_books, title);
        }

        if (candidates.empty()) {
            output_ << "Book not found" << std::endl;
            return true;
        }

        std::optional<app::BookInfo> selected;
        if (title.empty() || candidates.size() > 1) {
            selected = SelectBookFromList(candidates, input_, output_);
            if (!selected) {
                return true;   // cancel = молча
            }
        } else {
            selected = candidates.front();
        }

        if (!use_cases_.DeleteBook(selected->id)) {
            output_ << "Failed to delete book" << std::endl;
        }
    } catch (...) {
        output_ << "Failed to delete book" << std::endl;
    }

    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        const auto authors = GetAuthors();
        std::string author_id;

        if (name.empty()) {
            output_ << "Select author:" << std::endl;
            PrintVector(output_, authors);
            output_ << "Enter author # or empty line to cancel" << std::endl;

            std::string str;
            if (!std::getline(input_, str) || str.empty()) {
                return true;
            }

            int idx = std::stoi(str) - 1;
            if (idx < 0 || idx >= static_cast<int>(authors.size())) {
                output_ << "Failed to delete author" << std::endl;
                return true;
            }

            author_id = authors[idx].id;
        } else {
            const auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) {
                    return a.name == name;
                });

            if (it == authors.end()) {
                output_ << "Failed to delete author" << std::endl;
                return true;
            }

            author_id = it->id;
        }

        if (!use_cases_.DeleteAuthor(author_id)) {
            output_ << "Failed to delete author" << std::endl;
        }
    } catch (...) {
        output_ << "Failed to delete author" << std::endl;
    }

    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        const auto all_books = use_cases_.GetBooksWithAuthors();
        std::vector<app::BookInfo> candidates;

        if (title.empty()) {
            candidates = all_books;
        } else {
            candidates = FilterBooksByTitle(all_books, title);
        }

        if (candidates.empty()) {
            output_ << "Book not found" << std::endl;
            return true;
        }

        std::optional<app::BookInfo> selected;
        if (title.empty() || candidates.size() > 1) {
            selected = SelectBookFromList(candidates, input_, output_);
            if (!selected) {
                output_ << "Book not found" << std::endl;
                return true;
            }
        } else {
            selected = candidates.front();
        }

        output_ << "Enter new title or empty line to use the current one (" << selected->title << "):" << std::endl;

        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);

        output_ << "Enter publication year or empty line to use the current one (" << selected->publication_year << "):" << std::endl;

        std::string year_str;
        std::getline(input_, year_str);
        boost::algorithm::trim(year_str);

        int year = 0;
        if (!year_str.empty()) {
            year = std::stoi(year_str);
        }

        output_ << "Enter tags (current tags: " << JoinTagsSorted(selected->tags) << "):" << std::endl;
        std::string tags_line;
        std::getline(input_, tags_line);

        if (!use_cases_.EditBook(selected->id, new_title, year, util::NormalizeTags(tags_line))) {
            output_ << "Book not found" << std::endl;
        }
    } catch (...) {
        output_ << "Book not found" << std::endl;
    }

    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        const auto authors = GetAuthors();
        std::string author_id;

        if (name.empty()) {
            output_ << "Select author:" << std::endl;
            PrintVector(output_, authors);
            output_ << "Enter author # or empty line to cancel" << std::endl;

            std::string str;
            if (!std::getline(input_, str) || str.empty()) {
                return true;
            }

            int idx = std::stoi(str) - 1;
            if (idx < 0 || idx >= static_cast<int>(authors.size())) {
                output_ << "Failed to edit author" << std::endl;
                return true;
            }

            author_id = authors[idx].id;
        } else {
            const auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) {
                    return a.name == name;
                });

            if (it == authors.end()) {
                output_ << "Failed to edit author" << std::endl;
                return true;
            }

            author_id = it->id;
        }

        output_ << "Enter new name:" << std::endl;

        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);

        if (!use_cases_.EditAuthor(author_id, new_name)) {
            output_ << "Failed to edit author" << std::endl;
        }
    } catch (...) {
        output_ << "Failed to edit author" << std::endl;
    }

    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (...) {
        output_ << "Failed to Show Books" << std::endl;
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
    if (idx < 0 || idx >= static_cast<int>(authors.size())) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> result;
    for (const auto& [id, name] : use_cases_.GetAuthors()) {
        result.push_back({id, name});
    }
    return result;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> result;
    for (const auto& [title, year] : use_cases_.GetBooks()) {
        result.push_back({title, year});
    }
    return result;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> result;
    for (const auto& [title, year] : use_cases_.GetAuthorBooks(author_id)) {
        result.push_back({title, year});
    }
    return result;
}

}  // namespace ui

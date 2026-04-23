#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <sstream>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "../app/use_cases.h"
#include "../menu/menu.h"
#include "../util/tagged_uuid.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {

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

    // Новые команды
    menu_.AddAction("DeleteAuthor"s, "name"s, "Delete author"s,
        std::bind(&View::DeleteAuthor, this, ph::_1));

    menu_.AddAction("EditAuthor"s, "name"s, "Edit author"s,
        std::bind(&View::EditAuthor, this, ph::_1));

    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s,
        std::bind(&View::DeleteBook, this, ph::_1));

    menu_.AddAction("EditBook"s, "title"s, "Edit book"s,
        std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        // Читаем год и название из команды
        std::string year_str;
        std::getline(cmd_input, year_str);
        boost::algorithm::trim(year_str);
        
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        if (year_str.empty() || title.empty()) {
            output_ << "Failed to add book" << std::endl;
            return true;
        }
        
        int year = std::stoi(year_str);
        
        // Запрос автора
        output_ << "Enter author name or empty line to select from list:" << std::endl;
        std::string author_input;
        std::getline(input_, author_input);
        boost::algorithm::trim(author_input);
        
        std::string author_name;
        
        if (author_input.empty()) {
            // Выбор из списка
            auto author_id = SelectAuthor();
            if (!author_id) {
                output_ << "Failed to add book" << std::endl;
                return true;
            }
            auto authors = GetAuthors();
            auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) { return a.id == *author_id; });
            if (it != authors.end()) {
                author_name = it->name;
            } else {
                output_ << "Failed to add book" << std::endl;
                return true;
            }
        } else {
            author_name = author_input;
        }
        
        // Проверка существования автора
        auto authors = GetAuthors();
        auto it = std::find_if(authors.begin(), authors.end(),
            [&](const detail::AuthorInfo& a) { return a.name == author_name; });
        
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
        
        // Запрос тегов
        output_ << "Enter tags (comma separated):" << std::endl;
        std::string tags_line;
        std::getline(input_, tags_line);
        
        std::vector<std::string> tags = util::NormalizeTags(tags_line);
        
        use_cases_.AddBook(year, title, author_name, tags);
        
    } catch (...) {
        output_ << "Failed to add book" << std::endl;
    }
    return true;
}

bool View::ShowBooks() const {
    auto books = use_cases_.GetBooksWithAuthors();
    
    // Сортировка: по названию, затем по автору, затем по году
    std::sort(books.begin(), books.end(),
        [](const app::BookInfo& a, const app::BookInfo& b) {
            if (a.title != b.title) return a.title < b.title;
            if (a.author != b.author) return a.author < b.author;
            return a.publication_year < b.publication_year;
        });
    
    int i = 1;
    for (const auto& b : books) {
        output_ << i++ << " " << b.title << " by " << b.author << ", " << b.publication_year << std::endl;
    }
    
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);
    
    auto books = use_cases_.GetBooksWithAuthors();
    
    // Находим все книги с таким названием
    std::vector<app::BookInfo> matching_books;
    for (const auto& b : books) {
        if (b.title == title) {
            matching_books.push_back(b);
        }
    }
    
    if (matching_books.empty()) {
        return true;  // Ничего не выводим
    }
    
    // Сортируем для вывода
    std::sort(matching_books.begin(), matching_books.end(),
        [](const app::BookInfo& a, const app::BookInfo& b) {
            if (a.title != b.title) return a.title < b.title;
            if (a.author != b.author) return a.author < b.author;
            return a.publication_year < b.publication_year;
        });
    
    app::BookInfo selected_book;
    
    if (matching_books.size() == 1 && !title.empty()) {
        selected_book = matching_books[0];
    } else {
        // Выводим список
        for (size_t i = 0; i < matching_books.size(); ++i) {
            output_ << i + 1 << " " << matching_books[i].title 
                    << " by " << matching_books[i].author 
                    << ", " << matching_books[i].publication_year << std::endl;
        }
        
        output_ << "Enter the book # or empty line to cancel:" << std::endl;
        std::string choice;
        std::getline(input_, choice);
        boost::algorithm::trim(choice);
        
        if (choice.empty()) {
            return true;
        }
        
        int idx = std::stoi(choice) - 1;
        if (idx < 0 || idx >= static_cast<int>(matching_books.size())) {
            return true;
        }
        
        selected_book = matching_books[idx];
    }
    
    // Выводим информацию о книге
    output_ << "Title: " << selected_book.title << std::endl;
    output_ << "Author: " << selected_book.author << std::endl;
    output_ << "Publication year: " << selected_book.publication_year << std::endl;
    
    if (!selected_book.tags.empty()) {
        output_ << "Tags: ";
        for (size_t i = 0; i < selected_book.tags.size(); ++i) {
            if (i > 0) output_ << ", ";
            output_ << selected_book.tags[i];
        }
        output_ << std::endl;
    }
    
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        auto books = use_cases_.GetBooksWithAuthors();
        
        // Находим все книги с таким названием
        std::vector<app::BookInfo> matching_books;
        for (const auto& b : books) {
            if (b.title == title) {
                matching_books.push_back(b);
            }
        }
        
        if (matching_books.empty() && !title.empty()) {
            output_ << "Book not found" << std::endl;
            return true;
        }
        
        app::BookInfo selected_book;
        
        if (matching_books.size() == 1 && !title.empty()) {
            selected_book = matching_books[0];
        } else {
            // Выводим список
            int counter = 1;
            for (const auto& b : books) {
                output_ << counter++ << " " << b.title << " by " << b.author << ", " << b.publication_year << std::endl;
            }
            
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string choice;
            std::getline(input_, choice);
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return true;
            }
            
            int idx = std::stoi(choice) - 1;
            if (idx < 0 || idx >= static_cast<int>(books.size())) {
                return true;
            }
            
            selected_book = books[idx];
        }
        
        use_cases_.DeleteBook(domain::BookId::FromString(selected_book.id));
        
    } catch (...) {
        output_ << "Failed to delete book" << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        auto books = use_cases_.GetBooksWithAuthors();
        
        // Находим все книги с таким названием
        std::vector<app::BookInfo> matching_books;
        for (const auto& b : books) {
            if (b.title == title) {
                matching_books.push_back(b);
            }
        }
        
        if (matching_books.empty() && !title.empty()) {
            output_ << "Book not found" << std::endl;
            return true;
        }
        
        app::BookInfo selected_book;
        
        if (matching_books.size() == 1 && !title.empty()) {
            selected_book = matching_books[0];
        } else if (!matching_books.empty()) {
            // Выводим список
            for (size_t i = 0; i < matching_books.size(); ++i) {
                output_ << i + 1 << " " << matching_books[i].title 
                        << " by " << matching_books[i].author 
                        << ", " << matching_books[i].publication_year << std::endl;
            }
            
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string choice;
            std::getline(input_, choice);
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return true;
            }
            
            int idx = std::stoi(choice) - 1;
            if (idx < 0 || idx >= static_cast<int>(matching_books.size())) {
                return true;
            }
            
            selected_book = matching_books[idx];
        } else {
            // Выбираем из всех книг
            for (size_t i = 0; i < books.size(); ++i) {
                output_ << i + 1 << " " << books[i].title 
                        << " by " << books[i].author 
                        << ", " << books[i].publication_year << std::endl;
            }
            
            output_ << "Enter the book # or empty line to cancel:" << std::endl;
            std::string choice;
            std::getline(input_, choice);
            boost::algorithm::trim(choice);
            
            if (choice.empty()) {
                return true;
            }
            
            int idx = std::stoi(choice) - 1;
            if (idx < 0 || idx >= static_cast<int>(books.size())) {
                return true;
            }
            
            selected_book = books[idx];
        }
        
        // Редактирование
        output_ << "Enter new title or empty line to use the current one (" << selected_book.title << "):" << std::endl;
        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);
        
        output_ << "Enter publication year or empty line to use the current one (" << selected_book.publication_year << "):" << std::endl;
        std::string year_str;
        std::getline(input_, year_str);
        boost::algorithm::trim(year_str);
        int new_year = year_str.empty() ? 0 : std::stoi(year_str);
        
        // Формируем строку с текущими тегами
        std::string current_tags_str;
        for (size_t i = 0; i < selected_book.tags.size(); ++i) {
            if (i > 0) current_tags_str += ", ";
            current_tags_str += selected_book.tags[i];
        }
        
        output_ << "Enter tags (current tags: " << (current_tags_str.empty() ? "none" : current_tags_str) << "):" << std::endl;
        std::string tags_line;
        std::getline(input_, tags_line);
        boost::algorithm::trim(tags_line);
        
        std::vector<std::string> new_tags;
        if (!tags_line.empty()) {
            new_tags = util::NormalizeTags(tags_line);
        }
        
        use_cases_.EditBook(
            domain::BookId::FromString(selected_book.id),
            new_title,
            new_year,
            new_tags
        );
        
    } catch (const std::exception& e) {
        output_ << "Failed to edit book" << std::endl;
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        if (!name.empty()) {
            // Удаление по имени
            auto authors = GetAuthors();
            auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) { return a.name == name; });
            
            if (it != authors.end()) {
                use_cases_.DeleteAuthor(domain::AuthorId::FromString(it->id));
            } else {
                output_ << "Failed to delete author" << std::endl;
            }
        } else {
            // Выбор из списка
            auto author_id = SelectAuthor();
            if (author_id) {
                use_cases_.DeleteAuthor(domain::AuthorId::FromString(*author_id));
            }
        }
    } catch (...) {
        output_ << "Failed to delete author" << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        std::string author_id;
        
        if (!name.empty()) {
            // Редактирование по имени
            auto authors = GetAuthors();
            auto it = std::find_if(authors.begin(), authors.end(),
                [&](const detail::AuthorInfo& a) { return a.name == name; });
            
            if (it == authors.end()) {
                output_ << "Failed to edit author" << std::endl;
                return true;
            }
            author_id = it->id;
        } else {
            // Выбор из списка
            auto opt_id = SelectAuthor();
            if (!opt_id) {
                return true;
            }
            author_id = *opt_id;
        }
        
        output_ << "Enter new name:" << std::endl;
        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);
        
        if (new_name.empty()) {
            output_ << "Failed to edit author" << std::endl;
            return true;
        }
        
        use_cases_.EditAuthor(domain::AuthorId::FromString(author_id), new_name);
        
    } catch (...) {
        output_ << "Failed to edit author" << std::endl;
    }
    return true;
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

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
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

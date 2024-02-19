#include "2DOCore/user.hpp"

#include <optional>
#include <regex>
#include <stdexcept>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>

#include <Utils/result.hpp>
#include <Utils/type.hpp>


namespace SQL = SQLite;

namespace twodocore {
[[nodiscard]] String User::rtos(Role role) const {
    switch (role) {
        case Role::Admin:
            return "Admin";
            break;
        case Role::User:
            return "User";
            break;
        default:
            throw std::logic_error("Invalid role enum!");
            break;
    }
}

[[nodiscard]] Role User::stor(const String& role_str) const {
    static const std::map<std::string, Role> role_map = {
        {"User", Role::User}, {"Admin", Role::Admin}};

    const auto it = role_map.find(role_str);
    if (it != role_map.end()) {
        return it->second;
    } else {
        throw std::logic_error("Invalid role string!");
    }
}

UserDb::UserDb(StringView db_filepath)
    : m_db{db_filepath, SQL::OPEN_READWRITE | SQL::OPEN_CREATE} {
    if (!is_table_empty()) {
        SQL::Statement query{
            m_db,
            "CREATE TABLE IF NOT EXISTS users ("
            "user_id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
            "username VARCHAR(20) NOT NULL, "
            "role BOOLEAN NOT NULL, "
            "password VARCHAR(20) NOT NULL)"};

        query.exec();
        if (!query.isDone())
            throw std::runtime_error("Failure creating user table.");
    }
}

User UserDb::get_object(unsigned int id) const {
    SQL::Statement query{m_db, "SELECT * FROM users WHERE user_id = ?"};
    query.bind(1, id);

    query.executeStep();

    const auto user = User{
        (unsigned)query.getColumn(0).getInt(), query.getColumn(1).getString(),
        query.getColumn(2).getString(), query.getColumn(3).getString()};

    // nrvo?
    return user;
}

Vector<User> UserDb::get_all_objects() const {
    SQL::Statement query{m_db, "SELECT * FROM users"};

    Vector<User> users;
    while (query.executeStep()) {
        users.push_back(User{(unsigned)query.getColumn(0).getInt(),
                             query.getColumn(1).getString(),
                             query.getColumn(2).getString(),
                             query.getColumn(3).getString()});
    }

    return users;
}

bool UserDb::is_table_empty() const {
    return m_db.tableExists("users");
}

void UserDb::add_object(User& user) {
    SQL::Statement query{
        m_db, "INSERT INTO users (username, role, password) VALUES (?, ?, ?)"};
    query.bind(1, user.username());
    query.bind(2, user.role<String>());
    query.bind(3, user.password());

    query.exec();

    query = SQL::Statement{
        m_db, "SELECT user_id FROM users ORDER BY user_id DESC LIMIT 1"};

    query.executeStep();

    user.set_id(std::stoi(query.getColumn(0)));
}

void UserDb::update_object(const User& user) const {
    SQL::Statement query{m_db,
                         "UPDATE users SET username = ?, role = ?, "
                         "password = ? WHERE user_id = ?"};
    query.bind(1, user.username());
    query.bind(2, user.role<String>());
    query.bind(3, user.password());
    query.bind(4, std::to_string(user.id()));

    query.exec();
}

void UserDb::delete_object(unsigned int id) const {
    SQL::Statement query{m_db, "DELETE FROM users WHERE user_id = ?"};
    query.bind(1, std::to_string(id));

    query.exec();
}

std::optional<User> UserDb::find_object_by_unique_column(
    const String& column_value) const {
    SQL::Statement query{m_db, "SELECT * FROM users WHERE username = ?"};
    query.bind(1, column_value);

    try {
        if(!query.executeStep()) {
            return std::nullopt;
        }
    } catch (const SQL::Exception& e) {
        if (query.hasRow()) {
            throw e;
        }
    }

    const auto user = User{
        (unsigned)query.getColumn(0).getInt(), query.getColumn(1).getString(),
        query.getColumn(2).getString(), query.getColumn(3).getString()};

    return user;
};

tdu::Result<void, AuthErr> AuthenticationManager::username_validation(
    const String& username) {
    if (username.length() <= 0) {
        return tdu::Err(AuthErr::InvalidNameLength);
    }

    if (is_in_db(username)) {
        return tdu::Err(AuthErr::AlreadyExistingName);
    }

    return tdu::Ok();
};

tdu::Result<void, AuthErr> AuthenticationManager::password_validation(
    const String& password) {
    const std::regex upper_case_expression{"[A-Z]+"};
    const std::regex lower_case_expression{"[a-z]+"};
    const std::regex number_expression{"[0-9]+"};
    const std::regex special_char_expression{
        "[!@#$%^&*()_+\\-=\\[\\]{};:\\\",<.>/?]+"};

    if (password.length() < 8 && password.length() > 20) {
        return tdu::Err(AuthErr::InvalidPassLength);
    }
    if (!std::regex_search(password, upper_case_expression)) {
        return tdu::Err(AuthErr::MissingUpperCase);
    }
    if (!std::regex_search(password, lower_case_expression)) {
        return tdu::Err(AuthErr::MissingLowerCase);
    }
    if (!std::regex_search(password, number_expression)) {
        return tdu::Err(AuthErr::MissingNumber);
    }
    if (!std::regex_search(password, special_char_expression)) {
        return tdu::Err(AuthErr::MissingSpecialCharacter);
    }

    return tdu::Ok();
};

bool AuthenticationManager::is_in_db(const String& username) {
    return (m_user_db->find_object_by_unique_column(username)) ? true : false;
};
}  // namespace twodocore
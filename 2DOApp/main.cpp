#include <conio.h>
#include <iostream>
#include <memory>

#include <fmt/color.h>
#include <fmt/core.h>

#include <2DOApp/app.hpp>
#include <Utils/type.hpp>
#include <Utils/util.hpp>
#include <string>

namespace td = twodo;
namespace tdu = twodoutils;

class UserInput : public tdu::IUserInputHandler {
  public:
    String get_input() const override {
        auto input = String();

        std::cin.clear();
        std::getline(std::cin, input);

        return input;
    }

    String get_secret() const override {
        String secret;

        char ch;

#ifdef _WIN32
        while ((ch = _getch()) != '\r') {
#else
        while ((ch = _getch()) != '\n') {

#endif
            if (ch == '\b') {
                if (!secret.empty()) {
                    fmt::print("\b \b");

                    secret.pop_back();
                }
            } else {
                std::cout << '*';
                secret += ch;
            }
        }

        return secret;
    }
};

class MsgDisplayer : public tdu::IPrinter {
  public:
    void msg_print(StringView msg) const override { fmt::print("{}", msg); }

    void err_print(StringView msg) const override {
        fmt::print("{}", fmt::format(fmt::fg(fmt::color::red), msg));
    }

    void menu_print(StringView page_name,
                    const HashMap<String, String>& menu_pages) const override {
        if (!page_name.empty()) {
            fmt::println("{}", fmt::format(fg(fmt::color::beige), page_name));
        }

        for (const auto& page : menu_pages) {
            fmt::println("[{}] {}",
                         fmt::format(fg(fmt::color::alice_blue), page.first),
                         fmt::format(fg(fmt::color::alice_blue), page.second));
        }
        fmt::println("[{}] {}", fmt::format(fg(fmt::color::alice_blue), "0"),
                     fmt::format(fg(fmt::color::alice_blue), "Back"));
        fmt::print("-> ");
    }
};

int main() {
    try {
        td::App::getInstance()
            ->set_dependencies(std::make_shared<MsgDisplayer>(),
                               std::make_shared<UserInput>())
            ->run();
    } catch (const std::runtime_error& e) {
        tdu::log_to_file(e.what(), ERR_LOGS_FILE_NAME);
        fmt::print(stderr, "{}", std::move(e.what()));
    }
}
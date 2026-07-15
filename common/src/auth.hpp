//
// Created by Leonard on 2026-04-23.
//

#pragma once
#include <optional>
#include <string>

namespace chatapp::auth {

constexpr int USERNAME_MIN_LENGTH = 3;
constexpr int USERNAME_MAX_LENGTH = 32;
constexpr int PASSWORD_MIN_LENGTH = 3;
constexpr int PASSWORD_MAX_LENGTH = 32;

class Auth {
public:
    static std::optional<Auth> make(std::string username, std::string password);

    static std::optional<Auth> decode(std::string_view const &encoded);
    [[nodiscard]] std::string encode() const;

    std::string const &get_username() const;
    std::string const &get_password() const;

private:
    Auth(std::string username, std::string password);

    std::string username;
    std::string password;
};

} // namespace chatapp::auth

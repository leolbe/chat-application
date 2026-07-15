//
// Created by Leonard on 2026-04-23.
//

#include "auth.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <iostream>
#include <utility>

namespace iterators = boost::archive::iterators;
using Auth = chatapp::auth::Auth;

bool is_valid_credential_string(std::string &str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        bool is_digit = c >= '0' && c <= '9';
        bool is_upper = c >= 'A' && c <= 'Z';
        bool is_lower = c >= 'a' && c <= 'z';
        bool is_underscore = c == '_';
        return is_digit || is_upper || is_lower || is_underscore;
    });
}

std::optional<Auth> Auth::make(std::string username, std::string password) {
    if (username.size() < USERNAME_MIN_LENGTH ||
        username.size() > USERNAME_MAX_LENGTH ||
        password.size() < PASSWORD_MIN_LENGTH ||
        password.size() > PASSWORD_MAX_LENGTH) {
        return {};
    }
    if (!is_valid_credential_string(username) ||
        !is_valid_credential_string(password)) {
        return {};
    }
    return Auth(username, password);
}

std::optional<Auth> Auth::decode(std::string_view const &encoded) {
    using decoder = iterators::
        transform_width<iterators::binary_from_base64<const char *>, 8, 6>;

    auto stripped_size = encoded.size();
    while (stripped_size > 0 && encoded[stripped_size - 1] == '=') {
        stripped_size--;
    }

    std::string decoded;
    try {
        decoded = std::string(
            decoder(encoded.data()), decoder(encoded.data() + stripped_size));
    } catch (iterators::dataflow_exception const &e) {
        std::cerr << "error when decoding Auth: " << e.what() << "\n";
        return {};
    }

    auto sep_index = decoded.find(':');
    if (sep_index != std::string::npos) {
        return make(
            decoded.substr(0, sep_index), decoded.substr(sep_index + 1));
    } else {
        return {};
    }
}

std::string Auth::encode() const {
    using encoder = iterators::base64_from_binary<
        iterators::transform_width<const char *, 6, 8>>;

    auto decoded = username + ':' + password;

    // base64 encode
    std::string encoded = std::string(
        encoder(decoded.data()), encoder(decoded.data() + decoded.size()));
    encoded.append((3 - decoded.size() % 3) % 3, '=');

    return encoded;
}

std::string const &Auth::get_username() const {
    return username;
}

std::string const &Auth::get_password() const {
    return password;
}

Auth::Auth(std::string username, std::string password)
    : username(std::move(username)), password(std::move(password)) {
}

//
// Created by Leonard on 2026-04-14.
//

#include "client.hpp"

#include "auth.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>
#include <print>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;

namespace auth = chatapp::auth;
using chatapp::client::Client;

void fail(beast::error_code ec, char const *message) {
    std::cerr << message << ": " << ec.message() << "\n";
}

Client::Client(asio::io_context &ioc) : ioc(ioc) {
}

void Client::run_detached(std::string host, std::string port) {
    asio::co_spawn(ioc, session(host, port), asio::detached);
}

asio::awaitable<void> Client::session(std::string host, std::string port) {
    beast::flat_buffer buf;
    beast::error_code ec;

    asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
    auto endpoints = co_await resolver.async_resolve(
        host, port, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when resolving host");
        co_return;
    }

    if (endpoints.empty()) {
        fail(ec, "failed to resolve host");
        co_return;
    }

    asio::ip::tcp::socket socket(co_await asio::this_coro::executor);
    co_await socket.async_connect(
        *endpoints.begin(), asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when connecting to endpoint");
        co_return;
    }

    beast::tcp_stream stream(std::move(socket));

    bool logged_in = co_await login(buf, stream, host);
    if (logged_in) {
        // todo: switch to websocket, start instant messaging
    }

    stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);

    ioc.stop();
}

asio::awaitable<bool> Client::fetch_messages(
    beast::flat_buffer &buf,
    beast::tcp_stream &stream,
    std::string const &host,
    auth::Auth const &auth) {
    beast::error_code ec;

    http::request<http::string_body> req{http::verb::get, "/messages", 11};
    req.set(http::field::host, host);
    req.set(http::field::authorization, auth.encode());
    req.set(http::field::connection, "close");

    co_await http::async_write(
        stream, req, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP request");
        co_return false;
    }

    http::response<http::string_body> res;
    co_await http::async_read(
        stream, buf, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when reading HTTP response");
        co_return false;
    }

    switch (res.base().result()) {
    case http::status::ok:
        std::println("Messages:\n{}", res.body());
        break;
    case http::status::unauthorized:
        std::println("Failed to authorize!");
        break;
    default:
        std::println("Network error {}: {}", res.base().result_int(), res.body());
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> Client::login_prompt(
    beast::flat_buffer &buf, beast::tcp_stream &stream, std::string &host) {
    beast::error_code ec;

    std::string username;
    std::string password;
    std::print("Username: ");
    std::getline(std::cin, username);
    std::print("Password: ");
    std::getline(std::cin, password);

    auto auth = auth::Auth::make(username, password);
    if (!auth.has_value()) {
        std::println("Invalid username or password!");
        co_return true;
    }
    this->auth = auth;
    co_return true;
}

asio::awaitable<bool> Client::change_login_prompt(
    beast::flat_buffer &buf, beast::tcp_stream &stream, std::string &host) {
    beast::error_code ec;

    std::string username;
    std::string password;
    std::print("Current username: ");
    std::getline(std::cin, username);
    std::print("Current password: ");
    std::getline(std::cin, password);

    auto auth = auth::Auth::make(username, password);
    if (!auth.has_value()) {
        std::println("Invalid username or password!");
        co_return true;
    }

    std::string new_username;
    std::string new_password;
    std::print("New username: ");
    std::getline(std::cin, new_username);
    std::print("New password: ");
    std::getline(std::cin, new_password);

    auto new_auth = auth::Auth::make(new_username, new_password);
    if (!new_auth.has_value()) {
        std::println("Invalid username or password!");
        co_return true;
    }

    http::request<http::string_body> req{http::verb::post, "/change-login", 11};
    req.set(http::field::host, host);
    req.set(http::field::authorization, auth->encode());
    req.body() = new_auth->encode();
    req.prepare_payload();

    co_await http::async_write(
        stream, req, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP request");
        co_return false;
    }

    http::response<http::dynamic_body> res;
    co_await http::async_read(
        stream, buf, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when reading HTTP response");
        co_return false;
    }

    switch (res.base().result()) {
    case http::status::no_content:
        std::println("Successfully updated username/password!");
        this->auth = new_auth;
        break;
    case http::status::unauthorized:
        std::println("Failed to authorize!");
        break;
    case http::status::conflict:
        std::println("Username is already taken!");
        break;
    default:
        std::string body = boost::beast::buffers_to_string(res.body().data());
        std::println("Network error {}: {}", res.base().result_int(), body);
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> Client::signup_prompt(
    beast::flat_buffer &buf, beast::tcp_stream &stream, std::string &host) {
    beast::error_code ec;

    std::string username;
    std::string password;
    std::print("Username: ");
    std::getline(std::cin, username);
    std::print("Password: ");
    std::getline(std::cin, password);

    auto auth = auth::Auth::make(username, password);
    if (!auth.has_value()) {
        std::println("Invalid username or password!");
        co_return true;
    }

    http::request<http::string_body> req{http::verb::post, "/signup", 11};
    req.set(http::field::host, host);
    req.set(http::field::authorization, auth->encode());

    co_await http::async_write(
        stream, req, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP request");
        co_return false;
    }

    http::response<http::dynamic_body> res;
    co_await http::async_read(
        stream, buf, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when reading HTTP response");
        co_return false;
    }

    switch (res.base().result()) {
    case http::status::created:
        std::println("Successfully signed up!");
        this->auth = auth;
        break;
    case http::status::conflict:
        std::println("Username is already taken!");
        break;
    default:
        std::string body = boost::beast::buffers_to_string(res.body().data());
        std::println("Network error {}: {}", res.base().result_int(), body);
        co_return false;
    }
    co_return true;
}

asio::awaitable<bool> Client::login(
    beast::flat_buffer &buf, beast::tcp_stream &stream, std::string &host) {
    std::println("Welcome to chatapp!");
    while (std::cin) {
        std::println("l: login, lc: change username/password and login\ns: signup, q: quit");

        std::string inp;
        std::getline(std::cin, inp);

        if (inp == "l") {
            if (!co_await login_prompt(buf, stream, host)) {
                break;
            }
        } else if (inp == "lc") {
            if (!co_await change_login_prompt(buf, stream, host)) {
                break;
            }
        } else if (inp == "s") {
            if (!co_await signup_prompt(buf, stream, host)) {
                break;
            }
        } else if (inp == "q") {
            break;
        }

        if (this->auth.has_value()) {
            co_return co_await fetch_messages(
                buf, stream, host, this->auth.value());
        }
    }
    co_return false;
}


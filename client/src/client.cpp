//
// Created by Leonard on 2026-04-14.
//

#include "client.hpp"

#include "auth.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
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
using chatapp::client::Client;

void fail(beast::error_code ec, char const *message) {
    std::cerr << message << ": " << ec.message() << "\n";
}

Client::Client(asio::io_context &ioc) : ioc(ioc), state{ClientState::PreLogin} {
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

    http::request<http::string_body> request{http::verb::get, "/signup", 11};
    request.set(http::field::host, host);

    auto auth = auth::Auth::make("username", "password");
    request.set(http::field::authorization, auth->encode());

    request.set(http::field::connection, "close");

    co_await http::async_write(
        stream, request, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP request");
        co_return;
    }

    http::response<http::dynamic_body> response;
    co_await http::async_read(
        stream, buf, response, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when reading HTTP response");
        co_return;
    }

    std::cout << response << "\n";

    stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
}


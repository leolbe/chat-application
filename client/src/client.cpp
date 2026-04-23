//
// Created by Leonard on 2026-04-14.
//

#include "client.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <iostream>
#include <print>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
using chatapp::client::Client;

Client::Client(asio::io_context &ioc) : ioc(ioc), state{ClientState::PreLogin} {
}

void Client::run_detached(std::string host, std::string port) {
    asio::co_spawn(ioc, session(host, port), asio::detached);
}

asio::awaitable<void> Client::session(std::string host, std::string port) {
    beast::flat_buffer buf;

    try {
        asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
        auto endpoints =
            co_await resolver.async_resolve(host, port, asio::use_awaitable);

        asio::ip::tcp::socket socket(co_await asio::this_coro::executor);
        co_await socket.async_connect(*endpoints.begin(), asio::use_awaitable);

        beast::tcp_stream stream(std::move(socket));

        http::request<http::string_body> request{http::verb::get, "/", 11};
        request.set(http::field::host, host);
        request.set(http::field::connection, "close");

        co_await http::async_write(stream, request, asio::use_awaitable);

        http::response<http::dynamic_body> response;
        co_await http::async_read(stream, buf, response, asio::use_awaitable);

        std::cout << response << "\n";

        stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
    } catch (std::exception &e) {
        std::println("client exception: {}", e.what());
    }
}


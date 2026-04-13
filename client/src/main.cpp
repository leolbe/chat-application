//
// Created by Leonard on 2026-03-25.
//

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;

using asio::use_awaitable;

asio::awaitable<void> session(std::string host, std::string port) {
    beast::flat_buffer buf;

    try {
        asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
        auto endpoints =
            co_await resolver.async_resolve(host, port, use_awaitable);

        asio::ip::tcp::socket socket(co_await asio::this_coro::executor);
        co_await socket.async_connect(*endpoints.begin(), use_awaitable);

        beast::tcp_stream stream(std::move(socket));

        http::request<http::string_body> request{http::verb::get, "/", 11};
        request.set(http::field::host, host);
        request.set(http::field::connection, "close");

        co_await http::async_write(stream, request, use_awaitable);

        http::response<http::dynamic_body> response;
        co_await http::async_read(stream, buf, response, use_awaitable);

        std::cout << response << std::endl;

        stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
    } catch (std::exception &e) {
        std::printf("client exception: %s\n", e.what());
    }
}

int main(int argc, char *argv[]) {
    std::string host("127.0.0.1");
    std::string port("3033");

    asio::io_context ioc(1);

    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) {
        ioc.stop();
    });

    asio::co_spawn(ioc, session(host, port), asio::detached);

    ioc.run();
}

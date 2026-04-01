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

asio::awaitable<void> session(std::string host, std::string port) {
    try {
        asio::ip::tcp::resolver resolver(co_await asio::this_coro::executor);
        auto endpoints =
            co_await resolver.async_resolve(host, port, asio::use_awaitable);

        asio::ip::tcp::socket socket(co_await asio::this_coro::executor);
        co_await socket.async_connect(*endpoints.begin(), asio::use_awaitable);

        while (true) {
            std::string request;
            std::getline(std::cin, request);
            if (request.length() == 0) {
                break;
            }

            co_await socket.async_write_some(
                asio::buffer(request), asio::use_awaitable);

            char response[1024];
            size_t len = co_await socket.async_read_some(
                asio::buffer(response), asio::use_awaitable);

            std::printf("%s\n", response);
        }
        socket.close();
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

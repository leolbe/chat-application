//
// Created by Leonard on 2026-03-25.
//

#include <boost/asio.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/config.hpp>
#include <print>

namespace asio = boost::asio;

asio::awaitable<void> handler(asio::ip::tcp::socket socket) {
    try {
        char data[1024];
        while (true) {
            std::size_t len = co_await socket.async_read_some(
                asio::buffer(data), asio::use_awaitable);

            std::printf("%s\n", data);

            co_await async_write(
                socket, asio::buffer(data, len), asio::use_awaitable);
        }
    } catch (std::exception &e) {
        std::printf("server exception: %s\n", e.what());
    }
}

asio::awaitable<void> listener(asio::ip::address address, unsigned short port) {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, {address, port});
    while (true) {
        asio::ip::tcp::socket socket =
            co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(executor, handler(std::move(socket)), asio::detached);
    }
}

int main() {
    auto address = asio::ip::make_address("127.0.0.1");
    unsigned short port(3033);

    asio::io_context ioc(1);

    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) {
        ioc.stop();
    });

    asio::co_spawn(ioc, listener(address, port), asio::detached);

    ioc.run();

    return 0;
}

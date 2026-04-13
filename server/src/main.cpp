//
// Created by Leonard on 2026-03-25.
//

#include <boost/asio.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/config.hpp>
#include <print>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;

using asio::use_awaitable;

asio::awaitable<void> handler(beast::tcp_stream stream) {
    beast::flat_buffer buf;

    try {
        while (true) {
            http::request<http::string_body> request;
            co_await http::async_read(stream, buf, request, use_awaitable);

            std::printf(
                "method: %s, target: %s, http version: %d\n",
                request.base().method_string().data(),
                request.base().target().data(),
                request.base().version());

            http::response<http::string_body> response{
                http::status::ok, request.version()};
            response.set(http::field::server, "test-server/1.0");
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = "Hello!!";
            response.prepare_payload();

            co_await http::async_write(stream, response, use_awaitable);

            if (!request.keep_alive()) {
                break;
            }
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
            co_await acceptor.async_accept(use_awaitable);
        asio::co_spawn(
            executor,
            handler(beast::tcp_stream(std::move(socket))),
            asio::detached);
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

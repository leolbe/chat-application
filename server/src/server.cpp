//
// Created by Leonard on 2026-04-15.
//

#include "server.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace ip = boost::asio::ip;
using chatapp::server::Server;

Server::Server(asio::io_context &ioc)
    : ioc(ioc), strand{asio::make_strand(ioc)} {
}

void Server::run_detached(ip::basic_endpoint<ip::tcp> &&endpoint) {
    asio::co_spawn(ioc, listen(std::move(endpoint)), asio::detached);
}

asio::awaitable<void> Server::listen(ip::basic_endpoint<ip::tcp> endpoint) {
    auto executor = co_await asio::this_coro::executor;
    ip::tcp::acceptor acceptor(executor, endpoint);
    while (true) {
        ip::tcp::socket socket =
            co_await acceptor.async_accept(asio::use_awaitable);
        asio::co_spawn(
            executor,
            handle(beast::tcp_stream(std::move(socket))),
            asio::detached);
    }
}

asio::awaitable<void> Server::handle(beast::tcp_stream stream) {
    beast::flat_buffer buf;

    try {
        while (true) {
            http::request<http::string_body> req;
            co_await http::async_read(stream, buf, req, asio::use_awaitable);

            std::printf(
                "method: %s, target: %s, http version: %d\n",
                req.base().method_string().data(),
                req.base().target().data(),
                req.base().version());

            http::response<http::string_body> response{
                http::status::ok, req.version()};
            response.set(http::field::server, "test-server/1.0");
            response.set(http::field::content_type, "text/html");
            response.keep_alive(req.keep_alive());
            response.body() = "Hello!!";
            response.prepare_payload();

            co_await http::async_write(stream, response, asio::use_awaitable);

            if (!req.keep_alive()) {
                break;
            }
        }
    } catch (std::exception &e) {
        std::printf("server exception: %s\n", e.what());
    }
}

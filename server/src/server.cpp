//
// Created by Leonard on 2026-04-15.
//

#include "server.hpp"
#include "auth.hpp"
#include "protocol.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <iostream>
#include <print>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

namespace auth = chatapp::auth;
namespace protocol = chatapp::protocol;
using chatapp::server::Server;

void fail(beast::error_code ec, char const *message) {
    std::cerr << message << ": " << ec.message() << "\n";
}

asio::awaitable<void> send_response(
    beast::tcp_stream &stream, http::status status, std::string body) {
    beast::error_code ec;

    http::response<http::string_body> res(status, 11);

    res.set(http::field::server, protocol::SERVER_NAME);
    res.keep_alive(false);
    res.prepare_payload();
    if (!body.empty()) {
        res.set(http::field::content_type, "text/plain");
        res.body() = body;
    }

    co_await http::async_write(
        stream, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP response");
    }
}

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
    beast::error_code ec;

    stream.expires_after(CONNECTION_TIMEOUT);

    http::request_parser<http::string_body> parser;
    parser.body_limit(REQUEST_BODY_LIMIT);

    co_await http::async_read(
        stream, buf, parser, asio::redirect_error(asio::use_awaitable, ec));
    if (ec == http::error::end_of_stream) {
        stream.socket().shutdown(asio::socket_base::shutdown_send, ec);
        if (ec) {
            fail(ec, "error when shutting down socket");
        }
        co_return;
    } else if (ec) {
        fail(ec, "error when handling HTTP request");
        co_await send_response(
            stream, http::status::bad_request, "Bad request");
        co_return;
    }

    if (beast::websocket::is_upgrade(parser.get())) {
        std::make_shared<websocket::WebSocketSession>(
            std::move(stream.release_socket()))
            ->run_detached(std::move(parser.release()));
        co_return;
    }

    auto req = parser.release();

    if (req.method() == http::verb::get && req.target() == "/signup") {
        auto auth_it = req.base().find(http::field::authorization);
        if (auth_it == req.base().end()) {
            co_await send_response(
                stream,
                http::status::bad_request,
                "Missing 'Authorization' field");
            co_return;
        }
        auto auth_encoded = auth_it->value();

        auto auth = auth::Auth::decode(auth_encoded);
        if (!auth.has_value()) {
            co_await send_response(
                stream,
                http::status::unprocessable_entity,
                "Invalid username or password");
            co_return;
        }
        bool succeeded =
            co_await create_user(auth->get_username(), auth->get_password());

        if (succeeded) {
            co_await send_response(stream, http::status::created, "");
        } else {
            co_await send_response(
                stream, http::status::conflict, "Username is already taken");
        }
        co_return;
    }

    co_await send_response(
        stream, http::status::bad_request, "Unknown method or endpoint");
    co_return;
}

asio::awaitable<bool>
Server::create_user(std::string username, std::string password) {
    auto user_id = uuid_generator();
    co_await asio::dispatch(strand, asio::use_awaitable);
    if (users_lookup.contains(username)) {
        co_return false;
    }
    users.emplace(user_id, User{username, password});
    users_lookup.emplace(username, user_id);
    co_return true;
}

asio::awaitable<bool>
Server::authenticate(std::string const &username, std::string const &password) {
    co_await asio::dispatch(strand, asio::use_awaitable);
    try {
        UserId &user_id = users_lookup.at(username);
        User &user = users.at(user_id);
        co_return user.password == password;
    } catch (const std::exception &e) {
        co_return false;
    }
}

asio::awaitable<void> Server::join(websocket::WebSocketSession *session) {
    co_await asio::dispatch(strand, asio::use_awaitable);
    sessions.emplace(session);
}

asio::awaitable<void> Server::leave(websocket::WebSocketSession *session) {
    co_await asio::dispatch(strand, asio::use_awaitable);
    sessions.erase(session);
}

asio::awaitable<void> Server::send_message(UserId sender, std::string message) {
    auto ex = co_await asio::this_coro::executor;
    co_await asio::dispatch(strand, asio::use_awaitable);

    auto sender_username = users.at(sender).username;
    messages.emplace_back(sender, message);

    std::vector<std::weak_ptr<websocket::WebSocketSession>> recipients;
    recipients.reserve(sessions.size());
    for (auto session : sessions) {
        recipients.emplace_back(session->weak_from_this());
    }

    co_await asio::dispatch(ex, asio::use_awaitable);

    for (auto const &ptr : recipients) {
        if (auto wp = ptr.lock()) {
            if (wp->get_user() != sender) {
                wp->send(sender_username, message);
            }
        }
    }
}

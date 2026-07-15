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
namespace server = chatapp::server;
using chatapp::server::Server;

void fail(beast::error_code ec, char const *message) {
    std::cerr << message << ": " << ec.message() << "\n";
}

void fail(std::exception e, char const *message) {
    std::cerr << message << ": " << e.what() << "\n";
}

asio::awaitable<void> send_response(
    beast::tcp_stream &stream,
    http::status status,
    std::string const &body,
    bool keep_alive) {
    beast::error_code ec;

    http::response<http::string_body> res(status, 11);

    res.set(http::field::server, protocol::SERVER_NAME);
    res.keep_alive(keep_alive);
    res.prepare_payload();
    if (!body.empty()) {
        res.set(http::field::content_type, "text/plain");
        res.body() = body;
    }
    res.prepare_payload();

    co_await http::async_write(
        stream, res, asio::redirect_error(asio::use_awaitable, ec));
    if (ec) {
        fail(ec, "error when sending HTTP response");
    }
}

asio::awaitable<void> send_head_response(
    beast::tcp_stream &stream,
    http::status status,
    std::uint64_t content_length) {
    beast::error_code ec;

    http::response<http::string_body> res(status, 11);

    res.set(http::field::server, protocol::SERVER_NAME);
    res.keep_alive(false);
    res.prepare_payload();
    res.set(http::field::content_type, "text/plain");
    res.set(http::field::content_length, std::to_string(content_length));

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

    while (true) {
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
                stream, http::status::bad_request, "Bad request", false);
            co_return;
        }

        if (beast::websocket::is_upgrade(parser.get())) {
            std::make_shared<websocket::WebSocketSession>(
                std::move(stream.release_socket()))
                ->run_detached(std::move(parser.release()));
            co_return;
        }

        auto req = parser.release();

        if (req.target() == "/signup") {
            co_await handle_signup(stream, req);
        } else if (req.target() == "/messages") {
            co_await handle_messages(stream, req);
        } else {
            co_await send_response(
                stream,
                http::status::bad_request,
                "Unknown method or endpoint",
                false);
            co_return;
        }
    }
}

asio::awaitable<std::optional<auth::Auth>> parse_authentication(
    beast::tcp_stream &stream, http::request<http::string_body> &req) {
    auto auth_it = req.base().find(http::field::authorization);
    if (auth_it == req.base().end()) {
        co_await send_response(
            stream,
            http::status::bad_request,
            "Missing 'Authorization' field",
            false);
        co_return std::optional<auth::Auth>{};
    }
    auto auth_encoded = auth_it->value();

    auto auth = auth::Auth::decode(auth_encoded);
    if (!auth.has_value()) {
        co_await send_response(
            stream,
            http::status::unprocessable_entity,
            "Invalid username or password",
            false);
    }
    co_return auth;
}

asio::awaitable<std::optional<server::UserId>> Server::authenticate(
    beast::tcp_stream &stream, http::request<http::string_body> &req) {
    std::optional<auth::Auth> auth = co_await parse_authentication(stream, req);
    if (!auth.has_value()) {
        co_return std::optional<UserId>{};
    }

    co_await asio::dispatch(strand, asio::use_awaitable);
    try {
        UserId &user_id = users_lookup.at(auth->get_username());
        User &user = users.at(user_id);
        if (user.password == auth->get_password()) {
            co_return user_id;
        }
    } catch (const std::exception &e) {
        fail(e, "exception when authenticating user");
    }
    co_await send_response(
        stream, http::status::unauthorized, "Failed to authenticate", false);
    co_return std::optional<UserId>{};
}

asio::awaitable<void> Server::handle_signup(
    beast::tcp_stream &stream, http::request<http::string_body> &req) {
    if (req.method() != http::verb::post) {
        co_await send_response(
            stream,
            http::status::bad_request,
            "Unknown method for /signup",
            false);
        co_return;
    }

    auto auth = co_await parse_authentication(stream, req);
    if (!auth.has_value()) {
        co_return;
    }
    auto user_id = co_await create_user(auth.value());

    if (user_id.has_value()) {
        co_await send_response(stream, http::status::created, "", true);
        co_await send_message(user_id.value(), "CREATED");
    } else {
        co_await send_response(
            stream, http::status::conflict, "Username is already taken", false);
    }
}

asio::awaitable<void> Server::handle_messages(
    beast::tcp_stream &stream, http::request<http::string_body> &req) {
    auto user_id = co_await authenticate(stream, req);
    if (!user_id.has_value()) {
        co_return;
    }

    if (req.method() == http::verb::head) {
        std::uint64_t content_length = 0;
        co_await asio::dispatch(strand, asio::use_awaitable);
        for (auto const &message : messages) {
            std::string sender;
            try {
                sender = users.at(message.sender).username;
            } catch (std::out_of_range &) {
                sender = "[Unknown user]";
            }
            content_length +=
                1 + sender.size() + 2 + message.content.size() + 1;
        }
        co_await send_head_response(stream, http::status::ok, content_length);
    } else if (req.method() == http::verb::get) {
        std::string content;
        co_await asio::dispatch(strand, asio::use_awaitable);
        std::println("Fetching {} message(s)", messages.size());
        for (auto const &message : messages) {
            std::string sender;
            try {
                sender = users.at(message.sender).username;
            } catch (std::out_of_range &) {
                sender = "[Unknown user]";
            }
            content += "<" + sender + "> " + message.content + "\n";
        }

        co_await send_response(stream, http::status::ok, content, true);
    } else {
        co_await send_response(
            stream,
            http::status::bad_request,
            "Unknown method for /messages",
            false);
    }
}

asio::awaitable<std::optional<server::UserId>>
Server::create_user(auth::Auth const &auth) {
    auto user_id = uuid_generator();
    co_await asio::dispatch(strand, asio::use_awaitable);
    if (users_lookup.contains(auth.get_username())) {
        co_return std::optional<UserId>{};
    }
    users.emplace(user_id, User{auth.get_username(), auth.get_password()});
    users_lookup.emplace(auth.get_username(), user_id);

    // log
    std::println("Create user '{}'", auth.get_username());

    co_return user_id;
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

    // log
    std::println("<{}> {}", sender_username, message);
}

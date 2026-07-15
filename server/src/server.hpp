//
// Created by Leonard on 2026-04-15.
//

#pragma once
#include "auth.hpp"
#include "user.hpp"
#include "websocket.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace chatapp::server {

constexpr std::uint64_t REQUEST_BODY_LIMIT = 10000;
constexpr std::chrono::duration<std::int64_t> CONNECTION_TIMEOUT =
    std::chrono::seconds(30);

class Server : std::enable_shared_from_this<Server> {
public:
    explicit Server(boost::asio::io_context &ioc);

    void run_detached(
        boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> &&endpoint);

private:
    boost::uuids::random_generator uuid_generator;

    std::vector<Message> messages;
    std::unordered_map<UserId, User> users;
    std::unordered_map<std::string, UserId> users_lookup;
    std::unordered_set<websocket::WebSocketSession *> sessions;

    boost::asio::io_context &ioc;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;

    boost::asio::awaitable<void>
    listen(boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> endpoint);

    boost::asio::awaitable<void> handle(boost::beast::tcp_stream stream);

    boost::asio::awaitable<std::optional<UserId>> authenticate(
        boost::beast::tcp_stream &stream,
        boost::beast::http::request<boost::beast::http::string_body> &req);

    boost::asio::awaitable<void> handle_change_login(
        boost::beast::tcp_stream &stream,
        boost::beast::http::request<boost::beast::http::string_body> &req);

    boost::asio::awaitable<void> handle_signup(
        boost::beast::tcp_stream &stream,
        boost::beast::http::request<boost::beast::http::string_body> &req);

    boost::asio::awaitable<void> handle_messages(
        boost::beast::tcp_stream &stream,
        boost::beast::http::request<boost::beast::http::string_body> &req);

    boost::asio::awaitable<std::optional<UserId>>
    create_user(auth::Auth const &auth);

    boost::asio::awaitable<bool>
    update_user(UserId const &auth, auth::Auth const &new_auth);

    boost::asio::awaitable<void> join(websocket::WebSocketSession *session);

    boost::asio::awaitable<void> leave(websocket::WebSocketSession *session);

    boost::asio::awaitable<void>
    send_message(UserId sender, std::string message);
};

} // namespace chatapp::server

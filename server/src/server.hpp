//
// Created by Leonard on 2026-04-15.
//

#pragma once
#include "user.hpp"
#include "websocket.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/uuid/uuid.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace chatapp::server {

class Server : std::enable_shared_from_this<Server> {
public:
    Server(boost::asio::io_context &ioc);

    void run_detached(
        boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> &&endpoint);

private:
    std::vector<Message> messages;
    std::unordered_map<UserId, User> users;
    std::unordered_map<std::string, UserId> users_lookup;
    std::unordered_set<websocket::WebSocketSession *> sessions;

    boost::asio::io_context &ioc;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;

    boost::asio::awaitable<void>
    listen(boost::asio::ip::basic_endpoint<boost::asio::ip::tcp> endpoint);

    boost::asio::awaitable<void> handle(boost::beast::tcp_stream stream);

    boost::asio::awaitable<bool>
    authenticate(std::string const &username, std::string const &password);

    boost::asio::awaitable<void>
    join(websocket::WebSocketSession *session);

    boost::asio::awaitable<void>
    leave(websocket::WebSocketSession *session);

    boost::asio::awaitable<void>
    send_message(UserId sender, std::string message);
};

} // namespace chatapp::server

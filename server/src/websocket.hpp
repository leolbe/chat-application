//
// Created by Leonard on 2026-04-15.
//

#pragma once

#include "user.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <memory>

namespace chatapp::server::websocket {

class WebSocketSession : public std::enable_shared_from_this<WebSocketSession> {
public:
    WebSocketSession(boost::asio::ip::tcp::socket &&socket);
    UserId &get_user();

    void send(std::string sender, std::string message);

private:
    UserId user;
    boost::asio::ip::tcp::socket socket;
};

} // namespace chatapp::server::websocket

//
// Created by Leonard on 2026-04-15.
//

#include "websocket.hpp"

chatapp::server::websocket::WebSocketSession::WebSocketSession(
    boost::asio::ip::tcp::socket &&socket)
    : socket(std::move(socket)) {
}

chatapp::server::UserId &
chatapp::server::websocket::WebSocketSession::get_user() {
}

void chatapp::server::websocket::WebSocketSession::run_detached(
    boost::beast::http::request<boost::beast::http::string_body> &&request) {
}

void chatapp::server::websocket::WebSocketSession::send(
    std::string sender, std::string message) {
}

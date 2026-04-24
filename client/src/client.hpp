//
// Created by Leonard on 2026-04-14.
//

#pragma once
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <optional>
#include <string>

namespace chatapp::client {

enum class ClientState {
    PreLogin,
    Login
};

class Client {
public:
    explicit Client(boost::asio::io_context &ioc);

    void run_detached(std::string host, std::string port);

private:
    boost::asio::io_context &ioc;

    ClientState state;

    std::optional<std::string> login_base64;

    boost::asio::awaitable<void> session(std::string host, std::string port);
};

} // namespace chatapp::client

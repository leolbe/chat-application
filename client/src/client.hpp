//
// Created by Leonard on 2026-04-14.
//

#pragma once
#include "auth.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <optional>
#include <string>

namespace chatapp::client {

class Client {
public:
    explicit Client(boost::asio::io_context &ioc);

    void run_detached(std::string host, std::string port);

private:
    boost::asio::io_context &ioc;

    std::optional<auth::Auth> auth;

    boost::asio::awaitable<void> session(std::string host, std::string port);

    boost::asio::awaitable<bool> fetch_messages(
        boost::beast::flat_buffer &buf,
        boost::beast::tcp_stream &stream,
        std::string const &host,
        auth::Auth const &auth);

    boost::asio::awaitable<bool> login_prompt(
        boost::beast::flat_buffer &buf,
        boost::beast::tcp_stream &stream,
        std::string &host);

    boost::asio::awaitable<bool> change_login_prompt(
        boost::beast::flat_buffer &buf,
        boost::beast::tcp_stream &stream,
        std::string &host);

    boost::asio::awaitable<bool> signup_prompt(
        boost::beast::flat_buffer &buf,
        boost::beast::tcp_stream &stream,
        std::string &host);

    boost::asio::awaitable<bool> login(
        boost::beast::flat_buffer &buf,
        boost::beast::tcp_stream &stream,
        std::string &host);
};

} // namespace chatapp::client

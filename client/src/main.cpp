//
// Created by Leonard on 2026-03-25.
//

#include "client.hpp"

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace client = chatapp::client;

using asio::use_awaitable;

int main(int argc, char *argv[]) {
    std::string host("127.0.0.1");
    std::string port("3033");

    asio::io_context ioc(1);

    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) {
        ioc.stop();
    });

    client::Client client(ioc);
    client.run_detached(host, port);

    ioc.run();
}

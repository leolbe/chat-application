//
// Created by Leonard on 2026-03-25.
//

#include "server.hpp"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http/message.hpp>

namespace asio = boost::asio;
namespace server = chatapp::server;

int main() {
    auto address = asio::ip::make_address("127.0.0.1");
    unsigned short port(3033);

    asio::io_context ioc(1);

    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](auto, auto) {
        ioc.stop();
    });

    server::Server server(ioc);
    server.run_detached({address, port});

    ioc.run();

    return 0;
}

//
// Created by Leonard on 2026-03-25.
//

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <utils.hpp>
#include <print>

int main(int argc, char *argv[]) {
    boost::beast::error_code ec;
    std::println("{}", hello("client"));
    for (int i = 0; i < argc; i++) {
        std::println("{}", argv[i]);
    }
    return 0;
}

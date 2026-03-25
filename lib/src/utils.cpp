//
// Created by Leonard on 2026-03-25.
//

#include "utils.hpp"

#include <format>

std::string hello(std::string x) {
    return std::format("Hello from {}!", x);
}

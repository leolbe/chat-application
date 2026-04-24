//
// Created by Leonard on 2026-04-23.
//

#pragma once
#include <boost/uuid/uuid.hpp>

namespace chatapp::server {

using UserId = boost::uuids::uuid;

struct Message {
    UserId sender;
    std::string content;
};

struct User {
    std::string username;
    // no security for now
    std::string password;
};

}

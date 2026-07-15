//
// Created by Leonard on 2026-04-23.
//

#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

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

inline std::string user_id_to_string(UserId const &user_id) {
    return boost::uuids::to_string(user_id);
}

}

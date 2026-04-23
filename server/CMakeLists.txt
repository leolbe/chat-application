add_executable(chatapp_server src/main.cpp
        src/server.cpp
        src/server.hpp
        src/websocket.cpp
        src/websocket.hpp
        src/user.cpp
        src/user.hpp
)

target_link_libraries(chatapp_server PRIVATE
        common
        Boost::headers
)
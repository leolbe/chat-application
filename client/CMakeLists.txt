add_executable(chatapp_client src/main.cpp
        src/client.cpp
        src/client.hpp)

target_link_libraries(chatapp_client PRIVATE
        common
        Boost::headers
)

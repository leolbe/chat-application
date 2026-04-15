add_executable(chatapp_server src/main.cpp)

target_link_libraries(chatapp_server PRIVATE
        common
        Boost::headers
)
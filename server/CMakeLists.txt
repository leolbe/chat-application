add_executable(chat_server src/main.cpp)

target_link_libraries(chat_server PRIVATE
        lib
        Boost::headers
)
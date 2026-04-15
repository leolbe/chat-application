add_executable(chat_client src/main.cpp)

target_link_libraries(chatapp_client PRIVATE
        common
        Boost::headers
)

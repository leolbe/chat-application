add_executable(chat_client src/main.cpp)

target_link_libraries(chat_client PRIVATE
        lib
        Boost::headers
)

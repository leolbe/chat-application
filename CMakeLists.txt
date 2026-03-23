cmake_minimum_required(VERSION 3.24)

project(chat_application LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED)

add_executable(chat_application src/main.cpp)

target_link_libraries(chat_application PRIVATE
        Boost::headers
)

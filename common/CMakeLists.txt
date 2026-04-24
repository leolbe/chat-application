add_library(common STATIC
        src/auth.cpp
        src/auth.hpp
        src/protocol.hpp
)

target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(common PUBLIC
        Boost::headers
)
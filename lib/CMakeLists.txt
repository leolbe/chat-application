add_library(lib STATIC
        src/utils.cpp
        src/utils.hpp
)

target_include_directories(lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(lib PUBLIC
        Boost::headers
)
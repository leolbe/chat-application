add_library(common STATIC
        src/utils.cpp
        src/utils.hpp
)

target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(common PUBLIC
        Boost::headers
)
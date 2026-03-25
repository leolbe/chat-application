CC := env_var_or_default("CC", "clang")
CXX := env_var_or_default("CXX", "clang++")

help:
    @just --list

setup TYPE='Debug':
    mkdir -p build/
    conan install . \
        --build=missing \
        -s build_type={{TYPE}}

config TYPE='Debug': (setup TYPE)
    cmake . \
        -B build/{{TYPE}} \
        -DCMAKE_TOOLCHAIN_FILE=build/{{TYPE}}/generators/conan_toolchain.cmake \
        -DCMAKE_BUILD_TYPE={{TYPE}} \
        -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
        -DCMAKE_C_COMPILER={{CC}} \
        -DCMAKE_CXX_COMPILER={{CXX}} \
        -G Ninja

build TYPE='Debug':
    cmake --build build/{{TYPE}}

run-client TYPE='Debug' *ARGS='': (build TYPE)
    ./build/{{TYPE}}/client/chat_client {{ARGS}}

run-server TYPE='Debug' *ARGS='': (build TYPE)
    ./build/{{TYPE}}/server/chat_server {{ARGS}}

clean:
    rm -rf build/

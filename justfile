CC := env_var_or_default("CC", "clang")
CXX := env_var_or_default("CXX", "clang++")

setup TYPE='Debug':
    mkdir -p build/
    conan install . \
        --build=missing \
        -s build_type={{TYPE}}

build TYPE='Debug':
    cmake . \
        -B build/{{TYPE}} \
        -DCMAKE_TOOLCHAIN_FILE=build/{{TYPE}}/generators/conan_toolchain.cmake \
        -DCMAKE_BUILD_TYPE={{TYPE}} \
        -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja \
        -DCMAKE_C_COMPILER={{CC}} \
        -DCMAKE_CXX_COMPILER={{CXX}} \
        -G Ninja
    cmake --build build/{{TYPE}}

clean:
    rm -rf build/

run TYPE='Debug': (build TYPE)
    ./build/{{TYPE}}/chat_application

## Compiler Support
https://en.cppreference.com/w/cpp/compiler_support/20


# Build
### Pre steps
* /cppcheck.sh must be executable -> sudo chmod +x cppcheck.sh
* Install cppcheck -> sudo apt install cppcheck
* Install cmake -> sudo apt install cmake

## Building with Ninja
* sudo apt install ninja-build
* cd root
* mkdir build
* cd build
#### Clang
* -DCMAKE_C_COMPILER=clang-17 -DCMAKE_CXX_COMPILER=clang++-17
#### GCC
* -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
#### Debug Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Debug
#### ReleaseWithDebugInfo Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=RelWithDebInfo
#### MinSizeRelease Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel
#### Release Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release

#### Compile and Link
ninja

## Removing Cppcheck as build dependency
Cppcheck can be removed as a custom build target in case it causes troubles (Visual Studio does not like it e.g.). Or in case you just don't want to install it..
* open root/src/CMakeLists.txt
* Remove line 5 - "add_dependencies(${MAIN_PROJECT} cppcheck)"


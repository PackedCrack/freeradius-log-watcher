## Compiler Support
https://en.cppreference.com/w/cpp/compiler_support/23


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
#### Debug Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### ReleaseWithDebugInfo Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### MinSizeRelease Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!
#### Release Configuration
* cmake .. -G Ninja -D CMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=*!YOUR-COMPILER!* -DCMAKE_CXX_COMPILER=!YOUR-COMPILER!

#### Compile and Link
ninja

## Removing Cppcheck as build dependency
Cppcheck can be removed as a custom build target in case it causes troubles (It doesnt play well with CMake's PCH..).
* open root/src/CMakeLists.txt
* Remove line 5 - "add_dependencies(${MAIN_PROJECT} cppcheck)"


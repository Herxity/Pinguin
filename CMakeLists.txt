cmake_minimum_required(VERSION 3.9.1)
project(CPP_Server)
set(CMAKE_CXX_STANDARD 14)            # Enable c++14 standard
set(files ./lib/server.cpp ./tests/main.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lcurl")
add_executable(out ${files})
set_target_properties(out PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(out curl)

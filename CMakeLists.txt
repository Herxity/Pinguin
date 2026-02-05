cmake_minimum_required(VERSION 3.9.1) #arbitrary
project(Pinguin)


add_library(pinguin_lib ./lib/server.cpp)
#add new files here
set(integration ./tests/main.cpp)
set(units ./tests/pinguin_unit_tests.cpp)
#This is where we add our 'terminal flags'
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lcurl")

#decide things that deal w the final output executable
add_executable(out ${integration})
target_link_libraries(out pinguin_lib curl)
set_target_properties(out PROPERTIES LINKER_LANGUAGE CXX)

add_executable(units ${units})
target_link_libraries(units pinguin_lib)


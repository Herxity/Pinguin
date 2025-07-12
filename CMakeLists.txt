cmake_minimum_required(VERSION 3.9.1) #arbitrary
project(Pinguin)

#add new files here
set(files ./lib/server.cpp ./tests/main.cpp)

#This is where we add our 'terminal flags'
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lcurl")

#decide things that deal w the final output executable
add_executable(out ${files})
set_target_properties(out PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(out curl)

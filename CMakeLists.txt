cmake_minimum_required(VERSION 3.10)
project(google_cloud)
set(CMAKE_CXX_STANDARD 20)

add_library(socket INTERFACE)
target_include_directories(socket INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(main.exe main.cpp)
target_link_libraries(main.exe PRIVATE socket)

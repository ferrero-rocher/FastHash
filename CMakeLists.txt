cmake_minimum_required(VERSION 3.10)
project(kvstore_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

include_directories(include) 
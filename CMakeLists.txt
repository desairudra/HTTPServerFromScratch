cmake_minimum_required(VERSION 3.16)

project(HTTPServerFromScratch)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

file(GLOB SOURCES
    src/*.cpp
)

add_executable(HTTPServerFromScratch ${SOURCES})

target_compile_options(HTTPServerFromScratch PRIVATE
    -Wall
    -Wextra
    -O2
    -pthread
)

target_link_libraries(HTTPServerFromScratch
    pthread
)
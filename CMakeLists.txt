cmake_minimum_required(VERSION 3.25)
project(createplaylist)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if (WIN32)
    add_subdirectory(win32port)
    link_libraries(win32port)
endif()

add_executable(createplaylist main.cpp)

install(TARGETS createplaylist DESTINATION bin)

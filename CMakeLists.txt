cmake_minimum_required(VERSION 3.25)
project(createplaylist)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if (MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(nowide_standalone_v11.3.0)
link_libraries(nowide::nowide)

if (WIN32)
    add_subdirectory(win32port)
    link_libraries(win32port)
endif()


add_executable(createplaylist main.cpp)

install(TARGETS createplaylist DESTINATION bin)

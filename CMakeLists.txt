cmake_minimum_required(VERSION 3.20)
project(createplaylist)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (MSVC)
    add_compile_options(/utf-8)
endif()

if (MSVC)
    add_link_options(/NODEFAULTLIB:libucrt$<$<CONFIG:Debug>:d>.lib)
    link_libraries(ucrt$<$<CONFIG:Debug>:d>.lib)
endif(MSVC)

add_subdirectory(nowide_standalone_v11.3.0)
link_libraries(nowide::nowide)

if (WIN32)
    add_subdirectory(win32port)
    link_libraries(win32port)
endif()


add_executable(createplaylist main.cpp)

install(TARGETS createplaylist DESTINATION bin)


add_library(win32port win32-glob.cc win32-glob.h glob.h)

target_include_directories(win32port INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

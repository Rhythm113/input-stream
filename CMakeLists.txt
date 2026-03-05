cmake_minimum_required(VERSION 3.15)
project(input_stream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Common sources
set(SOURCES src/main.cpp)

# Platform-specific sources and libraries
if(WIN32)
    list(APPEND SOURCES src/input_injector_win.cpp)
    set(PLATFORM_LIBS ws2_32 user32)
    add_definitions(-D_WIN32_WINNT=0x0601)
elseif(APPLE)
    list(APPEND SOURCES src/input_injector_mac.cpp)
    find_library(COREGRAPHICS_LIB CoreGraphics)
    find_library(APPSERVICES_LIB ApplicationServices)
    set(PLATFORM_LIBS ${COREGRAPHICS_LIB} ${APPSERVICES_LIB})
else()
    list(APPEND SOURCES src/input_injector_linux.cpp)
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS Threads::Threads)
endif()

add_executable(input_stream ${SOURCES})

target_include_directories(input_stream PRIVATE include src)
target_link_libraries(input_stream PRIVATE ${PLATFORM_LIBS})

# Suppress httplib warnings
if(MSVC)
    target_compile_options(input_stream PRIVATE /W3 /wd4267 /wd4244)
else()
    target_compile_options(input_stream PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

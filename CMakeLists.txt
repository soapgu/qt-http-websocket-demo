cmake_minimum_required(VERSION 3.21)

project(qt_http_websocket_demo VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SERVER "构建服务端" ON)
option(BUILD_CLIENT "构建客户端" ON)

if(NOT BUILD_SERVER AND NOT BUILD_CLIENT)
    message(FATAL_ERROR "BUILD_SERVER 和 BUILD_CLIENT 不能同时关闭，请至少启用一个子项目。")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_BINARY_DIR}/bin")
endforeach()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

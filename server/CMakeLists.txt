if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# 仅获取源码，不加载上游的构建、安装或可选依赖配置。
FetchContent_Declare(cpp_httplib
    URL https://codeload.github.com/yhirose/cpp-httplib/tar.gz/refs/tags/v0.66.0
    URL_HASH SHA256=537a77cdb96d3b4ad64d0b7103971de52eac75cd4ee03a2fdb84417b0d742459
    SOURCE_SUBDIR qt_demo_unused_subdirectory
)
FetchContent_MakeAvailable(cpp_httplib)

find_package(Threads REQUIRED)
add_library(qt_demo_httplib INTERFACE)
target_include_directories(qt_demo_httplib SYSTEM INTERFACE "${cpp_httplib_SOURCE_DIR}")
target_link_libraries(qt_demo_httplib INTERFACE Threads::Threads)

add_executable(qt_http_server src/main.cpp)
target_link_libraries(qt_http_server PRIVATE qt_demo_httplib)

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core)

add_executable(qt_http_client src/main.cpp)
target_link_libraries(qt_http_client PRIVATE Qt6::Core)

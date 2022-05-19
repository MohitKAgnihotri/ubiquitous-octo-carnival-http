cmake_minimum_required(VERSION 3.16)
project(ubiquitous_octo_carnival_http C)

set(CMAKE_C_STANDARD 11)

link_libraries(pthread)

add_executable(http_client http_client.c http_client.h )
add_executable(http_server http_server.c http_server.h mime.c mime.h)
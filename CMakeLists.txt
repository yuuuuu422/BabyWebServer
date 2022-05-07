cmake_minimum_required(VERSION 3.21)
project(BabyWebServer)
set(CMAKE_CXX_STANDARD 14)

add_executable(BaByWebServer main.cpp http/http_conn.cpp)

find_library(LBRT pthread )
if (LBRT)
    target_link_libraries(BaByWebServer ${LBRT})
endif()
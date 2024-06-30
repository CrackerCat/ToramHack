cmake_minimum_required(VERSION 3.22.1)

project(toramhack)

find_package(cxx REQUIRED CONFIG)

find_package(shadowhook REQUIRED CONFIG)

link_libraries(log cxx::cxx shadowhook::shadowhook)

file(GLOB_RECURSE BNM_SRC ByNameModding/*.cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED main.cpp ${BNM_SRC})

include_directories(ByNameModding/include)
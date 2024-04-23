cmake_minimum_required(VERSION 3.22.1)

project(toramhack)

file(GLOB_RECURSE kitty KittyMemory/*.cpp)

add_library(${CMAKE_PROJECT_NAME} SHARED main.cpp ByNameModding/BNM.cpp ${kitty})

find_package(shadowhook REQUIRED CONFIG)

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC log shadowhook::shadowhook)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ByNameModding KittyMemory)
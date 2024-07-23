cmake_minimum_required(VERSION 3.22.1)

project(toramhack)

find_package(cxx REQUIRED CONFIG)

link_libraries(cxx::cxx)

if (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(ARCH "arm64")
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(ARCH "arm")
endif ()

file(GLOB BNM_SRC BNM-Android/src/*.cpp)

file(GLOB SHADOWHOOK_SRC
        shadowhook/shadowhook/src/main/cpp/*.c
        shadowhook/shadowhook/src/main/cpp/arch/${ARCH}/*.c
        shadowhook/shadowhook/src/main/cpp/common/*.c
        shadowhook/shadowhook/src/main/cpp/third_party/xdl/*.c
)

add_library(${CMAKE_PROJECT_NAME} SHARED main.cpp cJSON/cJSON.c ${BNM_SRC} ${SHADOWHOOK_SRC})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        cJSON
        shadowhook/shadowhook/src/main/cpp
        shadowhook/shadowhook/src/main/cpp/arch/${ARCH}
        shadowhook/shadowhook/src/main/cpp/include
        shadowhook/shadowhook/src/main/cpp/common
        shadowhook/shadowhook/src/main/cpp/third_party/bsd
        shadowhook/shadowhook/src/main/cpp/third_party/lss
        shadowhook/shadowhook/src/main/cpp/third_party/xdl
        BNM-Android/include/BNM
        BNM-Android/external/include
        BNM-Android/external/utf8
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE log)
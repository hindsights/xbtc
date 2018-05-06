cmake_minimum_required(VERSION 2.8)

project(xbtc)

include_directories("/usr/local/include")
link_directories("/usr/local/lib")
include_directories("/usr/local/opt/openssl/include")
link_directories("/usr/local/opt/openssl/lib")
include_directories("./src")
add_definitions(-std=c++11)

if (APPLE)
  add_definitions(-Wno-unused-local-typedefs -Wno-unused-variable -Wno-reorder)
endif ()

if (NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -fPIC")
endif ()

file(GLOB_RECURSE xbtc_cppfiles "src/*.cpp")
file(GLOB_RECURSE xbtc_hfiles "src/*.hpp")
set(xbtc_sources ${xbtc_cppfiles} ${xbtc_hfiles})
message("xbtc sources: ${xbtc_sources}")


add_library(xbtc STATIC ${xbtc_sources})

add_executable(xbtcspv "app/spv.cpp")
target_link_libraries(xbtc boost_thread boost_system z crypto leveldb)
target_link_libraries(xbtcspv xbtc)

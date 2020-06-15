cmake_minimum_required (VERSION 3.10)

project(vssd)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set( VSSD_SOURCES 
   src/vssd.cpp
)

set( VSSD_TESTS 
   test/vssd.cpp
)

find_package(spdlog 1 REQUIRED)
find_package(embree 3 REQUIRED)

add_library(${PROJECT_NAME} ${VSSD_SOURCES})
target_link_libraries(${PROJECT_NAME} spdlog)
target_link_libraries(${PROJECT_NAME} embree)
target_link_libraries(${PROJECT_NAME} stdc++fs)

add_executable(${PROJECT_NAME}_test ${VSSD_TESTS})
target_link_libraries( ${PROJECT_NAME}_test embree)
target_link_libraries( ${PROJECT_NAME}_test ${PROJECT_NAME})

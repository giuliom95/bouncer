cmake_minimum_required (VERSION 3.10)

project(vspt)

set(VSPT_MAJOR_VERSION 0)
set(VSPT_MINOR_VERSION 1)
set(VSPT_VERSION ${VSPT_MAJOR_VERSION}.${VSPT_MINOR_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set( VSPT_SOURCES 
   src/scene.cpp
   src/vspt.cpp
)

find_package(spdlog 1 REQUIRED)
find_package(embree 3 REQUIRED)

add_executable(${PROJECT_NAME} ${VSPT_SOURCES})
target_link_libraries(${PROJECT_NAME} spdlog)
target_link_libraries(${PROJECT_NAME} embree)
target_link_libraries(${PROJECT_NAME} stdc++fs)
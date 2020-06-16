cmake_minimum_required (VERSION 3.10)

project(bouncer)

set(BOUNCER_MAJOR_VERSION 0)
set(BOUNCER_MINOR_VERSION 1)
set(BOUNCER_VERSION ${BOUNCER_MAJOR_VERSION}.${BOUNCER_MINOR_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set( BOUNCER_SOURCES 
   src/scene.cpp
   src/bouncer.cpp
)

find_package(spdlog 1 REQUIRED)
find_package(embree 3 REQUIRED)

add_executable(${PROJECT_NAME} ${BOUNCER_SOURCES})
target_link_libraries(${PROJECT_NAME} spdlog)
target_link_libraries(${PROJECT_NAME} embree)
target_link_libraries(${PROJECT_NAME} stdc++fs)
cmake_minimum_required (VERSION 3.10)

project(vssd)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

set( VSSD_SOURCES 
   src/sceneloader.cpp
)

set( VSSD_TESTS 
   test/vssd.cpp
)

add_library(${PROJECT_NAME} ${VSSD_SOURCES})
add_executable(${PROJECT_NAME}_test ${VSSD_TESTS})
target_link_libraries( ${PROJECT_NAME} ${LIBS} m )

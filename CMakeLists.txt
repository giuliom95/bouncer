cmake_minimum_required (VERSION 3.10)

project(bouncer)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(BOUNCER_MAJOR_VERSION 0)
set(BOUNCER_MINOR_VERSION 1)
set(BOUNCER_VERSION ${BOUNCER_MAJOR_VERSION}.${BOUNCER_MINOR_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(NOT MATH_INCLUDE)
  message(FATAL_ERROR "Please define MATH_INCLUDE")
endif(NOT MATH_INCLUDE)
if(NOT GATHERER_INCLUDE)
  message(FATAL_ERROR "Please define GATHERER_INCLUDE")
endif(NOT GATHERER_INCLUDE)

get_filename_component(MATH_INCLUDE_ABS "${MATH_INCLUDE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
get_filename_component(GATHERER_INCLUDE_ABS "${GATHERER_INCLUDE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

include_directories(
  include
  ${MATH_INCLUDE_ABS}
  ${GATHERER_INCLUDE_ABS}
)

set( BOUNCER_SOURCES 
   src/camera.cpp
   src/scene.cpp
   src/bouncer.cpp
)

find_package(embree 3 REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenImageIO REQUIRED)
find_package(Boost COMPONENTS log filesystem REQUIRED)

add_executable(${PROJECT_NAME} ${BOUNCER_SOURCES})
target_link_libraries(${PROJECT_NAME} embree)
target_link_libraries(${PROJECT_NAME} OpenImageIO)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

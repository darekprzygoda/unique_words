cmake_minimum_required(VERSION 3.5)

project(uwords LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory( Catch2 )

add_compile_options(
  "-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

set( BOOST_VERSION 1.79.0 )
set( BOOST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/boost-${BOOST_VERSION}/include CACHE INTERNAL "Boost include directory" FORCE )
include_directories( SYSTEM ${BOOST_INCLUDE_DIR} )

add_library( util STATIC util.cpp )

add_executable( gen gen.cpp )
target_link_libraries( gen PRIVATE util )

add_executable( uwc uwc.cpp )
target_link_libraries( uwc PRIVATE util )

add_executable( test test.cpp)
target_link_libraries( test PRIVATE Catch2::Catch2WithMain util )

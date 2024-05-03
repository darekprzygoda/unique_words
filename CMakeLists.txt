cmake_minimum_required(VERSION 3.5)

project(uwords LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
  "-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

#set( BOOST_VERSION 1.79.0 )
#set( BOOST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/boost-${BOOST_VERSION}/include CACHE INTERNAL "Boost include directory" FORCE )
#include_directories( SYSTEM ${BOOST_INCLUDE_DIR} )

if ("$ENV{CATCH2_DIR}" STREQUAL "")
    set( CATCH2_DIR "${CMAKE_SOURCE_DIR}/../../Catch2/deploy" )
endif()

include_directories( SYSTEM "${CATCH2_DIR}/include" )
link_directories( "${CATCH2_DIR}/lib" )

# add_library(Catch2 STATIC IMPORTED)
# set_target_properties( Catch2 PROPERTIES
#   IMPORTED_LOCATION "${CATCH2_DIR}/lib/libCatch2.a"
#   INTERFACE_INCLUDE_DIRECTORIES "${CATCH2_DIR}/include"
# )
# add_library(Catch2Main STATIC IMPORTED)
#
# set_target_properties( Catch2Main PROPERTIES
#   IMPORTED_LOCATION "${CATCH2_DIR}/lib/libCatch2Main.a"
#   INTERFACE_INCLUDE_DIRECTORIES "${CATCH2_DIR}/include"
# )

add_library( util STATIC util.cpp )

add_executable( gen gen.cpp )
target_link_libraries( gen PRIVATE util )

add_executable( uwc uwc.cpp )
target_link_libraries( uwc PRIVATE util )

add_executable( test test.cpp)
target_link_libraries( test PRIVATE util Catch2Main Catch2 )

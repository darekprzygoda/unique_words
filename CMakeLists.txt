cmake_minimum_required(VERSION 3.5)

project(uwords LANGUAGES CXX)

add_compile_options(
  "-Wall" "-Wpedantic" "-Wextra" "-fexceptions"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
)

if (NOT "${CATCH2_DIR}" STREQUAL "")
    # message( WARNING "dur: ${CATCH2_DIR}" )
    include_directories( "${CATCH2_DIR}/include" )
    link_directories( "${CATCH2_DIR}/lib" )
endif()

add_library( util STATIC util.cpp )

add_executable( gen gen.cpp )
target_link_libraries( gen PRIVATE util )

add_executable( uwc uwc.cpp )
target_link_libraries( uwc PRIVATE util pthread )

add_executable( test test.cpp)
target_link_libraries( test PRIVATE util Catch2Main Catch2 )

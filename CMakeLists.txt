# CMake recipe for the whole project
# 
# Compilers: gcc, clang, MSVC
# Configurations: Release, Debug, Gprof (gcc only)
#

cmake_minimum_required (VERSION 3.7)

if (POLICY CMP0115)
  cmake_policy (SET CMP0115 NEW)
endif ()

project (chess)

# The default build type for this project is Release
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release
    RelWithDebInfo MinSizeRel Gprof."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Compiler options

set (CMAKE_C_STANDARD 99)
set (CMAKE_C_STANDARD_REQUIRED True)
set (CMAKE_C_EXTENSIONS False)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")

  add_compile_options (
    -msse4
    -Wfatal-errors
    -Wall
    -Werror
    -Wpedantic

    $<$<CONFIG:Gprof>:-pg>
    $<$<CONFIG:Gprof>:-g>
    $<$<CONFIG:Gprof>:-Wa,-adhln>
    $<$<CONFIG:Gprof>:-O3>
  )

  add_link_options (

    -rdynamic

    $<$<CONFIG:Gprof>:-pg>
    $<$<CONFIG:Gprof>:-g>
  )

endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang")

  add_compile_options (
    -msse4
    -Wfatal-errors
    -Wall
    -Werror
    -Wpedantic
  )

  add_link_options (
    -rdynamic
  )

endif ()

if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")

  add_compile_definitions (_CRT_SECURE_NO_WARNINGS)

endif ()

add_subdirectory (src)
add_subdirectory (app)
add_subdirectory (bench)

enable_testing ()
add_subdirectory (test)

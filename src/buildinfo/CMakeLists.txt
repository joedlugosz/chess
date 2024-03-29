# CMake recipe for `libbuildinfo` which contains information about the build

# Generate buildinfo.c
# buildinfo.c contains variables with the build information. As a generated
# file, it is not part of the repo, so it is placed in the binary directory.
# `cmake -P` sets all directory variables to the working dir, so the binary dir
# is supplied as an argument, and the supplied working directory is the source
# dir to access git information. This target is always run on builds (ALL) so
# that commit info is always current. The output file is specified as BYPRODUCTS
# so that its timestamp doesn't change unless its contents change. This avoids
# unnecessary rebuilds of dependencies.
add_custom_target (
  buildinfo_src ALL
  BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.c
  COMMAND ${CMAKE_COMMAND} -P buildinfo.cmake -- ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Target to build libbuildinfo
add_library (buildinfo STATIC ${CMAKE_CURRENT_BINARY_DIR}/buildinfo.c)
include_directories(buildinfo ${CMAKE_CURRENT_SOURCE_DIR})

#
# Generate a string containing the compiler flags that are used in the build,
# separated by spaces.
#

# flags_list is a combination of MAKE_CXX_FLAGS_<BUILD> and whatever flags are added
# using add_compile_options
string (TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" type_name)
set (cflags ${${type_name}})
if ("${cflags}" STREQUAL "")
  set (flags_list "")
else ()
  string (REPLACE " " ";" flags_list ${cflags})
endif ()
set (flags_list "${flags_list}${CMAKE_C_COMPILER_ARG1}")
get_target_property (value buildinfo COMPILE_OPTIONS)
if (value)
  list (APPEND flags_list "${value}")
endif ()
# flags is a space separated string with the contents of flags_list
# trimmed of any unnecessary spaces or empty items and omitting warning flags
set (flags "")
foreach (flag ${flags_list})
  string (STRIP ${flag} flag)
  if ((NOT ${flag} STREQUAL "") AND (NOT ${flag} MATCHES "^-W"))
    string (CONCAT flags "${flags}${flag} ")
  endif ()
endforeach ()
string (STRIP ${flags} flags)

target_compile_definitions (buildinfo PUBLIC -DBUILD_FLAGS="${flags}")
target_compile_definitions (buildinfo PUBLIC -DBUILD_CONFIG="${CMAKE_BUILD_TYPE}")
target_compile_definitions (buildinfo PUBLIC -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")

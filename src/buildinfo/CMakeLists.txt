# Generate build info file
add_custom_command (
  PRE_BUILD
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.c
  COMMAND ${CMAKE_COMMAND} -P buildinfo.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library (buildinfo STATIC buildinfo.c)
set_target_properties (buildinfo PROPERTIES LINKER_LANGUAGE C)

# Generate a string containing the compiler flags that are used in the build,
# separated by spaces.
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
# flags is a space separated string with the contents of flags_list,
# trimmed of any unnecessary spaces or empty items
set (flags "")
foreach (flag ${flags_list})
  string (STRIP ${flag} flag)
  if (NOT ${flag} STREQUAL "")
    string (CONCAT flags "${flags}${flag} ")
  endif ()
endforeach ()
string (STRIP ${flags} flags)

target_compile_definitions (buildinfo PUBLIC -DBUILD_FLAGS="${flags}")
target_compile_definitions (buildinfo PUBLIC -DBUILD_CONFIG="${CMAKE_BUILD_TYPE}")


# The default build type for this project is Release
# Gprof - generate information for gprof
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

# Generate version info header file
add_custom_command (
  PRE_BUILD
  OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h
  COMMAND ${CMAKE_SOURCE_DIR}/version.sh > ${CMAKE_SOURCE_DIR}/src/version.h
)

# info.c will be built as a separate library which depends on version.h
add_library (info STATIC info.c version.h)

# The rest of the app
set (
  SOURCES 
    commands.c
    evaluate.c
    fen.c
    history.c
    io.c
    log.c
    movegen.c
    moves.c
    options.c
    search.c
    state.c
    ui.c
)

# OS-dependent stuff
if (WIN32)
  list (APPEND SOURCES win.c)
else ()
  list (APPEND SOURCES posix.c)
endif ()

# Executable target
add_executable (chess ${SOURCES})
target_link_libraries (chess info)

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
get_target_property (value chess COMPILE_OPTIONS)
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

# Program info definitions supplied when compiling info.c
target_compile_definitions (info PUBLIC -DBUILD_FLAGS="${flags}")
target_compile_definitions (info PUBLIC -DBUILD_CONFIG="${CMAKE_BUILD_TYPE}")


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

add_subdirectory(buildinfo)

# The rest of the app
set (
  SOURCES 
    commands.c
    evaluate.c
    fen.c
    history.c
    io.c
    info.c
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
target_link_libraries (chess buildinfo)

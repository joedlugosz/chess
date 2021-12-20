
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

  add_compile_options (

    -std=c99

    -Wfatal-errors
    -Wall
    -Werror

    -msse4

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

# Generate version info
add_custom_command (
  PRE_BUILD
  OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h
  COMMAND ${CMAKE_SOURCE_DIR}/version.sh > ${CMAKE_SOURCE_DIR}/src/version.h
)
# info as a separate library to depend on version.h
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

add_executable (chess ${SOURCES})
target_link_libraries (chess info)

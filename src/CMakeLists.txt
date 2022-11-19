# Common code used by the app and tests

# Build information
add_subdirectory (buildinfo)
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("execinfo.h" HAVE_EXECINFO_H)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/compiler.h.in" "${CMAKE_CURRENT_BINARY_DIR}/compiler.h" @ONLY)

# Most of the app
set (
  SOURCES
    cmdline.c
    commands.c
    debug.c
    epd.c
    evaluate.c
    fen.c
    hash.c
    history.c
    io.c
    info.c
    movegen.c
    moves.c
    options.c
    search.c
    position.c
    ui.c
)

# OS-dependent stuff
if (WIN32)
  list (APPEND SOURCES win.c)
else ()
  list (APPEND SOURCES posix.c)
endif ()

include_directories(common ${CMAKE_CURRENT_BINARY_DIR})

# Library target
add_library (common STATIC ${SOURCES})
target_link_libraries(common buildinfo m)

if (WIN32)
  target_link_libraries(common dbghelp)
endif ()

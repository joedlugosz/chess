# Common code used by the app and tests

# Build information
add_subdirectory (buildinfo)

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

# Library target
add_library (common STATIC ${SOURCES})
target_link_libraries(common buildinfo m)

if (WIN32)
  target_link_libraries(common dbghelp)
endif ()

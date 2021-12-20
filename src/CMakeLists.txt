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
    posix.c
    search.c
    state.c
    ui.c
)

include_directories (.)
add_executable (chess ${SOURCES})
target_link_libraries(chess info)

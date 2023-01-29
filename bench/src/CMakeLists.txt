# bench/src/CMakeLists.txt
# Targets for benchmarking programs

add_executable (bench_search search.c)
target_link_libraries (bench_search common)
target_include_directories (bench_search PRIVATE 
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/bench
)

add_executable (bench_epd epd.c)
target_link_libraries (bench_epd common)
target_include_directories (bench_epd PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/bench
)

add_executable (bench_moves moves.c)
target_link_libraries (bench_moves common)
target_include_directories (bench_moves PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/bench
)

add_executable (find_magics find_magics.c)
target_link_libraries (find_magics common)
target_include_directories (find_magics PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/bench
)

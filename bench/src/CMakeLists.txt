
add_executable (bench_search search.c)
target_link_libraries (bench_search common)
target_include_directories (bench_search PRIVATE 
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/bench
)



add_executable (test_hash hash.c)
target_link_libraries (test_hash common test_common)
target_include_directories (test_hash PRIVATE 
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/test
)

add_test (
  NAME src-hash
  COMMAND test_hash
)

add_executable (test_history history.c)
target_link_libraries (test_history common test_common)
target_include_directories (test_history PRIVATE 
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_SOURCE_DIR}/test
)

add_test (
  NAME src-history
  COMMAND test_history
)

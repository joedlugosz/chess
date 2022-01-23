
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

# Testing

get_target_property(exe_dir chess BINARY_DIR)
set (exename "${exe_dir}/chess")

add_library (test_common STATIC test.c)

add_subdirectory (src)
add_subdirectory (perft)

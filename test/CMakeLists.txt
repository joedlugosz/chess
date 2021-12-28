# Testing

get_target_property(exe_dir chess BINARY_DIR)
set (exename "${exe_dir}/chess")

add_subdirectory (perft)

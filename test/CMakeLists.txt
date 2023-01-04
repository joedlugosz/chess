# Testing

if (PUBLISH)
  include ("../src/buildinfo/gitinfo.cmake")
  set (output "chess-${git_version}")
else ()
  set (output "chess")
endif ()
get_target_property(exe_dir ${output} BINARY_DIR)
set (exename "${exe_dir}/${output}")

add_library (test_common STATIC test.c)

add_subdirectory (src)
add_subdirectory (perft)

# Target to make the application
if (PUBLISH)
  include ("../src/buildinfo/gitinfo.cmake")
  set (output "chess-${git_version}")
else ()
  set (output "chess")
endif ()
add_executable (${output} main.c)
target_link_libraries (${output} common)
target_include_directories (${output} PRIVATE ../src)

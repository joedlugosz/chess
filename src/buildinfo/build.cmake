# Script to generate build info

find_package(Git)

# Identify commit
execute_process(
  COMMAND "${GIT_EXECUTABLE}" rev-parse --verify HEAD --short --dirty
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE git_version
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Identify OS and CPU
if (WIN32)
  set (os_name "Windows")
  execute_process(
    COMMAND set PROCESSOR_IDENTIFIER
    OUTPUT_VARIABLE target_name
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else ()
  execute_process(
    COMMAND uname -o
    OUTPUT_VARIABLE os_name
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND uname -p
    OUTPUT_VARIABLE target_name
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif ()

# Generate build info source
configure_file("${CMAKE_SOURCE_DIR}/build.c.in" "${CMAKE_BINARY_DIR}/build.c" @ONLY)

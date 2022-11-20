# Script to generate build info

# Arguments (required)
# 0     1  2               3  4
# cmake -P buildinfo.cmake -- <BINARY DIR>
set (bin_dir ${CMAKE_ARGV4})

find_package(Git QUIET)
if (Git_FOUND)
  # Identify version from last tag and commit
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" describe --tags --abbrev=0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE git_version_tag
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE git_branch_name
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE git_commit_short
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set (git_version "${git_version_tag}-${git_branch_name}-${git_commit_short}")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" show -s --format="%cd" --date=format:"%d/%M/%Y"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE source_date
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else ()
  set (git_version "Unknown")
endif ()

# Identify OS and CPU
if (WIN32)
  set (os_name "Windows")
  execute_process(
    COMMAND cmd.exe /c set PROCESSOR_IDENTIFIER
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
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/buildinfo.c.in" "${bin_dir}/buildinfo.c" @ONLY)

# If git tools can be found, set $git_version and $source_date from repo information.
find_package(Git QUIET)
if (Git_FOUND)
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
    COMMAND "${GIT_EXECUTABLE}" show -s --format="%cd" --date=format:"%b %d %Y"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE source_date
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else ()
  set (git_version "Unknown")
endif ()

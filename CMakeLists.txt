project (chess)
add_definitions (-std=c99)

add_custom_command(
  PRE_BUILD
  OUTPUT ${CMAKE_SOURCE_DIR}/src/version.h 
  COMMAND ${CMAKE_SOURCE_DIR}/version.sh > ${CMAKE_SOURCE_DIR}/src/version.h
)
add_library(info STATIC src/info.c src/version.h)

add_subdirectory (src)

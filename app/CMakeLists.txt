# The app

# Executable target
add_executable (chess main.c)
target_link_libraries (chess common)
target_include_directories (chess PRIVATE ../src)

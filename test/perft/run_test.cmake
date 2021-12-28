# Run a perft test to a specified depth and compare to a reference output

# Arguments (required)
# 0     1  2               3  4               5           6           7
# cmake -P run_test.cmake -- <REFERENCE DIR> <EXECUTABLE> <TEST NAME> <DEPTH>
set (reference_dir ${CMAKE_ARGV4})
set (exename ${CMAKE_ARGV5})
set (testname ${CMAKE_ARGV6})
set (depth ${CMAKE_ARGV7})

# Reference output file
set (reference_file ${reference_dir}/${testname}/${testname})
# Truncated reference output file for comprison with test output
set (ref_out_file "${testname}-ref.out")
# Test output file
set (test_out_file "${testname}-test.out")

# Empty list items are not ignored
cmake_policy (SET CMP0007 NEW)

# Test position is first line in reference file
file (READ "${reference_file}" reference_fen)
string (REGEX REPLACE "\n.*" "" reference_fen "${reference_fen}")

# Write test stdin to file
set (input "fen ${reference_fen} getfen perft ${depth} q\n")
file (WRITE test.in ${input})

# Run test
execute_process(
  COMMAND ${exename} x
  INPUT_FILE test.in
  OUTPUT_FILE ${test_out_file}
)

# Cleanup
execute_process(
  COMMAND ${CMAKE_COMMAND} -E remove test.in
)

# Output at depth n will be the fen string then n further lines
# Truncate the reference output so that it matches up to the depth
# and store it
math (EXPR end "${depth}+2")
file (READ "${reference_file}" reference_out)
string (REGEX REPLACE "\n" ";" reference_out "${reference_out}")
list (SUBLIST reference_out 0 ${end} reference_out)
string (REGEX REPLACE ";" "\n" reference_out "${reference_out}")
file (WRITE ${ref_out_file} "${reference_out}\n")

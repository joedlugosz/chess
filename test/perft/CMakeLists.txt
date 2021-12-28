set (
  tests
    starting
    position2
    position3
    position4
)

foreach (test ${tests})

  # Run test
  add_test (
    NAME perft-${test}-run
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake 
      -- ${CMAKE_CURRENT_SOURCE_DIR} ${exename} ${test} 5
  )
  # Compare output with reference
  add_test (
    NAME perft-${test}-check
    COMMAND ${CMAKE_COMMAND} -E compare_files ${test}-test.out ${test}-ref.out
  )

endforeach (test)


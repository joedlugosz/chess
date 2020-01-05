#!/usr/bin/bash

EXENAME=""

TEST_NUMBER=0
set_test() {
    TEST_NUMBER=$1
}
next_test() {
    ((TEST_NUMBER++))
}

test_engine() {
    echo Test $TEST_NUMBER
    COMMANDS=$1
    $EXENAME x <<< $COMMANDS > engine.out
    next_test
}

EXENAME=$1
set_test 1
test_engine 'q'

#!/usr/bin/bash

EXENAME=""

TEST_NUMBER=0
set_test() {
    TEST_NUMBER="$1"
}
next_test() {
    ((TEST_NUMBER++))
}

test_perft() {
    REFERENCE=$1
    DEPTH=$2
    read FEN < $REFERENCE
    $EXENAME x <<< 'fen '$FEN' getfen perft '$DEPTH' q' > perft.out
    diff perft.out <(head -n `expr $DEPTH + 2` $REFERENCE) > diff.out
    if [ $? -eq 1 ]
    then
        echo Failed test $TEST_NUMBER
        printf "   "
        sed -n 2,2p $REFERENCE 
        cat diff.out
        #cleanup
        #exit 1
    fi
    next_test
}

cleanup() {
    rm perft.out
    rm diff.out
}

EXENAME=$1

set_test 100
test_perft starting 5
test_perft position2 4
test_perft position3 3
test_perft position4 3
cleanup
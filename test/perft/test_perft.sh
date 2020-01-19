#!/usr/bin/bash

EXENAME=""

TEST_NUMBER=0
set_test() {
    TEST_NUMBER=$1
}
next_test() {
    ((TEST_NUMBER++))
}

test_perft() {
    echo Test $TEST_NUMBER
    REFERENCE=$1/$1
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

test_perftd() {
    REFERENCE=$1/$1
    DEPTH=$2
    read FEN < $REFERENCE
    #echo 'fen '$FEN' perftd '$DEPTH' q'
    $EXENAME x <<< 'fen '$FEN' perftd '$DEPTH' q' > perft.out
    diff <(sort perft.out) <(sort $1/stockfish$DEPTH) > diff.out 
    if [ $? -eq 1 ]
    then
        echo Failed test $TEST_NUMBER
        printf "   "
        #sed -n 2,2p $REFERENCE 
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

set_test 1
#test_perftd starting 4
#test_perftd position2 4

#set_test 100
test_perft starting 5
test_perft position2 5
test_perft position3 5
test_perft position4 5
cleanup
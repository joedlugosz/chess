#!/usr/bin/bash

EXENAME=""

TEST_NUMBER=0
set_test() {
    TEST_NUMBER="$1"
}
next_test() {
    ((TEST_NUMBER++))
}

test_fen_loopback() {
    INPUT=$1
    COMMAND='fen '$INPUT' getfen q'
    OUTPUT=`$EXENAME x <<< $COMMAND`
    if [ "$INPUT" != "$OUTPUT" ]
    then
        echo Failed test $TEST_NUMBER
        echo Expected: $INPUT
        echo Received: $OUTPUT
        exit 1
    fi
    next_test
}

test_fen_perft() {
    COMMAND='fen '$1' perft 5 q'
    $EXENAME x <<< $COMMAND
    next_test
}

EXENAME=$1

set_test 1
test_fen_loopback 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -'

set_test 100
test_fen_perft 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -'

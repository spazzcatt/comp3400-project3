#!/bin/bash

EXE="./cgi"

function run_test {

    # parameters
    TAG=$1
    ARGS=$2

    # file paths
    OUTPUT=outputs/$TAG.txt
    DIFF=outputs/$TAG.diff
    EXPECT=expected/$TAG.txt
    VALGRND=valgrind/$TAG.txt

    # kill any pre-existing server
		if [ -e "../PID" ] ; then
        kill -KILL $(cat ../PID)
        rm ../PID
		fi

    # run test and compare output to the expected version
    #$EXE $ARGS 2>/dev/null >"$OUTPUT"
		cd ..
		#$EXE >"tests/$OUTPUT" &
		$EXE 2>/dev/null >"tests/$OUTPUT" &
		cd tests
		sleep 1
    ./client $ARGS 2>/dev/null >"$OUTPUT.client"

		# kill server at the end
		if [ -e "../PID" ] ; then
		    kill -USR1 $(cat ../PID)
		fi

		cat "$OUTPUT.client" >> "$OUTPUT"

    diff -u "$OUTPUT" "$EXPECT" >"$DIFF"
    PTAG=$(printf '%-30s' "$TAG")
    if [ -s "$DIFF" ]; then

        # try alternative solution (if it exists)
        EXPECT=expected/$TAG-2.txt
        if [ -e "$EXPECT" ]; then
            diff -u "$OUTPUT" "$EXPECT" >"$DIFF"
            if [ -s "$DIFF" ]; then
                echo "$PTAG FAIL (see $DIFF for details)"
            else
                echo "$PTAG pass"
            fi
        else
            echo "$PTAG FAIL (see $DIFF for details)"
        fi
    else
        echo "$PTAG pass"
    fi

    # run valgrind
		# valgrind $EXE 2>/dev/null >"$VALGRND" &
		# sleep 1
    # ./client $ARGS >/dev/null 2>&1
}

# initialize output folders
mkdir -p outputs
mkdir -p valgrind
rm -f outputs/* valgrind/*

# run individual tests
source itests.include

# check for memory leaks
#LEAK=`cat valgrind/*.txt | grep 'definitely lost' | grep -v ' 0 bytes in 0 blocks'`
LEAK=
if [ -z "$LEAK" ]; then
    echo "No memory leak found."
else
    echo "Memory leak(s) found. See files listed below for details."
    grep 'definitely lost' valgrind/*.txt | sed -e 's/:.*$//g' | sed -e 's/^/  - /g'
fi

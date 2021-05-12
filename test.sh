#!/bin/bash - 

# finally the last slog...
echo "libhyl Test suite!"


# Test cases 
TESTS=( 
# non-existent config
	"examples/noconfig.com|/asdf" 
# bad syntax config
	"examples/badconfig.com|/asdf" 
# static paths (small file, <25kb)
	"examples/stresstest.com|/assets/tiny.jpg"
# static paths (big file, >2mb)
	"examples/stresstest.com|/assets/big.jpg"
# static paths (ridiculous file, >100mb)
	"examples/stresstest.com|/assets/testimony.wav"
# find not found domains
	"examples/stresstest.com|/asdf"
)


# cycle through all of the routes
for x in ${TESTS[@]}
do
	VALS=( `echo $x | awk -F '|' '{ print $1, $2 }'` )
	printf "\nRunning test: hypno-harness -d ${VALS[0]} -u ${VALS[1]}\n"
	hypno-harness -l lib/libhyl.so -d ${VALS[0]} -u ${VALS[1]} >/dev/null
done




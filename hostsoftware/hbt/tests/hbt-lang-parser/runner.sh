#!/bin/sh

dir=$(dirname $0)
hbc=$1
total=0
failed=0

for file in $dir/*.hbc
do
	total=$(($total + 1))
	expectation=$(head -n1 $file | sed -e 's,//,,')
	output=$($hbc -fsyntax-only $file 2>&1)
	if echo "$output" | grep "$expectation$" >/dev/null
	then
		echo "PASS: " $(basename $file)
	else
		echo $output
		echo "FAIL: " $(basename $file)
		failed=$(($failed + 1))
	fi
done

if [ $failed -ne 0 ]
then
	exit 1
else
	exit 0
fi

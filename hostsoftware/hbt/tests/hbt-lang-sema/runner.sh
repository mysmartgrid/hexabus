#!/bin/sh

dir=$(dirname $0)
hbc=$1
total=0
failed=0

for file in $dir/*.hbc
do
	total=$(($total + 1))
	if $hbc -fsyntax-only $file
	then
		echo "PASS: " $(basename $file)
	else
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

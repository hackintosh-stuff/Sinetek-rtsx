#!/bin/bash

# This program will overwrite the files from BSD into this repo (overwriting any
# changes!).

function my_copy() {
	DIR1="$1"
	DIR2="$2"

	echo "Copying files from '$DIR1' into '$DIR2' (.c)..."
	for i in $DIR1/*.c; do
		file="${i##*/}"
		file_ne="`basename $file .c`"
		if [ -e "$DIR2/$file_ne.cpp" ]; then
			cp "$DIR1/$file" "$DIR2/$file_ne.cpp"
		fi
	done
	echo "Copying files from '$DIR1' into '$DIR2' (.h)..."
	for i in $DIR1/*.h; do
		file="${i##*/}"
		if [ -e "$DIR2/$file" ]; then
			cp "$DIR1/$file" "$DIR2/$file"
		fi
	done
}

echo "WARNING: This program will overwrite openbsd-files in this repo."
echo "Are you sure (type YES to continue)??"
read resp
if [ "$resp" != "YES" ]; then
	echo "Aborting..."
	exit 1
fi

DIR1="$HOME/Downloads/src-master/sys/dev/sdmmc"
DIR2="`dirname $0`/Sinetek-rtsx"
my_copy "$DIR1" "$DIR2"

DIR1="$HOME/Downloads/src-master/sys/dev/ic"
my_copy "$DIR1" "$DIR2"

DIR1="$HOME/Downloads/src-master/sys/sys"
my_copy "$DIR1" "$DIR2"

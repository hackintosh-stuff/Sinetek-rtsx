#!/bin/bash

DIR="$HOME/Downloads"

if [ -e "$DIR/src-master.zip" -o -e "$DIR/src-master" ]; then
	echo "Target exists. I will not overwrite it."
	exit 1
fi

curl -L https://github.com/openbsd/src/archive/master.zip > "$DIR/src-master.zip"

unzip "$DIR/src-master.zip" \
	'src-master/sys/sys/device.h' \
	'src-master/sys/dev/sdmmc/*' \
	'src-master/sys/dev/ic/rtsx*' \
	-d "$DIR"

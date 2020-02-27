#!/usr/bin/sh

bin="../build/lang"
src=$(find . -iname "*.scp")

for file in $src ; do
	echo "Building $bin build $file"
	$bin build $file > "/dev/null" 2>&1
	if [ $? -ne 0 ] ; then
		printf "\033[1;31mERROR:\033[0m Could not build $file\n"
	fi
done

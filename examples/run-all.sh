#!/usr/bin/sh

bin="../build/ghoul"
src=$(find . -iname "*.gh")

for file in $src ; do
	trimmed=$(echo "$file" | sed 's@.*/@@' | cut -f 1 -d '.')
	echo "Building module $trimmed $file"
	$bin build "$trimmed" > "/dev/null" 2>&1
	if [ $? -ne 0 ] ; then
		printf "\033[1;31mERROR:\033[0m Could not build $file\n"
	fi
done

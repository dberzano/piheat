#!/bin/bash
cd "$( dirname "$0" )"
SOURCE_PREFIX="$( cd .. ; pwd )"
cat Doxyfile.in | sed -e "s|@SOURCE_PREFIX@|$SOURCE_PREFIX|g" > Doxyfile
exec doxygen Doxyfile

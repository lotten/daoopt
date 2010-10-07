#!/bin/bash

FILES="makefile subdir.mk"

for f in ${FILES} ; do
  for x in `find . -iname ${f} ` ; do
    cat $x | sed 's/ccache g/g/g' > ${x}.new
    mv ${x}.new ${x}
  done
done


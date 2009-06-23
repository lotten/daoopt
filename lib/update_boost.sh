#!/bin/bash

if [ "$1" == "" ] || [ "$2" == "" ] ; then echo "Missing parameters"; exit; fi

if [ ! -d $1/boost/ ] ; then echo "Directory $1 not found" ; exit ; fi
if [ ! -d $2 ] ; then echo "Directory $2 not found" ; exit ; fi

source=$1
target=$2

echo "$source -> $target"

# Create directory structure
L=`find boost/ -type d | sed 's/boost\///g'`
for D in $L ; do mkdir $target/$D ; done

# Now copy files
L=`find boost/ -type f | sed 's/boost\///g'`

for F in $L ; do
  echo "$source/boost/$F -> $target/$F"
  [ -r $source/boost/$F ] && \
  awk '{ gsub( /<boost\// , "\"boost/" ) ; \
         gsub( /\.hpp>/ , ".hpp\"" ) ; \
         print }' $source/boost/$F > $target/$F
done



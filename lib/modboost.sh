#!/bin/bash
[ ! "$1" == "" ] && \
  awk '{ gsub( /<boost\// , "\"boost/" ) ; \
    gsub( /\.hpp>/ , ".hpp\"" ) ; \
    print }' $1 > $1.new && \
  mv $1.new $1

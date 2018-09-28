#!/bin/bash

DATA_DIR=../data

TEXT_DATA=$DATA_DIR/text8
ZIPPED_TEXT_DATA="${TEXT_DATA}.zip"
SOURCE=http://mattmahoney.net/dc/text8.zip

if [ ! -e $TEXT_DATA ]; then
  if [ ! -e $ZIPPED_TEXT_DATA ]; then
    echo "Fetching $SOURCE"
    curl $SOURCE > $ZIPPED_TEXT_DATA
  fi
  echo "Unzipping " + $ZIPPED_TEXT_DATA
  cd $DATA_DIR
  unzip $ZIPPED_TEXT_DATA
else
  echo "$TEXT_DATA already exists."
fi

#!/bin/bash

# Our second experimental condition: pinned words, with any examples involving
# a pinned word repeated 1000 times.

DATA_DIR=../data
BIN_DIR=../bin
SRC_DIR=../src

TEXT_DATA=$DATA_DIR/text8
VECTOR_DATA=$DATA_DIR/experiment3/text8-vector.bin

if [ ! -e $VECTOR_DATA ]; then
  if [ ! -e $TEXT_DATA ]; then
		sh ./create-text8-data.sh
	fi
  echo -----------------------------------------------------------------------------------------------------
  echo -- Training vectors...
  time $BIN_DIR/word2vec -train $TEXT_DATA -output $VECTOR_DATA -cbow 0 -size 200 -window 8 -negative 25 -hs 0 -sample 1e-4 -threads 20 -binary 1 -iter 15 -pin 1 -pin-repeats 1000
fi

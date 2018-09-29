
# Joe Strout's CSC585 Project

This is the code repository for Joe Strout's Fall 2018 class project for CSC 585, Algorithms for Natural Language Processing, at [University of Arizona](http://www.arizona.edu). This repository contains:

+ A C implementation of word2vec, forked from https://github.com/dav/word2vec
+ Custom code to output particular sets of embedded word vectors for further analysis.

# Installation

The following instructions assume a recent macOS environment with *gcc* and *make* (obtained, for example, by installing the Xcode command-line tools).  Other Unix/Linux environments should work similarly.

1. Download the project from https://github.com/JoeStrout/csc585-project

2. At a shell prompt, cd to the `src` directory:
```bash
cd trunk/src
```

3. Build the executables with `make`:
```bash
make
```
This should compile and link cleanly, with no errors or warnings.  It uses *gcc* with only standard libraries.

4. Get the test data.  The easiest way to do this is via the `get-data.sh` script:
```bash
cd ../scripts
./get-data.sh
```
If this script fails, perhaps because your system lacks the *curl* or *unzip* commands, then manually download the [text8 zip file](http://mattmahoney.net/dc/text8.zip) and unzip it into the data directory.


# Training

To generate embedded word vectors, run the following script in `scripts` directory:
```bash
./create-text8-vector-data.sh
```
This looks in the `data` directory for the `test8` file downloaded in step 4 of the installation above, and applies the word2vec algorithm to generate embedded word vectors for the 1.7 million words therein.

Progress will be displayed as the training proceeds.  On my MacBook Pro, it takes about six minutes.  Go get a cup of coffee.


# Data extraction & analysis

To reproduce the baseline analysis in the HW3 paper, change to the `bin` directory and run the `strout-baseline` executable, passing in the path to the word vectors:
```bash
cd ../bin
./strout-baseline ../data/text8-vector.bin
```
This will write selected words, along with correct target values, to three CSV files in the data directory:

+ genderWords.csv: target value = 1 for female, 1 for male
+ latitudeWords.csv: target value is degrees North divided by 90
+ massWords.csv: target value is 0.1 * log10(mass in kg)

You can then open these CSV files in your favorite spreadsheet program, or paste them into Google Sheets as I have done [here](https://docs.google.com/spreadsheets/d/1K2xWKkExk595OfiI0aGBo7MCvoYJHXi13ZVKEBjEe2I).

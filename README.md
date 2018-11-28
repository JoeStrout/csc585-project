
# Joe Strout's CSC585 Project

This is the code repository for Joe Strout's Fall 2018 class project for CSC 585, Algorithms for Natural Language Processing, at [University of Arizona](http://www.arizona.edu). This repository contains:

+ A C implementation of word2vec, forked from https://github.com/dav/word2vec
+ Custom code to output particular sets of embedded word vectors for further analysis.

This repository's official home is [here](https://github.com/JoeStrout/csc585-project).

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
If this script fails, perhaps because your system lacks the *curl* or *unzip* commands, then manually download the [text8 zip file](http://mattmahoney.net/dc/text8.zip) and unzip it into the data directory.  The result is the first 100 mb of a normalized (lowercased, with punctuation removed) Wikipedia dump.

# How to Skip Training

Pre-trained word vectors four each of the four experiments can be found in the data directory.  For each experiment, you will find a notes file describing the experimental conditions; CSV files containing training and test words for gender, latitude, and mass (and in the case of experiment 4, also _isDangerous_ and _hasWheels_), and the full set of word embeddings in binary form (text8-vector.bin).

# Training

To regenerate basic embedded word vectors, run the following script in `scripts` directory:
```bash
./create-text8-vector-data.sh
```
This looks in the `data` directory for the `test8` file downloaded in step 4 of the installation above, and applies the word2vec algorithm (in skip-gram mode) to generate embedded word vectors for the 1.7 million words therein.

Alternatively, you can run one of the four experimental scripts, `experiment-1.sh` through `experiment-4.sh`.  The first of these is equivalent to `create-text8-vector-data.sh`, while the others apply different options:

* `experiment-1.sh`: baseline, equivalent to `create-text8-vector-data.sh`
* `experiment-2.sh`: simple pinning of gender, latitude, and mass
* `experiment-3.sh`: as above, but with pinned examples repeated 1000X
* `experiment-4.sh`: simple pinning with the addition of _hasWheels_ and _isDangerous_

Progress will be displayed as the training proceeds.  On my MacBook Pro, it takes about half an hour, except for experiment 3, which took about 24 hours.

After any experimental run, the `text8-vector.bin` word embeddings will be found in the data directory.  The next step is to extract and analyze the data of interest.

# Data extraction & analysis

To reproduce the analyses in the HW4 paper, change to the `bin` directory and run the `extract` executable, passing in the path to the word vectors:
```bash
cd ../bin
./extract ../data/text8-vector.bin
```
This will write selected words, along with correct target values, to five CSV files in the data directory:

+ genderWords.csv: target value = 1 for female, 1 for male
+ latitudeWords.csv: target value is degrees North divided by 90
+ massWords.csv: target value is 0.1 * log10(mass in kg)
+ hasWheels.csv: target value is 1 where _hasWheels_ is true, 0 where false
+ isDangerous.csv: target value is 1 where _isDangerous_ is true, 0 where false

You can then open these CSV files in your favorite spreadsheet program, or paste them into Google Sheets as I have done here:

+ [Experiment 1]https://docs.google.com/spreadsheets/d/1K2xWKkExk595OfiI0aGBo7MCvoYJHXi13ZVKEBjEe2I/edit)
+ [Experiment 2](https://docs.google.com/spreadsheets/d/19JAdXgdeqhVVEyZB_crp6Vdz6ipA_ZduYhDHIzE0MKw/edit)
+ [Experiment 3](https://docs.google.com/spreadsheets/d/1whWXltAz9JLxGX1eAXj1EX57EhAZvdwqRzGjrERz8tI/edit)
+ [Experiment 4](https://docs.google.com/spreadsheets/d/1I4IotygJBsB9d8Tt4K0PM4qMbyEC_L2TXUjB9Ew8xeE/edit)


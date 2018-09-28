//  Copyright 2013 Google Inc. All Rights Reserved.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <stdlib.h> // mac os x
// #include <malloc.h>

int vector_len;			// actual length (number of floats) of our vectors

// Very simple class to represent an embedded word vector.
// We currently do no automatic release of the data, as the vast majority of 
// vectors that will be passed around will be part of the actual dictionary,
// and should never be destroyed.  Callers should manually delete d on
// temporary results.
class Vector {
  public:
	float *d;		// dynamically-allocated array of length vector_len
	
	void Print(const char* terminator) {
		printf("%0.3f", d[0]);
		for (int i=1; i<vector_len; i++) printf(", %0.3f", d[i]);
		printf("%s", terminator);
	}
		
};

// Vocabulary: stored in an array of fixed-length records, max_w chars each.
long num_words;					// number of words we have
const long max_w = 50;				// max length of vocabulary words
char *vocab;							// actual vocabulary entries (text)

// Vectors: stored in an array of Vector pointers, parallel to the vocab array.
Vector* vectors;

bool Allocate(long numWords) {
	vocab = (char *)malloc((long)numWords * max_w * sizeof(char));
	if (!vocab) {
		printf("Vocab allocation failed\n");
		return false;
	}
	vectors = (Vector*)malloc(numWords * sizeof(Vector));
	if (!vectors) {
		printf("Vector allocation failed\n");
		return false;
	}	
	return true;
}

char* WordAtIndex(long index) {
	return &vocab[index * max_w];
}

long IndexOfWord(const char *word) {
	for (long i=0; i<num_words; i++) {
		if (strcmp(word, WordAtIndex(i)) == 0) return i;
	}
	return -1;
}

Vector VectorOfWord(const char *word) {
	return vectors[IndexOfWord(word)];
}

void OutputCSVHeader(FILE *f) {
	fprintf(f,"word,index,target");
	for (int i=0; i<vector_len; i++) fprintf(f, ",d[%d]", i);
	fprintf(f, "\n");
}

void OutputCSVLine(FILE *f, const char *word, float targetValue) {
	long idx = IndexOfWord(word);
	fprintf(f, "%s,%ld,%0.1f", word, idx, targetValue);
	Vector &v = vectors[idx];
	for (int i=0; i<vector_len; i++) fprintf(f, ",%0.5f", v.d[i]);
	fprintf(f, "\n");
}

// Output a CSV file containing the embedding vectors of particular words
// of interest, so we can do further analysis in external tools.
void OutputWordsOfInterest(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	OutputCSVLine(f, "female", 1);
	OutputCSVLine(f, "male", -1);
	OutputCSVLine(f, "she", 1);
	OutputCSVLine(f, "he", -1);
	OutputCSVLine(f, "queen", 1);
	OutputCSVLine(f, "king", -1);
	OutputCSVLine(f, "duchess", 1);
	OutputCSVLine(f, "duke", -1);
	OutputCSVLine(f, "aunt", 1);
	OutputCSVLine(f, "uncle", -1);
	OutputCSVLine(f, "girl", 1);
	OutputCSVLine(f, "boy", -1);
	OutputCSVLine(f, "niece", 1);
	OutputCSVLine(f, "nephew", -1);
	OutputCSVLine(f, "mother", 1);
	OutputCSVLine(f, "father", -1);
	OutputCSVLine(f, "wife", 1);
	OutputCSVLine(f, "husband", -1);
	OutputCSVLine(f, "nun", 1);
	OutputCSVLine(f, "priest", -1);
	OutputCSVLine(f, "actress", 1);
	OutputCSVLine(f, "actor", -1);
	OutputCSVLine(f, "bride", 1);
	OutputCSVLine(f, "groom", -1);
	OutputCSVLine(f, "lady", 1);
	OutputCSVLine(f, "gentleman", -1);
	OutputCSVLine(f, "waitress", 1);
	OutputCSVLine(f, "waiter", -1);
	OutputCSVLine(f, "daughter", 1);
	OutputCSVLine(f, "son", -1);
	OutputCSVLine(f, "princess", 1);
	OutputCSVLine(f, "prince", -1);
	OutputCSVLine(f, "her", 1);
	OutputCSVLine(f, "him", -1);	
}

int main(int argc, char **argv) {
	// validate arguments
	if (argc < 2) {
		printf("Usage: ./strout-baseline <FILE>\nwhere FILE contains word projections in the BINARY FORMAT\n");
		return 0;
	}

	// open the data file and read all entries into memory
	FILE *f = fopen(argv[1], "rb");
	if (f == NULL) {
		printf("Input file not found\n");
		return -1;
	}
	
	fscanf(f, "%ld", &num_words);
	fscanf(f, "%d", &vector_len);

	if (!Allocate(num_words)) return -1;

	for (long idx = 0; idx < num_words; idx++) {
		// read the vocab word (text), up to the first space or \n
		char *wordc = WordAtIndex(idx);
		while (true) {
			char c = fgetc(f);
			if (feof(f) or c == ' ') break;
			if (c != '\n') *wordc++ = c;
		}
		*wordc = 0;
		// then, read the vector entries
		Vector v;
		v.d = (float*)malloc(vector_len * sizeof(float));
		for (int i=0; i<vector_len; i++) {
			fread(&v.d[i], sizeof(float), 1, f);
		}
		vectors[idx] = v;
	}
	fclose(f);

	printf("Read %ld num_words, with vector length %d\n", num_words, vector_len);

	printf("Index of 'queen': %ld\n", IndexOfWord("queen"));
	VectorOfWord("queen").Print("\n");
	
	printf("Index of 'king': %ld\n", IndexOfWord("king"));
	VectorOfWord("king").Print("\n");
	
	const char* outfile = "../data/wordsOfInterest.csv";
	OutputWordsOfInterest(outfile);
	printf("Wrote CSV file to %s\n", outfile);

	return 0;
}

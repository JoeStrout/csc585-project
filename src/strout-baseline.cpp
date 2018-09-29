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
	if (idx < 0) {
		fprintf(stderr, "Word not found in vocabulary: %s\n", word);
		return;
	}
	fprintf(f, "%s,%ld,%0.2f", word, idx, targetValue);
	Vector &v = vectors[idx];
	for (int i=0; i<vector_len; i++) fprintf(f, ",%0.5f", v.d[i]);
	fprintf(f, "\n");
}

// Output a CSV file containing the embedding vectors of particular words
// of interest with gender targets, so we can do further analysis in external tools.
// We'll encode gender as follows: 1 = female, -1 = male.
void OutputGenderWords(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	// Training data:
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
	// Test data:
	OutputCSVLine(f, "daughter", 1);
	OutputCSVLine(f, "son", -1);
	OutputCSVLine(f, "princess", 1);
	OutputCSVLine(f, "prince", -1);
	OutputCSVLine(f, "her", 1);
	OutputCSVLine(f, "him", -1);
	
	printf("Wrote %s\n", filename);
}

// Output a CSV file containing the embedding vectors of particular words
// of interest with latitude targets, so we can do further analysis in external tools.
// We'll encode latitude as a linear scale, latitude (in degrees) / 90.
void OutputLatitudeWords(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	// conversion factor from latitude degrees to our encoding:
	double degrees = 1.0 / 90.0;
	// Training data:
	OutputCSVLine(f, "helsinki", 60 * degrees);
	OutputCSVLine(f, "bergen", 60 * degrees);
	OutputCSVLine(f, "oslo", 58 * degrees);
	OutputCSVLine(f, "stockholm", 58 * degrees);
	OutputCSVLine(f, "edinburgh", 55 * degrees);
	OutputCSVLine(f, "copenhagen", 55 * degrees);
	OutputCSVLine(f, "moscow", 55 * degrees);
	OutputCSVLine(f, "hamburg", 53 * degrees);
	OutputCSVLine(f, "amsterdam", 52 * degrees);
	OutputCSVLine(f, "berlin", 52 * degrees);
	OutputCSVLine(f, "london", 51 * degrees);
	OutputCSVLine(f, "prague", 50 * degrees);
	OutputCSVLine(f, "vancouver", 49 * degrees);
	OutputCSVLine(f, "paris", 48 * degrees);
	OutputCSVLine(f, "munich", 48 * degrees);
	OutputCSVLine(f, "vienna", 48 * degrees);
	OutputCSVLine(f, "budapest", 47 * degrees);
	OutputCSVLine(f, "montreal", 45 * degrees);
	OutputCSVLine(f, "venice", 45 * degrees);
	OutputCSVLine(f, "toronto", 43 * degrees);
	OutputCSVLine(f, "florence", 43 * degrees);
	OutputCSVLine(f, "boston", 42 * degrees);
	OutputCSVLine(f, "chicago", 41 * degrees);
	OutputCSVLine(f, "barcelona", 41 * degrees);
	OutputCSVLine(f, "rome", 41 * degrees);
	OutputCSVLine(f, "istanbul", 41 * degrees);
	OutputCSVLine(f, "madrid", 40 * degrees);
	OutputCSVLine(f, "naples", 40 * degrees);
	OutputCSVLine(f, "beijing", 39 * degrees);
	OutputCSVLine(f, "athens", 37 * degrees);
	OutputCSVLine(f, "seoul", 37 * degrees);
	OutputCSVLine(f, "tokyo", 35 * degrees);
	OutputCSVLine(f, "kyoto", 35 * degrees);
	OutputCSVLine(f, "beirut", 33 * degrees);
	OutputCSVLine(f, "cairo", 30 * degrees);
	OutputCSVLine(f, "delhi", 28 * degrees);
	OutputCSVLine(f, "miami", 25 * degrees);
	OutputCSVLine(f, "taipei", 25 * degrees);
	OutputCSVLine(f, "macau", 22 * degrees);
	OutputCSVLine(f, "honolulu", 21 * degrees);
	OutputCSVLine(f, "hanoi", 21 * degrees);
	OutputCSVLine(f, "mumbai", 18 * degrees);
	OutputCSVLine(f, "bangkok", 13 * degrees);
	OutputCSVLine(f, "caracas", 10 * degrees);
	OutputCSVLine(f, "nairobi", 1 * degrees);
	// Test data:
	OutputCSVLine(f, "dublin", 53 * degrees);
	OutputCSVLine(f, "zurich", 47 * degrees);
	OutputCSVLine(f, "lisbon", 38 * degrees);
	OutputCSVLine(f, "osaka", 35 * degrees);
	OutputCSVLine(f, "tucson", 32 * degrees);
	OutputCSVLine(f, "dubai", 25 * degrees);
	
	printf("Wrote %s\n", filename);
}

// Encode mass into an appropriate value for word vectors.
// We'll use a log scale, centered on 1 kg.  Examples:
//		1000 kg  -->  0.3
//		   1 kg  -->  0
//		   1 g   --> -0.3
//		   1 mg  --> -0.6
double encodeMass(float massInKg) {
	return log10(massInKg) * 0.1;
}

// Output a CSV file containing the embedding vectors of particular words
// of interest with latitude targets, so we can do further analysis in external tools.
// We'll encode latitude as a linear scale, latitude (in degrees) / 90.
void OutputMassWords(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	// Conversion factors to kilograms:
	float kg = 1;
	float g = 0.001;
	float mg = 0.000001;
	// Training data:
	OutputCSVLine(f, "elephant", encodeMass(5000*kg));
	OutputCSVLine(f, "hippopotamus", encodeMass(3750*kg));
	OutputCSVLine(f, "walrus", encodeMass(1000*kg));
	OutputCSVLine(f, "giraffe", encodeMass(800*kg));
	OutputCSVLine(f, "cow", encodeMass(800*kg));
	OutputCSVLine(f, "buffalo", encodeMass(700*kg));
	OutputCSVLine(f, "horse", encodeMass(700*kg));
	OutputCSVLine(f, "camel", encodeMass(500*kg));
	OutputCSVLine(f, "donkey", encodeMass(400*kg));
	OutputCSVLine(f, "bear", encodeMass(300*kg));
	OutputCSVLine(f, "boar", encodeMass(180*kg));
	OutputCSVLine(f, "lion", encodeMass(160*kg));
	OutputCSVLine(f, "gorilla", encodeMass(140*kg));
	OutputCSVLine(f, "tiger", encodeMass(120*kg));
	OutputCSVLine(f, "human", encodeMass(70*kg));
	OutputCSVLine(f, "cougar", encodeMass(63*kg));
	OutputCSVLine(f, "chimpanzee", encodeMass(45*kg));
	OutputCSVLine(f, "goat", encodeMass(40*kg));
	OutputCSVLine(f, "sheep", encodeMass(40*kg));
	OutputCSVLine(f, "dog", encodeMass(30*kg));
	OutputCSVLine(f, "bobcat", encodeMass(9*kg));
	OutputCSVLine(f, "rat", encodeMass(500*g));
	OutputCSVLine(f, "hamster", encodeMass(160*g));
	OutputCSVLine(f, "gerbil", encodeMass(60*g));
	OutputCSVLine(f, "gecko", encodeMass(30*g));
	OutputCSVLine(f, "ant", encodeMass(20*mg));
	// Test data:
	OutputCSVLine(f, "rhinoceros", encodeMass(1500*kg));
	OutputCSVLine(f, "moose", encodeMass(400*kg));
	OutputCSVLine(f, "dolphin", encodeMass(115*kg));
	OutputCSVLine(f, "coyote", encodeMass(13*kg));
	OutputCSVLine(f, "rabbit", encodeMass(3*kg));
	OutputCSVLine(f, "mouse", encodeMass(30*g));
	
	printf("Wrote %s\n", filename);
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

//	printf("Index of 'queen': %ld\n", IndexOfWord("queen"));
//	VectorOfWord("queen").Print("\n");
//	
//	printf("Index of 'king': %ld\n", IndexOfWord("king"));
//	VectorOfWord("king").Print("\n");
	
	OutputGenderWords("../data/genderWords.csv");
	OutputLatitudeWords("../data/latitudeWords.csv");
	OutputMassWords("../data/massWords.csv");
	
	return 0;
}

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

void OutputHasWheelsWords(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	
	// Training data:
 	OutputCSVLine(f, "cabbage", 0);
	OutputCSVLine(f, "grasshopper", 0);
	OutputCSVLine(f, "hornet", 0);
	OutputCSVLine(f, "peach", 0);
	OutputCSVLine(f, "donkey", 0);
	OutputCSVLine(f, "poppy", 0);
	OutputCSVLine(f, "hippo", 0);
	OutputCSVLine(f, "tarantula", 0);
	OutputCSVLine(f, "bra", 0);
	OutputCSVLine(f, "elephant", 0);
	OutputCSVLine(f, "cushion", 0);
	OutputCSVLine(f, "apple", 0);
	OutputCSVLine(f, "sheep", 0);
	OutputCSVLine(f, "tambourine", 0);
	OutputCSVLine(f, "bus", 1);
	OutputCSVLine(f, "crane", 0);
	OutputCSVLine(f, "peanut", 0);
	OutputCSVLine(f, "willow", 0);
	OutputCSVLine(f, "taxi", 1);
	OutputCSVLine(f, "flannel", 0);
	OutputCSVLine(f, "leg", 0);
	OutputCSVLine(f, "rabbit", 0);
	OutputCSVLine(f, "crab", 0);
	OutputCSVLine(f, "lemonade", 0);
	OutputCSVLine(f, "cape", 0);
	OutputCSVLine(f, "beaver", 0);
	OutputCSVLine(f, "ship", 0);
	OutputCSVLine(f, "sock", 0);
	OutputCSVLine(f, "bicycle", 1);
	OutputCSVLine(f, "tiger", 0);
	OutputCSVLine(f, "tuna", 0);
	OutputCSVLine(f, "thumb", 0);
	OutputCSVLine(f, "eagle", 0);
	OutputCSVLine(f, "sandwich", 0);
	OutputCSVLine(f, "gherkin", 0);
	OutputCSVLine(f, "sycamore", 0);
	OutputCSVLine(f, "rhubarb", 0);
	OutputCSVLine(f, "satsuma", 0);
	OutputCSVLine(f, "hyena", 0);
	OutputCSVLine(f, "caravan", 1);
	OutputCSVLine(f, "hummingbird", 0);
	OutputCSVLine(f, "trousers", 0);
	OutputCSVLine(f, "robe", 0);
	OutputCSVLine(f, "minibus", 1);
	OutputCSVLine(f, "mackerel", 0);
	OutputCSVLine(f, "apricot", 0);
	OutputCSVLine(f, "owl", 0);
	OutputCSVLine(f, "seaweed", 0);
	OutputCSVLine(f, "otter", 0);
	OutputCSVLine(f, "whisky", 0);
	OutputCSVLine(f, "dolphin", 0);
	OutputCSVLine(f, "spider", 0);
	OutputCSVLine(f, "mussel", 0);
	OutputCSVLine(f, "emu", 0);
	OutputCSVLine(f, "locust", 0);
	OutputCSVLine(f, "peacock", 0);
	OutputCSVLine(f, "ostrich", 0);
	OutputCSVLine(f, "warship", 0);
	OutputCSVLine(f, "jellyfish", 0);
	OutputCSVLine(f, "arm", 0);
	OutputCSVLine(f, "gorilla", 0);
	OutputCSVLine(f, "yoghurt", 0);
	OutputCSVLine(f, "wine", 0);
	OutputCSVLine(f, "magpie", 0);
	OutputCSVLine(f, "truck", 1);
	OutputCSVLine(f, "butter", 0);
	OutputCSVLine(f, "salmon", 0);
	OutputCSVLine(f, "camel", 0);
	OutputCSVLine(f, "scorpion", 0);
	OutputCSVLine(f, "ham", 0);
	OutputCSVLine(f, "lamb", 0);
	OutputCSVLine(f, "ambulance", 1);
	OutputCSVLine(f, "zebra", 0);
	OutputCSVLine(f, "flea", 0);
	OutputCSVLine(f, "daffodil", 0);
	OutputCSVLine(f, "pineapple", 0);
	OutputCSVLine(f, "tea", 0);
	OutputCSVLine(f, "rice", 0);
	OutputCSVLine(f, "grapefruit", 0);
	OutputCSVLine(f, "tomato", 0);
	OutputCSVLine(f, "crocodile", 0);
	OutputCSVLine(f, "coffee", 0);
	OutputCSVLine(f, "woodpecker", 0);
	OutputCSVLine(f, "clam", 0);
	OutputCSVLine(f, "sled", 0);
	OutputCSVLine(f, "buggy", 1);
	OutputCSVLine(f, "termite", 0);
	OutputCSVLine(f, "lettuce", 0);
	OutputCSVLine(f, "calf", 0);
	OutputCSVLine(f, "parsley", 0);
	OutputCSVLine(f, "flounder", 0);
	OutputCSVLine(f, "jelly", 0);
	OutputCSVLine(f, "squid", 0);
	OutputCSVLine(f, "rat", 0);
	OutputCSVLine(f, "hyacinth", 0);
	OutputCSVLine(f, "parakeet", 0);
	OutputCSVLine(f, "nightingale", 0);
	OutputCSVLine(f, "carriage", 1);
	OutputCSVLine(f, "pillow", 0);
	OutputCSVLine(f, "monkey", 0);
	OutputCSVLine(f, "moose", 0);
	OutputCSVLine(f, "scallop", 0);
	OutputCSVLine(f, "boat", 0);
	OutputCSVLine(f, "goat", 0);
	OutputCSVLine(f, "cauliflower", 0);
	OutputCSVLine(f, "motorbike", 1);
	OutputCSVLine(f, "oyster", 0);
	OutputCSVLine(f, "leopard", 0);
	OutputCSVLine(f, "buzzard", 0);
	OutputCSVLine(f, "snail", 0);
	OutputCSVLine(f, "sultana", 0);
	OutputCSVLine(f, "plum", 0);
	OutputCSVLine(f, "falcon", 0);
	OutputCSVLine(f, "cake", 0);
	OutputCSVLine(f, "herring", 0);
	OutputCSVLine(f, "ketchup", 0);
	OutputCSVLine(f, "turtle", 0);
	OutputCSVLine(f, "chocolate", 0);
	OutputCSVLine(f, "iguana", 0);
	OutputCSVLine(f, "finger", 0);
	OutputCSVLine(f, "bacon", 0);
	OutputCSVLine(f, "melon", 0);
	OutputCSVLine(f, "garlic", 0);
	OutputCSVLine(f, "watermelon", 0);
	OutputCSVLine(f, "champagne", 0);
	OutputCSVLine(f, "train", 1);
	OutputCSVLine(f, "prune", 0);
	OutputCSVLine(f, "cheetah", 0);
	OutputCSVLine(f, "ear", 0);
	OutputCSVLine(f, "alligator", 0);
	OutputCSVLine(f, "raisin", 0);
	OutputCSVLine(f, "beetle", 0);
	OutputCSVLine(f, "sugar", 0);
	OutputCSVLine(f, "walrus", 0);
	OutputCSVLine(f, "moth", 0);
	OutputCSVLine(f, "lemon", 0);
	OutputCSVLine(f, "platypus", 0);
	OutputCSVLine(f, "broccoli", 0);
	OutputCSVLine(f, "porsche", 1);
	OutputCSVLine(f, "squirrel", 0);
	OutputCSVLine(f, "toe", 0);
	OutputCSVLine(f, "jam", 0);
	OutputCSVLine(f, "shrimp", 0);
	OutputCSVLine(f, "minivan", 1);
	OutputCSVLine(f, "cloak", 0);
	OutputCSVLine(f, "lorry", 1);
	OutputCSVLine(f, "cucumber", 0);
	OutputCSVLine(f, "worm", 0);
	OutputCSVLine(f, "bike", 1);
	OutputCSVLine(f, "winch", 0);
	OutputCSVLine(f, "frog", 0);
	OutputCSVLine(f, "butterfly", 0);
	OutputCSVLine(f, "orange", 0);
	OutputCSVLine(f, "shark", 0);
	OutputCSVLine(f, "drum", 0);
	OutputCSVLine(f, "tugboat", 0);
	OutputCSVLine(f, "jacket", 0);
	OutputCSVLine(f, "raven", 0);
	OutputCSVLine(f, "shawl", 0);
	OutputCSVLine(f, "dragonfly", 0);
	OutputCSVLine(f, "cap", 0);
	OutputCSVLine(f, "scarf", 0);
	OutputCSVLine(f, "wolf", 0);
	OutputCSVLine(f, "llama", 0);
	OutputCSVLine(f, "sunflower", 0);
	OutputCSVLine(f, "turkey", 0);
	OutputCSVLine(f, "panther", 0);
	OutputCSVLine(f, "rhino", 0);
	OutputCSVLine(f, "moss", 0);
	OutputCSVLine(f, "cherry", 0);
	OutputCSVLine(f, "rattlesnake", 0);
	OutputCSVLine(f, "grape", 0);
	OutputCSVLine(f, "oak", 0);
	OutputCSVLine(f, "crayfish", 0);
	OutputCSVLine(f, "hawk", 0);
	OutputCSVLine(f, "gown", 0);
	OutputCSVLine(f, "van", 1);
	OutputCSVLine(f, "pear", 0);
	OutputCSVLine(f, "seagull", 0);
	OutputCSVLine(f, "stockings", 0);
	OutputCSVLine(f, "apron", 0);
	OutputCSVLine(f, "limousine", 1);
	OutputCSVLine(f, "carrot", 0);
	OutputCSVLine(f, "cod", 0);
	OutputCSVLine(f, "wheeler", 1);
	OutputCSVLine(f, "blueberry", 0);
	OutputCSVLine(f, "cricket", 0);
	OutputCSVLine(f, "doll", 0);
	OutputCSVLine(f, "kangaroo", 0);
	OutputCSVLine(f, "gloves", 0);
	OutputCSVLine(f, "pony", 0);
	OutputCSVLine(f, "horse", 0);
	OutputCSVLine(f, "chipmunk", 0);
	OutputCSVLine(f, "sparrow", 0);
	OutputCSVLine(f, "freighter", 0);
	OutputCSVLine(f, "cow", 0);
	OutputCSVLine(f, "pigeon", 0);
	OutputCSVLine(f, "pansy", 0);
	OutputCSVLine(f, "dress", 0);
	OutputCSVLine(f, "orchid", 0);
	OutputCSVLine(f, "partridge", 0);
	OutputCSVLine(f, "motorcycle", 1);
	OutputCSVLine(f, "soup", 0);
	OutputCSVLine(f, "foot", 0);
	OutputCSVLine(f, "pie", 0);
	OutputCSVLine(f, "milk", 0);
	OutputCSVLine(f, "rickshaw", 1);
	OutputCSVLine(f, "eel", 0);
	OutputCSVLine(f, "unicycle", 1);
	OutputCSVLine(f, "mosquito", 0);
	OutputCSVLine(f, "cart", 1);
	OutputCSVLine(f, "nut", 0);
	OutputCSVLine(f, "bean", 0);
	OutputCSVLine(f, "cockroach", 0);
	OutputCSVLine(f, "puppet", 0);
	OutputCSVLine(f, "celery", 0);
	OutputCSVLine(f, "minnow", 0);
	OutputCSVLine(f, "seal", 0);
	OutputCSVLine(f, "tulip", 0);
	OutputCSVLine(f, "lips", 0);
	OutputCSVLine(f, "marigold", 0);
	OutputCSVLine(f, "tobacco", 0);
	OutputCSVLine(f, "lime", 0);
	OutputCSVLine(f, "dates", 0);
	OutputCSVLine(f, "canary", 0);
	OutputCSVLine(f, "caterpillar", 0);
	OutputCSVLine(f, "goose", 0);
	OutputCSVLine(f, "yacht", 0);
	OutputCSVLine(f, "lily", 0);
	OutputCSVLine(f, "aeroplane", 1);
	OutputCSVLine(f, "potato", 0);
	OutputCSVLine(f, "lion", 0);
	OutputCSVLine(f, "tricycle", 1);
	OutputCSVLine(f, "banana", 0);
	OutputCSVLine(f, "birch", 0);
	OutputCSVLine(f, "bread", 0);
	OutputCSVLine(f, "scooter", 1);
	OutputCSVLine(f, "elm", 0);
	OutputCSVLine(f, "fir", 0);
	OutputCSVLine(f, "toad", 0);
	OutputCSVLine(f, "hair", 0);
	OutputCSVLine(f, "mayonnaise", 0);
	OutputCSVLine(f, "cat", 0);
	OutputCSVLine(f, "centipede", 0);
	OutputCSVLine(f, "strawberry", 0);
	OutputCSVLine(f, "radish", 0);
	OutputCSVLine(f, "trout", 0);
	OutputCSVLine(f, "starling", 0);
	OutputCSVLine(f, "onion", 0);
	OutputCSVLine(f, "tractor", 1);
	OutputCSVLine(f, "nose", 0);
	OutputCSVLine(f, "wasp", 0);
	OutputCSVLine(f, "wheelbarrow", 1);
	OutputCSVLine(f, "vessel", 0);
	OutputCSVLine(f, "skirt", 0);
	OutputCSVLine(f, "heron", 0);
	OutputCSVLine(f, "tortoise", 0);
	OutputCSVLine(f, "pig", 0);
	OutputCSVLine(f, "schooner", 0);
	OutputCSVLine(f, "octopus", 0);
	OutputCSVLine(f, "pelican", 0);
	OutputCSVLine(f, "wheelchair", 1);
	OutputCSVLine(f, "skunk", 0);
	OutputCSVLine(f, "lizard", 0);
	OutputCSVLine(f, "swan", 0);
	OutputCSVLine(f, "lobster", 0);
	OutputCSVLine(f, "hamster", 0);
	OutputCSVLine(f, "duck", 0);
	OutputCSVLine(f, "dandelion", 0);
	OutputCSVLine(f, "mushroom", 0);
	OutputCSVLine(f, "dove", 0);
	OutputCSVLine(f, "peas", 0);
	OutputCSVLine(f, "wagon", 1);
	OutputCSVLine(f, "raspberry", 0);
	OutputCSVLine(f, "kingfisher", 0);
	OutputCSVLine(f, "chestnut", 0);
	OutputCSVLine(f, "coach", 1);
	OutputCSVLine(f, "shirt", 0);
	OutputCSVLine(f, "wren", 0);
	OutputCSVLine(f, "frigate", 0);
	OutputCSVLine(f, "porcupine", 0);
	OutputCSVLine(f, "fern", 0);
	OutputCSVLine(f, "asparagus", 0);
	OutputCSVLine(f, "ant", 0);
	OutputCSVLine(f, "artichoke", 0);
	OutputCSVLine(f, "sweater", 0);
	OutputCSVLine(f, "daisy", 0);
	OutputCSVLine(f, "corn", 0);
	OutputCSVLine(f, "pumpkin", 0);
	OutputCSVLine(f, "suit", 0);
	OutputCSVLine(f, "penguin", 0);
	OutputCSVLine(f, "ox", 0);
	OutputCSVLine(f, "bear", 0);
	OutputCSVLine(f, "spinach", 0);
	OutputCSVLine(f, "eucalyptus", 0);
	OutputCSVLine(f, "flamingo", 0);
	OutputCSVLine(f, "tangerine", 0);
	
	// Test data:
	OutputCSVLine(f, "pine", 0);
	OutputCSVLine(f, "raccoon", 0);
	OutputCSVLine(f, "whale", 0);
	OutputCSVLine(f, "cider", 0);
	OutputCSVLine(f, "fox", 0);
	OutputCSVLine(f, "jeep", 1);
	OutputCSVLine(f, "biscuit", 0);
	OutputCSVLine(f, "giraffe", 0);
	OutputCSVLine(f, "arrow", 0);
	OutputCSVLine(f, "bat", 0);
	OutputCSVLine(f, "dog", 0);
	OutputCSVLine(f, "doughnut", 0);
	OutputCSVLine(f, "buffalo", 0);
	OutputCSVLine(f, "slug", 0);
	OutputCSVLine(f, "turnip", 0);
	OutputCSVLine(f, "mouse", 0);
	OutputCSVLine(f, "chicken", 0);
	OutputCSVLine(f, "carnation", 0);
	OutputCSVLine(f, "coconut", 0);
	OutputCSVLine(f, "battleship", 0);
	OutputCSVLine(f, "deer", 0);
	OutputCSVLine(f, "robin", 0);
	OutputCSVLine(f, "olive", 0);
	OutputCSVLine(f, "cannon", 1);
	OutputCSVLine(f, "carp", 0);
	OutputCSVLine(f, "mango", 0);
	OutputCSVLine(f, "trolley", 1);
	OutputCSVLine(f, "bee", 0);
	OutputCSVLine(f, "cheese", 0);
	OutputCSVLine(f, "rose", 0);
	OutputCSVLine(f, "hedgehog", 0);
	OutputCSVLine(f, "car", 1);
	OutputCSVLine(f, "goldfish", 0);
}

void OutputIsDangerousWords(const char *filename) {
	FILE *f = fopen(filename, "w");
	OutputCSVHeader(f);
	
	// Training data:
	OutputCSVLine(f, "chainsaw", 1);
	OutputCSVLine(f, "tricycle", 0);
	OutputCSVLine(f, "panther", 1);
	OutputCSVLine(f, "wolf", 1);
	OutputCSVLine(f, "grizzly", 1);
	OutputCSVLine(f, "syringe", 1);
	OutputCSVLine(f, "ball", 0);
	OutputCSVLine(f, "soup", 0);
	OutputCSVLine(f, "poison", 1);
	OutputCSVLine(f, "axe", 1);
	OutputCSVLine(f, "mop", 0);
	OutputCSVLine(f, "shovel", 0);
	OutputCSVLine(f, "giraffe", 0);
	OutputCSVLine(f, "hod", 0);
	OutputCSVLine(f, "crocodile", 1);
	OutputCSVLine(f, "crossbow", 1);
	OutputCSVLine(f, "jellyfish", 1);
	OutputCSVLine(f, "bullet", 1);
	OutputCSVLine(f, "gun", 1);
	OutputCSVLine(f, "methamphetamines", 1);
	OutputCSVLine(f, "snake", 1);
	OutputCSVLine(f, "scorpion", 1);
	OutputCSVLine(f, "hippo", 1);
	OutputCSVLine(f, "blade", 1);
	OutputCSVLine(f, "lemur", 0);
	OutputCSVLine(f, "gorillas", 1);
	OutputCSVLine(f, "rifle", 1);
	OutputCSVLine(f, "pitchfork", 1);
	OutputCSVLine(f, "glove", 0);
	OutputCSVLine(f, "warthog", 1);
	OutputCSVLine(f, "harpoon", 1);
	OutputCSVLine(f, "cleaver", 1);
	OutputCSVLine(f, "heroin", 1);
	OutputCSVLine(f, "rattlesnake", 1);
	OutputCSVLine(f, "cougar", 1);
	OutputCSVLine(f, "arrow", 1);
	OutputCSVLine(f, "puppet", 0);
	OutputCSVLine(f, "elephant", 1);
	OutputCSVLine(f, "methamphetamine", 1);
	OutputCSVLine(f, "bomb", 1);
	OutputCSVLine(f, "tigress", 1);
	OutputCSVLine(f, "valium", 1);
	OutputCSVLine(f, "sword", 1);
	OutputCSVLine(f, "porcupine", 0);
	OutputCSVLine(f, "weapon", 1);
	OutputCSVLine(f, "recorder", 0);
	OutputCSVLine(f, "motorcycle", 1);
	OutputCSVLine(f, "derringer", 1);
	OutputCSVLine(f, "antelope", 0);
	OutputCSVLine(f, "dinosaur", 1);
	OutputCSVLine(f, "firearm", 1);
	OutputCSVLine(f, "saw", 1);
	OutputCSVLine(f, "bayonet", 1);
	OutputCSVLine(f, "tiger", 1);
	OutputCSVLine(f, "doll", 0);
	OutputCSVLine(f, "methadone", 1);
	OutputCSVLine(f, "cannon", 1);
	OutputCSVLine(f, "toothbrush", 0);
	OutputCSVLine(f, "tyrannosaurus", 1);
	OutputCSVLine(f, "crayon", 0);
	OutputCSVLine(f, "rhinoceros", 1);
	OutputCSVLine(f, "cocaine", 1);
	OutputCSVLine(f, "tapir", 1);
	OutputCSVLine(f, "lions", 1);
	OutputCSVLine(f, "hoe", 0);
	OutputCSVLine(f, "whip", 1);
	OutputCSVLine(f, "helicopter", 1);
	OutputCSVLine(f, "broom", 0);
	OutputCSVLine(f, "otter", 0);
	OutputCSVLine(f, "tambourine", 0);
	OutputCSVLine(f, "jaguar", 1);
	OutputCSVLine(f, "cheetah", 1);
	OutputCSVLine(f, "steroid", 1);
	OutputCSVLine(f, "scissors", 1);
	OutputCSVLine(f, "lion", 1);
	OutputCSVLine(f, "drug", 1);
	OutputCSVLine(f, "amphetamine", 1);
	OutputCSVLine(f, "zebra", 0);
	OutputCSVLine(f, "rattle", 0);
	OutputCSVLine(f, "hyena", 1);
	OutputCSVLine(f, "alligator", 1);
	OutputCSVLine(f, "razor", 1);
	OutputCSVLine(f, "slingshot", 1);
	OutputCSVLine(f, "pistol", 1);
	OutputCSVLine(f, "viper", 1);
	OutputCSVLine(f, "blender", 0);
	OutputCSVLine(f, "goat", 0);
	OutputCSVLine(f, "tortoise", 0);
	OutputCSVLine(f, "spade", 0);
	OutputCSVLine(f, "python", 1);
	OutputCSVLine(f, "silverback", 1);
	OutputCSVLine(f, "shotgun", 1);
	OutputCSVLine(f, "toad", 0);
	OutputCSVLine(f, "rocket", 1);
	OutputCSVLine(f, "marble", 0);
	OutputCSVLine(f, "leopard", 1);
	OutputCSVLine(f, "turtle", 0);
	OutputCSVLine(f, "club", 1);
	OutputCSVLine(f, "handgun", 1);
	OutputCSVLine(f, "dromedary", 0);
	OutputCSVLine(f, "rabbit", 0);
	OutputCSVLine(f, "shark", 1);
	OutputCSVLine(f, "gazelle", 0);
	OutputCSVLine(f, "stabbed", 1);
	OutputCSVLine(f, "axes", 1);
	OutputCSVLine(f, "monkey", 0);
	OutputCSVLine(f, "narcotic", 1);
	OutputCSVLine(f, "kite", 0);
	OutputCSVLine(f, "bucket", 0);
	OutputCSVLine(f, "guenon", 0);
	OutputCSVLine(f, "balloon", 0);
	OutputCSVLine(f, "stabbing", 1);
	OutputCSVLine(f, "satchel", 0);
	OutputCSVLine(f, "spear", 1);
	OutputCSVLine(f, "plough", 1);
	OutputCSVLine(f, "camel", 0);
	OutputCSVLine(f, "knife", 1);
	OutputCSVLine(f, "hornbill", 0);
	OutputCSVLine(f, "boomerang", 1);
	OutputCSVLine(f, "scythe", 1);
	OutputCSVLine(f, "revolver", 1);
	OutputCSVLine(f, "tank", 1);
	OutputCSVLine(f, "swing", 0);
	
	// Test data:
	OutputCSVLine(f, "sledge", 0);
	OutputCSVLine(f, "allergy", 1);
	OutputCSVLine(f, "hatchet", 1);
	OutputCSVLine(f, "jelly", 0);
	OutputCSVLine(f, "meth", 1);
	OutputCSVLine(f, "wheelbarrow", 0);
	OutputCSVLine(f, "rhino", 1);
	OutputCSVLine(f, "dagger", 1);
	OutputCSVLine(f, "machete", 1);
	OutputCSVLine(f, "scalpel", 1);
	OutputCSVLine(f, "hippopotamus", 1);
	OutputCSVLine(f, "rake", 0);
	OutputCSVLine(f, "grenade", 1);
	OutputCSVLine(f, "reserve", 0);
	
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
	OutputHasWheelsWords("../data/hasWheels.csv");
	OutputIsDangerousWords("../data/isDangerous.csv");
	
	return 0;
}

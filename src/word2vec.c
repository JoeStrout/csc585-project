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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <time.h>
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define linux 1

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers
typedef unsigned char bool;
const bool true = 1;
const bool false = 0;

struct vocab_word {
  long count;
  int *point;
  char *word, *code, codelen;
};

char train_file[MAX_STRING], output_file[MAX_STRING];
char save_vocab_file[MAX_STRING], read_vocab_file[MAX_STRING];
struct vocab_word *vocab;
int binary = 0, cbow = 1, debug_mode = 2, window = 5, min_count = 5, num_threads = 12, min_reduce = 1;
bool optPin = false;
int pinRepeats = 1;

int *vocab_hash;
long long vocab_max_size = 1000, vocab_size = 0, layer1_size = 100;
long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;
real alpha = 0.025, starting_alpha, sample = 1e-3;
real *syn0, *syn1, *syn1neg, *expTable;
real *pins;			// array parallel to syn0, with 1 for free-to-change values, and 0 for pinned values
clock_t start;

int hs = 0, negative = 5;
const int table_size = 1e8;
int *table;

void InitUnigramTable() {
  int a, i;
  double train_words_pow = 0;
  double d1, power = 0.75;
  table = (int *)malloc(table_size * sizeof(int));
  for (a = 0; a < vocab_size; a++) train_words_pow += pow(vocab[a].count, power);
  i = 0;
  d1 = pow(vocab[i].count, power) / train_words_pow;
  for (a = 0; a < table_size; a++) {
    table[a] = i;
    if (a / (double)table_size > d1) {
      i++;
      d1 += pow(vocab[i].count, power) / train_words_pow;
    }
    if (i >= vocab_size) i = vocab_size - 1;
  }
}

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
void ReadWord(char *word, FILE *fin) {
  int a = 0, ch;
  while (!feof(fin)) {
    ch = fgetc(fin);
    if (ch == 13) continue;
    if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
      if (a > 0) {
        if (ch == '\n') ungetc(ch, fin);
        break;
      }
      if (ch == '\n') {
        strcpy(word, (char *)"</s>");
        return;
      } else continue;
    }
    word[a] = ch;
    a++;
    if (a >= MAX_STRING - 1) a--;   // Truncate too long words
  }
  word[a] = 0;
}

// Returns hash value of a word
int GetWordHash(const char *word) {
  unsigned long long a, hash = 0;
  for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
  hash = hash % vocab_hash_size;
  return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchVocab(const char *word) {
  unsigned int hash = GetWordHash(word);
  while (1) {
    if (vocab_hash[hash] == -1) return -1;
    if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
    hash = (hash + 1) % vocab_hash_size;
  }
  return -1;
}

// Reads a word and returns its index in the vocabulary
int ReadWordIndex(FILE *fin) {
  char word[MAX_STRING];
  ReadWord(word, fin);
  if (feof(fin)) return -1;
  return SearchVocab(word);
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
  unsigned int hash, length = strlen(word) + 1;
  if (length > MAX_STRING) length = MAX_STRING;
  vocab[vocab_size].word = (char *)calloc(length, sizeof(char));
  strcpy(vocab[vocab_size].word, word);
  vocab[vocab_size].count = 0;
  vocab_size++;
  // Reallocate memory if needed
  if (vocab_size + 2 >= vocab_max_size) {
    vocab_max_size += 1000;
    vocab = (struct vocab_word *)realloc(vocab, vocab_max_size * sizeof(struct vocab_word));
  }
  hash = GetWordHash(word);
  while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
  vocab_hash[hash] = vocab_size - 1;
  return vocab_size - 1;
}

// Used later for sorting by word counts
int VocabCompare(const void *a, const void *b) {
    return ((struct vocab_word *)b)->count - ((struct vocab_word *)a)->count;
}

// Sorts the vocabulary by frequency using word counts
void SortVocab() {
  int a, size;
  unsigned int hash;
  // Sort the vocabulary and keep </s> at the first position
  qsort(&vocab[1], vocab_size - 1, sizeof(struct vocab_word), VocabCompare);
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  size = vocab_size;
  train_words = 0;
  for (a = 0; a < size; a++) {
    // Words occuring less than min_count times will be discarded from the vocab
    if ((vocab[a].count < min_count) && (a != 0)) {
      vocab_size--;
      free(vocab[a].word);
    } else {
      // Hash will be re-computed, as after the sorting it is not actual
      hash=GetWordHash(vocab[a].word);
      while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
      vocab_hash[hash] = a;
      train_words += vocab[a].count;
    }
  }
  vocab = (struct vocab_word *)realloc(vocab, (vocab_size + 1) * sizeof(struct vocab_word));
  // Allocate memory for the binary tree construction
  for (a = 0; a < vocab_size; a++) {
    vocab[a].code = (char *)calloc(MAX_CODE_LENGTH, sizeof(char));
    vocab[a].point = (int *)calloc(MAX_CODE_LENGTH, sizeof(int));
  }
}

// Reduces the vocabulary by removing infrequent tokens
void ReduceVocab() {
  int a, b = 0;
  unsigned int hash;
  for (a = 0; a < vocab_size; a++) if (vocab[a].count > min_reduce) {
    vocab[b].count = vocab[a].count;
    vocab[b].word = vocab[a].word;
    b++;
  } else free(vocab[a].word);
  vocab_size = b;
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  for (a = 0; a < vocab_size; a++) {
    // Hash will be re-computed, as it is not actual
    hash = GetWordHash(vocab[a].word);
    while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
    vocab_hash[hash] = a;
  }
  fflush(stdout);
  min_reduce++;
}

// Create binary Huffman tree using the word counts
// Frequent words will have short unique binary codes
void CreateBinaryTree() {
  long long a, b, i, min1i, min2i, pos1, pos2, point[MAX_CODE_LENGTH];
  char code[MAX_CODE_LENGTH];
  long long *count = (long long *)calloc(vocab_size * 2 + 1, sizeof(long long));
  long long *binary = (long long *)calloc(vocab_size * 2 + 1, sizeof(long long));
  long long *parent_node = (long long *)calloc(vocab_size * 2 + 1, sizeof(long long));
  for (a = 0; a < vocab_size; a++) count[a] = vocab[a].count;
  for (a = vocab_size; a < vocab_size * 2; a++) count[a] = 1e15;
  pos1 = vocab_size - 1;
  pos2 = vocab_size;
  // Following algorithm constructs the Huffman tree by adding one node at a time
  for (a = 0; a < vocab_size - 1; a++) {
    // First, find two smallest nodes 'min1, min2'
    if (pos1 >= 0) {
      if (count[pos1] < count[pos2]) {
        min1i = pos1;
        pos1--;
      } else {
        min1i = pos2;
        pos2++;
      }
    } else {
      min1i = pos2;
      pos2++;
    }
    if (pos1 >= 0) {
      if (count[pos1] < count[pos2]) {
        min2i = pos1;
        pos1--;
      } else {
        min2i = pos2;
        pos2++;
      }
    } else {
      min2i = pos2;
      pos2++;
    }
    count[vocab_size + a] = count[min1i] + count[min2i];
    parent_node[min1i] = vocab_size + a;
    parent_node[min2i] = vocab_size + a;
    binary[min2i] = 1;
  }
  // Now assign binary code to each vocabulary word
  for (a = 0; a < vocab_size; a++) {
    b = a;
    i = 0;
    while (1) {
      code[i] = binary[b];
      point[i] = b;
      i++;
      b = parent_node[b];
      if (b == vocab_size * 2 - 2) break;
    }
    vocab[a].codelen = i;
    vocab[a].point[0] = vocab_size - 2;
    for (b = 0; b < i; b++) {
      vocab[a].code[i - b - 1] = code[b];
      vocab[a].point[i - b] = point[b] - vocab_size;
    }
  }
  free(count);
  free(binary);
  free(parent_node);
}

void LearnVocabFromTrainFile() {
  char word[MAX_STRING];
  FILE *fin;
  long long a, i;
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  fin = fopen(train_file, "rb");
  if (fin == NULL) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  vocab_size = 0;
  AddWordToVocab((char *)"</s>");
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
    train_words++;
    if ((debug_mode > 1) && (train_words % 100000 == 0)) {
      printf("%lldK%c", train_words / 1000, 13);
      fflush(stdout);
    }
    i = SearchVocab(word);
    if (i == -1) {
      a = AddWordToVocab(word);
      vocab[a].count = 1;
    } else vocab[i].count++;
    if (vocab_size > vocab_hash_size * 0.7) ReduceVocab();
  }
  SortVocab();
  if (debug_mode > 0) {
    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);
  }
  file_size = ftell(fin);
  fclose(fin);
}

void SaveVocab() {
  long long i;
  FILE *fo = fopen(save_vocab_file, "wb");
  for (i = 0; i < vocab_size; i++) fprintf(fo, "%s %ld\n", vocab[i].word, vocab[i].count);
  fclose(fo);
}

void ReadVocab() {
  long long a, i = 0;
  char c;
  char word[MAX_STRING];
  FILE *fin = fopen(read_vocab_file, "rb");
  if (fin == NULL) {
    printf("Vocabulary file not found\n");
    exit(1);
  }
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  vocab_size = 0;
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
    a = AddWordToVocab(word);
    // JJS: How does this work?  We wrote out (in SaveVocab) the word followed by the count.
    // But here we seem to be reading them in the opposite order.  I'm not sure this
    // is not a bug.
    fscanf(fin, "%ld%c", &vocab[a].count, &c);
    i++;
  }
  SortVocab();
  if (debug_mode > 0) {
    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);
  }
  fin = fopen(train_file, "rb");
  if (fin == NULL) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  file_size = ftell(fin);
  fclose(fin);
}

// Allocate a (probably quite large) chunk of memory, neatly aligned
// on 128-byte boundaries.
void *Alloc(long long sizeInBytes, const char *memo) {
	void *ptr = NULL;
#ifdef _MSC_VER
	ptr = _aligned_malloc(sizeInBytes, 128);
#elif defined  linux
	posix_memalign(&ptr, 128, sizeInBytes);
#else
	#error Not a supported platform
#endif
	if (ptr == NULL) {
		printf("Memmory allocation failed on %s (%lld bytes)\n", memo, sizeInBytes);
		exit(1);
	}
	return ptr;
}

void Pin(const char *word, long dimension, float value) {
	long index = SearchVocab(word);
	if (index < 0) {
		printf("Can't pin \"%s\" because it is not found in vocabulary.\n", word);
		return;
	}
	long v = index * layer1_size;
	syn0[v + dimension] = value;	// set specified dimension to pre-defined value
	pins[v + dimension] = 0;		// don't allow this value to change
	printf("Pinned value for \"%s\" (word index %ld) on dimension %ld to %f\n",
		word, index, dimension, value);
}

// Return whether the given word (by index) has any pinned dimensions.
bool IsPinned(long wordIndex) {
	long v = wordIndex * layer1_size;	
	// Currently only the first 5 dimensions are ever pinned, so:
	return pins[v] == 0 || pins[v+1] == 0 || pins[v+2] == 0
		|| pins[v+3] == 0 || pins[v+4] == 0;
}


void PinFromBlackboxData(const char *dataPath) {
	printf("Attempting to read: %s\n", dataPath);
	FILE *fin = fopen(dataPath, "rb");
	if (fin == NULL) {
		printf("ERROR: unable to read blackbox data: %s\n", dataPath);
		return;
	}
	char id[8];
	char property[80];
	char word[80];
	int value;
	int lineNum = 1;
	// skip the first (header) line
	fscanf(fin, "%79[^\n]", word);
	while (fscanf(fin, "%7[^,],%79[^,],%79[^,],%d\n", id, property, word, &value) == 4) {
		//printf("Read #%s: %s for %s is %d\n", id, property, word, value);
		if (strcmp(property, "has_wheels") == 0) {
			Pin(word, 3, value);
		} else if (strcmp(property, "is_dangerous") == 0) {
			Pin(word, 4, value);
		}
		lineNum++;
	}
	printf("Processed %d lines from %s\n", lineNum, dataPath);
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

// Initialize the 'pins' array and pinned values.
void InitPins() {
	// Start by initializing all pins to 1, allowing all values to be changed.
	for (long a = 0; a < vocab_size; a++) for (long b = 0; b < layer1_size; b++) {
    	pins[a * layer1_size + b] = 1;
    }
    
    // If not using the pinned option, then we're done.
    if (!optPin) return;

	// Then, pin select values.
	Pin("female", 0, 1);
	Pin("male", 0, -1);
	Pin("she", 0, 1);
	Pin("he", 0, -1);
	Pin("queen", 0, 1);
	Pin("king", 0, -1);
	Pin("duchess", 0, 1);
	Pin("duke", 0, -1);
	Pin("aunt", 0, 1);
	Pin("uncle", 0, -1);
	Pin("girl", 0, 1);
	Pin("boy", 0, -1);
	Pin("niece", 0, 1);
	Pin("nephew", 0, -1);
	Pin("mother", 0, 1);
	Pin("father", 0, -1);
	Pin("wife", 0, 1);
	Pin("husband", 0, -1);
	Pin("nun", 0, 1);
	Pin("priest", 0, -1);
	Pin("actress", 0, 1);
	Pin("actor", 0, -1);
	Pin("bride", 0, 1);
	Pin("groom", 0, -1);
	Pin("lady", 0, 1);
	Pin("gentleman", 0, -1);
	Pin("waitress", 0, 1);
	Pin("waiter", 0, -1);
	
	// conversion factor from latitude degrees to our encoding:
	double degrees = 1.0 / 90.0;

	Pin("helsinki", 1, 60 * degrees);
	Pin("bergen", 1, 60 * degrees);
	Pin("oslo", 1, 58 * degrees);
	Pin("stockholm", 1, 58 * degrees);
	Pin("edinburgh", 1, 55 * degrees);
	Pin("copenhagen", 1, 55 * degrees);
	Pin("moscow", 1, 55 * degrees);
	Pin("hamburg", 1, 53 * degrees);
	Pin("amsterdam", 1, 52 * degrees);
	Pin("berlin", 1, 52 * degrees);
	Pin("london", 1, 51 * degrees);
	Pin("prague", 1, 50 * degrees);
	Pin("vancouver", 1, 49 * degrees);
	Pin("paris", 1, 48 * degrees);
	Pin("munich", 1, 48 * degrees);
	Pin("vienna", 1, 48 * degrees);
	Pin("budapest", 1, 47 * degrees);
	Pin("montreal", 1, 45 * degrees);
	Pin("venice", 1, 45 * degrees);
	Pin("toronto", 1, 43 * degrees);
	Pin("florence", 1, 43 * degrees);
	Pin("boston", 1, 42 * degrees);
	Pin("chicago", 1, 41 * degrees);
	Pin("barcelona", 1, 41 * degrees);
	Pin("rome", 1, 41 * degrees);
	Pin("istanbul", 1, 41 * degrees);
	Pin("madrid", 1, 40 * degrees);
	Pin("naples", 1, 40 * degrees);
	Pin("beijing", 1, 39 * degrees);
	Pin("athens", 1, 37 * degrees);
	Pin("seoul", 1, 37 * degrees);
	Pin("tokyo", 1, 35 * degrees);
	Pin("kyoto", 1, 35 * degrees);
	Pin("beirut", 1, 33 * degrees);
	Pin("cairo", 1, 30 * degrees);
	Pin("delhi", 1, 28 * degrees);
	Pin("miami", 1, 25 * degrees);
	Pin("taipei", 1, 25 * degrees);
	Pin("macau", 1, 22 * degrees);
	Pin("honolulu", 1, 21 * degrees);
	Pin("hanoi", 1, 21 * degrees);
	Pin("mumbai", 1, 18 * degrees);
	Pin("bangkok", 1, 13 * degrees);
	Pin("caracas", 1, 10 * degrees);
	Pin("nairobi", 1, 1 * degrees);

	// Conversion factors to kilograms:
	float kg = 1;
	float g = 0.001;
	float mg = 0.000001;

	Pin("elephant", 2, encodeMass(5000*kg));
	Pin("hippopotamus", 2, encodeMass(3750*kg));
	Pin("walrus", 2, encodeMass(1000*kg));
	Pin("giraffe", 2, encodeMass(800*kg));
	Pin("cow", 2, encodeMass(800*kg));
	Pin("buffalo", 2, encodeMass(700*kg));
	Pin("horse", 2, encodeMass(700*kg));
	Pin("camel", 2, encodeMass(500*kg));
	Pin("donkey", 2, encodeMass(400*kg));
	Pin("bear", 2, encodeMass(300*kg));
	Pin("boar", 2, encodeMass(180*kg));
	Pin("lion", 2, encodeMass(160*kg));
	Pin("gorilla", 2, encodeMass(140*kg));
	Pin("tiger", 2, encodeMass(120*kg));
	Pin("human", 2, encodeMass(70*kg));
	Pin("cougar", 2, encodeMass(63*kg));
	Pin("chimpanzee", 2, encodeMass(45*kg));
	Pin("goat", 2, encodeMass(40*kg));
	Pin("sheep", 2, encodeMass(40*kg));
	Pin("dog", 2, encodeMass(30*kg));
	Pin("bobcat", 2, encodeMass(9*kg));
	Pin("rat", 2, encodeMass(500*g));
	Pin("hamster", 2, encodeMass(160*g));
	Pin("gecko", 2, encodeMass(30*g));
	Pin("ant", 2, encodeMass(20*mg));
	
	// has_wheels
	Pin("cabbage", 3, 0);
	Pin("grasshopper", 3, 0);
	Pin("hornet", 3, 0);
	Pin("peach", 3, 0);
	Pin("donkey", 3, 0);
	Pin("poppy", 3, 0);
	Pin("hippo", 3, 0);
	Pin("tarantula", 3, 0);
	Pin("bra", 3, 0);
	Pin("elephant", 3, 0);
	Pin("cushion", 3, 0);
	Pin("apple", 3, 0);
	Pin("sheep", 3, 0);
	Pin("tambourine", 3, 0);
	Pin("bus", 3, 1);
	Pin("crane", 3, 0);
	Pin("peanut", 3, 0);
	Pin("willow", 3, 0);
	Pin("taxi", 3, 1);
	Pin("flannel", 3, 0);
	Pin("leg", 3, 0);
	Pin("rabbit", 3, 0);
	Pin("crab", 3, 0);
	Pin("lemonade", 3, 0);
	Pin("cape", 3, 0);
	Pin("beaver", 3, 0);
	Pin("ship", 3, 0);
	Pin("sock", 3, 0);
	Pin("bicycle", 3, 1);
	Pin("tiger", 3, 0);
	Pin("tuna", 3, 0);
	Pin("thumb", 3, 0);
	Pin("eagle", 3, 0);
	Pin("sandwich", 3, 0);
	Pin("gherkin", 3, 0);
	Pin("sycamore", 3, 0);
	Pin("rhubarb", 3, 0);
	Pin("satsuma", 3, 0);
	Pin("hyena", 3, 0);
	Pin("caravan", 3, 1);
	Pin("hummingbird", 3, 0);
	Pin("trousers", 3, 0);
	Pin("robe", 3, 0);
	Pin("minibus", 3, 1);
	Pin("mackerel", 3, 0);
	Pin("apricot", 3, 0);
	Pin("owl", 3, 0);
	Pin("seaweed", 3, 0);
	Pin("otter", 3, 0);
	Pin("whisky", 3, 0);
	Pin("dolphin", 3, 0);
	Pin("spider", 3, 0);
	Pin("mussel", 3, 0);
	Pin("emu", 3, 0);
	Pin("locust", 3, 0);
	Pin("peacock", 3, 0);
	Pin("ostrich", 3, 0);
	Pin("warship", 3, 0);
	Pin("jellyfish", 3, 0);
	Pin("arm", 3, 0);
	Pin("gorilla", 3, 0);
	Pin("yoghurt", 3, 0);
	Pin("wine", 3, 0);
	Pin("magpie", 3, 0);
	Pin("truck", 3, 1);
	Pin("butter", 3, 0);
	Pin("salmon", 3, 0);
	Pin("camel", 3, 0);
	Pin("scorpion", 3, 0);
	Pin("ham", 3, 0);
	Pin("lamb", 3, 0);
	Pin("ambulance", 3, 1);
	Pin("zebra", 3, 0);
	Pin("flea", 3, 0);
	Pin("daffodil", 3, 0);
	Pin("pineapple", 3, 0);
	Pin("tea", 3, 0);
	Pin("rice", 3, 0);
	Pin("grapefruit", 3, 0);
	Pin("tomato", 3, 0);
	Pin("crocodile", 3, 0);
	Pin("coffee", 3, 0);
	Pin("woodpecker", 3, 0);
	Pin("clam", 3, 0);
	Pin("sled", 3, 0);
	Pin("buggy", 3, 1);
	Pin("termite", 3, 0);
	Pin("lettuce", 3, 0);
	Pin("calf", 3, 0);
	Pin("parsley", 3, 0);
	Pin("flounder", 3, 0);
	Pin("jelly", 3, 0);
	Pin("squid", 3, 0);
	Pin("rat", 3, 0);
	Pin("hyacinth", 3, 0);
	Pin("parakeet", 3, 0);
	Pin("nightingale", 3, 0);
	Pin("carriage", 3, 1);
	Pin("pillow", 3, 0);
	Pin("monkey", 3, 0);
	Pin("moose", 3, 0);
	Pin("scallop", 3, 0);
	Pin("boat", 3, 0);
	Pin("goat", 3, 0);
	Pin("cauliflower", 3, 0);
	Pin("motorbike", 3, 1);
	Pin("oyster", 3, 0);
	Pin("leopard", 3, 0);
	Pin("buzzard", 3, 0);
	Pin("snail", 3, 0);
	Pin("sultana", 3, 0);
	Pin("plum", 3, 0);
	Pin("falcon", 3, 0);
	Pin("cake", 3, 0);
	Pin("herring", 3, 0);
	Pin("ketchup", 3, 0);
	Pin("turtle", 3, 0);
	Pin("chocolate", 3, 0);
	Pin("iguana", 3, 0);
	Pin("finger", 3, 0);
	Pin("bacon", 3, 0);
	Pin("melon", 3, 0);
	Pin("garlic", 3, 0);
	Pin("watermelon", 3, 0);
	Pin("champagne", 3, 0);
	Pin("train", 3, 1);
	Pin("prune", 3, 0);
	Pin("cheetah", 3, 0);
	Pin("ear", 3, 0);
	Pin("alligator", 3, 0);
	Pin("raisin", 3, 0);
	Pin("beetle", 3, 0);
	Pin("sugar", 3, 0);
	Pin("walrus", 3, 0);
	Pin("moth", 3, 0);
	Pin("lemon", 3, 0);
	Pin("platypus", 3, 0);
	Pin("broccoli", 3, 0);
	Pin("porsche", 3, 1);
	Pin("squirrel", 3, 0);
	Pin("toe", 3, 0);
	Pin("jam", 3, 0);
	Pin("shrimp", 3, 0);
	Pin("minivan", 3, 1);
	Pin("cloak", 3, 0);
	Pin("lorry", 3, 1);
	Pin("cucumber", 3, 0);
	Pin("worm", 3, 0);
	Pin("bike", 3, 1);
	Pin("winch", 3, 0);
	Pin("frog", 3, 0);
	Pin("butterfly", 3, 0);
	Pin("orange", 3, 0);
	Pin("shark", 3, 0);
	Pin("drum", 3, 0);
	Pin("tugboat", 3, 0);
	Pin("jacket", 3, 0);
	Pin("raven", 3, 0);
	Pin("shawl", 3, 0);
	Pin("dragonfly", 3, 0);
	Pin("cap", 3, 0);
	Pin("scarf", 3, 0);
	Pin("wolf", 3, 0);
	Pin("llama", 3, 0);
	Pin("sunflower", 3, 0);
	Pin("turkey", 3, 0);
	Pin("panther", 3, 0);
	Pin("rhino", 3, 0);
	Pin("moss", 3, 0);
	Pin("cherry", 3, 0);
	Pin("rattlesnake", 3, 0);
	Pin("grape", 3, 0);
	Pin("oak", 3, 0);
	Pin("crayfish", 3, 0);
	Pin("hawk", 3, 0);
	Pin("gown", 3, 0);
	Pin("van", 3, 1);
	Pin("pear", 3, 0);
	Pin("seagull", 3, 0);
	Pin("stockings", 3, 0);
	Pin("apron", 3, 0);
	Pin("limousine", 3, 1);
	Pin("carrot", 3, 0);
	Pin("cod", 3, 0);
	Pin("wheeler", 3, 1);
	Pin("blueberry", 3, 0);
	Pin("cricket", 3, 0);
	Pin("doll", 3, 0);
	Pin("kangaroo", 3, 0);
	Pin("gloves", 3, 0);
	Pin("pony", 3, 0);
	Pin("horse", 3, 0);
	Pin("chipmunk", 3, 0);
	Pin("sparrow", 3, 0);
	Pin("freighter", 3, 0);
	Pin("cow", 3, 0);
	Pin("pigeon", 3, 0);
	Pin("pansy", 3, 0);
	Pin("dress", 3, 0);
	Pin("orchid", 3, 0);
	Pin("partridge", 3, 0);
	Pin("motorcycle", 3, 1);
	Pin("soup", 3, 0);
	Pin("foot", 3, 0);
	Pin("pie", 3, 0);
	Pin("milk", 3, 0);
	Pin("rickshaw", 3, 1);
	Pin("eel", 3, 0);
	Pin("unicycle", 3, 1);
	Pin("mosquito", 3, 0);
	Pin("cart", 3, 1);
	Pin("nut", 3, 0);
	Pin("bean", 3, 0);
	Pin("cockroach", 3, 0);
	Pin("puppet", 3, 0);
	Pin("celery", 3, 0);
	Pin("minnow", 3, 0);
	Pin("seal", 3, 0);
	Pin("tulip", 3, 0);
	Pin("lips", 3, 0);
	Pin("marigold", 3, 0);
	Pin("tobacco", 3, 0);
	Pin("lime", 3, 0);
	Pin("dates", 3, 0);
	Pin("canary", 3, 0);
	Pin("caterpillar", 3, 0);
	Pin("goose", 3, 0);
	Pin("yacht", 3, 0);
	Pin("lily", 3, 0);
	Pin("aeroplane", 3, 1);
	Pin("potato", 3, 0);
	Pin("lion", 3, 0);
	Pin("tricycle", 3, 1);
	Pin("banana", 3, 0);
	Pin("birch", 3, 0);
	Pin("bread", 3, 0);
	Pin("scooter", 3, 1);
	Pin("elm", 3, 0);
	Pin("fir", 3, 0);
	Pin("toad", 3, 0);
	Pin("hair", 3, 0);
	Pin("mayonnaise", 3, 0);
	Pin("cat", 3, 0);
	Pin("centipede", 3, 0);
	Pin("strawberry", 3, 0);
	Pin("radish", 3, 0);
	Pin("trout", 3, 0);
	Pin("starling", 3, 0);
	Pin("onion", 3, 0);
	Pin("tractor", 3, 1);
	Pin("nose", 3, 0);
	Pin("wasp", 3, 0);
	Pin("wheelbarrow", 3, 1);
	Pin("vessel", 3, 0);
	Pin("skirt", 3, 0);
	Pin("heron", 3, 0);
	Pin("tortoise", 3, 0);
	Pin("pig", 3, 0);
	Pin("schooner", 3, 0);
	Pin("octopus", 3, 0);
	Pin("pelican", 3, 0);
	Pin("wheelchair", 3, 1);
	Pin("skunk", 3, 0);
	Pin("lizard", 3, 0);
	Pin("swan", 3, 0);
	Pin("lobster", 3, 0);
	Pin("hamster", 3, 0);
	Pin("duck", 3, 0);
	Pin("dandelion", 3, 0);
	Pin("mushroom", 3, 0);
	Pin("dove", 3, 0);
	Pin("peas", 3, 0);
	Pin("wagon", 3, 1);
	Pin("raspberry", 3, 0);
	Pin("kingfisher", 3, 0);
	Pin("chestnut", 3, 0);
	Pin("coach", 3, 1);
	Pin("shirt", 3, 0);
	Pin("wren", 3, 0);
	Pin("frigate", 3, 0);
	Pin("porcupine", 3, 0);
	Pin("fern", 3, 0);
	Pin("asparagus", 3, 0);
	Pin("ant", 3, 0);
	Pin("artichoke", 3, 0);
	Pin("sweater", 3, 0);
	Pin("daisy", 3, 0);
	Pin("corn", 3, 0);
	Pin("pumpkin", 3, 0);
	Pin("suit", 3, 0);
	Pin("penguin", 3, 0);
	Pin("ox", 3, 0);
	Pin("bear", 3, 0);
	Pin("spinach", 3, 0);
	Pin("eucalyptus", 3, 0);
	Pin("flamingo", 3, 0);
	Pin("tangerine", 3, 0);
	
	// is_dangerous
	Pin("chainsaw", 4, 1);
	Pin("tricycle", 4, 0);
	Pin("panther", 4, 1);
	Pin("wolf", 4, 1);
	Pin("grizzly", 4, 1);
	Pin("syringe", 4, 1);
	Pin("ball", 4, 0);
	Pin("soup", 4, 0);
	Pin("poison", 4, 1);
	Pin("axe", 4, 1);
	Pin("mop", 4, 0);
	Pin("shovel", 4, 0);
	Pin("giraffe", 4, 0);
	Pin("hod", 4, 0);
	Pin("crocodile", 4, 1);
	Pin("crossbow", 4, 1);
	Pin("jellyfish", 4, 1);
	Pin("bullet", 4, 1);
	Pin("gun", 4, 1);
	Pin("methamphetamines", 4, 1);
	Pin("snake", 4, 1);
	Pin("scorpion", 4, 1);
	Pin("hippo", 4, 1);
	Pin("blade", 4, 1);
	Pin("lemur", 4, 0);
	Pin("gorillas", 4, 1);
	Pin("rifle", 4, 1);
	Pin("pitchfork", 4, 1);
	Pin("glove", 4, 0);
	Pin("warthog", 4, 1);
	Pin("harpoon", 4, 1);
	Pin("cleaver", 4, 1);
	Pin("heroin", 4, 1);
	Pin("rattlesnake", 4, 1);
	Pin("cougar", 4, 1);
	Pin("arrow", 4, 1);
	Pin("puppet", 4, 0);
	Pin("elephant", 4, 1);
	Pin("methamphetamine", 4, 1);
	Pin("bomb", 4, 1);
	Pin("tigress", 4, 1);
	Pin("valium", 4, 1);
	Pin("sword", 4, 1);
	Pin("porcupine", 4, 0);
	Pin("weapon", 4, 1);
	Pin("recorder", 4, 0);
	Pin("motorcycle", 4, 1);
	Pin("derringer", 4, 1);
	Pin("antelope", 4, 0);
	Pin("dinosaur", 4, 1);
	Pin("firearm", 4, 1);
	Pin("saw", 4, 1);
	Pin("bayonet", 4, 1);
	Pin("tiger", 4, 1);
	Pin("doll", 4, 0);
	Pin("methadone", 4, 1);
	Pin("cannon", 4, 1);
	Pin("toothbrush", 4, 0);
	Pin("tyrannosaurus", 4, 1);
	Pin("crayon", 4, 0);
	Pin("rhinoceros", 4, 1);
	Pin("cocaine", 4, 1);
	Pin("tapir", 4, 1);
	Pin("lions", 4, 1);
	Pin("hoe", 4, 0);
	Pin("whip", 4, 1);
	Pin("helicopter", 4, 1);
	Pin("broom", 4, 0);
	Pin("otter", 4, 0);
	Pin("tambourine", 4, 0);
	Pin("jaguar", 4, 1);
	Pin("cheetah", 4, 1);
	Pin("steroid", 4, 1);
	Pin("scissors", 4, 1);
	Pin("lion", 4, 1);
	Pin("drug", 4, 1);
	Pin("amphetamine", 4, 1);
	Pin("zebra", 4, 0);
	Pin("rattle", 4, 0);
	Pin("hyena", 4, 1);
	Pin("alligator", 4, 1);
	Pin("razor", 4, 1);
	Pin("slingshot", 4, 1);
	Pin("pistol", 4, 1);
	Pin("viper", 4, 1);
	Pin("blender", 4, 0);
	Pin("goat", 4, 0);
	Pin("tortoise", 4, 0);
	Pin("spade", 4, 0);
	Pin("python", 4, 1);
	Pin("silverback", 4, 1);
	Pin("shotgun", 4, 1);
	Pin("toad", 4, 0);
	Pin("rocket", 4, 1);
	Pin("marble", 4, 0);
	Pin("leopard", 4, 1);
	Pin("turtle", 4, 0);
	Pin("club", 4, 1);
	Pin("handgun", 4, 1);
	Pin("dromedary", 4, 0);
	Pin("rabbit", 4, 0);
	Pin("shark", 4, 1);
	Pin("gazelle", 4, 0);
	Pin("stabbed", 4, 1);
	Pin("axes", 4, 1);
	Pin("monkey", 4, 0);
	Pin("narcotic", 4, 1);
	Pin("kite", 4, 0);
	Pin("bucket", 4, 0);
	Pin("guenon", 4, 0);
	Pin("balloon", 4, 0);
	Pin("stabbing", 4, 1);
	Pin("satchel", 4, 0);
	Pin("spear", 4, 1);
	Pin("plough", 4, 1);
	Pin("camel", 4, 0);
	Pin("knife", 4, 1);
	Pin("hornbill", 4, 0);
	Pin("boomerang", 4, 1);
	Pin("scythe", 4, 1);
	Pin("revolver", 4, 1);
	Pin("tank", 4, 1);
	Pin("swing", 4, 0);
}

void InitNet() {
  long long a, b;
  unsigned long long next_random = 1;

  syn0 = Alloc((long long)vocab_size * layer1_size * sizeof(real), "syn0");
  pins = Alloc((long long)vocab_size * layer1_size * sizeof(real), "pins");
  
  if (hs) {
  	syn1 = Alloc((long long)vocab_size * layer1_size * sizeof(real), "syn1");
    for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++) {
		syn1[a * layer1_size + b] = 0;
	}
  }

  if (negative>0) {
    syn1neg = Alloc((long long)vocab_size * layer1_size * sizeof(real), "syn1neg");
    for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++)
     	syn1neg[a * layer1_size + b] = 0;
  }
  
  // Randomize initial weights
  for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++) {
    next_random = next_random * (unsigned long long)25214903917 + 11;
    syn0[a * layer1_size + b] = (((next_random & 0xFFFF) / (real)65536) - 0.5) / layer1_size;
  }
  
  // Create a binary tree assigning unique codes to each vocabulary word
  CreateBinaryTree();
  
  // Initialize pins and pinned values
  InitPins();
  
  // Test
  printf("IsPinned(%s): %d\n", "husband", IsPinned(SearchVocab("husband")));
  printf("IsPinned(%s): %d\n", "gecko", IsPinned(SearchVocab("gecko")));
  printf("IsPinned(%s): %d\n", "chicago", IsPinned(SearchVocab("chicago")));
  printf("IsPinned(%s): %d\n", "shoe", IsPinned(SearchVocab("shoe")));
  printf("IsPinned(%s): %d\n", "bucket", IsPinned(SearchVocab("bucket")));
}

void *TrainModelThread(void *id) {
  long long a, b, d, cw, word, last_word, sentence_length = 0, sentence_position = 0;
  long long word_count = 0, last_word_count = 0, sen[MAX_SENTENCE_LENGTH + 1];
  long long l1, l2, c, target, label, local_iter = iter;
  unsigned long long next_random = (long long)id;
  real f, g;
  clock_t now;
  real *neu1 = (real *)calloc(layer1_size, sizeof(real));
  real *neu1e = (real *)calloc(layer1_size, sizeof(real));
  FILE *fi = fopen(train_file, "rb");
  fseek(fi, file_size / (long long)num_threads * (long long)id, SEEK_SET);
  while (1) {
    if (word_count - last_word_count > 10000) {
      word_count_actual += word_count - last_word_count;
      last_word_count = word_count;
      if ((debug_mode > 1)) {
        now=clock();
        printf("%cAlpha: %f  Progress: %.2f%%  Words/thread/sec: %.2fk  ", 13, alpha,
         word_count_actual / (real)(iter * train_words + 1) * 100,
         word_count_actual / ((real)(now - start + 1) / (real)CLOCKS_PER_SEC * 1000));
        fflush(stdout);
      }
      alpha = starting_alpha * (1 - word_count_actual / (real)(iter * train_words + 1));
      if (alpha < starting_alpha * 0.0001) alpha = starting_alpha * 0.0001;
    }
    // Read an entire sentence into memory (into sen[] array)
    if (sentence_length == 0) {
      while (1) {
        word = ReadWordIndex(fi);
        if (feof(fi)) break;
        if (word == -1) continue;
        word_count++;
        if (word == 0) break;
        // The subsampling randomly discards frequent words while keeping the ranking same
        if (sample > 0) {
          real ran = (sqrt(vocab[word].count / (sample * train_words)) + 1) * (sample * train_words) / vocab[word].count;
          next_random = next_random * (unsigned long long)25214903917 + 11;
          if (ran < (next_random & 0xFFFF) / (real)65536) continue;
        }
        sen[sentence_length] = word;
        sentence_length++;
        if (sentence_length >= MAX_SENTENCE_LENGTH) break;
      }
      sentence_position = 0;
    }
    if (feof(fi) || (word_count > train_words / num_threads)) {
      word_count_actual += word_count - last_word_count;
      local_iter--;
      if (local_iter == 0) break;
      word_count = 0;
      last_word_count = 0;
      sentence_length = 0;
      fseek(fi, file_size / (long long)num_threads * (long long)id, SEEK_SET);
      continue;
    }
    // get the "center" word (which, in skipgram, we try to predict)
    word = sen[sentence_position];
    if (word == -1) continue;
    // clear accumulators
    for (c = 0; c < layer1_size; c++) neu1[c] = 0;
    for (c = 0; c < layer1_size; c++) neu1e[c] = 0;
    next_random = next_random * (unsigned long long)25214903917 + 11;
    b = next_random % window;
    if (cbow) {  //train the cbow architecture
      // in -> hidden
      cw = 0;
      for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
        c = sentence_position - window + a;
        if (c < 0) continue;
        if (c >= sentence_length) continue;
        last_word = sen[c];
        if (last_word == -1) continue;
        for (c = 0; c < layer1_size; c++) neu1[c] += syn0[c + last_word * layer1_size];
        cw++;
      }
      if (cw) {
        for (c = 0; c < layer1_size; c++) neu1[c] /= cw;
        if (hs) for (d = 0; d < vocab[word].codelen; d++) {
          f = 0;
          l2 = vocab[word].point[d] * layer1_size;
          // Propagate hidden -> output
          for (c = 0; c < layer1_size; c++) f += neu1[c] * syn1[c + l2];
          if (f <= -MAX_EXP) continue;
          else if (f >= MAX_EXP) continue;
          else f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
          // 'g' is the gradient multiplied by the learning rate
          g = (1 - vocab[word].code[d] - f) * alpha;
          // Propagate errors output -> hidden
          for (c = 0; c < layer1_size; c++) neu1e[c] += g * syn1[c + l2];
          // Learn weights hidden -> output
          for (c = 0; c < layer1_size; c++) syn1[c + l2] += g * neu1[c];
        }
        // NEGATIVE SAMPLING
        if (negative > 0) for (d = 0; d < negative + 1; d++) {
          if (d == 0) {
            target = word;
            label = 1;
          } else {
            next_random = next_random * (unsigned long long)25214903917 + 11;
            target = table[(next_random >> 16) % table_size];
            if (target == 0) target = next_random % (vocab_size - 1) + 1;
            if (target == word) continue;
            label = 0;
          }
          l2 = target * layer1_size;
          f = 0;
          for (c = 0; c < layer1_size; c++) f += neu1[c] * syn1neg[c + l2];
          if (f > MAX_EXP) g = (label - 1) * alpha;
          else if (f < -MAX_EXP) g = (label - 0) * alpha;
          else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;
          for (c = 0; c < layer1_size; c++) neu1e[c] += g * syn1neg[c + l2];
          for (c = 0; c < layer1_size; c++) syn1neg[c + l2] += g * neu1[c];
        }
        // hidden -> in
        for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
          c = sentence_position - window + a;
          if (c < 0) continue;
          if (c >= sentence_length) continue;
          last_word = sen[c];
          if (last_word == -1) continue;
          for (c = 0; c < layer1_size; c++) syn0[c + last_word * layer1_size] += neu1e[c];
        }
      }
    } else {  //train skip-gram
      // loop over the window of context words in the sentence
      for (a = b; a < window * 2 + 1 - b; a++) if (a != window) {
        // get the position of the context word in the sentence
        c = sentence_position - window + a;
        if (c < 0) continue;
        if (c >= sentence_length) continue;
        // store the actual context word (as vocabulary index) in last_word
        last_word = sen[c];
        if (last_word == -1) continue;	// (out-of-vocabulary word; ignore)
        // get a pointer into our input layer, thus finding the embedding for last_word
        l1 = last_word * layer1_size;
        
        // if either the target word or the context word contains a pinned value,
        // give it more weight by repeating this training process multiple times
        int repeats = 1;
        if (IsPinned(word) || IsPinned(last_word)) repeats = pinRepeats;
        
        for (int repeat=0; repeat < repeats; repeat++) {
			// clear the error terms corresponding to our hidden layer
			for (c = 0; c < layer1_size; c++) neu1e[c] = 0;
			// HIERARCHICAL SOFTMAX
			if (hs) for (d = 0; d < vocab[word].codelen; d++) {	// ?
			  f = 0;
			  l2 = vocab[word].point[d] * layer1_size;
			  // Propagate hidden -> output
			  for (c = 0; c < layer1_size; c++) f += syn0[c + l1] * syn1[c + l2];
			  if (f <= -MAX_EXP) continue;
			  else if (f >= MAX_EXP) continue;
			  else f = expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))];
			  // 'g' is the gradient multiplied by the learning rate
			  g = (1 - vocab[word].code[d] - f) * alpha;
			  // Propagate errors output -> hidden
			  for (c = 0; c < layer1_size; c++) neu1e[c] += g * syn1[c + l2];
			  // Learn weights hidden -> output
			  for (c = 0; c < layer1_size; c++) syn1[c + l2] += g * syn0[c + l1];
			}
			// NEGATIVE SAMPLING
			if (negative > 0) for (d = 0; d < negative + 1; d++) {
			  if (d == 0) {
				target = word;
				label = 1;
			  } else {
				next_random = next_random * (unsigned long long)25214903917 + 11;
				target = table[(next_random >> 16) % table_size];
				if (target == 0) target = next_random % (vocab_size - 1) + 1;
				if (target == word) continue;
				label = 0;
			  }
			  l2 = target * layer1_size;
			  f = 0;
			  for (c = 0; c < layer1_size; c++) f += syn0[c + l1] * syn1neg[c + l2];
			  if (f > MAX_EXP) g = (label - 1) * alpha;
			  else if (f < -MAX_EXP) g = (label - 0) * alpha;
			  else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;
			  for (c = 0; c < layer1_size; c++) neu1e[c] += g * syn1neg[c + l2];
			  for (c = 0; c < layer1_size; c++) syn1neg[c + l2] += g * syn0[c + l1];
			}
			// Learn weights input -> hidden (thus updating embedding of last_word),
			// gated by our 'pins' array.
			for (c = 0; c < layer1_size; c++) syn0[l1 + c] += neu1e[c] * pins[l1 + c];
			//if (last_word == iKing) printf("Updated iKing(%ld); dim 5 is now %f, pins[%lld]=%f\n", iKing, syn0[l1 + 5], l1 + 5, pins[l1 + 5]);
		
		} // next repeat
        
      } // next context word
    } // end of "if skipgram" (vs CBOW)
    
    sentence_position++;
    if (sentence_position >= sentence_length) {
      sentence_length = 0;
      continue;
    }

  } // next word in file
  
  fclose(fi);
  free(neu1);
  free(neu1e);
#ifdef _MSC_VER
_endthreadex(0);
#elif defined  linux 
  pthread_exit(NULL);
#endif
}

#ifdef _MSC_VER
DWORD WINAPI TrainModelThread_win(LPVOID tid){
	TrainModelThread(tid);
	return 0;
}
#endif

void TrainModel() {
  long a, b, c, d;
  FILE *fo;
  printf("Starting training using file %s\n", train_file);
  starting_alpha = alpha;
  if (read_vocab_file[0] != 0) ReadVocab(); else LearnVocabFromTrainFile();
  if (save_vocab_file[0] != 0) SaveVocab();
  if (output_file[0] == 0) return;
  InitNet();
  if (negative > 0) InitUnigramTable();
  start = clock();
  
#ifdef _MSC_VER
	HANDLE *pt = (HANDLE *)malloc(num_threads * sizeof(HANDLE));
	for (int i = 0; i < num_threads; i++){
		pt[i] = (HANDLE)_beginthreadex(NULL, 0, TrainModelThread_win, (void *)i, 0, NULL);
	}
	WaitForMultipleObjects(num_threads, pt, TRUE, INFINITE);
	for (int i = 0; i < num_threads; i++){
		CloseHandle(pt[i]);
	}
	free(pt);
#elif defined  linux 
  pthread_t *pt = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  for (a = 0; a < num_threads; a++) pthread_create(&pt[a], NULL, TrainModelThread, (void *)a);
  for (a = 0; a < num_threads; a++) pthread_join(pt[a], NULL);
#endif

  fo = fopen(output_file, "wb");
  if (classes == 0) {
    // Save the word vectors
    fprintf(fo, "%lld %lld\n", vocab_size, layer1_size);
    for (a = 0; a < vocab_size; a++) {
      fprintf(fo, "%s ", vocab[a].word);
      if (binary) for (b = 0; b < layer1_size; b++) fwrite(&syn0[a * layer1_size + b], sizeof(real), 1, fo);
      else for (b = 0; b < layer1_size; b++) fprintf(fo, "%lf ", syn0[a * layer1_size + b]);
      fprintf(fo, "\n");
    }
  } else {
    // Run K-means on the word vectors
    int clcn = classes, iter = 10, closeid;
    int *centcn = (int *)malloc(classes * sizeof(int));
    int *cl = (int *)calloc(vocab_size, sizeof(int));
    real closev, x;
    real *cent = (real *)calloc(classes * layer1_size, sizeof(real));
    for (a = 0; a < vocab_size; a++) cl[a] = a % clcn;
    for (a = 0; a < iter; a++) {
      for (b = 0; b < clcn * layer1_size; b++) cent[b] = 0;
      for (b = 0; b < clcn; b++) centcn[b] = 1;
      for (c = 0; c < vocab_size; c++) {
        for (d = 0; d < layer1_size; d++) cent[layer1_size * cl[c] + d] += syn0[c * layer1_size + d];
        centcn[cl[c]]++;
      }
      for (b = 0; b < clcn; b++) {
        closev = 0;
        for (c = 0; c < layer1_size; c++) {
          cent[layer1_size * b + c] /= centcn[b];
          closev += cent[layer1_size * b + c] * cent[layer1_size * b + c];
        }
        closev = sqrt(closev);
        for (c = 0; c < layer1_size; c++) cent[layer1_size * b + c] /= closev;
      }
      for (c = 0; c < vocab_size; c++) {
        closev = -10;
        closeid = 0;
        for (d = 0; d < clcn; d++) {
          x = 0;
          for (b = 0; b < layer1_size; b++) x += cent[layer1_size * d + b] * syn0[c * layer1_size + b];
          if (x > closev) {
            closev = x;
            closeid = d;
          }
        }
        cl[c] = closeid;
      }
    }
    // Save the K-means classes
    for (a = 0; a < vocab_size; a++) fprintf(fo, "%s %d\n", vocab[a].word, cl[a]);
    free(centcn);
    free(cent);
    free(cl);
  }
  fclose(fo);
}

int ArgPos(char *str, int argc, char **argv) {
  int a;
  for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
    if (a == argc - 1) {
      printf("Argument missing for %s\n", str);
      exit(1);
    }
    return a;
  }
  return -1;
}

int main(int argc, char **argv) {
  int i;
  if (argc == 1) {
    printf("WORD VECTOR estimation toolkit v 0.1c\n\n");
    printf("Options:\n");
    printf("Parameters for training:\n");
    printf("\t-train <file>\n");
    printf("\t\tUse text data from <file> to train the model\n");
    printf("\t-output <file>\n");
    printf("\t\tUse <file> to save the resulting word vectors / word clusters\n");
    printf("\t-size <int>\n");
    printf("\t\tSet size of word vectors; default is 100\n");
    printf("\t-window <int>\n");
    printf("\t\tSet max skip length between words; default is 5\n");
    printf("\t-sample <float>\n");
    printf("\t\tSet threshold for occurrence of words. Those that appear with higher frequency in the training data\n");
    printf("\t\twill be randomly down-sampled; default is 1e-3, useful range is (0, 1e-5)\n");
    printf("\t-hs <int>\n");
    printf("\t\tUse Hierarchical Softmax; default is 0 (not used)\n");
    printf("\t-negative <int>\n");
    printf("\t\tNumber of negative examples; default is 5, common values are 3 - 10 (0 = not used)\n");
    printf("\t-threads <int>\n");
    printf("\t\tUse <int> threads (default 12)\n");
    printf("\t-iter <int>\n");
    printf("\t\tRun more training iterations (default 5)\n");
    printf("\t-min-count <int>\n");
    printf("\t\tThis will discard words that appear less than <int> times; default is 5\n");
    printf("\t-alpha <float>\n");
    printf("\t\tSet the starting learning rate; default is 0.025 for skip-gram and 0.05 for CBOW\n");
    printf("\t-classes <int>\n");
    printf("\t\tOutput word classes rather than word vectors; default number of classes is 0 (vectors are written)\n");
    printf("\t-debug <int>\n");
    printf("\t\tSet the debug mode (default = 2 = more info during training)\n");
    printf("\t-binary <int>\n");
    printf("\t\tSave the resulting vectors in binary moded; default is 0 (off)\n");
    printf("\t-save-vocab <file>\n");
    printf("\t\tThe vocabulary will be saved to <file>\n");
    printf("\t-read-vocab <file>\n");
    printf("\t\tThe vocabulary will be read from <file>, not constructed from the training data\n");
    printf("\t-cbow <int>\n");
    printf("\t\tUse the continuous bag of words model; default is 1 (use 0 for skip-gram model)\n");
    printf("\t-pin <int>\n");
    printf("\t\tPin certain words/features; default is 0 (use 1 to pin)\n");
    printf("\t-pin-repeats <int>\n");
    printf("\t\tNumber of times to repeat training examples involving pinned words (default = 1)\n");
    
    printf("\nExamples:\n");
    printf("./word2vec -train data.txt -output vec.txt -size 200 -window 5 -sample 1e-4 -negative 5 -hs 0 -binary 0 -cbow 1 -iter 3\n\n");
    return 0;
  }
  output_file[0] = 0;
  save_vocab_file[0] = 0;
  read_vocab_file[0] = 0;
  if ((i = ArgPos((char *)"-size", argc, argv)) > 0) layer1_size = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-train", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0) strcpy(save_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0) strcpy(read_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-cbow", argc, argv)) > 0) cbow = atoi(argv[i + 1]);
  if (cbow) alpha = 0.05;
  if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0) alpha = atof(argv[i + 1]);
  if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-window", argc, argv)) > 0) window = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-sample", argc, argv)) > 0) sample = atof(argv[i + 1]);
  if ((i = ArgPos((char *)"-hs", argc, argv)) > 0) hs = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-negative", argc, argv)) > 0) negative = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-threads", argc, argv)) > 0) num_threads = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-iter", argc, argv)) > 0) iter = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0) min_count = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-classes", argc, argv)) > 0) classes = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-pin", argc, argv)) > 0) optPin = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-pin-repeats", argc, argv)) > 0) pinRepeats = atoi(argv[i + 1]);

  printf("Training mode: %s", cbow ? "CBOW" : "SkipGram");
  if (optPin) printf(" with pinned words; pin-repeats = %d", pinRepeats);
  printf("\n");

  vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
  vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
  expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
  for (i = 0; i < EXP_TABLE_SIZE; i++) {
    expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
    expTable[i] = expTable[i] / (expTable[i] + 1);                   // Precompute f(x) = x / (x + 1)
  }
  TrainModel();
  return 0;
}

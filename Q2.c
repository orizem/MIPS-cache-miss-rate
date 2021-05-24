#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

struct BinAddress {
	int* bin_tag;
	int* bin_set;
	int* bin_offset;

	int tagBits;
	int setBits;
	int offsetBits;
}typedef binAddress;

struct Associative_Way {
	int** bin_tag;
	int** bin_set;
	int** bin_offset;

	int* tagBits;
	int* setBits;
	int* offsetBits;

	int* validity;
	int* LRU;
}typedef associative_way;

float cacheSimulator(int cacheSize, int associativity, int blockSize, char* fileName);
void setLRU(associative_way* set, int associativity, int index);

float missRatio(float miss, float accesses);
binAddress* setAddress(int* binaryNum, int cacheSize, int blockSize, int associativity);
int isEqualBinNum(int* bin1, int size1, int* bin2, int size2);
void setCacheBlock(associative_way* cache, int index, binAddress* address);

int* decToBinary(unsigned int number, int sizeOfBinNum);
int binaryToDec(int* binaryNum, int sizeOfBinNum);
int CheckFile(FILE* file);

void main(int argc, char* argv[]) {
	if (argc != 5) {
		printf("Error!\nNot enugh arguments were entered.\nYou have entered %d instead of 5 arguments.\n", argc);
		exit(0);
	}

	else {
		printf("%s %s %s %s\n", argv[1], argv[2], argv[3], argv[4]);
		printf("Miss Rate: %g\n", cacheSimulator(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), argv[4]));
	}
}

float cacheSimulator(int cacheSize, int associativity, int blockSize, char* fileName) {
	associative_way* cache;
	FILE* file;
	binAddress* currentBinAddress;
	float numberOfMisses = 0.0f, numberOfAccesses = 0.0f, numberOfHits = 0.0f;
	unsigned int decimal_byte_address = 0;

	cache = (associative_way*)calloc(cacheSize / (blockSize * associativity), sizeof(associative_way));

	for (int i = 0; i < (cacheSize / (blockSize * associativity)); i++) {
		cache[i].bin_offset = (int**)calloc(associativity, sizeof(int*));
		cache[i].bin_set = (int**)calloc(associativity, sizeof(int*));
		cache[i].bin_tag = (int**)calloc(associativity, sizeof(int*));
		cache[i].offsetBits = (int*)calloc(associativity, sizeof(int));
		cache[i].setBits = (int*)calloc(associativity, sizeof(int));
		cache[i].tagBits = (int*)calloc(associativity, sizeof(int));
		cache[i].LRU = (int*)calloc(associativity, sizeof(int));
		cache[i].validity = (int*)calloc(associativity, sizeof(int));
	}

	//Open the file and check if it was readen correctly.
	file = fopen(fileName, "r");
	if (!CheckFile(file))
		return 0.0f;

	while (!feof(file)) {
		if (fscanf(file, "%u", &decimal_byte_address) != EOF) {
			currentBinAddress = setAddress(decToBinary(decimal_byte_address, 32), cacheSize, blockSize, associativity);

			int indexCacheAddress = binaryToDec(currentBinAddress->bin_set, currentBinAddress->setBits);
			int i = 0;

			//Check hit
			for (i = 0; i < associativity; i++) {
				if (isEqualBinNum(currentBinAddress->bin_tag, currentBinAddress->tagBits, cache[indexCacheAddress].bin_tag[i], cache[indexCacheAddress].tagBits[i])) {
					numberOfHits++;
					break;
				}
			}

			//Check if is miss
			if (i == associativity) {
				for (i = 0; i < associativity; i++) {
					if (cache[indexCacheAddress].validity[i] == 0) {
						setCacheBlock(&cache[indexCacheAddress], i, currentBinAddress);
						break;
					}
				}

				//In case all the values are with validity 1, check LRU
				if (i == associativity) {
					for (i = 0; i < associativity; i++) {
						if (cache[indexCacheAddress].LRU[i] == 0) {
							setCacheBlock(&cache[indexCacheAddress], i, currentBinAddress);
							break;
						}
					}
				}

				numberOfMisses++;
			}
			
			setLRU(&cache[indexCacheAddress], associativity, i);

			numberOfAccesses++;
		}
	}

	fclose(file);

	return missRatio(numberOfMisses, numberOfAccesses);
}

void setLRU(associative_way* set, int associativity, int index) {
	int highestLRU = 0, x;

	if (associativity == 1)
		return;

	x = set->LRU[index];

	for (int i = 0; i < associativity; i++) {
		if (set->LRU[i] > highestLRU)
			highestLRU = set->LRU[i];
	}

	if (highestLRU < associativity - 1)
		set->LRU[index] = highestLRU + 1;

	else {
		set->LRU[index] = associativity - 1;

		for (int j = 0; j < associativity; j++) {
			if (index != j && set->LRU[j] > x)
				set->LRU[j]--;
		}
	}
}

float missRatio(float miss, float accesses) {
	return miss / accesses;
}

binAddress* setAddress(int* binaryNum, int cacheSize, int blockSize, int associativity) {
	binAddress* _address;
	int numOfLines = 0, numOfSets = 0, offsetBits = 0, setBits = 0, tagBits = 0;
	int* bin_tag, * bin_set, * bin_offset;

	_address = (binAddress*)calloc(1, sizeof(binAddress));

	numOfLines = cacheSize / blockSize;

	numOfSets = numOfLines / associativity;
	setBits = (int)log2(numOfSets);

	offsetBits = (int)log2(blockSize);
	tagBits = 32 - offsetBits - setBits;

	bin_tag = (int*)calloc(tagBits, sizeof(int));
	bin_set = (int*)calloc(setBits, sizeof(int));
	bin_offset = (int*)calloc(offsetBits, sizeof(int));

	for (int i = 0; i < tagBits; i++)
		bin_tag[i] = binaryNum[i];

	for (int i = tagBits; i < tagBits + setBits; i++)
		bin_set[i - tagBits] = binaryNum[i];

	for (int i = tagBits + setBits; i < blockSize; i++)
		bin_offset[i - tagBits - setBits] = binaryNum[i];

	_address->tagBits = tagBits;
	_address->setBits = setBits;
	_address->offsetBits = offsetBits;

	_address->bin_tag = bin_tag;
	_address->bin_set = bin_set;
	_address->bin_offset = bin_offset;

	return _address;
}

int isEqualBinNum(int* bin1, int size1, int* bin2, int size2) {
	if (size1 != size2)
		return 0;

	for (int i = 0; i < size1; i++)
	{
		if (bin1[i] != bin2[i])
			return 0;
	}

	return 1;
}

void setCacheBlock(associative_way* cache, int index, binAddress* address) {
	cache->bin_tag[index] = address->bin_tag;
	cache->bin_set[index] = address->bin_set;
	cache->bin_offset[index] = address->bin_offset;

	cache->tagBits[index] = address->tagBits;
	cache->setBits[index] = address->setBits;
	cache->offsetBits[index] = address->offsetBits;

	cache->validity[index] = 1;
}

// function to convert decimal to binary 
int* decToBinary(unsigned int number, int blockSize)
{
	// array to store binary number 
	int* binaryNum = (int*)calloc(blockSize, sizeof(int));

	// counter for binary array 
	int i = blockSize - 1;
	while (number > 0) {

		// storing remainder in binary array 
		binaryNum[i] = number % 2;
		number = number / 2;
		i--;
	}

	return binaryNum;
}

// function to convert binary to decimal 
int binaryToDec(int* binaryNum, int blockSize)
{
	int num = 0, j = blockSize - 1, i = 0;

	while (i < blockSize) {
		num += (int)(binaryNum[i++] * pow(2, j--));
	}

	return num;
}

//Check if the file was readen correctly.
int CheckFile(FILE* file) {
	if (file == NULL) { //file does not exists.
		printf("\nError opening file\n");
		return 0;
	}
	return 1;
}
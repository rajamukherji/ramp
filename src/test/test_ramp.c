#include "../ramp.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

#ifdef DEBUG
#define OUTER 2
#define INNER 160
#else
#define OUTER 16000
#define INNER 16000
#endif

typedef struct block_t block_t;

struct block_t {
	block_t *Next;
	size_t Size;
	char Chars[];
};

//#define COMPARE_MALLOC

static void *thread_fn(void *Arg) {
	printf("Starting thread...\n");
#ifndef COMPARE_MALLOC
	ramp_t *Ramp = ramp_new((uintptr_t)Arg);
#endif
	size_t Size = 1;
	for (int J = 0; J < OUTER; ++J) {
		block_t *Blocks = NULL;
		for (int I = 0; I < INNER; ++I) {
			size_t Size = 1 + (Size * 21367) % 1024;
#ifdef COMPARE_MALLOC
			block_t *Block = malloc(sizeof(block_t) + Size);
#else
			block_t *Block = ramp_alloc(Ramp, sizeof(block_t) + Size);
#endif
#ifdef DEBUG
			char Char = 'A' + Size % 26;
			memset(Block->Chars, Char, Size);
#endif
			Block->Size = Size;
			Block->Next = Blocks;
			Blocks = Block;
		}
#ifdef DEBUG
		for (block_t *Block = Blocks; Block; Block = Block->Next) {
			char Char = Block->Chars[0];
			for (int I = 1; I < Block->Size; ++I) {
				if (Char != Block->Chars[I]) asm("int3");
			}
		}
#endif
#ifdef COMPARE_MALLOC
		for (block_t *Block = Blocks; Block;) {
			block_t *Next = Block->Next;
			free(Block);
			Block = Next;
		}
#else
		ramp_clear(Ramp);
#endif
	}
	ramp_free(Ramp);
	printf("Finished thread.\n");
}


int main(int Argc, char **Argv) {
#ifdef DEBUG
	size_t PageSize = 1 << 9;
#else
	size_t PageSize = 1 << 16;
#endif
	if (Argc > 1) PageSize = atoi(Argv[1]) ?: (1 << 16);
	pthread_t Threads[8];
	for (int I = 0; I < 8; ++I) pthread_create(Threads + I, NULL, (void *)thread_fn, (void *)(uintptr_t)PageSize);
	void *Return;
	for (int I = 0; I < 8; ++I) pthread_join(Threads[I], &Return);
	return 0;
}

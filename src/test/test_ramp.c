#include "../ramp.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

int main(int Argc, char **Argv) {
#ifdef DEBUG
	size_t PageSize = 1 << 9;
#else
	size_t PageSize = 1 << 16;
#endif
	if (Argc > 1) PageSize = atoi(Argv[1]) ?: (1 << 16);
#ifndef COMPARE_MALLOC
	ramp_t *Ramp = ramp_new(PageSize);
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
#ifndef COMPARE_MALLOC
	ramp_free(Ramp);
#endif
	return 0;
}

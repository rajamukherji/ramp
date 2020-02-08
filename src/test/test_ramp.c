#include "../ramp.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef DEBUG
#define OUTER 2
#define INNER 160
#else
#define OUTER 1600
#define INNER 16000
#endif

typedef struct block_t block_t;

struct block_t {
	block_t *Next;
	size_t Size;
	char Chars[];
};

#define COMPARE_MALLOC

int main(int Argc, char **Argv) {
	ramp_t *Ramp = ramp_new(1 << 16);
	srand(time(0));
	for (int J = 0; J < OUTER; ++J) {
		block_t *Blocks = NULL;
		for (int I = 0; I < INNER; ++I) {
			size_t Size = 1 + rand() % 1024;
#ifdef COMPARE_MALLOC
			block_t *Block = malloc(sizeof(block_t) + Size);
#else
			block_t *Block = ramp_alloc(Ramp, sizeof(block_t) + Size);
#endif
			char Char = 'A' + Size % 26;
			Block->Size = Size;
			Block->Next = Blocks;
			Blocks = Block;
			memset(Block->Chars, Char, Size);
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
	return 0;
}

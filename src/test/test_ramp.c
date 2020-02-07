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
#define INNER 1600
#endif

int main(int Argc, char **Argv) {
	ramp_t *Ramp = ramp_new(512);
	srand(time(0));
	for (int J = 0; J < OUTER; ++J) {
		for (int I = 0; I < INNER; ++I) {
			size_t Size = 1 + rand() % 1024;
			void *Block = ramp_alloc(Ramp, Size);
			memset(Block, 0, Size);
		}
		ramp_clear(Ramp);
	}
	ramp_free(Ramp);
	return 0;
}

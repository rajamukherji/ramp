#include "ramp.h"

typedef struct ramp_page_t ramp_page_t;
typedef struct ramp_defer_t ramp_defer_t;

struct ramp_page_t {
	ramp_page_t *Next;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp_defer_t {
	ramp_defer_t *Next;
	void (*CleanupFn)(void *);
};

struct ramp_t {
	ramp_page_t *Pages, *Full;
	ramp_defer_t *Defers;
	size_t PageSize, Space;
};

ramp_t *ramp_new(size_t PageSize) {
	PageSize += 15;
	PageSize &= ~15;
	ramp_page_t *Page = (ramp_page_t *)malloc(sizeof(ramp_page_t) + PageSize);
	Page->Next = 0;
	ramp_t *Ramp = (ramp_t *)malloc(sizeof(ramp_t));
	Ramp->Pages = Page;
	Ramp->Full = NULL;
	Ramp->Defers = NULL;
	Ramp->PageSize = PageSize;
	Ramp->Space = PageSize;
	return Ramp;
}

static void ramp_defer_free(void **Slot) {
	free(Slot[0]);
}

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

char *ramp_alloc(ramp_t *Ramp, size_t Size) {
	Size += 15;
	Size &= ~15;
	if (likely(Size <= Ramp->Space)) {
		Ramp->Space -= Size;
		return Ramp->Pages->Bytes + Ramp->Space;
	} else if (Size <= Ramp->PageSize) {
		ramp_page_t *Old = Ramp->Pages;
		ramp_page_t *New = Old->Next;
		Old->Next = Ramp->Full;
		Ramp->Full = Old;
		if (!New) {
			New = (ramp_page_t *)malloc(sizeof(ramp_page_t) + Ramp->PageSize);
			New->Next = NULL;
		}
		Ramp->Pages = New;
		Ramp->Space = Ramp->PageSize - Size;
		return New->Bytes + Ramp->Space;
	} else {
		char **Slot = (char **)ramp_defer(Ramp, sizeof(char *), (char *)ramp_defer_free);
		char *Bytes = Slot[0] = malloc(Size);
		return Bytes;
	}
}

char *ramp_defer(ramp_t *Ramp, size_t Size, void (*CleanupFn)(char *)) {
	ramp_defer_t *Defer = ramp_alloc(Ramp, sizeof(ramp_defer_t) + Size);
	Defer->Next = Ramp->Defers;
	Defer->CleanupFn = CleanupFn;
	Ramp->Defers = Defer;
	return Defer + 1;
}

void ramp_clear(ramp_t *Ramp) {
	for (ramp_defer_t *Defer = Ramp->Defers; Defer; Defer = Defer->Next) Defer->CleanupFn(Defer + 1);
	Ramp->Defers = NULL;
	ramp_page_t **Slot = &Ramp->Pages->Next;
	while (Slot[0]) Slot = &Slot[0]->Next;
	Slot[0] = Ramp->Full;
	Ramp->Full = NULL;
	Ramp->Space = Ramp->PageSize;
}

void ramp_reset(ramp_t *Ramp) {
	for (ramp_defer_t *Defer = Ramp->Defers; Defer; Defer = Defer->Next) Defer->CleanupFn(Defer + 1);
	Ramp->Defers = NULL;
	ramp_page_t *Old = Ramp->Pages;
	for (ramp_page_t *Page = Old->Next, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	Old->Next = NULL;
	for (ramp_page_t *Page = Ramp->Full, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	Ramp->Full = NULL;
	Ramp->Space = Ramp->PageSize;
}

void ramp_free(ramp_t *Ramp) {
	ramp_reset(Ramp);
	free(Ramp->Pages);
	free(Ramp);
}

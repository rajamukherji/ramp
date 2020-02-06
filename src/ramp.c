#include "ramp.h"

typedef struct ramp_page_t ramp_page_t;
typedef struct ramp_defer_t ramp_defer_t;

struct ramp_page_t {
	ramp_page_t *Next;
	void *FreeBytes;
	size_t FreeSpace;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp_defer_t {
	ramp_defer_t *Next;
	void (*CleanupFn)(void *);
};

struct ramp_t {
	ramp_page_t *Pages;
	ramp_defer_t *Defers;
	size_t PageSize;
};

ramp_t *ramp_new(size_t PageSize) {
	ramp_t *Ramp = (ramp_t *)malloc(sizeof(ramp_t));
	Ramp->Pages = NULL;
	Ramp->Defers = NULL;
	Ramp->PageSize = (PageSize + 15) & ~15;
	return Ramp;
}

static void ramp_defer_free(void **Slot) {
	free(Slot[0]);
}

void *ramp_alloc(ramp_t *Ramp, size_t Size) {
	Size += 15;
	Size &= ~15;
	if (Size >= Ramp->PageSize) {
		void **Slot = (void **)ramp_defer(Ramp, sizeof(void *), ramp_defer_free);
		void *Bytes = Slot[0] = malloc(Size);
		return Bytes;
	} else {
		 for (ramp_page_t **Slot = &Ramp->Pages, *Page = *Slot; Page; Page = *(Slot = &Page->Next)) {
			 if (Page->FreeSpace >= Size) {
				 void *Bytes = Page->FreeBytes;
				 Page->FreeSpace -= Size;
				 Page->FreeBytes += Size;
				 Slot[0] = Page->Next;
				 Slot = &Ramp->Pages;
				 while (Slot[0] && Slot[0]->FreeSpace < Page->FreeSpace) Slot = &Slot[0]->Next;
				 Page->Next = Slot[0];
				 Slot[0] = Page;
				 return Bytes;
			 }
		 }
		 ramp_page_t *Page = (ramp_page_t *)malloc(sizeof(ramp_page_t) + Ramp->PageSize);
		 Page->FreeBytes = Page->Bytes + Size;
		 Page->FreeSpace = Ramp->PageSize - Size;
		 ramp_page_t **Slot = &Ramp->Pages;
		 while (Slot[0] && Slot[0]->FreeSpace < Page->FreeSpace) Slot = &Slot[0]->Next;
		 Page->Next = Slot[0];
		 Slot[0] = Page;
		 return Page->Bytes;
	}
}

void *ramp_defer(ramp_t *Ramp, size_t Size, void (*CleanupFn)(void *)) {
	ramp_defer_t *Defer = ramp_alloc(Ramp, sizeof(ramp_defer_t) + Size);
	Defer->Next = Ramp->Defers;
	Defer->CleanupFn = CleanupFn;
	Ramp->Defers = Defer;
	return Defer + 1;
}

void ramp_clear(ramp_t *Ramp) {
	for (ramp_defer_t *Defer = Ramp->Defers; Defer; Defer = Defer->Next) Defer->CleanupFn(Defer + 1);
	size_t FreeSpace = Ramp->PageSize;
	for (ramp_page_t *Page = Ramp->Pages; Page; Page = Page->Next) Page->FreeSpace = FreeSpace;
	Ramp->Defers = NULL;
}

void ramp_reset(ramp_t *Ramp) {
	for (ramp_defer_t *Defer = Ramp->Defers; Defer; Defer = Defer->Next) Defer->CleanupFn(Defer + 1);
	for (ramp_page_t *Page = Ramp->Pages, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	Ramp->Defers = NULL;
	Ramp->Pages = NULL;
}

void ramp_free(ramp_t *Ramp) {
	ramp_reset(Ramp);
	free(Ramp);
}

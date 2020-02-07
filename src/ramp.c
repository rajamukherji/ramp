#include "ramp.h"

typedef struct ramp_page_t ramp_page_t;
typedef struct ramp_defer_t ramp_defer_t;

struct ramp_page_t {
	ramp_page_t *Next, *Sibling;
	void *FreeBytes;
	size_t FreeSpace;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp_defer_t {
	ramp_defer_t *Next;
	void (*CleanupFn)(void *);
};

struct ramp_t {
	ramp_page_t *Pages, *Full;
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
#ifdef DEBUG
	printf("[%d]", Size);
	for (ramp_page_t *Page = Ramp->Pages; Page; Page = Page->Next) {
		printf(" (");
		for (ramp_page_t *Sibling = Page; Sibling; Sibling = Sibling->Sibling) {
			printf(" %d", Sibling->FreeSpace);
		}
		printf(" )");
	}
	printf("\n");
#endif
	if (Size >= Ramp->PageSize) {
		void **Slot = (void **)ramp_defer(Ramp, sizeof(void *), ramp_defer_free);
		void *Bytes = Slot[0] = malloc(Size);
		return Bytes;
	} else if (!Ramp->Pages) {
		ramp_page_t *Page = (ramp_page_t *)malloc(sizeof(ramp_page_t) + Ramp->PageSize);
		Page->FreeBytes = Page->Bytes + Size;
		Page->FreeSpace = Ramp->PageSize - Size;
		Page->Next = Page->Sibling = NULL;
		Ramp->Pages = Page;
		return Page->Bytes;
	} else {
		ramp_page_t *Prev = NULL;
		for (ramp_page_t **Slot = &Ramp->Pages, *Page = *Slot; Page; Prev = Page, Page = *(Slot = &Page->Next)) {
			if (Page->FreeSpace >= Size) {
				void *Bytes = Page->FreeBytes;
				Page->FreeBytes = Bytes + Size;
				Page->FreeSpace -= Size;
				if (!Page->FreeSpace) {
					if (Page->Sibling) {
						Page->Sibling->Next = Page->Next;
						Slot[0] = Page->Sibling;
					} else {
						Slot[0] = Page->Next;
					}
					Page->Next = Ramp->Full;
					Page->Sibling = NULL;
					Ramp->Full = Page;
				} else if (Prev && Prev->FreeSpace == Page->FreeSpace) {
					if (Page->Sibling) {
						Page->Sibling->Next = Page->Next;
						Prev->Next = Page->Sibling;
					} else {
						Prev->Next = Page->Next;
					}
					Page->Sibling = Prev->Sibling;
					Prev->Sibling = Page;
				} else if (Page->Sibling) {
					Page->Sibling->Next = Page->Next;
					Page->Next = Page->Sibling;
					Page->Sibling = NULL;
				}
				return Bytes;
			}
		}
		ramp_page_t *Page = (ramp_page_t *)malloc(sizeof(ramp_page_t) + Ramp->PageSize);
		Page->FreeBytes = Page->Bytes + Size;
		Page->FreeSpace = Ramp->PageSize - Size;
		for (ramp_page_t **Slot = &Ramp->Pages, *Prev = *Slot; ; Prev = *(Slot = &Prev->Next)) {
			if (!Prev) {
				Page->Next = Page->Sibling = NULL;
				Slot[0] = Page;
				break;
			} else if (Prev->FreeSpace == Page->FreeSpace) {
				Page->Sibling = Prev->Sibling;
				Page->Next = NULL;
				Prev->Sibling = Page;
				break;
			} else if (Prev->FreeSpace > Page->FreeSpace) {
				Page->Sibling = NULL;
				Page->Next = Prev;
				Slot[0] = Page;
				break;
			}
		}
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
	ramp_page_t *Pages = NULL;
	for (ramp_page_t *Page = Ramp->Pages; Page;) {
		ramp_page_t *Next = Page->Next;
		for (ramp_page_t *Sibling = Page->Sibling; Sibling;) {
			Sibling->Next = Pages;
			Pages = Sibling;
			Sibling = Sibling->Sibling;
		}
		Page->Next = Pages;
		Pages = Page;
		Page = Next;
	}
	for (ramp_page_t *Page = Ramp->Full; Page;) {
		ramp_page_t *Next = Page->Next;
		Page->Next = Pages;
		Pages = Page;
		Page = Next;
	}
	for (ramp_page_t *Page = Pages; Page; Page = Page->Next) {
		Page->FreeSpace = Ramp->PageSize;
		Page->FreeBytes = Page->Bytes;
		Page->Sibling = NULL;
	}
	Ramp->Pages = Pages;
	Ramp->Full = NULL;
	Ramp->Defers = NULL;
}

void ramp_reset(ramp_t *Ramp) {
	for (ramp_defer_t *Defer = Ramp->Defers; Defer; Defer = Defer->Next) Defer->CleanupFn(Defer + 1);
	for (ramp_page_t *Page = Ramp->Pages, *Next; Page; Page = Next) {
		Next = Page->Next;
		for (ramp_page_t *Sibling = Page->Sibling, *Sibling2; Sibling; Sibling = Sibling2) {
			Sibling2 = Sibling->Sibling;
			free(Sibling);
		}
		free(Page);
	}
	for (ramp_page_t *Page = Ramp->Full, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	Ramp->Defers = NULL;
	Ramp->Pages = NULL;
	Ramp->Full = NULL;
}

void ramp_free(ramp_t *Ramp) {
	ramp_reset(Ramp);
	free(Ramp);
}

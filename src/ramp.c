#include "ramp.h"

#include <string.h>

typedef struct ramp_page_t ramp_page_t;

struct ramp_page_t {
	ramp_page_t *Next;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp_deferral_t {
	ramp_deferral_t *Next;
	void (*Callback)(void *);
	void *Arg;
};

struct ramp_t {
	ramp_page_t *Pages, *Full;
	ramp_deferral_t *Deferrals;
	size_t PageSize, Space;
};

static inline ramp_page_t *ramp_page_new(size_t PageSize) {
	ramp_page_t *Page = (ramp_page_t *)malloc(sizeof(ramp_page_t) + PageSize);
	Page->Next = NULL;
	return Page;
}

static inline void ramp_page_free(ramp_page_t *Page) {
	free(Page);
}

ramp_t *ramp_new(size_t PageSize) {
	PageSize += 15;
	PageSize &= ~15;
	ramp_page_t *Page = ramp_page_new(PageSize);
	ramp_t *Ramp = (ramp_t *)malloc(sizeof(ramp_t));
	Ramp->Pages = Page;
	Ramp->Full = NULL;
	Ramp->Deferrals = NULL;
	Ramp->PageSize = PageSize;
	Ramp->Space = PageSize;
	return Ramp;
}

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

void *ramp_alloc(ramp_t *Ramp, size_t Size) {
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
		if (!New) New = ramp_page_new(Ramp->PageSize);
		Ramp->Pages = New;
		Ramp->Space = Ramp->PageSize - Size;
		return New->Bytes + Ramp->Space;
	} else {
		void *Bytes = malloc(Size);
		ramp_defer(Ramp, free, Bytes);
		return Bytes;
	}
}

void *ramp_strdup(ramp_t *Ramp, const char *String) {
	size_t Length = strlen(String);
	char *Copy = ramp_alloc(Ramp, Length + 1);
	memcpy(Copy, String, Length);
	Copy[Length] = 0;
	return Copy;
}

ramp_deferral_t *ramp_defer(ramp_t *Ramp, void (*Callback)(void *), void *Arg) {
	ramp_deferral_t *Deferral = (ramp_deferral_t *)ramp_alloc(Ramp, sizeof(ramp_deferral_t));
	Deferral->Next = Ramp->Deferrals;
	Deferral->Callback = Callback;
	Deferral->Arg = Arg;
	Ramp->Deferrals = Deferral;
	return Deferral;
}

static void ramp_defer_nop(void *Arg) {}

void ramp_cancel(ramp_deferral_t *Deferral) {
	Deferral->Callback = ramp_defer_nop;
}

void ramp_clear(ramp_t *Ramp) {
	for (ramp_deferral_t *Deferral = Ramp->Deferrals; Deferral; Deferral = Deferral->Next) Deferral->Callback(Deferral->Arg);
	Ramp->Deferrals = NULL;
	ramp_page_t **Slot = &Ramp->Pages->Next;
	while (Slot[0]) Slot = &Slot[0]->Next;
	Slot[0] = Ramp->Full;
	Ramp->Full = NULL;
	Ramp->Space = Ramp->PageSize;
}

void ramp_reset(ramp_t *Ramp) {
	for (ramp_deferral_t *Deferral = Ramp->Deferrals; Deferral; Deferral = Deferral->Next) Deferral->Callback(Deferral->Arg);
	Ramp->Deferrals = NULL;
	ramp_page_t *Old = Ramp->Pages;
	for (ramp_page_t *Page = Old->Next, *Next; Page; Page = Next) {
		Next = Page->Next;
		ramp_page_free(Page);
	}
	Old->Next = NULL;
	for (ramp_page_t *Page = Ramp->Full, *Next; Page; Page = Next) {
		Next = Page->Next;
		ramp_page_free(Page);
	}
	Ramp->Full = NULL;
	Ramp->Space = Ramp->PageSize;
}

void ramp_free(ramp_t *Ramp) {
	ramp_reset(Ramp);
	ramp_page_free(Ramp->Pages);
	free(Ramp);
}

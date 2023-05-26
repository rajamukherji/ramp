#include "ramp2.h"

#include <string.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stddef.h>

typedef struct ramp2_page_t ramp2_page_t;

struct ramp2_page_t {
	ramp2_page_t *Next;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp2_deferral_t {
	ramp2_deferral_t *Next;
	ramp2_defer_fn Callback;
	void *Arg;
};

struct ramp2_group_t {
	ramp2_page_t * _Atomic Free;
	size_t PageSize;
};

struct ramp2_t {
	ramp2_group_t *Group;
	ramp2_page_t *Page, *Full;
	ramp2_deferral_t *Deferrals;
	size_t PageSize, Space;
};

ramp2_group_t *ramp2_group_new(size_t PageSize) {
	PageSize += 15;
	PageSize &= ~15;
	ramp2_group_t *Group = (ramp2_group_t *)malloc(sizeof(ramp2_group_t));
	Group->Free = NULL;
	Group->PageSize = PageSize;
	return Group;
}

void ramp2_group_reset(ramp2_group_t *Group) {
	ramp2_page_t *Old = atomic_exchange(&Group->Free, NULL);
	if (!Old) return;
	for (ramp2_page_t *Page = Old->Next, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
}

void ramp2_group_trim(ramp2_group_t *Group, int Count) {
	if (Count <= 0) return ramp2_group_reset(Group);
	ramp2_page_t *Old = atomic_exchange(&Group->Free, NULL);
	if (!Old) return;
	ramp2_page_t *First = Old, *Last;
	while (--Count > 0) {
		Last = Old;
		Old = Old->Next;
		if (!Old) break;
	}
	for (ramp2_page_t *Page = Old, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	ramp2_page_t *Free = Group->Free;
	do {
		Last->Next = Free;
	} while (!atomic_compare_exchange_weak(&Group->Free, &Free, First));
}

void ramp2_group_free(ramp2_group_t *Group) {
	ramp2_page_t *Page = Group->Free;
	while (Page) {
		ramp2_page_t *Next = Page->Next;
		free(Page);
		Page = Next;
	}
	free(Group);
}

static inline ramp2_page_t *ramp2_group_page_new(ramp2_group_t *Group) {
	ramp2_page_t *Page = Group->Free, *Free;
	do {
		if (!Page) {
			Page = (ramp2_page_t *)malloc(Group->PageSize);
			break;
		}
		Free = Page->Next;
	} while (!atomic_compare_exchange_weak(&Group->Free, &Page, Free));
	Page->Next = NULL;
	return Page;
}

ramp2_t *ramp2_new(ramp2_group_t *Group) {
	ramp2_t *Ramp = (ramp2_t *)malloc(sizeof(ramp2_t));
	Ramp->Group = Group;
	Ramp->Page = ramp2_group_page_new(Group);
	Ramp->Full = NULL;
	Ramp->Deferrals = NULL;
	Ramp->Space = Ramp->PageSize = Group->PageSize - offsetof(ramp2_page_t, Bytes);
	return Ramp;
}

#define likely(x)    __builtin_expect (!!(x), 1)
#define unlikely(x)  __builtin_expect (!!(x), 0)

void *ramp2_alloc(ramp2_t *Ramp, size_t Size) {
	Size += 15;
	Size &= ~15;
	if (likely(Size <= Ramp->Space)) {
		Ramp->Space -= Size;
		return Ramp->Page->Bytes + Ramp->Space;
	} else if (Size <= Ramp->PageSize) {
		ramp2_page_t *Old = Ramp->Page;
		Old->Next = Ramp->Full;
		Ramp->Full = Old;
		ramp2_page_t *New = Ramp->Page = ramp2_group_page_new(Ramp->Group);
		Ramp->Space = Ramp->PageSize - Size;
		return New->Bytes + Ramp->Space;
	} else {
		void *Bytes = malloc(Size);
		ramp2_defer(Ramp, free, Bytes);
		return Bytes;
	}
}

void *ramp2_strdup(ramp2_t *Ramp, const char *String) {
	size_t Length = strlen(String);
	char *Copy = ramp2_alloc(Ramp, Length + 1);
	memcpy(Copy, String, Length);
	Copy[Length] = 0;
	return Copy;
}

ramp2_deferral_t *ramp2_defer(ramp2_t *Ramp, ramp2_defer_fn Callback, void *Arg) {
	ramp2_deferral_t *Deferral = (ramp2_deferral_t *)ramp2_alloc(Ramp, sizeof(ramp2_deferral_t));
	Deferral->Next = Ramp->Deferrals;
	Deferral->Callback = Callback;
	Deferral->Arg = Arg;
	Ramp->Deferrals = Deferral;
	return Deferral;
}

static void ramp2_defer_nop(void *Arg) {}

void ramp2_cancel(ramp2_deferral_t *Deferral) {
	Deferral->Callback = ramp2_defer_nop;
}

void ramp2_clear(ramp2_t *Ramp) {
	for (ramp2_deferral_t *Deferral = Ramp->Deferrals; Deferral; Deferral = Deferral->Next) Deferral->Callback(Deferral->Arg);
	Ramp->Deferrals = NULL;
	if (Ramp->Full) {
		ramp2_page_t *First = Ramp->Full, *Last = First;
		while (Last->Next) Last = Last->Next;
		ramp2_group_t *Group = Ramp->Group;
		ramp2_page_t *Free = Group->Free;
		do {
			Last->Next = Free;
		} while (!atomic_compare_exchange_weak(&Group->Free, &Free, First));
		Ramp->Full = NULL;
	}
	Ramp->Space = Ramp->PageSize;
}

void ramp2_free(ramp2_t *Ramp) {
	for (ramp2_deferral_t *Deferral = Ramp->Deferrals; Deferral; Deferral = Deferral->Next) Deferral->Callback(Deferral->Arg);
	Ramp->Deferrals = NULL;
	Ramp->Page->Next = Ramp->Full;
	ramp2_page_t *First = Ramp->Page, *Last = First;
	while (Last->Next) Last = Last->Next;
	ramp2_group_t *Group = Ramp->Group;
	ramp2_page_t *Free = Group->Free;
	do {
		Last->Next = Free;
	} while (!atomic_compare_exchange_weak(&Group->Free, &Free, First));
	free(Ramp);
}

static void ramp2_unnest(ramp2_t *Ramp) {
	for (ramp2_deferral_t *Deferral = Ramp->Deferrals; Deferral; Deferral = Deferral->Next) Deferral->Callback(Deferral->Arg);
	Ramp->Deferrals = NULL;
	Ramp->Page->Next = Ramp->Full;
	ramp2_page_t *First = Ramp->Page, *Last = First;
	while (Last->Next) Last = Last->Next;
	ramp2_group_t *Group = Ramp->Group;
	ramp2_page_t *Free = Group->Free;
	do {
		Last->Next = Free;
	} while (!atomic_compare_exchange_weak(&Group->Free, &Free, First));
}

ramp2_t *ramp2_nest(ramp2_t *Ramp) {
	ramp2_t *Ramp2 = (ramp2_t *)ramp2_alloc(Ramp, sizeof(ramp2_t));
	ramp2_group_t *Group = Ramp->Group;
	Ramp2->Group = Group;
	Ramp2->Page = ramp2_group_page_new(Group);
	Ramp2->Full = NULL;
	Ramp2->Deferrals = NULL;
	Ramp2->Space = Ramp2->PageSize = Ramp->PageSize;
	ramp2_defer(Ramp, (void *)ramp2_unnest, Ramp2);
	return Ramp2;
}

#include "ramp.h"

typedef struct ramp_page_t ramp_page_t;
typedef struct ramp_ext_t ramp_ext_t;

struct ramp_page_t {
	ramp_page_t *Next;
	void *FreeBytes;
	size_t FreeSpace;
	char Bytes[] __attribute__ ((aligned(16)));
};

struct ramp_ext_t {
	ramp_ext_t *Next;
	void *Bytes;
};

struct ramp_t {
	ramp_page_t *Pages;
	ramp_ext_t *Exts;
	size_t PageSize;
};

ramp_t *ramp_new(size_t PageSize) {
	ramp_t *Ramp = (ramp_t *)malloc(sizeof(ramp_t));
	Ramp->Pages = 0;
	Ramp->Exts = 0;
	Ramp->PageSize = (PageSize + 15) & ~15;
	return Ramp;
}

void *ramp_alloc(ramp_t *Ramp, size_t Size) {
	Size += 15;
	Size &= ~15;
	if (Size >= Ramp->PageSize) {
		void *Bytes = malloc(Size);
		ramp_ext_t *Ext = ramp_alloc(Ramp, sizeof(ramp_ext_t));
		Ext->Bytes = Bytes;
		Ext->Next = Ramp->Exts;
		Ramp->Exts = Ext;
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

void ramp_reset(ramp_t *Ramp) {
	for (ramp_ext_t *Ext = Ramp->Exts; Ext; Ext = Ext->Next) free(Ext->Bytes);
	for (ramp_page_t *Page = Ramp->Pages, *Next; Page; Page = Next) {
		Next = Page->Next;
		free(Page);
	}
	Ramp->Exts = 0;
	Ramp->Pages = 0;
}

void ramp_clear(ramp_t *Ramp) {
	for (ramp_ext_t *Ext = Ramp->Exts; Ext; Ext = Ext->Next) free(Ext->Bytes);
	size_t FreeSpace = Ramp->PageSize;
	for (ramp_page_t *Page = Ramp->Pages; Page; Page = Page->Next) Page->FreeSpace = FreeSpace;
	Ramp->Exts = 0;
}

void ramp_free(ramp_t *Ramp) {
	ramp_reset(Ramp);
	free(Ramp);
}

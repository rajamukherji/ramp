#ifndef RAMP2_H
#define RAMP2_H

#include <stdlib.h>

typedef struct ramp2_t ramp2_t;
typedef struct ramp2_group_t ramp2_group_t;

/**
 * \brief allocates a new ramp2_group_t object.
 *
 * \param PageSize size of a new page.
 */
ramp2_group_t *ramp2_group_new(size_t PageSize);

/**
 * \brief frees unused memory blocks within ramp2_group_t instance.
 *
 * \param Group ramp2_group_t object allocated with ramp2_group_new.
 */
void ramp2_group_reset(ramp2_group_t *Group);

/**
 * \brief frees unused memory blocks within ramp2_group_t instance, keeping at most Count blocks.
 *
 * \param Group ramp2_group_t object allocated with ramp2_group_new.
 * \param Count number of blocks to keep.
 */
void ramp2_group_trim(ramp2_group_t *Group, int Count);

/**
 * \brief frees a ramp2_group_t object and all allocated pages, individual ramp2_t objects must be freed earlier.
 *
 * \param Group ramp2_group_t object allocated with ramp2_group_new.
 */
void ramp2_group_free(ramp2_group_t *Group);

/**
 * \brief allocates a new ramp2_t object.
 *
 * \param Group group object for sharing free pages.
 */
ramp2_t *ramp2_new(ramp2_group_t *Group);

/**
 * \brief allocates a block of memory within ramp2_t instance.
 *
 * \param Ramp ramp2_t object allocated with ramp2_new.
 * \param Size size of memory block to allocate.
 */
void *ramp2_alloc(ramp2_t *Ramp, size_t Size) __attribute__((malloc));

/**
 * \brief copies a string into a ramp2_t instance.
 *
 * \param Ramp ramp2_t object allocated with ramp2_new.
 * \param String string to copy.
 */
void *ramp2_strdup(ramp2_t *Ramp, const char *String) __attribute__((malloc));

typedef void (*ramp2_defer_fn)(void *);
typedef struct ramp2_deferral_t ramp2_deferral_t;

/**
 * \brief defers a function call until ramp2_clear or ramp2_reset.
 *
 * \param Ramp ramp2_t object allocated with ramp2_new.
 * \param Function function to call on reset.
 * \param Arg argument to pass to CleanupFn.
 * \return A deferral reference which can be used to cancel this deferral.
 */
ramp2_deferral_t *ramp2_defer(ramp2_t *Ramp, ramp2_defer_fn Function, void *Arg);

/**
 * \brief cancels a deferred call.
 *
 * \param Deferral ramp2_deferral_t returned by ramp2_defer.
 */
void ramp2_cancel(ramp2_deferral_t *Deferral);

/**
 * \brief frees memory allocated within ramp2_t instance while keeping memory blocks for reuse.
 *
 * \param Ramp ramp2_t object allocated with ramp2_new.
 */
void ramp2_clear(ramp2_t *Ramp);

/**
 * \brief frees a ramp2_t object and all allocated pages.
 *
 * \param Ramp ramp2_t object allocated with ramp2_new.
 */
void ramp2_free(ramp2_t *Ramp);

/**
 * \brief allocates a new ramp2_t object inside an existing ramp2_t, will be cleared automatically (using ramp2_defer).
 *
 * \param Ramp existing ramp2_t object.
 */
ramp2_t *ramp2_nest(ramp2_t *Ramp);

#endif

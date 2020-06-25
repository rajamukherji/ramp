#ifndef RAMP_H
#define RAMP_H

#include <stdlib.h>

typedef struct ramp_t ramp_t;

/**
 * \brief allocates a new ramp_t object.
 *
 * \param PageSize size of a new page.
 */
ramp_t *ramp_new(size_t PageSize);

/**
 * \brief allocates a block of memory within ramp_t instance.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 * \param Size size of memory block to allocate.
 */
void *ramp_alloc(ramp_t *Ramp, size_t Size) __attribute__((malloc));

/**
 * \brief copies a string into a ramp_t instance.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 * \param String string to copy.
 */
void *ramp_strdup(ramp_t *Ramp, const char *String) __attribute__((malloc));

typedef struct ramp_deferral_t ramp_deferral_t;

/**
 * \brief defers a function call until ramp_clear or ramp_reset.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 * \param Function function to call on reset.
 * \param Arg argument to pass to CleanupFn.
 * \return A deferral reference which can be used to cancel this deferral.
 */
ramp_deferral_t *ramp_defer(ramp_t *Ramp, void (*Function)(void *), void *Arg);

/**
 * \brief cancels a deferred call.
 *
 * \param Deferral ramp_deferral_t returned by ramp_defer.
 */
void ramp_cancel(ramp_deferral_t *Deferral);

/**
 * \brief frees memory allocated within ramp_t instance while keeping memory blocks for reuse.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 */
void ramp_clear(ramp_t *Ramp);

/**
 * \brief frees memory allocated within ramp_t instance and frees memory blocks.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 */
void ramp_reset(ramp_t *Ramp);

/**
 * \brief frees a ramp_t object and all allocated pages.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 */
void ramp_free(ramp_t *Ramp);

#endif

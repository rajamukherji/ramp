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
 * \param Size Size of memory block to allocate.
 */
void *ramp_alloc(ramp_t *Ramp, size_t Size) __attribute__((malloc));

/**
 * \brief copies a string into a ramp_t instance.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 * \param String String to copy.
 */
void *ramp_strdup(ramp_t *Ramp, const char *String) __attribute__((malloc));

/**
 * \brief create a deferred cleanup entry which will be called on reset.
 *
 * \param Ramp ramp_t object allocated with ramp_new.
 * \param Size Size of memory block to allocate.
 * \param CleanupFn Function to call on reset.
 */
void *ramp_defer(ramp_t *Ramp, size_t Size, void (*CleanupFn)(void *));

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

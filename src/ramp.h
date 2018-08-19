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
void *ramp_alloc(ramp_t *Ramp, size_t Size);

/**
 * \brief frees memory allocated within ramp_t instance.
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
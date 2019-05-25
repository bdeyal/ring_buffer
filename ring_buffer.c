#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "ring_buffer.h"

/*
 *  specialization for single byte data size
 */
static void get_uint8(const struct ring_buffer* rb, void* data)
{
    *((uint8_t*)data) = rb->buffer[rb->get_index];
}
static void put_uint8(struct ring_buffer* rb, const void* data)
{
    rb->buffer[rb->put_index] = *((uint8_t*)data);
}
/*---------------------------------------------------------------------------*/

/*
 *  specialization for two byte data size
 */
static void get_uint16(const struct ring_buffer* rb, void* data)
{
    *((uint16_t*)data) = ((uint16_t*)rb->buffer)[rb->get_index];
}
static void put_uint16(struct ring_buffer* rb, const void* data)
{
    ((uint16_t*)rb->buffer)[rb->put_index] = *((uint16_t*)data);
}
/*---------------------------------------------------------------------------*/

/*
 *  specialization for four byte data size
 */
static void get_uint32(const struct ring_buffer* rb, void* data)
{
    *((uint32_t*)data) = ((uint32_t*)rb->buffer)[rb->get_index];
}
static void put_uint32(struct ring_buffer* rb, const void* data)
{
    ((uint32_t*)rb->buffer)[rb->put_index] = *((uint32_t*)data);
}
/*---------------------------------------------------------------------------*/

/*
 *  specialization for eight byte data size
 */
static void get_uint64(const struct ring_buffer* rb, void* data)
{
    *((uint64_t*)data) = ((uint64_t*)rb->buffer)[rb->get_index];
}
static void put_uint64(struct ring_buffer* rb, const void* data)
{
    ((uint64_t*)rb->buffer)[rb->put_index] = *((uint64_t*)data);
}
/*---------------------------------------------------------------------------*/

/*
 *  specialization for 16 byte data size. When the compiler knows
 *  it's 16 bytes it optimizes away the memcpy call. For instance,
 *  on x86_64 memcpy w/ 16 bytes is replaced by two SSE instructions
 *  (or AVX depending on compilation flags)
 */
static void get_uint128(const struct ring_buffer* rb, void* data)
{
#if defined(HAVE_INT128)
    *((__uint128_t*)data) = ((__uint128_t*)rb->buffer)[rb->get_index];
#else
    const uint8_t* addr = &rb->buffer[16 * rb->get_index];
    memcpy(data, addr, 16);
#endif
}
static void put_uint128(struct ring_buffer* rb, const void* data)
{
#if defined(HAVE_INT128)
    ((__uint128_t*)rb->buffer)[rb->put_index] = *((__uint128_t*)data);
#else
    uint8_t* addr = &rb->buffer[16 * rb->put_index];
    memcpy(addr, data, 16);
#endif
}
/*---------------------------------------------------------------------------*/

/*
 *   general type and data size
 */
static void get_generic(const struct ring_buffer* rb, void* data)
{
    const uint8_t* addr = &rb->buffer[rb->datasize * rb->get_index];
    memcpy(data, addr, rb->datasize);
}
static void put_generic(struct ring_buffer* rb, const void* data)
{
    uint8_t* addr = &rb->buffer[rb->datasize * rb->put_index];
    memcpy(addr, data, rb->datasize);
}
/*---------------------------------------------------------------------------*/

/*
 *  Function table for get/put function
 */
struct ring_buffer_ops {
    void (*getf)(const struct ring_buffer* rb, void* data);
    void (*putf)(struct ring_buffer* rb, const void* data);
};

/*
 *  vtables for various sizes
 */
static struct ring_buffer_ops uint8_ops   = { get_uint8,   put_uint8 };
static struct ring_buffer_ops uint16_ops  = { get_uint16,  put_uint16 };
static struct ring_buffer_ops uint32_ops  = { get_uint32,  put_uint32 };
static struct ring_buffer_ops uint64_ops  = { get_uint64,  put_uint64 };
static struct ring_buffer_ops uint128_ops = { get_uint128, put_uint128 };
static struct ring_buffer_ops generic_ops = { get_generic, put_generic };
/*---------------------------------------------------------------------------*/

/*
 *   Public API
 */
struct ring_buffer* ring_buffer_create(size_t capacity, size_t element_size)
{
    size_t alloc_len;
    struct ring_buffer* result;

    if (element_size == 0 || capacity == 0) {
        errno = EINVAL;
        return NULL;
    }

    alloc_len = sizeof(struct ring_buffer) + (capacity * element_size);
    result = (struct ring_buffer*) malloc(alloc_len);
    if (!result) {
        errno = ENOMEM;
        return NULL;
    }

    result->count = 0;
    result->put_index = 0;
    result->get_index = 0;
    result->capacity = capacity;
    result->datasize = element_size;

    switch (element_size) {
    case 1:
        result->ops = &uint8_ops;
        break;
    case 2:
        result->ops = &uint16_ops;
        break;
    case 4:
        result->ops = &uint32_ops;
        break;
    case 8:
        result->ops = &uint64_ops;
        break;
    case 16:
        result->ops = &uint128_ops;
        break;
    default:
        result->ops = &generic_ops;
        break;
    }

    return result;
}
/*---------------------------------------------------------------------------*/

void ring_buffer_destroy(struct ring_buffer* rb)
{
    if (rb)
        free(rb);
}
/*---------------------------------------------------------------------------*/

int ring_buffer_put(struct ring_buffer* rb, const void* data)
{
    if (!rb || !data) {
        errno = EINVAL;
        return -1;
    }

    if (ring_buffer_full(rb)) {
        errno = ENOBUFS;
        return -1;
    }

    /*
     * dynamic dispatch for possibly specialized put operation
     */
    rb->ops->putf(rb, data);

    /* advance write index */
    if (++rb->put_index == rb->capacity)
        rb->put_index = 0;

    /* update count */
    ++rb->count;

    /* errno = 0; */
    return 0;
}
/*---------------------------------------------------------------------------*/

int ring_buffer_get(struct ring_buffer* rb, void* data)
{
    if (!data || !rb) {
        errno = EINVAL;
        return -1;
    }

    if (ring_buffer_empty(rb)) {
        errno = ENODATA;
        return -1;
    }

    /*
     * dynamic dispatch for possibly specialized get operation
     */
    rb->ops->getf(rb, data);

    /* advance read index */
    if (++rb->get_index == rb->capacity)
        rb->get_index = 0;

    /* reduce count */
    --rb->count;

    /* errno = 0; */
    return 0;
}
/*---------------------------------------------------------------------------*/

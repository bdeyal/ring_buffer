#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ring_buffer
{
	struct ring_buffer_ops* ops;
    size_t  capacity;
    size_t  count;
    size_t  put_index;
    size_t  get_index;
    size_t  datasize;
    unsigned char buffer[0];
};

/*
 * ctor, dtor
 */
struct ring_buffer* ring_buffer_create(size_t capacity, size_t element_size);
void                ring_buffer_destroy(struct ring_buffer* rb);

/*
 *  Operations: put, get
 */
int ring_buffer_put(struct ring_buffer* rb, const void* data);
int ring_buffer_get(struct ring_buffer* rb, void* data);

/*
 *  Trivial queries / operations imlemented inline
 */
inline size_t ring_buffer_size(const struct ring_buffer* rb) {
	return rb->count;
}
inline size_t ring_buffer_capacity(const struct ring_buffer* rb) {
	return rb->capacity;
}
inline bool ring_buffer_empty(const struct ring_buffer* rb) {
	return rb->count == 0;
}
inline bool ring_buffer_full(const struct ring_buffer* rb) {
	return rb->count == rb->capacity;
}
inline void ring_buffer_reset(struct ring_buffer* rb) {
	rb->count = 0;
	rb->put_index = 0;
	rb->get_index = 0;
}


#ifdef __cplusplus
}
#endif

#endif

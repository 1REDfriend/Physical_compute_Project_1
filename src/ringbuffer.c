#include "ringbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h> 

// Ring buffer implementation with thread safety
typedef struct ringbuffer
{
    uint8_t *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;
    SDL_mutex *mutex;
} ringbuffer_t;

ringbuffer_t *rb_create(size_t capacity)
{
    ringbuffer_t *rb = (ringbuffer_t *)calloc(1, sizeof(ringbuffer_t));
    if (!rb)
        return NULL;

    rb->data = (uint8_t *)malloc(capacity);
    if (!rb->data)
    {
        free(rb);
        return NULL;
    }

    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    rb->mutex = SDL_CreateMutex();

    if (!rb->mutex)
    {
        free(rb->data);
        free(rb);
        return NULL;
    }

    return rb;
}

void rb_destroy(ringbuffer_t *rb)
{
    if (!rb)
        return;

    if (rb->mutex)
    {
        SDL_DestroyMutex(rb->mutex);
    }

    if (rb->data)
    {
        free(rb->data);
    }

    free(rb);
}

size_t rb_push(ringbuffer_t *rb, const uint8_t *data, size_t n)
{
    if (!rb || !data || n == 0)
        return 0;

    SDL_LockMutex(rb->mutex);

    size_t available = rb->capacity - rb->size;
    size_t to_write = (n > available) ? available : n;

    for (size_t i = 0; i < to_write; i++)
    {
        rb->data[rb->tail] = data[i];
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->size++;
    }

    SDL_UnlockMutex(rb->mutex);

    return to_write;
}

size_t rb_pop(ringbuffer_t *rb, uint8_t *out, size_t n)
{
    if (!rb || !out || n == 0)
        return 0;

    SDL_LockMutex(rb->mutex);

    size_t to_read = (n > rb->size) ? rb->size : n;

    for (size_t i = 0; i < to_read; i++)
    {
        out[i] = rb->data[rb->head];
        rb->head = (rb->head + 1) % rb->capacity;
        rb->size--;
    }

    SDL_UnlockMutex(rb->mutex);

    return to_read;
}

size_t rb_size(ringbuffer_t *rb)
{
    if (!rb)
        return 0;

    SDL_LockMutex(rb->mutex);
    size_t size = rb->size;
    SDL_UnlockMutex(rb->mutex);

    return size;
}

size_t rb_capacity(ringbuffer_t *rb)
{
    return rb ? rb->capacity : 0;
}

void rb_clear(ringbuffer_t *rb)
{
    if (!rb)
        return;

    SDL_LockMutex(rb->mutex);
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    SDL_UnlockMutex(rb->mutex);
}

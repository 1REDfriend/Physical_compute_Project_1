#include "ringbuffer.h"
#include <stdlib.h>
#include <string.h>
#include <SDL.h>

typedef struct ringbuffer {
    uint8_t* data;
    size_t   cap;
    size_t   head; // write pos
    size_t   tail; // read pos
    size_t   sz;
    SDL_mutex* mu;
} ringbuffer_t;

ringbuffer_t* rb_create(size_t capacity) {
    ringbuffer_t* rb = (ringbuffer_t*)calloc(1, sizeof(*rb));
    if (!rb) return NULL;
    rb->data = (uint8_t*)malloc(capacity);
    if (!rb->data) { free(rb); return NULL; }
    rb->cap = capacity;
    rb->mu = SDL_CreateMutex();
    if (!rb->mu) { free(rb->data); free(rb); return NULL; }
    return rb;
}

void rb_destroy(ringbuffer_t* rb) {
    if (!rb) return;
    if (rb->mu) SDL_DestroyMutex(rb->mu);
    free(rb->data);
    free(rb);
}

size_t rb_capacity(ringbuffer_t* rb) {
    return rb ? rb->cap : 0;
}

size_t rb_size(ringbuffer_t* rb) {
    if (!rb) return 0;
    SDL_LockMutex(rb->mu);
    size_t s = rb->sz;
    SDL_UnlockMutex(rb->mu);
    return s;
}

void rb_clear(ringbuffer_t* rb) {
    if (!rb) return;
    SDL_LockMutex(rb->mu);
    rb->head = rb->tail = rb->sz = 0;
    SDL_UnlockMutex(rb->mu);
}

static size_t _min(size_t a, size_t b) { return a < b ? a : b; }

size_t rb_push(ringbuffer_t* rb, const uint8_t* data, size_t n) {
    if (!rb || !data || n == 0) return 0;
    SDL_LockMutex(rb->mu);
    size_t space = rb->cap - rb->sz;
    size_t to_write = _min(space, n);
    size_t first = _min(to_write, rb->cap - rb->head);
    memcpy(rb->data + rb->head, data, first);
    size_t second = to_write - first;
    if (second) memcpy(rb->data + 0, data + first, second);
    rb->head = (rb->head + to_write) % rb->cap;
    rb->sz += to_write;
    SDL_UnlockMutex(rb->mu);
    return to_write;
}

size_t rb_pop(ringbuffer_t* rb, uint8_t* out, size_t n) {
    if (!rb || !out || n == 0) return 0;
    SDL_LockMutex(rb->mu);
    size_t avail = rb->sz;
    size_t to_read = _min(avail, n);
    size_t first = _min(to_read, rb->cap - rb->tail);
    memcpy(out, rb->data + rb->tail, first);
    size_t second = to_read - first;
    if (second) memcpy(out + first, rb->data + 0, second);
    rb->tail = (rb->tail + to_read) % rb->cap;
    rb->sz -= to_read;
    SDL_UnlockMutex(rb->mu);
    return to_read;
}

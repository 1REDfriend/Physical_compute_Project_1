#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ringbuffer ringbuffer_t;

/** สร้าง ring buffer สำหรับไบต์ (thread-safe ด้วย mutex ภายใน) */
ringbuffer_t* rb_create(size_t capacity);
/** ทำลาย */
void rb_destroy(ringbuffer_t* rb);
/** push ข้อมูล n ไบต์ (คืนค่าจำนวนที่ push ได้จริง) */
size_t rb_push(ringbuffer_t* rb, const uint8_t* data, size_t n);
/** pop ออกสูงสุด n ไบต์ (คืนค่าจำนวนที่ดึงได้จริง) */
size_t rb_pop(ringbuffer_t* rb, uint8_t* out, size_t n);
/** จำนวนข้อมูลที่มี */
size_t rb_size(ringbuffer_t* rb);
/** ความจุ */
size_t rb_capacity(ringbuffer_t* rb);
/** ล้าง */
void rb_clear(ringbuffer_t* rb);

#ifdef __cplusplus
}
#endif

#endif // RINGBUFFER_H

#ifndef CIRCBUF_H
#define CIRCBUF_H

#include <stdio.h>
#include <stdint.h>

struct circbuf {
    int16_t *data;
    uint32_t read;
    uint32_t write;
    unsigned int len;
};

struct circbuf *circbuf_make(size_t size, unsigned int len);
struct circbuf circbuf_init(size_t size, unsigned int len);
void cbuf_push(struct circbuf *buf, short data);
short cbuf_shift(struct circbuf *buf);

#endif

#ifdef CBUF_IMPLEMENT

#include "assert.h"
#include "stdlib.h"

unsigned int powof2(unsigned int num)
{
    if (num != 0) {
	num--;
	num |= (num >> 1);
	num |= (num >> 2);
	num |= (num >> 4);
	num |= (num >> 8);
	num |= (num >> 16);
	num++;
    }

    return num;
}

int ispow2(int num)
{
    return (num && !(num & (num - 1)));
}

struct circbuf *circbuf_make(size_t size, unsigned int len)
{
    struct circbuf *new = malloc(sizeof(*new));
    new->len = powof2(len);
    new->data = calloc(sizeof(short), new->len);
    new->read = new->write = 0;
    return new;
}

struct circbuf circbuf_init(size_t size, unsigned int len)
{
    struct circbuf new;
    new.len = powof2(len);
    new.data = calloc(sizeof(short), new.len);
    new.read = new.write = 0;

    return new;
}

int cbuf_size(struct circbuf *buf) {
    return buf->write - buf->read;
}

int cbuf_empty(struct circbuf *buf) {
    return buf->read == buf->write;
}

int cbuf_full(struct circbuf *buf) {
    return cbuf_size(buf) >= buf->len;
}

uint32_t cbuf_mask(struct circbuf *buf, uint32_t n) {
    return n & (buf->len-1);
}

void cbuf_push(struct circbuf *buf, short data) {
    //assert(!cbuf_full(buf));

    buf->data[cbuf_mask(buf,buf->write++)] = data;
}

short cbuf_shift(struct circbuf *buf) {
    //assert(!cbuf_empty(buf));
    return buf->data[cbuf_mask(buf, buf->read++)];
}

#endif

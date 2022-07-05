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

struct circbuf circbuf_init(size_t size, unsigned int len);
void cbuf_push(struct circbuf *buf, short data);
short cbuf_shift(struct circbuf *buf);


#endif
#ifndef CIRCBUF_H
#define CIRCBUF_H

#include <stdio.h>

struct circbuf {
    void *head;
    void *tail;
    void *data;
    size_t size;
    int len;
};

struct circbuf circbuf_init(size_t size, int len);
void cbuf_append(struct circbuf *buf, void *data, int n);
void *cbuf_take(struct circbuf *buf);






#endif
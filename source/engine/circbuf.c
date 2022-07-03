#include "circbuf.h"




struct circbuf circbuf_init(size_t size, int len)
{
    struct circbuf new;
    new.size = size;
    new.len = len;
    new.data = calloc(size, len);
    new.head = new.tail = new.data;

    return new;
}

void cbuf_append(struct circbuf *buf, void *data, int n)
{

    for (int i = 0; i < n; i++) {
        memcpy(buf->head, data+(buf->size*i), buf->size);

        buf->head += buf->size;

        if (buf ->head == buf->data + buf->size*buf->len) buf->head = buf->data;
    }

/*
    size_t cpbytes = n * buf->size;
    size_t rembytes = (buf->size * buf->len) - (buf->head - buf->data);

    if (cpbytes <= rembytes)
        memcpy(buf->head, data, cpbytes);
    else {
        memcpy(buf->head, data, rembytes);
        cpbytes -= rembytes;
        buf->head = buf->data;
        memcpy(buf->head, data+rembytes, cpbytes);
    }
*/

}

void *cbuf_take(struct circbuf *buf)
{
    void *ret = buf->tail;

    if (buf->tail == buf->data + buf->len*buf->size) buf->tail = buf->data;
    else buf->tail += buf->size;

    return ret;
}
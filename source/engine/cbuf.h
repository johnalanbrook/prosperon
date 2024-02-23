#ifndef CIRCBUF_H
#define CIRCBUF_H

#include <stdlib.h>
#include <stdint.h>

static inline unsigned int powof2(unsigned int x)
{
  x = x-1;
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return x+1;
}

struct rheader
{
  unsigned int read;
  unsigned int write;
  int len;
};

#define ringheader(r) ((struct rheader *)r-1)

static inline void *ringmake(void *ring, size_t elemsize, unsigned int n)
{
  n = powof2(n);
  if (ring) {
    struct rheader *h = ringheader(ring);
    if (n <= h->len) return h+1;
    h = realloc(h, elemsize*n+sizeof(struct rheader));
    return h+1;
  }
  
  struct rheader *h = malloc(elemsize*n+sizeof(struct rheader));
  h->len = n; h->read = 0; h->write = 0;
  return h+1;
}

#define ringnew(r,n) (r = ringmake(r, sizeof(*r),n))
#define ringfree(r) ((r) ? free(ringheader(r)) : 0)
#define ringmask(r,v) (v & (ringheader(r)->len-1))
#define ringpush(r,v) (r[ringmask(r,ringheader(r)->write++)] = v)
#define ringshift(r) (r[ringmask(r,ringheader(r)->read++)])
#define ringsize(r) ((r) ? ringheader(r)->write - ringheader(r)->read : 0)
#define ringfull(r) ((r) ? ringsize(r) == ringheader(r)->len : 0)
#define ringempty(r) ((r) ? ringheader(r)->read == ringheader(r)->write : 0)

#endif

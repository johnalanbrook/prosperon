#ifndef FREELIST_H
#define FREELIST_H

/* Given a pointer to a struct, create a free list
   Struct must have a 'next' field
*/

struct freelistheader
{
  unsigned int first;
  unsigned int len;
};

#define freelist_header(p) ((struct freelistheader*)p-1)
static inline void *freelist_make(struct freelistheader *list, size_t elemsize, unsigned int n)
{
  list = malloc(elemsize*n*sizeof(struct freelistheader));
  list->first = 0;
  list->len = n;
  return list+1;
}

static inline unsigned int freelist_check(struct freelistheader *h, void *data, size_t elemsize)
{
  if (h->first > h->len) return -1;
  unsigned int id = h->first;
  return id;
}

#define freelist_size(p,l) do{p = freelist_make(p,sizeof(*p),l); for(int i = 0; i < l; i++) { p[i].next = i+1; }}while(0)
#define freelist_len(p) (freelist_header(p)->len)
#define freelist_first(p) (freelist_header(p)->first)
#define freelist_grab(i,p) do{i=freelist_header(p)->first; freelist_header(p)->first = p[i].next; p[i].next = -1;}while(0)
#define freelist_kill(p,i) do{p[i].next = freelist_first(p);freelist_first(p)=i;}while(0)
#define freelist_free(p) (free(freelist_header(p)))

struct link_header
{
  int len;
  void *head;
  void *last;
};

static inline void *add_link(struct link_header *h, size_t size)
{
  void *n = malloc(size);
  return n;
}
#define link_header(p) ((struct link_header*)p-1)
#define link_head(p) (link_header(p)->head)
#define link_last(p) (link_header(p)->last)
#define link_add(p,v) do{
#define link_each(p,fn) do {void *f = p; while (link_header(p)->head != NULL) { fn(p); p = p->next; }; p = f; }while(0)
#define link_e(p,i) (

#define stack_size(p,n) (p = malloc(sizeof(p)*n))
#define stack_push(p,v) ()
#define stack_pop(p) ()

#endif

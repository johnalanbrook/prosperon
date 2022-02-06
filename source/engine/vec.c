#include "vec.h"
#include "log.h"
#include <string.h>
#include <stdlib.h>

struct vec *vec_make(size_t width, int size)
{
    struct vec *new = calloc(1, sizeof(struct vec));
    new->size = size;
    new->width = width;
    new->data = calloc(new->size, new->width);
    return new;
}

void *vec_get(struct vec *vec, int n)
{
    if (n < vec->len)
	return (char *) vec->data + (n * vec->width);

    YughLog(0, 4,
	    "Attempted to access element %d of a vec with %d elements.", n,
	    vec->len);
    return NULL;
}

void vec_walk(struct vec *vec, void (*fn)(void *data))
{
    for(int i = 0; i < vec->len; i++)
	fn((char*)vec->data + (i * vec->width));
}

void vec_delete(struct vec *vec, int n)
{
    if (n < vec->len) {
	vec->len--;
	memcpy(vec_p(vec, n), vec_p(vec, vec->len), vec->width);
    }

}

void vec_del_order(struct vec *vec, int n)
{
    if (n < vec->len) {
	vec->len--;
	memmove(vec_p(vec, n), vec_p(vec, n + 1),
		vec->width * (vec->len - n));
    }
}

void *vec_add(struct vec *vec, const void *data)
{
    if (vec->size == vec->len)
	vec_expand(vec);

    if (data)
	memcpy(vec_p(vec, vec->len), data, vec->width);
    else
	memset(vec_p(vec, vec->len), 0, vec->width);

    vec->len++;
    return vec_get(vec, vec->len - 1);
}

void *vec_add_sort(struct vec *vec, void *data,
		   int (*sort)(void *a, void *b))
{
    if(vec->size == vec->len)
	vec_expand(vec);

    int n = vec->len - 1;
    while (sort(vec_p(vec, n), data))
	n--;

    return vec_insert(vec, data, n);
}

void *vec_insert(struct vec *vec, void *data, int n)
{
    if (vec->size == vec->len)
	vec_expand(vec);

    memmove(vec_p(vec, n), vec_p(vec, n + 1), vec->width * (vec->len - n));
    vec->len++;
    memcpy(vec_p(vec, n), data, vec->width);
    return vec_get(vec, n);
}

char *vec_p(struct vec *vec, int n)
{
    return ((char *) vec->data + (vec->width * n));
}

void *vec_set(struct vec *vec, int n, void *data)
{
    if (vec->len >= n)
	return NULL;

    memcpy(vec_p(vec, n), data, vec->width);
    return vec_get(vec, n);
}

void vec_expand(struct vec *vec)
{
    vec->size *= 2;
    vec->data = realloc(vec->data, vec->size * vec->width);
}

void vec_store(struct vec *vec, FILE * f)
{
    fwrite(&vec->len, sizeof(vec->len), 1, f);
    fwrite(&vec->size, sizeof(vec->size), 1, f);
    fwrite(&vec->width, sizeof(vec->width), 1, f);
    fwrite(vec->data, vec->width, vec->len, f);
}

void vec_load(struct vec *vec, FILE * f)
{
    fread(&vec->len, sizeof(vec->len), 1, f);
    fread(&vec->size, sizeof(vec->size), 1, f);
    fread(&vec->width, sizeof(vec->width), 1, f);
    fread(vec->data, vec->width, vec->len, f);
}

void vec_clear(struct vec *vec)
{
    vec->len = 0;
}

void vec_fit(struct vec *vec)
{
    vec->size = vec->len;
    vec->data = realloc(vec->data, vec->size * vec->width);
}

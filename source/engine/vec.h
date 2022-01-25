#ifndef VEC_H
#define VEC_H

#include <stdio.h>

struct vec {
    int len;
    int size;
    int width;
    void *data;
};

struct vec *vec_make(int width, int size);
void *vec_get(struct vec *vec, int n);
void vec_walk(struct vec *vec, void (*fn)(void *data));
void vec_clear(struct vec *vec);
void *vec_add(struct vec *vec, void *data);

/* sort returns 0 for a<=b, 1 for a>b */
void *vec_add_sort(struct vec *vec, void *data,
		   int (*sort)(void *a, void *b));
void *vec_insert(struct vec *vec, void *data, int n);
void *vec_set(struct vec *vec, int n, void *data);
void vec_delete(struct vec *vec, int n);
void vec_del_order(struct vec *vec, int n);
void vec_store(struct vec *vec, FILE *f);
void vec_load(struct vec *vec, FILE *f);
char *vec_p(struct vec *vec, int n);
void vec_expand(struct vec *vec);
void vec_fit(struct vec *vec);

#endif
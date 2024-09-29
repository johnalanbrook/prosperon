#ifndef NOTA_H
#define NOTA_H

#define NOTA_BLOB 0x00
#define NOTA_TEXT 0x10
#define NOTA_ARR 0x20
#define NOTA_REC 0x30
#define NOTA_FLOAT 0x40
#define NOTA_INT 0x60
#define NOTA_SYM 0x70

#define NOTA_FALSE 0x00
#define NOTA_TRUE 0x01
#define NOTA_NULL 0x02
#define NOTA_INF 0x03
#define NOTA_PRIVATE 0x08
#define NOTA_SYSTEM 0x09

typedef struct NOTA {
  char *head;
} NOTA;

int nota_type(char *nota);

char *nota_read_blob(long long *len, char *nota);
char *nota_read_text(char **text, char *nota);
char *nota_read_array(long long *len, char *nota);
char *nota_read_record(long long *len, char *nota);
char *nota_read_float(double *d, char *nota);
char *nota_read_int(long long *l, char *nota);
char *nota_read_sym(int *sym, char *nota);

void print_nota_hex(char *nota);

char *nota_write_blob(unsigned long long n, char *nota);
char *nota_write_text(const char *s, char *nota);
char *nota_write_array(unsigned long long n, char *nota);
char *nota_write_record(unsigned long long n, char *nota);
char *nota_write_float(double n, char *nota);
char *nota_write_int(long long n, char *nota);
char *nota_write_sym(int sym, char *nota);

#endif

#ifndef NOTA_H
#define NOTA_H

#define NOTA_BLOB 0x00
#define NOTA_TEXT 0x10
#define NOTA_ARR 0x20
#define NOTA_REC 0x30
#define NOTA_FLOAT 0x40
#define NOTA_INT 0x60
#define NOTA_SYM 0x70

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
char *nota_read_bool(int *b, char *nota);

void print_nota_hex(char *nota);

char *nota_write_blob(unsigned long long n, char *nota);
char *nota_write_text(char *s, char *nota);
char *nota_write_array(unsigned long long n, char *nota);
char *nota_write_record(unsigned long long n, char *nota);
char *nota_write_float(double n, char *nota);
char *nota_write_int(long long n, char *nota);
char *nota_write_bool(int b, char *nota);

#endif

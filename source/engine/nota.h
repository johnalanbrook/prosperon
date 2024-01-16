#ifndef NOTA_H
#define NOTA_H

#define NOTA_BLOB 0x00
#define NOTA_TEXT 0x10
#define NOTA_ARR 0x20
#define NOTA_REC 0x30
#define NOTA_FLOAT 0x40
#define NOTA_INT 0x60
#define NOTA_SYM 0x70

int nota_type(char *nota);
void nota_write_int(long long n, char *nota);
long long nota_read_int(char *nota);
char *nota_read_num(char *nota, long long *n);
void nota_write_float(double n, char *nota);
double nota_read_float(char *nota);
void nota_write_bool(int b, char *nota);
int nota_read_bool(char *nota);
void nota_write_blob(unsigned long long n, char *nota);
void nota_write_array(unsigned long long n, char *nota);
void nota_write_text(char *s, char *nota);
char *nota_read_text(char *nota);

#endif

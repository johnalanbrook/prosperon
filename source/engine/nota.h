#ifndef NOTA_H
#define NOTA_H

void nota_write_int(long long n, char *nota);
long long nota_read_int(char *nota);
long long nota_read_num(char *nota);
double nota_read_float(char *nota);
void nota_write_blob(unsigned long long n, char *nota);
void nota_write_array(unsigned long long n, char *nota);
void nota_write_text(char *s, char *nota);

#endif

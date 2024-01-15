#include "nota.h"
#include "stdio.h"
#include "math.h"
#include "string.h"

#define NOTA_CONT 0x80
#define NOTA_DATA 0x7f
#define NOTA_INT_DATA 0x07
#define NOTA_INT_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_FLOAT 0x40
#define NOTA_HEAD_DATA 0x0f
#define CONTINUE(CHAR) (CHAR>>7)
#define NOTA_BLOB 0x00
#define NOTA_TEXT 0x10
#define NOTA_REC 0x30
#define NOTA_ARR 0x20
#define NOTA_INT 0x60
#define NOTA_SYM 0x70

int nota_bits(long long n, int sb)
{
  int bits = ilogb(n)+1;
  bits-=sb; /* start bit */
  int chars = bits/7;
  if (bits%7>0) chars++;
  bits = sb + (chars*7);
  return bits;
}

char *nota_continue_num(long long n, char *nota, int sb)
{
  int bits = nota_bits(n, sb);
  bits -= sb;
  if (bits > 0)
    nota[0] |= NOTA_CONT;

  int shex = ~(~0 << sb);
  nota[0] ^= shex & (n>>bits);
  
  int i = 1;
  while (bits > 0) {
    bits -= 7;
    int head = bits == 0 ? 0 : NOTA_CONT;
    nota[i] = head | (0x7f & (n >> bits));
    i++;
  }

  return &nota[i];
}


void print_nota(char *nota)
{
  int chars = 0;
  if (!(nota[0]>>4 ^ NOTA_TEXT>>4)) {
    chars = nota_read_int(nota);
    printf("there are %d chars in the text\n", chars);
  }
  char *c = nota;
  do {
    printf("%02X ", (unsigned char)(*c));
  } while(CONTINUE(*(c++)));

  for (int i = 0; i < chars; i++)
    printf("%02X ", (unsigned char)c[i]);
    
  printf("\n");
}

void nota_write_int(long long n, char *nota)
{
  printf("number %ld\n", n);
  char sign = 0;
  
  if (n < 0) {
    sign = 0x08;
    n *= -1;
  }
  
  nota[0] = NOTA_INT | sign;  

  nota_continue_num(n, nota, 3);
  print_nota(nota);
}

long long nota_read_num(char *nota)
{
  long long n = 0;
  char *c = nota;
  n |= (*c) & NOTA_HEAD_DATA;
  
  while (CONTINUE(*(c++)))
    n = (n<<7) | (*c) & NOTA_DATA;

  return n;
}

void nota_write_float(double n, char *nota)
{
  printf("number %g\n", n);
  nota[0] = NOTA_FLOAT;
}

double nota_read_float(char *nota)
{
}

long long nota_read_int(char *nota)
{
  long long n = 0;
  char *c = nota;
  n |= (*c) & NOTA_INT_DATA; /* first three bits */
  while (CONTINUE(*(c++)))
    n = (n<<7) | (*c) & NOTA_DATA;

  if (NOTA_INT_SIGN(*nota)) n *= -1;
  return n;
}

/* n is the number of bits */
void nota_write_blob(unsigned long long n, char *nota)
{
  printf("blob %ld\n", n);
  nota[0] = NOTA_BLOB;
  nota_continue_num(n, nota, 4);
  print_nota(nota);
}

void nota_write_array(unsigned long long n, char *nota)
{
  printf("array %ld\n", n);
  nota[0] = NOTA_ARR;
  nota_continue_num(n, nota, 4);
  print_nota(nota);
}

void nota_write_text(char *s, char *nota)
{
  printf("text %s\n", s);
  nota[0] = NOTA_TEXT;
  long n = strlen(s);
  char *start = nota_continue_num(n, nota, 4);
  memcpy(start, s, n);
  print_nota(nota);
}

#include "nota.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"
#include "kim.h"

#define NOTA_CONT 0x80
#define NOTA_DATA 0x7f
#define NOTA_INT_DATA 0x07
#define NOTA_INT_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_SIG_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_EXP_SIGN(CHAR) (CHAR & (1<<4))
#define NOTA_TYPE 0x70
#define NOTA_HEAD_DATA 0x0f
#define CONTINUE(CHAR) (CHAR>>7)

#define UTF8_DATA 0x3f

/* define this to use native string instead of kim. Bytes are encoded instead of runes */
#define NOTA_UTF8

int nota_type(char *nota) { return *nota & NOTA_TYPE; }

char *nota_skip(char *nota)
{
  while (CONTINUE(*nota))
    nota++;

  return nota+1;
}

char *nota_read_num(long long *n, char *nota)
{
  if (!n)
    return nota_skip(nota);
    
  *n = 0;
  *n |= (*nota) & NOTA_HEAD_DATA;
  
  while (CONTINUE(*(nota++)))
    *n = (*n<<7) | (*nota) & NOTA_DATA;

  return nota;
}

int nota_bits(long long n, int sb)
{
  if (n == 0) return sb;
  int bits = sizeof(n)*CHAR_BIT - __builtin_clzll(n);
  bits-=sb; /* start bit */
  return ((bits + 6) / 7)*7 + sb;
}

char *nota_continue_num(long long n, char *nota, int sb)
{
  int bits = nota_bits(n, sb);
  bits -= sb;
  if (bits > 0)
    nota[0] |= NOTA_CONT;
  else
    nota[0] &= ~NOTA_CONT;

  int shex = (~0) << sb;
  nota[0] &= shex; /* clear shex bits */
  nota[0] |= (~shex) & (n>>bits);

  int i = 1;
  while (bits > 0) {
    bits -= 7;
    int head = bits == 0 ? 0 : NOTA_CONT;
    nota[i] = head | (NOTA_DATA & (n >> bits));
    i++;
  }

  return &nota[i];
}


void print_nota_hex(char *nota)
{
  while (*nota) {
    printf("%02X ", (unsigned char)(*nota));
    nota++;
  }
  printf("\n");
  
  return;
  long long chars = 0;
  if (!((*nota>>4 & 0x07) ^ NOTA_TEXT>>4))
    nota_read_num(&chars, nota);

  if ((*nota>>5) == 2 || (*nota>>5) == 6)
    chars = 1;

  for (int i = 0; i < chars+1; i++) {
    do {
      printf("%02X ", (unsigned char)(*nota));
    } while(CONTINUE(*(nota++)));
  }
    
  printf("\n");
}

char *nota_write_int(long long n, char *nota)
{
  char sign = 0;
  
  if (n < 0) {
    sign = 0x08;
    n *= -1;
  }
  
  nota[0] = NOTA_INT | sign;  

  return nota_continue_num(n, nota, 3);
}


#define NOTA_DBL_PREC 6
#define xstr(s) str(s)
#define str(s) #s

char *nota_write_float(double n, char *nota)
{
  if (n == 0)
    return nota_write_int(0, nota);
  
  int sign = n < 0 ? ~0 : 0;
  if (sign) n *= -1;
  char ns[2+NOTA_DBL_PREC+5];
  snprintf(ns, 2+NOTA_DBL_PREC+5, "%." xstr (NOTA_DBL_PREC) "e", n);

  long long e = atoll(&ns[2+NOTA_DBL_PREC+1]);
  ns[2+NOTA_DBL_PREC] = 0;

  char *z = ns + 1 + NOTA_DBL_PREC;
  while (*z == '0')
    z--;

  *(z+1) = 0;

  int expadd = (ns+strlen(ns)) - strchr(ns,'.') - 1;
  e-=expadd;
  ns[1] = ns[0];
  long long sig = atoll(ns+1);

  if (e == 0)
    return nota_write_int(sig * (sign ? -1 : 1), nota);

  int expsign = e < 0 ? ~0 : 0;
  if (expsign) e *= -1;

  nota[0] = NOTA_FLOAT;
  nota[0] |= 0x10 & expsign;
  nota[0] |= 0x08 & sign;

  char *c = nota_continue_num(e, nota, 3);
  return nota_continue_num(sig, c, 7);
}

char *nota_read_float(double *d, char *nota)
{
  long long sig = 0;
  long long e = 0;

  char *c = nota;
  e = (*c) & NOTA_INT_DATA; /* first three bits */

  while (CONTINUE(*c)) {
    e = (e<<7) | (*c) & NOTA_DATA;
    c++;
  }
  
  c++;

  do 
    sig = (sig<<7) | *c & NOTA_DATA;
  while (CONTINUE(*(c++)));

  if (NOTA_SIG_SIGN(*nota)) sig *= -1;
  if (NOTA_EXP_SIGN(*nota)) e *= -1;

  *d = (double)sig * pow(10.0, e);
  return c;
}

char *nota_read_int(long long *n, char *nota)
{
  if (!n)
    return nota_skip(nota);

  *n = 0;
  char *c = nota;
  *n |= (*c) & NOTA_INT_DATA; /* first three bits */
  while (CONTINUE(*(c++)))
    *n = (*n<<7) | (*c) & NOTA_DATA;

  if (NOTA_INT_SIGN(*nota)) *n *= -1;

  return c;
}

/* n is the number of bits */
char *nota_write_blob(unsigned long long n, char *nota)
{
  nota[0] = NOTA_BLOB;
  return nota_continue_num(n, nota, 4);
}

char *nota_write_array(unsigned long long n, char *nota)
{
  nota[0] = NOTA_ARR;
  return nota_continue_num(n, nota, 4);
}

char *nota_read_array(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_read_record(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_read_blob(long long *len, char *nota)
{
  if (!len) return nota;
  return nota_read_num(len, nota);
}

char *nota_write_record(unsigned long long n, char *nota)
{
  nota[0] = NOTA_REC;
  return nota_continue_num(n, nota, 4);
}

char *nota_write_sym(int sym, char *nota)
{
  *nota = NOTA_SYM | sym;
  return nota+1;
}

char *nota_read_sym(int *sym, char *nota)
{
  if (*sym) *sym = (*nota) & 0x0f;
  return nota+1;
}

char *nota_read_text(char **text, char *nota)
{
  long long chars;
  nota = nota_read_num(&chars, nota);

#ifdef NOTA_UTF8
  *text = calloc(chars+1,1);
  memcpy(*text, nota, chars);
  nota += chars;
#else
  char utf[chars*4];
  char *pp = utf;
  kim_to_utf8(&nota, &pp, chars);
  *pp = 0;
  *text = strdup(utf);  
#endif

  return nota;
}

char *nota_write_text(char *s, char *nota)
{
  nota[0] = NOTA_TEXT;
  
#ifdef NOTA_UTF8
  long long n = strlen(s);
  nota = nota_continue_num(n,nota,4);
  memcpy(nota, s, n);
  return nota+n;
#else
  long long n = utf8_count(s);
  nota = nota_continue_num(n,nota,4);  
  utf8_to_kim(&s, &nota);
  return nota;
#endif
}


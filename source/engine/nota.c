#include "nota.h"
#include "stdio.h"
#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "limits.h"

#define NOTA_CONT 0x80
#define NOTA_DATA 0x7f
#define NOTA_INT_DATA 0x07
#define NOTA_INT_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_SIG_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_EXP_SIGN(CHAR) (CHAR & (1<<4))
#define NOTA_TYPE 0x70
#define NOTA_HEAD_DATA 0x0f
#define CONTINUE(CHAR) (CHAR>>7)

#define NOTA_FALSE 0x00
#define NOTA_TRUE 0x01
#define NOTA_PRIVATE 0x08
#define NOTA_SYSTEM 0x09

#define UTF8_DATA 0x3f

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

/* kim is 7, 14, then 21 */

int utf8_bytes(char *s)
{
  int bytes = __builtin_clz(~(*s));
  if (!bytes) return 1;
  return bytes-24;
}

int utf8_count(char *s)
{
  int count = 0;
  char *p = s;

  while(*s) {
    count++;
    s += utf8_bytes(s);
  }
  
  return count;
}

int decode_utf8(char **s) {
  int k = **s ? __builtin_clz(~(**s << 24)) : 0;  // Count # of leading 1 bits.
  int mask = (1 << (8 - k)) - 1;                  // All 1's with k leading 0's.
  int value = **s & mask;
  for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
    value <<= 6;
    value += (**s & 0x3F);
  }
  return value;
}

void encode_utf8(char **s, char *end, int code) {
  if (code < 255) {
    **s = code;
    (*s)++;
    return;
  }

  char val[4];
  int lead_byte_max = 0x7F;
  int val_index = 0;
  while (code > lead_byte_max) {
    val[val_index++] = (code & 0x3F) | 0x80;
    code >>= 6;
    lead_byte_max >>= (val_index == 1 ? 2 : 1);
  }
  val[val_index++] = (code & lead_byte_max) | (~lead_byte_max << 1);
  while (val_index-- && *s < end) {
    **s = val[val_index];
    (*s)++;
  }
}

void encode_kim(char **s, char *end, int code)
{
  if (code < 255) {
    **s = 0 | (NOTA_DATA & code);
    (*s)++;
    return;
  }
  
  int bits = ((32 - __builtin_clz(code) + 6) / 7) * 7;

  while (bits > 7) {
    bits -= 7;
    **s = NOTA_CONT | NOTA_DATA & (code >> bits);
    (*s)++;
  }
  **s = NOTA_DATA & code;
  (*s)++;
}

int decode_kim(char **s)
{
  int rune = **s & NOTA_DATA;
  while (CONTINUE(**s)) {
    rune <<= 7;
    (*s)++;
    rune |= **s & NOTA_DATA;
  }
  (*s)++;
  return rune;
}

char *utf8_to_kim(char *utf, char *kim)
{
  while (*utf)
    encode_kim(&kim, NULL, decode_utf8(&utf));

  return kim;
}

void kim_to_utf8(char *kim, char *utf, int runes)
{
  for (int i = 0; i < runes; i++)
    encode_utf8(&utf, utf+4, decode_kim(&kim));
    
  *utf = 0;
}

char *nota_read_text(char **text, char *nota)
{
  long long chars;
  nota = nota_read_num(&chars, nota);
  char utf[chars*4];
  kim_to_utf8(nota, utf, chars);
  *text = strdup(utf);
  return nota;
}

char *nota_write_bool(int b, char *nota)
{
  *nota = NOTA_SYM | (b ? NOTA_TRUE : NOTA_FALSE);
  return nota+1;
}

char *nota_read_bool(int *b, char *nota)
{
  if (b) *b = (*nota) & 0x0f;
  return nota+1;
}

char *nota_write_text(char *s, char *nota)
{
  char *start = nota;
  nota[0] = NOTA_TEXT;
  long long n = utf8_count(s);
  nota = nota_continue_num(n,nota,4);
  return utf8_to_kim(s, nota);
}


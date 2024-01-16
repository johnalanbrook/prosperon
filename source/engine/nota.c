#include "nota.h"
#include "stdio.h"
#include "math.h"
#include "string.h"

#define NOTA_CONT 0x80
#define NOTA_DATA 0x7f
#define NOTA_INT_DATA 0x07
#define NOTA_INT_SIGN(CHAR) (CHAR & (1<<3))
#define NOTA_TYPE 0x70
#define NOTA_HEAD_DATA 0x0f
#define CONTINUE(CHAR) (CHAR>>7)

#define NOTA_FALSE 0x00
#define NOTA_TRUE 0x01
#define NOTA_PRIVATE 0x08
#define NOTA_SYSTEM 0x09

#define UTF8_DATA 0x3f

int nota_type(char *nota) { return *nota & NOTA_TYPE; }

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
    nota[i] = head | (NOTA_DATA & (n >> bits));
    i++;
  }

  return &nota[i];
}


void print_nota_hex(char *nota)
{
  long long chars = 0;
  if (!((*nota>>4 & 0x07) ^ NOTA_TEXT>>4)) {
    nota_read_num(nota, &chars);
    printf("print with %d points\n", chars);    
  }

  for (int i = 0; i < chars+1; i++) {
    do {
      printf("%02X ", (unsigned char)(*nota));
    } while(CONTINUE(*(nota++)));
  }
    
  printf("\n");
}

void nota_write_int(long long n, char *nota)
{
  printf("number %lld\n", n);
  char sign = 0;
  
  if (n < 0) {
    sign = 0x08;
    n *= -1;
  }
  
  nota[0] = NOTA_INT | sign;  

  nota_continue_num(n, nota, 3);
  print_nota_hex(nota);
}

char *nota_read_num(char *nota, long long *n)
{
  *n = 0;
  *n |= (*nota) & NOTA_HEAD_DATA;
  
  while (CONTINUE(*(nota++)))
    *n = (*n<<7) | (*nota) & NOTA_DATA;

  return nota;
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
  printf("blob %lld\n", n);
  nota[0] = NOTA_BLOB;
  nota_continue_num(n, nota, 4);
  print_nota_hex(nota);
}

void nota_write_array(unsigned long long n, char *nota)
{
  printf("array %lld\n", n);
  nota[0] = NOTA_ARR;
  nota_continue_num(n, nota, 4);
  print_nota_hex(nota);
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
  
  int bits = 32 - __builtin_clz(code);
  if (bits <= 7)
    bits = 7;
  else if (bits <= 14)
    bits = 14;
  else
    bits = 21;

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

void utf8_to_kim(char *utf, char *kim)
{
  while (*utf)
    encode_kim(&kim, NULL, decode_utf8(&utf));
}

void kim_to_utf8(char *kim, char *utf, int runes)
{
  for (int i = 0; i < runes; i++)
    encode_utf8(&utf, utf+4, decode_kim(&kim));
    
  *utf = 0;
}

char *nota_read_text(char *nota)
{
  long long chars;
  nota = nota_read_num(nota, &chars);
  printf("reading %d runes\n", chars);
  char utf[chars*4];
  kim_to_utf8(nota, utf, chars);
  return strdup(utf);
}

void nota_write_bool(int b, char *nota)
{
  *nota = NOTA_SYM | (b ? NOTA_TRUE : NOTA_FALSE);
}

int nota_read_bool(char *nota)
{
  return *nota & 0x0f;
}

void nota_write_text(char *s, char *nota)
{
  char *start = nota;
  nota[0] = NOTA_TEXT;
  long n = utf8_count(s);
  printf("text %s with %d points\n", s, n);  
  nota = nota_continue_num(n,nota,4);
  utf8_to_kim(s, nota);
  print_nota_hex(start);
}

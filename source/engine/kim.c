#include "kim.h"

#define KIM_CONT 0x80
#define KIM_DATA 0x7f
#define CONTINUE(CHAR) (CHAR>>7)

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

/* decode and advance s, returning the character cde */
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

/* Write and advance s with code in utf-8 */
void encode_utf8(char **s, int code) {
  char val[4];
  int lead_byte_max = 0x7F;
  int val_index = 0;
  while (code > lead_byte_max) {
    val[val_index++] = (code & 0x3F) | 0x80;
    code >>= 6;
    lead_byte_max >>= (val_index == 1 ? 2 : 1);
  }
  val[val_index++] = (code & lead_byte_max) | (~lead_byte_max << 1);
  while (val_index--) {
    **s = val[val_index];
    (*s)++;
  }
}

/* write and advance s with code in kim */
void encode_kim(char **s, int code)
{
  if (code < KIM_CONT) {
    **s = 0 | (KIM_DATA & code);
    (*s)++;
    return;
  }
  
  int bits = ((32 - __builtin_clz(code) + 6) / 7) * 7;

  while (bits > 7) {
    bits -= 7;
    **s = KIM_CONT | KIM_DATA & (code >> bits);
    (*s)++;
  }
  **s = KIM_DATA & code;
  (*s)++;
}

/* decode and advance s, returning the character code */
int decode_kim(char **s)
{
  int rune = **s & KIM_DATA;
  while (CONTINUE(**s)) {
    rune <<= 7;
    (*s)++;
    rune |= **s & KIM_DATA;
  }
  (*s)++;
  return rune;
}

/* write a null-terminated utf8 stream into a kim string */
void utf8_to_kim(char **utf, char **kim)
{
  while (**utf)
    encode_kim(kim, decode_utf8(utf));
}

/* write number of runes from a kim stream int a utf8 stream */
void kim_to_utf8(char **kim, char **utf, int runes)
{
  for (int i = 0; i < runes; i++)
    encode_utf8(utf, decode_kim(kim));
}

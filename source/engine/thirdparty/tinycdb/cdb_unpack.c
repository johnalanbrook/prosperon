/* cdb_unpack.c: unpack a 32bit integer from network byte order
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt+cdb@corpit.ru.
 * Public domain.
 */

#include "cdb.h"

unsigned
cdb_unpack(const unsigned char buf[4])
{
  unsigned n = buf[3];
  n <<= 8; n |= buf[2];
  n <<= 8; n |= buf[1];
  n <<= 8; n |= buf[0];
  return n;
}

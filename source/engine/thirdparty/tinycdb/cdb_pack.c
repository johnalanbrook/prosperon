/* cdb_pack.c: pack a 32bit integer (to network byte order)
 *
 * This file is a part of tinycdb package by Michael Tokarev, mjt+cdb@corpit.ru.
 * Public domain.
 */

#include "cdb.h"

void
cdb_pack(unsigned num, unsigned char buf[4])
{
  buf[0] = num & 255; num >>= 8;
  buf[1] = num & 255; num >>= 8;
  buf[2] = num & 255;
  buf[3] = num >> 8;
}

/* cdb_seq.c: sequential record retrieval routines
 *
 * This file is a part of tinycdb package.
 * Copyright (C) 2001-2023 Michael Tokarev <mjt+cdb@corpit.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "cdb_int.h"

int
cdb_seqnext(unsigned *cptr, struct cdb *cdbp) {
  unsigned klen, vlen;
  unsigned pos = *cptr;
  unsigned dend = cdbp->cdb_dend;
  const unsigned char *mem = cdbp->cdb_mem;
  if (pos > dend - 8)
    return 0;
  klen = cdb_unpack(mem + pos);
  vlen = cdb_unpack(mem + pos + 4);
  pos += 8;
  if (dend - klen < pos || dend - vlen < pos + klen)
    return errno = EPROTO, -1;
  cdbp->cdb_kpos = pos;
  cdbp->cdb_klen = klen;
  cdbp->cdb_vpos = pos + klen;
  cdbp->cdb_vlen = vlen;
  *cptr = pos + klen + vlen;
  return 1;
}

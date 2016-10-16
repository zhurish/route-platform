/* Library of various efficient Hash Functions.
**
** Copyright (C) 2002-2006 Pranjal Kumar Dutta <prdutta@users.sourceforge.net>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*-----------------------------------------------------------------------------
 * Following are from lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 *
 * These are functions for producing 32-bit hashes for hash table lookup.
 * hashword(), hashlittle(), hashbig(), mix(), and final() are externally 
 * useful functions.  Routines to test the hash are included if SELF_TEST 
 * is defined.  You can use this free for any purpose.  It has no warranty.
 *
 * You probably want to use hashlittle().  hashlittle() and hashbig()
 * hash byte arrays.  hashlittle() is is faster than hashbig() on
 * little-endian machines.  Intel and AMD are little-endian machines.
 *
 * If you want to find a hash of, say, exactly 7 integers, do
 * a = i1;  b = i2;  c = i3;
 *  mix(a,b,c);
 *  a += i4; b += i5; c += i6;
 *  mix(a,b,c);
 *  a += i7;
 *  final(a,b,c);
 * then use c as the hash value.  If you have a variable length array of
 * 4-byte integers to hash, use hashword().  If you have a byte array (like
 * a character string), use hashlittle().  If you have several byte arrays, or
 * a mix of things, see the comments above hashlittle().
 *---------------------------------------------------------------------------*/
#include <zebra.h>

#include "jhash.h"

/*----------------------------------------------------------------------------
 * My best guess at if you are big-endian or little-endian.  This may need 
 * adjustment.
 *--------------------------------------------------------------------------*/
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
     __BYTE_ORDER == __LITTLE_ENDIAN) || \
    (defined(i386) || defined(__i386__) || defined(__i486__) || \
     defined(__i586__) || defined(__i686__) || defined(vax) || defined(MIPSEL))
# define HASH_LITTLE_ENDIAN 1
# define HASH_BIG_ENDIAN 0
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
       __BYTE_ORDER == __BIG_ENDIAN) || \
      (defined(sparc) || defined(POWERPC) || defined(mc68000) || defined(sel))
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 1
#else
# define HASH_LITTLE_ENDIAN 0
# define HASH_BIG_ENDIAN 0
#endif

#define hashsize(n) ((u_int32_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define rot(x,k) (((x)<<(k)) ^ ((x)>>(32-(k))))

/*-----------------------------------------------------------------------------
 * Function : mix -- mix 3 32-bit values reversibly.
 *
 * This is reversible, so any information in (a,b,c) before mix() is
 * still in (a,b,c) after mix().

 * If four pairs of (a,b,c) inputs are run through mix(), or through
 * mix() in reverse, there are at least 32 bits of the output that
 * are sometimes the same for one pair and different for another pair.
 * This was tested for:
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or 
 * all zero plus a counter that starts at zero.
 *
 * Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
 * satisfy this are
 *   4  6  8 16 19  4
 *   9 15  3 18 27 15
 *  14  9  3  7 17  3
 * Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
 * for "differ" defined as + with a one-bit base and a two-bit delta.  I
 * used http://burtleburtle.net/bob/hash/avalanche.html to choose 
 * the operations, constants, and arrangements of the variables.
 *
 * This does not achieve avalanche.  There are input bits of (a,b,c)
 * that fail to affect some output bits of (a,b,c), especially of a.  The
 * most thoroughly mixed value is c, but it doesn't really even achieve
 * avalanche in c.
 *
 * This allows some parallelism.  Read-after-writes are good at doubling
 * the number of bits affected, so the goal of mixing pulls in the opposite
 * direction as the goal of parallelism.  I did what I could.  Rotates
 * seem to cost as much as shifts on every machine I could lay my hands
 * on, and rotates are much kinder to the top and bottom bits, so I used
 *rotates.
 *--------------------------------------------------------------------------*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*-----------------------------------------------------------------------------
 * Function : final -- final mixing of 3 32-bit values (a,b,c) into c
 *
 * Pairs of (a,b,c) values differing in only a few bits will usually
 * produce values of c that look totally different.  This was tested for
 * pairs that differed by one bit, by two bits, in any combination
 * of top bits of (a,b,c), or in any combination of bottom bits of
 * (a,b,c).
 * "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
 * the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
 * is commonly produced by subtraction) look like a single 1-bit
 * difference.
 * the base values were pseudorandom, all zero but one bit set, or 
 * all zero plus a counter that starts at zero.
 *
 * These constants passed:
 * 14 11 25 16 4 14 24
 * 12 14 25 16 4 14 24
 * and these came close:
 * 4  8 15 26 3 22 24
 * 10  8 15 26 3 22 24
 * 11  8 15 26 3 22 24
 *-------------------------------------------------------------------------*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*--------------------------------------------------------------------
 * Function : hashword
 * Input    : k = The key, an array of u_in32_t values.
 *          : length = The length of key in u_int32_t values.
 *          : initval = The previous hash, or an arbitrary value.
 * Output   : Returns the 32bit hash value.
 * Synopsis : The function hashword() is identical to hashlittle() on 
 *            little-endian machines, and identical to hashbig() on big-endian 
 *            machines, except that the length has to be measured in uint32_ts 
 *            rather than in bytes.  hashlittle() is more complicated than 
 *            hashword() only because  hashlittle() has to dance around fitting
 *            the key bytes into registers.
 *            This works on all machines.  To be useful, it requires
 *            -- that the key be an array of uint32_t's, and
 *            -- that all your machines have the same endianness, and
 *            -- that the length be the number of uint32_t's in the key
 *-----------------------------------------------------------------*/
u_int32_t 
hashword (u_int32_t *k,
          size_t  length,                      
          u_int32_t  initval)              
{
  u_int32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + (((u_int32_t)length)<<2) + initval;

  /* handle most of the key */
  while (length > 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mix(a,b,c);
    length -= 3;
    k += 3;
  }

  /* handle the last 3 uint32_t's */
  switch(length)    
  { 
     case 3 : c+=k[2];
     case 2 : b+=k[1];
     case 1 : a+=k[0];
     final(a,b,c);
     case 0:     /* case 0: nothing left to add */
     break;
  }
  /*-- report the result */
  return c;
}


/*-----------------------------------------------------------------------------
 * Function : hashlittle()
 * Input    : k = the key (the unaligned variable-length array of bytes)
 *          : length = the length of the key, counting by bytes
 *          : initval = can be any 4-byte value
 * Output   : Returns a 32-bit value.  Every bit of the key affects every bit 
 *            of the return value.  Two keys differing by one or two bits will
 *            have totally different hash values.
 *
 * Synopsis : hash a variable-length key into a 32-bit value
 *            The best hash table sizes are powers of 2.  There is no need to 
 *            do mod a prime (mod is sooo slow!).  If you need less than 32 
 *            bits,use a bitmask.  For example, if you need only 10 bits, do
 *            h = (h & hashmask(10));
 *            In which case, the hash table should have hashsize(10) elements.
 *            If you are hashing n strings (uint8_t **)k, do it like this:
 *             for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);
 *
 * By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
 * code any way you wish, private, educational, or commercial.  It's free.
 *
 * Use for hash table lookup, or anything where one collision in 2^^32 is
 * acceptable.  Do NOT use for cryptographic purposes.
 *---------------------------------------------------------------------------*/
u_int32_t 
hashlittle( void *key, 
            size_t length, 
            u_int32_t initval)
{
  u_int32_t a,b,c;
  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((u_int32_t)length) + initval;

  if (HASH_LITTLE_ENDIAN && !((((u_int8_t *)key)-(u_int8_t *)0) & 0x3)) {
    u_int32_t *k = key;                                 /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

  } else if (HASH_LITTLE_ENDIAN && !((((u_int8_t *)key)-(u_int8_t *)0) & 0x1)) {
    u_int16_t *k = key;                                 /* read 16-bit chunks */

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((u_int32_t)k[1])<<16);
      b += k[2] + (((u_int32_t)k[3])<<16);
      c += k[4] + (((u_int32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    switch(length)
    {
    case 12: c+=k[4]+(((u_int32_t)k[5])<<16);
             b+=k[2]+(((u_int32_t)k[3])<<16);
             a+=k[0]+(((u_int32_t)k[1])<<16);
             break;
    case 11: c+=((u_int32_t)(k[5]&0xff))<<16;/* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((u_int32_t)k[3])<<16);
             a+=k[0]+(((u_int32_t)k[1])<<16);
             break;
    case 9 : c+=k[4]&0xff;                /* fall through */
    case 8 : b+=k[2]+(((u_int32_t)k[3])<<16);
             a+=k[0]+(((u_int32_t)k[1])<<16);
             break;
    case 7 : b+=((u_int32_t)(k[3]&0xff))<<16;/* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((u_int32_t)k[1])<<16);
             break;
    case 5 : b+=k[2]&0xff;                /* fall through */
    case 4 : a+=k[0]+(((u_int32_t)k[1])<<16);
             break;
    case 3 : a+=((u_int32_t)(k[1]&0xff))<<16;/* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k[0]&0xff;
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    u_int8_t *k = key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((u_int32_t)k[1])<<8;
      a += ((u_int32_t)k[2])<<16;
      a += ((u_int32_t)k[3])<<24;
      b += k[4];
      b += ((u_int32_t)k[5])<<8;
      b += ((u_int32_t)k[6])<<16;
      b += ((u_int32_t)k[7])<<24;
      c += k[8];
      c += ((u_int32_t)k[9])<<8;
      c += ((u_int32_t)k[10])<<16;
      c += ((u_int32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((u_int32_t)k[11])<<24;
    case 11: c+=((u_int32_t)k[10])<<16;
    case 10: c+=((u_int32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((u_int32_t)k[7])<<24;
    case 7 : b+=((u_int32_t)k[6])<<16;
    case 6 : b+=((u_int32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((u_int32_t)k[3])<<24;
    case 3 : a+=((u_int32_t)k[2])<<16;
    case 2 : a+=((u_int32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}

/*-----------------------------------------------------------------------------
 * Function : hashbig()
 * This is the same as hashword() on big-endian machines.  It is different
 * from hashlittle() on all machines.  hashbig() takes advantage of
 * big-endian byte ordering. 
 *---------------------------------------------------------------------------*/
u_int32_t 
hashbig (void *key, 
         size_t length, 
         u_int32_t initval)
{
  u_int32_t a,b,c;

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((u_int32_t)length) + initval;

  if (HASH_BIG_ENDIAN && !((((u_int8_t *)key)-(u_int8_t *)0) & 0x3)) {
    u_int32_t *k = key;                                 /* read 32-bit chunks */

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]<<8; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]<<16; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]<<24; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]<<8; a+=k[0]; break;
    case 6 : b+=k[1]<<16; a+=k[0]; break;
    case 5 : b+=k[1]<<24; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]<<8; break;
    case 2 : a+=k[0]<<16; break;
    case 1 : a+=k[0]<<24; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    u_int8_t *k = key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += ((u_int32_t)k[0])<<24;
      a += ((u_int32_t)k[1])<<16;
      a += ((u_int32_t)k[2])<<8;
      a += ((u_int32_t)k[3]);
      b += ((u_int32_t)k[4])<<24;
      b += ((u_int32_t)k[5])<<16;
      b += ((u_int32_t)k[6])<<8;
      b += ((u_int32_t)k[7]);
      c += ((u_int32_t)k[8])<<24;
      c += ((u_int32_t)k[9])<<16;
      c += ((u_int32_t)k[10])<<8;
      c += ((u_int32_t)k[11]);
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((u_int32_t)k[11])<<24;
    case 11: c+=((u_int32_t)k[10])<<16;
    case 10: c+=((u_int32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((u_int32_t)k[7])<<24;
    case 7 : b+=((u_int32_t)k[6])<<16;
    case 6 : b+=((u_int32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((u_int32_t)k[3])<<24;
    case 3 : a+=((u_int32_t)k[2])<<16;
    case 2 : a+=((u_int32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;
}


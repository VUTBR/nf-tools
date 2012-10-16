/*
 * pg_crc.h
 *
 * PostgreSQL CRC support
 *
 * See Ross Williams' excellent introduction
 * A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS, available from
 * http://www.ross.net/crc/ or several other net sites.
 *
 * We use a normal (not "reflected", in Williams' terms) CRC, using initial
 * all-ones register contents and a final bit inversion.
 *
 * The 64-bit variant is not used as of PostgreSQL 8.1, but we retain the
 * code for possible future use.
 *
 *
 * Portions Copyright (c) 1996-2009, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * $PostgreSQL: pgsql/src/include/utils/pg_crc.h,v 1.21 2009/01/01 17:24:02 momjian Exp $
 *
 * Modificated by Radek KREJCI <krejci@liberouter.org>,
 * CESNET, z.s.p.o, 2009
 *
 * $Id: pg_crc.h 1 2010-12-22 09:59:32Z elich $
 *
 */

#ifndef PG_CRC_H
#define PG_CRC_H

/* 
 * CRC64
 */

/*
 * If we have a 64-bit integer type, then a 64-bit CRC looks just like the
 * usual sort of implementation.  If we have no working 64-bit type, then
 * could fake it with two 32-bit registers.  (Note: experience has shown that
 * the two-32-bit-registers code is as fast as, or even much faster than, the
 * 64-bit code on all but true 64-bit machines.)
 */

/* Constant table for CRC calculation */
extern const uint64_t pg_crc64_table[];


#define UINT64CONST(x) ((uint64_t) x##ULL)

/* Initialize a CRC accumulator */
#define INIT_CRC64(crc) (crc = UINT64CONST(0xffffffffffffffff))

/* Finish a CRC calculation */
#define FIN_CRC64(crc)	(crc ^= UINT64CONST(0xffffffffffffffff))

/* Accumulate some (more) bytes into a CRC */
#define UPDATE_CRC64(crc, data, len)	\
do { \
	uint64_t       __crc0 = crc; \
	unsigned char *__data = (unsigned char *) (data); \
	uint32_t	__len = (len); \
	while (__len-- > 0) \
	{ \
		int		__tab_index = ((int) (__crc0 >> 56) ^ *__data++) & 0xFF; \
		__crc0 = pg_crc64_table[__tab_index] ^ (__crc0 << 8); \
	} \
	crc = __crc0; \
} while (0)

/* Check for equality of two CRCs */
#define EQ_CRC64(c1,c2)  (c1 == c2)

#endif   /* PG_CRC_H */

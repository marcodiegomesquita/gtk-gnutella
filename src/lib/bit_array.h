/*
 * $Id: inputevt.h 10580 2006-03-14 22:58:17Z cbiere $
 *
 * Copyright (c) 2006, Christian Biere
 *
 *----------------------------------------------------------------------
 * This file is part of gtk-gnutella.
 *
 *  gtk-gnutella is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gtk-gnutella is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gtk-gnutella; if not, write to the Free Software
 *  Foundation, Inc.:
 *      59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *----------------------------------------------------------------------
 */

/**
 * @ingroup lib
 * @file
 *
 * Bit arrays. 
 *
 * @author Christian Biere
 * @author Raphael Manfredi
 * @date 2006
 */

#ifndef _bit_array_h_
#define _bit_array_h_

#include "common.h"

/*
 * Functions for handling arrays of bits. On BSD systems, the macros from
 * <bitstring.h> could be used for better efficiency.
 * The following implementation tries to eliminate loop overhead by handling
 * all bits of a "char" at once, where possible.
 */

typedef unsigned long bit_array_t;

#if LONG_MAX == 0x7fffffffL
#if CHAR_BIT == 8
#define BIT_ARRAY_BITSHIFT	(2 + 3)
#elif CHAR_BIT == 16
#define BIT_ARRAY_BITSHIFT	(2 + 4)
#else
#error "Unsupported size of char"
#endif	/* CHAR_BIT */
#elif (ULONG_MAX >> 31) > 0xffffffffUL
#if CHAR_BIT == 8
#define BIT_ARRAY_BITSHIFT	(3 + 3)
#elif CHAR_BIT == 16
#define BIT_ARRAY_BITSHIFT	(3 + 4)
#else
#error "Unsupported size of char"
#endif	/* CHAR_BIT */
#endif	/* LONG_MAX */

#define BIT_ARRAY_BITSIZE (CHAR_BIT * sizeof(bit_array_t))
#define BIT_ARRAY_BITMASK (BIT_ARRAY_BITSIZE - 1)
#define BIT_ARRAY_WORD(base, i) base[(i) >> BIT_ARRAY_BITSHIFT]
#define BIT_ARRAY_BIT(base, i) (1UL << ((i) & BIT_ARRAY_BITMASK)) 

/**
 * Use the macro BIT_ARRAY_SIZE for allocating a properly sized bit array
 * for "n" bits. Example:
 *
 * static bit_array_t ba[BIT_ARRAY_SIZE(100)];
 */
#define BIT_ARRAY_SIZE(n) \
	((n) / (CHAR_BIT * sizeof(bit_array_t)) + \
	 ((n) % (CHAR_BIT * sizeof(bit_array_t)) ? 1 : 0))

/**
 * Use the macro BIT_ARRAY_BYTE_SIZE for dynamic allocation.
 * Example:
 *
 *  bit_array_t *bits = malloc(BIT_ARRAY_BYTE_SIZE(num_bits));
 *
 **/
 #define BIT_ARRAY_BYTE_SIZE(n) (BIT_ARRAY_SIZE(n) * sizeof (bit_array_t))

/**
 * Re-allocates "base" so that it can hold at least "n" bits.
 *
 * @param base The base address of the bit array, may be NULL.
 * @param n The number of bits the bit array should hold.
 * @return the re-allocated bit array.
 *
 * @attention DO NOT USE IN MEMORY ALLOCATING ROUTINES!
 */
static inline bit_array_t *
bit_array_realloc(bit_array_t *base, size_t n)
{
	size_t size;

	STATIC_ASSERT(0 == (BIT_ARRAY_BITSIZE & BIT_ARRAY_BITMASK));
	
	size = BIT_ARRAY_BYTE_SIZE(n);
	return g_realloc(base, size);
}


/**
 * Sets bit number "i" of the bit array "base".
 * @note: For optimum performance, there are no checks at all.
 */
static inline void
bit_array_set(bit_array_t *base, size_t i)
{
	BIT_ARRAY_WORD(base, i) |= BIT_ARRAY_BIT(base, i);
}

/**
 * Sets bit number "i" of the bit array "base".
 * @note: For optimum performance, there are no checks at all.
 */
static inline void 
bit_array_clear(bit_array_t *base, size_t i)
{
	BIT_ARRAY_WORD(base, i) &= ~BIT_ARRAY_BIT(base, i);
}

/**
 * Flips bit number "i" of the bit array "base".
 * @note: For optimum performance, there are no checks at all.
 *
 * @return The new state of the bit.
 */
static inline gboolean
bit_array_flip(bit_array_t *base, size_t i)
{
	return BIT_ARRAY_WORD(base, i) ^= BIT_ARRAY_BIT(base, i);
}

/**
 * Retrieves bit number "i" of the bit array "base".
 * @note: For optimum performance, there are no checks at all.
 * @return TRUE if the bit is set, FALSE otherwise.
 */
static inline gboolean
bit_array_get(const bit_array_t *base, size_t i)
{
	return 0 != (BIT_ARRAY_WORD(base, i) & BIT_ARRAY_BIT(base, i));
}

/**
 * Clears all bits starting at "from" up to "to" inclusive.
 * @note: For optimum performance, there are no checks at all.
 *
 * @param base The base address of the bit array.
 * @param from The first bit.
 * @param to The last bit, must be equal or above "from".
 * @return TRUE if the bit is set, FALSE otherwise.
 */
static inline void 
bit_array_clear_range(bit_array_t *base, size_t from, size_t to)
{
	size_t i;
	
	g_assert(from <= to);

	for (i = from; i <= to; /* NOTHING */) {
		if (0 == (i & BIT_ARRAY_BITMASK)) {
			size_t n = (to - i) >> BIT_ARRAY_BITSHIFT;

			if (n != 0) {
				size_t j = i >> BIT_ARRAY_BITSHIFT;

				i += n * BIT_ARRAY_BITSIZE;
				do {
					base[j++] = 0;
				} while (--n != 0);
				continue;
			}
		}
		bit_array_clear(base, i++);
	}
}

/**
 * Sets all bits starting at "from" up to "to" inclusive.
 * @note: For optimum performance, there are no checks at all.
 *
 * @param base The base address of the bit array.
 * @param from The first bit.
 * @param to The last bit, must be equal or above "from".
 * @return TRUE if the bit is set, FALSE otherwise.
 */
static inline void 
bit_array_set_range(bit_array_t *base, size_t from, size_t to)
{
	size_t i;
	
	g_assert(from <= to);

	for (i = from; i <= to; /* NOTHING */) {
		if (0 == (i & BIT_ARRAY_BITMASK)) {
			size_t n = (to - i) >> BIT_ARRAY_BITSHIFT;

			if (n != 0) {
				size_t j = i >> BIT_ARRAY_BITSHIFT;

				i += n * BIT_ARRAY_BITSIZE;
				do {
					base[j++] = (bit_array_t) -1;
				} while (--n != 0);
				continue;
			}
		}
		bit_array_set(base, i++);
	}
}

/**
 * Peforms a linear scan for the first unset bit of the given bit array.
 *
 * @param base The base address of the bit array.
 * @param from The first bit.
 * @param to The last bit, must be equal or above "from".
 * @return (size_t) -1, if no unset bit was found. On success the
 *        index of the first unset bit is returned.
 */
static inline size_t
bit_array_first_clear(const bit_array_t *base, size_t from, size_t to)
{
	size_t i;

	g_assert(from <= to);

	for (i = from; i <= to; /* NOTHING */) {
		if (0 == (i & BIT_ARRAY_BITMASK)) {
			size_t n = (to - i) >> BIT_ARRAY_BITSHIFT;

			if (n != 0) {
				size_t j = i >> BIT_ARRAY_BITSHIFT;

				while (n-- > 0) {
					if (base[j++] != (bit_array_t) -1) {
						bit_array_t value = base[j - 1];
						while (value & 0x1) {
							value >>= 1;
							i++;
						}
						return i;
					}
					i += BIT_ARRAY_BITSIZE;
				}
				continue;
			}
		}
		if (!bit_array_get(base, i))
			return i;
		i++;
	}

	return (size_t) -1;
}

/**
 * Peforms a linear scan for the last set bit of the given bit array.
 *
 * @param base The base address of the bit array.
 * @param from The first bit.
 * @param to The last bit, must be equal or above "from".
 * @return (size_t) -1, if no set bit was found. On success the
 *        index of the last set bit is returned.
 */
static inline size_t
bit_array_last_set(const bit_array_t *base, size_t from, size_t to)
{
	size_t i;

	g_assert(from <= to);

	for (i = to; i >= from; /* NOTHING */) {
		if (BIT_ARRAY_BITMASK == (i & BIT_ARRAY_BITMASK)) {
			size_t n = (i - from) >> BIT_ARRAY_BITSHIFT;

			if (n != 0) {
				size_t j = i >> BIT_ARRAY_BITSHIFT;

				while (n-- > 0) {
					if (base[j--] != 0) {
						bit_array_t value = base[j + 1];
						bit_array_t mask = 1UL << (BIT_ARRAY_BITSIZE - 1);
						while (mask != 0) {
							if (value & mask)
								return i;
							mask >>= 1;
							i--;
						}
						g_assert_not_reached();
					}
					i -= BIT_ARRAY_BITSIZE;
				}
				continue;
			}
		}
		if (bit_array_get(base, i))
			return i;
		i--;
	}

	return (size_t) -1;
}

#undef BIT_ARRAY_BIT
#undef BIT_ARRAY_BITSIZE
#undef BIT_ARRAY_BITMASK
#undef BIT_ARRAY_BITSHIFT
#undef BIT_ARRAY_WORD

#endif /* _bit_array_h_ */
/* vi: set ts=4 sw=4 cindent: */

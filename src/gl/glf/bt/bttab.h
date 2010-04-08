/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	bttab.h
**
** Description:
**	Declare bt tables and BT_XXX_MACROs for clients.
**
** History:
**	Revision 1.2  86/02/10  12:35:25  perform
**	define:
**	BT_BIT0_MACRO	(bit zero of byte containing the argument)
**	BT_FULLBYTES_MACRO	(number of "full" bytes in vector)
**	BT_NUMBYTES_MACRO	(number of bytes in vector)
**
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**/

GLOBALREF u_char bt_bitmsk[];
GLOBALREF u_char bt_clrmsk[];
GLOBALREF u_char bt_highmsk[];
GLOBALREF u_char bt_lowmsk[];
GLOBALREF u_char bt_ffstab[];
GLOBALREF u_char bt_flstab[];
GLOBALREF u_char bt_cnttab[];

/*
** This BT implementation is VERY wired to 8 bit bytes.  There are table sizes,
** shift and mask values based on the number of bits.
**
** The approach taken is to use macros to hide the "magic" constants that are
** used. On many machines division and modulo by BITSPERBYTE (8 here) are not
** optimized to the equivalent logical operation, and an expensive division is
** done.
**
** Specifically:
**
**		(n / BITSPERBYTE) is always written as (n >> 3) and
**			is hidden by the BT_[FULL,NUM]BYTES_MACROs
**		(n * BITSPERBYTE) is always written as (n << 3) and
**			is hidden by BT_NUMBITS_MACRO
**		(n % BITSPERBYTE) is always written as (n & 07) and
**			is hidden by BT_MOD_MACRO
**
** Unfortunately the `size' argument passed to BT routines is not guaranteed
** to be a multiple of BITSPERBYTE, so we must insure that we don't miss the
** ``last few'' bits due to skipping the last non-full byte.  This is done by
** rounding up the size argument to BT_NUMBYTES_MACRO.
**
** Additionally the macro BT_HIBYTE_MACRO is used to determine the highest
** numbered byte in a byte-stream with the specified size (number of bits).
** BT_NUMBYTES_MACRO doesn't work because size is specified numbering from
** 1 but position (bit and byte) is numbered from 0.
**
**		BT_HIBYTE_MACRO(n) = (BT_NUMBYTES_MACRO(n) - 1)
**
** BT_BIT0_MACRO yields the number of the first bit (BIT 0) in the
** byte containing the bit `pos', e.g.
**		BT_BIT0_MACRO(1)   =   0
**		BT_BIT0_MACRO(11)  =   8
**		BT_BIT0_MACRO(16)  =  16
**		BT_BIT0_MACRO(131) = 128
*/

# define BT_BITMAX			(BITSPERBYTE - 1)
# define BT_FULLBYTES_MACRO(pos)	((pos) >> 3)
# define BT_NUMBYTES_MACRO(pos)		(BT_FULLBYTES_MACRO((pos)+BT_BITMAX))
# define BT_HIBYTE_MACRO(size)		(BT_NUMBYTES_MACRO(size) - 1)
# define BT_NUMBITS_MACRO(numbytes)	((numbytes) << 3)
# define BT_MOD_MACRO(pos)		((pos) & 07)
# define BT_LOWBYTE_MASK		(~(BT_BITMAX))
# define BT_BIT0_MACRO(pos)		((pos) & BT_LOWBYTE_MASK)

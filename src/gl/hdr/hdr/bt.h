/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/
# ifndef BT_HDR_INCLUDED
# define BT_HDR_INCLUDED

/**CL_SPEC
** Name: BT.H - Global definitions for the BT compatibility library.
**
** Specification:
**
** Description:
**      The file contains the type used by BT and the definition of the
**      BT functions.  These are used for bit array manipulation.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**      13-dec-1988 (seputis)
**          made compatible with CL spec used i4  instead of i4
**	6-jan-92 (seputis)
**	    added BTxor
**	22-aug-92 (seputis)
**	    moved to GL, changed to use prototypes, and use ii prefix
**	    Removed BT_OK reference
**	21-apr-95 (tutto01)
**	    Added INTSIZE 4 to take advantage of 32 bit systems.
**	30-sep-1996 (canor01)
**	    Protect against multiple inclusion of bt.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-oct-2004 (gupsh01)
**	    Added BT_SWAP_BYTES_MACRO and 
**	    BT_BYTE_NUMS_MACRO.
**    06-apr-2005 (gupsh01)
**	    Fixed BT_SWAP_BYTES_MACRO, to correctly identify
**	    swapping mechanism for boundary line values.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	6-Jan-2009 (kibro01)
**	    Add BTclearmask
**   26-feb-2009 (Ralph Loen) Bug 121726)
**          Revised BT_BYTES_NUM_MACRO to account for bytes with
**          maximum values (0xFF).
**	20-Jun-2009 (kschendel) SIR 122138
**	    Remove INTSIZE, not used in bt and doesn't belong here anyway.
**/

/*
**  Forward and/or External function references.
*/


/* use defines to force inclusion of BT.H and to reduce name space
** pollution */
#define		BTand	    iiBTand
#define		BTnot	    iiBTnot
#define		BTclear	    iiBTclear
#define		BTcount	    iiBTcount
#define		BThigh	    iiBThigh
#define		BTnext	    iiBTnext
#define		BTor	    iiBTor
#define		BTxor	    iiBTxor
#define		BTset	    iiBTset
#define		BTsubset    iiBTsubset
#define		BTtest	    iiBTtest
#define		BTclearmask iiBTclearmask

/* Name: BT_SWAP_BYTES_MACRO - swaps the bytes on a non byte swapped
**
** Description -
**
**      This MACRO is used on a little endian machine, to obtain an
**      equivalent byte swapped value, which has sequence of bytes
**      from the highers order to the lowest order.
*/
#ifdef LITTLE_ENDIAN_INT
#define BT_SWAP_BYTES_MACRO(i) ( (i <= 0xFF) ? (i) : ( (i <= 0xFFFF) \
  ? (((i & 0xFFFF) >> 8) | ((i & 0xFF) << 8)) \
    : (( i <= 0xFFFFFF ) \
    ? (((i & 0xFFFFFF) >> 16) | ((i & 0xFFFF) << 16)) \
      : (((i & 0xFFFFFFFF) >> 16) | ((i & 0xFFFF) << 16)))))
#else
#define BT_SWAP_BYTES_MACRO(i) (i)
#endif

#define BT_BYTE_NUMS_MACRO(i) ((i <= 0xFF) \
        ? (1) : ((i <= 0xFFFF) ? (2) \
                  : ((i <= 0xFFFFFF) ? (3):(4))))


FUNC_EXTERN VOID BTand(
		i4		    size,
		char *              mask,
		char *              vector
);

FUNC_EXTERN VOID BTnot(
		i4		    size,
		char *              vector
);

FUNC_EXTERN VOID BTclear(
		i4		    pos,
		char *              vector
);

FUNC_EXTERN i4  BTcount(
		char *              vector,
		i4		    size
);

FUNC_EXTERN i4  BThigh(
		char *              vector,
		i4		    size
);

FUNC_EXTERN i4  BTnext(
		i4		    n,
		char *              vector,
		i4		    size
);

FUNC_EXTERN VOID BTor(
		i4		    size,
		char *              mask,
		char *              vector
);

FUNC_EXTERN VOID BTxor(
		i4		    size,
		char *              mask,
		char *              vector
);

FUNC_EXTERN VOID BTset(
		i4		    pos,
		char *              vector
);

FUNC_EXTERN bool BTsubset(
		char *              vector,
		char *              set,
		i4		    size
);

FUNC_EXTERN bool BTtest(
		i4		    pos,
		char *              vector
);

FUNC_EXTERN VOID BTclearmask(
		i4		    pos,
		char *              mask,
		char *              vector
);

# endif /* BT_HDR_INCLUDED */

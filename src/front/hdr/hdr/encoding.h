/*
**	static	char	Sccsid[] = "%W% %G%";
*/

/*
** encoding.h
**
** masks and constants used in encoding binary data to a text field,
** and then decoding it.
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
**	11/10/89 (dkh) - Added definition of MAX_B1CFTXT for B1.
*/


#ifndef EBCDIC
# define	SPACE		041
#else
# define	SPACE		0x81
#endif
# define	MASK0_1		0374
# define	MASK2_7		0003
# define	MASK0_3		0360
# define	MASK4_7		0017
# define	MASK0_5		0300
# define	MASK6_7		0077

/*
** constants for maximum sizes of buffers for binary and text data.
*/
# ifdef SEINGRES
# define	MAX_CFBIN	186
# define	MAX_CFTXT	248
# else
# define	MAX_CFBIN	1470
# define	MAX_CFTXT	1960
# define	MAX_B1CFTXT	1900	/*  This must be less than MAX_CFTXT.
					**  If not, code must be changed.
					*/
# endif

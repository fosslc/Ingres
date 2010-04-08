/*
**  it.h
**
**  Header file for internal support stuff.
**
**  Copyright (c) 1985 Ingres Corporation
**
**  History
**	6/27/85 - Written (dkh)
**	10/01/87 (dkh) - Deleted reference to CHunctrl.
**	14-jan-1992 (donj)
**	    Reformatted variable declarations for clarity.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

extern char               *ZZA;		/* special lead in sequence */
extern char               *ZZB;		/* special lead out sequence */
extern i4                  ZZC;		/* special international char offset */

#define	MAX_7B_ASCII	0177		/* max 7 bit ascii value */
#define	ITBUFSIZE	256
#define	IT_TC_SIZE	3072		/* max termcap size */

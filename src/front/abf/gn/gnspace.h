/*
**Copyright (c) 1987, 2004, 2010 Ingres Corporation
** All rights reserved.
*/

/**
** Name:	gnspace.h - SPACE structure
**
** Description
**	SPACE structure which gives pertinent details concerning a
**	numbered name space.
**
** History
**	19-Jul-2010 (frima01) Bug 124097
**	    Increase NAMEBUFFER to 267 for long identifier plus prefix.
*/

typedef struct _n_sp
{
	struct _n_sp *next;	/* link for list of existing spaces */
	VOID (*afail)();	/* allocation failure routine */
	PTR htab;		/* hash table */
	i4 gcode;		/* number of this space */
	char *pfx;		/* prefix */
	i4 pfxlen;		/* pfx to save constant recomputing */
	i4 ulen;		/* uniqueness length COUNTING pfx */
	i4 tlen;		/* truncation length COUNTING pfx */
	i4 crule;		/* case rule */
	i4 arule;		/* non-alphanumeric rule */
	i2 tag;
} SPACE;

#define TABSIZE 600	/* hash table size */
#define NAMEBUFFER 267
#define SHIFTLEN 3

/*
** minimum allowable uniqueness length.  Made long enough that
** partial characters for multi-character character sets won't
** be a problem
*/
#define MINULEN 2

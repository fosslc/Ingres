/*
**  dsmapout.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	01-dec-93 (smc)
**		Bug #58882
**		Made addr a portable PTR
**              Dummied out hash cast when linting to avoid a plethora of
**              lint truncation warnings on machines where i4 < ptr,
**		which can safely be ignored.
**	15-jan-1996 (toumi01; from 1.1 axp_osf port)
**		(schte01) added 640502 change 06-apr-93 (kchin)
**		Added a couple of changes to fix formindex problem due to
**		pointer being truncated when porting to 64-bit platform,
**		axp_osf.
**	28-feb-2000 (somsa01)
**		Added MXSZERR, which is an error that the size is greater
**		than 2GB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** NOTE:
**	we cope with addresses by explicitly turning them into char *
**	before adding an offset.  The offsets are coming from DStemplates
**	which are presumed to be byte offsets.
*/

/* DSset returns the address of the pointer field */
# define DSset(s, p)	((PTR **) (((char *) s) + (p)->ds_off))

/* DSget returns the address of the relocatable offset */
# define DSget(s, p)	((PTR *) (((char *) s) + (p)->ds_off))

/* DSaddrof returns the pointer value */
# define DSaddrof(s, p)	(* DSset(s, p))
# define DSsaddrof(s, p)	*((char **)(((char *) s) + (p)->ds_off))

/* DSsizeof returns number of array elements or union tag */
# define DSsizeof(s, p) (* (i4 *) (((char *) s) + (p)->ds_sizeoff))

/*
** hash function: fold two "halves" of address together with exclusive OR,
** then take low order bits.  Shifting low part of address loses
** significant bits on the PC.  XOR'ing in the high word is an attempt to
** make up for alignment restrictions which might make the low few bits
** 0 for most addresses on large machines.
**
** hash(s) must produce an integer in the range 0 to HASHTAB_SZ - 1
*/
# ifndef LINT
# define hash(s)	(((((long)s) >> 16) ^ ((long)s)) & 0xFF)
# else
# define hash(s)	0
# endif /* LINT */

# define HASHTAB_SZ	256

# define	DSCIRCL		1
# define	OPENERR		2
# define	IDNTFND		3
# define	INSTERR		4
# define	MXSZERR		5


typedef struct DShash {
	PTR	addr;		/* addr of the data structure */
	PTR	offset;		/* position of the data structure in the shared
				** data region
				**/
	struct 	DShash	*next;	/* point to the next linked node */
} DSHASH;

/* Tag for memory allocation */
# define	DSMEMTAG	42

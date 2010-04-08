/*
**  vfnmuniq.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**  static	char	Sccsid[] = "@(#)vfunique.h	30.1	12/3/84";
**	03/13/87 (dkh) - Added support for ADTs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



# define	FOUND			0
# define	NOT_FOUND		1

# define	HASH_TBL_SIZE		17

# define	AFIELD			0
# define	ATBLFD			1

extern	bool	vfuchkon;

struct VFSYMTBL
{
	i4	length;
	char	name[FE_MAXNAME + 1];
	struct	VFSYMTBL	*next;
};

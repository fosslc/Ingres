# include	<compat.h>
# include	<gl.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		TEglob.c
**
**	Function:
**		
**
**	Arguments:
**
**	Result:
**		Initialize TE global variables.  These variables used to be
**		in the termdr.
**
**	Side Effects:
**		None
**
**	History:
**		10/83 - (mmm)
**			written
**
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/


# ifdef ALLTERM
bool	AM = 0,	BS = 0,	CA = 0,	DA = 0,	DB = 0,	EO = 0,	GT = 0,
	HZ = 0,	IN = 0,	MI = 0,	MS = 0,	NC = 0,	OS = 0,	UL = 0,
	XN = 0,	NONL = 0, UPCSTERM = 0, normtty = 0, _pfast = 0;
# else
bool	UPCSTERM = 0;
# endif

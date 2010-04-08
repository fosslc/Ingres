/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<oslkw.h>


FUNC_EXTERN KW *iskeyword();	/* from osl directory */

/**
** Name:	vqkeywrd.c - Keyword discriminator
**
** Description:
**	This file defines:
**		IIVQikwIsKeyWord - return whether or not a name is 
**				a keyword for 4GL/SQL
**
** History:
**	01/04/90 (tom)	- created
**/


/*{
** Name:	IIVQikwIsKeyWord	- is it a keyword
**
** Description:
**	Return TRUE if the given token is a keyword in 4GL/SQL
**
** Inputs:
**	char *name;	- name to test
**
** Outputs:
**	Returns:
**			TRUE - if it is a keyword
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	10/04/89 (tom) - created
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
bool
IIVQikwIsKeyWord(name)
char *name;
{
	KW *kw;

	kw = iskeyword(name);

	if (kw != NULL && kw->kwtok != 0)
	{
		return (TRUE);
	}

	return (FALSE);
}



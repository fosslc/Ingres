/*
**	Copyright (c) 2004 Ingres Corporation
*/

# ifndef	IILANG_H
# define	IILANG_H

/**
** Name:	iilang.h  -	Contains defintion of IIlanguage.
**
** Description:
**	Contains definition of IIlanguage
**
** History:
**	20-may-1987 (Joe)
**		Took out of run30.h to be used in tbacc30 also.
**	09/23/89 (dkh) - Porting changes integration.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** If a user has not repreprocessed since 3.0 then they will go through
** the calls that reference IIlanguage.  This global was removed as it is
** not needed within the RTS (only non-C layers).  Changed it into a macro
** that will never set the current language (ie, IIlang will ignore a negative
** language) but will return the current host language.
*/

FUNC_EXTERN 	i4	IIlang();
# define IIlanguage	IIlang((i4) -1)

# endif /* IILANG_H */

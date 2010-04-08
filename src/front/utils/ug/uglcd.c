/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<nm.h>
#include 	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:	uglcd.c -	 Front-End Return Language Code Module.
**
** Description:
**	Contains the routine that sets and returns the language code for
**	the front-ends.  Defines:
**
**	iiuglcd_langcode()	return language code.
**
** History:
**	Revision 6.0  87/04/09  peter
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static	i4	language = 0; 

/*{
** Name:	iiuglcd_langcode() -	 Return the Language Code.
**
** Description:
**	Returns the value of the current language to be used by the message
**	routines for this user.  It gets the value of langcode from ERlangcode,
**	and then stores it in a static variable (language).  If a bad language
**	value is given, the language is set to English, and an error is
**	printed to the user.
**
** Returns:
**	{nat}  Value of language code, to use in calls to ERlookup.
**
** Side Effects:
**	Sets statics done and language.
**
** History:
**	09-apr-1987 (peter)
**		Written.
**	05/07/87 (dkh) - Changed call to iiugerr() to IIUGerr().
**	06/90 (jhw) - Add param. count to 'IIUGerr()' call!
*/
i4
iiuglcd_langcode()
{
    if (language == 0)
    {
	if (ERlangcode((char *)NULL, &language) != OK)
	{ /* Bad language.  Get it's name and use English */
	    char	*nm;

	    if ( ERlangcode(ER_DEFAULTLANGUAGE, &language) != OK )
	    { /* ... should not happen unless a coding error occurred. */
		return 0;
	    }
	    NMgtAt(ERx("II_LANGUAGE"), &nm);
	    IIUGerr(E_UG0001_Bad_Language, UG_ERR_ERROR, 1, (PTR)nm);
	}
    }
    return language;
}


/*{
** Name:	iiuglcd_check() -	 Validate the Language Code.
**
** Description:
**	Sets the static language variable to the language code to be 
**	used by other message functions.
**	It gets the value of langcode from ERlangcode(), and
**	stores it in a static variable (language) which iiuglcd_langcode()
**      also uses.  If a bad language is specified in II_LANGUAGE,
**	the language is set to English, an error message is output 
**	(in English), and an error status code is returned.
**
** Returns:
**	FAIL if an invalid langauge was specified in II_LANGUAGE
**	OK if OK
**
** Side Effects:
**	Sets static variable 'language'.
**
** History:
**	31-mar-95 (brogr02) - adapted from iiuglcd_langcode()
*/
STATUS
iiuglcd_check()
{
	if (ERlangcode((char *)NULL, &language) != OK)
	{ /* Bad language.  Get its name and use English */
	    char	*nm;

	    if ( ERlangcode(ER_DEFAULTLANGUAGE, &language) == OK )
	    { 
		/* Leave language set to the default value, so 
		   IIUGerr() will work */
	        NMgtAt(ERx("II_LANGUAGE"), &nm);
	        IIUGerr(E_UG0001_Bad_Language, UG_ERR_ERROR, 1, (PTR)nm);
	    }
	    return FAIL;
	}
        return OK;
}

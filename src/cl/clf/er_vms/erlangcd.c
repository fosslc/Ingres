/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <nm.h>
#include    <er.h>
#include    <cv.h>
#ifndef	VMS
#include    <si.h>	    /* Needed for "erloc.h", if not VMS */
#endif
#include    <cs.h>	    /* Needed for "erloc.h" */
#include    "erloc.h"

/*{
** Name:	ERlangcode	- Return internal code for language
**
** Description:
**	This procedure returns internal code of language in parameter 'code',
**	when passed a string representing the language.	This internal code 
**	is used to discriminate internally language in program.	
**	If parameter 'language' is NULL pointer, return default language
**	code (code of ER_DEFAULTLANGUAGE or II_LANGUAGE).
**
** Inputs:
**	language	string pointer to language name
**
** Outputs:
**	code		internal code for input language
**	Returns:
**		OK
**		ER_BADLANGUAGE		cannot find language.
**	Exceptions:
**		none
**
** Side Effects:
**	none
** History:
**	01-Oct-1986 (kobayashi) - first written.
**	01-Feb-1993 (teresal) - replace call to STcopy with STlcopy to
**				match unix porting change.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/
STATUS
ERlangcode(language,code)
char *language;
i4 *code;
{
i4 i;
char *nm;
char lngname[ER_MAXLNAME + 1];

    if (language == (char *)NULL)
    {
	NMgtAt("II_LANGUAGE",&nm);
	if ((nm != NULL) && (*nm != EOS))
	{
	    /* language is strings being found by 'NMgtat' */

	    language = nm;
	}
	else
	{
	    /* 
	    ** It changes language to internal default, as 'II_LANGUAGE'
	    ** isn't found.
	    */
	    
	    language = ER_DEFAULTLANGUAGE;
	}
    }

    /* change language name to lower case */

    _VOID_ STlcopy(language,lngname,ER_MAXLNAME);
    CVlower(lngname);

    /* search the code agree with language string */

    for(i = 0; ER_langt[i].language != (char *)NULL; i++)
    {
        if (STcompare(lngname,ER_langt[i].language) == 0)
        {
	    *code = ER_langt[i].code;
	    return(OK);
	}
    }
    return(ER_BADLANGUAGE);
}

/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <nm.h>
#include    <er.h>
#include    <cs.h>
#include    <cv.h>
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
**	01-Oct-1990 (anton)
**	    Include cs.h for reentrancy support
**	21-mar-1991 (seng)
**	    Change STcopy to STlcopy since that seems to be the intention
**	    of the programmer.  Integrated from 6.3/04 port.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
ERlangcode(char *language, i4  *code)
{
i4  i;
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

    STncpy(lngname, language, ER_MAXLNAME);
    lngname[ ER_MAXLNAME ] = EOS;
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

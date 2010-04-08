/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
# include   <compat.h>
# include   <gl.h>
# include   <st.h>
# include   <er.h>
#ifndef	VMS
#include    <si.h>	    /* Needed for "erloc.h", if not VMS */
#endif
# include   <cs.h>	    /* Needed for "erloc.h" */
# include   "erloc.h"

/*{
**  Name:   ERlangstr - get language string corresponding to internal language
**				code.
**
**  Description:
**	This procedure sets language string corresponding to internal language 
**	code.
**	This function can make directory path for each language.
**	Basically, this function has the opposite function to ERlangcode.
**
**  Input:
**		i4 code	internal language code
**		char *str	buffer to set language string
**			(It must define for the length by macro ER_MAX_LANGSTR)
**
**  Output:
**		none
**	Return:
**	    status  --- OK 			success
**               	ER_BADLANGUAGE		missing language code number
**
**	EXception:
**		none
**  Sideeffect:
**	    This function sets the language string (ex. "english") to pointed
**	    buffer.
**  History:
**	17-Apr-1987 (kobayashi) - first written
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

STATUS
ERlangstr(code,str)
i4 	code;
char	*str;
{
    i4 i;

    for (i = 0; ER_langt[i].language != (char *)NULL; ++i)
    {
	if (ER_langt[i].code == code)
	{
		STcopy(ER_langt[i].language, str);
		return(OK);
	}
    }
    /* not found message */
    return(ER_BADLANGUAGE);
}

/*
**Copyright (c) 2004 Ingres Corporation
*/
# include   <compat.h>
# include   <gl.h>
# include   <er.h>
# include   <cs.h>
# include   <st.h>
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
**	01-Oct-1990 (anton)
**	    Include cs.h for reentrancy support
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
ERlangstr(code,str)
i4  	code;
char	*str;
{
    i4  i;

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

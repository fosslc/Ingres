/*
**  include files
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#define sepst_c

#include <sepdefs.h>
#include <fndefs.h>

/*
** History:
**	05-jul-1992 (donj)
**	    Created.
**	04-feb-1993 (donj)
**	    Included lo.h because sepdefs.h now references LOCATION type.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

char
*FIND_SEPstring(char *token,char *string)
{
    i4                     token_len ;
    i4                     string_len ;
    char                  *cp = NULL ;
    char                  *pp = NULL ;
    char                  *tp = NULL ;

    if (token == NULL || *token == EOS)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr,ERx("FIND_SEPstring> token is null\n"));
	return (NULL);
    }
    if (string == NULL || *string == EOS)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr,ERx("FIND_SEPstring> string is null\n"));
	return (NULL);
    }
    string_len = STlength(string);

    if ((token_len = STlength(token)) < string_len)
    {
	if (tracing&TRACE_PARM)
	    SIfprintf(traceptr
		     ,ERx("FIND_SEPstring> token <%s> is too small (%d chars)\n")
		     ,token,token_len);

	return (NULL);
    }

    for (tp = token, cp = NULL, pp = NULL
	;(*tp != EOS)&&(cp == NULL)&&(token_len-- >= string_len)
	;CMnext(tp))
    {
	if ((STbcompare(tp, string_len, string, string_len, TRUE) == 0)
	    &&((pp == NULL)||CMoper(pp)||CMwhite(pp)))
	{
	    cp = tp;
	}
	pp = tp;
    }

    return (cp);
}

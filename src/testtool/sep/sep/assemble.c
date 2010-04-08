/*
**      assemble_line
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**     4-jan-1994 (donj)
**      Added some parens, re-formatted some things qa_c complained
**      about.
**    17-may-1994 (donj)
**	VMS needs <si.h> for FILE structure declaration.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#define assemble_c

#include <fndefs.h>

GLOBALREF    char         *lineTokens [] ;

i4
assemble_line(i4 tokens, char *buffer)
{
    register char         *ct = buffer ;
    i4                     i ;
    i4                     j ;
    i4                     toklen ;

    if (tokens > 1)
    {
	for (i = 1; i < tokens; i++)
	{
	    STcopy(lineTokens[i],ct);
	    toklen = SEP_CMstlen(lineTokens[i],0);

	    for (j=0; j<toklen; j++)
	    {
		CMnext(ct);
	    }

	    CMcpychar(ERx(" "),ct);
	    CMnext(ct);
	}
    }
    *ct = EOS;
}

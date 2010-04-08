/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>
#include <termdr.h>

#define evalif_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	26-apr-1994 (donj)
**	    Created.
**    17-may-1994 (donj)
**      VMS needs <si.h> for FILE structure declaration.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-apr-2001 (mcgem01)
**	    Increase maximum length of an if statement to 128 characters.
*/

/*
**  Reference global variables
*/

GLOBALREF    char          msg [] ;
GLOBALREF    char         *ErrC ;

GLOBALREF    i4            tracing ;
GLOBALREF    i4            testGrade ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    bool          batchMode ;

bool
Eval_If(char *commands[])
{

    STATUS                 eval_err ;
    char                   buffer [128] ;
    char                  *tokptr [12] ;
    register i4            tok_ctr = 0 ;
    i4                     answer = TRUE ;


    for (tok_ctr=0; tok_ctr<12; tok_ctr++) tokptr[tok_ctr] = 0;

    *buffer = EOS;
    for (tok_ctr=2; commands[tok_ctr]; tok_ctr++)
	STcat(buffer, commands[tok_ctr]);	/* Condense the expression. */


    *msg = EOS;

    answer = SEP_Eval_If( buffer, &eval_err, msg);

    if (eval_err != OK)
    {
	testGrade = FAIL;
	if (*msg == EOS)
	{
	    disp_line(STpolycat(4,ErrC,ERx("Evaluate Expresion <"),buffer,
				ERx(">"),msg),0,0);
	}
	else
	{
	    disp_line(msg,0,0);
	}
	append_line(msg,1);
        return(FAIL);
    }

    if (answer == TRUE)
    {
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }

}

/*
**      shell_cmmd
**
**      Description:
**      equivalent to get_command but for shell mode, it prompts the
**      user for the command
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**    10-may-1994 (donj)
**	Took out obsolete code.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <cv.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <termdr.h>

#define shellcmd_c

#include <sepdefs.h>
#include <fndefs.h>

/*
**      Global variables
*/
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char          buffer_2 [] ;
GLOBALREF    WINDOW       *statusW ;
GLOBALREF    char         *lineTokens [] ;

#define     BLANKLINE   -9900

/*
**	shell_cmmd
**  
**	Description:
**	equivalent to get_command but for shell mode, it prompts the
**	user for the command
*/

STATUS
shell_cmmd(char *cmmd)
{
    STATUS                 ret_val = OK ;
    i4                     tokens ;
    char                   prompt [5] ;

    STcopy(( cmmd ? cmmd : SEP_PROMPT ),prompt );

    while ( ret_val == OK )
    {
	if (STcompare(cmmd,KFE_PROMPT) == 0)
	{
	    disp_prompt(prompt,NULL,NULL);
	    getstr(buffer_1);
	}
	else
	{
	    get_string(statusW, prompt, buffer_1);
	}	    

	if (*buffer_1 != EOS)
	{
	    if (( ret_val = ( STbcompare(ERx("exit"), 4, buffer_1, STlength(buffer_1), TRUE) 
			      ? OK : ENDFILE )) == OK )
	    {
		STprintf(buffer_2,ERx("%s %s"),prompt,buffer_1);
		tokens = TEST_LINE;
		break_line(buffer_2,&tokens,lineTokens);
		set_sep_state(tokens);
		assemble_line(tokens, buffer_1);

		break;
	    }
	}
    }

    return ( ret_val == OK ? ( lineTokens[1] ? OK : BLANKLINE ) : ret_val );
}

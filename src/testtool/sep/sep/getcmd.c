/*
**  get_command
**
**  Description:
**  subroutine to read a line from the test script until first token
**  in line matches paramater 'cmmdkey',
**  parameters,
**  fdesc   -   file pointer for test script
**  cmmdkey -   value to be compared against first token of test script line
**
**  Note: line read from the test script will be divided in tokens
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    06-Mar-2002 (hanje04)
**	    Bug 107266
**	    When checking for '.' at the begining of a line, also check to
**	    see that the next character is an 'e' or and 'i' (for .if, .else
**	    or .endif) to avoid problems with fill files containing lines 
**	    starting with '.'.
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <termdr.h>

#define getcmd_c

#include <sepdefs.h>
#include <fndefs.h>

/*
**      Global variables
*/
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char          buffer_2 [] ;
GLOBALREF    char         *lineTokens [] ;
GLOBALREF    i4            sep_state ;
GLOBALREF    i4            sep_state_old ;
GLOBALREF    bool          ignore_silent ;
GLOBALREF    WINDOW       *statusW ;
GLOBALREF    bool          updateMode ;

/* pointer to legal commands b-tree */
GLOBALREF    i4            testLine ;

#define     BLANKLINE   -9900

/*
** Name: get_command
**
** History:
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_commmand, next_record
*/
STATUS
get_command(FILE *fdesc,char *cmmdkey, char print)
{
    STATUS                 ioerr ;
    STATUS                 found ;
    i4                     tokens ;
    char		   message [TEST_LINE] ;
    char                  *ignoring = ERx("Ignoring line %d ...") ;
    bool		   sep_cmd = FALSE ;
    bool		   tm_cmd = FALSE ;
    bool		   con_cmd = FALSE ;
    bool		   dot_cmd = FALSE ;

    if (cmmdkey == NULL)
    {
	testLine++;
	ioerr =  next_record(fdesc,print);
	if (ioerr == OK)
	{
	    STcopy(buffer_1,buffer_2);
	    break_line(buffer_2,&tokens,lineTokens);
	    set_sep_state(tokens);
	}
	return(ioerr);
    }
    else if (!(sep_cmd   = (STcompare(cmmdkey, SEP_PROMPT)     == 0)))
	    if (!(tm_cmd = (STcompare(cmmdkey, TM_PROMPT)      == 0)))
		 dot_cmd = (STcompare(cmmdkey, CONTROL_PROMPT) == 0);


    found = FAIL;
    do
    {
	testLine++;
	ioerr = next_record(fdesc,print);
	if ((ioerr != FAIL) && (ioerr != ENDFILE))
	{
	    char	  *cp1 = buffer_1 ;
	    char	  *cp2 = buffer_2 ;
	    char	  *cp3 = &buffer_1[1] ;

	    if ((sep_cmd || tm_cmd || dot_cmd) && *cp1 == '.' && \
						/* 
						** if it's a '.' check it's 
						** also an if, else or endif
						*/
						 (*cp3 == 'i' || *cp3 == 'e'))
	    {
		CMcpyinc(cp1,cp2);
		CMcpychar(ERx(" "),cp2);
		CMnext(cp2);
		STcopy(cp1, cp2);
		con_cmd = TRUE;
	    }
	    else
	    {
		STcopy(buffer_1, buffer_2);
		con_cmd = FALSE;
	    }

	    break_line(buffer_2, &tokens, lineTokens);
	    set_sep_state(tokens);

	    if ((sep_state&(BLANK_LINE_STATE|IN_ENDING_STATE|IN_COMMENT_STATE))
		|| ((sep_state&IN_COMMENT_STATE) == 0
		    && (sep_state_old&IN_COMMENT_STATE)))
	    {
		if ((sep_state&IN_COMMENT_STATE)
		    || (sep_state_old&IN_COMMENT_STATE))
		    append_line(buffer_1,0);

		if (!ignore_silent)
		{
		    STprintf(message,ignoring,testLine);
		    put_message(statusW, message);
		}
	    }    
	    else
	    if ((tokens == 0)
		|| ((sep_state&IN_CANON_STATE) && (sep_cmd||tm_cmd||dot_cmd)))
	    {
		if (updateMode)
		    append_line(buffer_1,0);

		if (!ignore_silent)
		{
		    STprintf(message,ignoring,testLine);
		    put_message(statusW, message);
		}    
	    }    
	    else
	    {
		if (con_cmd || (STcompare(lineTokens[0], cmmdkey) == 0))
		{
		    assemble_line(tokens, buffer_1);
		    found = OK;
		}
		else
		{
		    if (updateMode)
			append_line(buffer_1,0);

		    if (!ignore_silent)
		    {
			STprintf(message,ignoring,testLine);
			put_message(statusW, message);
		    }
		}
	    }
	}
    } 
    while (ioerr == OK && found == FAIL);

    if (ioerr != OK)
	return(ioerr);
    else
    {
	if (lineTokens[1])
	    return(OK);
	else
	if (STcompare(FILL_PROMPT,cmmdkey) == 0)
	    return(OK);
	else
	    return(BLANKLINE);
    }
}

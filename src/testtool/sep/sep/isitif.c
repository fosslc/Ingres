/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <cm.h>
#include <lo.h>
#include <er.h>

#define isitif_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**      26-apr-1994 (donj)
**          Created
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
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
*/

/*
**  defines
*/

/*
**  Reference global variables
*/

GLOBALREF    char          holdBuf [] ;
GLOBALREF    char          buffer_1 [] ;
GLOBALREF    char          buffer_2 [] ;
GLOBALREF    char         *lineTokens [] ;

GLOBALREF    bool          ignore_silent ;
GLOBALREF    bool          shellMode ;

GLOBALREF    i4            tracing ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    CMMD_NODE    *sepcmmds ;

/*
**  Is_it_If_statement
*/

i4
Is_it_If_statement(FILE *testFile,i4 *counter,bool Ignore_it)
{
    char                  *cp1 = NULL ;
    char                  *cp2 = NULL ;
    i4                     cmd ;
    bool                   yes_its_if = FALSE ;
    bool                   yes_its_tc = FALSE ;

    cmd = 0;
    if ((!shellMode)&&(SEP_CMstlen(lineTokens[0],0) > 1)&&
	(CMcmpcase(lineTokens[0],ERx(".")) == 0))
    {
	cp1 = buffer_1 ;		/* Fix the ".if" statement. */
	cp2 = buffer_2 ;

	STcopy(buffer_1, holdBuf);
	CMcpyinc(cp1,cp2);
	CMcpychar(ERx(" "),cp2);
	CMnext(cp2);
	STcopy(cp1, cp2);
	STcopy(buffer_2, buffer_1);
	break_line(buffer_2, counter, lineTokens);
    }

    cmd = classify_cmmd(sepcmmds, lineTokens[1]);
    if (cmd == IF_CMMD || cmd == ELSE_CMMD || cmd == ENDIF_CMMD)
    {
	yes_its_if = TRUE;
    }
    else if (cmd == TEST_CASE_CMMD || cmd == TEST_CASE_END_CMMD)
    {
	yes_its_tc = TRUE;
    }

    if (yes_its_if || yes_its_tc)
    {
	append_line(holdBuf, 0);

	if (Ignore_it)
	    ignore_silent == TRUE;

	if (yes_its_if)
	{
	    process_if(testFile, lineTokens, cmd);
	}
	else if (yes_its_tc)
	{
	    if (cmd == TEST_CASE_CMMD)
	    {
		testcase_start(&lineTokens[1]);
	    }
	    else if (cmd == TEST_CASE_END_CMMD)
	    {
		testcase_end(FALSE);
	    }
	}

	if (Ignore_it)
	    ignore_silent == FALSE;
    }
    else
    {
	cmd = 0;
    }

    return (cmd);
}

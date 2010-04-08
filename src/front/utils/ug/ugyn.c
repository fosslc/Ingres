/*
** Copyright (c) 1987, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    ugyn.c -	Front-End Utility Check User Affirmation Response.
**
** Description:
**	Contains the routine used to check a user affirmation response.
**	Defines:
**
**	IIUGyn()	check user affirmation response.
**
** History:
**	Revision 6.0  87/07/29  peter
**	Initial revision.
**	08/23/87 (dkh) - Fixed to use "==" instead of "!=" in comparison
**			 after call to CVlower().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	24-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
**/

/*
** Name:    IIUGyn() -	Check User Affirmation Response.
**
** Description:
**	This routine checks a string and verifies it as either
**	a valid 'yes' or a 'no'.  This is put in one place to 
**	localize the internationalization efforts.  The routine
**	is passed a string, which it checks against the valid
**	5 'yes' strings and 5 'no' strings which are allowed
**	in the erug.msg file.  The comparison is done without
**	regard to case.  It returns two return values -- a boolean
**	function return value, and an optional full status return
**	if requested.
**
**	The boolean return is TRUE if a valid 'yes' response is
**	found, and FALSE in all other cases.  Therefore, a null
**	response or an empty response will be considered a 'no'.
**
**	If the status value is requested, the return values are
**	as follows:
**
**		E_UG0004_Yes_Response	- valid 'yes' found.
**		E_UG0005_No_Response	- valid 'no' found.
**		E_UG0006_NULL_Response	- empty response string,
**					  or NULL string sent.
**		E_UG0007_Illegal_Response	- invalid string, which
**					  does not compare with any of
**					  the 'yes' or 'no' strings.
**
**	Upon return, you may send the status value to IIUGer to
**	be printed.  No parameters are needed.
**
**	The IIUGver routine should be used if you want a prompt
**	and response evaluation done.
**
** Inputs:
**	response	{char *}  Buffer containing user response.
**
** Outputs:
**	status	{STATUS *}  pointer to a STATUS that will be set to
**			  one of the statuses above.  If NULL, only
**			  the boolean return value is given.
**
** Returns:
**	{bool}  TRUE if yes string found, FALSE otherwise.
**
** History:
**	29-jul-1987 (peter)
**		Written.
**	08/23/87 (dkh) - Fixed to use "==" instead of "!=" in comparison
**			 after call to CVlower().
*/

bool
IIUGyn (char *response, STATUS *status)
{
    static const ER_MSGID	yeses[] = { F_UG0001_Yes1,
						F_UG0002_Yes2,
						F_UG0003_Yes3,
						F_UG0004_Yes4,
						F_UG0005_Yes5
				};
    static const ER_MSGID	nos[] = { F_UG0006_No1,
						F_UG0007_No2,
						F_UG0008_No3,
						F_UG0009_No4,
						F_UG000A_No5
				};

    ER_MSGID	retstat;

    if (response == NULL || *response == EOS)
	retstat = E_UG0006_NULL_Response;
    else
    { /* Response is given */
	char	testbuf[FE_PROMPTSIZE+1];

	STlcopy(response, testbuf, sizeof(testbuf)-1);
	STtrmwhite(testbuf);
	CVlower(testbuf);
	if (*testbuf == EOS)
	{ /* empty response */
	    retstat = E_UG0006_NULL_Response;
	}
	else
	{
	    register i4	i;

	    retstat = E_UG0007_Illegal_Response;
	    for (i = 0 ; i < (sizeof(yeses)/sizeof(yeses[0])) ; ++i)
	    {
		if (STequal(testbuf, ERget(yeses[i])))
		{
		    retstat = E_UG0004_Yes_Response;
		    break;
		}
		else if (STequal(testbuf, ERget(nos[i])))
		{
		    retstat = E_UG0005_No_Response;
		    break;
		}
	    } /* end for */
	}
    }

    if (status != NULL)
	*status = (STATUS)retstat;

    return (bool)(retstat == E_UG0004_Yes_Response);
}

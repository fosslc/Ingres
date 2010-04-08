/*
**	dump.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	dump.c	- File contains dump routines.
**
** Description:
**	This file contains routines that dump various information
**	to support the testing flags.  Routines defined here are:
**	- FDdmpinit - Dump initialization.
**	- FDvfrbfdmp - Return dump FILE pointer for VIFRED/RBF.
**	- FDdmpcur - Dump the current screen.
**	- FDdmpmsg - Output special message to dump file.
**
** History:
**	02/04/85 (???) - Version 3.0 work.
**	04/22/85 DRH	Added FDdmpmsg routine to dump messages to test
**			output file.
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	09/21/88 (dkh) - Fixed venus bug 3461.
**	05-jan-89 (bruceb)
**		Added new parameter to call on FTdumpscr to indicate that
**		the call didn't originate with Printform.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Changed arguments in FDdmpmsg() from i4
**              to PTR for 64-bit portability. (axp_osf port)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<er.h>
# include	<adf.h>
# include       <ft.h>
# include       <fmt.h>
# include       <frame.h>

GLOBALREF	FILE	*fddmpfile;

#ifdef SEP

#include <tc.h>

GLOBALREF	TCFILE	*IIFDcommfile;

#endif /* SEP */


/*{
** Name:	FDdmpinit - Dump initialization.
**
** Description:
**	Assigns passed in FILE pointer to a global FILE pointer used
**	by other parts of the forms system to generate test results.
**	Causes the current frame to be dumped to the file at the exit
**	to a putfrm call.  Used to generate testable output.
**
** Inputs:
**	file	FILE pointer to set for dump output.  No assignment if
**		this is NULL.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDdmpinit(file)
FILE	*file;
{
	if (file == NULL)
		return;
	fddmpfile = file;
}



/*{
** Name:	FDvfrbfdmp - Return dump FILE pointer to VIFRED/RBF.
**
** Description:
**	Special call from VIFRED/RBF to get the current dump FILE pointer
**	If no call to FDdmpinit() has been done before this routine
**	is called, then the NULL pointer is returned.
**
** Inputs:
**	None.
**
** Outputs:
**	file	The dump FILE pointer is returned via this parameter.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDvfrbfdmp(file)
FILE	**file;
{
	*file = fddmpfile;
}




/*{
** Name:	FDdmpcur - Dump the current screen image to the dump file.
**
** Description:
**	Dump the current screen image to the dump file by calling
**	FTdumpscr().  No dump is done if the dump FILE pointer is NULL
**	(i.e., no dump file has been set up yet).
**
** Inputs:
**	None.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDdmpcur()
{
	if (fddmpfile != NULL)
		FTdumpscr(NULL, fddmpfile, TRUE, (bool)FALSE);
#ifdef SEP
	if (IIFDcommfile != NULL)
		FTdumpscr(NULL, (FILE *)IIFDcommfile, TRUE, (bool)FALSE);
#endif /* SEP */
    
}





/*{
** Name:	FDdmpmsg - Output a message to dump file.
**
** Description:
**	Output a message to the current dump file so more meanigful
**	test outputs will appear in the test files.  Useful for
**	logging user actions and menu selections.
**	No output is done if the dump FILE pointer is NULL.
**	The maximum length of the string and the null terminator
**	can not exceed 512, as that is the size of the buffer
**	allocated on the stack.
**
** Inputs:
**	str	Message to output with possible printf like formatting
**		constructs embedded in the text.
**	a1	First argument to format in message.
**	a2	Second argument to format in message.
**	a3	Third argument to format in message.
**	a4	Fourth argument to format in message.
**	a5	Fifth argument to format in message.
**	a6	Sixth argument to format in message.
**	a7	Seventh argument to format in message.
**	a8	Eigth argument to format in message.
**	a9	Nineth argument to format in message.
**	a10	Tenth argument to format in message.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDdmpmsg( str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 )
register char	*str;	/* printf-type format string */
PTR	a1;		/* a1 - a10 are arguments to str */
PTR	a2;
PTR	a3;
PTR	a4;
PTR	a5;
PTR	a6;
PTR	a7;
PTR	a8;
PTR	a9;
PTR	a10;
{
	/*
	**  The size of the buffer is twice the maximum size of an
	**  ER string.  This should suffice to cover argument
	**  expansion.
	*/
	char	buf[ER_MAX_LEN + ER_MAX_LEN + 1];
#ifdef SEP
	char	*cptr;
#endif

	if ( fddmpfile != NULL )
	{
		STprintf( buf, str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 );
		SIfprintf( fddmpfile, ERx("\n%%%% %s"), buf );
	}

#ifdef SEP

	if ( IIFDcommfile != NULL )
	{
		STprintf( buf, str, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10 );
		TCputc('\n',IIFDcommfile);
		TCputc('%',IIFDcommfile);
		TCputc('%',IIFDcommfile);
		TCputc(' ',IIFDcommfile);
		cptr = buf;
		while (*cptr != '\0')
		{
		    TCputc(*cptr,IIFDcommfile);
		    cptr++;
		}
	}
#endif


	return;
}

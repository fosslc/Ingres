# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<pc.h>
# include	<ex.h>
# include	"pclocal.h"
# include	<PCerr.h>
# include	<si.h>

/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PCread.c
 *
 *	Function:
 *		PCread
 *
 *	Arguments:
 *		char	*command;
 *		char	*buf;
 *
 *	Result:
 *		make the system call "command" and store it's
 *			output in "buf".  If the return is terminated
 *			by a new-line it is stripped.
 *
 *		NOTE: "buf" must be large enough to hold what
 *			is returned, or you won't like the results.
 *
 *		Returns:
 *			OK		-- success
 *			PC_RD_CALL	-- one of the passed in arguments is 
 *					   NULL
 *			PC_RD_CLOSE	-- stream corrupted, should never happen
 *			PC_RD_OPEN	-- trouble establishing communication 
 *					   pipes
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
**	25-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add include for <clconfig.h>.
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	15-nov-2010 (stephenb)
**	    Include ex.h for prototyping.
 */

/* static char	*Sccsid = "@(#)PCread.c	3.2  4/11/84"; */

 




STATUS
PCread(command, buf)
char	*command;
char	*buf;
{
	FILE	*popen(),
		*output;
	char	*b;
	i4	temp;


	/*
		output set to stream which reads from command's
			standard output.
	*/

	if ((buf == NULL) || (command == NULL))
		PCstatus = PC_RD_CALL;
	else if (!(output = popen(command, "r")))
		PCstatus = PC_RD_OPEN;
	else
	{
		/*
			read characters from "output" into buf
		*/

		for (b = buf; ((temp = SIgetc(output)) != EOF);  b++)
		{
			*b = (char) temp;
		}

		if (*(b - 1)  == '\n') 
			*(b - 1) = EOS;
		else
			*b       = EOS;

		PCstatus = ((pclose(output) == -1) ? PC_RD_CLOSE : OK);
	}

	return(PCstatus);
}

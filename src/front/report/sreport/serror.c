/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h>
# include	<sglob.h>
# include	<si.h>
# include	<ex.h>
# include	<errw.h>
# include	"ersr.h"
# include	<ug.h>

/*
** Name:	s_error() -	Report Specifier Print Error Routine.
**
** Description:
**   S_ERROR - print out an error from SREPORT which contains the line
**	number and file in the message.	 This simply
**	calls the error routine IIUGerr() to write out the
**	error, which is included in the file "error.7".
**	This will stick in the line number and filename as the first two
**	parameters, so be sure to include them in the error msg.
**
**	This converts all of the argv strings into PARM structures.
**
** Parameters:
**	err_num - error number, modulo ERRBASE.
**	type - seriousness of error.
**		FATAL - fatal error.
**		NONFATAL - non fatal (continue processing).
**	arg1,arg2... - any number of string ptrs for use with
**		the error printing routines.  These will correspond
**		with the %2, %3, etc params in the error file.
**		This list must be null-terminated to flag the end of the list.
**		MAXIMUM IS 9 (INCLUDING 0) !!!
**
** Returns:
**	none.  Exits on FATAL errors.
**
**
** Side Effects:
**	Increments the global count Err_count.	If more than
**	MAXERR nonfatal errors, or one fatal error occurs, stop.
**
** Special Non-standard features:
**	This routine processes a variable length argument list.
**	It assumes that a count of the number of parameters is
**	ended with a null-terminator.
**
** Trace Flags:
**	12.0, 12.6.
**
** History:
**	6/4/81 (ps) - written.
**	8/22/83 (gac)	add trace for deleting and adding reports.
**	11/15/83 (gac)	bug 1797 fixed -- error msg doesn't bomb out.
**	11/30/83 (gac)	print line in error if nonfatal.
**	05/15/86 (jhw)	Changed to call 'FDerror()'.
**	12/21/89 (elein) Fixed call to rsyserr
**	1/10/90 (elein) ensure call to rsyserr has i4 ptr
**	04-sep-1992 (rdrane)
**		Replace call to r_syserr() with direct call to IIUGerr()
**		as UG_ERR_FATAL.
**	12-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Cast (long) 3rd... EXsignal args to avoid truncation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


/* VARARGS2 */
VOID
s_error (err_num, type, a1, a2, a3, a4, a5, a6, a7, a8, a9)
ERRNUM	err_num;
i2	type;
char	*a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	register i4	err_argc;
	char		n_buf[6];	/* string buffer for number */
	i4		lerr_num;

	/* start of routine */



	/* find the number of error parameters to send to IIUGerr */

	err_argc = 2;


	if (a1 != NULL)
	{
	   ++err_argc;
	   if (a2 != NULL)
	   {
	      ++err_argc;
	      if (a3 != NULL)
	      {
		 ++err_argc;
		 if (a4 != NULL)
		 {
		    ++err_argc;
		    if (a5 != NULL)
		    {
		       ++err_argc;
		       if (a6 != NULL)
		       {
			  ++err_argc;
			  if (a7 != NULL)
			  {
			     ++err_argc;
			     if (a8 != NULL)
			     {
				++err_argc;
				if (a9 != NULL)
				{
				  lerr_num  = err_num;
				  IIUGerr(E_SR000A_s_error_Too_many_args,
					  UG_ERR_FATAL,1,&lerr_num);
				}
			     }
			  }
		       }
		    }
		 }
	      }
	   }
	}

	CVna(Line_num, n_buf);
	IIUGerr(E_RW1000_|err_num, UG_ERR_ERROR, err_argc,
		En_sfile, n_buf, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	++Err_count;


	if (Err_count > MAXERR)
	{
		CVna(Err_count, n_buf);
		IIUGerr(E_RW1000_|999, UG_ERR_ERROR, 1, n_buf);
		type = FATAL;
	}

	if (type == FATAL)
	{
		EXsignal(EXEXIT, 2, (long)0, (long)ABORT);
	}
	else
	{
		register char	*lp;

		SIfprintf(stdout, ERget(E_SR0008_Line_in_error), Line_buf);

		for (lp = Line_buf; lp < Tokchar; ++lp)
			SIputc(*lp == '\t' ? '\t' : ' ', stdout);

		SIfprintf(stdout, ERget(E_SR0009_Error_processing_cont));
		SIflush(stdout);
	}

	return;				/* nonfatal errors only */
}

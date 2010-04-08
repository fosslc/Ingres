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
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<ex.h>
# include	<errw.h>
# include	<ug.h>

/**
** Name:	rerror.c -	Print Report Sub-System Error Module.
**
** Description:
**	Contains the routine used to record and output errors for the
**	programs of the report sub-system.
**
**	r_error()	print report sub-system error.
**
**		Revision 50.0  86/05/15	 10:53:24  wong
**		Modified to call 'FDerror()' instead of 'IIerror()'.
**
*/

/*{
** Name:	r_error() -	Print Report Sub-System Error.
**
** Description:
**   R_ERROR - print out an error from one of the report programs. It
**	calls the front-end error routine FEUGerr to write out the
**	error, which is included in the file "error.7".	 If more that
**	MAXERR nonfatal errors have occurred, the program stops.
**
**	The argument parameters are passed as a null-terminated argument
**	list.  This checks each argument to determine how many to pass to
**	IIUGerr.
**
** Parameters:
**	err_num - error number, modulo ERRBASE.
**	type - seriousness of error.
**		FATAL - fatal error.
**		NONFATAL - non fatal (continue processing).
**	arg1,arg2... - any number of string ptrs for use with the error
**			printing routines.  These will correspond with the
**			%0, %1, etc params in the error file.
**			This list must be null-terminated to flag the end
**			of the list.
**			MAXIMUM IS 9 (INCLUDING 0) !!!
**
** Returns:
**	none.  Exits on FATAL errors.
**
**
** Side Effects:
**	Increments the global count Err_count.	If more than
**	MAXERR nonfatal errors, or one fatal error occurs, stop.
**
**
** Special Non-standard features:
**	This routine processes a variable length argument list.
**	It assumes that a count of the number of parameters is
**	ended with a null-terminator.
**
** Trace Flags:
**	2.0, 2.1.
**
** History:
**	3/4/81 (ps)	written.
**	7/2/82 (ps)	add change to IIerrflag.
**	5/15/86 (jhw)	Changed to call 'FDerror()'.
**	18-jun-86 (mgw) bug # 9302
**		set Err_type = type in r_error() so that report returns
**		the proper status back to the operating system.
**	12/29/86 (rld)	Changed to call IIerrnum instead of setting IIerrflag.
**	09/07/88 (elein) JupBug 3201
**			Add call to hit return if about to exit due to errors
**	12/21/89 (elein)
**		Corrected call to r_syserr
**	1/10/90 (elein)
**		Ensure call to r_syserr is i4 ptr
**	10/13/90 (elein)
**		Move end batch close write to log file to here so
**		that batch log is closed for all types of fatal errors.
**	09/26/95 (harpa06)
**		Change the exit status to fail when a fatal error occurrs.
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Cast (long) 3rd... EXsignal args to avoid truncation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-Jun-2002 (hanal04) Bug 107927 INGEMB 66
**              Replaced call to IIUGmsg() with r_prompt() to prevent
**              core dump on Unixware when report is called from ABF.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#define BFRSIZE 80

/* VARARGS2 */
VOID
r_error (err_num, type, a1, a2, a3, a4, a5, a6, a7, a8, a9)
ERRNUM	err_num;
i2	type;
char	*a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	register i4	err_argc;
	i4 lerr_num;
        char            buffer[BFRSIZE];        /* temporary buffer */
        SYSTIME         stime;                  /* current time */
        i4         cpu;



	/* find the number of error parameters to send to IIUGerr */

	err_argc = 0;

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
				   lerr_num =  err_num;
				   r_syserr(E_RW0021_r_error_Too_many_args, &lerr_num);
				}
			     }
			  }
		       }
		    }
		 }
	      }
	   }
	}


	if (St_err_on)
	{	/* report the error */
		IIUGerr(E_RW1000_|err_num, UG_ERR_ERROR, err_argc,
			a1, a2, a3, a4, a5, a6, a7, a8, a9);
		IIerrnum(1,0); /* set error number to 0 */
		++Err_count;
	}


	if (Err_count > MAXERR)
	{
		char	n_buf[6];	/* string buffer for number */

		CVna(Err_count, n_buf);
		IIUGerr(E_RW1000_|999, UG_ERR_ERROR, 1, n_buf);
		type = FATAL;
	}

	/* Bug 9302 - return proper status to operating system in Err_type */
	Err_type = type;

	if (type == FATAL)
	{
                if (St_batch)
                {
                        /* Print out footer for batch report */
                        TMnow (&stime);
                        TMstr (&stime, buffer);
                        cpu = TMcpu();
                        IIUGmsg(ERget(E_RW13FE_batchfooter), FALSE, 2, buffer,
                                &cpu);
                }

		/* JB 3201: RW called from another program */
		if (r_xpipe != NULL && St_to_term)
		{
                    r_prompt(ERget(S_RW0025_Hit_RETURN_when_done), FALSE);
		}
		EXsignal(EXINTR, 2, (long)0, (long)ABORT); /* get out */
	}

	return;				/* nonfatal errors only */
}

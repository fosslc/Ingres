/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<stype.h> 
# include	<sglobi.h> 
# include	<errw.h> 
# include       <uigdata.h>
# include       <ui.h>
# include       <ug.h>
# include	<si.h>
# include	<tm.h> 
# include	<ex.h> 
# include	<er.h>
# include	<rglob.h>


/*
**   R_REPORT - main routine of report writer.
**	This is called by the r_main (main program of
**	the report writer) to control the overall logic of 
**	the report formatter.  This is also called by
**	the program interface to the report writer.
**
**	In either case, the calling sequence for the report
**	is a command line which looks like:
**
**	REPORT [dbname] repname [flags] [(parameters)]
**		where	dbname	is the name of the database,
**				used only if called as standalone.
**			repname is the report name.
**			flags	are optional flags.
**			parameters	are optional parameters.
**
**	Parameters:
**		argc	count of number of parameters.
**		argv	addresses of parameters.
**
**	Returns:
**		Err_type, either ABORT or NOABORT.
**
**	Called by:
**		r_main, iireport.
**
**	Trace Flags:
**		0.0, 1.0, 1.1.
**
**	History:
**		10/14/81 (ps) - written.
**		1/17/83 (ps)	add return code for r_rep_do.
**		10/28/83(nml) - changes put in to reflect UNIX 2.0 
**				updates. Removed references to run 
**				and just test value of EXdeclare on
**				return to decide whether or not to
**				do return(Err_type).
**		6/29/84	(gac)	fixed bug 2327 -- default line maximum is
**				width of terminal unless -l or -f is specified.
**		12/1/84 (rlm)	ACT reduction - call to r_reset for post load
**				phase cleanup
**		04-dec-1987 (rdesmond)
**			removed call to OOinit() to place in r_rep_load(),
**			after call to FEingres().
**		6/27/89 (elein) garnet
**			Add call for cleanup query
**              9/21/89 (elein) PC Integration
**                      Cast nats in EXsignal call
**		04-dec-89 (sylviap)
**			Check St_copyright to see if copyright banner should be
**			suppressed.
**		02-jan-90 (sylviap)
**			Added report information, such as start/end time, if 
**			  running a batch report.
**			Added call to r_fix_parl.  Moved up from r_rep_do, so
**			  if any errors occurred, report execution stops.  
**			  Needed for batch reports.
**		4/30/90 (elein)
**			Moved rfixparl call to be after Err_count check.
**			We want to check Err_copunt after rrepld now for
**			more consistent looking error messages.  Rfixparl
**			does not call r_error which is the one that sets
**			Err_count.
**		10/9/90 (elein) 33889
**			Exit error if cleanup fails in order to have a
**			consistent exit and commit status
**
**			Also moved batch footer for error case to r_error
**			so it is printed consistently.
**		16-sep-91 (seng)
**			Add CVna() before second call to r_error().
**			If condition to first call to r_error is not true,
**			Err_count will never get converted for second
**			call to r_error().  On the RS/6000, r_error() will
**			print junk for this error message.
**		7-oct-1992 (rdrane)
**			Use new constants in (s/r)_reset invocations.
**			Converted r_error() err_num value to hex
**			to facilitate lookup in errw.msg file.
**	11-jan-1996 (toumi01; from 1.1 axp_osf port)
**		Cast (long) 3rd... EXsignal args to avoid truncation.
**	17-dec-1996 (angusm)
**		Add II_4GL_DECIMAL functionality, to allow reports
**		to use different setting to II_DECIMAL (bug 75576)
**	28-Jul-1998 consi01 bug 92001
**		Set decimal symbol to . (period) when we are loading the report
**		definition in. Reset back to orginal value after the report has
**		loaded. This will allow the use of hard coded values like 9.99
**		in the script whilst still printing a , as the decimal point.
**	16-08-1999 (carsu07)
**		Moved change 436813 for bug 92001 from rreport.c to rgdouble.c.
**		This change needs to be moved to prevent bug 98262.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#define BFRSIZE 80

i4
r_report(argc,argv)
i4	argc;
char	*argv[];
{
	/* external declarations */

	FUNC_EXTERN	i4	r_exit();	/* exit handler */
	FUNC_EXTERN	ADF_CB	*FEadfcb();
	FUNC_EXTERN DB_STATUS IIUGlocal_decimal(ADF_CB *, PTR);

	/* internal declarations */

	char		buffer[BFRSIZE];	/* temporary buffer */
	EX_CONTEXT	context;
	SYSTIME         stime;             	/* current time */
	i4		cpu;
	DB_STATUS	status;
	char		cp[2];


	/* start of MAIN routine */

	if (EXdeclare(r_exit, &context))
	{
		EXdelete();
		return(Err_type);	/* ALL EXITS COME HERE */
	}

	/* start up abstract datatype facility (ADF) */
	Adf_scb = FEadfcb();
	/* Overwrite II_DECIMAL setting with II_4GL_DECIMAL */
	cp[0]=cp[1]='\0';
	status = IIUGlocal_decimal(Adf_scb, cp);
	if (status)
	{
		IIUGerr(E_RW1417_BadDecimal, 0, 1, cp);
		return(status);
	}
	IIUIck1stAutoCom();

	/* set query language of control block to QUEL in order to allow
	** the unique aggregates sumu, countu, and avgu, which are not allowed
	** in SQL.
	*/
	Adf_scb->adf_qlang = DB_QUEL;

	/* set storage tags for reset levels */
	Rst5_tag=FEgettag();
	Rst4_tag=FEgettag();

	r_init(argv);			/* set up tracers, links, etc. */


	r_crack_cmd(argc,argv);		/* crack the command line */

	FTtermsize(&En_cols, &En_lines, &En_initstr);

	/* check if copyright banner should be suppressed */
	if (!St_copyright)
		FEcopyright(ERx("REPORT"), ERx("1981"));
	if (St_batch)
	{
		/* Print out header for batch report */
		TMnow (&stime);
		TMstr (&stime, buffer); 
		IIUGmsg(ERget(E_RW13FD_batchheader), FALSE, 2, En_report,
			buffer);
	}

# ifdef CFEVMSTAT
	r_FEtrace (ERx("load entry"),En_acount);
# endif

       /* We are about to load the report so save the */
       /* decimal value and reset it to . (period)    */
 
	r_rep_load();			/* may abort on errors */
					/* then again, it may not */

      /* The report has been loaded so recover original decimal setting */

# ifdef CFEVMSTAT
	r_FEtrace (ERx("before cleanup"));
# endif

	/* post load phase cleanup */
	r_reset(RP_RESET_CLEANUP,RP_RESET_LIST_END);

# ifdef CFEVMSTAT
	r_FEtrace (ERx("after cleanup"));
# endif

	/* if no errors have been found, do the report */

	if (Err_count == 0)
	{	

		r_fix_parl(FALSE);     /* prompt for runtime parameters not
					* specified on command line and convert
					* all to db data value.
					*/

		if (!r_rep_do())	/* run the report */
		{	/* errors found in query */
			r_error(0x1A, FATAL, NULL);
		}
	}
	else
	{
		CVna((i4)Err_count,buffer);	/* convert number to string */
		r_error(0x0A, FATAL, buffer, NULL);
	}

	/* Do clean up query */
	if( r_sc_qry(Ptr_clean_top) == FAIL)
	{
		CVna((i4)Err_count,buffer);	/* convert number to string */
		r_error(0x0A, FATAL, buffer, NULL);
	}

	if (St_batch)
	{
		/* Print out footer for batch report */
		TMnow (&stime);
		TMstr (&stime, buffer); 
		cpu = TMcpu();
		IIUGmsg(ERget(E_RW13FE_batchfooter), FALSE, 2, buffer, &cpu); 
	}
	/* shut down ADF */
	adg_shutdown();

	EXsignal(EXEXIT, 2, (long) 0, (long) NOABORT); 	/* terminate quietly */
	/* NOTREACHED */
	EXdelete();
}

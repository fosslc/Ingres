/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<lo.h>
# include	<pc.h>
# include	<si.h>
# include	<ut.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<eqrun.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<iilibqxa.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include	<lqgdata.h>
# include	<erlq.h>

/**
+* Filename:	iiutsys.c
** Purpose:	Allow entry into INGRES subsystems from EQUEL/ESQL programs.
**
** Defines:
-*		IIutsys	- Interface to UTexe.
** Notes:
**
** History:
**	18-oct-1985	- Written for INGRES 4.0 to call REPORT. (ncg)
**	02-sep-1988	- Added CALL SYSTEM. (ncg) to 
**	12-dec-1988	- Generic error processing. (ncg)
**	23-mar-1989	- Added cast to STlength. (teresa)
**      05-jan-90 	- Added new parameters to UTexe and PCcmdline. (sylviap)
**      05-jan-90 	- Added new parameter, err_code, to PCcmdline.  
**		          Changed UTexe call to pass an CL_ERR_DESC. (sylviap)
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	02-dec-1992 (larrym)
**	    Added XA support.  No calls allowed in XA.
**	21-Jan-93 (fredb)
**		Porting changes: Pass LOCATION containing a filename of
**		'$STDLIST' to UTexe and PCcmdline as the err_log parameter.
**		This will prevent the i/o from being redirected to the bit
**		bucket.
**	09-Sep-93 (valerier)
**		Added arguments to UTexe call for WindowsNT
**	19-jan-94 (larrym)
**	    Added error message when an XA application tries to call an
**	    ingres app (and have the connection passed to it).  This is
**	    not allowed in XA
**	10-apr-95 (pchang) - orig by nick
**	    Only call IIbreak if we called a subsystem; don't do it if we
**	    called the OS (65872).
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN STATUS	IIx_flag();

GLOBALREF LIBQGDATA	*IILIBQgdata;

/*
+* Procedure:	IIutsys
** Purpose:	Provide interface to UTexe
** Parameters:
**	uflag	- i4	- Are we loading, executing, etc.
**			  IIutPROG	- Passing a program name,
**			  IIutARG	- Passing an argument pair,
**			  IIutEXE	- Build call to UTexe.
**	uname	- char *- Name of program/or argument title.
**	uval	- char *- Value of an argument name (string).
** Return Values:
-*	None
** Notes:
**   1. With each call a static argv is built to send to UTexe.
**	Example:
**		## CALL QBF (database = "equeldb", table = "mytab")
**	would generate:
**		IIutsys( IIutPROG, "qbf", NULL );
**		IIutsys( IIutARG, "database", "equeldb" );
**		IIutsys( IIutARG, "table", "mytab" );
**		IIutsys( IIutEXE, NULL, NULL );
**   2. When the "database" argument is read an automatic -X flag is
**	passed via the "equel = %S" rule (if INGRES is on).
**   3. If in FORMS mode then reset to normal before call, and back to forms
**      after call.
**   4. If calling SYSTEM then this is tagged as a special case, as we want
**	to generate PCcmdline rather than a UT call.  Currently, the only
**	argument allowed to CALL SYSTEM is COMMAND.
*/

void
IIutsys( uflag, uname, uval )
i4	uflag;
char	*uname, *uval;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    char		nmbuf[IIutNMLEN +1];		    /* Object names */
    char		minus_x[MAX_LOC +1];		    /* -X flag */
    register	i4	arg;
    register	char	*cp;
    STATUS		utval;
    CL_ERR_DESC		err_code;
    LOCATION		err_loc;
    char		err_locb[MAX_LOC + 1];

#ifdef hp9_mpe
    STcopy( ERx("$STDLIST"), err_locb);
    LOfroms( FILENAME, err_locb, &err_loc);
#endif

    switch (uflag)
    {
      case IIutPROG:		/* First time through */
	STlcopy( uname, thr_cb->ii_th_ut.ut_program, IIutNMLEN );
	STtrmwhite( thr_cb->ii_th_ut.ut_program );
	thr_cb->ii_th_ut.ut_buf[0] = '\0';
	thr_cb->ii_th_ut.ut_argcnt = 0;
	return;

      case IIutARG:		/* Adding argument pair */
	if (thr_cb->ii_th_ut.ut_program[0] == '\0')
	    return;				/* Not thru IIutPROG */
	if (uname == (char *)0 || uval == (char *)0)
	    return;				/* Bad arguments */
	if ((arg = thr_cb->ii_th_ut.ut_argcnt++) >= IIutMAXARGS)
	    return;				/* Too many arguments */
	STlcopy( uname, nmbuf, IIutNMLEN );
	STtrmwhite( nmbuf );
	/* Special case for CALL SYSTEM that does not go through UTexe */
	if ( ! STbcompare(thr_cb->ii_th_ut.ut_program,0,ERx("system"),0,TRUE) )
	{
	    if (arg > 0)			/* Command already saved */
		return;
	    thr_cb->ii_th_ut.ut_argcnt = 0;	/* Assume null argument */
	    thr_cb->ii_th_ut.ut_args[0] = (char *)0;	/* Default to spawn */
	    if (STbcompare(nmbuf, 0, ERx("command"), 0, TRUE) == 0)
	    {
		if (uval != (char *)0 && *uval != '\0')
		{
		    thr_cb->ii_th_ut.ut_argcnt = 1;
		    thr_cb->ii_th_ut.ut_args[0] = STalloc(uval);
		}
	    }
	    return;
	}
	if ( ((i4)STlength(thr_cb->ii_th_ut.ut_buf) + 
	      (i4)STlength(nmbuf) + 3) >= IIutBUFLEN )
	    return;				/* Ovfl of static buffer */
	if (arg > 0)
	    STcat( thr_cb->ii_th_ut.ut_buf, ERx(",") );
	cp = STalloc( uval );
	STtrmwhite( cp );
	STcat( thr_cb->ii_th_ut.ut_buf, nmbuf );
	STcat( thr_cb->ii_th_ut.ut_buf, ERx("=%S") );
	thr_cb->ii_th_ut.ut_args[arg] = cp;
	/* Do we need the -X flag */
	if (   (STbcompare(nmbuf, 0, ERx("database"), 0, TRUE) == 0)
	    && (IIlbqcb->ii_lq_flags & II_L_CONNECT)
	    && (IIx_flag(cp, minus_x) == OK))
	{
	    STcat(thr_cb->ii_th_ut.ut_buf, ERx(",equel=%S"));
	    thr_cb->ii_th_ut.ut_args[ thr_cb->ii_th_ut.ut_argcnt++ ] = 
							    STalloc(minus_x);
	}
	return;

      case IIutEXE:		/* Make the call */
	if (thr_cb->ii_th_ut.ut_program[0] == '\0')
	    return;				/* Not yet thru IIutPROG */
	if (IILIBQgdata->form_on && IILIBQgdata->f_end)
	{
	    (*IILIBQgdata->f_end)( TRUE );		/* Restore to NORMAL */
	    SIfprintf( stdout, ERx("\n") );
	    SIflush( stdout );
	}
	if ( ! STbcompare(thr_cb->ii_th_ut.ut_program,0,ERx("system"),0,TRUE) )
	{
#ifdef hp9_mpe
	    utval = PCcmdline( (LOCATION *)0, thr_cb->ii_th_ut.ut_args[0], 
				PC_WAIT, &err_loc, &err_code );
#else
	    utval = PCcmdline( (LOCATION *)0, thr_cb->ii_th_ut.ut_args[0], 
				PC_WAIT, NULL, &err_code );
#endif
	}
	else
	{
	    if (IIXA_CHECK_IF_XA_MACRO)
	    {
		/* Call Application is not allowed in XA */
	        IIlocerr(GE_HOST_ERROR, E_LQ00D5_XA_ILLEGAL_STMT, 
			 II_ERR, 0, (PTR) 0);
		return;
	    }
#ifdef wgl_wnt
	    utval = UTexe( UT_WAIT, NULL, NULL, NULL, 
			   thr_cb->ii_th_ut.ut_program, &err_code, 
			   thr_cb->ii_th_ut.ut_buf, 
			   thr_cb->ii_th_ut.ut_argcnt, 
			   thr_cb->ii_th_ut.ut_args[0], 
			   thr_cb->ii_th_ut.ut_args[1], 
			   thr_cb->ii_th_ut.ut_args[2], 
			   thr_cb->ii_th_ut.ut_args[3], 
			   thr_cb->ii_th_ut.ut_args[4], 
			   thr_cb->ii_th_ut.ut_args[5], 
			   thr_cb->ii_th_ut.ut_args[6], 
			   thr_cb->ii_th_ut.ut_args[7], 
			   thr_cb->ii_th_ut.ut_args[8], 
			   thr_cb->ii_th_ut.ut_args[9],
			   NULL, NULL, NULL, NULL, NULL, NULL );
#else
#ifdef hp9_mpe
	    utval = UTexe( UT_WAIT, &err_loc, NULL, NULL, 
			   thr_cb->ii_th_ut.ut_program, &err_code, 
			   thr_cb->ii_th_ut.ut_buf, 
			   thr_cb->ii_th_ut.ut_argcnt, 
			   thr_cb->ii_th_ut.ut_args[0], 
			   thr_cb->ii_th_ut.ut_args[1], 
			   thr_cb->ii_th_ut.ut_args[2], 
			   thr_cb->ii_th_ut.ut_args[3], 
			   thr_cb->ii_th_ut.ut_args[4], 
			   thr_cb->ii_th_ut.ut_args[5], 
			   thr_cb->ii_th_ut.ut_args[6], 
			   thr_cb->ii_th_ut.ut_args[7], 
			   thr_cb->ii_th_ut.ut_args[8], 
			   thr_cb->ii_th_ut.ut_args[9] );
#else
	    utval = UTexe( UT_WAIT, NULL, NULL, NULL, 
			   thr_cb->ii_th_ut.ut_program, &err_code, 
			   thr_cb->ii_th_ut.ut_buf, 
			   thr_cb->ii_th_ut.ut_argcnt, 
			   thr_cb->ii_th_ut.ut_args[0], 
			   thr_cb->ii_th_ut.ut_args[1], 
			   thr_cb->ii_th_ut.ut_args[2], 
			   thr_cb->ii_th_ut.ut_args[3],
			   thr_cb->ii_th_ut.ut_args[4], 
			   thr_cb->ii_th_ut.ut_args[5], 
			   thr_cb->ii_th_ut.ut_args[6], 
			   thr_cb->ii_th_ut.ut_args[7], 
			   thr_cb->ii_th_ut.ut_args[8], 
			   thr_cb->ii_th_ut.ut_args[9] );
#endif
#endif
	    /*
	    ** Calling QUEL (and some others) cause dirty data to be left in 
	    ** the back-end sending pipes, because of use of SET [NO]EQUEL 
	    ** NASTY_HACK.  To assure that we have synced up, we send an 
	    ** IIbreak so that we can resume with what we were doing. Also 
	    ** in case any other subsystems ended prematurely we send an 
	    ** IIbreak.
	    */
	    if ( IIlbqcb->ii_lq_flags & II_L_CONNECT)
		IIbreak();
	    /*
	    ** If returning from the following subsystems then prompt before
	    ** continuing with the program:
	    **		currently none - used to be report
	    if ( STbcompare( thr_cb->ii_th_ut.ut_program, 0, 
			     ERx("report"), 0, TRUE ) == 0 )
	    {
		SIfprintf(stdout, ERget(S_LQ0212_CALL_CONTINUE));
		SIgetrec(nmbuf, IIutNMLEN, stdin);
	    }
	    */
	}
	if (IILIBQgdata->form_on && IILIBQgdata->f_end)
		(*IILIBQgdata->f_end)( FALSE );		/* Restore to FORMS */
	if (utval != OK)
	    IIlocerr( GE_HOST_ERROR, E_LQ0030_UTBAD, 
		      II_ERR, 1, thr_cb->ii_th_ut.ut_program );
	thr_cb->ii_th_ut.ut_program[0] = '\0';
	for (arg = 0; arg < thr_cb->ii_th_ut.ut_argcnt; arg++)
	    MEfree( thr_cb->ii_th_ut.ut_args[arg] );
	thr_cb->ii_th_ut.ut_argcnt = 0;
	thr_cb->ii_th_ut.ut_buf[0] = '\0';
	return;
    }
}

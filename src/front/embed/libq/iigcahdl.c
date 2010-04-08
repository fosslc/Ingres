/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<generr.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iigcahndl.c - Provides handler for GCA communications to call out.
**
**  Description:
**	Defines a global pointer and function to use for processing odd
**	information that may return from within the FE GCA module.
**
**  Defines:
**	IIhdl_gca	- Global handler called by GCA.
-*
**  History:
**	25-sep-1986	- Written (ncg)
**	26-aug-1987 	- Rewritten to use GCA. (ncg)
**	18-feb-1988	- Added PCexit(-1) call to IIabort due to change in
**			  IIabort so it no longer exits.  (marge)
**	19-may-1989	- Modified names of globals for multiple connect. (bjb)
**	19-mar-1990	- Handle IICGC_HVENT case for phoenix/alerters. (bjb)
**	17-apr-1990	- Save low-level GCA errors in LIBQ buffer (bjb)
**	13-jun-1990	(barbara)
**	    Don't quit on association failure errors unless set_ingres
**	    or ii_embed_set setting tells us to.  (Bug 21832).
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	13-mar-1991 (barbara)
**	    Added PRINTTRACE feature.  Printing trace (possibly to a file)
**	    is now done in separate module.
**	4-nov-93 (robf)
**          Add handling for GCA_PROMPT
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	24-mar-1999 (somsa01)
**	    In IIhdl_gca(), corrected arguments to IIUGsber().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
+*  Name: IIhdl_gca - Handler that is always called by GCA.
**
**  Description:
**	The handler for GCA to be called in one of the six conditions as 
**	described in iicgca.h.  The handler can be called with a variety of
**	arguments from GCA.
**
**  Inputs:
**	hflag - What condition triggered this.
**	harg  - The data associated with the condition.
**
**	The following definitions describe the flag and the argument:
**
**	  Flag		   Data
**
**	 IICGC_HTRACE      Fixed length (h_length) trace data (h_data[0]).
**			   h_errorno > 0 - First time,
**			   h_errorno = 0 - Subsequent times,
**			   h_errorno < 0 - Last time (no data, but flush)
**		 	   These values are used to toggle the forms system.
**	 IICGC_HDBERROR    Error # (h_errorno), and fixed length (h_length)
**			   error message (h_data[0]).  If num args (h_numargs)
**			   is zero then use IIUG to get the errors.
**			   If data is null, then we can reset the error 
**			   status internally.
**			   Check h_use for processing errors, warnings and
**			   user messages.
**	 IICGC_HCLERROR    CL status # (h_errorno), and null terminated routine
**			   name (h_data[0]). 
**	 IICGC_HGCERROR    II GCA error # (h_errorno), and null terminated
**			   routine name (h_data[0]). 
**	 IICGC_HUSERROR    GCA user error # (h_errorno), and null terminated
**			   routine name (h_data[0]). 
**	 IICGC_HCOPY	   h_errorno holds the COPY message type.
**	 IICGC_HFASSOC	   Ignored.
**	 IICGC_HVENT	   No data.
**	 IICGC_HPROMPT     No data.
**
**  Outputs:
**	None
**	Returns:
**	    0 - Unused.
**	Errors:
**	    None
**
**  Side Effects:
**	1. If an error is being passed then sets the global error number.
-*	
**  History:
**	25-sep-1986	- Written (ncg)
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	09-dec-1987	- Removed new-line printing, as server does it. (ncg)
**	21-dec-1987	- If trace function is set, then pass trace data. (ncg)
**	18-feb-1988	- Added PCexit(-1) call to IIabort due to change in
**			  IIabort so it no longer exits.  (marge)
**	26-feb-1988	- If DB error data is null then reset status. (ncg)
**	11-may-1988	- Modified to allow user messages, based on the
**			  value of h_use. (ncg)
**	09-dec-1988	- Value of h_use is now the same as gca_severity,
**			  rather than a special value. (ncg)
**	18-jan-1989	- Added local error # to IIlocerr call (after changing
**			  cgcutils.c). (ncg)
**	10-may-1989	- Minor change to handling IICGC_HGCERROR; also, if
**			  IICGC_HDBERROR is used w/out text, now call IIlocerr. 
**	19-may-1989	- II_ERR_DESC is now a part of II_GLB_CB. (bjb)
**	19-mar-1990	- Handle IICGC_HVENT, which means a GCA_EVENT
**			  message has been read.  Call IILQeaEvAdd to
**			  read event. (bjb)
**	17-apr-1990	- Instead of dumping low-level GCA errors to screen,
**			  save them in buffer for concatenation to LIBQ error.
**			  Removed case of IICGC_HCLERROR.  This type of error
**			  is now handled in libqgca.(bjb)
**	13-jun-1990	(barbara)
**	    Don't quit on association failure errors unless set_ingres
**	    or ii_embed_set setting tells us to.  (Bug 21832).
**	25-feb-1991	(teresal)
**	    IIdberr interface has changed as part of COPY bug fix 36059.
**	13-mar-1991 (barbara)
**	    Added PRINTTRACE feature.  Printing trace (possibly to a file)
**	    is now done in separate module.
**	22-mar-1991 (barbara)
**	    Make use of (unsupported) copyhandler in interface to IIcopy.
**	01-nov-1992 (larrym)
**	    Changed IICGC_HGCERROR case to call IIUGsber, instead of
**	    IIUGber.  Will use SQLSTATE if available.
**	18-jan-1993 (larrym)
**	    fixed "non-feature" of not setting SQLSTATE when a user message
**	    is returned from the dbms.
**	24-mar-1999 (somsa01)
**	    Corrected arguments to IIUGsber().
*/

i4
IIhdl_gca(hflag, harg)
i4		hflag;
IICGC_HARG	*harg;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    switch (hflag)
    {
      case IICGC_HTRACE:		/* Print or flush trace data */
	if ( MUp_semaphore( &IIglbcb->iigl_svrtrc.ii_tr_sem ) == OK )
        {
	    i4			(*trace_func)() = IIglbcb->ii_gl_trace;
	    DB_DATA_VALUE	dbt;
      
	    /*
	    ** If PRINTTRACE is not set use specially-set trace handler (TM);
	    ** otherwise use LIBQ trace printer.
	    */
	    if ( ! trace_func || IIglbcb->iigl_svrtrc.ii_tr_flags & II_TR_FILE )
		trace_func = (i4 (*)())IILQstpSvrTracePrint;

	    if (harg->h_errorno >= 0) 		/* Trace data message */
	    {

		II_DB_SCAL_MACRO(dbt, DB_CHA_TYPE, harg->h_length,
				harg->h_data[0]);
		_VOID_(*trace_func)(FALSE, &dbt);
	    }
	    else				/* End of trace data */
	    {
		_VOID_(*trace_func)(TRUE, (DB_DATA_VALUE *)0);
	    }

	    MUv_semaphore( &IIglbcb->iigl_svrtrc.ii_tr_sem );
	}
	break;

      case IICGC_HDBERROR:
	if (harg == (IICGC_HARG *)0)			/* Reset error */
	    IIdberr( thr_cb, (i4)0, (i4)0, GCA_ERDEFAULT, 0, 
		     (char *)0, TRUE );
	else if (harg->h_use & GCA_ERMESSAGE)		/* User message */
	{
	    IIdbermsg( thr_cb, harg->h_errorno, 
		       (i4)harg->h_length, harg->h_data[0] );
	    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			     harg->h_sqlstate->db_sqlstate );
	}
	else if (harg->h_numargs == 1)			/* Error with text */
	    /* note:  IIsdberr calls IILQdaSIsetItem */
	    IIsdberr(thr_cb, harg->h_sqlstate, harg->h_errorno, harg->h_local, 
		     harg->h_use, (i4)harg->h_length, harg->h_data[0], TRUE);
	else
	{
	    /* Error w/o text - invalid protocol */

	    char gbuf[20];
	    char lbuf[20];
	    char pbuf[20];

	    CVla( harg->h_errorno, gbuf );
	    CVla( harg->h_local, lbuf );
	    CVla( harg->h_numargs, pbuf );
	    IIlocerr( GE_COMM_ERROR, E_LQ0009_GCFMT, II_ERR, 3,
			gbuf, lbuf, pbuf );
	}
	break;

      case IICGC_HGCERROR:
      /*
      ** Don't go through IIlocerr because this is probably the first error
      ** and will therefore get recorded into the SQLCA -- and yet it's the
      ** least informative error.  Save error text for later concatenation
      ** to LIBQ errortext.  Also, use local error rather than generic
      ** because IIUGsber won't find generic error. 
      */
      {
	char *submsg = IIlbqcb->ii_lq_error.ii_er_submsg;
	DB_SQLSTATE  db_sqlst;

	if (submsg == (char *)0)
	    submsg = (char *)MEreqmem((u_i4)0, (u_i4)(ER_MAX_LEN +1),
			TRUE, (STATUS *)0);
	if (submsg != (char *)0)
	{
	    /* get SQLSTATE and message */
	    IIUGsber(&db_sqlst, submsg, ER_MAX_LEN, harg->h_local, 
		    II_ERR, (i4)harg->h_numargs, harg->h_data[0],
		    harg->h_data[1], harg->h_data[2], harg->h_data[3],
		    harg->h_data[4], NULL, NULL, NULL, NULL, NULL);
	    /* save SQLSTATE in the ANSI SQL Diagnostic Area */
	    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE,
			     db_sqlst.db_sqlstate );
	}
	IIlbqcb->ii_lq_error.ii_er_submsg = submsg;
	break;
      }

      case IICGC_HUSERROR:
	IIlocerr(harg->h_errorno, harg->h_local, II_ERR, (i4)harg->h_numargs,
		harg->h_data[0], harg->h_data[1], harg->h_data[2],
		harg->h_data[3], harg->h_data[4]);
	break;

      case IICGC_HFASSOC:
	IIlbqcb->ii_lq_flags &= ~II_L_CONNECT;
	if ( IIglbcb->ii_gl_flags & II_G_PRQUIT )
	{
	    if (   IIlbqcb->ii_lq_error.ii_er_submsg != (char *)0
		&& IIlbqcb->ii_lq_error.ii_er_submsg[0] != '\0' )
		IImsgutil(IIlbqcb->ii_lq_error.ii_er_submsg);
	    IImsgutil(ERget(S_LQ0210_QUIT));
	    IIabort();
	    PCexit(-1);
	}
	IIlocerr( GE_COMM_ERROR, E_LQ002D_ASSOCFAIL, II_ERR, 0, (char *)0 );
	break;

      case IICGC_HCOPY:
	IIcopy((i4)harg->h_errorno, IIglbcb->ii_gl_hdlrs.ii_cpy_hdl);
	break;

      case IICGC_HVENT:
	IILQeaEvAdd( thr_cb );
	break;

      case IICGC_HPROMPT:
	IILQdbprompt( thr_cb );
    }
    return 0;
}

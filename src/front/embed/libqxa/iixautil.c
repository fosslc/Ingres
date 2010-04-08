
/*
** Copyright (c) 2004 Ingres Corporation
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cv.h>
# include	<er.h>
# include	<nm.h>
# include	<lo.h>
# include	<pc.h>
# include	<si.h>			/* needed for iicxfe.h */
# include	<st.h>
# include	<generr.h>
# include	<erlq.h>
# include	<iicx.h>
# include	<iicxxa.h>
# include	<iicxfe.h>
# include       <iixautil.h>
# include	<iisqlca.h>
# include       <iilibq.h>
# include       <iilibqxa.h>
# include	<lqgdata.h>


/*
**  Name: iixautil.c - Utility routines called from other LIBQXA routines.
**
**  Description:
**      This module contains utility routines dealing with LIBQXA.
**	
**  Defines:
**
**	IIxa_fe_setup_logicals()
**	IIxa_fe_set_libq_handler()
**	IIxa_error()
**	IIxa_fe_qry_trace_handler()
**
**
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	03-nov-1992	- First written (mani)
**	20-nov-1992 (larrym)
**	    added IIxa_fe_set_libq_handler.
**	13-jan-1993 (larrym)
**	    added XA tracing functionality.  Added IIxa_fe_qry_trace_handler.
**	25-Aug-1993 (fredv)
**	    Needed to include <st.h>.
**	18-Dec-97 (gordy)
**	    LIBQ current session moved to thread-local-storage.
**	20-Aug-98 (thoda04)
**	    The use of DLL's on NT requires that IILIBQgdata be GLOBALDLLREF'ed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

/* forward declares */
VOID
IIxa_fe_qry_trace_handler(char *cb, char *outbuf, i4  flag);
i4
IIxa_fe_error_trace_handler(void);



/*
**   Name: IIxa_fe_setup_logicals()
**
**   Description: 
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful. 
**
**   History:
**       3-Nov-1992 - First written (mani)
**	13-jan-1993 (larrym)
**	    Added check for II_XA_TRACE_FILE
**	25-jan-1993 (larrym)
**	    removed calling of libq modules to process libq logicals.  
**	01-mar-1993 (larrym)
**	    added check for II_XA_SESSION_CACHE_LIMIT
**	09-sep-1993 (larrym)
**	    Added logic to check II_XA_TRACE_FILE for "%p" and substitute
**	    the process PID so processes running from the same environment
**	    can have different filenames.
**      12-Oct-1993 (mhughes)
**          Added flush of trace buffer to file after every trace call so that
**          the user doesn't have to wait until xa_close to see trace output.
**	13-Dec-1993 (mhughes)
**	    If not using a per-process log file then open log file in append 
**	    mode. Modify trace headers to match.
*/

DB_STATUS
IIxa_fe_setup_logicals()
{

    char	*xa_trc_file_ptr;		/* points to environ space */
    char	xa_trc_file_name[MAX_LOC+1];	/* trace file name         */
    char	*xa_session_pool_limit_str;	/* used by NMgtAt */
    i4		xa_session_pool_limit;		/* actual limit */
    SYSTIME	now;
    char	nowbuf[100];
    i4		nowend;
    i4		status;
    PID		pid;			        /* used for fname uniqueness */
    char	*pid_index=NULL;	       	/* ptr to %p in filename */
    LOCATION	trace_loc;			/* native file location */
    FILE	*trace_file;			/* native file handle */
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
    GLOBALDLLREF   LIBQGDATA    *IILIBQgdata;	/* used to trace err msgs */
# else
    GLOBALREF      LIBQGDATA    *IILIBQgdata;	/* used to trace err msgs */
# endif

    /* Process XA specific logicals */

    /* check for XA trace file */
    NMgtAt( ERx("II_XA_TRACE_FILE"), &xa_trc_file_ptr );

    PCpid(&pid);	    /* get the pid */

    if (xa_trc_file_ptr != NULL && *xa_trc_file_ptr)
    {
	/* 
	** Process filename.  We allow using a %p in the filename whereupon
	** we'll substitute the PID, so that multiple processes running from
	** the same environment will generate unique filenames.
	**
	** the big *if*.  first, is there a '%'; second, is it the only '%'; 
	** third, is there a 'p' after the '%'
	*/
	if(((pid_index = STindex(xa_trc_file_ptr, ERx("%"), MAX_LOC)) != 0)
	   && (STrindex(xa_trc_file_ptr, ERx("%"), MAX_LOC) == pid_index)
	   && ( *(pid_index + 1) == 'p'))
	{
	    *(pid_index + 1) = 'd'; /* change the "%p" to "%d" for SIprintf */
	    STprintf(xa_trc_file_name, xa_trc_file_ptr, (i4)pid);
	}
	else
	{
	    /* normal file name */
            STlcopy(xa_trc_file_ptr, xa_trc_file_name, MAX_LOC);
	    STprintf( IIcx_fe_thread_main_cb->process_id, ERx("%d"), (i4)pid );
	}

	/* lock the IICX_FE_THREAD_MAIN_ CHECK */

	IIcx_fe_thread_main_cb->fe_main_flags |= IICX_XA_MAIN_TRACE_XA;

	/* open trace file
	** If writing a per-process log file open a new file for write
	** else open log file in append mode. It's the user's responsibility
	** to tidy-up too the logs if they get too big.
	*/
	LOfroms(FILENAME, xa_trc_file_name, &trace_loc );
	if ( ( pid_index != NULL ) && ( *pid_index != EOS ) )
	    status = SIfopen(&trace_loc, ERx("w"), SI_TXT, (i4)0, 
			     &trace_file);
	else
	    status = SIfopen(&trace_loc, ERx("a"), SI_TXT, (i4)0,
			     &trace_file);

        if (status != OK)
	{
	    /* unlock the IICX_FE_THREAD_MAIN_ CHECK */
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ00D4_XA_BAD_VALUE, II_ERR, 1, 
		ERx("II_XA_TRACE_FILE"));
	    return E_DB_ERROR;
	}
        IIcx_fe_thread_main_cb->fe_main_trace_file = trace_file;
	/* unlock the IICX_FE_THREAD_MAIN_ CHECK */

	/* put start message in trace file */
	/* Get time stamp */
    	TMnow(&now);
    	TMstr(&now, nowbuf);
    	/* TMstr on UNIX puts a newline into the buffer */
    	nowend = STlength(nowbuf);
    	if (nowbuf[nowend -1] == '\n')
             nowbuf[nowend -1] = EOS;
       	SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file, 
                  ERx(IIXATRCENDLN), nowbuf);
       	SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file, 
                  ERx("Started XA Trace for process %d at %s\n\n"), pid, nowbuf);
	SIflush((FILE *)IIcx_fe_thread_main_cb->fe_main_trace_file);
	/* add trace function to query trace handler */
	IILQqthQryTraceHdlr(IIxa_fe_qry_trace_handler, NULL, TRUE);
	IILIBQgdata->excep = IIxa_fe_error_trace_handler;
    }

    /* Process SESSION CACHE LIMIT */
    NMgtAt( ERx("II_XA_SESSION_CACHE_LIMIT"), &xa_session_pool_limit_str );
    if (xa_session_pool_limit_str != NULL && *xa_session_pool_limit_str)
    {
	status = CVan(xa_session_pool_limit_str, &xa_session_pool_limit );
	switch(status) 
	{
	    case OK:
	    {
		/* LOCK the XA XN MAIN CB. CHECK !!! */
	        /* override default limit (set in iicxinit.c) */
	        IIcx_xa_xn_main_cb->max_free_xa_xn_cbs = xa_session_pool_limit;
		/* UNLOCK the XA XN MAIN CB. CHECK !!! */
	        break;
	    }
	    case CV_OVERFLOW:
	    case CV_SYNTAX:
	    default:
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ00D4_XA_BAD_VALUE, II_ERR, 1, 
		ERx("II_XA_SESSION_CACHE_LIMIT"));
	    return E_DB_ERROR;
	}
    }
    return E_DB_OK;

} /* IIxa_fe_setup_logicals */


/*
**   Name: IIxa_fe_set_libq_handler()
**
**   Description: 
**	Set's the IIglbcb->ii_gl_lbq_hdlr to point to the libq handler.
**	In essence, this tells libq that this is an XA application.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful. 
**
**   History:
**	20-nov-1992 (larrym)
**	    written.
*/

DB_STATUS
IIxa_fe_set_libq_handler(i2 active)
{

    if(active)
    {
	IIglbcb->ii_gl_xa_hdlr = IIxaHdl_libq;
    }
    else
    {
	IIglbcb->ii_gl_xa_hdlr = NULL; 
    }
	return E_DB_OK;
}

/* 
** function to be called by (one of) libq's exception handler(s).  The
** address of this function is passed to libq when we process the 'process'
** environment variables.  It has to be a i4, cause that's the way the
** libq function pointer is declared.  It's return value is never checked.
*/
i4
IIxa_fe_error_trace_handler(void)
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if (IIlbqcb->ii_lq_error.ii_er_smsg != (char *)0)
    {
	IIxa_fe_qry_trace_handler( (PTR) 0, IIlbqcb->ii_lq_error.ii_er_smsg,
				   TRUE);
/*	IIxa_fe_qry_trace_handler( (PTR) 0, IIXATRCENDLN, TRUE); */
    }
}

/*
**   Name: IIxa_fe_qry_trace_handler()
**
**   Description: 
**
**   Inputs:
**	cb	- optional control block.  Not used.
**	outbuf	- text to write out to trace file
**	flag	- TRUE, if called from XA, FALSE, if called from libq
**   Outputs:
**
**   History:
**      12-Oct-1993 (mhughes)
**          Added flush of trace buffer to file after every trace call so that
**          the user doesn't have to wait until xa_close to see trace output.
**	6-feb-1998(angusm)
**	    Put back substituted '\n'.
*/
VOID
IIxa_fe_qry_trace_handler(char *cb, char *outbuf, i4  flag)
{
	i4	len;

    if (flag == FALSE)
    {
        /* 
        ** called from LIBQ, need to parse outbuf:
        */

        if(!IIxa_check_ingres_state( (IICX_CB *)0, IICX_XA_T_ING_INTERNAL ))
        { 
	    /* internal state. we only want query text */ 
	    if ( !STbcompare(ERx("Query text:"),11, outbuf, 0, FALSE))
	    {
		/* replace new line */
		len = STlength(outbuf) - 1;
		outbuf[len] = '\0';
		SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file,
		ERx("\t'%s' sent to RM\n"), &outbuf[14]);
		outbuf[len] = '\n';
	    }
	    else
	    {
		/* what ever it is, we don't want it. */
	    }

	}
	else
	{
	    /* user's query. only want query text, and end-o-line hyphens */
	    if ( !STbcompare(ERx("Query text:"),11, outbuf, 0, FALSE))
            {
	        SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file, 
	            ERx("Appl Code:\n%s"),&outbuf[12]);
	    }
	    else if (!STcompare(IIXATRCENDLN, outbuf) )
	    {
                SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file, outbuf);
	    }
            else 
            {
	        /* we don't need no stinking query response time */
            }
	}
    }
    else
    {
        SIfprintf(IIcx_fe_thread_main_cb->fe_main_trace_file, outbuf);
    }

    /* Force buffer to flush to trace file */
    SIflush( (FILE *)IIcx_fe_thread_main_cb->fe_main_trace_file );

    return;
}

/*
** 
*/
DB_STATUS
IIxa_fe_shutdown()
{
    if( (IIcx_fe_thread_main_cb) 
       && (IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA))
    {
	IIxa_fe_qry_trace_handler( (PTR) 0, 
	    ERx("\tclosing XA trace file...\n"), TRUE);
	IIxa_fe_qry_trace_handler( (PTR) 0, IIXATRCENDLN, TRUE);
	SIflush((FILE *)IIcx_fe_thread_main_cb->fe_main_trace_file);
	SIclose((FILE *)IIcx_fe_thread_main_cb->fe_main_trace_file);
	IIcx_fe_thread_main_cb->fe_main_flags &= ~IICX_XA_MAIN_TRACE_XA;
    }
}

/*
**   Name: IIxa_error()
**
**   Description: 
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful. 
**
**   History:
**       3-Nov-1992 - First written (mani)
**	18-jan-1993 (larrym)
**	    modified to call locerr.
*/

VOID
IIxa_error(
                           i4   generic_errno,
                           i4   iixa_errno,
                           i4        err_param_count,
                           PTR       err_param1,
                           PTR       err_param2,
                           PTR       err_param3,
                           PTR       err_param4,
                           PTR       err_param5 
                          )
{

    IIlocerr(generic_errno, iixa_errno, II_ERR, err_param_count, err_param1,
	    err_param2, err_param3, err_param4, err_param5 );

} /* IIxa_error */

/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<er.h>
#include	<si.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<gca.h>
#include	<generr.h>
#include	<fe.h>
#include	<ug.h>
#include	<iisqlca.h>
#include	<lqgdata.h>
#include	<iilibq.h>
#include	<iilibqxa.h>
#include	<erlq.h>
#include	<sqlstate.h>	/* needed for SQLSTATE defines */
# include       <stdarg.h>

/**
+* Name:    iierror.c -	Standard EQUEL Error Reporting Module.
**
** Defines:
**	IIlocerr()	Standard error reporter for LIBQ, LIBQGCA and LIBQXA.
**	IIdberr()	Error reporter for DBMS errors.
**	IIdbermsg()	Message reporter for DBMS user messages.
**	IIer_save()	Save away current error number and text.
**	IIret_err	Return error number argument.
**	IIno_err()	Returns zero
**	IItest_err()	Return current value of error flag.
**	IILQfrserror()	Call user error handler.
**	IImsgutil()	Outputs miscellaneous LIBQ messages to terminal
**	IIdbmsg_disp()	Display database procedure user message.
**
** History:
**	08-oct-1986 - Converted to 6.0 error handling.
**	13-apr-1987 - Modified for 6.0 Embedded SQL support.
**	04-may-1987 - Fixed bug in IIer_save.
**	29-jun-87 (bab)	Added VARARGS comment to help quiet lint.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	26-aug-1987 - Updated for internationalization (bjb).
**	21-oct-1987 (peter)
**		Use new IIUGfemFormatErrorMessage routine.
**	11-may-1988 (neil)
**		Added IIdbermsg, IIdbmsg_disp and modified IIdberr,
**		IImsgutil (ncg)
**	09-dec-1988 (neil)
**		Modified IIlocerr and IIdberr to handle generic error messages.
**		After processing errors the result should be:
**			      		SQL		QUEL
**					--------------------
**			  ii_er_num:	genericno	local (INGRES)
**			  ii_er_other:	local (?)	generic
**			  ii_er_smsg:	Formatted error text for both
**	03-may-1989 (mgw)
**		Put the generic error number into errno before calling
**		IIugber() to handle errors comming from the GCA COMM Server.
**	19-may-1989 (bjb)
**		Modified global names for multiple connect support. (bjb)
**	15-mar-1990 (barbara)
**		Don't pass global ii_er_num to user's IIseterr handler because
**		users should not be allowed to reset global error var.
**	17-apr-1990 (barbara)
**		Append subsidiary (gca) errors to error text in IIlocerr.
**	15-oct-1990 (barbara)
**		Fixed bug 9398 and 20010.  Mark warnings (generic code of 50)
**		and informational messages (generic code of 0) as warnings.
**	        IInextget can then keep going on warnings.
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	15-mar-1991 (kathryn)
**	    Added support for user defined handlers:
**		errorhandler - eventhandler - messagehandler.
**	01-apr-1991 (kathryn)
**	    Push and Pop session ids around user handlers to check that user 
**	    has not switched sessions in handler without switching back.
**	23-sep-91 (seg)
**	    Can't declare errno locally -- it's redefined by OS/2 C libraries.
**	03-nov-1992 (larrym)
**	    Added SQLSTATE support.  See IIlocerr and IIdbmserr.
**      07-oct-93 (sandyd)
**          Added <fe.h>, which is now required by <ug.h>.
**	03-sep-96 (chech02) bug# 78450
**	    Moved some local variables to global heap area  to  
**	    prevent 'stack overflow' in windows 3.1 port.
**	26-dec-1996 (donc) 
**	    Add IIfakerr entry point.  This is needed by OpenROAD.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	02-apr-1999 (somsa01)
**	    In IIfakeerr(), corrected argument to IIsqInit().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-2003 (hanch04)
**	    changed IIlocerr to use var args.
**/

GLOBALREF LIBQGDATA	*IILIBQgdata;

i4		IIret_err();
i4		IIno_err();
VOID		IImsgutil();

/*{
+* Name:	IIlocerr() -	Standard LIBQ error reporting routine.
**
**  Description:
**	IIlocerr is called directly by a routine detecting an error (and
**	not by the back-end which goes through a handler).
**
**	The error is printed if the DML is not DB_SQL and the call 
**	(*ii_gl_seterr)() returns > 0. Otherwise no error message is printed.
**	Note that ii_gl_seterr points at the user's error handler if the
**	embedded program made an IIseterr() call.  
**
**	Now supports SQLSTATE.  Currently this routine is called only
**	with errors in LC, LQ, LS, and CO.   If you have another facility
**	call IIlocerr, the erxx.msg file must have define SQLSTATEs for every
**	non-fast message.  See front!hdr!hdr!erlq.msg for an example.
**
** Inputs:
**	genericno	- Generic error number
**	errno		- LIBQ error number
**	severity	- Currently always 0
**	parcount	- Number of parameters for error message
**	a?		- PTRs to up to 5 parameters.
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
** History:
**	17-jul-1985 - Added calls to routines to map errors
**		      for SQL and added IIdml routine. (jrc)
**	08-oct-1986 - Converted to 6.0 (mrw)
**	26-aug-1987 - To use ER and IIUG routines.(bjb)
**	09-dec-1988 - Modified to handle generic errors and reduced to 5
**		      parameters. (ncg)
**	03-may-1989 (mgw)
**		Put the generic error number into errorno before calling
**		IIugber() to handle errors coming from the GCA COMM Server.
**	14-apr-1989 - Support user-set default for error reporting (bjb)
**	19-apr-1989 - Modified flag names for multiple connect. (bjb)
**	15-mar-1990 (barbara)
**		Don't pass global ii_er_num to user's IIseterr handler because
**		users should not be allowed to reset global error var.
**	17-apr-1990 (barbara)
**		Append text from "subsidiary" errors (e.g. GCA errors) to
**		text saved for INQUIRE_INGRES ERRORTEXT.  These errors used
**		to be printed to stdout in IIhdl_gca, but they are now buffered.
**		Print subsidiary errors if necessary.
**	15-mar-1991 (kathryn)
**		Add calls to user defined error handler if one was set.
**	21-mar-1991 (kathryn)
**		Remove references to II_E_FRSERR. FRS never calls IIlocerr.
**	23-sep-91 (seg)
**	    Can't declare errno locally -- it's redefined by OS/2 C libraries.
**	03-nov-1992 (larrym)
**	    Added SQLSTATE support.  See note above.  Now calls IIUGsber 
**	    (instead of IIUGber) which returns the SQLSTATE as well as the
**	    error message text.  SQLSTATE's are saved in the ANSI SQL
**	    Diagnostic Area as the RETURNED_SQLSTATE element. 
**	13-nov-1992 (larrym)
**	    Modified call to IIsdberr to be consitent with prototype.
*/

VOID
/* VARARGS */
IIlocerr( i4 genericno, i4 errnum, i4 severity, i4 parcount, ...)
{
    va_list     ap;

    PTR		aa[5];
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4	(*seterr)() = IIglbcb->ii_gl_seterr;
    i4		(*err_hdl)() = IIglbcb->ii_gl_hdlrs.ii_err_hdl;
#ifdef WIN16
static     char	*buf= NULL;		/* Pre-formatted error text */
static     char	*fbuf= NULL;		/* Post-formatted error text */
i4	   buf_len = ER_MAX_LEN +1;
i4	   fbuf_len = 4 * ER_MAX_LEN + 1;
#else
    char	buf[ER_MAX_LEN +1];		/* Pre-formatted error text */
    char	fbuf[4*ER_MAX_LEN +1];		/* Post-formatted error text */
    i4		buf_len = sizeof(buf);
    i4		fbuf_len = sizeof(fbuf);
#endif /* WIN16 */
    II_ERR_DESC *edesc = &IIlbqcb->ii_lq_error;
    DB_SQLSTATE db_sqlst;			/* SQLSTATE from IIUGsber(); */
    i4 i;

    /*
    * Write variable-num ptrs to destination
    */
    va_start(ap, parcount);
    for (i = 0; i < parcount; i++)
    {
	aa[i] = va_arg(ap, PTR);
    }
    va_end(ap);

#ifdef WIN16
    if (buf == NULL)
    {
      buf  = MEreqmem(0, buf_len, FALSE, (STATUS *)NULL);
      fbuf = MEreqmem(0, fbuf_len, FALSE, (STATUS *)NULL);
    }
#endif /* WIN16 */

    /* Maybe from INGRES through IIgcahdl */
    if (errnum == 0)
	errnum = genericno;
    /* Generic numbers should always be positive (some old headers are not) */
    if (genericno < 0)
	genericno = -genericno;

    IIUGsber(&db_sqlst, buf, buf_len-1, errnum, severity, 
	     parcount, aa[0], aa[1], aa[2], aa[3], aa[4], 
	     (PTR)0, (PTR)0, (PTR)0, (PTR)0, (PTR)0);
    /* save sqlstate in Diagnostic Area */
    IILQdaSIsetItem( thr_cb, IIDA_RETURNED_SQLSTATE, db_sqlst.db_sqlstate );

    /* format message */
    _VOID_ IIUGfemFormatErrorMessage(fbuf, fbuf_len, buf, FALSE);

    /* Format saved subsidiary errors from GCA */
    if (edesc->ii_er_submsg != (char *)0 && edesc->ii_er_submsg[0] != '\0')
    {
	i4	flen;
	flen = STlength(fbuf);
        _VOID_ IIUGfemFormatErrorMessage(fbuf + flen, fbuf_len - flen,
		edesc->ii_er_submsg, FALSE);
    }

    if (IIlbqcb->ii_lq_dml == DB_SQL)
    {
	if (edesc->ii_er_flags & II_E_DBMS)	/* Default to local errors? */
	{
	    edesc->ii_er_num  = errnum;
	    edesc->ii_er_other = genericno;
	}
	else
	{
	    edesc->ii_er_num = genericno;
	    edesc->ii_er_other = errnum;
	}
    }
    else /* QUEL is reverse */
    {
	edesc->ii_er_num = errnum;
	edesc->ii_er_other = genericno;
    }
    edesc->ii_er_estat = severity;

    IIer_save( IIlbqcb, fbuf );		/* Save the error */

    /* 
    ** If there is a handler, and it points at something that returns
    ** non-0, and this is not an ESQL error, print it.
    */
    if (    (seterr)
	&&  (!IISQ_SQLCA_MACRO(IIlbqcb))
	&&  (!(edesc->ii_er_flags & II_E_PRT))	/* Not recursive error */
	&&  (!err_hdl))
    {
	i4	er_num = edesc->ii_er_num;

	edesc->ii_er_flags |=  II_E_PRT;
	if (((*seterr)(&er_num)) == IIerOK)
	{
	    edesc->ii_er_flags &= ~II_E_PRT;
	    return;
	}
	edesc->ii_er_flags &= ~II_E_PRT;
    }

    /* 
    ** If user defined error handler was set via SET_SQL (errorhandler)
    ** Call the users handler here.
    */

    if ( err_hdl  &&  ! (thr_cb->ii_hd_flags & II_HD_ERHDLR) )

    {
	IILQsepSessionPush( thr_cb );
	thr_cb->ii_hd_flags |= II_HD_ERHDLR;
	_VOID_ (*err_hdl)();
	thr_cb->ii_hd_flags &= ~II_HD_ERHDLR;
	
	/* 
	** Issue error if user has switched sessions in handler and not
	** switched back.
	*/

	_VOID_ IILQscSessionCompare( thr_cb );	/* issues the error */
	IILQspSessionPop( thr_cb );
    }

    if (   ( ! IISQ_SQLCA_MACRO( IIlbqcb )  &&  ! err_hdl )
	|| (edesc->ii_er_flags & II_E_SQLPRT)
       )
    {
        /* Make sure the channel is open for I/O messages */
	if (!SIisopen(stdout))
	    IImain();
	/* Format/print subsidiary error messages from GCA */
	if (edesc->ii_er_submsg != (char *)0 && edesc->ii_er_submsg[0] != '\0')
	    IIUGeppErrorPrettyPrint(stdout, edesc->ii_er_submsg, FALSE);
	/* Format/print current error */
        IIUGeppErrorPrettyPrint(stdout, buf, FALSE);
    }

    if ( IIglbcb->ii_gl_flags & II_G_DEBUG )  IIdbg_dump( IIlbqcb );
    if ( IILIBQgdata->excep )  (*IILIBQgdata->excep)();
}

/*{
+*  Name: IIdberr - Report DBMS errors during execution of embedded program.
**
**  Description:
**	IIdberr is now a cover into IIsdberr.  But the description here 
**	remains.
**
**	IIdberr() is called by a handler when an error is reported back from 
**	the DBMS.  The DBMS has passed back the text of the message, including
**	extraneous timestamp information.  This routine uses the IIUG
**	routines to print and/or buffer the error (for the SQLCA and for
**	LIBQ's error-saving) without the unwanted text.
**
**	If the argument is zero, then reset the error data in case of the 
**	Terminal Monitor (to allow warnings).  This can be extended to user
**	applications at a later date by removing the check for TM.
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	dbgeneric	- Number of error.  If this error number is zero,
**			  and we are running the TM, then reset error
**			  status to allow the skipping of warnings.
**			  This error number is normally the generic error
**			  number (as specified by GCA).  If we are running
**			  INGRES (prior to generic error modifications) this
**			  will be the INGRES error number.
**	dblocal		- DBMS-specific error number.
**	dbstatus	- Severity status of error (comes from gca_severity of
**			  GCA_ERROR).  If GCA_ERFORMATTED is set then the
**			  error text has already been formatted by a host
**			  gateway and we should not reformat it using UG.
**	dberrlen	- Length of following text.
**	dberrtext	- Full text received from DBMS (not null terminated)
**	dbtimestmp	- Flag to indicate if a time stamp is included in the
**			  error text: 
**				TRUE - time stamp
**				FALSE - no time stamp.  
**
**  Outputs:
**	
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	Error text is printed to stdout if 
**	1) this is an EQUEL program, and user has not notified us
**	   of an error handler via IIseterr(); or
**	2) This is an ESQL program NOT using an SQLCA.
**	
**	The error text is saved for use in INQUIRE_EQUEL (unless ESQL
**	and not the first error in current statement).  For ESQL it is
**	also saved into the SQLCA if appropriate.
**	
**  History:
**	26-aug-1987 - extracted from IIf_hndl_err() (bjb)
**	21-oct-1987 (peter)
**		Use new IIUGfemFormatErrorMessage routines.
**	26-feb-1988 - if 0 error number then reset error information. (ncg)
**	11-may-1988 - added more argument to handle warnings - currently
**		      unused. (ncg)
**	09-dec-1988 - Modified to handle generic errors with already formatted
**		      error text. (ncg)
**	19-may-1989 - Modified names of global flags for multiple connect. (bjb)
**	14-aug-1989 - Fixed generic/local error toggling to really toggle. (bjb)
**	15-mar-1990 (barbara)
**		Don't pass global ii_er_num to user's IIseterr handler because
**		users should not be allowed to reset global error var.
**	15-oct-1990 (barbara)
**		Bugs 9398, 9160.  Set II_Q_WARN flag so that SELECT/RETIEVE
**		loops know to keep going on warnings vs. errors.  This flag
**		is checked in IInext, IIerrtest and turned off in IIqry_end.
**	25-feb-1991 (teresal)
**		Added timestamp parameter.  This is part of COPY bug fix 
**		36059 so ADF user errors can be displayed - these errors
**		do not have timestamps so IIUGfemFormatErrorMessage needs
**		to be called with printtime=FALSE.
**      15-mar-1991 (kathryn)
**              Add call to user defined error handler if one was set.
**      21-mar-1991 (kathryn)
**              Remove references to II_E_FRSERR. FRS never calls IIdberr.
**	01-nov-1992 (larrym)
**		Changed interface to IIdberr to include an SQLSTATE, and 
**		then changed the name of the function to IIsdberr.  IIdberr
**		is now just a cover into IIsdberr.
**	11-feb-1993 (larrym)
**	    changed STlcopy to MEcopy; SQLSTATE is not null terminated.
*/

VOID
IIdberr
(
    II_THR_CB	*thr_cb,
    i4	dbgeneric,
    i4	dblocal,
    i4		dbstatus,
    i4		dberrlen,
    char	*dberrtext,
    bool	dbtimestmp
)
{
DB_SQLSTATE	db_sqlst;

    /* 
    **  The only function that should still be calling this routine is
    **	iicopy.  We don't care about sqlstates in this case because it 
    **  has already called IIlocerr, and the sqlstate was dealt with there.
    */
    MEcopy (SS5000K_SQLSTATE_UNAVAILABLE, 
	     DB_SQLSTATE_STRING_LEN,
	     db_sqlst.db_sqlstate);
    IIsdberr( thr_cb, &db_sqlst, dbgeneric, dblocal, dbstatus, 
	      dberrlen, dberrtext, dbtimestmp );
}

/*{
+*  Name: IIsdberr - Report DBMS errors during execution of embedded program.
**
**  Description:
**	IIsdberr() is called by a handler when an error is reported back from 
**	the DBMS.  The DBMS has passed back the text of the message, including
**	extraneous timestamp information.  This routine uses the IIUG
**	routines to print and/or buffer the error (for the SQLCA and for
**	LIBQ's error-saving) without the unwanted text.
**
**	If the argument is zero, then reset the error data in case of the 
**	Terminal Monitor (to allow warnings).  This can be extended to user
**	applications at a later date by removing the check for TM.
**
**	This function is now also passed the SQLSTATE which it
**	saves in the ANSI SQL Diagnostic Area as the RETURNED_SQLSTATE.
**	It's not really said anywhere else, but about the only function
**	that calls this routine is the gca handler in libq!iigcahdl.c,
**	which is an interface to libqgca and so the SQLSTATE is passed
**	to us from the DBMS.  
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	db_sqlst	- the SQLSTATE associated with the error.
**	dbgeneric	- Number of error.  If this error number is zero,
**			  and we are running the TM, then reset error
**			  status to allow the skipping of warnings.
**			  This error number is normally the generic error
**			  number (as specified by GCA).  If we are running
**			  INGRES (prior to generic error modifications) this
**			  will be the INGRES error number.
**	dblocal		- DBMS-specific error number.
**	dbstatus	- Severity status of error (comes from gca_severity of
**			  GCA_ERROR).  If GCA_ERFORMATTED is set then the
**			  error text has already been formatted by a host
**			  gateway and we should not reformat it using UG.
**	dberrlen	- Length of following text.
**	dberrtext	- Full text received from DBMS (not null terminated)
**	dbtimestmp	- Flag to indicate if a time stamp is included in the
**			  error text: 
**				TRUE - time stamp
**				FALSE - no time stamp.  
**
**  Outputs:
**	
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	Error text is printed to stdout if 
**	1) this is an EQUEL program, and user has not notified us
**	   of an error handler via IIseterr(); or
**	2) This is an ESQL program NOT using an SQLCA.
**	
**	The error text is saved for use in INQUIRE_EQUEL (unless ESQL
**	and not the first error in current statement).  For ESQL it is
**	also saved into the SQLCA if appropriate.
**	
**  History:
**	01-nov-1992 (larrym)
**      	extracted from IIdberr.  Added SQLSTATE support.
*/

VOID
IIsdberr
(
    II_THR_CB	*thr_cb,
    DB_SQLSTATE	*db_sqlst,
    i4	dbgeneric,
    i4	dblocal,
    i4		dbstatus,
    i4		dberrlen,
    char	*dberrtext,
    bool	dbtimestmp
)
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4	(*seterr)() = IIglbcb->ii_gl_seterr;
    i4		(*err_hdl)() = IIglbcb->ii_gl_hdlrs.ii_err_hdl;
    char	buf[ER_MAX_LEN +1];		/* Preformatted error text */
    char	fbuf[2*ER_MAX_LEN +1];		/* Post-formatted error text */
    II_ERR_DESC *edesc = &IIlbqcb->ii_lq_error;

    /* 
    ** If using SQL,  Save sqlstate into the ANSI SQL Diagnostic Area
    ** There really should always be an SQLSTATE if this function is
    ** called.
    */
    if (IIlbqcb->ii_lq_dml == DB_SQL && (db_sqlst != (DB_SQLSTATE *)0))
	IILQdaSIsetItem(thr_cb, IIDA_RETURNED_SQLSTATE, db_sqlst->db_sqlstate);
	
    /* If an error was already reported within LIBQ then skip DBMS error */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;

    /* Allow the TM to continue on errors that are warnings */
    if (dbgeneric == (i4)0 && dblocal == (i4)0)
    {
	if (IIglbcb->ii_gl_flags & II_G_TM)
	    edesc->ii_er_num = 0;
	return;
    }
    /* Generic numbers should always be positive (some old headers are not) */
    if (dbgeneric < 0)
	dbgeneric = -dbgeneric;
    /*
    ** Because INGRES (as of 6.1) still returns the INGRES-specific error 
    ** as dbgeneric and dblocal = 0, we assign both to the INGRES error #.
    */
    if (dblocal == 0)
	dblocal = dbgeneric;

    /* Flag allows looping statements to continue on warnings */
    if (dbgeneric == E_GE0032_WARNING || dbgeneric == 0) 
	IIlbqcb->ii_lq_curqry |= II_Q_WARN;
    else
	IIlbqcb->ii_lq_curqry &= ~II_Q_WARN;

    if (IIlbqcb->ii_lq_dml == DB_SQL)
    {
	if (edesc->ii_er_flags & II_E_DBMS)	/* Default to local errors? */
	{
	    edesc->ii_er_num  = dblocal;
	    edesc->ii_er_other = dbgeneric;
	}
	else
	{
	    edesc->ii_er_num = dbgeneric;
	    edesc->ii_er_other = dblocal;
	}
    }
    else /* QUEL is reverse */
    {
	edesc->ii_er_num = dblocal;
	edesc->ii_er_other = dbgeneric;
    }
    if (dbstatus & GCA_ERFORMATTED)
    {
	/* Already formatted (gateway) - copy to already formatted buffer */
	edesc->ii_er_estat = (dbstatus & ~GCA_ERFORMATTED);
	STlcopy(dberrtext, fbuf, min(dberrlen, sizeof(fbuf) - 2));
	STcat(fbuf, ERx("\n"));
    }
    else
    {
	/* Pre-formatted - must be INGRES error text in post-Erlookup format */
	edesc->ii_er_estat = dbstatus;
	STlcopy(dberrtext, buf, min(dberrlen, sizeof(buf) - 1));
	_VOID_ IIUGfemFormatErrorMessage(fbuf, sizeof(fbuf), buf, dbtimestmp);
    }
    IIer_save( IIlbqcb, fbuf );

    /*
    ** If there is a handler, and it points at something that returns
    ** non-0, and this is not an ESQL error, print it.
    */
    if (    seterr 
	&&  (!IISQ_SQLCA_MACRO(IIlbqcb))
	&&  ! (edesc->ii_er_flags & II_E_PRT)  
	&&  ! err_hdl
       )
    {
	i4	er_num = edesc->ii_er_num;

	edesc->ii_er_flags |= II_E_PRT;
	if ((*seterr)(&er_num) == IIerOK)
	{
	    edesc->ii_er_flags &= ~II_E_PRT;
	    return;
	}
	edesc->ii_er_flags &= ~II_E_PRT;
    }
    if ( err_hdl  &&  ! (thr_cb->ii_hd_flags & II_HD_ERHDLR) )
    {
	IILQsepSessionPush( thr_cb );
	thr_cb->ii_hd_flags |= II_HD_ERHDLR;
	_VOID_ (*err_hdl)();
	thr_cb->ii_hd_flags &= ~II_HD_ERHDLR;

	/*
	** Issue error if user has switched sessions in handler and not
	** switched back.
	*/
	_VOID_ IILQscSessionCompare( thr_cb );  /* issues the error */
	IILQspSessionPop( thr_cb );
    }


    if (    ( ! IISQ_SQLCA_MACRO( IIlbqcb )  &&  ! err_hdl )
	||  (edesc->ii_er_flags & II_E_SQLPRT)
       )
    {
	if (dbstatus & GCA_ERFORMATTED)	/* Already formatted */
	    IImsgutil(fbuf);
	else				/* Use INGRES error reporting */
	    IIUGeppErrorPrettyPrint(stdout, buf, TRUE);
    }

    if ( IIglbcb->ii_gl_flags & II_G_DEBUG )  IIdbg_dump( IIlbqcb );
    if ( IILIBQgdata->excep )  (*IILIBQgdata->excep)();
}


/*{
**  Name: IIdbermsg - Report user message from DBMS during procedure execution.
**
**  Description:
**	IIdbermsg is called by the GCA handler when a user message (inside a
**	GCA error buffer) is reported back from the DBMS.  The DBMS has
**	passed back the user's message number and/or the text of the user's
**	message.  This routine saves the message data for program processing.
**	Note that the message may be printed if used with an SQLCA.
**
**  Inputs:
**	thr_cb		Thread-local-storage control block.
**	msgno		- Message number.  If unused, then this is set to zero.
**	msglen		- Length of following message.  If there was no message
**			  text then this is set to zero.
**	msgtext		- Full message text received from DBMS (not null
**			  terminated).  If unused, this should be ignored.
**
**  Outputs:
**	
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	Error text is printed to stdout if:
**	1) this is an EQUEL program, and user has not notified us
**	   of an error handler via IIseterr() - no support yet; or
**	2) This is an ESQL program NOT using an SQLCA or has set SQLPRINT.
**	3) This is the Terminal Monitor.
**	
**	The error text is saved for use in INQUIRE_INGRES.
**	
**  History:
**	11-may-1988 - Written. (ncg)
**	19-may-1989 - Changed names of globals for multiple connections. (bjb)
**	15-mar-1991 - (kathryn) 
**              Add calls to user defined message handler if one was set.
**	01-apr-1991 - (kathryn)
**		Don't print messages when handler is set.
**	20-apr-1991 - (kathryn)
**		No longer display messages which are the result of firing a
**		rule.  Whenever handling will be used to catch these.
*/

VOID
IIdbermsg( II_THR_CB *thr_cb, i4 msgno, i4  msglen, char *msgtext )
{
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		(*msg_hdl)() = IIglbcb->ii_gl_hdlrs.ii_msg_hdl;
    i4	gl_flags = IIglbcb->ii_gl_flags;
    II_ERR_DESC	*edesc = &IIlbqcb->ii_lq_error;

    /* Save message info for status-inquiring statements */
    edesc->ii_er_mnum = msgno;
    if (msglen > 0)				/* There is some text */
    {
	if (edesc->ii_er_mbuflen <= msglen)	/* Buffer too small - free it */
	{
	    if (edesc->ii_er_msg != (char *)0)
		MEfree(edesc->ii_er_msg);
	    edesc->ii_er_msg = (char *)MEreqmem((u_i4)0, (u_i4)(msglen + 1),
						 TRUE, (STATUS *)0);
	    if (edesc->ii_er_msg == (char *)0)	/* Couldn't allocate */
		edesc->ii_er_mbuflen = 0;
	    else
		edesc->ii_er_mbuflen = msglen;
	}
	if (edesc->ii_er_msg != (char *)0)	/* Copy message text */
	    STlcopy(msgtext, edesc->ii_er_msg, msglen);
    }
    else if (edesc->ii_er_mbuflen > 0)		/* No text - zap previous ? */
    {
	edesc->ii_er_msg[0] = '\0';
    }

    /* Allow the TM to continue on errors that are warnings */

    /* Note that IIsqSaveErr is only called if this is an ESQL statement */
    if (IISQ_SQLCA_MACRO(IIlbqcb))
	_VOID_ IIsqSaveErr( IIlbqcb, FALSE, msgno, msgno, edesc->ii_er_msg );

    if ( msg_hdl  &&  ! (thr_cb->ii_hd_flags & II_HD_MSHDLR) )
    {
	IILQsepSessionPush( thr_cb );
        thr_cb->ii_hd_flags |= II_HD_MSHDLR;
	_VOID_ (*msg_hdl)();
	thr_cb->ii_hd_flags &= ~II_HD_MSHDLR;

	/*
	** Issue error if user has switched sessions in handler and not
	** switched back.
	*/

	_VOID_ IILQscSessionCompare( thr_cb );  /* issues the error */
	IILQspSessionPop( thr_cb );

    }
    /*
    ** Display messages, if we don't have an SQLCA, or we're in the TM, or
    ** the logical SQLPRINT has been defined.  Format is similar to 
    ** errors. 
    */
    if ( ( ! msg_hdl  && 
           ( ! IISQ_SQLCA_MACRO( IIlbqcb )  ||  gl_flags & II_G_TM ) )  ||
	 edesc->ii_er_flags & II_E_SQLPRT )
    {
	IIdbmsg_disp( IIlbqcb );
    }
} /* IIdbermsg */


/*{
+*  Name: IIer_save - Save the current error number and message, if desired.
**
**  Description:
**	The error number and message are saved for later use by the
**	INQUIRE_INGRES statement.  If the query language is QUEL, the
**	convention is to save the LAST error; if the query language is
**	SQL, we save the FIRST.  Thus, if the error comes from an
**	ESQL statement and the current ESQL statement has already logged an
**	error, we do not save the error the second error.  This ensures that 
**	the INQUIRE_INGRES status information remains in sync with the status 
**	information stored in the SQLCA.
**	The error id is also saved for use by WHENEVER .. SQLPRINT.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	msg		- The formatted error message text.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	08-oct-1986 - Written. (mrw)
**	04-may-1987 (rogerk)
**	    Increased length of message buffer that is allocated by one to
**	    make room for the null terminator.
**	21-oct-1987 (peter)
**		Remove the msgname parameter and add the printtime
**		parameter.
**	09-dec-1988 (neil)
**		Modified to remove errno and printtime parameter.
*/

VOID
IIer_save( II_LBQ_CB *IIlbqcb, char * msg )
{
    II_ERR_DESC *edesc = &IIlbqcb->ii_lq_error;
    i4		msglen;

    /* 
    ** We want to save error information if this is a QUEL statement
    ** OR if this is an ESQL statement and the error is the first one in 
    ** the statement (we don't overwrite existing ESQL errors). 
    ** Note that IIsqSaveErr is only called if this is an ESQL statement.
    */
    if (   !IISQ_SQLCA_MACRO(IIlbqcb)
	|| IIsqSaveErr(IIlbqcb,TRUE,edesc->ii_er_num,edesc->ii_er_other,msg))
    {
      /* Save error info for status-inquiring statements */
	edesc->ii_er_snum = edesc->ii_er_num;
	msglen = STlength(msg);
	if (edesc->ii_er_sbuflen <= msglen)
	{
	    if (edesc->ii_er_smsg != (char *)0)
		MEfree( edesc->ii_er_smsg );
    	    if ((edesc->ii_er_smsg = (char *)MEreqmem((u_i4)0,
		(u_i4)(msglen + 1), TRUE, (STATUS *)NULL)) == NULL)
	    {
	        edesc->ii_er_smsg =  (char *)0;
		msglen = 0;
	    }
	    edesc->ii_er_sbuflen = msglen;
	}
        if (edesc->ii_er_smsg)
	    STlcopy( msg, edesc->ii_er_smsg, msglen );
    }
}


/*{
+*  Name: IIret_err - returns its single argument for IIerror.
**
**  Description:
**
**  Inputs:
**	err		- Pointer to the error number.
**
**  Outputs:
**	Returns:
**	    The pointed-at error number.
**	Errors:
**	    None.
-*
**  Side Effects:
**	None.
**  History:
**	dd-mmm-yyyy - text (?)
*/

i4
IIret_err(err)
i4	*err;
{
    return *err;
}

/*{
+*  Name: IIno_err - returns 0.
**
**  Description:
**	Called from IIerror (through (*ii_gl_seterr)())
**	to supress error message printing.
**
**  Inputs:
**	err		- Pointer to the error number.
**
**  Outputs:
**	Returns:
**	    0.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	dd-mmm-yyyy - text (?)
*/

i4
IIno_err(err)
i4	*err;
{
    return (i4)0;
}

/*{
+*  Name: IItest_err - Return the current value of the error flag.
**
**  Description:
**
**  Inputs:
**	None.
**
**  Outputs:
**	Returns:
**	    The current value of the error flag.
**	Errors:
**	    None.
-*
**  Side Effects:
**	None.
**  History:
**	dd-mmm-yyyy - text (?)
*/

i4
IItest_err()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    return( IIlbqcb->ii_lq_error.ii_er_num );
}

/*{
+*  Name: IILQfrserror - Call user's error handler, if any.
**
**  Description:
**	This routine is called by the forms system so that errors may be
**	handled by the user's own error handler, if one has been defined
**	via IIseterr().  If not, this routine returns the passed in
**	error-id.
**
**  Notes:
**	This function does not set the RETURNED_SQLSTATE field of the
**	ANSI SQL Diagnostic Area, as the forms system is a vendor
**	extension.
**	
**  Inputs:
**	errno		- Number of current error
**
**  Outputs:
**	Nothing.
**	Returns:
**	    Results of user's error handler, if any
**	    or incoming errorno.
**	Errors:
**	    None.
-*
**  Side Effects:
**	User's error handler, if one has been declared, will be called.
**	
**  History:
**	02-sep-1987 - written (bjb)
**	23-sep-91 (seg)
**	    Can't declare errno locally -- it's redefined by OS/2 C libraries.
**	03-nov=1992 (larrym)
**	    Dealt with SQLSTATES.  For now, forms errors won't generate an
**	    SQLSTATE, but if we decide they should then the code's in here.
**	    to do it, it just needs to be uncommented.
*/

ER_MSGID
IILQfrserror(errnum)
ER_MSGID	errnum;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    II_ERR_DESC	*edesc = &IIlbqcb->ii_lq_error;
    i4	(*seterr)() = IIglbcb->ii_gl_seterr;

    if ( seterr  &&  ! (edesc->ii_er_flags & II_E_PRT) )
    {
	edesc->ii_er_flags |= II_E_PRT;

	if ( (*seterr)( &errnum ) == IIerOK )
	{
	    edesc->ii_er_flags &= ~II_E_PRT;
	    return( IIerOK );
	}

	edesc->ii_er_flags &= ~II_E_PRT;
    }

    return( errnum );
}


/*{
+*  Name: IImsgutil -- Dump error message to output, test for IIform_on.
**
**  Description:
**	Outputs to standard error when Forms system is not active.
**	Forms system output is to terminal and prompts for a return.
**
**  Inputs:
**	bufr		- The message to emit.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	22-jul-1987 (jhw) - Changed to output to 'IIerrout', settable error
**			    output file.
**	06-jun-1988 (ncg) - Resurrected after IIUGmsg proved inappropriate
**			    for long messages.
*/

VOID
IImsgutil(bufr)
char	*bufr;
{
    if (IILIBQgdata->form_on && IILIBQgdata->win_err)
    {
	(*IILIBQgdata->win_err)(bufr);
    }
    else
    {
        /* Make sure the channel is open for I/O messages */
	if (!SIisopen(stdout))
	    IImain();
	SIfprintf(stdout, ERx("%s"), bufr);
	SIflush(stdout);
    }
} /* IImsgutil */


/*{
**  Name: IIdbmsg_disp - Display database procedure messages.
**
**  Description:
**	The message number and/or message text is formatted using IIUGfmt.
**	After formatting we use IImsgutil to print it.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None.
**
**  Side Effects:
**	
**  History:
**	11-may-1988	- Written (ncg)
**	07-jun-1988	- Modified to use IIUGfmt and IImsgutil because
**			  IIUGmsg displays only 1 line if in forms mode. (ncg)
**	21-jun-1993 (larrym)
**	    fixed bug 50589 - we were suppressing printing a MESSAGE 
**	    statement if the message num was 0.  Now were not.
*/

VOID
IIdbmsg_disp( II_LBQ_CB *IIlbqcb )
{
    II_ERR_DESC	*edesc = &IIlbqcb->ii_lq_error;
    i4	msgno;
    char	*msgtext;
    char	buf[ER_MAX_LEN +2];

    /* Make sure the channel is open for I/O messages */
    if (!SIisopen(stdout))
	IImain();

    msgno = edesc->ii_er_mnum;
    if ((msgtext = edesc->ii_er_msg) != (char *)0)
    {
	if (*msgtext == '\0')
	    msgtext = (char *)0;
    }
    buf[0] = EOS;
    if (msgtext != (char *)0)		/* Number and text */
	IIUGfmt(buf, ER_MAX_LEN, ERget(F_LQ0301_UMSG_BOTH), 2, &msgno, msgtext);
    else /* Just number */
	IIUGfmt(buf, ER_MAX_LEN, ERget(F_LQ0303_UMSG_NUMBER), 1, &msgno);

    if (!IILIBQgdata->form_on)
	STcat(buf, ERx("\n"));
    
    IImsgutil(buf);
} /* IIdbmsg_disp */

/*{
** Name:	IIfakeerr - Fake a LIBQ-style error from outside a query
**
** Description:
**	Some conditions are best treated like DBMS errors even though they
**	don't involve either the DBMS or LIBQ directly.  An example is a
**	failure involving QG's temp file.  To the client, the problem is that
**	his SELECT failed; that the failure was in QG and not in LIBQ (or
**	lower down) is irrelevant.
**
**	This routine is used to 'fake' a LIBQ error.  It can only be called 
**	when no query is active.
**
** Inputs:
**	dml		i4		DML which caused the error
**	genericno 	i4 	generic error number
**	errnum		i4		DBMS-specific error number
**	severity	i4		error severity
**	parcount	i4		number of error parameters
**	a1-a5		PTR		error parameters
**	
** Re-entrancy:
**	Re-entrant
**
** History:
**	4/94 (Mike S) Initial version
**	02-apr-1999 (somsa01)
**	    Corrected argument to IIsqInit().
*/
VOID
IIfakeerr( i4 dml,i4 genericno, i4 errnum, i4 severity, i4 parcount, ...)
{
    va_list ap;

    va_start( ap, parcount );

    if (dml == DB_SQL)
	IIsqInit((struct s_IISQLCA *)0);
    IIlocerr(genericno, errnum, severity, parcount, ap);

    va_end( ap );
    IIqry_end();
}

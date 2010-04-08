/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<pc.h>		 
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<si.h>		/* needed for iicxfe.h */
# include	<st.h>
# include	<lo.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<adf.h>
# include	<gca.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iilibqxa.h>
# include	<lqgdata.h>
# include	<erlq.h>

/**
+*  Name: iiingres.c - Starts up the back end.
**
**  Description:
**	IIdbconnect calls "ingres" to fire up the back end.  IIingopen
**	is a cover routine for EQUEL programs, which sets the DML to QUEL.
**	There is a similar cover routine for ESQL programs (IIsqConnect()).
**
**  Defines:
**	IIingopen	- QUEL connection to INGRES
**	IIdbconnect	- General connection to any DBMS
**	IIx_flag	- Build the connection flag for child process
**
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	05-feb-1988	- Modified IIdbconnect to handle WITH clause (ncg)
**	18-feb-1988	- Modified IIdbconnect to call PCatexit (marge)
**	12-dec-1988	- Generic error processing. (ncg)
**	19-may-1989	- Changed for multiple connections. (bjb)
**	23-mar-1990	- Fixed bug b9157. (bjb)
**	14-jun-1990 (barbara)
**	    Call IIsendlog whether or not connection was successful.
**	    Some of the II_EMBED_SET values (e.g. PROGRAMQUIT) apply even
**	    if no connection.
**	05-jul-1990 (barbara)
**	    Above fix didn't work if second or later multiple connection
**	    failed. (II_G_CONNECT flag was already set and IIsendlog tried to
**	    send ING_SET queries).  Fix is to call IIsendlog in two places.
**	21-sep-1990 (barbara)
**	    Reading of II_EMBED_SET logical is now done before connection.
**	    This allows us to process NOTIMEZONE; also to trace connection.
**	03-oct-1990 (barbara)
**	    Backed out time zone fix until dbms is ready.
**      12-oct-1990 (stevet)
**          Added IIUGinit call to initialize character set attribute table.
**	04-nov-90 (bruceb)
**		Added ref of IILIBQgdata so file would compile.
**	14-nov-1990 (barbara)
**	    DBMS says it's time to go ahead with timezone fix.
**	28-nov-1990 (barbara)
**	    Fixed bug 33202.  Initialize rowcount to "no rows" value.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	    All non-fatal errors on connection now leave application in
**	    "no session".
**	14-mar-1991 (teresal)
**	    Add SAVEQUERY feature.
**	14-mar-1991 (teresal)
**	    Modify IIdbconnect to change behavior of when tracing
**	    of GCA messages and queries is enabled (PRINTGCA and
**	    PRINTQRY).  PRINTGCA will now trace connection flags for
**	    subsequent multiple session not just for the first session.
**	    PRINTGCA, PRINTQRY, and SAVEQUERY can now be enabled 
**	    if SET_INGRES is done between sessions or before the
**	    first connection.
**	21-mar-1991 (teresal)
**	    Put in desktop porting changes.
**	    
**	13-mar-1991 (barbara)    
**	    If 'trap-server-tracing' (PRINTTRACE) enabled, open/write
**	    to file at connect time.
**	17-aug-91 (leighb) DeskTop Porting Change: added cv.h
**	19-feb-92 (leighb) DeskTop Porting Change: IIbreak() is a VOID.
**	6-apr-1992 (fraser - for leighb)
**	    Added this history entry.  Removed # ifdef MAXARG ... # endif
**	    surrounding # undef MAXARG.  Note: there is a better way to
**	    resolve the conflict between the definition of MAXARG in this
**	    file and its definition elsewhere; no time to do it today
** 	    because it's INTREGRATION DAY. 
**	09-dec-1992 (teresal)
**	    Modified ADF control block to be per session rather
**	    than global (part of decimal changes).
**	21-dec-1992 (larrym)
**	    added support for INQURE_SQL (CONNECTION_TARGET)
**	07-jan-1993 (larrym)
**	    Created MAXCONARG to take care of problem noted by Fraser above.
**	18-jan-1993 (larrym)
**	    Added <si.h>.  A modification to iicxfe.h required this
**	26-jan-1993 (teresal)
**	    Replaced call to FEadfcb() with a LIBQ interface to allocate
**	    an ADF control block. This allows us to have a separate 
**	    ADF control block per session.
**	07-apr-1993 (larrym)
**	    Modified to accept session -98 as an indicator to generate both
**	    a session ID and a connection name.  This was needed for XA
**	15-jun-1993 (larrym)
**	    fixed bug 50756, where disconnection to a non-existent session
**	    gave you two errors, one for the session not existing, one
**	    for trying to disconnect it.  Ended up reworking a lot of the
**	    session stuff.  See iisqcrun.c, iiinqset.c, and iilqsess.c for
**	    more info.
**	22-jul-1993 (seiwald)
**	    Changed IIdbconnect to pass remote connection arguments
**	    (remote_system_user, remote_system_password) to IIcgcconnect.
**	26-jul-1993 (larrym)
**	    fixed bug where -X failed on duplicate connection name.
**	19-Aug-1993 (larrym)
**	    fixed bug with always reseting the II_G_WITHLIST. 
**	9-nov-93 (robf)
**          Add support for -dbms_password
**	19-jan-1994 (larrym)
**	    Fixed bug 58029; when allocating a new IIlbqcb, the address 
**	    of the user's SQLCODE was not copied into the new cb.  If an
**	    error occured, the user's SQLCODE would not get updated with
**	    the error number.  This is similar to bug 7565
**      18-Feb-1994 (daveb) 33593, 35805, 35873, 36034, 38382,
**  	    42323, 45002, 45436, 47179, 51315, 51673, 54404
**          change IIdbconnect to not to the IIsendlog; we'll always
**  	    do it in IIsqParms after sending client connection information.
**          Don't send client info on -X.
**	25-Apr-1994 (larrym)
**	    Fixed bug 61137, connection failure not getting propigated to 
**	    correct libq control block.  Modified interface to 
**	    IILQcseCopySessionError to allow specification of source and
**	    destination control blocks (see iilqsess.c also)
**	25-Jul-96 (gordy)
**	    Added support for local_system_user and local_system_password.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	30-Apr-99 (gordy)
**	    Copy any error info to the new current session when
**	    a connection request fails.
**      22-oct-2007 (rucmi01) Bug 120317
**          In IIdbconnect() reset out flags (in "outargs") to
**          default values if flag for that outarg is not set (e.g. -f float 
**          flag). Otherwise new connections take on same formatting values as
**          previous connection, if it had an explicit flag setting.
**/

# define	MAXCONARG	16	/* Max number of args to IIingopen */
# define	ARGLEN 		127	/* Allow for -X max name size */

FUNC_EXTERN VOID	IIdbconnect();
FUNC_EXTERN i4		IIhdl_gca();		 
FUNC_EXTERN VOID	IIbreak();		 
FUNC_EXTERN VOID	IIlqExit();

GLOBALREF LIBQGDATA	*IILIBQgdata;
GLOBALCONSTREF  ADF_OUTARG      Adf_outarg_default; /*Default output fmts.*/


/*{
+*  Name: IIingopen - Set DML to QUEL and call IIdbconnect
**
**  Description:
**	Set LIBQ's query language global flag to QUEL, and pass on all
**	parameters to IIdbconnect().
**
**  Inputs:
**	lang  - language (for backward compatibility)
**	pn    - parameters for ingres command line
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    IIglbcb->ii_lq_dml gets set to DB_QUEL.
**	
**  History:
**	22-apr-1987 - written (bjb)
**	07-jan-1993 (larrym)
**	    Changed call to IIdbconnect to have 18 params (to be consistent
**	    with IIsqConnect's usage of it).
*/

VOID
IIingopen( lang, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, 
	   p13, p14, p15 )
i4	lang;
char	*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10,
	*p11, *p12, *p13, *p14, *p15;
{
    IIdbconnect( DB_QUEL, lang, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
		p11, p12, p13, p14, p15, (PTR) 0 );
}

/*{
+*  Name: IIdbconnect - Initialize access to INGRES for this process.
**
**  Description:
**	IIdbconnect() sets some default status information in LIBQ, 
**	processes INGRES arguments and connects to the database.
**	It may  be called from an embedded program (via IIingopen for
**	EQUEL programs and via IIsqConnect for ESQL programs) or it may
**	be called direct from front end code.
**	If a session id has been saved by IILQsidSessionId(), this
**	connection is intended by the user to be one of several database
**	connections. 
**
**  Inputs:
**	dml   - DML language
**	lang  - Host language (for backward compatibility)
**	pn    - Parameters for ingres command line
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    TWODB	Already connected to DBMS
**	    Errors connected with start up.
-*
**  Side Effects:
**	
**  History:
**	07-feb-1985 - 	Modified interface to IN, and lang dependencies. (ncg)
**	13-mar-1986 -	Added setting of IIos_errno so operating system errors
**			can be detected as well as ingres errors.  Also changed
**			check for err_exit so it will print o/s errors before
**			returning.  This section is only used by rtingres and 
**			ingcntrl. (Bug 7572) (marian)
**	26-feb-1987 -	Added call to FEadfcb() to get ADF control block (bjb)
**	05-feb-1988 - 	Checked for WITH list on CONNECT statement.  The WITH
**			list is normally processed till IIsqParms, but in
**			error cases it must be turned off here. (ncg)
**	18-feb-1988 -	Added call to PCatexit() to push cleanup routine for
**			ATEX recovery feature.  (marge)
**	22-jun-1988 - 	Modified interface to IIcgc_connect for gateways and
**			INGRES/NET. (ncg)
**	07-sep-1988 - 	Check for no database name (ncg)
**	20-may-1989 -	Changed for multiple connections.  Must now set global
**			II_GLB_CB as well as local II_LBQ_CB states.  Also must
**			distinguish between single and multiple connects. (bjb)
**	18-aug-1989 -	Bug #7565: Newly allocated IIlbqcb must inherit previous
**			IIlbqcb's sqlca pointer for correct error handling.(bjb)
**	09-may-1990 -	Fixed up error handling for illegal multiple conn. (bjb)
**	05-jul-1990 (barbara)
**	    Call IIsendlog if connection succeeded and also if it failed.
**	    In the latter case we want to set some II_EMBED_SET logicals only.
**	23-aug-1990 (barbara)
**	    Handle null dbname.  Previously a null dbname caused code to
**	    bypass ingarv loop.  err_exit wasn't set and program exited
**	    without a message.   (bug 21727)
**	21-sep-1990 (barbara)
**	    Handled II_EMBED_SET logicals separately from database logicals.
**	    Reading of II_EMBED_SET logical is now done before connection.
**	    This allows us to process NOTIMEZONE; also to trace connection.
**	    Pass to IIcgc_connect whether or not to send timezone info to dbms.
**	03-oct-1990 (barbara)
**	    Backed out time zone fix until dbms is ready.
**      12-oct-1990 (stevet)
**          Added IIUGinit call to initialize character set attribute table.
**	09-nov-1990 (teresal)
**	    Modify IIdbconnect to parse the ADF format flags and update the
**          FE ADF control block with these flags.  Parsing of output format 
**	    flags used to be done in cgcassoc.c (Bug fix 33675).  Also moved
**	    the processing of logicals to be before parsing of format flags
**	    so bad format flag errors will be local or generic error
**	    depending on II_EMBED_SET logical.
**	14-nov-1990 (barbara)
**	    DBMS says it's time to go ahead with timezone fix.
**	28-nov-1990 (barbara)
**	    Fixed bug 33202.  Initialize rowcount to "no rows" value.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	    All non-fatal errors on connection now leave application in
**	    "no session".
**	28-jan-1990 (teresal)
**	    Modified the "-f" flag so that if scale is not given,
**	    e.g., "-f8n15", the default scale is zero not three.
**	    Also, fixed it so "-f" error will display entire flag. 
**	11-mar-1991 (teresal)
**	    Unknown flags no longer generate the LIBQ error 
**	    E_LQ0006, these flags are now sent on to the server
**	    via LIBQGCA as GCA_MISC_PARM.
**	13-mar-1991 (barbara)    
**	    If 'trap-server-tracing' (PRINTTRACE) enabled, open/write
**	    to file at connect time.
**	15-apr-1991 (kathryn)
**	    Set II_G_APPTRC for child processes.  The trace files for
**	    "printqry, printtrace and printgca" were closed in IIx_flag()
**	    and should be re-opened in append so that all tracing from the
**	    child process is appended to the file of the parent process.
**	12-aug-1991 (teresal)
**	    IIlbqcb must set ii_er_flags to detect if local errors have
**	    been set outside of a session. (Bug 37573)
**	01-dec-1991 (kathryn)  Bug 41476
**	    When II_G_QRYTRC is set - Always call IIdbg_info at connect time, 
**	    even when file is already open so that IIlbqcb.ii_lq_dbg will be 
**	    allocated.
**      03-nov-1992 (mani)
**          Make the processing of LIBQ II_EMBED_SET logicals conditional on
**          XA extensions *not* being turned on. If XA is turned on, LIBQ
**          logicals are processed on the first xa_open(), and not here.
**	17-nov-1992 (larrym)
**	    Changed interface to IILQsnSessionNew(), now passing ptr to 
**	    session_id (&ses_id), to support system generated session id's.
**	    If the ses_id saved in IIglbcb->ii_gl_sstate.ss_idsave == special
**	    SysGen constant, we figure out the next available session id,
**	    connect to it, and return it in ses_id.  See iilqsess.c for more
**	    info.
**	19-nov-1992 (larrym)
**	    Changed call to IIxa_check_if_xa to IIXA_CHECK_IF_XA_MACRO.
**	02-dec-1992 (larrym)
**	    Checks first thing if this is an xa app, if so, connects only
**	    allowed in certain contexts.
**	07-jan-1993 (teresal)
**	    Use LIBQ's routine IILQasAdfcbSetup() instead of directly
**	    calling FEadfcb() to allocate an ADF Control Block.
**	25-jan-1993 (larrym)
**	    removed special XA check before processing LIBQ logicals.  XA
**	    now has its own logical(s) that it checks when it needs to, and
**	    LIQB resumes its checking of logicals at connection time.
**	02-mar-1993 (larrym)
**	    Modified for XPG4 connections; now all sessions are 'multi'.
**	25-jun-1993 (kathryn)	Bug 51573
**	    Update default adf control block as well as 
**	    session adf control block with formatting flags
**	    so that the FEadfcb will have correct info for TM.
**	23-Aug-1993 (larrym)
**	    Fixed bug 54431, where we were never processing the with
**	    clause as a result of changes made on 02-mar-1993.
**      18-Feb-1994 (daveb) 33593, 35805, 35873, 36034, 38382,
**  	    42323, 45002, 45436, 47179, 51315, 51673, 54404
**          change IIdbconnect to not to the IIsendlog; we'll always
**  	    do it in IIsqParms after sending client connection information.
**	25-Jul-96 (gordy)
**	    Added support for local_system_user and local_system_password.
**	30-Apr-99 (gordy)
**	    Copy any error info to the new current session when
**	    a connection request fails.
**       7-Sep-2004 (hanal04) Bug 48179 INGSRV2543
**          Do no apply ING_SET if -Z flag used in the CONNECT.
*/

VOID
IIdbconnect( dml, lang, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, 
	   p13, p14, p15, p16 )
i4	dml;
i4	lang;
char	*p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11, *p12, *p13, 
	*p14, *p15, *p16;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    char	*usrargv[MAXCONARG +1];		/* Filled with pn's */
    char	**usrptr;			/* Points at usrargv */
    char	argdat[MAXCONARG +1][ARGLEN +1];/* Data for Ingres */
    char	*ingargv[MAXCONARG +1];		/* Argv for Ingres */
    i4		ingargc;			/* Count of Ing args */
    char	*cp;
    STATUS	status;
    DB_STATUS	db_status;
    i4		child = FALSE;			/* Was spawned */
    bool	err_exit = TRUE;		/* Restrict exit on error */
    bool	unknown_flag = FALSE;		/* Unknown flag found */
    bool	gen_conname_flag = FALSE; 	/* should we gen a connname?*/
    char	dbname[MAX_LOC + 1];
    bool	withlist;
    bool	con_name_valid;			/* is connection name valid? */
    i4		ses_id;				/* Session id */
    II_LBQ_CB	*IIoldlbqcb;			/* Prev. connection (or NULL)*/
    i4		flag_float;     		/* Flags floating point: */
    char	fstyle_val;     		/*    Style */
    i4	fwidth_val;     		/*    Width */
    i4	fprec_val;      		/*    Precision */	
    i4	tmp_val;			/* Temp result of CVal */
    char	*cptemp;			/* Temp pointer */	
    char	*lcl_username = NULL;		/* local username */
    char	*lcl_password = NULL;		/* local password */
    char	*rem_username = NULL;		/* remote username */	
    char	*rem_password = NULL;		/* remote passwd */	
    char	*dbms_password=0;		/* dbms passwd */
    II_LBQ_CB   *def = &thr_cb->ii_th_defsess;
    /* bools for which flags have been set */
    bool        c0Set=FALSE, i1Set=FALSE, i2Set=FALSE, i4Set=FALSE,
                f4Set=FALSE, f8Set=FALSE, t0Set=FALSE;
    
# ifdef II_DEBUG
    if (IIdebug)
	SIprintf( ERx("IIingres %s\n"), p1 );
# endif

    if (IIXA_CHECK_IF_XA_MACRO)
    {
	/* Connects only allowed if INGRES STATE is INTERNAL */
	if (IIXA_CHECK_SET_INGRES_STATE(IICX_XA_T_ING_INTERNAL))
	{
	    /* IIXA_CHECK_... will generate an XA error */
	    IIlocerr(GE_HOST_ERROR, E_LQ0001_STARTUP, II_ERR, 0, (char *)0);
	    return;
	}
    }

    IImain();

    /* 
    ** Save WITHLIST flag.  Make sure II_T_WITHLIST is off so 
    ** if we do have to restore the old connection, we won't 
    ** try to do any with-list processing.
    */
    withlist = (thr_cb->ii_th_flags) & II_T_WITHLIST ? TRUE : FALSE;
    thr_cb->ii_th_flags &= ~II_T_WITHLIST;

    /*
    ** Save off ses_id and con_name.
    */
    ses_id = thr_cb->ii_th_sessid;
    thr_cb->ii_th_sessid = 0;
    con_name_valid = (thr_cb->ii_th_flags & II_T_CONNAME) ? TRUE : FALSE;
    thr_cb->ii_th_flags &= ~II_T_CONNAME;

    /*
    ** Generate connection name once the session ID is assigned.
    */
    if ( (ses_id  &&  ! con_name_valid)  ||  ses_id == IIsqconSysGen )
	gen_conname_flag = TRUE;

    /* did user try to connect using session zero? */
    if ( thr_cb->ii_th_flags & II_T_SESSZERO )
    {
        IIlocerr(GE_LOGICAL_ERROR, E_LQ00BC_SESSZERO, II_ERR, 0, (PTR)0);
	thr_cb->ii_th_flags &= ~II_T_SESSZERO;
	return;
    }

    /* save off current connect state */
    IIoldlbqcb = IIlbqcb;
    IILQcseCopySessionError( &IIlbqcb->ii_lq_error, 
			     &thr_cb->ii_th_defsess.ii_lq_error );
    IIlbqcb->ii_lq_error.ii_er_num = IIerOK;
    IILQssSwitchSession( thr_cb, NULL );	/* Switch to default */

    /*
    ** We pass the address of ses_id now.  If 
    ** ses_id == 0, then a system generated 
    ** session will be passed back. 
    */
    if ( ! (IIlbqcb = IILQsnSessionNew( &ses_id )) )
    {
	/* restore old connection state */
	IILQssSwitchSession( thr_cb, IIoldlbqcb );
	IIlbqcb = thr_cb->ii_th_session;
	IILQcseCopySessionError(&thr_cb->ii_th_defsess.ii_lq_error,
				&IIlbqcb->ii_lq_error);
	return;
    }

    /* Set up current LIBQ state */
    IILQssSwitchSession( thr_cb, IIlbqcb );
    IIlbqcb->ii_lq_dml = dml;
    IIlang(lang);

    /* Bug 7565: Must copy pointer to user's SQLCA from IIsqInit */
    IIlbqcb->ii_lq_sqlca = IIoldlbqcb->ii_lq_sqlca;
    /* Bug 37573: Must copy ii_er_flag to know if local error is turned on */
    IIlbqcb->ii_lq_error.ii_er_flags = IIoldlbqcb->ii_lq_error.ii_er_flags;
    /* 
    ** Bug 58029: Must copy SQLCODE address into new sess CB so that 
    ** IIsqSetSqlCode will know to copy generror into User's SQLCODE.
    */
    IIlbqcb->ii_lq_sqlcdata = IIoldlbqcb->ii_lq_sqlcdata;

    if ( gen_conname_flag )
    {
	/*
	** we have to generate a connection name of the iin (where n is
	** the session id) variety.
	*/
	STprintf( thr_cb->ii_th_cname, "ii%d", ses_id );
	con_name_valid = TRUE;
    }

    /* Allocate an ADF control block for the current session */
    if (IILQasAdfcbSetup( thr_cb ) != OK)
    {
	/* restore old connection state */
	IILQcseCopySessionError( &IIlbqcb->ii_lq_error,
				 &IIoldlbqcb->ii_lq_error );
	IILQssSwitchSession( thr_cb, IIoldlbqcb );
	IIlbqcb = thr_cb->ii_th_session;
	IILQsdSessionDrop( thr_cb, ses_id );
	return;
    }

    /* Allocate LIBQGCA descriptor */
    if (IIcgc_alloc_cgc(&IIlbqcb->ii_lq_gca, IIhdl_gca) == FAIL)
    {
	/* restore old connection state */
	IILQcseCopySessionError( &IIlbqcb->ii_lq_error,
				 &IIoldlbqcb->ii_lq_error );
	IILQssSwitchSession( thr_cb, IIoldlbqcb );
	IIlbqcb = thr_cb->ii_th_session;
	IILQsdSessionDrop( thr_cb, ses_id );
	return;
    }

    /* Set up argument vector for ingres call */
    usrargv[0] = p1;	usrargv[1] = p2;	usrargv[2] = p3;
    usrargv[3] = p4;	usrargv[4] = p5;	usrargv[5] = p6;
    usrargv[6] = p7;	usrargv[7] = p8;	usrargv[8] = p9;
    usrargv[9] = p10;	usrargv[10] = p11;	usrargv[11] = p12;
    usrargv[12] = p13;	usrargv[13] = p14;	usrargv[14] = p15;
    usrargv[15] = p16;	usrargv[16] = (char *)0; 

    /* Process logicals */
    IILQpeProcEmbed( IIlbqcb );

    dbname[0] = '\0';
    ingargc = 0;
    usrptr = usrargv;
    /* If the database ptr is a null ptr, we want to keep going.  Bug 21727. */
    if (*usrptr == (char *)0)
    {
	ingargv[ ingargc++ ] = (char *)0;
	usrptr++;
    }
    for (; *usrptr && ingargc < MAXCONARG;)
    {
	/*
	** Pick up the local and remote username and password.
	** The switch value follows the switch as a separate argument.
	*/

        if( !STcompare( *usrptr, "-local_system_user" ))
	{
	    usrptr++; 
	    lcl_username = *usrptr++; 
	    continue;
	}

        if( !STcompare( *usrptr, "-local_system_password" ))
	{
	    usrptr++; 
	    lcl_password = *usrptr++; 
	    continue;
	}

        if( !STcompare( *usrptr, "-remote_system_user" ))
	{
	    usrptr++; 
	    rem_username = *usrptr++; 
	    continue;
	}

        if( !STcompare( *usrptr, "-remote_system_password" ))
	{
	    usrptr++; 
	    rem_password = *usrptr++; 
	    continue;
	}

	/*
	** Pick up dbms password, sent to cgc as single parameter -Ppwd
	*/
        if( !STcompare( *usrptr, "-dbms_password" ))
	{
	    usrptr++; 
	    dbms_password = *usrptr++; 
	    STprintf(argdat[ingargc], "-P%s", dbms_password);
	    ingargv[ ingargc] = argdat[ingargc];
	    ingargc++;
	    continue;
	}

	STlcopy( *usrptr++, argdat[ingargc], ARGLEN );
	cp = argdat[ ingargc ];
	STtrmwhite( cp );
        /* Get rid of any null arguments */
	if (*cp == '\0')
	    continue;
	if (*cp == '-')		/* Flag arguments? */
	{
	    II_LBQ_CB	*def = &thr_cb->ii_th_defsess;
		   
	    switch (cp[1])
	    {
	      case 'c':                         /* -cddd */
                if (CVal(&cp[2], &tmp_val) != OK)
		    /* Unknown flag - pass on to server */
		    break;
		IIlbqcb->ii_lq_adf->adf_outarg.ad_c0width = tmp_val;
		def->ii_lq_adf->adf_outarg.ad_c0width = tmp_val;
                c0Set = TRUE;
		continue;
	      case 'e':	/* Flag no longer supported */
		IIlbqcb->ii_lq_flags |= II_L_DASHE;
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0005_FLAGS, II_ERR, 1, cp);
		/* INdash_e( IIbreak ); */
		continue;
	      case '$':	/* Called from front-end */
	      case 'E': /* or from user */
		err_exit = FALSE;	
		continue;
	      case 'i':                         /* -i{1|2|4}ddd */
                switch (cp[2])
                { 
                  case '1':
                    if (CVal(&cp[3], &tmp_val) != OK)
		    {
                        unknown_flag = TRUE;
			break;
		    }	
		    IIlbqcb->ii_lq_adf->adf_outarg.ad_i1width = tmp_val;
		    def->ii_lq_adf->adf_outarg.ad_i1width = tmp_val;
                    i1Set = TRUE;
                    break;
                  case '2':
                    if (CVal(&cp[3], &tmp_val) != OK)
		    {
                        unknown_flag = TRUE;
			break;
		    }	
		    IIlbqcb->ii_lq_adf->adf_outarg.ad_i2width = tmp_val;
		    def->ii_lq_adf->adf_outarg.ad_i2width = tmp_val;
                    i2Set = TRUE;
                    break;
                  case '4':
                    if (CVal(&cp[3], &tmp_val) != OK)
		    {
                        unknown_flag = TRUE;
			break;
		    }	
		    IIlbqcb->ii_lq_adf->adf_outarg.ad_i4width = tmp_val;
		    def->ii_lq_adf->adf_outarg.ad_i4width = tmp_val;
                    i4Set = TRUE;
                    break;
                  default:
                    unknown_flag = TRUE;
                }      
		/* Pass unknown flags on to server */
		if (unknown_flag)
		    break;	
                continue;	
	      case 'f':                         /* -f{4|8}{e|f|g|n}ddd[.ddd] */
                if (cp[2] == '4')
                    flag_float = 4;
                else if (cp[2] == '8')
                    flag_float = 8;
                else
		    /* Unknown flag - pass on to server */
		    break;	

                CVlower(&cp[3]);
                if (   cp[3] == 'e'
                    || cp[3] == 'f'
                    || cp[3] == 'g'
                    || cp[3] == 'n')
                {
                    fstyle_val = cp[3];
                }
                else
                {
		    /* Unknown flag - pass on to server */
		    break;	
                }		
		fprec_val = 0;          /* In case of default */
                if (cptemp = STindex(&cp[4], ERx("."), 0))
                { 
                    *cptemp = '\0';
                    if (CVal(++cptemp, &fprec_val) != OK)
                    {
			/* Unknown flag - put dot back before 
			** sending flag to server  */
			*(--cptemp) = '.';
			break;
                    }
                } 
                if (CVal(&cp[4], &fwidth_val) != OK)
		{
		    /* Unknown flag - put dot back before 
		    ** sending flag to server  */
		    if (cptemp)
			*(--cptemp) = '.';
                    break;
		} 
                /* Set Adf float format info */
                switch (flag_float)
		{ 
                    case 4:
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f4style = fstyle_val;
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f4width = fwidth_val;
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f4prec = fprec_val;
		      def->ii_lq_adf->adf_outarg.ad_f4style = fstyle_val;
		      def->ii_lq_adf->adf_outarg.ad_f4width = fwidth_val;
		      def->ii_lq_adf->adf_outarg.ad_f4prec = fprec_val;
                      f4Set = TRUE;
                      break;
                    case 8:
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f8style = fstyle_val;
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f8width = fwidth_val;
                      IIlbqcb->ii_lq_adf->adf_outarg.ad_f8prec = fprec_val;
		      def->ii_lq_adf->adf_outarg.ad_f8style = fstyle_val;
		      def->ii_lq_adf->adf_outarg.ad_f8width = fwidth_val;
		      def->ii_lq_adf->adf_outarg.ad_f8prec = fprec_val;
                      f8Set = TRUE;
                      break;
                }
                continue;
	      case 'Q':	/* FE does not want QUEL cursor/xact compatibility */
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0005_FLAGS, II_ERR, 1, cp);
		continue;
	      case 'L':	/* Query language flag ignored */
		continue;
	      case 't':                         /* -tddd */
                if (CVal(&cp[2], &tmp_val) != OK)
		    break;
		IIlbqcb->ii_lq_adf->adf_outarg.ad_t0width = tmp_val;
		def->ii_lq_adf->adf_outarg.ad_t0width = tmp_val;
                t0Set = TRUE;
		continue;
	      case 'X':	/* Child process */
		/*
		** If we are in a child process, then we must suppress any
		** gateway WITH clause, as session startup parameters are
		** not even processed.  -X, though, does need to be passed
		** to LIBQGCA.
		*/
		child = TRUE;
		withlist = FALSE;
		IIglbcb->iigl_qrytrc.ii_tr_flags |= II_TR_APPEND; 
		IIglbcb->iigl_msgtrc.ii_tr_flags |= II_TR_APPEND; 
		IIglbcb->iigl_svrtrc.ii_tr_flags |= II_TR_APPEND; 
		break;
              case 'Z': /* Do not apply ING_SET */
                IIlbqcb->ii_lq_flags |= II_L_NO_ING_SET;
                continue;
	    } /* switch (cp[1]) */
	}
	else if (*cp != '+' && !CMwhite(cp))	/* Save db name */
	{
	    STlcopy(cp, dbname, MAX_LOC);
	}  /* if (*cp == '-') */
	ingargv[ ingargc++ ] = cp;
    } /* for usrptr loop */

    /*
    ** For flags which have not been set, set default values (-1)
    */

    if (c0Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_c0width =
                        Adf_outarg_default.ad_c0width;
        def->ii_lq_adf->adf_outarg.ad_c0width =
                        Adf_outarg_default.ad_c0width;
    }
    if (i1Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_i1width =
                        Adf_outarg_default.ad_i1width;
        def->ii_lq_adf->adf_outarg.ad_i1width =
                        Adf_outarg_default.ad_i1width;
    }
    if (i2Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_i2width =
                        Adf_outarg_default.ad_i2width;
        def->ii_lq_adf->adf_outarg.ad_i2width =
                        Adf_outarg_default.ad_i2width;
    }
    if (i4Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_i4width =
                        Adf_outarg_default.ad_i4width;
        def->ii_lq_adf->adf_outarg.ad_i4width =
                        Adf_outarg_default.ad_i4width;
    }
    if (f4Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f4style =
                        Adf_outarg_default.ad_f4style;
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f4width =
                        Adf_outarg_default.ad_f4width;
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f4prec =
                        Adf_outarg_default.ad_f4prec;
        def->ii_lq_adf->adf_outarg.ad_f4style =
                        Adf_outarg_default.ad_f4style;
        def->ii_lq_adf->adf_outarg.ad_f4width =
                        Adf_outarg_default.ad_f4width;
        def->ii_lq_adf->adf_outarg.ad_f4prec =
                        Adf_outarg_default.ad_f4prec;
    }
    if (f8Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f8style =
                        Adf_outarg_default.ad_f8style;
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f8width =
                        Adf_outarg_default.ad_f8width;
        IIlbqcb->ii_lq_adf->adf_outarg.ad_f8prec =
                        Adf_outarg_default.ad_f8prec;
        def->ii_lq_adf->adf_outarg.ad_f8style =
                        Adf_outarg_default.ad_f8style;
        def->ii_lq_adf->adf_outarg.ad_f8width =
                        Adf_outarg_default.ad_f8width;
        def->ii_lq_adf->adf_outarg.ad_f8prec =
                        Adf_outarg_default.ad_f8prec;
    }
    if (t0Set==FALSE)
    {
        IIlbqcb->ii_lq_adf->adf_outarg.ad_t0width =
                        Adf_outarg_default.ad_t0width;
        def->ii_lq_adf->adf_outarg.ad_t0width =
                        Adf_outarg_default.ad_t0width;
    }

    /*
    ** Username and password pairs must be provided
    ** together.  If either is not provided, don't
    ** pass either.
    */
    if ( ! lcl_username  ||  ! *lcl_username  ||
	 ! lcl_password  ||  ! *lcl_password )
    {
	lcl_username = NULL;
	lcl_password = NULL;
    }

    if ( ! rem_username  ||  ! *rem_username  ||
	 ! rem_password  ||  ! *rem_password )
    {
	rem_username = NULL;
	rem_password = NULL;
    }

    /*
    ** Set the tracing state for this session.
    */
    IIdbg_toggle( IIlbqcb, IITRC_SWITCH );
    IILQgstGcaSetTrace( IIlbqcb, IITRC_SWITCH );
    IILQsstSvrSetTrace( IIlbqcb, IITRC_SWITCH );

    /* If database name or child process */
    if (dbname[0] != '\0' || child)
    {
	/* Save dbname in iilbqcb to support INQUIRE CONNECTION_TARGET */
	IIlbqcb->ii_lq_con_target = STalloc(dbname);

	/*
	** If no connection name provided, use target database.
	*/
	IIlbqcb->ii_lq_con_name = con_name_valid 
				  ? STalloc( thr_cb->ii_th_cname ) 
				  : STalloc( dbname );

	/* 
	** check for unique connection name! 
	** It would be nice if we could do this in IILQsnSessionNew, 
	** but we need the connection target before we can check it.
	*/
	/* don't do check for -X connect! */
	if ( ! child && IILQsbnSessionByName(IIlbqcb->ii_lq_con_name, IIlbqcb) )
	{
	    /* duplicate connection name! */
	    IIlocerr( GE_SYNTAX_ERROR, E_LQ0041_DUPCONNAME, 
		      II_ERR, 1, IIlbqcb->ii_lq_con_name );
	    /* restore old connection state */
	    IILQcseCopySessionError( &IIlbqcb->ii_lq_error,
				     &IIoldlbqcb->ii_lq_error );
	    IILQssSwitchSession( thr_cb, IIoldlbqcb );
	    IIlbqcb = thr_cb->ii_th_session;
	    IILQsdSessionDrop( thr_cb, ses_id );
	    return;
	}

	ingargv[ ingargc ] = (char *)0;	/* Last in set */

	status = IIcgc_connect(
		    ingargc, ingargv, dml, IIglbcb->ii_gl_sstate.ss_gchdrlen, 
		    !child, !(IIglbcb->iigl_embset.iies_values & II_ES_NOZONE),
		    dbname, IIlbqcb->ii_lq_adf, IIhdl_gca, IIlbqcb->ii_lq_gca,
		    lcl_username, lcl_password, rem_username, rem_password );

	if (status == OK)			/* Everything ok */
	{
	    IIlbqcb->ii_lq_flags |= II_L_CONNECT;

	    if ( withlist )
		thr_cb->ii_th_flags |= II_T_WITHLIST;	/* Restore flag */
	    else  if ( child )
		IIsendlog( dbname, child );		/* Process logicals */
	    else
	    {
		/* send default set right now */
		thr_cb->ii_th_flags |= II_T_WITHLIST;
		IIsqParms(IIsqpEND, (char *)0, 0, dbname, child);
		thr_cb->ii_th_flags &= ~II_T_WITHLIST;		
	    }

	    /* Connect is not a row-affecting statement */
	    IIlbqcb->ii_lq_rowcount = GCA_NO_ROW_COUNT;
	    return;
	}
    }
    if (dbname[0] == '\0' && !child)
    {
	/* Bug 3339 - Flag if there was no database name */
	IIlocerr(GE_SYNTAX_ERROR, E_LQ0006_ASSOC_BAD_FLAG, II_ERR, 1, ERx(""));
    }
    else if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
    {
	IIlocerr(GE_HOST_ERROR, E_LQ0001_STARTUP, II_ERR, 0, (char *)0);
    }

    /* restore old connection state */
    IILQcseCopySessionError( &IIlbqcb->ii_lq_error,
			     &IIoldlbqcb->ii_lq_error );
    IILQssSwitchSession( thr_cb, IIoldlbqcb );
    IIlbqcb = thr_cb->ii_th_session;
    IILQsdSessionDrop( thr_cb, ses_id );

    /* Error starting INGRES - handle cases */
    if (IILIBQgdata->form_on && err_exit)
    {
	if (IILIBQgdata->f_end)
	    (*IILIBQgdata->f_end)( TRUE );
	IILIBQgdata->form_on = FALSE;
    }

    if ( err_exit ) PCexit( -1 );
    return;
}


/*
** Name: IILQaiAssocID
**
** Description:
**	Returns the GCA association ID for the current connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	i4		Association ID, -1 if not connected.
**
** History:
*/

i4
IILQaiAssocID()
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    if ( IIlbqcb->ii_lq_gca  &&  IIlbqcb->ii_lq_flags & II_L_CONNECT )
	return( IIlbqcb->ii_lq_gca->cgc_assoc_id );
    else
	return( (i4)(-1) );
}


/*{
+*  Name: IIx_flag - Return the name for the command line -X flags.
**
**  Description:
**	Interface to IIcgc_save in order to save the state of GCA.
**	This routine should be called before spawning a child process to
**	attach to the same database.  The routine calls GCA to save the state
**	and returns to the caller an ID that should be used for the command
**	line (-X flags) of the process to be spawned.
**
**  Inputs:
**	dbname		- Current database name.  This name is not verified
**			  and is only used so that the child process that will
**			  be spawned will have some kind of database-name
**			  context.  The name should be <= MAX_LOC.
**
**  Outputs:
**	minus_x		- Pointer to buffer with saved name.  The buffer should
**			  be MAX_LOC in length. The name is copied (null
**			  terminated into the buffer) and should be used with
**			  the -X flag for the child process command line.
**	Returns:
**	    		- Pointer to buffer passed in.
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	26-aug-1987 	- Written for GCA. (ncg)
**	15-apr-1991 (kathryn)
**	    Close trace files here as parent is spawning a sub-process.
**	    Set II_G_APPTRC so that on return to this parent process the 
**	    trace files will be re-opened in append mode rather than write 
**	    mode which will truncate the files.
**	01-nov-1991  (kathryn)
**	    Trace file information now stored in new IILQ_TRACE structure
**	    of IIglbcb rather than directly in IIglbcb.
*/

STATUS
IIx_flag(dbname, minus_x)
char	*dbname;
char	*minus_x;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    STATUS	stat;

    *minus_x = '\0';
    if ( IIlbqcb->ii_lq_gca  &&  IIlbqcb->ii_lq_flags & II_L_CONNECT )
	stat = IIcgc_save(IIlbqcb->ii_lq_gca, dbname, minus_x);
    else
	return FAIL;

    /* close trace files */
    IIdbg_toggle( IIlbqcb, IITRC_RESET );
    IILQgstGcaSetTrace( IIlbqcb, IITRC_RESET );
    IILQsstSvrSetTrace( IIlbqcb, IITRC_RESET );

    return stat;
}

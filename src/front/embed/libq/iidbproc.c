/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<gca.h>
# include       <fe.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<iilibqxa.h>
# include	<eqrun.h>
# include	<erlq.h>

/**
**  Name: iidbproc.c - Routines to handle database procedure access.
**
**  Defines:
**	IILQpriProcInit		- Set up state for procedure access
**	IILQprvProcValio	- Pass a parameter name and value
**	IILQprvProcGTTParm	- Pass a global temptab parameter
**	IILQprsProcStatus	- Control run-time state of execution
**
**  Notes:
**
**  History:
**	02-may-1988	- Written for 6.1 (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	24-may-1989	- Support for Dynamic SQL execution. (ncg)
**	26-mar-1990	- Use stack of session ids for nested loop statements
**			  to catch errors in switching between loops and
**			  sessions.  bug 9159. (barbara)
**	03-apr-1990	- Turn on XACT flag after successful execution of
**			  dbproc.
**	11-jun-1990	(barbara)
**	    Fixed bugs 30623 and others regarding floating literals sent
**	    as parameters to a dbproc.
**	02-jan-1991 (barbara)
**	    As part of fix to bug 35000 (null database parameter crashes
**	    server), IILQprvProcValio now checks for that condition to avoid
**	    sending an incomplete GC_DATA_VALUE.
**	15-dec-1992 (larrym)
**	    added BYREF capability to dbprocedure parameters.
**	15-jun-1993 (larrym)
**	    removed BYREF capability from dbprocedure parameters.
**	23-aug-1993 (larrym)
**	    Added owner.procedure support.  Added 3 new modules:
**	    IIproc_find to find an existing proc_id, IIproc_add to add
**	    a new procedure ID, and IIproc_send which invokes a procedure.
**	    These have been modeled after the existing qid functions in 
**	    iiqid.c; but we needed to add an owner field so these new
**	    functions were added.
**	01-sep-1993 (larrym)
**	    fixed bug with change above; ret status wasn't getting updated
**	    for GCA_PROTO_60.
**	 4-oct-96 (nick)
**	    We may get back a proc id for something other than the proc
**	    that we invoked hence we need to test for this.  #78814
**	 9-oct-96 (nick)
**	    II_DEBUG my previous change ; I've fixed the root cause ( though
**	    there may be others I guess )
**	2-may-97 (inkdo01)
**	    Added function IILQprvProcGTTParm to pass global temporary 
**	    table parameter.
**	08-nov-1999 (kitch01)
**          Bug 99170. In function IILQpriProcInit.
**          Reset the gca message type if the GCA_PROTOCOL_LEVEL is
**          less than 60. This allows an embedded SQL program to execute procedures
**          in 1.2 and then switch sessions to 6.4.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	2-may-01 (inkdo01)
**	    Added parm to ProcStatus call indicating number of result row
**	    list entries (or 0 if not row producing procedure).
**	07-Aug-2009 (drivi01)
**	    Clean up warnings, fix precedence.
**	    Drop the 6th parameter from IIputdomio function as it
**          only takes 5 parameters.
**	    This is done in efforts to port to Visual Studio 2008.
**	 1-Oct-09 (gordy)
**	    GCA changes to support long procedure names.
**/

/* 
** Values to return to program for state handling.
*/
# define	PROC_CONTINUE	1
# define	PROC_ENDLOOP	0

/* static func defs */
static IIDB_PROC_ID     *IIproc_find( II_LBQ_CB *, FE_GCI_IDENT * );
static IIDB_PROC_ID     *IIproc_add( II_LBQ_CB *, FE_GCI_IDENT * );
static VOID		IIproc_retinit( II_LBQ_CB *, i4);

/*{
**  Name: IILQpriProcInit - Set up state for DB procedure access.
**
**  Description:
**	This routine will be called for CREATE and DROP PROCEDURE as well
**	as EXECUTE PROCEDURE.
**
**	If called for CREATE or DROP PROCEDURE, then the routine only
**	prepares the GCA message buffer with the correct message id.
**	The rest of those statements are handled just like regular
**	queries (via IIwritio and IIsyncup).
**
**	The procedure name is optional for CREATE and DROP, since
**	the full text of the query is sent, with procedure names,
**	in the message.  Furthermore, multiple tables may be
**	specified with DROP, so it may not make sense to send a name.
**
**	If called for EXECUTE PROCEDURE, then the routine sets up the
**	internal state for invoking the named procedure.  If the named
**	procedure is found in the internal "DBMS id" list (also used
**	for repeated queries) then that id is used, otherwise the name
**	is used with default values.  During procedure initialization
**	the GCA procedure invocation message is built.  The run-time
**	procedure state that is set up allows us to process any
**	results which should be returned to the caller in the later
**	(looping) routine, IILQprsProcStatus.
**
**	This routine does not confirm that the identified procedure
**	name even exists, and, in fact, if it does not exist, it is
**	treated in the same way as an error during the procedure
**	invocation. 
**
**	The procedure name is not stored locally.  Any state set during
**	procedure invocation must be turned off when the global exit
**	is called (IIlqexit).
**
**  Inputs:
**	proc_type	- Reason for being called.
**			  IIPROC_CREATE - Set GCA_CREPROC message.
**			  IIPROC_DROP   - set GCA_DRPPROC message.
**			  IIPROC_EXEC	- Set GCA_INVPROC message,
**					  and deposit procedure id.
**	proc_name	- Procedure name string (null terminated, blanks
**			  ignored).  This name is only used for EXECUTE.
**  Outputs:
**	Returns:
**	    VOID
**	Errors:
**	    E_LQ00B0_PROCNAME - Procedure name is null or empty.
**
**  Preprocessor Generated Code:
**
**	EXEC SQL EXECUTE PROCEDURE p 
**		[ (a = :ivar, b = BYREF(:fvar:ind)) ]
**		[ INTO :status ];
**	
**
**	IIsqInit(&sqlca);
**	IILQpriProcInit(IIPROC_EXEC, "p");
**	[
**	  IILQprvProcValio("a", FALSE, NULL, ISREF, DB_INT_TYPE, 4, &ivar);
**	  IILQprvProcValio("b", TRUE, &ind, ISREF, DB_FLT_TYPE, 8, &fvar);
**	]
**	while (IILQprsProcStatus()) {
**	    if (sqlca.sqlcode < 0)
**		user_error();
**	    else if (sqlca.sqlcode == II_SQLMESSAGE)
**		user_message();
**	}
**	[ if (IInextget() != (int)0) {
**          IIgetdomio(&ind, ISREF, DB_FLT_TYPE, 8, &fvar);
**	  } /* IInextget 
**	  IIsqFlush((char *)0,(int)0); 
**	]
**	[ IIeqiqio(NULL, TRUE, DB_INT_TYPE, 4, &status, "iiret"); ]
**
**	If global temporary table is passed as a parameter (if passed at all,
**	it must be the ONLY parameter passed), IILQprvProcValio calls are
**	replaced by:
**	  IILQprvProcGTTParm("a", "temptabname");
**	(for a = session.temptabname).
**
**  Side Effects:
**	1. Any side effects from starting a query (via IIqry_start).
**	2. Initializes the return-status field in the 'IIlbqcb' struct.
**	3. Explicitly turn off the lock-out flag of procedures.
**	   This must be done from IIbreak too.
**	4. Turn on the local procedure "exec" flag for SQLDA support from
**	   IIsqDaIn.  This flag is turned off before returning to the user
**	   from IILQprsProcStatus.
**	
**  History:
**	02-may-1988	- Written for 6.1. (neil)
**	24-may-1989	- Support for Dynamic SQL execution - initialize
**			  procedure flag. (ncg)
**	08-dec-1989	- Save the dml in case of queries in handler. (bjb)
**	15-dec-1992 (larrym)
**	    Updated comments with BYREF example.  No code changes.
**	21-feb-1993 (larrym)
**	    Added code to prohibit dbprocedure from committing if app is XA.
**	23-aug-1993 (larrym)
**	    Added support for owner.procedure.  If this is GCA_PROTO_60 or
**	    greater then we send down an GCA1_INVPROC instead of GCA_INVPROC.
**	    We also manage the query/proc id's differently for the new
**	    protocol; three new functions IIproc_find, IIproc_add, and
**	    IIproc_send are used for this purpose.
**	08-nov-1999 (kitch01)
**          Bug 99170. In function IILQpriProcInit.
**          Reset the gca message type if the GCA_PROTOCOL_LEVEL is
**          less than 60. This allows an embedded SQL program to execute 
**	    procedures in 1.2 and then switch sessions to 6.4.
**	 1-Oct-09 (gordy)
**	    Simplified the non-execute cases.  Pulled IIsend_proc() into
**	    this routine since there was no advantage in having it in a
**	    separate routine.  Use GCA2_INVPROC at GCA protocol level 68.
*/

VOID
IILQpriProcInit(proc_type, proc_name)
i4	proc_type;
char	*proc_name;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IIDB_QRY_ID		proc_id, *proc_idp;
    IIDB_PROC_ID	*proc_p;
    i4			name_len;
    i4			gcatype;
    char		name_buf[GCA_MAXNAME+1];
    i4			proc_mask;
    DB_DATA_VALUE	dbv;

    /*
    ** This array is dependent on the values defined in eqrun.h for
    ** the "proc_type" argument generated by the preprocessor.
    */
    static struct {
	i4	gcatype;
	char	*qrystr;
    } procs[] = {
		    {GCA_CREPROC, ERx("create procedure")},
		    {GCA_DRPPROC, ERx("drop procedure")},
		    {GCA_INVPROC, ERx("execute procedure")}
		 };

    gcatype = procs[proc_type].gcatype;

    /* If not EXECUTE - start message and return */
    if (proc_type != IIPROC_EXEC)
    {
	IIqry_start( gcatype, 0, 0, procs[proc_type].qrystr );
	return;
    }

    /* Validate procedure name */
    if (   proc_name == (char *)0
	|| *proc_name == '\0'
	|| *proc_name == ' ')
    {
	IIlocerr(GE_INVALID_IDENT, E_LQ00B0_PROCNAME, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return;
    }

    if (IIlbqcb->ii_lq_gca->cgc_version < GCA_PROTOCOL_LEVEL_60 )
    {
	/* Make sure we can start a GCA message */
	if ( IIqry_start( gcatype, 0, 0, procs[proc_type].qrystr ) != OK)
	    return;

        /*
        ** Convert name for search - temporary copy so that we can
        ** lower case it before the search.
        */
        name_len = STlcopy(proc_name, name_buf, GCA_MAXNAME);
        CVlower(name_buf);
        MEmove((u_i2)name_len, name_buf, ' ', GCA_MAXNAME, proc_id.gca_name);

        /* Get internal procedure id - if not found make one */
        proc_idp = IIqid_find( IIlbqcb, FALSE, proc_id.gca_name, 0, 0 );
        if (proc_idp == (IIDB_QRY_ID *)0)
        {
            proc_id.gca_index[0] = 0;
            proc_id.gca_index[1] = 0;
            proc_idp = &proc_id;
        }
        IIqid_send( IIlbqcb, proc_idp );

    } /* old protocol */
    else
    { /* new improved protocol */

        FE_GCI_IDENT    ownproc;

	gcatype = GCA1_INVPROC;

        /* parse out the owner and procedure name */
        IIUGgci_getcid(proc_name, (PTR)&ownproc, TRUE);

        /*
        ** the DBMS expects a blank filled owner name if none exists so we
        ** deal with that here.  It turns out that it's more efficient to send
        ** down 4 blanks.
        */
        if( !STcompare(ownproc.owner, "") )
        {
            STcopy(ERx("    "), ownproc.owner);
        }

        if ( (proc_p = IIproc_find( IIlbqcb, &ownproc )) == NULL )
        {
            /* new procedure */
            proc_p = IIproc_add( IIlbqcb, &ownproc );

	    /*
	    ** Since there isn't a BE proc ID, use the GCA invoke proc
	    ** message which doesn't carry a GCA_ID if allowed by the
	    ** protocol level.
	    */
	    if ( IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_68 )
	    	gcatype = GCA2_INVPROC;
        }

	/* Make sure we can start a GCA message */
	if ( IIqry_start( gcatype, 0, 0, procs[proc_type].qrystr ) != OK )
	    return;

	if ( gcatype == GCA2_INVPROC )
	{
	    /* send the procedure name */
	    II_DB_SCAL_MACRO( dbv, DB_CHA_TYPE,
			      STlength( ownproc.object ), ownproc.object );

	    IIcgc_put_msg( IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv );
	}
	else
	{
	    IIDB_PROC_ID	procID;

	    /*
	    ** A proc ID may not have been added due to storage
	    ** restrictions.  If this is the case, generate a
	    ** temp proc ID.
	    */
	    if ( proc_p == NULL )
	    {
	    	proc_p = &procID;
		procID.gca_id_proc.gca_index[0] = 0;
		procID.gca_id_proc.gca_index[1] = 0;
	        MEmove( STlength( ownproc.object ), ownproc.object, 0,
			sizeof( procID.gca_id_proc.gca_name ),
			procID.gca_id_proc.gca_name );
	    }

	    /* send the GCA_ID part */
	    IIqid_send ( IIlbqcb, &proc_p->gca_id_proc );
	}

	II_DB_SCAL_MACRO( dbv, DB_CHA_TYPE,
			  STlength( ownproc.owner ), ownproc.owner );

	IIcgc_put_msg( IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv );
    }

    /* Add the GCA procedure flags field */
    proc_mask = 0;

    /* If app is XA, don't allow db procedures to COMMIT */
    if (IIXA_CHECK_IF_XA_MACRO)
	proc_mask |= GCA_IP_NO_XACT_STMT;

    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(proc_mask), &proc_mask);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);

    /* Initialize local procedure state */
    IIlbqcb->ii_lq_flags &= ~II_L_DBPROC;	/* Turn off lock out flag */
    IIlbqcb->ii_lq_prflags = II_P_EXEC;		/* Start of exec procedure */
    IIlbqcb->ii_lq_rproc = 0;			/* Default return status */
    IIlbqcb->ii_lq_svdml = IIlbqcb->ii_lq_dml;	/* Save dml */

    /* Initialize GCA procedure state */
    IIlbqcb->ii_lq_gca->cgc_retproc.gca_retstat = 0;
    IIlbqcb->ii_lq_gca->cgc_retproc.gca_id_proc.gca_index[0] = 0;
    IIlbqcb->ii_lq_gca->cgc_retproc.gca_id_proc.gca_index[1] = 0;

    /*
    ** Save session id asociated with this db procedure execution loop.
    ** This helps LIBQ catch errors from inadvertently accessing another
    ** session's data in the loop.  (bug #9159)
    */
    IILQsepSessionPush( thr_cb );

    /* This is followed by an optional list of parameters */

} /* IILQpriProcInit */

/*{
**  Name: IILQprvProcValio - Pass a procedure parameter name and value.
**
**  Description:
**	During procedure invocation zero or more arguments may be passed
**	to the DBMS.  This procedure packages the parameter name and
**	the parameter value for the GCA message.  All the parameter
**	value descriptions and addresses are passed to IIputdomio for
**	processing using ADH.  Note that unlike variables passed to
**	queries, this routine can also accept a typeless NULL constant
**	(in the query case that constant would be sent as a string) -
**	this is taken care of by ADH.
**
**	If an error was encountered during IILQpriProcInit then this
**	routine just returns without validating its arguments.
**	
**	This routine does not confirm that the parameter name actually
**	exists.  If it does not exist it is treated in the same way as
**	an error during procedure invocation (with respect to generated
**	control flow).
**
**	Note that this routine may not even be called if no arguments
**	were passed to the procedure.
**
**	Note that we now support passing parameters by reference (BYREF).
**
**      If we ever want to get the column names back, we should 
**      set (IIlbqcb->ii_lq_flags & II_L_NAMES), and gcatype = GCA_QUERY
**      or just set gcamask |= GCA_NAMES_MASK;
**
**  Inputs:
**	param_name	- Argument name (null terminated, blanks ignored)
**	param_byref	- Does input parameter use the procedure BYREF
**			  clause?  
**	indaddr		- The address of the null indicator variable
**	isref		- TRUE/FALSE: is datavalue an address or 
**			  a value.  This is NOT the same as the
**			  language BYREF flag.
**	type		- The generated data type
**	length		- The length of the variable
**	addr		- The address/value of the variable
**  Outputs:
**	Returns:
**	    VOID
**	Errors:
**	    E_LQ00B1_PARAMNAME - Procedure parameter name is null or empty.
**
**  Preprocessor Generated Code:
**
**	EXEC SQL EXECUTE PROCEDURE p
**		(a = BYREF(:ivar:ind), b = 'abc', c = 123, d = NULL);
**
**	...
**	IILQprvProcValio("a", TRUE, &ind, ISREF, DB_INT_TYPE, 4, &ivar);
**	IILQprvProcValio("b", FALSE, NULL, ISREF, DB_CHR_TYPE, 0, "abc");
**	IILQprvProcValio("c", FALSE, NULL, BYVAL, DB_INT_TYPE, 4, 123);
**	IILQprvProcValio("d", FALSE, NULL, BYVAL, DB_NUL_TYPE, 1, 0);
**	...
**
**  Side Effects:
**	1. Filling GCA message buffers.
**	
**  History:
**	02-may-1988	- Written for 6.1. (ncg)
**	11-jun-1990	(barbara)
**	    Floating point literals occupy two places on the stack.  Need
**	    to pass both these stack elements to IIputdomio.  From IIputdomio
**	    on down, the address of the data is used, so need not "hold on" to
**	    the second entity in a literal float.  This problem only seems to
**	    show up on Sequent, possibly because Sun and VMS use the same
**	    stack frame in IIputdom as in IILQprvProcValio.  (bugs 30623,
**	    30626, 30627)
**	02-jan-1991 (barbara)
**	    As part of fix to bug 35000 (null database parameter crashes
**	    server), IILQprvProcValio now checks for that condition to avoid
**	    sending an incomplete GC_DATA_VALUE.
**	01-dec-1992 (larrym)
**	    changed IILQprvProcValio to pass BYREF parameter information to 
**	    GCA.   It's passed in the param_mask.
*/

/*VARARGS6*/
VOID
IILQprvProcValio(param_name, param_byref, indaddr, isref, type, length, addr1, addr2)
char		*param_name;
i4		param_byref;				/* UNUSED */
i2		*indaddr;
i4		isref;
i4		type;
i4		length;
PTR		addr1;
PTR		addr2;		/* Only used for f8 by value */
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    char		name_buf[RD_MAXNAME +1];	/* Trimmed name */
    DB_DATA_VALUE	dbv;				/* For GCA data */
    i4		param_mask;

    /* If earlier error return */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;

    /* Validate parameter name */
    if (   param_name == (char *)0
	|| *param_name == '\0'
	|| *param_name == ' ')
    {
	IIlocerr(GE_INVALID_IDENT, E_LQ00B1_PARAMNAME, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return;
    }
    IIstrconv(II_TRIM, param_name, name_buf, RD_MAXNAME);

    if (isref && addr1 == (char *)0) 	/* Bug #35000 */
    {
	IIlocerr(GE_SYNTAX_ERROR, E_LQ0010_EMBNULLIN, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return;
    }
    /* Add parameter name as a GCA_NAME */
    II_DB_SCAL_MACRO(dbv, DB_CHA_TYPE, STlength(name_buf), name_buf);;
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv);

    /* Add the GCA parameter flags field */
    param_mask = 0;

    /* Is parameter being passed BYREF? */
    if( param_byref ) param_mask |= GCA_IS_BYREF;

    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(param_mask), &param_mask);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);

    /* Now add the parameter value */
    IIputdomio(indaddr, isref, type, length, addr1);
} /* IILQprvProcValio */

/*{
**  Name: IILQprvProcGTTParm - Pass a global temporary table procedure
**	parameter.
**
**  Description:
**	During procedure invocation, exactly one global temporary table
**	may be passed as an alternative to scalar parameter values. This
**	function packages the parameter name and temptable name for the
**	GCA message. The table name is simply passed as a string constant
**	to allow use of the GCA_P_PARAM structure. This avoids the need 
**	to define new structures and a new protocol level. A flag is set
**	to indicate the passing of the temp table parameter.
**
**	If an error was encountered during IILQpriProcInit then this
**	routine just returns without validating its arguments.
**	
**
**  Inputs:
**	param_name	- Argument name (null terminated, blanks ignored)
**	temptab_name	_ Unqualified temporary table name.
**  Outputs:
**	Returns:
**	    VOID
**	Errors:
**	    E_LQ00B1_PARAMNAME - Procedure parameter name is null or empty.
**
**  Preprocessor Generated Code:
**
**	EXEC SQL EXECUTE PROCEDURE p
**		(a = session.gttname);
**
**	...
**	IILQprvProcGTTParm("a", "gttname");
**	...
**
**  Side Effects:
**	1. Filling GCA message buffers.
**	
**  History:
**	02-may-1997	- Written (inkdo01)
*/

VOID
IILQprvProcGTTParm(param_name, temptab_name)
char		*param_name;
char		*temptab_name;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    char		name_buf[RD_MAXNAME +1];	/* Trimmed name */
    DB_DATA_VALUE	dbv;				/* For GCA data */
    i4		param_mask;

    /* If earlier error return */
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;

    /* Validate parameter name */
    if (   param_name == (char *)0
	|| *param_name == '\0'
	|| *param_name == ' ')
    {
	IIlocerr(GE_INVALID_IDENT, E_LQ00B1_PARAMNAME, II_ERR, 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return;
    }
    IIstrconv(II_TRIM, param_name, name_buf, RD_MAXNAME);

    /* Add parameter name as a GCA_NAME */
    II_DB_SCAL_MACRO(dbv, DB_CHA_TYPE, STlength(name_buf), name_buf);;
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv);

    /* Add the GCA parameter flags field */
    param_mask = 0;

    /* Is parameter being passed BYREF? */
    param_mask |= GCA_IS_GTTPARM;

    II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(param_mask), &param_mask);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);

    /* Now add the parameter value */
    IIputdomio(0, 1, DB_CHR_TYPE, 0 /*STlength(temptab_name)*/,
	 temptab_name);
} /* IILQprvProcGTTParm */

/*{
**  Name: IILQprsProcStatus - Control the dynamic state of a procedure.
**
**  Description:
**	If the procedure invocation initialization was successful, then
**	this routine sends the GCA message completion to the DBMS.  We turn
**	around and start reading results from the DBMS.  During procedure
**	execution, user messages and errors may return to the caller.
**	User messages are the result of a MESSAGE statement, while 
**	errors can happen as the result of a statement in the body of the
**	procedure.  Note that errors that occur as the result of bad procedure
**	invocation (ie, unknown procedure name or illegal parameter value)
**	are treated like errors within the body of the procedure.
**
**	Each "event" (error or message) is returned to the caller, as this
**	routine is called in a loop, using PROC_CONTINUE.  The caller may
**	process the event using the language error handling mechanism.
**	At the end of the procedure (when a response message or procedure
**	"return" message is received) this routine returns the PROC_ENDLOOP
**	event which terminates the generated loop.
**
**	If a procedure "return" message is received then the internal
**	procedure id is saved away for subsequent execution (see 
**	IILQpriProcInit) and the return status is saved for later
**	access by IIeqiqio (see generated code).
**
**	While inside the procedure loop a lock flag will be set so that
**	no other queries may be issued while the procedure is being
**	processed (II_L_DBPROC).  This is similar to the data retrieval
**	flag set for locking nested queries out.  Since a procedure can
**	be broken out of by a call to IIbreak we must make sure that we
**	do not leave any internal state on after IIbreak.
**
**	If, during a user error handling routine, an attempt is made to
**	execute a database statement, then this is caught in IIqry_start
**	and a local procedure nesting error is set (II_P_NESTERR).  This
**	is done so that on return to this routine for procedure control,
**	we can detect that a nested query was attempted, and immediately
**	terminate the control.
**
**	There are two different classes of errors, for which the generated
**	code must work:
**
**	1. Initialization errors, where, for some reason, LIBQ prevented the
**	   caller from continuing (ie, a null procedure name) will prevent
**	   any procedure routine from continuing with the procedure invocation.
**	   Note that they all check the error status.  This routine checks
**	   the error code on its first entry.  If set, it returns PROC_CONTINUE
**	   to allow the caller to process that error.  The subsequent call
**	   to this routine will reset the status and will return PROC_ENDLOOP
**	   to terminate the loop.
**
**	   If, during initialization, some data was sent to the DBMS (or
**	   put into a GCA message buffer), then we still must make sure that
**	   the data is flushed and sent the GCA message.
**
**	   GCA errors during initialization are handled like initialization
**	   errors - allow the caller to read the error and then terminate.
**
**	2. Execution error are errors that occur within the executing
**	   procedure code.  There can be many of these during procedure
**	   execution.  When seen, this routine returns a continuation
**	   event to the caller to allow for error processing (this happens
**	   for user messages too).  When the final response message ("return"
**	   message) is read from the DBMS, the termination event is
**	   returned to the caller.
**
**	   Invocation errors indicate that the procedure was not executed;
**	   perhaps the user lacked permission or the procedure didn't exist.
**	   These errors are handled just like an execution error within the
**	   body of the procedure.  One difference is that if the body of
**	   the procedure was not reached the value of the return status
**	   is undefined and will be set to zero.
**
**  Pseudo Code for State Control:
**
**	    -- If a LIBQ initialization error occurred (QERR)
**	    -- or procedure nesting error.
**	    if (init error occurred)
**		if (not procedure_loop)		-- First time round
**		    if (in_query)		-- Flush, if got into GCA
**			end GCA message to DBMS;
**		    endif
**		    procedure_loop = true;
**		    return continue;		-- Allow caller to read error
**		else				-- Next time through
**		    end LIBQ control;
**		    set return status to default;
**		    procedure_loop = false;
**		    return endloop;		-- Terminate generated flow
**		endif
**	    endif 
**
**	    if (not in_query)			-- e.g., IIbreak in handler?
**		return endloop;
**
**	    -- If user handler switched sessions, give error and
**	    -- break loop on next iteration.
**	    if (current session != session id saved from loop start)
**		Pop session id
**		procedure_loop = true;
**		return continue;
**
**	    -- No initialization error occurred, we must be in query,
**	    -- so start/continue executing the procedure.
**	    -- Read for GCA errors and user messages.
**
**	    if (not procedure_loop) 		-- First time round
**		end GCA message to DBMS;
**		procedure_loop = true;
**	    endif
**
**	    -- Read errors from GCA (user messages are like errors here).
**	    -- Make sure that while "in procedure_loop" new errors may
**	    -- override old SQL codes.
**
**	    status = read till "procedure return" from GCA;
**	    if (status = error message from GCA)
**		-- We read a GCA_ERROR, return to caller for processing
**		return continue;
**	    else 			
**		set procedure id and return status;
**              if (old protocol level) 
**		    -- We ended the query 
**		    procedure_loop = false;
**		    end LIBQ control;
**		    return endloop;
**		else
**		    read either GCA_TDESCR or GCA_RESPONSE
**		    if (GCA_TDESCR)
**		       do set up for receiving TUPLES.
**		    end LIBQ control;
**		    return endloop;
**	    endif
**
**  Inputs:
**	resultCount	- number of entries in RESULT ROW clause, or 0 if 
**			  not a row producing procedure
**  Outputs:
**	Returns:
**	    PROC_CONTINUE/1	- Continue to process status
**	    PROC_ENDLOOP/0	- End status processing
**	Errors:
**	    None
**
**  Preprocessor Generated Code:
**
**	See IILQpriProcInit for generated code.
**
**  Side Effects:
**	1. Set/reset procedure flag for global lock-out and to provide some
**	   information to subsequent calls to this routine.
** 	2. Upon entry always turn off local "exec" flag.  This is only used
**	   for Dynamic SQL and we don't want this flag on while outside of
**	   a procedure.  Note that the procedure loop may be exited through
**	   IIbreak which leaves the flag alone - this "dirty" flag could cause
**	   later Dynamic SQL EXECUTE statements to come through here.
**	3. Before reading results, turn on LIBQGCA's "get event" flag.  Any
**	   events returning with db procedure results/messages will then cause
**	   a return to this routine (rather than being consumed and saved
**	   silently).  By returning control to this routine, events can be
**	   caught by user WHENEVER handling in the ProcStat program loop.
**	
**  History:
**	03-may-1988	- Written (ncg)
**	24-may-1989	- Turn off temp "exec" flag for Dynamic SQL. (ncg)
**	15-dec-1989	- Turn on "get event" flag until done reading. (bjb)
**	08-dec-1989	- Restore dml (nested qry may have changed it). (bjb)
**	26-mar-1990	- Check that loop successive loop iterations are
**			  within the same session.  Pop session stack at
**			  successful completion of EXEC PROC statement. (bjb)
**	03-apr-1990	- Turn on XACT flag after successful execution of
**			  dbproc.  Until the dbms updates protocol to return
**			  a RESPONSE block on procedure execution, we don't
**			  know transaction state.  Best to assume that we are
**			  in a transaction. Bug 9190. (bjb)
**	01-dec-1992 (larrym)
**		added functionallity for BYREF procedures, included GCA
**		protocol changes.   Since we now always get back a GCA_RESPONSE
**		we won't automatically set the XACT flag.
**	21-jun-1993 (larrym)
**	    Fixed bug 50990.  In some error cases there was just a return; and
**	    we need return PROC_ENDLOOP instead.
**	23-aug-1993 (larrym)
**	    Added support for owner.procedure.  If this is GCA_PROTO_60 or
**	    greater we manage the query/proc id's differently 
**	    using the new functions IIproc_find, IIproc_add, and
**	    IIproc_send.
**	 4-oct-96 (nick)
**	    We may get back a proc id for something other than the proc
**	    that we invoked hence we need to test for this.  #78814
**	25-apr-00 (inkdo01)
**	    Changes to support more varied message sequences of row
**	    producing procedures.
**       9-may-2000 (wanfr01)
**          Bug 101250, INGCBT272.  Added II_L_ENDPROC to indicate that
**          we are relooping due to a message received during ENDPROC
**          processing of a database procedure.
**	 16-may-2000 (wanfr01)
**	    fixed typo in prior fix.
**	2-may-01 (inkdo01)
**	    Add result count parm to ProcStatus for detection of mis-matched
**	    result entries.
**	 1-Oct-09 (gordy)
**	    There are situations in which there is no current proc ID, so
**	    conditionally save the ID.
*/

i4
IILQprsProcStatus( resultCount )
i4	resultCount;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    GCA_RP_DATA	*gca_retproc;
    II_RET_DESC *ret;		/* for BYREF parameters */
    char	*qrystr = "EXECUTE PROCEDURE";


    /* Always turn off local "exec" flag - see side effects */
    IIlbqcb->ii_lq_prflags &= ~II_P_EXEC;

    /* Restore dml saved by IILQpriProcInit  */
    IIlbqcb->ii_lq_dml = IIlbqcb->ii_lq_svdml;

    /*
    ** If earlier initialization error or nesting error
    ** then handle separately. Note: row producing proc can
    ** legitimately lead to re-entry of ProcStatus with NESTERR
    ** turned on.
    */
    if (   (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	|| (IIlbqcb->ii_lq_prflags & II_P_NESTERR &&
	   ~(IIlbqcb->ii_lq_prflags & II_P_RESROWS)))
    {
	if ((IIlbqcb->ii_lq_flags & II_L_DBPROC) == 0)	/* First time */
	{
	    /* Need to flush GCA? */
	    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
	    {
		IIcgc_end_msg(IIlbqcb->ii_lq_gca);
		IIlbqcb->ii_lq_prflags |= II_P_SENT;
	    }

	    IIlbqcb->ii_lq_flags |= II_L_DBPROC;	/* Into loop */

	    /* Allow caller to read error */
	    return PROC_CONTINUE;			
	}
	else						/* Next time through */
	{
	    /*
	    ** End off LIBQ control.  In the unlikely event that there was
	    ** an erroneous nested query AND we had sent something to GCA,
	    ** the INQRY flag will have been turned off by the ending of
	    ** the bad nested query, so we turn it back on here.
	    */
	    if (IIlbqcb->ii_lq_prflags & (II_P_NESTERR|II_P_SENT))
		IIlbqcb->ii_lq_curqry |= II_Q_INQRY;
	    IIqry_end();

	    IIlbqcb->ii_lq_flags &= ~II_L_DBPROC;	/* Out of loop */
	    IIlbqcb->ii_lq_prflags = II_P_DEFAULT;

	    /* Terminated generated flow - return status is left zero */
	    return PROC_ENDLOOP;			
	} /* If first time or not */
    } /* If initialization error */

    /* User handler might have called IIbreak() -- if so exit loop */
    if ((IIlbqcb->ii_lq_curqry & II_Q_INQRY) == 0 &&
	((IIlbqcb->ii_lq_prflags & II_P_RESROWS) == 0))
	return PROC_ENDLOOP;

    /*
    ** If session saved by IILQpriProcInit doesn't match id of current
    ** session, arrange to drop out of loop next time.
    */
    if (IILQscSessionCompare( thr_cb ) == FALSE)
    {
	IILQspSessionPop( thr_cb );		/* Pop loop id */
	IIlbqcb->ii_lq_flags |= II_L_DBPROC;	/* So we exit loop next time */
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return PROC_CONTINUE;			/* Allow caller to read error */
    }

    /*
    ** No initialization error occurred, so we must be in query.
    ** If first time through end the GCA message, and prepare to
    ** read results.
    ** If subsequent time through then just carry on reading until
    ** we read the return or response message.
    */

    if ((IIlbqcb->ii_lq_flags & II_L_DBPROC) == 0)	/* First time */
    {
	IIcgc_end_msg(IIlbqcb->ii_lq_gca);		/* Flush to GCA */
	IIlbqcb->ii_lq_prflags |= II_P_SENT;
	IIlbqcb->ii_lq_flags |= II_L_DBPROC;		/* Into loop */
    }

    /*
    ** If this is a row producing procedure, the first message may be
    ** a GCA_TDESCR. If so, IIproc_retinit is called to simulate the
    ** IIretinit processing of a regular select loop. This loads the
    ** fetch buffer formats and prepares for the return of data in 
    ** subsequent GCA_TUPLE messages.
    **
    ** Also if this is a row producing procedure and this is the 2nd 
    ** call to ProcStatus (as indicated by II_P_RESROWS), the current 
    ** message should be a GCA_RETPROC or a GCA_ERROR. In either case, 
    ** IIcgc_read_results needn't be called and control should pass to
    ** the appropriate logic.
    **
    ** Otherwise, read till the procedure "return" message.  This will
    ** keep on returning errors from GCA (user messages are like errors
    ** for this purpose).  Each error that is returned will be returned to
    ** the calling application for processing.
    **
    ** For event processing, return to user each time an event returns
    ** along with procedure results.  (IICGC_EVENTGET_MASK)
    **
    ** IIcgc_read_results returns zero when done.  If executed successfully
    ** then GCA_RETPROC will be returned with a return status, and
    ** procedure id.  If not executed successfully (ie, invocation failed
    ** because of parameter data type clash) then an error followed by
    ** GCA_RESPONSE will be returned.  For both of these cases the IIcgc
    ** routine will return zero.
    */

    IIlbqcb->ii_lq_gca->cgc_m_state |= IICGC_EVENTGET_MASK;

    if (!(IIlbqcb->ii_lq_flags & II_L_ENDPROC))
    {
      if ((!(IIlbqcb->ii_lq_prflags & II_P_RESROWS) ||
         IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_RETPROC)
        && (IIcgc_read_results(IIlbqcb->ii_lq_gca, FALSE, GCA_RETPROC) != 0)
        && (IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_RETPROC))
       if (IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_TDESCR
        && IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_RETPROC)
      {
	  /*
	  ** Hopefully a GCA_ERROR message - error (or message) information
	  ** for caller is already set.
	  */
          IIlbqcb->ii_lq_gca->cgc_m_state &= ~IICGC_EVENTGET_MASK;
	  return PROC_CONTINUE;
      }
      else if (IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type == GCA_TDESCR)
      {
	  /* This is a TDESCR message. Set flag to indicate row 
	  ** producing procedure and call IIproc_retinit to simulate
	  ** select loop IIretinit processing. */
	  IIlbqcb->ii_lq_prflags |= II_P_RESROWS;
	  IIproc_retinit( IIlbqcb, resultCount );
	  return PROC_CONTINUE;
      }

      /* If we're here, we just got the GCA_RETPROC.  */
  
      /* Invocation did complete - save status */
      gca_retproc = &IIlbqcb->ii_lq_gca->cgc_retproc;
      IIlbqcb->ii_lq_rproc = gca_retproc->gca_retstat;
    }

    if (IIlbqcb->ii_lq_gca->cgc_version < GCA_PROTOCOL_LEVEL_60 )
    {

        if (   (gca_retproc->gca_id_proc.gca_index[0] != 0)
            || (gca_retproc->gca_id_proc.gca_index[1] != 0))
        {
	    /* save procedure id */
            IIqid_add( IIlbqcb, FALSE, 0, 0, &gca_retproc->gca_id_proc );
        }

        IIqry_end();

	IIlbqcb->ii_lq_flags &= ~II_L_DBPROC;	/* Out of loop */
	IIlbqcb->ii_lq_prflags = II_P_DEFAULT;
	IIlbqcb->ii_lq_gca->cgc_m_state &= ~IICGC_EVENTGET_MASK;

	/*
	** Bug 9190.  Turn on transaction flag.  Because EXECUTE PROC
	** doesn't return a GCA_RESPONSE, LIBQ doesn't know if transaction
	** has been started.  Assume it has, to be on the safe side.
        */

        IIlbqcb->ii_lq_flags |= II_L_XACT;
        IILQspSessionPop( thr_cb );	/* Pop session id */
        /* Terminated generated flow */
        return PROC_ENDLOOP;			
    }
    /* ELSE expect either GCA_RESPONSE or GCA_TDESCR */

    if (!(IIlbqcb->ii_lq_flags & II_L_ENDPROC)) 
    {
# ifdef II_DEBUG
    /*
    ** actually, don't do this unless the proc name matches our current
    ** proc held in ii_lq_curproc as there are circumstances where we
    ** can get back a different id than the one we executed
    */

      if (IIdebug)
      {
    	if ( IIlbqcb->ii_lq_curproc != NULL  &&
	     STbcompare(IIlbqcb->ii_lq_curproc->gca_id_proc.gca_name, 
		GCA_MAXNAME, gca_retproc->gca_id_proc.gca_name, 
		GCA_MAXNAME, TRUE) == 0)
	{
	    IIlbqcb->ii_lq_curproc->gca_id_proc.gca_index[0] =
		gca_retproc->gca_id_proc.gca_index[0];
	    IIlbqcb->ii_lq_curproc->gca_id_proc.gca_index[1] =
		gca_retproc->gca_id_proc.gca_index[1];
	}
	else
	{
	    SIprintf(ERx("IILQprsProcStatus: Mismatched proc names\n"));
	}
      }
      else
      {
# endif /* II_DEBUG */

	/* 
	** save off index (even if it's 0's; IIproc_add will deal with that )
	** (there might not be a current proc ID)
	*/ 
    	if ( IIlbqcb->ii_lq_curproc != NULL )
	{
	    IIlbqcb->ii_lq_curproc->gca_id_proc.gca_index[0] =
		gca_retproc->gca_id_proc.gca_index[0];
	    IIlbqcb->ii_lq_curproc->gca_id_proc.gca_index[1] =
		gca_retproc->gca_id_proc.gca_index[1];
	}

# ifdef II_DEBUG
      }
# endif /* II_DEBUG */
    }

    if (IIcgc_read_results(IIlbqcb->ii_lq_gca, FALSE, GCA_TDESCR) == 0)
    {
	/* got a GCA_RESPONSE (or error) */
	IIqry_end();
        IIlbqcb->ii_lq_flags &= ~II_L_ENDPROC; 
	IIlbqcb->ii_lq_flags &= ~II_L_DBPROC;	/* Out of loop */
	IIlbqcb->ii_lq_prflags = II_P_DEFAULT;
	IIlbqcb->ii_lq_gca->cgc_m_state &= ~IICGC_EVENTGET_MASK;
	IILQspSessionPop( thr_cb );
	return PROC_ENDLOOP;
    }

    /*
    **  If not actually a Tuple Descriptor, then it is probably a GCA_EVENT.
    */
    if (IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_TDESCR) 
    {
        IIlbqcb->ii_lq_gca->cgc_m_state &= ~IICGC_EVENTGET_MASK;
        IIlbqcb->ii_lq_flags |= II_L_ENDPROC;
        return PROC_CONTINUE;
    }

    IIlbqcb->ii_lq_flags &= ~II_L_ENDPROC;

    /* 
    ** Byref params are being returned.  Expecting GCA_TDESCR and GCA_TUPLES 
    ** from dbms.  The following functionality is similar to the latter part 
    ** of IIretinit. 
    */

    /* 
    ** If we want to let BYREF params be returned after a RAISE ERROR, 
    ** this is probably the point at which we should reset the error 
    ** number(s).  I don't think you can get to this point unless the
    ** procedure actually executed successfully (I mean, it existed and
    ** the user had permission to run it, not, did all statements complete).
    ** need to reset IIlbqcb->ii_lq_error structure, and conditionally,
    ** sqlca.sqlcode, SQLCODE, and/or SQLSTATE depending on what the user
    ** is using. 
    */

    /*
    ** Since this started out as a dbproc, the NESTERR flag was set by
    ** IIlbqcb->ii_lq_prflags |= II_P_NESTERR.  Now that we're emulating
    ** a select statement, we need to: 
    */
    IIlbqcb->ii_lq_prflags &= ~ II_P_NESTERR;
    ret = &IIlbqcb->ii_lq_retdesc;
    IIlbqcb->ii_lq_retdesc.ii_rd_flags |= II_R_NESTERR; 
    /* 
    ** Singleton selects (which we're now emulating) don't push the sesion id,
    ** so we'll pop it off and pretend we didn't either.
    */
    IILQspSessionPop( thr_cb );		

    /* 
    ** We'll use this flag to tell IIrdDescribe which error message to use
    */
    IIlbqcb->ii_lq_flags |= II_L_DBPROC; 	/* we're in a proc statment */
    IIlbqcb->ii_lq_flags &= ~II_L_RETRIEVE;	/* not a retrieve statement */

    /* No more whenever handling for this statement; turn off EVENT_MASK */
    IIlbqcb->ii_lq_gca->cgc_m_state &= ~IICGC_EVENTGET_MASK;

    /*
    **  if this were a real SELECT, this is the point where we'd do an 
    **  IIqry_read(), but we've already "read" the tuple descriptor above. 
    */

    /* Save response time */
    IIdbg_response( IIlbqcb, FALSE );

    /*
    ** If an error occurred during the setup, cleanup
    ** any returning data, as the preprocessor will exit the dbproc loop
    */
    if ((IIlbqcb->ii_lq_gca->cgc_result_msg.cgc_message_type != GCA_TDESCR)
        || (II_DBERR_MACRO(IIlbqcb)))
    {
        /* Assign error number to make sure preprocessor skips loop */
        if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
        {
/* NOTE:  need new error message here. For now, qrystr = "EXECUTE PROCEDURE */
    	IIlocerr(GE_LOGICAL_ERROR, E_LQ003A_RETINIT, II_ERR, 1, qrystr);
    	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
        }
        IIqry_end();
        return PROC_ENDLOOP;                 /* End of query */
    }

    if (IIrdDescribe(GCA_TDESCR, &ret->ii_rd_desc) != OK)
    {
        if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
        {
            IIlocerr(GE_LOGICAL_ERROR, E_LQ00B2_PROCDESC, II_ERR, 1, qrystr);
            IIlbqcb->ii_lq_curqry |= II_Q_QERR;
        }
        IIqry_end();
        return PROC_ENDLOOP;
    }

    /* Setup global information */
    IIlbqcb->ii_lq_rowcount = 0;
    IIlbqcb->ii_lq_retdesc.ii_rd_colnum = II_R_NOCOLS;
    IIlbqcb->ii_lq_retdesc.ii_rd_flags = II_R_DEFAULT;

    /* Terminated generated flow */
    return PROC_ENDLOOP;

} /* IILQprsProcStatus */

/* static routines */

/*{
+*  Name: IIproc_find - Find a DB-procedure id.
**
**  Description:
**      Given an FE_GCI_IDENT (which is basically an object with an owner
**      specification), find the matching DB-query id in the "defined" 
**      procedure id list.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	ownproc		- Sturcture containing procedure name and owner
**
**  Outputs:
**      Returns:
**          IIDB_PROC_ID * - Pointer to first DB-procedure id which matched the
**                          FE-id.  If not found, return NULL.
**
**      Errors:
**          None
-*
**  Side Effects:
**      None
**
**  History:
**	23-Aug-1993 (larrym)
**	    written.
*/

static IIDB_PROC_ID *
IIproc_find( II_LBQ_CB *IIlbqcb, FE_GCI_IDENT *ownproc )
{
    IIDB_PROC_ID *proc_p;

# define        IIPROC_COMPARE(op,pp) (\
(STcompare(op->owner, pp->gca_proc_own)) ||\
(STcompare(op->object, pp->gca_id_proc.gca_name)))


    /* loop until found or NULL */
    for (proc_p = IIlbqcb->ii_lq_procids.ii_pi_defined;
         proc_p != NULL;
         proc_p = proc_p->ii_pi_next)
    {
        if ( IIPROC_COMPARE(ownproc, proc_p) == 0 )
        {
            break; /* found it! */
        }
    }

    IIlbqcb->ii_lq_curproc = proc_p;
    return( proc_p );
}

/*{
+*  Name: IIproc_add - Add a db procedure id to the "defined" list.
**
**  Description:
**      This routine adds a new FE & DB-procedure id to the defined list of
**      procedure ids.  This list can later be searched (IIproc_find) for the
**      new id.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	ownproc		- Sturcture containing procedure name and owner
**
**  Outputs:
**      Returns:
**          IIDB_PROC_ID * - New DB-procedure id or NULL if can't add.
**          
**      Errors:
**          None
-*
**  Side Effects:
**      Optionally allocates a node pool, and add a node to the "defined"
**      list.
**
**  History:
**	23-aug-1993 (larrym)
**	    written.
**	 1-Oct-09 (gordy)
**	    Returns NULL if name/owner exceeds GCA_ID storage size.
*/
static IIDB_PROC_ID *
IIproc_add( II_LBQ_CB *IIlbqcb, FE_GCI_IDENT *ownproc )
{
# define II_PROC_ALLOC   20

    IIDB_PROC_ID *proc_p;
    II_PROC_IDS  *proc_list_p;
    int          i;

    /*
    ** Don't add if procedure name or owner exceed storage
    ** space.  The requests will still go the the server,
    ** we just won't use a proc ID for subsequent requests.
    */
    if ( STlength( ownproc->owner ) >= sizeof(proc_p->gca_proc_own)  ||
	 STlength( ownproc->object ) >= sizeof(proc_p->gca_id_proc.gca_name) )
    {
	/* clear current proc ID */
	IIlbqcb->ii_lq_curproc = NULL;
	return( NULL );
    }

    /*
    ** If for whatever reason the last executed procedure didn't return
    ** a procedure ID, then the first element on the ID list will have zeroes
    ** for the procedure ID.  We'll re-use that list element for this next
    ** procedure.
    */
    proc_list_p = &IIlbqcb->ii_lq_procids;

    if (proc_list_p->ii_pi_pool == NULL)
    {
	MUp_semaphore( &IIglbcb->ii_gl_sem );
        if ( ! proc_list_p->ii_pi_tag )  proc_list_p->ii_pi_tag = FEgettag();
        proc_p = (IIDB_PROC_ID *)FEreqmem( proc_list_p->ii_pi_tag,
					   (u_i4)(II_PROC_ALLOC * 
						   sizeof( *proc_p )),
					   TRUE, NULL );
	MUv_semaphore( &IIglbcb->ii_gl_sem );

	/* couldn't snag more memory */
        if ( ! proc_p )  return( NULL );

        proc_list_p->ii_pi_pool = proc_p;
        /* format the pool into a link list */
        for (i = 1; i < II_PROC_ALLOC; i++, proc_p++)
            proc_p->ii_pi_next = proc_p + 1;
        proc_p->ii_pi_next = NULL;      /* end of list */
    }

    proc_p = proc_list_p->ii_pi_defined; /* get first element */
    if ((proc_p != NULL)
       && (proc_p->gca_id_proc.gca_index[0] == 0)
       && (proc_p->gca_id_proc.gca_index[0] == 0) )
    {
       /* Do nothing.  We'll use this proc_p for the next procedure */
       ;
    }
    else
    {
        /* get one off the pool */
        proc_p = proc_list_p->ii_pi_pool;
        proc_list_p->ii_pi_pool = proc_p->ii_pi_next;

        /* now move it onto defined list */
        proc_p->ii_pi_next = proc_list_p->ii_pi_defined;
        proc_list_p->ii_pi_defined = proc_p;
    }

    /* now fill in the goodies */
    STcopy(ownproc->object, proc_p->gca_id_proc.gca_name);
    STcopy(ownproc->owner, proc_p->gca_proc_own );
    proc_p->gca_id_proc.gca_index[0] = 0;
    proc_p->gca_id_proc.gca_index[1] = 0;

    /* save pointer to current proc */
    IIlbqcb->ii_lq_curproc = proc_p;

    /* all done */
    return (proc_p);

}


/*{
+*  Name: IIproc_retinit - Simulate IIretinit for processing of row
**	producing procedures.
**
**  Description:
**	This routine is called after EXECUTE PROCEDURE has been written to the
**	DBMS and GCA_TDESCR message has been returned. The TDESCR indicates 
**	that GCA_TUPLE messages will follow, containing the result rows of the
**	procedure call. This sets up the structures to process the results
**	in order to assign the DB values into host language variables.
**
**  Inputs:
**	IIlbqcb - II_LBQ_CB * - ptr to session control block.
**	resultCount - number of entries in RESULT ROW list (or 0).
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    Implicitly processes errors from the DBMS.
-*
**  Side Effects:
** 	1. If an error occurs during this routine then the retrieve flags are
**	   set to INITERR so that IInextget() will fail and no data will
**	   attempt to be retrieved.  This was added to allow ESQL singleton 
**	   SELECT queries not to include the call to IIerrtest (which bypass 
**	   the IIflush call).
**	2. The DML (query language) in case LIBQ's record of this is changed
**	   by another query nested inside a EXEC PROCEDURE loop.  Nesting may
**	   occur illegally (a query) or legally (the GET EVENT statement).
**	   IInextget() always restores ii_lq_dml from the saved value.  In
**	   this way, the parent EXEC PROCEDURE will do the correct error
**	   handling, i.e. assign values to the SQLCA if ii_lq_dml is DB_SQL.
**	
**  History:
**	25-apr-00 (inkdo01)
**	    Written (cloned from IIretinit) for row producing procedures.
**	2-may-01 (inkdo01)
**	    Added code to verify number of result clause entries against
**	    TDESCR
*/

void
IIproc_retinit(IIlbqcb, resultCount)
II_LBQ_CB	*IIlbqcb;
i4		resultCount;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_RET_DESC	*ret;
    char	*qrystr = "EXECUTE PROCEDURE";


    ret = &IIlbqcb->ii_lq_retdesc;

    /* Message buffer is already primed with TDESCR. Where IIretinit
    ** has to call IIqry_read to locate it, we've already got it. */
    if (IIrdDescribe(GCA_TDESCR, &ret->ii_rd_desc) != OK)
    {
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ003B_RETDESC, II_ERR, 1, qrystr);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	IIqry_end();
	return;
    }

    if (IIlbqcb->ii_lq_retdesc.ii_rd_desc.rd_numcols != resultCount)
    {
	/* Result list entry count doesn't match TDESCR. */
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    char	ebuf1[20], ebuf2[20];
	    CVna(IIlbqcb->ii_lq_retdesc.ii_rd_desc.rd_numcols, ebuf1); /* i4  */
	    CVna(resultCount, ebuf2); /* i4  */
	    IIlocerr(GE_CARDINALITY, E_LQ003C_RETCOLS, II_ERR, 2, ebuf1, ebuf2);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	IIqry_end();
	return;
    }

    /* Setup global information */
    IIlbqcb->ii_lq_flags |= II_L_RETRIEVE;
    IIlbqcb->ii_lq_rowcount = 0;
    IIlbqcb->ii_lq_retdesc.ii_rd_colnum = II_R_NOCOLS;
    IIlbqcb->ii_lq_retdesc.ii_rd_flags = II_R_DEFAULT;

    /*
    ** Save session id associated with this procedure loop.  This helps LIBQ
    ** catch errors from inadvertently fetching another session's data
    ** in loop.   (bug #9159)
    */
    if (!(IIlbqcb->ii_lq_flags & II_L_SINGLE))
    	IILQsepSessionPush( thr_cb );
}

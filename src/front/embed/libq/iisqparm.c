/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include   	<id.h>
# include	<me.h>
# include   	<pc.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<erlq.h>
# include	<eqrun.h>
# include	<gca.h>
# include	<iicgca.h>

/* bug fix #62708 from INGRES 6.4 by angusm (harpa06) */
# include       <iisqlca.h>

# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iisqparm.c - Manages the WITH clause of the SQL CONNECT statement.
**
**  Defines:
**	IIsqParms   - Build gateway-specific argument passing values.
**
**  Notes:
-*
**  History:
**	09-sep-1987	- Written for INGRES 6.0 DG gateway. (ncg)
**	12-mar-1989	- Added reassoc to complete 2-phase commit.(bjb)
**	18-jun-1990 (barbara)
**	    Process logicals (IIsendlog) even if association was not successful.
**	    Part of bug fix 21832.
**	06-jul-1990 (barbara)
**	   IIsendlog needs an accurate setting of II_G_CONNECT flag.
**	27-sep-1990 (barbara)
**	    Revert to calling IIsendlog only if connected.  II_EMBED_SET
**	    logical has been handled by IIdbconnect.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**      18-Feb-1994 (daveb) 33593, 35805, 35873, 36034, 38382,
**  	    42323, 45002, 45436, 47179, 51315, 51673, 54404
**          Send client information on END, starting build-up
**  	    of params if none have been sent so far.
**	19-nov-1994 (harpa06)
**          Integrated bug fix #62708 by angusm from INGRES 6.4 code (harpa06)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	30-Apr-99 (gordy)
**	    Copy any error code to the new current session when
**	    a connection request fails.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jan-2005 (wansh01)
**          INGNET 161 bug # 113737
**	    IIsend_untrusted_info() supress [uid,pwd] info in connection 
**	    string for vnodeless connection.  
**	21-Jan-2005 (wansh01)
**	    no longer set uid and pwd for vnodeless connection.	
**	    only mask the pwd in login for vnodeless connection	
**           	
**/

# define	IIsqpMAX_PARM_SIZE	1000	/* Max size for parameter */

static VOID IIsend_untrusted_info( II_LBQ_CB *, i4  );

/*{
+*  Name: IIsqParms - Build the SQL CONNECT params for gateways & 2PC.
**
**  Description:
**	This routine accepts varying types of parameters that are sent to a
**	gateway as a string of command line arguments in the form of:
**
**		name = value {, name = value}
**	where 
**		value is an integer literal, quoted string literal or unquoted
**		name.
**
**	The command line string is sent inside a GCA_MD_ASSOC message using
**	the session parameter GCA_GW_PARM.  In the usual static case, each
**	component of the clause (a = b) arrives here as a separate session parm.
**	This routine always sends one session parm to GCA after constructing
**	the complete message.  This session parm is added to the other
**	CONNECT-time session parms (ie, database, query language) as one
**	long GCA_MD_ASSOC message.
**
**	Before this routine is called, IIsqMods is called and the global
**	WITHLIST flag is set.  This flag indicates to IIdbconnect NOT to
**	complete the session startup, but leave it for this routine.
**	However, if IIdbconnect finds a -X flag (child process start
**	up), then the WITHLIST flag is reset, as no WITH clause is sent
**	for child processes.  If IIdbconnect failed with an error then the
**	WITHLIST flag is turned off, as this message will not overcome the
**	startup failure.
**
**	After the WITH clause is sent, the rest of the connection startup
**	is completed, the WITHLIST flag is turned off, the DML is reset to the
**	default (as though we had exited via IIsqConnect) and control is
**	returned to the caller.
**
**	The parameters can also be the high and low transaction ids needed
**	to reassociate to complete 2-phase commit of a distributed transaction.
**	In this case, instead of sending the whole command line string, we
**	send the two integers in the char format "number:number" using the
**	session parameter GCA_RESTART.
**
**  Inputs:
**	flag		- IIsqpINI - Initialize the parameter message, 
**			  IIsqpADD - Added parameter to message,
**			  IIsqpEND - Send the message and end the request,
**				     and ignore rest of arguments.
**	name_spec	- Null terminated string that identifies the left side
**			  of the clause.  If 'val_type' is DB_CHR_TYPE and
**			  'str_val' is null then this implies that the whole
**			  with_clause was specified in a variable, and is in
**			  the 'name_spec' argument.  If 'name_spec' is empty
**			  and 'flag' is IIsqINI then an empty GCA_GW_PARM
**			  session parm is added - this allows programs to
**			  be hard coded with a with_clause that may sometimes
**			  have nothing.  Note that something does have to be
**			  sent as the code generated indicates to previous
**			  routines to expect ONE MORE session parameter (the
**			  call to IIsqMods).
**	val_type	- DB_CHR_TYPE - value is 'str_val'
**			  DB_INT_TYPE - value is 'int_val'.
**	str_val		- Null terminated string - the right side of clause if
**			  it is expressed as a string literal (ie, not a numeric
**			  literal or variable).  This argument may be null if
**			  'name_spec' has specified the whole with_clause.
**			  If 'str_val' is not null this interface always quotes
**		          the value when adding to the message, so that:
**				a = abc
**			  will later appear as:
**				a = 'abc'
**	int_val		- Numeric value of argument if type is DB_INT_TYPE. 
**
**  Outputs:
**	Returns:
**	    None
**	Errors:
**	    None
**
**  Code Generation Examples of CONNECT WITH Clause:
**
**  1. EXEC SQL CONNECT ...
**	     WITH a = a_name, b = 'b string', c = :strvar, d = 123, e = :intvar;
**	
**	strvar = "xx";
**	intvar = 55;
**
**	IIsqMods(IImodWITH);
**	IIsqConnect(EQ_C, ....);
**	IIsqParms(IIsqpINI, "a", DB_CHR_TYPE, "a_name",   0);
**	IIsqParms(IIsqpADD, "b", DB_CHR_TYPE, "b string", 0);
**	IIsqParms(IIsqpADD, "c", DB_CHR_TYPE, strvar,     0);
**	IIsqParms(IIsqpADD, "d", DB_INT_TYPE, (char *)0,  123);
**	IIsqParms(IIsqpADD, "e", DB_INT_TYPE, (char *)0,  intvar);
**	IIsqParms(IIsqpEND, (char *)0,     0, (char *)0,  0);
**
**	GCA_GW_PARM content:
**	     a = a_name, b = 'b string', c = 'xx', d = 123, e = 55
**
**  2. EXEC SQL CONNECT ... WITH :with_clause_var;
**
**	IIsqMods(IImodWITH);
**	IIsqConnect(EQ_C, ....);
**	IIsqParms(IIsqpINI, with_clause_var, DB_CHR_TYPE, (char *)0, 0);
**	IIsqParms(IIsqpEND, (char *)0, 0, (char *)0, 0);
**
**  3. EXEC SQL CONNECT ... 
**	     WITH HIGHDXID=intval|:intvar, LOWDXID=intval|:intvar;
**
**	IIsqMods(IImodWITH);
**	IIsqConnect(EQ_C, ....);
**	IIsqParms(IIsqpINI, "HIGHDXID", DB_INT_TYPE, (char *)0, intval);
**	IIsqParms(IIsqpADD, "LOWDXID",  DB_INT_TYPE, (char *)0, intval);
**	IIsqParms(IIsqpEND, (char *)0,    0,        (char *)0, 0);
-*
**  Side Effects:
**	
**  History:
**	11-sep-1987	- Written (ncg)
**	02-oct-1987	- Modified to allocate and free buffer (ncg)
**	05-feb-1988	- WITH message is now part of startup (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	12-mar-1989	- Support for 2PC (bjb)
**	19-may-1989	- Changed names of globals for multiple connects. (bjb)
**	18-jun-1990 (barbara)
**	    Process logicals even if association was not successful. Part of bug
**	    fix 21832.
**	06-jul-1990 (barbara)
**	   IIsendlog needs an accurate setting of II_G_CONNECT flag.
**	27-sep-1990 (barbara)
**	    Revert to calling IIsendlog only if connected.  II_EMBED_SET
**	    logical has been handled by IIdbconnect.
**      6-oct-1992 (nandak - rejoined by larrym)
**          Added support for XA xid (GCA_XA_RESTART)
**      18-Feb-1994 (daveb)  33593, 35805, 35873, 36034, 38382,
**  	    42323, 45002, 45436, 47179, 51315, 51673, 54404
**          Send client information on END, starting build-up
**  	    of params if none have been sent so far.
**      19-nov-1994 (harpa06)
**          Integrated bug fix #62708 by angusm from INGRES 6.4 code (harpa06)
**	30-Apr-99 (gordy)
**	    Copy any error code to the new current session when
**	    a connection request fails.
**	08-nov-2002 (drivi01)
**	    Added an error check for W_LQ00ED_ING_SET_USAGE error, which is
**	    produced as result of a select, update, delete, alter, drop, modify 
**	    or create statements assigned to ING_SET variable. (bug 109058)
*/

VOID
IIsqParms(flag, name_spec, val_type, str_val, int_val)
i4	flag;
char	*name_spec;
i4	val_type;
char	*str_val;
i4	int_val;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4		index;		/* For adding parameters */
    DB_DATA_VALUE	dbv;		/* For sending to GCA */
    char		*cp;		/* Pointer into param buffer */
    i4			len;		/* Current length of data */
    char		int_buf[50];	/* To convert integers */
    STATUS		status = OK;	/* Closing status */

    /*
    ** If an earlier routine turned off the WITH flag, then do not send
    ** any more data.
    */
    if ( ! (thr_cb->ii_th_flags & II_T_WITHLIST) )
	return;

    switch (flag)
    {
      case IIsqpINI:
	/* If the with_clause is empty then do not bother */
	if (name_spec == (char *)0 || *name_spec == '\0')
	    return;

	/* Allocate the parameter buffer (+2 for ", " +1 for null) */
	IIlbqcb->ii_lq_parm.ii_sp_buf = (char *)MEreqmem((u_i4)0,
			(u_i4)(IIsqpMAX_PARM_SIZE+3), TRUE, (STATUS *)0);
	if (IIlbqcb->ii_lq_parm.ii_sp_buf == (char *)0)	/* Couldn't allocate */
	    return;
	IIlbqcb->ii_lq_parm.ii_sp_flags = II_PC_DEFAULT;
	IIlbqcb->ii_lq_parm.ii_sp_buf[0] = '\0';

	/* FALL THROUGH to add the first parameter to the message */

      case IIsqpADD:
	cp = IIlbqcb->ii_lq_parm.ii_sp_buf;
	/* If buffer not allocated, then don't bother */
	if (cp == (char *)0)
	    return;

	/* 
	** Generate: 
	** 		[,]name=value 
	** and trim blanks off strings.
	*/
	len = STlength(cp);
	cp = cp + len;

	if (STbcompare(ERx("highdxid"), 8, name_spec, 8, TRUE) == 0)
	{
	    if (   len > 0
	        || IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XID 
		|| IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XA_XID
		|| val_type != DB_INT_TYPE
	       )
	    {
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0008_XID, II_ERR, 0, (char *)0);
		return;
	    }
	    CVna(int_val, cp);
	    STcat(cp, ERx(":"));
	    IIlbqcb->ii_lq_parm.ii_sp_flags = II_PC_XID;
	    return;
	}

	if (STbcompare(ERx("lowdxid"), 7, name_spec, 8, TRUE) == 0)
	{
	    if (   len == 0
		|| IIlbqcb->ii_lq_parm.ii_sp_flags != II_PC_XID
		  || val_type != DB_INT_TYPE
	       )
	    {
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0008_XID, II_ERR, 0, (char *)0);
		return;
	    }
	    CVna(int_val, cp);
	    return;
	}

        if (STbcompare(ERx("xa_xid"), 11, name_spec, 11, TRUE) == 0)
        {
	    if (   len > 0
	        || IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XID 
	        || IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XA_XID 
		|| val_type != DB_CHR_TYPE
	       )
	    {
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0008_XID, II_ERR, 0, (char *)0);
		return;
	    }
	    STcat(cp, str_val);
	    IIlbqcb->ii_lq_parm.ii_sp_flags = II_PC_XA_XID;
	    return;
        }

	if (len > 0) 		/* Not first time */
	{
	    /* Can't mix gateway params with 2PC params */
	    if (   (IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XID)
		|| (IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XA_XID) )
	    {
		IIlbqcb->ii_lq_parm.ii_sp_flags = II_PC_DEFAULT;
		IIlocerr(GE_SYNTAX_ERROR, E_LQ0008_XID, II_ERR, 0, (char *)0);
	    }
	    STcat(cp, ERx(","));
	    cp++;
	}

	STlcopy(name_spec, cp, IIsqpMAX_PARM_SIZE - len);
	STtrmwhite(cp);
	if (val_type == DB_CHR_TYPE)
	{
	    /* If str_val is null then whole thing is in name_spec */
	    if (str_val)
	    {
		STcat(cp, ERx("='"));
		STcat(cp, str_val);
		STtrmwhite(cp);
		STcat(cp, ERx("'"));
	    }
	}
	else	/* Integer */
	{
	    STcat(cp, ERx("="));
	    CVna(int_val, int_buf);
	    STcat(cp, int_buf);
	}
	return;

      case IIsqpEND:		/* int_val is "child" spawn flag */

	/* Complete WITH clause message */

	/* tack on untrusted client info now, if not -X connection and */
	/* not XA special reconnect */
	if( !int_val &&
	   IIlbqcb->ii_lq_parm.ii_sp_flags != II_PC_XID &&
	   IIlbqcb->ii_lq_parm.ii_sp_flags != II_PC_XA_XID )
	    IIsend_untrusted_info( IIlbqcb, IIlbqcb->ii_lq_parm.ii_sp_buf 
					    ? IIsqpADD : IIsqpINI );

	if (IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XID)
	    index = GCA_RESTART;
	else if (IIlbqcb->ii_lq_parm.ii_sp_flags == II_PC_XA_XID)
	    index = GCA_XA_RESTART;
	else
	    index = GCA_GW_PARM;


	II_DB_SCAL_MACRO(dbv, DB_INT_TYPE, sizeof(index), &index);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &dbv);
 	if (IIlbqcb->ii_lq_parm.ii_sp_buf != (char *)0)
 	{
 	    cp = IIlbqcb->ii_lq_parm.ii_sp_buf;
 	    II_DB_SCAL_MACRO(dbv, DB_CHA_TYPE, STlength(cp), cp);
 	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VDBV, &dbv);
 	    MEfree(cp);
 	    IIlbqcb->ii_lq_parm.ii_sp_buf = (char *)0;
 	}
 	else		/* WITH clause may have been empty */
 	{
 	    II_DB_SCAL_MACRO(dbv, DB_CHA_TYPE, 0, ERx(""));
 	    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VDBV, &dbv);
 	}

/* Imported from INGRES 6.4 code line -- bug fix #62708 by angusm (harpa06) */

	/*
        ** reset DML to SQL, to ensure correct handling
        ** of failure to reconnect to WILLING_COMMIT
        ** 2PC TX (E_US4711, E_US4712, E_US4715)
        ** (bug 62708)
        */
 
        if (index == GCA_RESTART)
                IIlbqcb->ii_lq_dml = DB_SQL;
 
	/* Complete connection to server */
	status = IIcgc_tail_connect(IIlbqcb->ii_lq_gca);
	if (status == OK)
	{
	    IIsendlog( str_val, int_val );
		if (IIlbqcb->ii_lq_error.ii_er_num==W_LQ00ED_ING_SET_USAGE)
		{
			IIlocerr(GE_WARNING, W_LQ00ED_ING_SET_USAGE, II_ERR, 0, (char *)0 );	
			IIlbqcb->ii_lq_error.ii_er_num=IIerOK;
		}
	}
	else
	{
	    /* Flag not connected, release sess and give an error */
	    i4  ses_id = IIlbqcb->ii_lq_sid;
	    if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
		IIlocerr(GE_HOST_ERROR, E_LQ0001_STARTUP, II_ERR, 0, (char *)0);
	    IILQcseCopySessionError( &IIlbqcb->ii_lq_error, 
				     &thr_cb->ii_th_defsess.ii_lq_error );
	    IILQssSwitchSession( thr_cb, NULL );
	    IIlbqcb = thr_cb->ii_th_session;
	    IILQsdSessionDrop( thr_cb, ses_id );
	}

	/*
	** Even if connection was not successful, some II_EMBED_SET logicals,
	** e.g. PROGRAMQUIT require setting.
	*/

	/*
	** Because this is the last routine called at CONNECT time, unset
	** the WITH clause flag, and reset the DML to the default.
	*/
	thr_cb->ii_th_flags &= ~II_T_WITHLIST;
	IIlbqcb->ii_lq_dml = DB_QUEL;
	return;
    }
}


/*{
** Name:	IIsend_untrusted_info
**
** Description:
**	Send untrustable client information along with any existing
**  	with clause that might be in process.  Called while processing
**  	IIsqParms( IIsqpEND, ... ).
**
** Inputs:
**	op  	whether this is the first WITH param, or an ADD.
**
** Outputs:
**	none
**
** Returns:
**	none.
**
** Side Effects: (if not obvious from description)
**
** History:
**      18-Feb-1994 (daveb)
**          created.
**      11-Jan-2005 (wansh01)
**          INGNET 161 bug # 113737
**	    supress [uid,pwd] connection info.  
*/

static VOID
IIsend_untrusted_info( II_LBQ_CB *IIlbqcb, i4  op )
{
    char buf[ GL_MAXNAME + 1 ];	/* IDuser */
    char *bufp = buf;
    PID	pid;
    char *pleft, *pright, *pcomma;
    int  len; 
    char connInfo[ MAX_LOC + 1 ];	/* Connection info*/

    IDname( &bufp );
    STzapblank( buf, buf );
    IIsqParms( op, ERx("user"), DB_CHR_TYPE, buf, 0  );
    GChostname( buf, sizeof(buf) );
    IIsqParms( IIsqpADD, ERx("host"), DB_CHR_TYPE, buf, 0 );
    (void)TEname( buf );
    IIsqParms( IIsqpADD, ERx("tty"), DB_CHR_TYPE, buf, 0 );
    PCpid( &pid );
    IIsqParms( IIsqpADD, ERx("pid"), DB_INT_TYPE, (char *)0, pid );
    if (( IIlbqcb->ii_lq_con_target[0] == '@')  
           && (pleft = STchr(IIlbqcb->ii_lq_con_target,'['))
	   && (pright = STchr(IIlbqcb->ii_lq_con_target,']'))
	   && ((pleft) && (pcomma= STchr(pleft,','))))
    {
	len = pcomma - IIlbqcb->ii_lq_con_target +1;
	STncpy(connInfo,IIlbqcb->ii_lq_con_target, len);
	connInfo[len]='\0';
	STcat(connInfo,"****");
	STcat(connInfo,pright);
	IIsqParms( IIsqpADD, ERx("conn"), DB_CHR_TYPE, connInfo, 0 );
     }
    else
	IIsqParms( IIsqpADD, ERx("conn"), DB_CHR_TYPE,
    	    	IIlbqcb->ii_lq_con_target, 0 );
}


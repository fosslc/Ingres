/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<cm.h>
# include	<cv.h>
# include	<nm.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<eqrun.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>
/**
**  Name: iisndlog.c  - Send logical attributes to the back end.
** 
**  Defines:
**	IIsendlog 	- Send logicals to DBMS.
**	IILQreReadEmbed	- Read II_EMBED_SET logicals.
**	IILQpeProcEmbed - Process the various settings of II_EMBED_SET.
**
**  Locals:
**	sndLog    - Logical sending (local).
**	sndFile   - File included (local).
**	sndQryLog - Send the query built by the logicals (local).
**
**  History:
**	28-mar-1984	- Rewrote for CL and renamed for all Logical
**			  attributes. (ncg)
**	23-jul-1985  	- Added call to send multinational sets to BE. (jrc)
**	23-dec-1985 	- Added support for ING_SYSTEM_SET and ING_SET_dbname
**			  and allowed them top point to files of commands. (edh)
**	05-may-1987	- Rewritten. (ncg)
**	16-dec-1987	- Added support for II_EMBED_SET, a logical for 
**			  frontend query and error tracing. (bjb)
**	31-aug-1988	- Removed the processing of multi-national formats.
**			  Now part of libqgca startup with ADF. (ncg)
**	14-apr-1989	- Recognize new value "dbmserror" for II_EMBED_SET.(bjb)
**	19-may-1989	- Changed names of globals for multiple connect. (bjb)
**	06-dec-1989	- Recognize "eventdisplay" for II_EMBED_SET. (bjb)
**	30-apr-1990	- Recognize II_EMBED_SET/prefetchrows & printcsr. (ncg)
**	30-may-1990	(barbara)
**	    Process II_EMBED_SET values even if this is not the first
**	    session.  Some values require setting the "local" IILBQCB.
**	14-jun-1990	(barbara)
**	    Recognize "programquit" for backwards compatibility after
**	    bug fix 21832.  Also, this routine is now called whether or
**	    not database connection was successful.  If not connected,
**	    or if reassociating for 2 phase commit, don't process "dbms"
**	    logicals.
**	21-sep-1990	(barbara)
**	    Separate entirely the processing of ING_SET and other db logicals
**	    from that of II_EMBED_SET logical.  IIsendlog now handles db
**	    logicals.  Two routines, IILQreReadEmbed() and IILQpeProcEmbed(),
**	    process II_EMBED_SET logicals; procLibqLog goes away.  This
**	    change was motivated by having to read NOTIMEZONE before database
**	    connection when all II_EMBED_SET logicals were being read after
*	    database connection.  Now all values of II_EMBED_SET are processed
**	    before database connection and a side effect of that change is
**	    that we can now use the PRINTGCA setting to process connection
**	    flags.
**	1-nov-1990	(barbara)
**	    Put back II_EMBED_SET printcsr which I eliminated somewhere along
**	    the line and cleaned up some other problems.
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	16-mar-1991 (teresal)
**	    Add II_EMBED_SET "savequery" feature.
**	13-mar-1991 (barbara)
**	    Support II_EMBED_SET printtrace.
**	10-apr-1991 (kathryn)
**	    Support II_EMBED_SET trace files.
**	    "gcafile <filename>"  for "printgca" output 
**	    "qryfile <filename>"  for "printqry" output
**	    "tracefile <filename>"for "printtrace" output.
**	20-jul-1991 (teresal)
**	    Changed EVENT to DBEVENT.
**	01-nov-1991  (kathryn)
**	    Changed IILQpeProcEmbed() to save trace file names in newly added
**	    IILQ_TRACE structures of the IIglbcb rather than directly in
**	    IIglbcb.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

static  VOID	sndLog( II_LBQ_CB *, char * );
static  VOID	sndFile( II_LBQ_CB *, char * );
static  VOID	sndQryLog( II_LBQ_CB *, char *, bool, bool );

/*{
+*  Name: IIsendlog - Send logicals at INGRES startup.
**
**  Description:
**	At INGRES startup send value of logical attributes (ie, trace and
**	tweaking flags).
**
**  The "SET" logicals:
**
**	There are 3 tiers of SET logicals -- process, database,
**	and system.  This is similar to the terminal monitor's login
**	files.  Also, the logicals can "point to" a file of commands by
**	simply having the first word be "include".  Example:
**
**	  ING_SYSTEM_SET  ==  "include sys_ingres:[ingres]tune.set"
**
**	This logical would instruct this routine to open the file
**	TUNE.SET and send a whole list of commands to the backend.
**
**	The basic working is as follows.
**
**	if	the logical exists
**	then	if	the first word is "include"
**		then	treat the name as a file of commands
**		else	send just the string
**
**  Inputs:
**	db - char * - Database name (or NULL).
**	child - i4  - Is this a spawned process.  If so do not send SET
**		      statements as we attach to the same DBMS.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	31-aug-1988 - Removed check of LOCK_INGBACK for "nodeadlock", removed
**		      call to multinational, added check for "if spawned." (ncg)
**	24-may-1989 - Don't process II_EMBED_SET on subsequent connects. (bjb)
**	30-may-1990 	(barbara)
**	    DO process II_EMBED_SET on subsequent connects.  procLibqLog
**	    will select which values should be processed.
**	14-jun-1990	(barbara)
**	    Don't process dbms logicals if not connected or if reassociation.
**	21-sep-1990 (barbara)
**	    Reading of II_EMBED_SET logical is transferred to IILQreReadEmbed().
**       7-Sep-2004 (hanal04) Bug 48179 INGSRV2543
**          Do no apply ING_SET if -Z flag used in the CONNECT.
*/

void
IIsendlog(db, child)
char	*db;	/* pointer to database name (may include network prefix) */
i4	child;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    char	*logstr;
    char	*colonloc;		/* Point at colon */
    char	dblogstr[MAX_LOC+1];	/* Build ING_SET_dbname logical */


    /* If spawned process or not connected, do NOT send ING_SET queries */
    if ( child  ||  ! (IIlbqcb->ii_lq_flags & II_L_CONNECT)  ||
	 IIlbqcb->ii_lq_parm.ii_sp_flags )
	return;

    NMgtAt( ERx("ING_SYSTEM_SET"), &logstr );
    if (logstr != NULL && *logstr)
    {
	STlcopy(logstr, dblogstr, MAX_LOC);
	sndLog( IIlbqcb, dblogstr );
    }

    /*
    **	Database names can have node name prefixes on them.
    **	The standard INGRES/Net format is
    **
    **		node::database
    **
    **	The following steps read thru the "db" string and moves
    **	the string past that last colon (:) found.  It will still
    **	work if no network name is present.
    */

    if (db != NULL)
    {
	/* If we find a colon, move the pointer past the last one */
	if ((colonloc = STrchr(db, ':')) != NULL)
	    db = ++colonloc;
	if (db[0] != '\0')
	{
	    STcopy( ERx("ING_SET_"), dblogstr );
	    CVupper( db );
	    STcat( dblogstr, db );
	    NMgtAt( dblogstr, &logstr );
	    if (logstr != NULL && *logstr)
	    {
		STlcopy(logstr, dblogstr, MAX_LOC);
		sndLog( IIlbqcb, dblogstr );
	    }
	}
    }

    if( ! (IIlbqcb->ii_lq_flags & II_L_NO_ING_SET))
    {
        NMgtAt( ERx("ING_SET"), &logstr );
        if (logstr != NULL && *logstr)
        {
	    STlcopy(logstr, dblogstr, MAX_LOC);
	    sndLog( IIlbqcb, dblogstr );
        }
    }
}

/*{
+*  Name: sndLog - Send value of logical attribute.
**
**  Description:
**	The logicals have been evaluated, and now they can be sent to the DBMS.
**	This routine handles:
**	1. Simple logicals that have just 1 or more queries separated by
**	   semicolons (ie, $ def ing_set "set printqry; set trace point dm123").
**	2. Logicals that point at include files (ie, "include tune.set").
**
**  Inputs:
**	str - Evaluated logical string
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	05-may-1987	- Rewritten (ncg)
**	26-oct-1995 (canor01)
**	    Allow the ING_SET string to be entirely surrounded
**	    by quotes.
**	08-nov-2002 (drivi01)
**	    Added a new if loop to check for delete, update, select
**	    create, drop, alter, or modify statements in ING_SET 
**	    variable as a fix to bug #109058, to prevent above 
**	    statements from being executed.
*/

static VOID
sndLog( II_LBQ_CB *IIlbqcb, char *str )
{
    char	*start_cp, *semi_cp;

    /* Skip over initial white space */
    for(; CMwhite(str); CMnext(str))
	;

    /* Trim any trailing white space */
    STtrmwhite(str);

    /* Allow entire string to be quoted */
    if ( !CMdbl1st(str) && *str == '"' && str[STlength(str)-1] == '"' )
    {
	CMnext(str);
	str[STlength(str)-1] = '\0';
    }

    /*
    ** If the first word is "include", then they must want a file.
    ** When passing the string, first skip over the word "include".
    */
    if (STbcompare(ERx("include"), 7, str, 7, TRUE) == 0)
    {
	sndFile( IIlbqcb, str + 7 );
    }
    /*
    ** Fix for bug #109058, searches for select, delete, drop, create,
    ** alter, modify, or update statements, if found then ING_SET isn't 
    ** executed.
    */
    else if (STbcompare(ERx("select"), 6, str, 6, TRUE)==0 ||
			STbcompare(ERx("delete"), 6, str, 6, TRUE)==0 ||
			STbcompare(ERx("drop"), 4, str, 4, TRUE)==0 ||
			STbcompare(ERx("create"), 6, str, 6, TRUE)==0 ||
			STbcompare(ERx("alter"), 5, str, 5, TRUE)==0 ||
			STbcompare(ERx("update"), 6, str, 6, TRUE)==0 ||
			STbcompare(ERx("modify"), 6, str, 6, TRUE)==0)
	{
		
			IIlbqcb->ii_lq_error.ii_er_num=W_LQ00ED_ING_SET_USAGE;
	}
	else
    {
	/*
	** Send "set" values to backend. For each semicolon, split
	** the statement.
	*/
	for (start_cp = str; start_cp && *start_cp;)
	{
	    if (semi_cp = STindex(start_cp, ERx(";"), 0))
	    {
		*semi_cp = '\0';
		sndQryLog( IIlbqcb, start_cp, TRUE, TRUE );
		start_cp = semi_cp+1;		/* Skip the semi */
	    }
	    else			/* At end of string or no semis */
	    {
		sndQryLog( IIlbqcb, start_cp, TRUE, TRUE );
		break;
	    }
	}
    }
}

/*{
+*  Name: sndFile - Open and send statements inside the included file.
**
**  Description:
**	Read all the statements (separated by semicolons) in the included
**	ing_set file, and send them to the DBMS.
**
**  Inputs:
**	str - File name to open and read.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	01-may-1986	- Written (edh)
*/

static VOID
sndFile( II_LBQ_CB *IIlbqcb, char *str )
{
#define	MAXREC	256
    i4		count;		/* count the number of lines found */
    char	rec[MAXREC+1];  /* used to buffer lines from a file */
    char	locbuf[MAX_LOC+1]; /* used to hold location string */
    LOCATION	loc;		/* location for file */
    FILE	*fp;		/* file descriptor */

    /* Skip over initial white space */
    for(; CMwhite(str); CMnext(str))
	    ;
    STcopy( str, locbuf );
    LOfroms( PATH & FILENAME, locbuf, &loc );

    count = 0;
    if (SIopen(&loc, ERx("r"), &fp) == OK)
    {
	while (SIgetrec(rec, MAXREC, fp) == OK)
	{
	    count++;
	    /*
	    ** Send the record without trimming any white space or
	    ** newlines.  Trimming all white may result in errors since
	    ** the text will be squeezed together and possibly get
	    ** syntax errors.  Remove semicolons from end of line.
	    */
	    if (rec[STlength(rec) -2] == ';')
	    {
		rec[STlength(rec)-2] = '\0';
		sndQryLog( IIlbqcb, rec, TRUE, TRUE );
		count = 0;
	    }
	    else
		sndQryLog( IIlbqcb, rec, TRUE, FALSE );
	}
	SIclose( fp );
    }

    /* If we sent something to the backend, sync up again */
    if (count > 0)
	sndQryLog( IIlbqcb, (char *)0, FALSE, TRUE );
}


/*{
+*  Name: sndQryLog - Send actual query part of iisendlog
**
**  Description:
**	This routine just calls the IIwrite routine with the actual
**	query. If 'sync' is set then it also calls IIsyncup. The
**	routine is extracted so that the dml language doesn't change
**	underneath us.
**
**	This routine handles the following special cases (if 'check' is
**	set):
**	1. Case of "faking" preprocessing with -d (ie, "equel-d")
**
**	Other special cases are now under the II_EMBED_SET logical and
**	are processed in procLibqLog().	   
**
**  Inputs:
**	str	- Query string to send (if null nothing is sent).
**	user	- This is a user string so check for the special cases
**		  we want to handle.
**	sync	- Call IIsyncup?
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	
**  History:
**	05-may-1987	- Written (ncg)
**	09-nov-1987	- Modified to check for new flags (ncg)
**	24-may-1989	- Changed names of globals for multiple connections.
**	09-sep-1991	(kathryn)  Bug 38933
**		Don't compare EOS (blank line) with "equel-d".
**		
*/

static VOID
sndQryLog( II_LBQ_CB *IIlbqcb, char *str, bool user, bool sync )
{
    i4		dml;
    char	*cp;

    if (str)
    {
	if (user)
	{
	    /* Skip over initial white space */
	    for(cp = str; CMwhite(cp); CMnext(cp))
		    ;
	    if (*cp != EOS && STbcompare(ERx("equel-d"), 0, cp, 7, TRUE) == 0)
	    {
		/* Allow Equel run-time -d simulation */
		IIdbg_info( IIlbqcb, ERx(""), 1 );
		return;
	    }
	} /* if checking user strings */

	IIwritio(FALSE, (i2 *)0, 1, DB_CHR_TYPE, 0, str);
    } /* if there is a string */

    if (sync && (IIlbqcb->ii_lq_curqry & II_Q_INQRY))
    {
        dml = IIlbqcb->ii_lq_dml;	/* It may change inside IIsyncup */
	IIsyncup((char *)0, 0);
	IIlbqcb->ii_lq_dml = dml;
    } /* if need to sync up */
}

/* {
** Name: IILQesaEmbSetArg - Read and copy  II_EMBED_SET attribute argument.
**
** Description:
**	Read an II_EMBED_SET attribute argument from II_EMBED_SET buffer and
**	copy to buffer specified by "desptr". Characters will be read from
**	"srcptr" and copied into "destptr" until:  1) a semicolon is found,  
**	2) whitespace is found, or 3) "length" characters have been copied.
**	The string copied to "desptr" will be null terminated.
**
**	This routine is called from IILQreReadEmbed to parse arguments for
**	those attributes which require/allow arguments.  Attributes which
**	require arguments are "gcafile", "qryfile" and "tracefile".
**	
**	EXAMPLE: 	setenv II_EMBED_SET "printgca; gcafile myfilename; ..."
**
**	When IILQreReadEmbed() parses "gcafile" it will call this routine with
**	"srcptr" pointing to "myfilename" in the II_EMBED_SET buffer and 
**	"destptr" pointing to an empty buffer into which "myfilename" will be 
**	copied. "srcptr" will then be positioned at the semicolon in the
**	II_EMBED_SET buffer.
**		
** Inputs:
**	srcptr:  Pointer into II_EMBED_SET buffer where argument begins.
**	destptr: Buffer to copy argument into.
**	length:  Maximum number of characters to copy.
**
** Outputs:
**	destptr: Buffer into which character string was copied.
**
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
** Side Effects:
**	srcptr:  Points to end of character string argument in II_EMBED_SET 
**		 buffer.
**
** History:
**	10-apr-1991 (kathryn)
**		Written.
**	10-oct-92 (leighb) DeskTop Porting Change: added VOID return type.
*/
VOID
IILQesaEmbSetArg(srcptr,destptr,length)
char	*srcptr;
char	*destptr;
i4	length;
{
	char	*tp;
	int	i;

	tp = destptr;

	while (CMwhite(srcptr))
	{
		length--;
		CMnext(srcptr);
	}
	while (   *srcptr != EOS   && *srcptr != ';' 
	       && !CMwhite(srcptr) && length > 0)
	{
		length --;
		CMcpyinc(srcptr, tp);
	}
	*tp = EOS;
}

/*{
**  Name: IILQreReadEmbed - Read and save info from LIBQ logical II_EMBED_SET
**
**  Description:
**	The II_EMBED_SET logical allows frontend programs to set some
**	LIBQ error behavior and to trace information.  This routine reads the
**	logical and saves the information.  IILQpeProcEmbed is then called
**	to make use of the information.  The following values may be set.
**	Note that, like ING_SET, the user may specify more than one of these
**	values by separating each with a semi-colon.
**
**	    Value	        Action
**	    -------------	-----------------------------------------
**	    "errorqry"		Print query on error (equivalent to
**				ING_SET "equel-d", but applies to esql, too)
**	    "sqlprint"		Print all SQL errors to output
**	    "printqry"		Print all query text and timing information
**				to specified file - default is "iiprtqry.log" 
**				file.  (An improved version of ING_SET 
**				"set printqry" feature which causes the query 
**				text to be saved by the FE rather than
**				relying on the DBMS to send it to us.)
**	    "qryfile fname"     Write "printqry" output to file <fname>.
**	    "printgca"		Trace GCA messages (and timing) to "gcafile"
**				file - default is "iiprtgca.log" file.
**	    "gcafile fname"	Write "printgca" output to file <fname>.
**	    "printtrace"	Server trace messages get printed to a file.
**				Default file is "iiprttrc.log" file.
**	    "tracefile fname"   Write "printtrace" output to file <fname>.
**	    "dbmserror"		Treat local (dbms) errors as default errors
**				for backwards compatibility.  This enables
**				post-6.2 ESQL appl'ns to see old (local) INGRES
**				error numbers in INQUIRE_EQUEL/errorno and
**				SQLCA/sqlcode.
**	    "dbeventdisplay"	Print events as they return from dbms.
**	    "prefetchrows"	How many rows to pre-fetch on read-only cursor.
**	    "printcsr"		Trace various LIBQ & LIBQGCA cursor operations.
**	    "programquit"	Quit on fatal communications errors (for
**				backwards compatibility).
**	    "notimezone"	Don't send timezone information at startup.
**	    "nobyref"		emulate GCA_PROTOCOL_50 semantics when 
**				executing a DB PROCEDURE.  
**
**	Note: If adding an II_EMBED_SET value to this routine, also add an
**	action to IILQpeProcEmbed.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**
**  History:
**	21-sep-1990 (barbara)
**	    Extracted from IIsendlog and procLibqLog.
**	1-nov-1990	(barbara)
**	    Put back II_EMBED_SET printcsr which I eliminated somewhere along
**	    the line.
**	14-mar-1991 (teresal)
**	    Modified II_EMBED_SET parameters "printgca", "printqry" and
**	    "errorqry" to just set the flag and don't start the
**	    tracing until connect time.
**	13-mar-1991 (barbara)
**	    Support II_EMBED_SET printtrace.
**	10-apr-1991 (kathryn)
**	    Support II_EMBED_SET gcafile, qryfile, and tracefile.
**	01-nov-1991  (kathryn)
**	    Save trace information in newly added IILQ_TRACE structures of the
**	    IIglbcb rather than directly in IIglbcb.
**	14-dec-1992 (larrym)
**	    TEMPORARILY added "nobyref" to deal with FE/BE integration issues
**	    this should be removed before the product goes to testing.
*/
VOID
IILQreReadEmbed()
{
    char	*cp, *semi_cp;
    char	logbuf[MAX_LOC +1];
    char    	tmpbuf[MAX_LOC + 1];

    NMgtAt( ERx("II_EMBED_SET"), &cp );
    if (cp == NULL || *cp == '\0')
	return;
    STlcopy(cp, logbuf, MAX_LOC);
    /* 
    ** Parse string for all II_EMBED_SET values
    */
    for (cp = logbuf; cp && cp;)
    {
	/* Skip initial white space before each value */
	for (; CMwhite(cp); CMnext(cp))
	    ;
	if (*cp == EOS)		/* Fall out - else STbcompare = 0 */
	    break;
	if (STbcompare(ERx("errorqry"), 0, cp, 8, TRUE) == 0)
	{
	    /*	
	    ** Allow EQUEL/ESQL runtime simulation of "-d" flag.
	    ** Note: only set "-d" flag if "printqry" is not on.
	    ** "printqry" always overrides "-d".
	    */
	    if ( ! (IIglbcb->iigl_qrytrc.ii_tr_flags & II_TR_FILE) )
		IIglbcb->ii_gl_flags |= II_G_DEBUG;
	}
	else if (STbcompare(ERx("sqlprint"), 0, cp, 8, TRUE) == 0)
	{
	    IIglbcb->iigl_embset.iies_values |= II_ES_SQLPRT;
	}
	else if (STbcompare(ERx("printqry"), 0, cp, 8, TRUE) == 0)
	{
	    /*
	    ** We can't call IIdbg_toggle() at this point to turn 
	    ** on tracing.  Set the trace flag, tracing will be 
	    ** enabled when the first connection is made.
	    */
	    IIglbcb->iigl_qrytrc.ii_tr_flags |= II_TR_FILE;
	    IIglbcb->ii_gl_flags &= ~II_G_DEBUG;	/* Override "-d" */
	}
	else if (STbcompare(ERx("printgca"), 0, cp, 8, TRUE) == 0)
	{
	    /*
	    ** We can't call IILQgstGcaSetTrace() at this point
	    ** to turn on tracing.  Set the trace flag, tracing
	    ** will be enabled when the first connection is made.
	    */
	    IIglbcb->iigl_msgtrc.ii_tr_flags |= II_TR_FILE;
	}
	else if (STbcompare(ERx("printtrace"), 0, cp, 10, TRUE) == 0)
	{
	    /*
	    ** We can't call IILQsstSvrSetTrace() at this point
	    ** to turn on tracing.  Set the trace flag, tracing
	    ** will be enabled when the first connection is made.
	    */
	    IIglbcb->iigl_svrtrc.ii_tr_flags |= II_TR_FILE;
	}
	else if (STbcompare(ERx("dbmserror"), 0, cp, 7, TRUE) == 0)
	{
	    IIglbcb->iigl_embset.iies_values |= II_ES_DBMSER;
	}
	else if (STbcompare(ERx("dbeventdisplay"), 0, cp, 12, TRUE) == 0)
	{
	    IIglbcb->iigl_embset.iies_values |= II_ES_EVTDSP;
	}
	else if (STbcompare(ERx("prefetchrows"), 0, cp, 12, TRUE) == 0)
	{
	    char	rowbuf[21], *rp;

	    for (cp += 12;		/* Skip word until digits */
		 *cp != EOS && *cp != ';' && !CMdigit(cp);
		 CMnext(cp))
		;
	    IIglbcb->iigl_embset.iies_prfarg = 0;	/* Initialize */
	    if (CMdigit(cp))		/* Scan in & convert number */
	    {
		rp = rowbuf;
		while (CMdigit(cp) && (rp - rowbuf) < (sizeof(rowbuf)-1))
		    CMcpyinc(cp, rp);
		*rp = EOS;
		CVan(rowbuf, &IIglbcb->iigl_embset.iies_prfarg);
	    }
	    IIglbcb->iigl_embset.iies_values |= II_ES_PFETCH;
	}
	else if (STbcompare(ERx("printcsr"), 0, cp, 8, TRUE) == 0)
	{
	    IIglbcb->ii_gl_flags |= II_G_CSRTRC;
	}
	else if (STbcompare(ERx("programquit"), 0, cp, 11, TRUE) == 0)
	{
	    IIglbcb->ii_gl_flags |= II_G_PRQUIT;
	}
	else if (STbcompare(ERx("notimezone"), 0, cp, 10, TRUE) == 0)
	{
	    IIglbcb->iigl_embset.iies_values |= II_ES_NOZONE;
	}
	else if (STbcompare(ERx("savequery"), 0, cp, 9, TRUE) == 0)
	{
	    /*
	    ** We can't call IIdbg_save() at this point to turn 
	    ** on tracing.  Set the trace flag, tracing will be 
	    ** enabled when the first connection is made.
	    */
	    IIglbcb->ii_gl_flags |= II_G_SAVQRY;
	    IIglbcb->ii_gl_flags &= ~II_G_DEBUG;	/* Override "-d" */
	}
	else if (STbcompare(ERx("gcafile"), 0, cp, 7, TRUE) == 0)
	{
		cp += 7;
		IILQesaEmbSetArg(cp,tmpbuf,MAX_LOC - (cp - logbuf));
		IIglbcb->iigl_msgtrc.ii_tr_fname = STalloc(tmpbuf);
	}
	else if (STbcompare(ERx("qryfile"), 0, cp, 7, TRUE) == 0)
	{
		cp += 7;
		IILQesaEmbSetArg(cp,tmpbuf,MAX_LOC - (cp - logbuf));
		IIglbcb->iigl_qrytrc.ii_tr_fname = STalloc( tmpbuf );
	}
	else if (STbcompare(ERx("tracefile"), 0, cp, 9, TRUE) == 0)
	{
		cp += 9;
		IILQesaEmbSetArg(cp,tmpbuf,MAX_LOC - (cp - logbuf));
		IIglbcb->iigl_svrtrc.ii_tr_fname = STalloc( tmpbuf );

	}

	if (semi_cp = STindex(cp, ERx(";"), 0))
	{
	    cp = semi_cp+1;
	}
	else
	{
	    break;
	}
    }
}

/*{
**  Name: IILQpeProcEmbed - Process saved info from LIBQ logical II_EMBED_SET
**
**  Description:
**	The values in the II_EMBED_SET logical are parsed by IILQreReadEmbed.
**	This routine transfers the saved information into settings in the
**	IIglbcb (information global to an application) and the IIlbqcb
**	(information that applies to an individual FE session). 
**
**  Inputs:
**	None
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
**
**  Side Effects:
**	    None.
**
**  History:
**	21-sep-1990 (barbara)
**	    Extracted from IIsendlog and procLibqLog.
**	07-mar-1991 (barbara)
**	    Support II_EMBED_SET printtrace.
**	10-apr-1991 (kathryn)
**	    Support II_EMBED_SET gcafile, qryfile and tracefile.
**	14-dec-1992 (larrym)
**	    Added temp value NOBYREF to deal with FE/BE integration issues
**	    this should be removed before the product goes beta.
**	18-may-1993 (larrym)
**	    removed NOBYREF functionality.
*/

VOID
IILQpeProcEmbed( II_LBQ_CB *IIlbqcb )
{
    if (IIglbcb->iigl_embset.iies_values & II_ES_DBMSER)
    {
	/* 
	** Set default error reporting to dbms errors only if
	** SET_INGRES statement has not already set it to generic.
	*/
	if ((IIlbqcb->ii_lq_error.ii_er_flags & II_E_GENERIC) == 0)
	    IIlbqcb->ii_lq_error.ii_er_flags |= II_E_DBMS;
    }
    if (IIglbcb->iigl_embset.iies_values & II_ES_SQLPRT) 
    { 
        /* Print all ESQL errors to output */
	IIlbqcb->ii_lq_error.ii_er_flags |= II_E_SQLPRT;
    }
    if (IIglbcb->iigl_embset.iies_values & II_ES_EVTDSP)
    {
	/* Print event information as events return from dbms */
	IIlbqcb->ii_lq_event.ii_ev_flags |= II_EV_DISP;
    }
    if (IIglbcb->iigl_embset.iies_values & II_ES_PFETCH)
    {
	/* Save number of rows to prefetch */
	IIlbqcb->ii_lq_csr.css_pfrows = IIglbcb->iigl_embset.iies_prfarg;
    }
}

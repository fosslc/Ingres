/*
** Copyright (c) 2004, 2007 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<er.h>
# include	<st.h>
# include	<cm.h>
# include	<me.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<fe.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<eqrun.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iicsrun.c	- Runtime cursor support routines
**
**  Description:
**	The routines in this file manage, for each open cursor, the 
**	cursor "state".  The state is a dynamically allocated structure
**	(kept in a linked list of structures for all open cursors) It
**	holds the cursor-id (both the preprocessor-generated id and the 
**	DBMS id) and an array of descriptors of the result columns for
**	this cursor.  The current cursor state is remembered in cursor
**	statements spanning more than one call into this module, i.e.,
**	RETRIEVE cursor and REPLACE cursor.
**	These routines are called directly from an embedded host language 
**	program to execute cursor statements.  They call the LIBQ routines
**	which will send the cursor query to the DBMS.  With the exception 
**	of the OPEN cursor statement, no statement is sent to the DBMS unless 
**	the cursor state exists.
**	The local routines (prefixed with IIcs_) directly manipulate
**	the list of cursor states.
**
**  Defines:
**	IIcsOpen(cursor_name,num1,num2)		- Send define cursor to DBMS
**	IIcsQuery(cursor_name,num1,num2)	- Open cursor state
**	IIcsRetrieve(cursor_name,num1,num2)	- Set up to RETRIEVE cursor
**	IIcsGetio(indaddr,isvar,type,length,address) - Get result column
**	IIcsERetrieve()				- End RETRIEVE cursor
**	IIcsParget(target_list,var_addr,transfunc) - PARAM entry to IIcsGetio
**	IIcsReplace(cursor_name,num1,num2)	- Set up REPLACE cursor
**	IIcsERplace(cursor_name,num1,num2)	- End REPLACE cursor
**	IIcsDelete(tb_nm,cursor_name,num1,num2)	- Execute a DELETE cursor
**	IIcsClose(cursor_name,num1,num2)	- Execute a CLOSE cursor
**	IIcsAllFree()				- Free all cursor memory
**	IIcsRdO(what, rdo)			- Save/send readonly state
**	IILQcgdCursorGetDesc			- Get # of column in descriptor.
**	IILQcgnCursorGetName			- Get name of column.
**	IILQcsgCursorSkipGet			- Skip a column in a fetch.
**	IILQcikCursorIsKnown			- Does LIBQ know this cursor?
**    Locals:
**	IIcs_MakeState				- Allocate a new cursor state
**	IIcs_GetState				- Find a cursor state
**	IIcs_FreeState				- Free a cursor state
-*
**  Notes:
**	1.  REPEAT cursor statements are defined through the same
**	LIBQ routine as non-cursor statements (i.e. IIexDefine).  They
**	are executed through IIexExec (LIBQ) and IIcsQuery.  Note that 
**	REPEAT appears on the DECLARE CURSOR statement only, but becomes 
**	relevant in the OPEN CURSOR statement.  The REPEAT query code in
**	the preprocessor and run-time routines do work, and have been 
**	turned off (via a flag in the preprocessor/undocumented) due to
**	lack off support in QEF.
**
**  History:
**	23-sep-1986 	- written (bjb)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	18-feb-1988	- Added IIcsAllFree() (marge)
**	22-jun-1988	- Modified to use IIDB_QRY_ID which is a GCA_ID, as
**			  DB_CURSOR_ID may be too small. (ncg)
**	12-dec-1988	- Modified error handling for generic errors.  Also
**			  removed check that tries to free already closed
**			  cursors (this can get complicated when figuring
**			  in SQL and QUEL vs INGRES and gateways. Cursors
**			  are now automatically freed via IIxact. (ncg)
**	27-jan-1989	- Allow dynamic naming of cursors through zero-ids.
**			  Cursor names in variables don't have unique ids.
**			  All calls to IIcs_GetState accept cursor_name. (ncg)
**	28-sep-1989	- We now save the FE cursor name as well as the DBMS
**			  cursor name.  They may not be the same (GW's) and the
**			  names must be mapped for dynamic cursor naming. (neil)
**	14-mar-1990	(barbara)
**	    Do a case insensitive comparison on dynamic cursor names to
**	    preserve the INGRES case-insensitive semantics. (Bug #8655)
**	30-apr-1990	(neil)
**	    Cursor performance: Allocate, use and free the alternate read
**	    descriptor associated with a read-only cursor.
**	13-sep-1990  (Joe)
**	    Took out error for too few columns fetched from IIcsERetrieve.
**          Added new routines for use by Windows 4GL cursors.
**	13-sep-1990 (Joe)
**	    Added routine IIITcikCursorIsKnown for use by Windows4GL cursor
**	    objects.
**	04-oct-1990 (barbara)
**	    Don't change input cursor ids on IIcsOpen even when passed by
**	    value.  Some VMS compilers, e.g. COBOL, access violate when a
**	    literal argument is changed.   Problem seems to be that COBOL
**	    passes address of restricted area (LIB$K_VM_FIXED) instead of a
**	    stack frame ptr when "BY VALUE <literal>" args are involved.
**	    Also cleaned up a couple of lint errors (neil did you notice this?)
**	03-dec-1990 (barbara)
**	    Moved state (for INQUIRE_INGRES) from GLB_CB to LBQ_CB (bug 9160)
**	03-jul-1991 (wong)  Corrected double free of cursor state in
**		'IIcsClose()'.  Bug #37537.
**	24-mar-92 (leighb) DeskTop Porting Change:
**	    added conditional function pointer prototype.
**	16-Nov-95 (gordy)
**	    Added support for GCA1_FETCH message.
**	11-apr-1996 (pchang)
**	    If pre-fetching rows and thus, reading data from an alternate stream
**	    in IIcsERetrieve(), check for the presence of the GCA_REMNULS_MASK 
**	    in the saved response block in the alternate stream and copy this 
**	    to the response block in the client descriptor area so that the
**	    correct warning is issued in the SQLSTATE value 01003 (B75630). 
**	18-march-1997(angusm)
**	    Cross-integrate fix for bug 74554 from ingres63p
**      23-may-96 (loera01)
**          Added support for compressed variable-length datatypes in IIcsQuery()
**          and IIcsGetio().
** 	18-dec-1996 - Bug 79612:  In IIcsGetio(), set IIgetdata() flags argument 
**	    from IICS_STATE block, rather than LBQ_CB.  The rd_flags member of the 
**	    LBQ_CB block was getting overwritten when a cursor fetch loop contained 
**	    an embedded select statement. (loera01) 
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-Mar-07 (gordy)
**	    Scrollable cursors are at GCA protocol level 67.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/* 
** Local utility routines to manage cursor states
*/
static IICS_STATE	*IIcs_GetState( II_LBQ_CB *, char *, i4, i4  );
static void		IIcs_FreeState( II_LBQ_CB *, IICS_STATE * );
static IICS_STATE	*IIcs_MakeState( II_LBQ_CB * );
static STATUS		cursor_skip( II_LBQ_CB * );

/*{
+*  Name: IIcsOpen	- Send OPEN cursor (really define/open cursor) to BE.
**
**  Description:
**	This routine is called just before the text of the query
**	associated with the new cursor is sent to the DBMS.  It
**	assumes that it will be followed by a IIwritedb call 
**	(transmitting the query text) and an IIcsQuery call.
**	Thus IIcsOpen and IIcsQuery are atomic.
**	Note that IIexExec (not in this file) and IIcsQuery are also 
**	atomic.
**
**	If the ids are zero (dynamic cursor name) then we first try to find
**	the cursor name (someone may really be reopening an open cursor and
**	we don't want to give it to them for nothing).  If we don't find it
**	already open, then the ids are both set to the next dynamic highwater.
**
**  Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor.  If these numbers
**			  are zero then this is a dynamically named cursor, and
**			  we use the current dynamic id (css_dynmax).
**
**  Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
**
**  Preprocessor Generated calls:
**  - Regular syntax
**	EXEC SQL OPEN cursor_name
**
**	IIsqInit(&sqlca);
**	IIcsOpen( cursor_name, id, id );
**	IIwritedb( <query_text> );
**	IIcsQuery( cursor_name, id, id );
**
**  - Dynamic syntax
**	EXEC SQL OPEN cursor_name using :var
**
**	IIsqInit(&sqlca);
**	IIcsOpen( cursor_name, id, id );
**	IIwritedb( statement_name );
**	IIsqExStmt( (char *)0, 1 );
**	IIputdomio( &nullind, isvar, type, len, &var );
**	IIcsQuery( cursor_name, id, id );
**
**  Query sent to DBMS:
**	A GCA_QUERY block is sent containing the text 
**	    "open cursor <qry-id> for <user-defined-query>"
**	    The cursor-id is sent as data.
**
**  Side Effects:
**	None.
-*	
**  History:
**	23-sep-1986 - Written (bjb)
**	10-aug-1987 - Modified to use GCA. (ncg)
**	27-jan-1989 - Modified to allow dynamic cursor names. (ncg)
**	04-oct-1990 (barbara)
**	    Don't change input cursor ids on IIcsOpen even when passed by
**	    value.  Some VMS compilers, e.g. COBOL, access violate when a
**	    literal argument is changed.   Problem seems to be that COBOL
**	    passes address of restricted area (LIB$K_VM_FIXED) instead of a
**	    stack frame ptr when "BY VALUE <literal>" args are involved.
*/

void
IIcsOpen( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbv;
    IIDB_QRY_ID		dbcsr;
    IICS_STATE		*dyn_csr;	/* Existing dynamic cursor */
    char		*qrystr;
    i4			l_num1 = num1;
    i4			l_num2 = num2;  	/* See History comment */

    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ? ERx("open cursor ") : ERx("open ");

    if (IIqry_start(GCA_QUERY, 0, 0, qrystr) != OK)
	return;

    STmove(cursor_name, ' ', GCA_MAXNAME, dbcsr.gca_name);
    if (l_num1 == 0 && l_num2 == 0)			/* Dynamic - new or old? */
    {
	dyn_csr = IIcs_GetState( IIlbqcb, cursor_name, l_num1, l_num2);
	if (dyn_csr == (IICS_STATE *)NULL)	/* Dynamic and new */
	{
	    l_num1 = --IIlbqcb->ii_lq_csr.css_dynmax;
	    l_num2 = l_num1;
	}
	else					/* Dynamic and exists! */
	{
	    /*
	    ** Already found - this may be a valid attempt to open one that
	    ** we did not know was closed, or an invalid attempt to reopen
	    ** an open cursor. Return original front-end ids.
	    */
	    l_num1 = dyn_csr->css_fename.gca_index[0];
	    l_num2 = dyn_csr->css_fename.gca_index[1];
	}
    }

    /* Send "open" using local DB_DATA_VALUE */
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(qrystr), qrystr);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );

    dbcsr.gca_index[0] = (i4)l_num1;
    dbcsr.gca_index[1] = (i4)l_num2;
    IIqid_send( IIlbqcb, &dbcsr );		/* Send cursor_id */

    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ? ERx(" for ") : ERx(" cursor for ");
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(qrystr), qrystr);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
}

/*{
+*  Name: IIcsOpenScroll	- Send OPEN cursor to BE for scrollable cursor.
**
**  Description:
**	This routine is called just before the text of the query
**	associated with the new cursor is sent to the DBMS.  It
**	assumes that it will be followed by a IIwritedb call 
**	(transmitting the query text) and an IIcsQuery call.
**	Thus IIcsOpen and IIcsQuery are atomic.
**	Note that IIexExec (not in this file) and IIcsQuery are also 
**	atomic.
**
**	If the ids are zero (dynamic cursor name) then we first try to find
**	the cursor name (someone may really be reopening an open cursor and
**	we don't want to give it to them for nothing).  If we don't find it
**	already open, then the ids are both set to the next dynamic highwater.
**
**	This function was simply cloned from IIcsOpen to support the additional
**	parameter containing the scroll options. Everything else is the same,
**	although Quel bits have been removed.
**
**  Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor.  If these numbers
**			  are zero then this is a dynamically named cursor, and
**			  we use the current dynamic id (css_dynmax).
**	scroll_opts	- Either "scroll" or "keyset scroll".
**
**  Outputs:
**	Returns:
**		Nothing.
**	Errors:
**		None.
**
**  Preprocessor Generated calls:
**  - Regular syntax
**	EXEC SQL OPEN cursor_name
**
**	IIsqInit(&sqlca);
**	IIcsOpenScroll( cursor_name, id, id, scroll_opts );
**	IIwritedb( <query_text> );
**	IIcsQuery( cursor_name, id, id );
**
**  - Dynamic syntax
**	EXEC SQL OPEN cursor_name using :var
**
**	IIsqInit(&sqlca);
**	IIcsOpenScroll( cursor_name, id, id, scroll_opts );
**	IIwritedb( statement_name );
**	IIsqExStmt( (char *)0, 1 );
**	IIputdomio( &nullind, isvar, type, len, &var );
**	IIcsQuery( cursor_name, id, id );
**
**  Query sent to DBMS:
**	A GCA_QUERY block is sent containing the text 
**	    "open cursor <qry-id> <scroll_opts> for <user-defined-query>"
**	    The cursor-id is sent as data.
**
**  Side Effects:
**	None.
-*	
**  History:
**	12-feb-2007 (dougi)
**	    Cloned from IIcsOpen for scrollable cursors.
*/

void
IIcsOpenScroll( cursor_name, num1, num2, scroll_opts )
char	*cursor_name;
i4	num1, num2;
char	*scroll_opts;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbv;
    IIDB_QRY_ID		dbcsr;
    IICS_STATE		*dyn_csr;	/* Existing dynamic cursor */
    char		*qrystr;
    i4			l_num1 = num1;
    i4			l_num2 = num2;  	/* See History comment */

    qrystr = ERx("open ");

    if (IIqry_start(GCA_QUERY, 0, 0, qrystr) != OK)
	return;

    STmove(cursor_name, ' ', GCA_MAXNAME, dbcsr.gca_name);
    if (l_num1 == 0 && l_num2 == 0)		/* Dynamic - new or old? */
    {
	dyn_csr = IIcs_GetState( IIlbqcb, cursor_name, l_num1, l_num2);
	if (dyn_csr == (IICS_STATE *)NULL)	/* Dynamic and new */
	{
	    l_num1 = --IIlbqcb->ii_lq_csr.css_dynmax;
	    l_num2 = l_num1;
	}
	else					/* Dynamic and exists! */
	{
	    /*
	    ** Already found - this may be a valid attempt to open one that
	    ** we did not know was closed, or an invalid attempt to reopen
	    ** an open cursor. Return original front-end ids.
	    */
	    l_num1 = dyn_csr->css_fename.gca_index[0];
	    l_num2 = dyn_csr->css_fename.gca_index[1];
	}
    }

    /* Send "open" using local DB_DATA_VALUE */
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(qrystr), qrystr);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );

    dbcsr.gca_index[0] = (i4)l_num1;
    dbcsr.gca_index[1] = (i4)l_num2;
    IIqid_send( IIlbqcb, &dbcsr );		/* Send cursor_id */

    /* Insert scroll options, then "cursor for". */
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(scroll_opts), scroll_opts);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
    qrystr = ERx(" cursor for ");
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(qrystr), qrystr);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
}

/*{
+*  Name: IIcsQuery 	- Create cursor state and fill in descriptor
**
**  Description:
**	This routine is called just after the text of the query
**	associated with the cursor has been sent to the DBMS.  (For
**	details see notes for IIcsOpen.)  IIcsQuery is always called 
**	directly following calls to IIcsOpen and IIwritedb.  
**
**	It ascertains whether or not the DBMS processed the query 
**	successfully.   If so, this routine reads the DBMS-generated
**	cursor_id and allocates a cursor state with a number of column 
**	descriptors (number of cols is obtained from the DBMS).  (It first 
**	frees the existing cursor state, if any -- this situation can arise 
**	if a cursor was closed indirectly via some severe error.)  Descriptors 
**	are then read from the DBMS into the new cursor state.
**
**	If the BE returns any errors (e.g., that the cursor is already 
**	open), then this routine returns silently, assuming an error has been 
**	printed by lower routines.  (There may or may not be an existing 
**	front end cursor state for the open cursor.  If there isn't,
**	this error will show up when the program tries to fetch.)  If errors 
**	crop up in reading the descriptor from the backend, any existing 
**	cursor state is freed.
**
**	If the BE returns a read-only descriptor then we try to open an
**	alternate stream against which we can issue FETCH statements with
**	improved performance.
**
**  Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSOPEN - Unable to open cursor <cursor_name>
**
**  Side Effects:
**	Unless there's an error, a new cursor state is allocated.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	26-mar-1987 - modified to use new row descriptor info. (ncg)
**	27-jan-1989 - Modified to allow dynamic cursor names. (ncg)
**	28-sep-1989 - Save FE cursor name as well. (ncg)
**	30-apr-1990 - Open alternate stream for read-only cursor (ncg)
**      23-may-1996 - Added support for compressed variable-length datatypes.
**                    (loera01)
*/

void
IIcsQuery( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1;
i4	num2;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE		*statep;
    ROW_DESC		local_row_desc;
    IIDB_QRY_ID		dbcsr;		    /* Holder for DBMS csr_id */
    i4			stat;		    /* Status info */
    i4		gl_flags = IIglbcb->ii_gl_flags;

    /* Send query to DBMS only if not nested in a retrieve loop */
    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
	IIcgc_end_msg(IIlbqcb->ii_lq_gca);

    /* Was there a local error? */
    if (((IIlbqcb->ii_lq_curqry & II_Q_INQRY) == 0) ||
        (IIlbqcb->ii_lq_curqry & II_Q_QERR))
    {
	IIqry_end();			/* Will reset flags anyway */
	return;
    }

    stat = IIqry_read( IIlbqcb, GCA_QC_ID );
    if (stat == FAIL || II_DBERR_MACRO(IIlbqcb))
    {
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ0055_CSOPEN, II_ERR, 1, cursor_name);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	IIqry_end();
	return;			/* End of query: II_Q_DONE will have been set */
    }

    /* No BE errors */
    statep = IIcs_GetState( IIlbqcb, cursor_name, num1, num2 );
    if (statep != NULL)	
    {
	/* BE has opened new cursor; FE thinks it's already open. */
	IIcs_FreeState( IIlbqcb, statep );
    }
    /* Read DBMS cursor_id */
    if (IIqid_read( IIlbqcb, &dbcsr ) == FAIL)
    {
	IIlocerr(GE_LOGICAL_ERROR, E_LQ0055_CSOPEN, II_ERR, 1, cursor_name);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return;
    }

    local_row_desc.rd_numcols = 0;
    local_row_desc.rd_dballoc = 0;
    local_row_desc.rd_nmalloc = 0;
    if (IIrdDescribe(GCA_TDESCR, &local_row_desc) == OK)
    {
	statep = IIcs_MakeState( IIlbqcb );
	if (statep == NULL)		/* Error in allocation */
	{
	    IIlocerr(GE_NO_RESOURCE, E_LQ0057_CSALLOC, II_ERR, 1, cursor_name);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	else
	{
	    /* Assign DBMS cursor id */
	    MEcopy(dbcsr.gca_name, GCA_MAXNAME, statep->css_name.gca_name);
	    statep->css_name.gca_index[0] = dbcsr.gca_index[0];
	    statep->css_name.gca_index[1] = dbcsr.gca_index[1];
	    /* Save FE cursor id (including name) */
	    STmove(cursor_name, ' ', GCA_MAXNAME, statep->css_fename.gca_name);
	    if (num1 == 0 && num2 == 0)		/* Dynamic? */
	    {
		/* Save front-end ids as sent in IIcsOpen and mark as dynamic */
		statep->css_fename.gca_index[0] = IIlbqcb->ii_lq_csr.css_dynmax;
		statep->css_fename.gca_index[1] = IIlbqcb->ii_lq_csr.css_dynmax;
		statep->css_flags = II_CS_DYNAMIC;
	    }
	    else				/* Static */
	    {
		/* Save front-end ids as passed in from preprocessor */
		statep->css_fename.gca_index[0] = num1;
		statep->css_fename.gca_index[1] = num2;
		statep->css_flags = 0;
	    }
	    /* Assign all row descriptor information */
	    MEcopy((PTR)&local_row_desc, sizeof(local_row_desc),
		   (PTR)&statep->css_cdesc);

	    /* Save default value of 1 row for INQUIRE_INGRES PREFETCHROWS */
	    IIlbqcb->ii_lq_csr.css_pfcgrows = 1;

	}
	/* Is this read-only (& not 1-row)?  Try to open alternate stream */
	statep->css_pfstream = NULL;
	if (   (statep->css_cdesc.rd_flags & RD_READONLY)
	    && (statep->css_cdesc.rd_flags & RD_UNBOUND) == 0
	    && (IIlbqcb->ii_lq_csr.css_pfrows != 1)
	   )
	{
	    /*
	    ** Ignore allocation errors if we cannot open a stream.  If this
	    ** is the case then this cursor will be reduced to the old 1-row
	    ** cursor-FETCH type.
	    */
	    stat = IIcga1_open_stream(
			IIlbqcb->ii_lq_gca,
			(gl_flags & II_G_CSRTRC) ? IICGC_ATRACE : 0,
			cursor_name,
			statep->css_cdesc.rd_gca->rd_gc_tsize,
			IIlbqcb->ii_lq_csr.css_pfrows,
			&statep->css_pfstream);
	    if ( gl_flags & II_G_CSRTRC )
	    {
		SIprintf(ERx("PRTCSR  - Open %s: stream open %s\n"),
			 cursor_name,
			 stat == OK ? ERx("ok") : ERx("failed (use 1-row)"));
		SIflush(stdout);
	    }
	    /* If CGA figured that just 1 row can be pre-fetched then toss it */
	    if (stat == OK)
	    {
		if ((IIlbqcb->ii_lq_csr.css_pfcgrows =
		    statep->css_pfstream->cgc_arows) == 1)
		{
		    if ( gl_flags & II_G_CSRTRC )
		    {
			SIprintf(ERx("PRTCSR  - Open: close 1-row stream\n"));
			SIflush(stdout);
		    }
		    IIcga2_close_stream(IIlbqcb->ii_lq_gca,
			statep->css_pfstream);
		    statep->css_pfstream = NULL;
		}
	    }
	}
	else  if ( gl_flags & II_G_CSRTRC )
	{
	    SIprintf(ERx("PRTCSR  - Open %s: no pre-fetch stream applied\n"),
		     cursor_name);
	    SIflush(stdout);
	} /* If read only */
    }
    else
    {
	IIlocerr(GE_LOGICAL_ERROR, E_LQ0056_CSDESC, II_ERR, 1, cursor_name);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
    }
    /* 
    ** Whether or not an error occurred, flush to end
    */
    IIqry_end();
}
    

/*{
+*  Name: IIcsRetrieve 	- Set up to retrieve data from a cursor
**
**  Description:
**	This routine is simply a stub that calls IIcsRetScroll to
**	perform the actual FETCH call. It was necessitated by the 
**	need for 2 extra parameters in support of the fetch orientation
**	options of FETCH on a scrollable cursor.
**
**	It handles FETCH statements with no accompanying orientation
**	which are compiled in the old manner.
**
**  Inputs:
**	cursor_name	- User defined cursor name.
**	num1, num2	- Unique integers identifying cursor.
**
**  Outputs:
**	Returns:
**		0	- Cursor was not found or beyond last row
**		1	- Cursor was found
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <FETCH/RETRIEVE>
**
**  Preprocessor Generated calls:
**  - Regular syntax
**	EXEC SQL FETCH cursor_name INTO :var1, :var2;
**
**	IIsqInit(&sqlca);
**	if (IIcsRetrieve( cursor_name, id, id ) != 0);
**	{
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    - If dynamic SQL above calls are replaced by IIcsDaGet
**	    IIcsERetrieve();
**	}
**
**  Query sent to DBMS:
**	No query text is sent.  A GCA_FETCH/GCA1_FETCH block is sent 
**	containing the cursor-id (and possibly a fetch count).
**
**  Side Effects:
**	css_cur in IIlbqcb struct is set for the following IIcsGetio calls.
-*	
**  History:
**	13-feb-2007 (dougi)
**	    Written for scrollable cursors (replaces old IIcsRetrieve()).
*/
i4
IIcsRetrieve( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1, num2;
{

    /* Pass call through to IIcsRetScroll() with dummy args. */
    return(IIcsRetScroll( cursor_name, num1, num2, -1, -1 ));
}



/*{
+*  Name: IIcsRetScroll 	- Set up to retrieve data from a cursor
**
**  Description:
**	This function is the old IIcsRetrieve(), complete with comments
**	and history, re-named and with additional logic to process FETCH 
**	requests with orientation (for scrollable cursors).
**
**	This routine looks for a cursor state identified by the caller.  
**	If it does not find one, it reports an error as it would not be 
**	able to interpret the resulting data.  It returns failure to the 
**	caller.
**
**	Normal Processing:
**
**	If the cursor is open, a "FETCH/RETRIEVE cursor_id" block is
**	issued.  (Note that the query text was sent with the
**	OPEN cursor command.)  If the BE returns no errors, then the 
**	data returning should be exactly described by the descriptor
**	in the saved cursor state.  Success is returned to the caller.
**	The data will be copied into user variables with each following 
**	call to IIcsGetio.
**
**	If the BE returns the status that it is beyond the last row, the
**	global flag is set and failure is returned.	
**
**	Pre-Fetch Processing:
**
**	If the cursor has an alternate pre-fetch buffer stream then data
**	may be available without calling the BE.  If the stream actually
**	has data pooled then we turn on alternate stream processing to allow
**	LIBQGCA to detour the data retrieval.  If the pool is empty then
**	before we decide to load up more data we double-check any previously
**	retrieved response status to make sure we didn't reach the end of
**	the BE result table on a previous load-stream operation.
**
**	Protocol Note for 6.3 and above:
**
**	Prior to 6.4 version of messages we have to send the pre-fetch row
**	count w/o extending the message data.  The row count was encoded at
**	the tail end of the cursor name (in the cursor_id).  In the future we
**	must check:
**		if (cgc->protocol > 6.3_before_versioning)
**		    encode row count in gca_name;
**		else if (cgc->protocol > 6.3_after_versioning)
**		    add row count to message;
**		endif;
**
**	Scrollable Cursor Processing:
**
**	Scrollable cursors can be accessed with FETCHes with orientations.
**	The orientation is transformed into an anchor/offset pair, indicating
**	the desired row in the result set. The extra fields are passed to
**	the BE as a GCA2_FETCH message.
**
**  Inputs:
**	cursor_name	- User defined cursor name.
**	num1, num2	- Unique integers identifying cursor.
**	fetcho		- Fetch orientation (FIRST, LAST, NEXT, PRIOR,
**			ABSOLUTE, RELATIVE).
**	fetchn		- Fetch constant (used with ABSOLUTE, RELATIVE).
**
**  Outputs:
**	Returns:
**		0	- Cursor was not found or beyond last row
**		1	- Cursor was found
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <FETCH/RETRIEVE>
**
**  Preprocessor Generated calls:
**  - Regular syntax
**	EXEC SQL FETCH [<fetch orientation>] cursor_name INTO :var1, :var2;
**
**	IIsqInit(&sqlca);
**	if (IIcsRetScroll( cursor_name, id, id, n, n ) != 0);
**	{
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    - If dynamic SQL above calls are replaced by IIcsDaGet
**	    IIcsERetrieve();
**	}
**
**  Query sent to DBMS:
**	No query text is sent.  A GCA_FETCH/GCA1_FETCH/GCA2_FETCH block 
**	is sent containing the cursor-id and optionally the row count and
**	fetch orientation options.
**
**  Side Effects:
**	css_cur in IIlbqcb struct is set for the following IIcsGetio calls.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	10-aug-1987 - Modified to use GCA. (ncg)
**	24-may-1988 - Sets tuple reading descriptor for INGRES/NET. (ncg)
**	12-dec-1988 - No longer tries to detect DBMS-closed cursor. (ncg)
**	30-apr-1990 - Cursor performance: Support pre-fetch. (ncg)
**	21-may-1990 - Call IIqry_start even at end of pre-fetched cursor. (ncg)
**	24-may-1993 - New protocol requires new message for DELETE.
**	16-Nov-95 (gordy)
**	    Added support for GCA1_FETCH message.
**	18-march-1997(angusm)
**	    Cross-integrate fix for bug 74554 from ingres63p
**	13-feb-2007 (dougi)
**	    Transformed to IIcsRetScroll() from IIcsRetrieve() to
**	    support fetch orientation options of scrollable cursors.
**	15-Mar-07 (gordy)
**	    Scrollable cursors are at GCA protocol level 67.
**	5-july-2007 (dougi)
**	    Tidy (& re-enable) pre-fetch for non-scrollable cursors.
**	10-july-2007 (dougi)
**	    Small change to above to prevent endless looping on fetch.
**	08-mar-2008 (toumi01) BUG 119457
**	    Disambiguate fetcho == 0; for non-scrollable cursors fetcho
**	    and fetchn will be -1 for both direct calls and calls through
**	    the IIcsRetrieve stub function.
*/
i4
IIcsRetScroll( cursor_name, num1, num2, fetcho, fetchn )
char	*cursor_name;
i4	num1, num2;
i4	fetcho, fetchn;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    i4		stat;		
    char	*qrystr;
    IICGC_ALTRD	*pfstream;	/* For pre-fetched cursors */
    IIDB_QRY_ID	ext_id;	    	/* Extended cursor id (for 6.3) */
    IIDB_QRY_ID	*send_id;	/* Current cursor id to send */
    i4		pfrows;		/* How many rows to ask for */
    char	pfrowbuf[20];	/* For row count conversions */
    i4	gl_flags = IIglbcb->ii_gl_flags;
    bool	scroll;

    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ?
		ERx("retrieve cursor") : ERx("fetch");

    IIlbqcb->ii_lq_csr.css_cur = IIcs_GetState(IIlbqcb,cursor_name,num1,num2);
    if (IIlbqcb->ii_lq_csr.css_cur == NULL)    /* State needed to read data */
    {
	IIlocerr(GE_CURSOR_UNKNOWN, E_LQ0058_CSNOTOPEN, II_ERR, 2,
		 cursor_name, qrystr);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return 0;
    }
    IIlbqcb->ii_lq_csr.css_cur->css_colcount = 0;    	/* No cols yet */
    if (fetcho < 0 && fetchn < 0)
	scroll = FALSE;
    else scroll = TRUE;

    if ( gl_flags & II_G_CSRTRC )
    {
	SIprintf(ERx("PRTCSR  - Fetch %s: fetcho: %d, fetchn:%d\n"), 
		     cursor_name, fetcho, fetchn);
	SIprintf(ERx("PRTCSR  - protocol level: %d\n"),
		 	 IIlbqcb->ii_lq_gca->cgc_version);
    }
    pfstream = IIlbqcb->ii_lq_csr.css_cur->css_pfstream;
    if (pfstream != NULL)				/* Pre-fetched? */
    {
	if (pfstream->cgc_amsgq != NULL && !scroll)	/* Unread data? */
	{
	    /* Confirm the context of the request */
	    if (IIqry_start(0 /* = No GCA access */, 0, 0, qrystr) != OK)
	    {
		IIlbqcb->ii_lq_csr.css_cur = NULL;
		return 0;
	    }
	    /* Set up for alternate read operation */
	    pfstream->cgc_aflags |= IICGC_AREADING;
	    IIlbqcb->ii_lq_gca->cgc_alternate = pfstream;
	    if ( gl_flags & II_G_CSRTRC )
		SIprintf(ERx("PRTCSR  - Fetch %s: use stream\n"), cursor_name);
	    return 1;
	}
	/*
	** The pre-fetch message queue is empty.  Do we need to reload it
	** or was the last load operation the end of the result table?
	** Also, if this is a non-NEXT FETCH on a scrollable cursor, we
	** must call BE to reposition.
	*/
	if (   pfstream->cgc_aresp.gca_rowcount == 0
	    || ((pfstream->cgc_aresp.gca_rqstatus & GCA_END_QUERY_MASK) &&
		!scroll)
	   )
	{
	    IIlbqcb->ii_lq_csr.css_cur = NULL;

	    /* Confirm the context of the request */
	    if (IIqry_start(0 /* = No GCA access */, 0, 0, qrystr) != OK)
		return 0;

	    /*
	    ** A pre-fetched response indicates we're done.  Copy it over
	    ** and have IIqry_end process it as though it just occurred.
	    */
	    if ( gl_flags & II_G_CSRTRC )
	    {
	        SIprintf(ERx("PRTCSR  - Fetch %s (end): resp-rows = %d, "),
			 cursor_name, pfstream->cgc_aresp.gca_rowcount); 
	        SIprintf(ERx("end-of-cursor = Y\n"));
		SIflush(stdout);
	    }
	    pfstream->cgc_aresp.gca_rowcount = 0;	/* No rows */
	    STRUCT_ASSIGN_MACRO(pfstream->cgc_aresp,
				IIlbqcb->ii_lq_gca->cgc_resp);
	    IIlbqcb->ii_lq_curqry |= II_Q_PFRESP;	/* Use pre-f'd resp */
	    IIqry_end();
	    return 0;
	}
	/*
	** The pre-fetch queue is empty, and we are not at the end of the
	** result data set.  Load in as many rows as we can.
	*/
	pfrows = pfstream->cgc_arows;
	if ( gl_flags & II_G_CSRTRC )
	{
	    SIprintf(ERx("PRTCSR  - Fetch %s: load stream with %d rows\n"), 
		     cursor_name, pfrows);
	}
    }
    else
    {
	/* This is either a non-pre-fetchable cursor or DBMS can't handle */ 
	pfrows = 1;
    }
    IIlbqcb->ii_lq_gca->cgc_alternate = NULL;		/* No pre-fetching */

    if ( pfrows > 1  && 
	 IIlbqcb->ii_lq_gca->cgc_version < GCA_PROTOCOL_LEVEL_2 )
	pfrows = 1;

    if ( fetcho >= 0 &&
	 IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_67 )
    {
	if ( IIqry_start( GCA2_FETCH, 0, 0, qrystr ) != OK )
	{
	    IIlbqcb->ii_lq_csr.css_cur = NULL;
	    return( 0 );
	}

	/* Compute values for gca_anchor, gca_offset & gca_rowcount. */
	if (pfrows <= 0 || scroll)
	    pfrows = 1;		/* disable prefetch for scrollable cursors */

	switch(fetcho) {
	  case fetFIRST:
	    fetchn = 1;
	    fetcho = GCA_ANCHOR_BEGIN;
	    break;
	  case fetLAST:
	    fetchn = -1;
	    fetcho = GCA_ANCHOR_END;
	    break;
	  case fetNEXT:
	    fetchn = 1;
	    fetcho = GCA_ANCHOR_CURRENT;
	    break;
	  case fetPRIOR:
	    fetchn = -1;
	    fetcho = GCA_ANCHOR_CURRENT;
	    break;
	  case fetRELATIVE:
	    fetcho = GCA_ANCHOR_CURRENT;
	    break;
	  case fetABSOLUTE:
	    if (fetchn >= 0)
		fetcho = GCA_ANCHOR_BEGIN;
	    else fetcho = GCA_ANCHOR_END;
	    break;
	}
    }
    else if ( pfrows > 1  &&
	 IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_62 )
    {
	if ( IIqry_start( GCA1_FETCH, 0, 0, qrystr ) != OK )
	{
	    IIlbqcb->ii_lq_csr.css_cur = NULL;
	    return( 0 );
	}
    }
    else
    {
	if ( IIqry_start( GCA_FETCH, 0, 0, qrystr ) != OK )
	{
	    IIlbqcb->ii_lq_csr.css_cur = NULL;
	    return( 0 );
	}
    }

    send_id = &IIlbqcb->ii_lq_csr.css_cur->css_name;

    if ( pfrows > 1  &&
    	 IIlbqcb->ii_lq_gca->cgc_version < GCA_PROTOCOL_LEVEL_62 )
    {
	/* Special encoded form of row-request under old FETCH data */
	if ( gl_flags & II_G_CSRTRC )
	    SIprintf(ERx("PRTCSR  - Fetch: encode row count (%d)\n"), pfrows);
	ext_id.gca_index[0] = send_id->gca_index[0];
	ext_id.gca_index[1] = send_id->gca_index[1];
	MEcopy(send_id->gca_name, DB_GW1_MAXNAME_32, ext_id.gca_name);
	CVna(pfrows, pfrowbuf);
	STmove(pfrowbuf, ' ', GCA_MAXNAME - DB_GW1_MAXNAME_32, 
	       &ext_id.gca_name[DB_GW1_MAXNAME_32]);
	send_id = &ext_id;
    }
    IIqid_send( IIlbqcb, send_id );

    if ( fetcho >= 0 &&
	 IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_67 )
    {
	DB_DATA_VALUE intdbv;

	/* Add formal row count */
	if ( gl_flags & II_G_CSRTRC )
	    SIprintf(ERx("PRTCSR  - Fetch: sending row count (%d)\n"), pfrows);
	II_DB_SCAL_MACRO(intdbv, DB_INT_TYPE, sizeof(pfrows), &pfrows);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &intdbv);

	/* Write "anchor" value to message. */
	if ( gl_flags & II_G_CSRTRC )
	    SIprintf(ERx("PRTCSR  - Fetch: sending fetch orientation (%d)\n"), 
					fetcho);
	II_DB_SCAL_MACRO(intdbv, DB_INT_TYPE, sizeof(fetcho), &fetcho);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &intdbv);

	/* Write "offset" value to message. */
	if ( gl_flags & II_G_CSRTRC )
	    SIprintf(ERx("PRTCSR  - Fetch: sending fetch offset (%d)\n"), 
					fetchn);
	II_DB_SCAL_MACRO(intdbv, DB_INT_TYPE, sizeof(fetchn), &fetchn);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &intdbv);
    }
    else if ( pfrows > 1  &&
	 IIlbqcb->ii_lq_gca->cgc_version >= GCA_PROTOCOL_LEVEL_62 )
    {
	DB_DATA_VALUE pfdbv;

	/* Add formal row count */
	if ( gl_flags & II_G_CSRTRC )
	    SIprintf(ERx("PRTCSR  - Fetch: sending row count (%d)\n"), pfrows);
	II_DB_SCAL_MACRO(pfdbv, DB_INT_TYPE, sizeof(pfrows), &pfrows);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VVAL, &pfdbv);
    }

    IIcgc_end_msg(IIlbqcb->ii_lq_gca);	   /* Unconditionally send */

    /* Was there a local error? */
    if (((IIlbqcb->ii_lq_curqry & II_Q_INQRY) == 0) ||
        (IIlbqcb->ii_lq_curqry & II_Q_QERR))
    {
	IIqry_end();			/* Will reset flags anyway */
	return 0;
    }

    /* Pre-read for following reads via IIcsGetio */
    stat = IIqry_read( IIlbqcb, GCA_TUPLES );

    /* 
    ** If an error occurred during the setup for cursor RETRIEVE 
    ** return, as subsequent FETCHES will not work. 
    */
    if (stat == FAIL || II_DBERR_MACRO(IIlbqcb))
    {
	/* 
	** Report error only if not reported already.
	** However, no more rows is not an error -- this condition
	** can be reported via INQUIRE_EQUEL.
	*/
	if (IIlbqcb->ii_lq_error.ii_er_num == IIerOK &&
	    	(IIlbqcb->ii_lq_qrystat & GCA_END_QUERY_MASK) == 0)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_LQ0059_CSINIT, II_ERR, 2,
		     qrystr, cursor_name );
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	}
	if (IIlbqcb->ii_lq_qrystat & GCA_END_QUERY_MASK)
	    IIlbqcb->ii_lq_rowcount = 0;
	if (pfstream != NULL)		/* Check some pre-fetch cases */
	{
	    /* We may have encountered a warning prior to data set */
	    if (   (stat == OK)
		&& (IIcgc_more_data(IIlbqcb->ii_lq_gca, GCA_TUPLES) > 0))
	    {
		if ( gl_flags & II_G_CSRTRC )
		    SIprintf(ERx("PRTCSR  - Fetch: skip early warning\n"));
		return 1;
	    }
	    /* Else trace end-of-query */
	    else  if ( gl_flags & II_G_CSRTRC )
	    {
		if (II_DBERR_MACRO(IIlbqcb))
		    SIprintf(ERx("PRTCSR  - Fetch (end): error returned\n"));
		else	/* Must be end of query */
		    SIprintf(ERx("PRTCSR  - Fetch (end): end-of-cursor = Y\n"));
		SIflush(stdout);
	    }
	}
	IIlbqcb->ii_lq_csr.css_cur = NULL;
	IIqry_end();
	return 0;
    }
    return 1;				/* Successfully set up for IIcsGetios */
}

/*{
+*  Name: IIcsGetio 	- Deposit result column into user's variable
**
**  Description:
**	Using the description pointed at by the current cursor,
**	copy the result data into the user's variable.  This
**	routine is only called if IIcsRetrieve returned successfully.
**
**	Before copying make sure that the caller has not exceeded
**	the number of result columns there are (may happen indirectly
**	via a PARAM target list).  If any errors occur during copying
**	(data type inconsistencies), flag the condition.
**
**  Inputs:
**	indaddr		- Address of null indicator
**	isvar		- Should always be 1 (if generated by preprocessor)
**	type		- Identifies generated type
**	length		- Length of data type.
**	address		- Variable address.
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSXCOLS - Fetching beyond last column of cursor.
**
**  Side Effects:
**	css_cur->css_colcount in IIlbqcb struct is incremented for the 
**	next call.
-*	
**  History:
**	22-sep-1986 - written (bjb)
**	01-apr-1993 - (kathryn)
**	    Add support for Large Objects.
**      23-may-1996 - Added support for compressed variable-length datatypes.
**          (loera01)
** 	18-dec-1996 - Bug 79612:  Set IIgetdata() flags argument from IICS_STATE 
**          block, rather than LBQ_CB. The rd_flags member of the LBQ_CB 
** 	    block was getting overwritten when a cursor fetch loop 
**	    contained an embedded select statement. (loera01)
*/

/*VARARGS4*/
void
IIcsGetio( indaddr, isvar, type, length, address )
i2	*indaddr;
i4	isvar, type, length;
char	*address;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_EMBEDDED_DATA	ed;
    DB_DATA_VALUE	*dbv;	/* Will point to cursor state descriptor */
    IICS_STATE		*csr_cur;
    char 		csrname[ GCA_MAXNAME +1 ];
    char		*qrystr;
    i4                  flags;


    if (isvar == 0 || IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;
    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ?
		ERx("retrieve cursor") : ERx("fetch");
    csr_cur = IIlbqcb->ii_lq_csr.css_cur;	/* Current csr being fetched */
    /*
    ** Fetching too many cols?  If so, give error first time this
    ** happens.
    */
    if (csr_cur->css_colcount >= csr_cur->css_cdesc.rd_numcols)
    {
	STlcopy( csr_cur->css_name.gca_name, csrname, GCA_MAXNAME );
	STtrmwhite( csrname );
	IIlocerr(GE_CARDINALITY, E_LQ005B_CSXCOLS, II_ERR, 2, qrystr, csrname);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	if (IISQ_SQLCA_MACRO(IIlbqcb))
	    IIsqWarn( thr_cb, 3 );		/* Status info for SQLCA */
	if (indaddr)				/* Reset null status */
	    *indaddr = 0;
	return;
    }

    /* Get the descriptor */
    dbv = &csr_cur->css_cdesc.RD_DBVS_MACRO(csr_cur->css_colcount); 
    csr_cur->css_colcount++;

    if (IIDB_LONGTYPE_MACRO(dbv))
    {
	IILQlvg_LoValueGet( thr_cb, indaddr, isvar, type, length,
			    (PTR)address, dbv, csr_cur->css_colcount );
    }
    else
    {
	ed.ed_type = type;
	ed.ed_length = length;
	ed.ed_data = (PTR)address;
	ed.ed_null = indaddr;
        flags = (csr_cur->css_cdesc.rd_flags & RD_CVCH) ? IICGC_VVCH : 0;
    	IIgetdata( thr_cb, dbv, &ed, csr_cur->css_colcount,flags );
    }
}


/*{
+*  Name: IIcsERetrieve 	- End the cursor retrieval execution
**
**  Description:
**	This routine should be called after the last column is
**	retrieved via IIcsGetio calls.  If the number of columns
**	retrieved (css_colcount) is not the same as those
**	in the descriptor (css_cdesc.rd_numcols) then an error is issued
**	and the remaining result data from the BE is flushed.
**
**	In the case of processing a pre-fetched cursor there are some
**	difference steps:
**	1. We must always flush to the end of the column/data descriptor
**	   so that we are positioned correctly for the next FETCH.  This
**	   is true regardless if there was an error or not.  This change
**	   can be considered as a general improvement.
**	2. If we were reading from the pre-fetched stream then we must
**	   fabricate response structure for IIqry_end.
**	3. If we are about to load a new data stream then make sure to load
**	   all remaining data before returning to the user.
**
**  Inputs:
**	None
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSCOLS - Too few column retrievals - no longer raised.
**		E_LQ0040_SHORTREAD - Short read during column skipping.
**		E_LQ005D_CSR_PFLOAD - Error pre-fetching rows for cursor.
**
**  Side Effects:
**		Reset any error CS_FETCH_ERR flags (if any were set to
**			suppress data copying).
**		On exit css_cur in the IIlbqcb struct is reset to NULL.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	30-apr-1990 - alternate stream pre-fetch processing. (ncg)
**	13-sep-1990  - Took out error on too few columns.  Now reads
**		       all the unfetched columns. (joe)
**	11-apr-1996 (pchang)
**		If pre-fetching rows and thus, reading data from an alternate 
**		stream, check for the presence of the GCA_REMNULS_MASK in the 
**		saved response block in the alternate stream and copy this to
**		the response block in the client descriptor area so that the
**		correct warning is issued in the SQLSTATE value 01003 (B75630). 
*/

void
IIcsERetrieve()
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE		*csr_cur;
    IICGC_ALTRD		*pfstream;	/* Pre-fetch cursor processing */
    char 		csrname[GCA_MAXNAME+1];	/* For errors */
    char 		rowbuf[20];
    i4		gl_flags = IIglbcb->ii_gl_flags;

    csr_cur = IIlbqcb->ii_lq_csr.css_cur;	/* Current csr being fetched */
    pfstream = csr_cur->css_pfstream;

    /*
    ** Fetched too few cols (skip if no other error or if pre-fetch stream)?
    ** When replaced by IILQcsgCursorSkipGet make sure to allow pre-fetch to
    ** also skip when II_Q_QERR is set.
    */
    if (   (csr_cur->css_colcount < csr_cur->css_cdesc.rd_numcols)
	&& ((IIlbqcb->ii_lq_curqry & II_Q_QERR) == 0 || (pfstream != NULL))
       )
    {
	if ( pfstream  &&  gl_flags & II_G_CSRTRC )
	{
	    SIprintf(ERx("PRTCSR  - Fetch: read %d of %d columns (skip %d)\n"),
		     csr_cur->css_colcount, csr_cur->css_cdesc.rd_numcols,
		     csr_cur->css_cdesc.rd_numcols - csr_cur->css_colcount);
	}

	/* Skip the columns by setting the DBV data to NULL */
	while (csr_cur->css_colcount < csr_cur->css_cdesc.rd_numcols)
	{
	    if (cursor_skip( IIlbqcb ) != OK)
		break;
	}
	if (IISQ_SQLCA_MACRO(IIlbqcb))
	    IIsqWarn( thr_cb, 3 );	/* Status information for SQLCA */
    } /* If too few columns retrieved */

    if (pfstream)			/* Maybe read or load */
    {
	if (   IIlbqcb->ii_lq_gca->cgc_alternate == pfstream
	    && (pfstream->cgc_aflags & IICGC_AREADING)
	   )
	{
	    /*
	    ** Reset pre-fetch read processing and fabricate a response, even
	    ** if there was an error.
	    */
	    if ( gl_flags & II_G_CSRTRC )
	    {
		SIprintf(ERx("PRTCSR  - Fetch (end): read 1 row\n"));
		SIflush(stdout);
	    }
	    pfstream->cgc_aflags &= ~IICGC_AREADING;
	    /* Fabricate response for IIqry_end */
	    IIlbqcb->ii_lq_curqry |= II_Q_PFRESP;
    	    IIlbqcb->ii_lq_gca->cgc_resp.gca_rowcount = 1;
    	    IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus = GCA_OK_MASK;
	}
	else
	{
	    if (pfstream->cgc_arows > 1)	/* Should always be > 1 */
	    {
		if ( gl_flags & II_G_CSRTRC )
		{
		    SIprintf(ERx("PRTCSR  - Fetch (end): load rest of data\n"));
		    SIflush(stdout);
		}
		if (IIcga3_load_stream(IIlbqcb->ii_lq_gca, pfstream) != OK)
		{
		    CVna(pfstream->cgc_arows, rowbuf);
		    MEcopy(csr_cur->css_name.gca_name, GCA_MAXNAME, csrname);
		    csrname[GCA_MAXNAME] = EOS;
		    STtrmwhite(csrname);
		    IIlocerr(GE_COMM_ERROR, E_LQ005D_CSR_PFLOAD, II_ERR,
			     2, rowbuf, csrname);
		    IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus |= GCA_FAIL_MASK;
		}
		else
		{
		    IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus = GCA_OK_MASK;

		    if (pfstream->cgc_aresp.gca_rqstatus & GCA_REMNULS_MASK)
			IIlbqcb->ii_lq_gca->cgc_resp.gca_rqstatus |= 
							    GCA_REMNULS_MASK;
		}
		IIlbqcb->ii_lq_gca->cgc_resp.gca_rowcount = 1;
	    }
	    else  if ( gl_flags & II_G_CSRTRC )
	    {
		SIprintf(ERx("PRTCSR  - Fetch (end): 1-row load ignored\n"));
		SIflush(stdout);
	    }
	} /* If reading or need to load */
	IIlbqcb->ii_lq_gca->cgc_alternate = NULL;
    }

    /* Flush results from backend, getting status */
    IIqry_end();
    IIlbqcb->ii_lq_csr.css_cur = NULL;
}


/*{
+*  Name: IIcsParGet 	- PARAM entry point to IIcsGetio
**
**  Description:
**	This routine is a cover on the sequence of IIcsGetio calls
**	to copy retrieved data into variables.  Each IIcsGetio call
**	may be preceded by a call to IIretind if the PARAM format/
**	target-list indicates the use of host null indicators.
**
**	The target list input string is processed and at each "%"
**	character the variable array element (or a local buffer
**	for non-C languages) is sent as an argument to IIcsGetio.  
**	If "translation function" is set (for non-C languages on many 
**	systems) the transfunc is called to copy the data retrieved 
**	by IIcsGetio into the host variable.  The translation function
**	is also called to copy the indicator information into the host
**	indicator (if any).
**
**	See IIParProc for information on accepted format specifications.
**
**  Inputs:
**	target_list		- Comma separated target list
**	argv			- Array of variable addresses
**	transfunc		- Translation function pointer
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		BADPARAM - Error in format string for target list
**
**  Preprocessor generated calls:
**	if (IIcsRetrieve( cursor_name, id, id ) != 0);
**	{
**	    IIParGet(target_list, varaddress, translation);
**	    IIcsERetrieve();
**	}
**
**  Query sent to DBMS:
**	See notes for IIcsRetrieve.  For each IIParGet, one column
**	of data is got from the DBMS (via IIcsGetio).
**
**  Side Effects:
**	None
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	11-mar-1987 - added null support (bjb)
**	18-jun-1997 (kosma01)
**	   AIX insists that the function prototype actual match
**	   the argument types actually used with the function.
**	   (transfunc)
*/

void
IIcsParGet( target_list, argv, transfunc )
char	*target_list;
char	**argv;
#ifdef	CL_HAS_PROTOS					 
i4	(*transfunc)(i4,i4,PTR,PTR);			 
#else							 
i4	(*transfunc)();
#endif							 
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    register char	*e_st;
    register char	**av;
    i4		hosttype;        
    i4		hostlen, is_ind;
    i2		temp_ind, *ind_ptr;
    i4		stat;			/* Status of parsing target string */
    char	errbuf[20];		/* Error messages from IIParProc() */
    i4		data[DB_GW4_MAXSTRING/sizeof(i4) +1]; 

    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;
    av = argv;
    for (e_st = target_list; *e_st;)
    {
	if (*e_st != '%')
	{
	    CMnext(e_st);
	    continue;
	}
	e_st++;
	is_ind = FALSE;			/* Assume no indicator specification */
	stat = IIParProc( e_st,&hosttype,&hostlen,&is_ind,errbuf,II_PARRET );
	if (stat < 0)
	{
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ001F_BADPARAM, II_ERR, 1, errbuf);
	    IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	    return;
	}
	e_st += stat;			/* Advance pointer in target str */

	ind_ptr = (i2 *)0;		/* Assume is_ind is FALSE */
	if (transfunc)
	{
	    if (is_ind == TRUE)
		ind_ptr = &temp_ind;
	    IIcsGetio( ind_ptr, TRUE, hosttype, hostlen, (char *)data );
	    if (is_ind == TRUE)
		(*transfunc)(DB_INT_TYPE, sizeof(i2), (PTR)ind_ptr, (PTR)av[1]);
	    /* 
	    ** Only translate data if indicator tells us it's non-null 
	    */
	    if (is_ind == FALSE || temp_ind != DB_EMB_NULL)
		(*transfunc)( hosttype, hostlen, (PTR)data, (PTR)(*av++) );
	}
	else
	{
	    if (is_ind == TRUE)
		ind_ptr = (i2 *)av[1];
	    IIcsGetio( ind_ptr, TRUE, hosttype, hostlen, *av++ );
	}
	if (is_ind == TRUE)
	    av++;
    }
}

/*{
+*  Name: IIcsReplace 	- Send a REPLACE CURSOR query to the BE.
**
**  Description:
**	The routine looks for a cursor state identified by the caller.  
**	If it does not find one, it issues an error, sets the global 
**	error status and returns.  If it does find one, it saves it 
**	so that IIEReplace may free it if the DBMS says the cursor is 
**	not open.
**
**  Inputs:
**	cursor_name		- User defined cursor name
**	num1, num2		- Unique integers identifying cursor
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <REPLACE>
**
**  Preprocessor generated code:
**  - EQUEL Syntax
**	## REPLACE CURSOR c (col = val)
**
**	IIcsReplace( cursor-name, id, id );
**	IIwritedb( query-text );
**	IIsetdom( isvar, type, len, addr );
**	IIwritedb( query-text );
**	IIcsERplace( cursor-name, id, id );
**
**  - ESQL Syntax
**	EXEC SQL UPDATE t SET col = val WHERE CURRENT OF c;
**
**	IIsqInit(&sqlca);
**	IIwritedb( query-text );
**	IIsetdom( isvar, type, len, addr );
**	IIwritedb( query-text );
**	IIcsERplace( cursor-name, id, id );
**
**  Query sent to DBMS:
**	For QUEL, a GCA_QUERY block is sent containing the text:
**	    "replace cursor <qry-id> <user-defined-query>" 
**	    The cursor-id and any user-variables are sent as data.
**	For SQL the query text looks like:
**	    "update t <user-defined-query> where current of <qry-id>" 
**	     but is only generated at IIcsERplace.
**	
**  Side Effects:
**	css_cur in the IIlbqcb is set to be the cursor state (if found)
-*	
**  History:
**	22-sep-1986 - written (bjb)
**	10-aug-1987 - Modified to use GCA. (ncg)
*/

void
IIcsReplace( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE 	dbv;
    char		*qrystr = ERx("replace cursor");  /* Always QUEL here */

    IIlbqcb->ii_lq_csr.css_cur = IIcs_GetState(IIlbqcb,cursor_name,num1,num2);
    if (IIlbqcb->ii_lq_csr.css_cur == NULL)
    {
	IIlocerr(GE_CURSOR_UNKNOWN, E_LQ0058_CSNOTOPEN, II_ERR,
		 2, cursor_name, qrystr);
	IIlbqcb->ii_lq_curqry = II_Q_QERR;	/* Clear other bits */
	return;
    }
    if (IIqry_start(GCA_QUERY, 0, 0, qrystr) != OK)
	return;
    II_DB_SCAL_MACRO(dbv, DB_QTXT_TYPE, STlength(qrystr), qrystr);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
    IIqid_send( IIlbqcb, &IIlbqcb->ii_lq_csr.css_cur->css_name );
}


/*{
+*  Name: IIcsERplace		- Close off the REPLACE CURSOR call.
**
**  Description:
**	Get a response to the query from the BE.
**
**  Inputs:
**	cursor_name		- User defined cursor name
**	num1, num2		- Unique integers identifying cursor
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <update>
**
**  Side Effects:
**	css_cur is reset to NULL.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	13-mar-1987 - added arguments in preparation for SQL (ncg)
**	10-aug-1987 - Modified to use GCA. (ncg)
**	12-dec-1988 - No longer tries to detect DBMS-closed cursor. (ncg)
**	15-may-1990 - Or in bit on error to avoid clearing qry state. (ncg)
**	13-sep-1990 (Joe)
**	    If the cursor is known to be closed to LIBQ, then add
**	    a bad where clause so that the DBMS does not do an update.
*/

void
IIcsERplace( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    DB_DATA_VALUE	dbv;

    if (IIlbqcb->ii_lq_curqry & II_Q_INQRY)
    {
	/* SQL ? Then add the WHERE CURRENT OF clause */
	if (IIlbqcb->ii_lq_dml == DB_SQL)
	{
	    IIlbqcb->ii_lq_csr.css_cur = IIcs_GetState( IIlbqcb, cursor_name, 
							num1, num2 );
	    if (IIlbqcb->ii_lq_csr.css_cur == NULL)
	    {
		IIlocerr(GE_CURSOR_UNKNOWN, E_LQ0058_CSNOTOPEN, II_ERR,
			 2, cursor_name, ERx("update"));
		IIlbqcb->ii_lq_curqry |= II_Q_QERR;	/* Or in other bits */
		II_DB_SCAL_MACRO(dbv,DB_QTXT_TYPE,25,
				 ERx(" where cursor closed ... "));
		IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
	    }
	    else
	    {
		II_DB_SCAL_MACRO(dbv,DB_QTXT_TYPE,18,ERx(" where current of "));
		IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VQRY, &dbv );
		IIqid_send( IIlbqcb, &IIlbqcb->ii_lq_csr.css_cur->css_name );
	    }
	}
	IIcgc_end_msg( IIlbqcb->ii_lq_gca );		/* Send query */
    }
    /* Flush response block */
    IIqry_end();
    IIlbqcb->ii_lq_csr.css_cur = NULL;
}


/*{
+*  Name: IIcsDelete 	- Send a DELETE CURSOR query to the BE.
**
**  Description:
**	The routine looks for a cursor state identified by the
**	caller.  If it finds one it sends a "DELETE cursor_id" block 
**	to the DBMS; if not, it prints an error and returns.
**
**  Inputs:
**	table_name	- Table name - may be null if not known.
**	cursor_name	- User defined cursor name.
**	num1, num2	- Unique integers identifying cursor.
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <DELETE>
**
**  Preprocessor generated code:
**	IIcsDelete( table_name, cursor-name, id, id );
**
**  Query sent to DBMS:
**	No query text is sent.  A GCA_DELETE block is sent containing
**	the cursor-id, and if there is a table name, the table name as well.
**
**  Side Effects:
**	None
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	10-aug-1987 - Modified to use GCA. (ncg)
**	08-jun-1988 - Modified for new GCA_DELETE message. (ncg)
**	12-dec-1988 - No longer tries to detect DBMS-closed cursor. (ncg)
**	24-may-1993 (kathryn)
**	    New protocol (60) requires new message GCA1_DELETE to allow
**	    for owner.tablename.
**	10-jun-1993 (lan)
**	    Fixed a bug where an ACCVIO occurred when table_name is NULL.
**	12-jun-1993 (kathryn)
**	    Back out previous change - ACCVIO occurred in IIUGgci_getcid()
**	    and that function has been changed.
*/

void
IIcsDelete( table_name, cursor_name, num1, num2 )
char	*table_name;
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE		*statep;
    DB_DATA_VALUE	dbv;
    FE_GCI_IDENT	owntbl;
    char		*qrystr;
    char		tabbuf[DB_MAXNAME + 1];
    i4			gcatype;

    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ?
		ERx("delete cursor") : ERx("delete");

    statep = IIcs_GetState(IIlbqcb, cursor_name, num1, num2);
    if (statep == NULL)
    {
	IIlocerr(GE_CURSOR_UNKNOWN, E_LQ0058_CSNOTOPEN, II_ERR,
		 2, cursor_name, qrystr);
	IIlbqcb->ii_lq_curqry = II_Q_QERR;	/* Clear other bits */
	IIqry_end();
	return;
    }
    if (IIlbqcb->ii_lq_gca->cgc_version >=  GCA_PROTOCOL_LEVEL_60)
	gcatype = GCA1_DELETE;
    else
	gcatype = GCA_DELETE;
	
    if (IIqry_start(gcatype, 0, 0, qrystr) != OK)
	return;
    IIqid_send( IIlbqcb, &statep->css_name );

    /* GCA_DELETE conatins the cursor id followed by owner then table name */
    if (gcatype == GCA1_DELETE)
    {
	IIUGgci_getcid(table_name, &owntbl, TRUE);
	II_DB_SCAL_MACRO(dbv,DB_CHA_TYPE, STlength(owntbl.owner),owntbl.owner);
	IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv);
	II_DB_SCAL_MACRO(dbv,DB_CHA_TYPE,STlength(owntbl.object),owntbl.object);
    }
     /* GCA_DELETE conatins the cursor id followed by the table name */
    else
    {
        if (table_name)
	    IIstrconv(II_TRIM, table_name, tabbuf, DB_MAXNAME);
    	else
	    *tabbuf = EOS;
        II_DB_SCAL_MACRO(dbv, DB_CHA_TYPE, STlength(tabbuf), tabbuf);
    }
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, FALSE, IICGC_VNAME, &dbv);
    IIcgc_end_msg(IIlbqcb->ii_lq_gca);

    /* Flush response block */
    IIqry_end();
}


/*{
+*  Name: IIcsClose 	- Send a CLOSE CURSOR query to the BE.
**
**  Description:
**	The routine looks for a cursor state identified by the
**	caller.  If it finds one it sends a "CLOSE cursor_id" block 
**	to the DBMS and then frees the cursor state; if not, it prints an 
**	error and returns.
**
**  Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor
**
**  Outputs:
**	Returns:
**		Nothing
**	Errors:
**		CSNOTOPEN - Cursor <cursor_name> not open for <CLOSE>
**
**  Preprocessor generated code:
**	IIcsClose( cursor-name, id, id );
**
**  Query sent to DBMS:
**	No query text is sent.  A GCA_CLOSE block is sent containing
**	the cursor-id.
**
**  Side Effects:
**	If a cursor state is found, it is freed.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	30-apr-1990 - Cursor performance: trace pre-fetch closure. (ncg)
**	03-jul-1991 - Check that cursor was not automatically freed by "close"
**		processing before trying to free it again to avoid double free
**		of allocated memory.  Bug #37537.
*/

void
IIcsClose( cursor_name, num1, num2 )
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE	*statep;
    i4	gl_flags = IIglbcb->ii_gl_flags;

    statep = IIcs_GetState(IIlbqcb, cursor_name, num1, num2);
    if (statep == NULL)
    {
	IIlocerr(GE_CURSOR_UNKNOWN, E_LQ0058_CSNOTOPEN, II_ERR,
		 2, cursor_name, ERx("close"));
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	IIqry_end();
	return;
    }
    /* Send "close" block */
    if (IIqry_start(GCA_CLOSE, 0, 0, ERx("close")) != OK)
	return;
    IIqid_send( IIlbqcb, &statep->css_name );
    IIcgc_end_msg(IIlbqcb->ii_lq_gca);
    /* Flush response block */
    IIqry_end();
    /*
    ** Test that 'IIqry_end()' didn't already free all cursor states
    ** (including this one) through 'IIcsAllFree()' if it was executed
    ** in AUTOCOMMIT ON mode before trying to free it.  (Bug #37537.)
    */
    if ( IIlbqcb->ii_lq_csr.css_first != NULL )
    { /* ... no cursor state was freed by "close" */
	if ( statep->css_pfstream  &&  gl_flags & II_G_CSRTRC )
 	    SIprintf(ERx("PRTCSR - Close %s: stream being freed\n"),cursor_name);
	IIcs_FreeState( IIlbqcb, statep );
    }
}

/*{
+*  Name: IIcsRdO - Set, reset, or send the readonly status to the BE.
**
**  Description:
**	This is for repeat cursors.  We are called (with II_EXSETRDO) from
**	OPEN CURSOR cx [FOR READONLY], and (with II_EXGETRDO) from rep_close,
**	to implement the "for readonly" clause.
**
**	If "what" is II_EXSETRDO then remember "for readonly".
**	If "what" is II_EXCLRRDO then remember "not for readonly".
**	If "what" is II_EXVARRDO then examine the "rdo" string.  If it is
**	"for readonly" (with optional leading and trailing whitespace, and
**	optional extra intermediate whitespace), then remember "for readonly";
**	else remember "not for readonly".  Case is insignificant.
**	If "what" is II_EXGETRDO, then if we remembered "for readonly" then
**	send "for readonly" to the database.  Remember "not for readonly".
**	Initially memory is "not for readonly".
**
**	Values of local "for_readonly" - meaningful only for II_EXVARRDO:
**		-1	string contained non-blank garbage
**	 	 0	string contained only whitespace
**	 	 1	string contained "for readonly", ignoring case and
**			whitespace
**
**  Inputs:
**	what		- Set or send the readonly status
**	rdo		- If "set", the readonly string.
**
**  Outputs:
**	Returns:
**	    Void
**	Errors:
**	    CSRDO	- Bad "for readonly" string.
-*
**  Side Effects:
**	Sets/resets a static flag, optionally sends "for readonly" to the BE.
**  History:
**	29-dec-1986 - Written. (mrw)
**	07-may-1987 - Modified. (ncg)
*/

void
IIcsRdO( what, rdo )
i4	what;
char	*rdo;
{
    II_THR_CB	*thr_cb = IILQthThread();
    char	rdo_buf[21];

    switch (what)
    {
      case II_EXCLRRDO:
	thr_cb->ii_th_readonly = 0;
	break;
      case II_EXSETRDO:
	thr_cb->ii_th_readonly = 1;
	break;
      case II_EXVARRDO:
	if (rdo == NULL)
	    thr_cb->ii_th_readonly = 0;
	else
	{
	    IIstrconv(II_CONV, rdo, rdo_buf, 20);
	    if (STzapblank(rdo_buf, rdo_buf) == 0)	/* All blanks ? */
		thr_cb->ii_th_readonly = 0;
	    else if (STbcompare(rdo_buf, 0, ERx("forreadonly"), 0, TRUE) == 0)
		thr_cb->ii_th_readonly = 1;
	    else 					/* Something else */
		thr_cb->ii_th_readonly = -1;
	}
	break;
      case II_EXGETRDO:
	switch (thr_cb->ii_th_readonly)
	{
	  case -1: /* Error in "for readonly" string. */
	    IIlocerr(GE_SYNTAX_ERROR, E_LQ005C_CSRDO, II_ERR, 0, (char *)0);
	    break;
	  case 0: /* No "for readonly" string, or a blank string. */
	    break;
	  case 1: /* A valid "for readonly" string. */
	    IIwritedb( ERx(" for readonly ") ); /* Surrounding blanks req'red */
	    break;
	}
	thr_cb->ii_th_readonly = 0;
	break;
    }
}

/*{
+*  Name: IIcs_GetState 	- Find the cursor state with a given id.
**
**  Description:
**	Just search the list pointed at by css_first until finding
**	a cursor state whose id matches id passed in by user.
**	Return pointer to that state or NULL if not found.
**	If the ids are zero then the cursor is looked up by name.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	cursor_name	- Cursor name to look for in Dynamic case (numx = 0)
**	num1,num2	- Cursor identifying numbers.  If these are zero then
**			  the cursor is looked up by name.
**
**  Outputs:
**	Returns:
**		Pointer to cursor state whose id numbers/name matches inputs
**	        NULL - State not found
**	Errors:
**		None.
**
**  Side Effects:
**	None
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	27-jan-1989 - Modified to look up by name if cursor ids are zero. (ncg)
**	28-sep-1989 - Modified to look up dynamic cursor names by FE name.
**		      GW's return a different internal DBMS name to the FE name
**		      and this is a user (FE) name passed in which must be 
**		      searched against original FE names. (neil)
**	14-mar-1990	(barbara)
**	    Do a case insensitive comparison on dynamic cursor names to
**	    preserve the INGRES case-insensitive semantics. (Bug #8655)
*/

static IICS_STATE *
IIcs_GetState( II_LBQ_CB *IIlbqcb, char *cursor_name, i4  num1, i4  num2 )
{
    register IICS_STATE	*cs;
    bool		dyn = FALSE;
    char		dynamic_name[GCA_MAXNAME];

    if (num1 == 0 && num2 == 0)		/* Dynamic */
    {
	dyn = TRUE;
	STmove(cursor_name, ' ', GCA_MAXNAME, dynamic_name);
    }

    /*
    ** Loop comparing dynamic with dynamic and static with static.  Dynamic
    ** cursors are searched by name, and static ones by number.
    */
    for (cs = IIlbqcb->ii_lq_csr.css_first; cs; cs = cs->css_next)
    {
	if (dyn && cs->css_flags == II_CS_DYNAMIC)
	{
	    if (STbcompare(dynamic_name, GCA_MAXNAME,
			    cs->css_fename.gca_name, GCA_MAXNAME, TRUE) == 0)
		return cs;
	}
	else if (!dyn && cs->css_flags != II_CS_DYNAMIC)
	{
	    if (   cs->css_fename.gca_index[0] == num1
		&& cs->css_fename.gca_index[1] == num2)
		return cs;
	}
    }
    return NULL;
}

/*{
+*  Name: IIcs_FreeState 	- Remove a cursor state from list and free
**
**  Description:
**	Given a pointer to a cursor state, search a list of states to
**	find the one whose address matches the pointer.  Free
**	that state and its pointers.
**
**  Inputs:
**	IIlbqcb		Current Session control block.
**	statep		- Pointer to state to be freed
**
**  Outputs:
**	Returns:
**		Void
**	Errors:
**		None
**
**  Side Effects:
**	May change css_first, pointer to head of list of cursor states.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	25-mar-1987 - modified to free local pointers too. (ncg)
**	30-apr-1990 - Free alternate stream (ncg)
*/

static void
IIcs_FreeState( II_LBQ_CB *IIlbqcb, IICS_STATE *statep )
{
    register	IICS_STATE	*cs;

    if (statep == NULL)
	return;
    if (statep->css_pfstream != NULL)
	IIcga2_close_stream(IIlbqcb->ii_lq_gca, statep->css_pfstream);
    if (statep->css_cdesc.rd_dballoc > 0 && statep->css_cdesc.rd_gca != NULL)
	MEfree((PTR)statep->css_cdesc.rd_gca);
    if (statep->css_cdesc.rd_nmalloc > 0 && statep->css_cdesc.rd_names != NULL)
	MEfree((PTR)statep->css_cdesc.rd_names);
    if (statep == IIlbqcb->ii_lq_csr.css_first)	/* Special case first element */
    {
	IIlbqcb->ii_lq_csr.css_first = IIlbqcb->ii_lq_csr.css_first->css_next;
	statep->css_next = NULL;
	MEfree( (PTR)statep );
	return;
    }
    for (cs=IIlbqcb->ii_lq_csr.css_first; cs; cs=cs->css_next)
    {
	if (cs->css_next == statep)
	{
	    cs->css_next = cs->css_next->css_next;
	    statep->css_next = NULL;
	    MEfree( (PTR)statep );
	    return;
	}
    }
}

/*{
+*  Name: IIcs_MakeState	- Allocate a cursor state and attach to list.
**
**  Description:
**	This routine allocates a cursor state (a fixed size structure).
**	If the memory allocation goes successfully, the cursor state
**	is placed at the top of the list of states.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	Returns:
**		Pointer to newly allocated state 
**		(may be NULL if allocated failed)
**	Errors:
**		None
**
**  Side Effects:
**	If state successfully allocated, css_first will be changed.
-*	
**  History:
**	23-sep-1986 - written (bjb)
**	25-mar-1987 - modified to alloate fixed size strucure. (ncg)
*/

static IICS_STATE *
IIcs_MakeState( II_LBQ_CB *IIlbqcb )
{
    IICS_STATE	*statep;

    if ((statep = (IICS_STATE *)MEreqmem((u_i4)0, (u_i4)sizeof(IICS_STATE),
	TRUE, (STATUS *)NULL)) == NULL)
    {
	return NULL;
    }
    if (IIlbqcb->ii_lq_csr.css_first == NULL)
    {
	IIlbqcb->ii_lq_csr.css_first = statep;
	statep->css_next = NULL;	/* Just for safety */
    }
    else
    {
	statep->css_next = IIlbqcb->ii_lq_csr.css_first;
	IIlbqcb->ii_lq_csr.css_first = statep;
    }
    return statep;
}


/*{
**    Name:    IIcsAllFree    - Cleanup cursors to enable db restart
**
**    Description:
**	This routine frees memory used by all open cursors - it does not
**	communicate with the backend.  This routine is called from IIexit
**	and, now, also IIxact when a transaction is closed.
**
**    Inputs:
**	IIlbqcb		Current session control block.
**    Outputs:
**	  Returns:
**	      	Nothing
**	  Errors:
**	      	None:
**    Side effects:
**	  Frees memory used by All currently opened cursors 
**
**    History:
**	  18-feb-1988 - Written.  (marge) 
*/

VOID
IIcsAllFree( II_LBQ_CB *IIlbqcb )
{
    register IICS_STATE  *cp;	   /* Ptr into linked list of open cursors */
    register IICS_STATE	*nextcp;   /* Ptr to next open cursor */

    cp = IIlbqcb->ii_lq_csr.css_first; 
    while (cp)
    {
    	nextcp = cp->css_next;	/* Save ptr to next cursor state */
    	IIcs_FreeState( IIlbqcb, cp );
	cp = nextcp;
    }
}

/*{
** Name:	IILQcgdCursorGetDesc - Get the # of columns for a descriptor.
**
** Description:
**	This routine is used to return the descriptor information for a cursor.
**	The routine returns the number of result columns given by the cursor.
**	This routine is intended to be used in conjunction with
**	IILQcgnCursorGetName to get the names of the result columns for
**	a cursor.
**
**	It can only be called if the cursor given by the first three
**	arguments was successfully opened.
**
** Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor
**
** Outputs:
**	numcols		Will be set to the number of result columns
**			for each row the cursor will return.
**
**	Returns:
**		0 if the cursor's state could not be found.  This would
**                normally mean that the cursor is not known to libq.
**              1 if the cursor was found.  In this case, numcols will
**		  have been given a valid value.
**
** History:
**	21-feb-1990 (Joe)
**	    Written
*/
i4
IILQcgdCursorGetDesc(cursor_name, num1, num2, numcols)
char	*cursor_name;
i4	num1, num2;
i4	*numcols;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE	*state;

    state = IIcs_GetState(IIlbqcb, cursor_name, num1, num2);
    if (state == NULL)
    {
	return 0;
    }
    *numcols = state->css_cdesc.rd_numcols;
    return 1;
}

/*{
** Name:	IILQcgnCursorGetName - Get the name of a result column.
**
** Description:
**	This routine is intended to be used in conjunction with
**	IILQcgdCursorGetDesc to get the names of the result columns
**	for a cursor.
**	
**	This routine can be called to get the name of each result column.
**	The name and length returned by the routine are controlled by
**	LIBQ.  They are only guaranteed to be valid while the cursor is
**	opened.  Once the cursor is closed, then the names are no longer
**	valid.  IT IS THE CALLER'S RESPONSIBILITY TO ENSURE THAT THEY DON"T
**	USE THE DATA AFTER THE CURSOR HAS BEEN CLOSED.
**
** Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor
**	col_num		- The number of the column for which the name
**			  is being requested.  Column number 0 is the
**			  first column.
**
** Outputs:
**	name_len	- This will be set to the number of bytes in
**			  the name of the column.
**
**	name		- This will be set to point to the name of the
**                        column.  THIS IS NOT an EOS terminated string!
**			  The length returned in name_len tells where
**			  the end of the string is.
**
**	Returns:
**		0 if the name could not be gotten.  This can mean one of:
**	            a) The names were not gotten when the cursor was
**		       fetched.  To get the column names fetched you
**                     need to do an IIsqMods(IImodNAMES) before the open.
**                  b) The column number is out of range.
**
**		1 if the name was gotten.  Note that name_len and name are
**		  only set to valid values if the return returns 1.
**
** History:
**	21-feb-1990 (Joe)
**          First Written
*/
i4
IILQcgnCursorGetName(cursor_name, num1, num2, col_num, name_len, name)
char	*cursor_name;
i4	num1, num2;
i4	col_num;
i4	*name_len;
char	**name;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE	*state;

    *name = NULL;
    *name_len = 0;
    state = IIcs_GetState(IIlbqcb, cursor_name, num1, num2);
    if (state == NULL
	|| !(state->css_cdesc.rd_flags & RD_NAMES)
	|| state->css_cdesc.rd_names == NULL
	|| col_num >= state->css_cdesc.rd_numcols)
    {
	return 0;
    }
    *name = state->css_cdesc.rd_names[col_num].rd_nmbuf;
    *name_len = state->css_cdesc.rd_names[col_num].rd_nmlen;
    return 1;
}

/*{
** Name:	IILQcikCursorIsKnown	- Whether libq knows this cursor.
**
** Description:
**	This routine tells the client whether libq knows about this cursor.
**	This is used by the 4GL cursor object routines to see whether the
**	cursor has been closed.
**
** Inputs:
**	cursor_name	- User defined cursor name
**	num1, num2	- Unique integers identifying cursor
**
** Outputs:
**	Returns:
**		TRUE if libq knows about this cursor.  NOTE, this does not
**		     mean the cursor is open, only that libq doesn't think
**		     it is closed.  The DBMS could have closed the cursor
**		     because of an error without telling libq.
**	        FALSE if libq doesn't know about the cursor.
**
** History:
**	20-jul-1990 (Joe)
**	    First Written
*/
bool
IILQcikCursorIsKnown(cursor_name, num1, num2)
char	*cursor_name;
i4	num1, num2;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    return (IIcs_GetState(IIlbqcb, cursor_name, num1, num2) != NULL);
}

/*{
** Name:	IILQcsgCursorSkipGet -	Skip a column during a fetch.
**
** Description:
**	This routine can be called in place of IIcsGetio to skip the
** 	current column and move on to the next column.  It is used by
**	Sapphire and in IIcsERetrieve to skip unwanted columns during a fetch.
**
**	Like IIcsGetio, this can only be called between matching
**	IIcsRetrieve and IIcsERetrieve calls.
**
** Example of Use:
**	
**	Given a cursor whose select statement is:
**
**		SELECT c1, c2, c3 FROM t;
**
**	To fetch only the values for c1 and c3 you can do:
**
**	IIsqInit(&sqlca);
**	if (IIcsRetrieve( cursor_name, id, id ) != 0);
**	{
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    IILQcsgCursorSkipGet();
**	    IIcsGetio(&ind,isvar,type,len,addr);
**	    IIcsERetrieve();
**	}
**
** Side Effects:
**	Changes the current column for the current cursor
**	(i.e. csr_cur->css_colcount is incremented).
**
** History:
**	21-feb-1990 (Joe)
**	    Written for Windows 4GL.
**	14-sep-1990 (Joe)
**	    Integrated into 64.  Put in support for prefetch.
*/
VOID
IILQcsgCursorSkipGet()
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICS_STATE		*csr_cur;
    char 		csrname[ GCA_MAXNAME +1 ];
    char		*qrystr;

    csr_cur = IIlbqcb->ii_lq_csr.css_cur;	/* Current csr being fetched */
    qrystr = IIlbqcb->ii_lq_dml == DB_QUEL ?
		ERx("retrieve cursor") : ERx("fetch");
    /*
    ** Fetching too many cols?  If so, give error first time this
    ** happens.
    */
    if (csr_cur->css_colcount >= csr_cur->css_cdesc.rd_numcols)
    {
	STlcopy( csr_cur->css_name.gca_name, csrname, GCA_MAXNAME );
	STtrmwhite( csrname );
	IIlocerr(GE_CARDINALITY, E_LQ005B_CSXCOLS, II_ERR, 2, qrystr, csrname);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	if (IISQ_SQLCA_MACRO(IIlbqcb))
	    IIsqWarn( thr_cb, 3 );		/* Status info for SQLCA */
	return;
    }
    if (IIlbqcb->ii_lq_curqry & II_Q_QERR)
	return;
    _VOID_ cursor_skip( IIlbqcb );
}

/*{
** Name:	cursor_skip		- Do the real work of skipping a column
**
** Description:
**	This routine does the real work of skipping a column.  It is called
**	by IIcsERetrieve and IILQcsgCursorSkipGet.  These two routines
**	need slightly different behavior.  IILQcsgCursorSkipGet is supposed
**	to act like IIcsGetio.  If there were any errors on subsequent
**	gets or skips, then IILQcsgCursorSkipGet should be a noop.
**
**	IIcsERetrieve needs to do some skipping if it is a prefetched
**	cursor even if there are errors.  Also, IIcsERetrieve needs to
**	be notified of errors.
**
** Side Effects:
**	Changes the current column for the current cursor
**	(i.e. csr_cur->css_colcount is incremented).
**
** History:
**	14-sep-1990 (Joe)
**	    Written.
*/
static STATUS
cursor_skip( II_LBQ_CB *IIlbqcb )
{
    DB_DATA_VALUE	*dbv;	/* Will point to cursor state descriptor */
    IICS_STATE		*csr_cur;
    IICGC_ALTRD		*pfstream;
    i4			skiplen;


    csr_cur = IIlbqcb->ii_lq_csr.css_cur;	/* Current csr being fetched */
    pfstream = csr_cur->css_pfstream;
    /*
    ** If there was an error, and this isn't a prefetched cursor,
    ** then we can't do anything.  Otherwise, skip a column's data.
    */
    if ((IIlbqcb->ii_lq_curqry & II_Q_QERR) && pfstream == NULL)
	return FAIL;

    /* Get the descriptor */
    dbv = &csr_cur->css_cdesc.RD_DBVS_MACRO(csr_cur->css_colcount); 

    /*
    ** By setting this to NULL, IIcgc_get_value will skip the
    ** data.
    */
    dbv->db_data = NULL;
    skiplen =  IIcgc_get_value(IIlbqcb->ii_lq_gca, GCA_TUPLES,
			       IICGC_VVAL, dbv);
    if (skiplen != dbv->db_length && skiplen != IICGC_GET_ERR)
    {
	/*
	** A short read on a pre-fetched cursor will make the next
	** fetch react as though there's no more data.
	*/
	IIlocerr(GE_COMM_ERROR, E_LQ0040_SHORTREAD, II_ERR,
		 0, (char *)0);
	IIlbqcb->ii_lq_curqry |= II_Q_QERR;
	return E_LQ0040_SHORTREAD;
    }
    csr_cur->css_colcount++;
    return OK;
}

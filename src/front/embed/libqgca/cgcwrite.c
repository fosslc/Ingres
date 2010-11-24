/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<me.h>
# include	<cm.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<erlc.h>

/**
+*  Name: cgcwrite.c - This file contains routines to write messages to GCA.
**
**  Description:
**	The routines in this file do the writing of the messages to GCA.
**	They act as a layer between user applications (as seen by LIBQ) and the
**	GCA module.  Mostly these routines just act as buffer managers
**	and police the protocol.
**
**  Defines:
**
**	IIcgc_init_msg - 	Initialize a message buffer.
**	IIcgc_put_msg - 	Add data to message buffer.
**	IIcgc_end_msg - 	End the message (GCA_SEND with end_of_data).
**	IIcgc_break - 		Interrupt query (GCA_ATTENTION & GCA_IACK msg).
**	IIcc1_put_qry - 	Add query text to query buffer.
**	IIcc2_put_gcv - 	Add a GCA data value to message buffer.
**	IIcc3_put_bytes - 	Low level (atomic) byte copier.
**	IIcc1_send - 		Mid-message send (GCA_SEND w/o end_of_data).
-*
**  History:
**	28-jul-1987	- Written (ncg)
**	09-may-1988	- Added support for DB procedures (ncg)
**	23-jan-1989     - Removed all GCANEW ifdef's (ncg)
**	16-may-1989	- Replaced MEcopy with a MECOPY macro call (bjb)
**	28-feb-1990	- Turn off COPY_MASK as part of fix to bug 7927. (bjb)
**	30-apr-1990	- "Reset" when interrupting alternate-stream. (ncg)
**	14-jun-1990	(barbara)
**	    Don't automatically quit on association fail errors. (bug 21832)
**	01-nov-1991 	(kathryn)
**	    Change first parameter of cgc_debug() calls from TRUE (start of new
**	    query) and FALSE (addition to current query text) to:
**	    IIDBG_INIT - New query - reinitialize query trace buffer.
**          IIDBG_ADD  - Append text strings to current query trace buffer.
**	    IIDBG_END  - Sending query to DBMS - Output trace of query text.
**	05-oct-1992  	(kathryn)
**	    Change IIcc2_put_gcv() to allow for large objects (BLOBS).
**	16-Nov-95 (gordy)
**	    Added GCA1_FETCH message.
**      18-jan-99 (loera01) Bug 94914
**          For IIcc1_send(), fill gca_protocol field of the trace 
**          control block if printgca tracing is turned on, and the caller is 
**          a child process.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-may-2001 (abbjo03)
**	    Add support for NCHAR/Unicode.
**	18-May-01 (gordy)
**	    The embeded length in NVCHR is number of characters, not bytes.
**      12-sep-2001 (stial01)
**          Added DB_NVCHR_TYPE handling
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**	 1-Oct-09 (gordy)
**	    Added GCA2_INVPROC message.
**/

/*{
**  Name: IIcgc_init_msg - Initialize a message buffer to send to server.
**
**  Description:
**	At the start of each message request the client must set up the message
**	buffers that will be sent to the server (GCA_FORMAT was issued once
**	at the association-connect time).  The result information (used when 
**	reading from the server) is also initialized.
**
**	Nothing is actually sent to the server in this routine.
**
**	There are two buffers to set up: a message data buffer and a query text
**	buffer (not actually a message buffer).  The query text buffer is used
** 	only for query text which is passed to us in small pieces but we want
**	to send in one big chunk - this was done so that little pieces of
**	queries will not each have a special GC_DATA_VALUE when sent.
**	When the query buffer is filled, it is sent via the message data buffer
**	as a GCA_DATA_VALUE of type DB_QTXT_TYPE.  If there is no query text
**	associated with the query (ie, GCA_FETCH) then the query buffer is
** 	not even used.
**
**	If there is query debugging (the EQUEL or ESQL -d flag) then this
**	routine sets up the initial debugging values (ie, some message types
**	are mapped to query strings - GCA_Q_BTRAN = "begin transaction").
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	message_type	- Message type of request.
**	query_lang	- Query language value (DB_SQL, DB_QUEL, DB_NOLANG)
**			  used for some message types.
**	modifier	- Modifier for some message types.  Used for query
**			  text messages.
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Examples:
**
**	Stmt: DELETE FROM employee;
**	Call: IIcgc_init_msg(cgc, GCA_QUERY, DB_SQL, 0);
**
**	Stmt: COMMIT;
**	Call: IIcgc_init_msg(cgc, GCA_COMMIT, DB_SQL, 0);
**
**	Stmt: DESCRIBE s1 INTO desc;
**	Call: IIcgc_init_msg(cgc, GCA_QUERY, DB_SQL, GCA_NAMES_MASK);
**
**
**  Side Effects:
**
**  History:
**	14-aug-1986		Written for Jupiter. (ncg)
**	16-jul-1987		Rewritten for GCA. (ncg)
**	03-feb-1988		GCA_FORMAT is now called once at
**				start of session. (ncg)
**	09-may-1988		Added some new message types to fall in
**				the "query text" group. (ncg)
**	13-mar-1989		GCA_SECURE now has debugging string. (bjb)
**	02-aug-1989		Initialize rowcount to a special number as a
**			 	way of letting LIBQ know at the end of a query
**				whether a response block was received. (bjb)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if
**	    there is no connection.  Added check that cgc is non-null.
**	01-nov-1991	(kathryn)
**	    Change parameter of cgc_debug() from TRUE to its equivalent 
**	    IIDBG_INIT. New query - trace buffer should be re-initialized.
**      10-dec-1992     (nandak,mani,larrym)
**          Added GCA_XA_SECURE
**	24-may-1993 (kathryn)
**	    Added new GCA1_DELETE message to GCA_DELETE case.
**      17-jun-1993 (larrym)
**          Added new GCA1_INVPROC message to GCA_INVPROC case.
**	4-nov-93 (robf)
**          Added new GCA_PRREPLY
**	16-Nov-95 (gordy)
**	    Added GCA1_FETCH message.
**	13-feb-2007 (dougi)
**	    Added GCA2_FETCH message.
**	 1-Oct-09 (gordy)
**	    Added GCA2_INVPROC message.
*/

VOID
IIcgc_init_msg(cgc, message_type, query_lang, modifier)
IICGC_DESC	*cgc;
i4		message_type;
i4		query_lang;
i4		modifier;
{
    GCA_Q_DATA		query_data;	/* Temp for values put in qry message */
    IICGC_MSG		*cgc_msg;	/* Write message buffer */
    DB_DATA_VALUE	dbg_dbv;	/* Application debugging info */
    char		*dbg_str;

    /* Was connection successful or have we already left the server? */
    if (cgc == (IICGC_DESC *)0 || cgc->cgc_g_state & IICGC_QUIT_MASK)
	return;

    /* Reset states associated with each message */
    cgc->cgc_query_lang = query_lang;
    cgc->cgc_m_state = 0;

    /* Set up write information */
    cgc_msg = &cgc->cgc_write_msg;
    cgc_msg->cgc_d_used = 0;
    cgc_msg->cgc_message_type = message_type;

    /* Set up query text buffer */
    if (   message_type == GCA_QUERY
	|| message_type == GCA_DEFINE
	|| message_type == GCA_CREPROC
	|| message_type == GCA_DRPPROC)
    {
	/* Turn on indication that this is a textual query */
	cgc->cgc_m_state |= IICGC_QTEXT_MASK;

	/* No query text yet */
	cgc->cgc_qry_buf.cgc_q_used = 0;

	/* Deposit language id and query modifier (GCA_NAMES_MASK, etc) */

	query_data.gca_language_id = query_lang;
	query_data.gca_query_modifier = modifier;
	MECOPY_CONST_MACRO((PTR)&query_data,
		(u_i2)(sizeof(i4) + sizeof(i4)),
	       (PTR)cgc_msg->cgc_data);
	cgc_msg->cgc_d_used += sizeof(i4) + sizeof(i4);
    }

    /* Set up result information */
    cgc->cgc_result_msg.cgc_d_length = 0;
    cgc->cgc_result_msg.cgc_d_used   = 0;
    cgc->cgc_result_msg.cgc_data     = (char *)0;
    cgc->cgc_result_msg.cgc_message_type = 0;

    /* And the response block */
    cgc->cgc_resp.gca_rid = 0;
    cgc->cgc_resp.gca_rqstatus = 0;
    /* If no response block received, LIBQ will recognize this constant */
    cgc->cgc_resp.gca_rowcount = GCA_ROWCOUNT_INIT;
    cgc->cgc_resp.gca_rhistamp = 0;
    cgc->cgc_resp.gca_rlostamp = 0;

    /* Debugging on? */
    if (cgc->cgc_debug)
    {
	switch (message_type)
	{
	  case GCA_Q_BTRAN:
	    dbg_str = ERx("begin transaction");
	    break;
	  case GCA_Q_ETRAN:
	    dbg_str = ERx("end transaction");
	    break;
	  case GCA_ABORT:
	    dbg_str = ERx("abort");
	    break;
	  case GCA_ROLLBACK:
	    dbg_str = ERx("rollback");
	    break;
	  case GCA_COMMIT:
	    dbg_str = ERx("commit");
	    break;
	  case GCA_SECURE:
	    dbg_str = ERx("prepare to commit");
	    break;
	  case GCA_XA_SECURE:
	    dbg_str = ERx("xa - prepare to commit");
	    break;
	  case GCA_QUERY:
	  case GCA_DEFINE:
	  case GCA_CREPROC:
	  case GCA_DRPPROC:
	    dbg_str = ERx("");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_INVOKE:
	    dbg_str = ERx("execute query ");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_FETCH:
	  case GCA1_FETCH:
	  case GCA2_FETCH:
	    dbg_str = query_lang == DB_SQL ?
			ERx("fetch ") : ERx("retrieve cursor ");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_DELETE:
	  case GCA1_DELETE:
	    dbg_str = query_lang == DB_SQL ?
			ERx("delete where current of ") : ERx("delete cursor ");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_CLOSE:
	    dbg_str = query_lang == DB_SQL ?
			ERx("close ") : ERx("close cursor ");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_INVPROC:
          case GCA1_INVPROC:
	  case GCA2_INVPROC:
	    dbg_str = ERx("execute procedure ");
	    cgc->cgc_m_state |= IICGC_DBG_MASK;
	    break;
	  case GCA_ATTENTION:
	    dbg_str = ERx("interrupt query");
	    break;
	  case GCA_MD_ASSOC:
	    dbg_str = ERx("modify association");
	    break;
	  case GCA_PRREPLY:
	    dbg_str = ERx("reply to prompt");
	    break;
	  case GCA_RELEASE:
	    dbg_str = ERx("release session");
	    break;
	  default:
	    dbg_str = (char *)0;
	    break;
	}
	if (dbg_str)
	{
	    dbg_dbv.db_datatype = DB_QTXT_TYPE;
	    dbg_dbv.db_length = STlength(dbg_str);
	    dbg_dbv.db_prec = 0;
	    dbg_dbv.db_data = (PTR)dbg_str;
	    (*cgc->cgc_debug)(IIDBG_INIT, &dbg_dbv);
	}
	else
	    (*cgc->cgc_debug)(IIDBG_INIT, (DB_DATA_VALUE *)0);
    }
} /* IIcgc_init_msg */

/*{
**  Name: IIcgc_put_msg - Put values into a message buffer.
**
**  Description:
**	This routine adds values to the message buffer.  The values may be
**	query text (ie, GCA_QUERY), query ids (ie, GCA_DEFINE or GCA_FETCH),
**	DB data values (ie, GCA_QUERY or GCA_INVOKE), or i4s (ie, type of
**	GCA_Q_BTRAN).  Query text is added to the query text buffer, while
**	the other data is added to the data message buffer.
**	
**	The types of the arguments have some special notes:
**
**	1. Query text:
**	   1.1. Added to the query buffer.
**	2. Query id:
**	   2.1. If sent in the context of textual query then the query id must
**	 	be indicated by a ~Q marker in the query buffer, and the id is
**		sent as three GCA data values in the data buffer.  If 'put_mark'
**	   	is off, then the ~Q marker is not added.
**	   2.2. If sent in a non-textual context then a GCA_ID is put into the
**		data buffer.
**	3. DB data value:
**	   3.1.	If sent in the context of textual query then the value must be
**		indicated by a ~V marker in the query buffer, and the value is
**		sent as a GCA data value in the data buffer. If 'put_mark'
**	   	is off, then the ~V marker is not added.
**	   3.2.	If sent in a non-textual context then the value is sent as a
**		GCA data value in the data buffer (without marking with ~V).
**		This may happen in a GCA_INVOKE message.
**	4. Query value:
**	   4.1.	The actual data (pointed at by the DB data value)
**		is added to the data buffer (not as a GCA data value).
**	   	This may happen in various messages, such as GCA_Q_BTRAN.
**	5. Object name:
**	   5.1. The specified name (pointed at by the DB data value) is
**		sent as a GCA_NAME structure.  This may happen with the
**		GCA_DELETE and GCA_INVPROC messages.  (The name is null
**		terminated to help tracing - the null is not included in
**		the GCA_NAME length).
**
**	If debugging is on and the values are "debug-able" (query text, DB data
**	values, query id names) then process those values.
**	
**  Inputs:
**	cgc		- Client general communications descriptor.
**	put_mark	- TRUE/FALSE - indicates whether to put the ~Q and
**			  ~V markers into the query text.  Currently, LIBQ
**			  always uses TRUE for data parameters and query ids,
**			  as they are interspersed with the queries.  However,
**			  some other user, may have built the whole query (with
**			  the markers already included) and then want to pass 
**			  the parameters.  This argument is ignored if there
**			  is no query text associated with the query (such as 
**			  GCA_INVOKE) or if 'put_tag' is not a query_id (VID)
**			  or db_data_value (VDBV).
**	put_tag		- Indicates what value the argument actually holds:
**			  IICGC_VQRY - pointer to DB data value with query text
**				       (type = DB_QTXT_TYPE).
**			  IICGC_VID  - pointer to DB data value pointing at a 
**				       valid GCA_ID (type = 0).
**			  IICGC_VDBV - pointer to DB data value with a
**				       value parameter (send as GCA data value).
**			  IICGC_VVAL - pointer to DB data value (send value
**				       without GCA data value).
**			  IICGC_VNAME- pointer to DB data value, pointing at a 
**				       null terminated name.
**	put_arg		- Argument to put into message. Type of argument
**			  is a DB data value.  DB data value MUST have the
**			  true length even if it describes a complex object.
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Example Calls (may not be exact, but give an idea):
**
**	Stmt: INSERT INTO emp VALUES (:i4var);
**	Call: IIcgc_init_msg(cgc, GCA_QUERY, DB_SQL, 0);
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VQRY,
**					   dbv_of("insert into emp values ("));
**	      IIcgc_put_msg(cgc, TRUE,  IICGC_VDBV, dbv_of(i4var));
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VQRY, dbv_of(")"));
**	      IIcgc_end_msg(cgc);
**
**	Stmt: BEGIN TRANSACTION;
**	Call: IIcgc_init_msg(cgc, GCA_Q_BTRAN, DB_QUEL, 0);
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VID,  dbv_of(xact_qid));
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VVAL, dbv_of(xact_type));
**	      IIcgc_end_msg(cgc);
**
**	Stmt: FETCH cursor;
**	Call: IIcgc_init_msg(cgc, GCA_FETCH, DB_SQL, 0);
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VID,  dbv_of(cursor_id));
**	      IIcgc_end_msg(cgc);
**
**	Stmt: EXECUTE PROCEDURE p (arg1 = :f8var);
**	Call: IIcgc_init_msg(cgc, GCA_INVPROC, DB_SQL, 0);
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VID,   dbv_of(proc_id("p")));
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VNAME, dbv_of("arg1"));
**	      IIcgc_put_msg(cgc, FALSE, IICGC_VDBV,  dbv_of(f8var);
**	      IIcgc_end_msg(cgc);
**
**  Side Effects:
**	None
**	
**  History:
**	15-aug-1986	- Written (ncg)
**	16-jul-1987	- Rewritten for GCA (ncg)
**	10-may-1988	- Modified to support DB procedures and the
**			  IICGC_VNAME tag (ncg)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if
**	    there is no connection.  Added check that cgc is non-null.
**	01-nov-1991 	(kathryn)
**	    Change parameter of cgc_debug() from FALSE to its equivalent 
**	    IIDBG_ADD. Text should be added to current query trace buffer.
**	15-sep-1993	(kathryn)
**	    Add GCA1_INVPROC to list of DBV's which should convert DB_CHA_TYPE
**	    to DB_VCH_TYPE.
**      30-jul-1999 (stial01)
**          Added case IICGC_FLUSHQTXT, to be called before writing
**          large object data into the GCA buffer. This will pre-fill the
**          query text with blanks, then call IIcc1_put_qry with 3 more blanks,
**          which should make sure the back end sees the query (so far)
**          before the large object data.
**      07-nov-2000 (stial01)
**          Removed IICGC_FLUSHQTXT
**	 1-Oct-09 (gordy)
**	    Add GCA2_INVPROC to list of DBV's which should convert DB_CHA_TYPE
**	    to DB_VCH_TYPE.
*/

VOID
IIcgc_put_msg(cgc, put_mark, put_tag, put_arg)
IICGC_DESC	*cgc;
bool		put_mark;
i4		put_tag;
DB_DATA_VALUE	*put_arg;
{
    DB_DATA_VALUE	dbv;		/* Local DBV */
    GCA_ID		*qid; 		/* Local query id */
    i4			message_type;	/* Message type for char convertion */
    char		dbg_id[GCA_MAXNAME+4];	/* Debugging id name */
    i4             name_len;       /* For GCA_NAME length */

    if (cgc == (IICGC_DESC *)0)
	return;
    switch (put_tag)
    {
      case IICGC_VQRY:				/* Query text - add it */
	IIcc1_put_qry(cgc, put_arg);
	break;

      case IICGC_VID:				/* Query id - textual or not */
	qid = (GCA_ID *)put_arg->db_data;
	if (cgc->cgc_m_state & IICGC_QTEXT_MASK)
	{
	    /* Put the ~Q marker and the query id as 3 GCA data values */
	    if (put_mark)
	    {
		dbv.db_datatype = DB_QTXT_TYPE;
		dbv.db_prec = 0;
		dbv.db_length = 4;
		dbv.db_data = (PTR)ERx(" ~Q ");
		IIcc1_put_qry(cgc, &dbv);
	    }

	    dbv.db_datatype = DB_INT_TYPE;			/* First int */
	    dbv.db_length   = sizeof(qid->gca_index[0]);
	    dbv.db_data     = (PTR)&qid->gca_index[0];
	    IIcc2_put_gcv(cgc, FALSE, &dbv);

	    dbv.db_data = (PTR)&qid->gca_index[1];		/* Second */
	    IIcc2_put_gcv(cgc, FALSE, &dbv);

	    dbv.db_datatype = DB_CHA_TYPE;			/* Name */
	    dbv.db_length   = GCA_MAXNAME;
	    dbv.db_data     = (PTR)qid->gca_name;
	    IIcc2_put_gcv(cgc, FALSE, &dbv);
	}
	else /* Not inside query text so send as an id */
	{
	    /* Put the query id as a structure - treat as atomic data */
	    IIcc3_put_bytes(cgc, TRUE, sizeof(*qid), (char *)qid);
	} /* If query text */
	break;

      case IICGC_VDBV:					/* GCA value param */
	if ((cgc->cgc_m_state & IICGC_QTEXT_MASK) && put_mark)
	{
	    /* Put the ~V marker */
	    dbv.db_datatype = DB_QTXT_TYPE;
	    dbv.db_prec = 0;
	    dbv.db_length = 4;
	    dbv.db_data = (PTR)ERx(" ~V ");
	    IIcc1_put_qry(cgc, &dbv);
	}
	/* Only convert chars for query data parameters */
	message_type = cgc->cgc_write_msg.cgc_message_type;
	IIcc2_put_gcv(cgc, (cgc->cgc_m_state & IICGC_QTEXT_MASK)
		        || message_type == GCA_INVPROC
			|| message_type == GCA1_INVPROC
			|| message_type == GCA2_INVPROC
		        || message_type == GCA_INVOKE,
		        put_arg);
	break;

      case IICGC_VVAL:					/* Regular value */
	/*
	** These values should only be i4 types.  But we allow here
	** for non-nullable floats and char too.
	*/
	if (   put_arg->db_datatype == DB_INT_TYPE
	    || put_arg->db_datatype == DB_FLT_TYPE)
	{
	    IIcc3_put_bytes(cgc, TRUE, (i4)put_arg->db_length, 
					(char *)put_arg->db_data);
	}
	else /* Had better be chars */
	{
	    IIcc3_put_bytes(cgc, FALSE, (i4)put_arg->db_length, 
					(char *)put_arg->db_data);
	} /* If type is numeric */
	break;

      case IICGC_VNAME:					/* Send as GCA_NAME */
	name_len = put_arg->db_length;
	IIcc3_put_bytes(cgc, TRUE, sizeof(name_len), (char *)&name_len);
	IIcc3_put_bytes(cgc, FALSE, (i4)name_len, (char *)put_arg->db_data);
	break;

    } /* end switch tag */


    /* Debugging on? */
    if (cgc->cgc_debug && (cgc->cgc_m_state & IICGC_DBG_MASK))
    {
	switch (put_tag)
	{
	  case IICGC_VQRY:		    /* Debug query text and values */
	  case IICGC_VDBV:
	    (*cgc->cgc_debug)(IIDBG_ADD, put_arg);
	    break;
	  case IICGC_VID:		    /* Debug id name */
	    dbg_id[0] = '(';
	    STlcopy(qid->gca_name, dbg_id+1, GCA_MAXNAME);
	    name_len = STtrmwhite(dbg_id);
	    STcopy(ERx(") "), dbg_id + name_len);
	    name_len += 2;
	    dbv.db_datatype = DB_QTXT_TYPE;
	    dbv.db_prec     = 0;
	    dbv.db_length   = name_len;
	    dbv.db_data     = (PTR)dbg_id;
	    (*cgc->cgc_debug)(IIDBG_ADD, &dbv);
	    break;
	  case IICGC_VNAME:		    /* Debug GCA_NAME */
	    dbg_id[0] = ' ';
	    STlcopy(put_arg->db_data, dbg_id+1, GCA_MAXNAME);
	    STcat(dbg_id, ERx("= "));
	    dbv.db_datatype = DB_QTXT_TYPE;
	    dbv.db_prec     = 0;
	    dbv.db_length   = STlength(dbg_id);
	    dbv.db_data     = (PTR)dbg_id;
	    (*cgc->cgc_debug)(IIDBG_ADD, &dbv);
	    break;
	  case IICGC_VVAL:		    /* Never debug */
	    break;
	}
    } /* If debugging */
} /* IIcgc_put_msg */

/*{
**  Name: IIcc1_put_qry - Put query text into the query buffer.
**
**  Description:
**	This routine is internal to the iicgc routines.
**
**	The caller passes a DB data value (with type DB_QTXT_TYPE - left 
**	unchecked) and query text data.  The query text is added to the query
**	buffer.  If the query buffer is full then the text is pointed at
**	and passed to put_bytes in order to put into the final message buffer.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	qry_dbv		- DB data value with type DB_QTXT_TYPE, pointing at
**		     	  a character string. 
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Side Effects:
**	None
**	
**  History:
**	17-jul-1987	- Written for GCA (ncg)
**	21-oct-1988	- Improved CMcopy interface (ncg)
*/

VOID
IIcc1_put_qry(cgc, qry_dbv)
IICGC_DESC	*cgc;
DB_DATA_VALUE	*qry_dbv;
{
    IICGC_QRY		*qry_buf;	/* Local query bufferer */
    i4			in_len;		/* Length of input query text */
    char		*in_ptr;	/* Pointer to input query text */
    GCA_DATA_VALUE	gcv;		/* For sending to put_bytes */
    u_i4		cplen;		/* Length used in copying data */

    qry_buf = &cgc->cgc_qry_buf;

    in_len = (i4)qry_dbv->db_length;
    in_ptr = (char *)qry_dbv->db_data;

    while (in_len > 0)
    {
	/* Can the new data fit - use lesser of data size and size left */
	cplen = min(in_len, qry_buf->cgc_q_max - qry_buf->cgc_q_used);
	if (cplen == in_len)		/* It all fits */
	{
	    MECOPY_VAR_MACRO((PTR)in_ptr, (u_i2)cplen,
		   (PTR)(qry_buf->cgc_q_buf + qry_buf->cgc_q_used));
	}
	else				/* Break it up */
	{
	    cplen =
	       CMcopy(in_ptr, cplen, qry_buf->cgc_q_buf + qry_buf->cgc_q_used);
	}

	if (cplen)
	{
	    in_ptr += cplen;
	    in_len -= cplen;
	    qry_buf->cgc_q_used += cplen;
	} /* If data was copied */

	/*
	** If we got here and there is still more text to copy then put
	** this current piece of query text into the message buffer, and
	** start over.  Note that if we put it into the message buffer even
	** if there were no more text bytes left (but the query buffer was
	** full), then we may have to send a whole GCA data value just for a
	** terminating null which is put in later (IIcgc_end_msg). 
	*/
	if (in_len > 0)
	{
	    /* Put out query text description - treat as though atomic */
	    gcv.gca_type      = DB_QTXT_TYPE;
	    gcv.gca_precision = 0;
	    gcv.gca_l_value   = qry_buf->cgc_q_used;
	    IIcc3_put_bytes(cgc, TRUE, sizeof(gcv.gca_type) +
				       sizeof(gcv.gca_precision) +
				       sizeof(gcv.gca_l_value),
				       (char *)&gcv);
	    /* Put out actual query text */
	    IIcc3_put_bytes(cgc, FALSE, (i4)gcv.gca_l_value,
					qry_buf->cgc_q_buf);

	    qry_buf->cgc_q_used = 0;	/* Reset query text counter */
	} /* If more input length */
    } /* While input length > 0 */
} /* IIcc1_put_qry */

/*{
**  Name: IIcc2_put_gcv - Put a GCA data value into the message buffer.
**
**  Description:
**	This routine is internal to the iicgc routines.
**
**	The callers DB data value is put into the message buffer as a GCA data
**	value.  Any data type is legal here (even DB_QTXT_TYPE which probably
**	means the argument came from the query bufferer).  The GCA data value
**	is only materialized into the the message buffer.  In fact, the fields
**	of the DB data value are rearranged, and the data follows the initial
**	type description as shown:
**
**	     dbv:		     gcv:
**		.db_datatype		.gca_type
**		.db_prec		.gca_precision
**		.db_length		.gca_l_value
**		.db_data 		.gca_value (contiguous memory)
**
**	All character data that is part of a GCA data value which is a
**	parameter to a query is sent as VARCHAR to the server.  The second
**	argument, 'convert_char', indicates whether this should happen, and
**	is set when passing a user's DB data value to us from IIcgc_put_msg. 
**	If we tried to only send as VARCHAR when length(CHAR) > DB_CHAR_MAX
**	then we would have a problem with REPEAT query execution where one
**	time we are sending 10 chars and the next time 1000 chars.
**	Data compression is not tried here (this could be done if we check
**	for null-valued strings etc).
**
**	The Long-text type, DB_LTXT_TYPE, is not converted to VARCHAR as
**	it is convertable to anything in the DBMS.  In fact, null constants
**	are sent as that type.
**
**	When calling put_bytes to actually copy data bytes, note when the data
**	can be broken up (character strings) or not (numeric).
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	convert_char	- TRUE/FALSE - convert character data to VARCHAR or not.
**			  If TRUE & CHAR, then dynamically convert to VARCHAR.
**	put_dbv		- DB data value to put into message.
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Side Effects:
**	None
**	
**  History:
**	15-aug-1986	- Written (ncg)
**	20-apr-1987	- Modified for character data (ncg)
**	17-jul-1987	- Rewritten for GCA (ncg)
**	07-jul-1988	- Longtext is not sent as Varchar (ncg)
**	05-oct-1992  (kathryn)
**	    Set ischar to TRUE for large objects (DB_LVCH_TYPE/DB_LBIT_TYPE).
**	    ischar is used to determine whether or not the data should be sent
**	    atomically (true for all but character datatypes). 
**	    Large objects may be too large to fit in one message buffer. 
**	26-jul-1993 (kathryn)
**	    Set ischar true for DB_BYTE_TYPE, DB_VBYTE_TYPE and DB_LBYTE_TYPE.
**	16-sep-1993 (kathryn)
**	    When convert_char is set, map fixed length byte type (DB_BYTE_TYPE)
**	    to varying length byte type (DB_VBYTE_TYPE).
**	14-may-2001 (abbjo03)
**	    Convert NCHAR variables into DB_NVCHR_TYPE.
**	18-May-01 (gordy)
**	    The embeded length in NVCHR is number of characters, not bytes.
*/
VOID
IIcc2_put_gcv(cgc, convert_char, put_dbv)
IICGC_DESC	*cgc;
bool		convert_char;
DB_DATA_VALUE	*put_dbv;
{
    GCA_DATA_VALUE	gcv;		/* Local GCV */
    i4			basetype;	/* Base type of DBV */
    bool		isnullable;	/* Is type nullable */
    bool		isvarlen;	/* Is varying length type */
    bool		ischar;		/* Is character type */
    i2			vchcount;	/* Length of varchar data */
    i2			datalen;	/* Length of data */
    char		*put_data;	/* Data area */

    gcv.gca_type      = put_dbv->db_datatype;
    basetype          = abs(gcv.gca_type);
    isnullable        = gcv.gca_type < 0;
    gcv.gca_l_value   = put_dbv->db_length;
    gcv.gca_precision = put_dbv->db_prec;
    put_data	      = (char *)put_dbv->db_data;

    /*
    ** If fixed length character string and convert_char is on, then make
    **	it a varying length character string.
    ** If fixed length byte string and convert_char is on, then make it a 
    **  varying length byte string.
    */
    if (   (convert_char)
	&& (basetype == DB_CHA_TYPE || basetype == DB_CHR_TYPE
	    || basetype == DB_NCHR_TYPE || basetype == DB_BYTE_TYPE))
    {
	/*
	**  Map from 	Fixed      to	Varying
	**
	**  Type:   	DB_CHR_TYPE	DB_VCH_TYPE
	**  Length:	L		L+DB_CNTSIZE
	**  Prec:	P		P
	**  Data:	data		[2-byte count = L] data
	**
	**  Type:   	-DB_CHR_TYPE	-DB_VCH_TYPE
	**  Length:	L+1		L+DB_CNTSIZE+1
	**  Prec:	P		P
	**  Data:	data+null_byte	[2-byte count = L] data + null_byte
	**
	**  Map (+/-)DB_NCHR_TYPE to (+/-)DB_NVCHR_TYPE with length, prec and
	**  data as defined above. 
	**
	**  Map (+/-)DB_BYTE_TYPE to (+/-)DB_VBYTE_TYPE with length, prec and
	**  data as defined above. 
	*/
	if (basetype == DB_CHA_TYPE || basetype == DB_CHR_TYPE)
	    gcv.gca_type     = isnullable ? -DB_VCH_TYPE : DB_VCH_TYPE;
	else if (basetype == DB_NCHR_TYPE)
	    gcv.gca_type = isnullable ? -DB_NVCHR_TYPE : DB_NVCHR_TYPE;
	else if (basetype == DB_BYTE_TYPE)
	    gcv.gca_type     = isnullable ? -DB_VBYTE_TYPE : DB_VBYTE_TYPE;
	gcv.gca_l_value += sizeof(vchcount);	/* = fixed length + count */

	/* Put first 3 fields as though numeric and atomic data */
	IIcc3_put_bytes(cgc, TRUE, sizeof(gcv.gca_type) +
				   sizeof(gcv.gca_precision) +
				   sizeof(gcv.gca_l_value),
				   (char *)&gcv);
	/*
	** Data portion consists of 2-byte count (which is the original fixed
	** length count adjusted for character size [less one if the original 
	** type was nullable]) plus data.
	*/
	vchcount = datalen = (i2)put_dbv->db_length;
	if (isnullable) vchcount--;	/* Do not include null byte */
	if (basetype == DB_NCHR_TYPE) vchcount /= DB_ELMSZ;	/* Characters */
	IIcc3_put_bytes(cgc, TRUE, sizeof(vchcount), (char *)&vchcount);
	IIcc3_put_bytes(cgc, FALSE, (i4)datalen, put_data);
    }
    else	/* No need to (or not) converting character data */
    {
	isvarlen = (   basetype == DB_VCH_TYPE 
		    || basetype == DB_TXT_TYPE
		    || basetype == DB_VBYTE_TYPE
		    || basetype == DB_NVCHR_TYPE
		    || basetype == DB_LTXT_TYPE) ? TRUE : FALSE;
	ischar	 = (   isvarlen
		    || basetype == DB_CHR_TYPE
		    || basetype == DB_CHA_TYPE
		    || basetype == DB_BYTE_TYPE
		    || basetype == DB_LVCH_TYPE
		    || basetype == DB_LBYTE_TYPE
		    || basetype == DB_GEOM_TYPE
		    || basetype == DB_POINT_TYPE
		    || basetype == DB_MPOINT_TYPE
		    || basetype == DB_LINE_TYPE
		    || basetype == DB_MLINE_TYPE
		    || basetype == DB_POLY_TYPE
		    || basetype == DB_MPOLY_TYPE
		    || basetype == DB_GEOMC_TYPE
		    || basetype == DB_LBIT_TYPE
		    || basetype == DB_QTXT_TYPE) ? TRUE : FALSE;

	if (basetype == DB_CHR_TYPE)		/* Convert CHR to CHA */
	    gcv.gca_type = isnullable ? -DB_CHA_TYPE : DB_CHA_TYPE;
	else if (isvarlen && basetype != DB_LTXT_TYPE
		 && basetype != DB_VBYTE_TYPE 
		 && basetype != DB_NVCHR_TYPE)
		   
	    /* Add most var-len as VCH */
	    gcv.gca_type = isnullable ? -DB_VCH_TYPE : DB_VCH_TYPE;

	/* Put out type description - treat first 3 fields as though atomic */
	IIcc3_put_bytes(cgc, TRUE, sizeof(gcv.gca_type) +
				   sizeof(gcv.gca_precision) +
				   sizeof(gcv.gca_l_value),
				   (char *)&gcv);

	/* If varying length data then put 2-byte count followed by data */
	if (isvarlen)
	{
	    vchcount = *(i2 *)put_data;
	    IIcc3_put_bytes(cgc, TRUE, sizeof(vchcount), (char *)&vchcount);
	    /* Decrement length, and increment data */
	    gcv.gca_l_value -= sizeof(vchcount);
	    put_data 	    += sizeof(vchcount);
	} /* if is varying length */

	/* If character/numeric type inform put_bytes (not atomic/atomic) */
	IIcc3_put_bytes(cgc, ischar == FALSE, (i4)gcv.gca_l_value, put_data);
    }  /* If converting character data */
} /* IIcc2_put_gcv */

/*{
**  Name: IIcc3_put_bytes - Low level byte copier for message data.
**
**  Description:
**	This routine is internal to the iicgc routines.
**
**	This routine is the low level routine for IIcgc_put_msg and its
**	derivatives (put_qry and put_gcv).  The routine does not know about
**	context or about its arguments (the values it will deposit).  It does
**	have an indication whether the value it is copying can be split
**	(strings) or not (numerics).
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	atomic		- TRUE/FALSE - the data is atomic and cannot be split.
**			  Note that this usually implies non-string data.
**			  However, some abstract data types, such as dates,
**			  can be split on their component boundaries by a lower
**			  level presentation layer that does know about the
**			  internals of such types.
**	bytes_len	- Number of bytes to copy (i4).
**	bytes_ptr	- Ptr to byte sequence (char * - must be 1 byte sizes).
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Side Effects:
**	None
**	
**  History:
**	19-aug-1986	- Written (ncg)
**	20-jul-1987	- Rewritten for GCA (ncg)
**	04-aug-1988	- Modified to use MEcopy rather than CMcopy (ncg)
*/

VOID
IIcc3_put_bytes(cgc, atomic, bytes_len, bytes_ptr)
IICGC_DESC	*cgc;
bool		atomic;
i4		bytes_len;
char		*bytes_ptr;
{
    u_i4		cplen;		/* Length used in copying data */
    IICGC_MSG		*cgc_msg;	/* "Write" message buffer */

    /*
    ** If it can fit, then copy it in one go, otherwise copy it in
    ** chunks at a time (if not atomic) and write to server.  Adjust
    ** byte counters while copying.
    */
    cgc_msg = &cgc->cgc_write_msg;
    while (bytes_len > 0)
    {
	/*
	** If atomic data then only copy if you can copy it all.  If it cannot
	** fit then write the message buffer and copy after that.
	*/
	if (atomic)
	{
	    /* Data fit in number of bytes left */
	    if (bytes_len <= (cgc_msg->cgc_d_length - cgc_msg->cgc_d_used))
	    {
		cplen = (u_i4)bytes_len;
		MECOPY_VAR_MACRO((PTR)bytes_ptr, (u_i2)cplen,
		       (PTR)(cgc_msg->cgc_data + cgc_msg->cgc_d_used));
	    }
	    else
	    {
		cplen = 0;	/* No bytes copied */
	    }
	}
	else  /* Not atomic data - can be broken */
	{
	    /*
	    ** Can the new data fit - use lesser of data size and size left.
	    ** Note that we use MEcopy rather than CMcopy to allow
	    ** binary data to be sent as well (binary data may include the
	    ** introductory character).  In the case of I/NET the presentation
	    ** layer will have to be aware of such a possibility.
	    */
	    cplen = min(bytes_len, cgc_msg->cgc_d_length - cgc_msg->cgc_d_used);
	    MECOPY_VAR_MACRO((PTR)bytes_ptr, (u_i2)cplen,
		   (PTR)(cgc_msg->cgc_data + cgc_msg->cgc_d_used));
	} /* If atomic or not */

	if (cplen)		/* If something was copied then adjust */
	{
	    bytes_ptr += cplen;
	    bytes_len -= cplen;
	    cgc_msg->cgc_d_used += cplen;
	}

	/*
	** If we got here and there are still more bytes to send then send
	** them off - this implies that we need more room.  Note that if we
	** sent it even if there were no more bytes left (but the message was
	** full),  then we may have to send a whole extra block just to get
	** the "end of data" flag in.
	*/
	if (bytes_len > 0)
	    _VOID_ IIcc1_send(cgc, FALSE);
    }
} /* IIcc3_put_bytes */

/*{
**  Name: IIcgc_end_msg - Flush message to server.
**
**  Description:
**	The message buffer has been filled with information.  If there is
**	query text associated with this message, then add the terminating null
**	and put the query text into the message buffer.  Turn on the "end of
**	data" indicator when calling IIcc1_send.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**
**  Outputs:
**	Returns:
**		None
**	Errors:
**		None
**
**  Side Effects:
**	1. Add a null byte to the query text (if there was any).
**	2. Any side effects whose source is in IIcc1_send.
**	
**  History:
**	18-aug-1986	- Written (ncg)
**	17-jul-198	- Rewritten for GCA (ncg)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if
**	    there is no connection.  Added check that cgc is non-null.
**	01-nov-1991  (kathryn)
**	    Add call cgc_debug(IIDBG_END). Query trace text will now be 
**	    generated prior to sending message to DBMS.
*/

VOID
IIcgc_end_msg(cgc)
IICGC_DESC	*cgc;
{
    IICGC_QRY		*qry_buf;	/* The query text buffer */
    DB_DATA_VALUE	dbv_qry;	/* Query text inside DB data value */

    if (cgc == (IICGC_DESC *)0)
	return;
    /*
    ** If this message has query text, then put a null on the end of the
    ** query buffer and add it to the message buffer.  The null will ALWAYS
    ** fit in the buffer, as the "max" size of the query buffer is actually
    ** one less than the size allocated as a space was reserved for this
    ** null byte sentinel.
    */
    if (cgc->cgc_m_state & IICGC_QTEXT_MASK)
    {
	qry_buf = &cgc->cgc_qry_buf;

	qry_buf->cgc_q_buf[qry_buf->cgc_q_used] = '\0';
	qry_buf->cgc_q_used++;
	dbv_qry.db_datatype = DB_QTXT_TYPE;
	dbv_qry.db_prec = 0;
	dbv_qry.db_length = qry_buf->cgc_q_used;
	dbv_qry.db_data = (PTR)qry_buf->cgc_q_buf;
	IIcc2_put_gcv(cgc, FALSE, &dbv_qry);
	qry_buf->cgc_q_used = 0;	/* Reset query text counter */
    }
    if (cgc->cgc_debug)
	(*cgc->cgc_debug)(IIDBG_END, (DB_DATA_VALUE *)0);
    _VOID_ IIcc1_send(cgc, TRUE);
} /* IIcgc_end_msg */

/*{
**  Name: IIcc1_send - Send a message buffer to server.
**
**  Description:
**	This routine is internal to the iicgc routines.
**
**	This routine uses the GCA_SEND service to send a complete or partial
**	message to the server.  This routine does not know the syntax of what
**	it is sending.  After sending the message buffer the buffer counters
**	are reset as at the start.
**
**	Before sending a message the internal GCA trace handler may be called
**	to generate message tracing.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	end_of_data	- TRUE/FALSE - is this buffer the last?
**
**  Outputs:
**	Returns:
**		STATUS
**	Errors:
**		E_LC0030_WRITE_SEND_FAIL - GCA SEND service failed on message.
**		E_LC0031_WRITE_ASSOC_FAIL - Database partner gone.
**
**  Side Effects:
**	1. Resets values of "write" message buffer.
** 	2. If there was a GCA_SEND error then we must suppress
** 	   the following read. We turn on the "last" mask, so that the
** 	   read routines will behave as though they were already read.
**    	3. If the communication channels closed down then this implicitly
**	   quits.
**	4. The trace generator may indirectly call outside to its emitting
**	   function.
**	
**  History:
**	18-aug-1986	- Written (ncg)
**	17-jul-1987	- Rewritten for GCA (ncg)
**	19-sep-1988	- Added calls to trace generator (ncg)
**	14-jun-1990	(barbara)
**	    Don't automatically quit on association fail errors.  Removed 
**	    call to IIcgc_disconnect. (bug 21832)
**      18-jan-99 (loera01) Bug 94914
**          Fill gca_protocol field of the trace control block if printgca 
** 	    tracing is turned on, and the caller is a child process.
*/

STATUS
IIcc1_send(cgc, end_of_data)
IICGC_DESC	*cgc;
bool		end_of_data;
{
    GCA_PARMLIST	*gca_parm;	/* General GCA parameter */
    GCA_SD_PARMS	*gca_snd;	/* GCA_SEND parameter */
    GCA_TR_BUF		gca_trb;	/* GCA message trace parameter */
    STATUS		gca_stat;	/* Status for IIGCa_call - unused */
    IICGC_MSG		*cgc_msg;	/* "Write" message buffer */
    IICGC_HARG		handle_arg;	/* Argument to handler */
    char		stat_buf[IICGC_ELEN + 1];	/* Status buffer */


    cgc_msg = &cgc->cgc_write_msg;

    /* If the message is an interrupt then use the interrupt parameter */
    if (cgc_msg->cgc_message_type == GCA_ATTENTION)
	gca_parm = &cgc_msg->cgc_intr_parm;
    else
	gca_parm = &cgc_msg->cgc_parm;

    /* GCA_SEND for message buffer */
    gca_snd = &gca_parm->gca_sd_parms;
    gca_snd->gca_association_id = cgc->cgc_assoc_id;	 	/* Input */
    gca_snd->gca_message_type   = cgc_msg->cgc_message_type;	/* Input */
    gca_snd->gca_buffer         = cgc_msg->cgc_buffer;	 	/* Input */
    gca_snd->gca_msg_length     = cgc_msg->cgc_d_used;	 	/* Input */
    gca_snd->gca_end_of_data    = (i4)end_of_data;		/* Input */
    gca_snd->gca_descriptor    = (PTR)cgc->cgc_tdescr;		/* Input */
    gca_snd->gca_status         = E_GC0000_OK;		 	/* Output */


    /* If tracing then generate internal trace data */
    if (cgc->cgc_trace.gca_emit_exit)
    {
        if (cgc->cgc_g_state & IICGC_CHILD_MASK)
            cgc->cgc_trace.gca_protocol = cgc->cgc_version;
	gca_trb.gca_service 		= GCA_SEND;
	gca_trb.gca_association_id 	= cgc->cgc_assoc_id;
	gca_trb.gca_message_type 	= cgc_msg->cgc_message_type;
	gca_trb.gca_d_length 		= cgc_msg->cgc_d_used;
	gca_trb.gca_data_area 		= cgc_msg->cgc_data;
	gca_trb.gca_descriptor 		= (PTR)cgc->cgc_tdescr;
	gca_trb.gca_end_of_data 	= (i4)end_of_data;
	IIcgct2_gen_trace(&cgc->cgc_trace, &gca_trb);
    }

    IIGCa_cb_call( &IIgca_cb, GCA_SEND, gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (gca_snd->gca_status != E_GC0000_OK)	/* Check for success */
    {
	if (gca_stat == E_GC0000_OK)
	    gca_stat = gca_snd->gca_status;
    }
    if (gca_stat != E_GC0000_OK)
    {
	IIcc3_util_status(gca_stat, &gca_snd->gca_os_status, stat_buf);
	if (gca_stat == E_GC0001_ASSOC_FAIL) 	/* Partner went away */
	{
	    IIcc1_util_error(cgc, FALSE, E_LC0031_WRITE_ASSOC_FAIL, 1,
			     stat_buf, (char *)0, (char *)0, (char *)0);
	}
	else
	{
	    char stat_buf3[30];
	    IIcc1_util_error(cgc, FALSE, E_LC0030_WRITE_SEND_FAIL, 3,
			     ERx("GCA_SEND"),
			     IIcc2_util_type_name(cgc_msg->cgc_message_type,
						 stat_buf3),
			     stat_buf, (char *)0);
	} /* if check error status */
	cgc->cgc_g_state |= IICGC_FASSOC_MASK;
	if ((cgc->cgc_m_state & IICGC_START_MASK) == 0)
	     _VOID_ (*cgc->cgc_handle)(IICGC_HFASSOC, &handle_arg);
	gca_stat = FAIL;
	cgc->cgc_m_state |= IICGC_LAST_MASK;	/* Suppress read */
    }
    else
    {
	gca_stat = OK;
    }  /* if status ok */

    /* 
    ** There is no need to reformat (GCA_FORMAT) the message buffer.  This
    ** only happens at the start of the session.
    */
    cgc_msg->cgc_d_used = 0;

    return gca_stat;
} /* IIcc1_send */

/*{
+*  Name: IIcgc_break - Interrupt a query.
**
**  Description:
**	This routine interrupts query processing in the server.
**
**	The routine initially sends the GCA_ATTENTION message, and any results
**	until the GCA_IACK reply are purged.  The "interrupt" type is used by
**	read_buffer in order to purge (and not process) data.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	breakval	- Value to send with interrupt.
**
**  Outputs:
**	Returns:
**	    	None
**		
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	17-sep-1986	- Written (ncg)
**	22-jul-1987	- Rewritten for GCA (ncg)
**	28-feb-1990	- Turn off COPY_MASK as part of fix to bug 7927 (bjb)
**	30-apr-1990	- "Reset" when interrupting alternate-stream. (ncg)
**	14-jun-1990	(barbara)
**	    With bug fix 21832 query processing may go on even if
**	    there is no connection.  Added check that cgc is non-null.
**	02-aug-1993 (kathryn)
**	    Don't loop indefinitely if server has gone away.
*/

VOID
IIcgc_break(cgc, breakval)
IICGC_DESC	*cgc;
i4		breakval;
{
    i4		interrupt_data;;

    /* Have we already left the server */
    if (cgc == (IICGC_DESC *)0 || cgc->cgc_g_state & IICGC_QUIT_MASK)
	return;

    /* Reset any alternate load/read processing */
    if (cgc->cgc_alternate)
	IIcga6_reset_stream(cgc);

    /* 
    ** Initialize the interrupt message, add the interrupt modifier, send
    ** the message.
    */
    IIcgc_init_msg(cgc, GCA_ATTENTION, DB_NOLANG, 0);
    interrupt_data = breakval;
    IIcc3_put_bytes(cgc, TRUE, sizeof(interrupt_data), (char *)&interrupt_data);
    IIcgc_end_msg(cgc);

    /* Read results until a GCA_IACK message */
    do {
	IIcc3_read_buffer(cgc, IICGC_NOTIME, GCA_NORMAL);
    } while ((cgc->cgc_result_msg.cgc_message_type != GCA_IACK) &&
	     !(cgc->cgc_g_state & IICGC_FASSOC_MASK));
    /* Ignore the data in the buffer */

    cgc->cgc_g_state &= ~IICGC_COPY_MASK;
} /* IIcgc_break */

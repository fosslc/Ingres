/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<cm.h>
# include	<tm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<adh.h>
# include	<gca.h>
# include	<fe.h>
# include	<ug.h>
# include	<iicgca.h>
# include	<eqrun.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>

/**
+*  Name: iidbg.c - Support for the "printqry", "savequery"  and "-d" features.
**
**  Description:
**	This file corresponds functionally to the 5.0 file iilastcom.c.
**	It handles saving and printing the text of the current query.
**	IIdbg_info is called by IIsync to track the current filename and line
**	number, IIdbg_gca is called indirectly from iicgcwrite.c to save the
**	current statement, IIdbg_dump is called by the default cgc_debug 
**	handler and dbg_end is called at the start of the next query
**	to invalidate the saved statement.
**
**	This file is also for handling the "printqry" feature. 
**	When "printqry" is set via II_EMBED_SET or SET_SQL, the current
**	query, along with query timing information is printed to a log file.
**	If the user has not specified a name for the log file, then the
**	default "iiprtqry.log" will be used.  In addition, if a "printqry" 
**	data handler has been registered, then the handler will also be invoked
**	with the query and timing information.
**	The II_EMBED_SET/SET_SQL "printqry" is a more useful and more 
**	efficient version of the ING_SET "set printqry" feature.  
**
**	This file also handles the "savequery/querytext" feature
**	which saves the last query sent to the server so the user
**	can inquire on it.  This feature uses the same method as
**	"printqry" to save away the query text.  In fact, the query
**	text in db_inbuf is used for both "printqry" and "savequery/
**	querytext".
**
**  Defines:
**	IIdbg_gca		- Save the current piece of the current query.
**	IIdbg_info		- Initialize; save file name and line number.
**	IIdbg_dump		- Print the text of the current statement.
**	IIdbg_getquery		- Get the last saved query text.
**	IIdbg_response		- Generate current time, cputime for printqry.
**	IIdbg_toggle		- Toggle state of printqry feature.
**	IILQqthQryTraceHdlr	- Register printqry output handler.
**  (locals)
**	dbg_file		- Open/close tracing log file.
**	dbg_text_add		- Save a textual piece of a query.
**	dbg_data_add		- Save a db_value piece of a query.
**	dbg_buf_get		- Allocate buffer for storing query.
**	dbg_end			- Clear the current statement buffer.
**
**  Notes:
**	One dynamically-allocated  buffer is available for storing the query;
**	at dump time it is copied to another, with long lines folded.
**	Excess text is discarded - this isn't a problem as the forms system
**	truncates messages to about 400 chars.
-*
**  History:
**	18-nov-1986	- Written. (mrw)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	16-dec-1987	- Support for II_EMBED_SET "printqry" feature (bjb)
**	06-sep-1988	- Added IIdbg_toggle for more isolated debugging (ncg)
**	22-may-1989	- Changed for multiple connections. (bjb)
**	22-jan-1990	- Fixed up LOfroms call to use a buffer. (bjb)
**	14-mar-1991	- Add support for saving last query text. (teresal)
**	10-apr-1991	- Add support for user specified "qryfile".
**	01-nov-1991	(kathryn)
**	    Changed the handling of PRINTQRY feature to output query trace
**	    prior to sending query to DBMS. IIdbg_dump will now be called
**	    indirectly from cgcwrite prior to sending query to DBMS.
**	29-nov-91 (seg)
**	    buf is declared as an ALIGN_RESTRICT, not char *.  Need to cast.
**	15-dec-1991 (kathryn)
**	    Added funtions IILQqthQryTraceHandler() and local dbg_buf_get().
**	    To Complete changes for W4GL debugger printqry data handler.
**	15-Jun-92 (seng/fredb)
**	    Integrated fredb's change of 22-Apr-91 from 6202p:
**      	22-apr-1991 (fredb) hp9_mpe
**                 Added MPE/XL specific code because we do not have the
**		   concept of filename extensions and we need to set specific
**		   file attributes using the extra parameters of SIfopen.
**	10-oct-92 (leighb) DeskTop Porting Change:
**	    Added extra space for iiprtgca.log name, because MS-WIN requires
**	    full pathnames (it's too stupid to have a workable default dir).
**	5-jan-1993 (donc)
**          Fix for bug 48625 - SIfprintf being called EVEN IF the log file
**	    couldn't be opened! Added a check for this.
**	07-oct-93 (sandyd)
**	    Added <fe.h>, which is now required by <ug.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	12-dec-96 (somsa01)
**	    Fix for SIR #78787; doubled size of DB_IBUF_SIZ to 2048 and
**	    DB_OBUF_SIZ to 2560.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      27-Oct-2003 (hanal04) Bug 111162 INGCBT499
**          Printing query text was dependant on the value of DB_IBUF_SIZ 
**          and DB_OBUF_SIZ as the maximum size of the buffers used to 
**          store and dump query text.
**          The buffers are now allocated dynamically and newer bigger 
**          buffers allocated if we have query text that exceeds the 
**          current buffer size.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*
NO_OPTIM = sos_us5
*/

/*
** Data
*/

#define	DB_LINE_MAX		79		/* Max output line length */
#define	DB_FNAM_MAX		32		/* Max length of a file name */
#define	DB_IBUF_SIZ		2048		/* Initial Max saved qry text */
#define DB_OBUF_RES		3		/* Reserved chars in outbuf */
#define	DB_OBUF_PAD		512 		/* pad size for qry text */
#define DB_OBUF_SIZ             DB_IBUF_SIZ+DB_OBUF_PAD
						/* Initial Max saved qry text */
#define DB_BANN_LEN		256		/* Max banner for debug msg */
#ifdef MVS
#define QRYTRCFILE	 ERx("DD:PRINTQRY")
#else
#  ifdef hp9_mpe
#    define QRYTRCFILE	 ERx("iiprtqry")
#  else
#    ifdef PMFEWIN3					 
#      define QRYTRCFILE ERx("iiprtqry.log                                                  ")
#    else
#      define QRYTRCFILE ERx("iiprtqry.log")
#    endif
#  endif
#endif
#define IITRCENDLN	ERx("------------------------------------------------\n")

/*}
+*  Name: DBG_BUF - Buffer the debugging info.
**
**  Description:
**	The query is saved in db_inbuf (allocated in IIdbg_info);
**	IIdbg_dump prints it, eliding ~V and ~Q, and folding long lines.
-*
**  History:
**	20-nov-1986	- Written. (mrw)
**	06-sep-1988	- Added db_savedprt. (ncg)
**	19-may-1989	- Multiple connection changes. Printqry file pointer
**			  and savedprt flag are now kept in IIglbcb. (bjb)
**	14-mar-1991	- Add support for saving last query text. (teresal)
**	10-apr-1991	- Add support for user specified trace file name.
**	01-nov-1991	- Add db_qry_done to dbg_buf structure.
**      27-Oct-2003 (hanal04) Bug 111162 INGCBT499
**          - db_inbuf is now a pointer to a dynamically allocated buffer.
**          - Added db_inbuf_size to record current size of db_inbuf.
**          - Added db_outbuf for dynamic allocation of output buffer
**            which previously lived in IIdbg_dump()
**          - Added db_outbuf_size to record current size of db_outbuf. This
**            size does not include the DB_OBUF_RES (currently 3) chars
**            which are reserved for NULLs, line feeds and double byte
**            handling.
**	
*/

typedef struct dbg_buf {
    char	*db_in_cur;			/* Next free spot in db_inbuf */
    i4		db_line_num;			/* Saved dbg line number */
    char	db_file_name[DB_FNAM_MAX+1];	/* Saved dbg file name */
    char	*db_inbuf;	                /* Saved query buffer */
    i4          db_inbuf_size;			/* Current size of db_inbuf */
    char        *db_outbuf;                     /* Output buffer */
    i4          db_outbuf_size;			/* Current size of db_outbuf */
						/* Does not include reserved */
    SYSTIME	db_begin_t;			/* Time of qry start */
    SYSTIME	db_resp_t;			/* Time of qry response */
    i4	db_cpu_resp;			/* CPU time at qry response */
    bool	db_done_resp;			/* TRUE if response time got */
    bool	db_qry_done;			/* Trace for this qry is done */
} DBG_BUF;
/* 
** IIlbqcb->ii_lq_dbg is our (DBG_BUF *).  ii_lq_dbg is in the local
** session control block rather than the global control block because
** of the following: a block structured query might have saved away part
** of the qry before flushing.  Session may be switched in the middle of
** the block and a new qry started.  We want to make sure that the text
** associated with the two queries is not intermixed in db_inbuf.
*/

static VOID	dbg_text_add( II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *dbv );
static VOID	dbg_add( II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *dbv );
static STATUS	dbg_buf_get( II_LBQ_CB *IIlbqcb );

/*
*/

static VOID
dbg_file( II_LBQ_CB *IIlbqcb, i4  action )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    FILE	*trace_file = (FILE *)qrytrc->ii_tr_file;
    char	*title = ERx("off");

    /*
    ** Check to see that we actually need 
    ** to open or close the trace file.
    */
    if ( (action == IITRC_ON  &&  trace_file)     ||
	 (action == IITRC_OFF  &&  ! trace_file)  ||
	 (action == IITRC_SWITCH  &&  ! trace_file) )
	return;

    if ( action == IITRC_SWITCH )  title = ERx("switched");

    if ( action == IITRC_ON )
    {
	LOCATION	loc;
	STATUS		stat;

	/*
	** If trace file name not provided, use default.
	*/
	if ( ! qrytrc->ii_tr_fname )
	    qrytrc->ii_tr_fname = STalloc( QRYTRCFILE );

	LOfroms( PATH & FILENAME, qrytrc->ii_tr_fname, &loc );
# ifdef hp9_mpe
	if ( qrytrc->ii_tr_flags & II_TR_APPEND )
	    stat = SIfopen( &loc, ERx("a"), SI_TXT, 252, &trace_file );
	else
	    stat = SIfopen( &loc, ERx("w"), SI_TXT, 252, &trace_file );
# else
	if ( qrytrc->ii_tr_flags & II_TR_APPEND )
	    stat = SIopen( &loc, ERx("a"), (FILE **)&trace_file );
	else
	    stat = SIopen( &loc, ERx("w"), (FILE **)&trace_file );
# endif
	if ( stat != OK )
	{
	    /* Take simplest error path; don't want IIlocerr functionality */
	    IIUGerr(E_LQ0007_PRINTQRY, 0, 1, qrytrc->ii_tr_fname);

	    /* 
	    ** We can't call IIdbg_toggle() here because
	    ** of possible conflicts with the tracing
	    ** semaphore, so just turn off tracing here.
	    */
	    qrytrc->ii_tr_flags &= ~II_TR_FILE;
	    return;
	}

	qrytrc->ii_tr_file = (PTR)trace_file;
	title = ERx("on");
    }

    SIfprintf( trace_file,
	       ERx("----------- printqry = %s (session %d)----------\n"),
	       title, IIlbqcb->ii_lq_sid );
    SIflush( trace_file );

    if ( action != IITRC_OFF )
	qrytrc->ii_tr_sid = IIlbqcb->ii_lq_sid;
    else
    {
	SIclose( trace_file );
	qrytrc->ii_tr_file = NULL;
	qrytrc->ii_tr_sid = 0;
	qrytrc->ii_tr_flags |= II_TR_APPEND;  /* Don't overwrite if reopened */
    }

    return;
}

/*{
+*  Name: dbg_end - Clear the current statement buffer and other fields.
**
**  Description:
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    None.
-*
**  Side Effects:
**	The text of the current statement is forgotten.
**	A timestamp flag is reset (relevant for printqry feature only).
**
**  History:
**	18-nov-1986 - Written. (mrw)
**	16-dec-1987 - Added support for II_EMBED_SET "printqry" feature (bjb).
**	02-may-1988 - Changed name to IIdbg_end from IIdbg_reset - check-7
**		      conflict with IIdbg_response. (ncg)
**	10-oct-92 (leighb) DeskTop Porting Change: added VOID return type.
*/

static VOID
dbg_end( II_LBQ_CB *IIlbqcb )
{
    DBG_BUF	*dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;

    if ( dbg_buf )
    {
	*dbg_buf->db_inbuf = '\0';
	dbg_buf->db_in_cur = dbg_buf->db_inbuf;
	*dbg_buf->db_file_name = '\0';
	dbg_buf->db_cpu_resp = 0;
	dbg_buf->db_done_resp = FALSE;
	dbg_buf->db_qry_done = FALSE;
    }

    return;
}

/*{
+*  Name: IIdbg_info - Save the current file name and line number.
**
**  Description:
**	For handling the "-d" preprocessor debugging flag, this routine is 
**	called by IIsync to save the file name and line number of the current 
**	statement.  It is also called by the routines in iisndlog.c for
**	handling the runtime equivalent of the "-d" flag and also the
**	II_EMBED_SET/SET_SQL  "printqry" feature.  Note that the latter feature
**	overrides the "-d" feature.
**	If we can't allocate a buffer for saving the query, we don't
**	complain; nobody will try to save into a non-existent buffer.
**	For the printqry feature, a logfile in the local directory is opened.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	filenm		- The name of the current source file.
**	lineno		- The number of the current line.
**
**  Outputs:
**	Returns:
**	    Nothing.
**	Errors:
**	    
-*
**  Side Effects:
**	May allocate the saved query buffer.
**  History:
**	18-nov-1986 - Written. (mrw)
**	19-may-1989 - Changed names of globals for multiple connects. (bjb)
**	10-oct-1989 - Porting change for MVS -- print query log file. (bjb)
**	10-apr-1991 (kathryn)
**	    Added support for user specified file name for "printqry" output.
**	    Check ii_gl_qryfile for user specified file name set via:
**	    II_EMBED_SET "qryfile <filename>" or SET_SQL (qryfile = 'filename')
**	    If qryfile was not specified, then set ii_gl_qryfile to default 
**	    file name "iiprtqry.log".
**	15-apr-1991 (kathryn)
**	    If II_G_APPTRC flag is set then open printqry file in append mode.
**	    A child process has been called and we want its tracing to be
**	    appended to the file of its parent.
**	01-nov-1991 (kathryn)
**          Made changes for trace file information now stored in new
**          IILQ_TRACE structure of IIglbcb rather than directly in IIglbcb.
**	15-dec-1991 (kathryn)
**	    Move code from this routine to new routine dbg_buf_get(). Needed
**	    so that query buffer may be allocated from IIdbg_toggle as well,
**	    when printqry output handler is registered.
*/

VOID
IIdbg_info( II_LBQ_CB *IIlbqcb, char *filenm, i4  lineno )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    DBG_BUF	*dbg_buf;

    MUp_semaphore( &IIglbcb->ii_gl_sem );

    if ( ! filenm  ||  IIglbcb->ii_gl_flags & II_G_SAVQRY  ||
	 qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR) )
    {
	MUv_semaphore( &IIglbcb->ii_gl_sem );
	return;
    }

    IIglbcb->ii_gl_flags |= II_G_DEBUG;
    MUv_semaphore( &IIglbcb->ii_gl_sem );

    if ( dbg_buf_get( IIlbqcb ) == FAIL )  
    {
	if ( IIlbqcb->ii_lq_gca )  IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	return;
    }

    dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;

    /* Trim the name -- might be padded (eg, COBOL). */
    _VOID_ IIstrconv( II_TRIM, filenm, dbg_buf->db_file_name, DB_FNAM_MAX );
    dbg_buf->db_line_num = lineno;

    return;
}

/*{
+*  Name: IIdbg_gca - Save the current piece of the current statement.
**
**  Description:
**	This routine is the one pointed at by cgcwrite.c.  It is called
**	indirectly through IIlbqcb->ii_lq_gca->cgc_debug when debugging 
**	(-d flag) or the "printqry" or "savequery" features are on from: 
**	    IIcgc_init_msg - action IIDBG_INIT -  Beginning new query.
**	    IIcgc_put_msg  - action IIDBG_ADD  -  Adding query text to message.
**	    IIcgc_end_msg  - action IIDBG_END  -  Sending Query to DBMS
**
**	For the printqry feature: 
**	 If "action" is IIDBG_END, (called from IIcgc_end_msg just prior to
**	 sending query to DBMS), we get the current time, representing the 
**	 time the query was sent to the DBMS, and call IIdbg_dump to dump the 
**	 query text to the printqry log file.  In addition, IIdbg_dump will
**	 invoke the query text output handler if one is registered.
**	 
**
**  Inputs:
**	action	- IIDBG_INIT - New query, re-initialize query trace buffer and
**			       save the current time.
**		  IIDBG_ADD  - Add text to current query trace buffer.
**		  IIDBG_END  - Query text is complete - Dump the query text.
**
**	dbv	- DB data value with the following information: 
**		  This is a null pointer if the query is not even
**		  debuggable (ie, non-user-style message).  This need
**		  only be checked when "start" is true.
**		  If type is DB_QTXT_TYPE then this is query string
**		  data that should be added (length >= 0).
**		  Any other type is a data value parameter.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None.
-*
**  Side Effects:
**	Stores the query in a dynamic buffer.
**  History:
**	18-nov-1986 - Written. (mrw)
**	26-aug-1987 - Modified to use GCA. (ncg)
**	26-dec-1987 - Added support for printqry (bjb)
**	19-may-1989 - Changed names of globals for multiple connects. (bjb)
**	01-nov-1991   (kathryn)
**	    Changed first parameter to i4  rather than boolean for the
**	    addition "printqry" action IIDBG_END.  Query text will now be
**	    printed/handled prior to sending query to DBMS. 
*/

VOID
IIdbg_gca( i4  action, DB_DATA_VALUE *dbv )
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    DBG_BUF	*dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;

    if ( dbg_buf )
	if ( action == IIDBG_END )
	{
	    if ( qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR) )
	    {
		TMnow( &dbg_buf->db_begin_t);
		IIdbg_dump( IIlbqcb );
	    }
	}	
	else 
	{
	    if ( action == IIDBG_INIT )  dbg_end( IIlbqcb );
	    if ( dbv )  dbg_add( IIlbqcb, dbv );
	}

    return;
}

/*{
+*  Name: IIdbg_dump - Print the text of the current statement.
**
**  Description:
**	This routine is called to print the current query text if the
**	"-d" debugging feature or the "printqry" feature is on.
**	If debugging, we are being called from iierror.c; if printqry, we
**	are being called from  IIqry_read() or IIqry_end().
**	For debugging only, print the header line containing the file name and 
**	line number (saved by IIdbg_info).  Then walk through the saved text 
**	looking for the end of the saved text.  Print the current text and data.
**	Iterate until the end of the saved text.  Fold long lines and turn
**	control characters into spaces.  We handle control chars here rather
**	than in dbg_text_add just because it seems simpler to handle output
**	formatting in the output routine (we have to fold long lines anyway,
**	and this clearly seems like the correct place for that).  
**
**	For the printqry feature, if set via II_EMBED_SET or SET_SQL then print 
**	the query text to a logfile.  In addition, if a printqry data handler
**	is registered, then invoke the handler with the query text.  Note that
**	if printqry tracing was turned on by both the user and by the 
**	registration of a trace data handler, then trace output will go to
**	both the logfile and the handler.
**
**	The output buffer is DB_OBUF_SIZ+3 because we need to add a newline
**	and a nul at the end, but if the last char was Kanji and it was at
**	outbuf[DB_OBUF_SIZ-1] we'll write the second byte past DB_OBUF_SIZ.
**
**  Inputs:
**	IIlbqcb		Current session control block.
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
**	18-nov-1986 - Written. (mrw)
**	16-dec-1987 - Added support for "printqry" (bjb)
**	02-feb-1988 - Modified handling of control chars. (ncg)
**	19-may-1989 - Changed names of globals for multiple connects. (bjb)
**	15-apr-1991  (kathryn)
**	    When file for "printqry" trace is not open, but tracing has been
**	    turned on, Call IIdbg_info to open the file.
**	01-nov-1991  (kathryn)
**	    For "printqry" only dump query text here.  The query timing info
**	    will now be dumped by IIdbg_response after the query has completed.
**	15-dec-1991  (kathryn)
**	    If a printqry output handler is registerd, then invoke it.
**	19-dec-1991 (kathryn)
**	    Reset db_logfile to ii_tr_file after call to IIdbg_info.
**      27-Oct-2003 (hanal04) Bug 111162 INGCBT499
**          Moved outbuf buffer into the DBG_BUF structure so that
**          increases in the size of db_inbuf could be matched with
**          dynamic allocations of db_outbuf.
**	02-May-2005 (bonro01)
**	    Need to allow room for 2-bytes for '\n' and '\0'
*/

VOID
IIdbg_dump( II_LBQ_CB *IIlbqcb )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    DBG_BUF	*dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;
    i4	gl_flags = IIglbcb->ii_gl_flags;
    char	*outbuf;
    char	*o_ptr;
    char	*i_ptr;
    i4		ch_pos;

    /* Just return if there are no errors or tracing not enabled */
    if ( 
	 ! dbg_buf  ||  *dbg_buf->db_inbuf == EOS  ||
	 ! (gl_flags & II_G_DEBUG  ||  
	    qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR)) 
       )  
	return;

    /* Initialise after we have checked for a null dbg_buf */
    outbuf = dbg_buf->db_outbuf;
    o_ptr = outbuf;

    if ( ! (gl_flags & II_G_DEBUG) )
	STcopy( ERx("Query text:"), outbuf );	/* Query tracing */
    else
    {
        /* Output file name and line number. */
        if ( ! *dbg_buf->db_file_name )
	    IIUGfmt( outbuf, DB_BANN_LEN, ERget(S_LQ0202_DEBUG3), 0 );
	else  if ( dbg_buf->db_line_num <= 0 )
	    IIUGfmt( outbuf, DB_BANN_LEN, ERget(S_LQ0201_DEBUG2), 1,
		     dbg_buf->db_file_name );
        else
	{
	    i4 linenum = dbg_buf->db_line_num;
	    IIUGfmt( outbuf, DB_BANN_LEN, ERget(S_LQ0200_DEBUG1),
		     2, dbg_buf->db_file_name, &linenum );	
	}
    }

    STcat( outbuf, ERx("\n  ") );
    o_ptr = outbuf + STlength(outbuf);
    ch_pos = 2;

    for (i_ptr = dbg_buf->db_inbuf; i_ptr < dbg_buf->db_in_cur;)
    {
	switch (*i_ptr)
	{
	  case '\0':
	    *i_ptr = '?';
	    ch_pos++;
	  case '\n':
	    ch_pos = 0;
	    break;
	  default:
	    if (CMcntrl(i_ptr))
		*i_ptr = '.';
	    CMbyteinc(ch_pos, i_ptr);
	    break;
	}
	if (o_ptr-outbuf+2 < dbg_buf->db_outbuf_size)
	{
	    CMcpychar(i_ptr, o_ptr);
	    CMnext( o_ptr );
	}
	CMnext( i_ptr );
	if (ch_pos >= DB_LINE_MAX)
	{
	    if (o_ptr-outbuf+2 < dbg_buf->db_outbuf_size)
	    {
		*o_ptr++ = '\n';
		ch_pos = 0;
	    }
	    if (o_ptr-outbuf+2 < dbg_buf->db_outbuf_size)
	    {
		*o_ptr++ = ' ';
		ch_pos++;
	    }
	    if (o_ptr-outbuf+2 < dbg_buf->db_outbuf_size)
	    {
		*o_ptr++ = ' ';
		ch_pos++;
	    }
	}
    }

    *o_ptr++ = '\n';
    *o_ptr++ = '\0';

    if ( gl_flags & II_G_DEBUG )
	IImsgutil( outbuf );
    else  if ( MUp_semaphore( &qrytrc->ii_tr_sem ) == OK )
    {
	FILE	*trace_file;
	char	timebuf[100];
	char	hdlr_buf[150];
	i4	newline;

	/* Write query text, query begin time and CPU time */
	TMstr( &dbg_buf->db_begin_t, timebuf );
        newline = STlength( timebuf );
        if ( timebuf[ newline - 1 ] == '\n' )  timebuf[ newline - 1 ] = '\0';

	if ( qrytrc->ii_tr_flags & II_TR_HDLR )
	{
	    IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, outbuf, FALSE );
	    STpolycat( 3, ERx("Query Send Time:       "),
		       timebuf, ERx("\n"), hdlr_buf );
	    IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, hdlr_buf, FALSE );
	}

	if ( qrytrc->ii_tr_flags & II_TR_FILE )
	{
	    /* 
	    ** Tracing is turned on but the file is closed - it may be 
	    ** that we are returning from a child process. The child 
	    ** process closed the file so we need to re-open it for the
	    ** parent.
	    */
            if ( ! qrytrc->ii_tr_file )  dbg_file( IIlbqcb, IITRC_ON );

	    if ( (trace_file = (FILE *)qrytrc->ii_tr_file) )
	    {
		/*
		** Check to see if the session has been switched
		** since the last trace was written.
		*/
	        if ( qrytrc->ii_tr_sid != IIlbqcb->ii_lq_sid )
		    dbg_file( IIlbqcb, IITRC_SWITCH );

		/* Write query text, query begin time and CPU time */
		SIfprintf( trace_file, ERx("%s"), outbuf );	
		SIfprintf( trace_file, 
			   ERx("Query Send Time:       %s\n"), timebuf );
		SIflush( trace_file );
	    }
	}

	MUv_semaphore( &qrytrc->ii_tr_sem );
    }

    return;
}

/*{
+*  Name: IIdbg_getquery - Get last saved query text.
**
**  Description:
**	This routine is called to get the text of the last query 
**	sent to the server, i.e., "INQUIRE_INGRES (:svar = QUERYTEXT)".
**
**	Do only a small massage of the query text to eliminate any
**	control characters.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	dbv		- The data value to put query text into.
**
**  Outputs:
**	Returns:
**	    STATUS	- Indicates if a query text was sucessfully
**			  retrieved. Values returned are: 
**			    OK   - Query text sucessfully retrieved
**			    FAIL - No query text found - buffer was empty
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	
**  History:
**	15-mar-1991 - Written. (teresal)
*/

STATUS
IIdbg_getquery( II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *dbv )
{
    char	*i_ptr;
    DBG_BUF	*dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;

    /* Just return FAIL if there are is no query in the buffer */
    if ( dbg_buf == NULL || *dbg_buf->db_inbuf == '\0' )  return( FAIL );

    /* Eliminate any control characters */
    for( i_ptr = dbg_buf->db_inbuf; i_ptr < dbg_buf->db_in_cur; )
    {
	if ( *i_ptr == '\0' )
	    *i_ptr = '?';
	else  if ( CMcntrl( i_ptr ) )
	    *i_ptr = '.';
	CMnext( i_ptr );
    }

    *i_ptr = '\0';
    dbv->db_data = dbg_buf->db_inbuf;
    dbv->db_length = STlength( (char *)dbv->db_data );

    return( OK );
}

/*{
**  Name: IIdbg_response - Generate query timing information for printqry. 
**
**  Description:
** 	This routine is called to generate query timing information for the
**	"printqry" feature.
**
**	For queries that retrive data:
**	This routine is called by IIqry_read() to generate the "Query Response
**	Time" and "Response CPU Time" which is the time that the first piece 
**	of data is returned from the DBMS.  It is called by IIqry_end() to 
**	generate the "Qry End CPU Time".
**
**	For queries that do not retrieve data:
**	This routine is called by IIqry_end() to generate the 
**	"Query Response Time" which is the time that the query completes, and 
**      the "Qry End CPU Time".  
**
**	When "printqry" has been set via II_EMBED_SET or SET_SQL, the query
**	timing information is appended to the specified "printqry" log file.
**	
**	In addition if a printqry trace output handler has been registered
**	it will be invoked here.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	None
**	
**  History:
**	16-dec-1987 - written (bjb)
**	01-nov-1991   (kathryn)
**	    Handle the query timing information in this routine rather than 
**	    saving it away for IIdbg_dump().  Add qry_end as parameter, 
**	    specifying whether or not query has fully completed.
*/

VOID
IIdbg_response( II_LBQ_CB *IIlbqcb, bool qry_end )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    DBG_BUF	*dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;

    if ( ! dbg_buf  ||  dbg_buf->db_qry_done  ||
         ! (qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR))  ||
	 MUp_semaphore( &qrytrc->ii_tr_sem ) != OK ) 
	return;

    if ( ! dbg_buf->db_done_resp )
    {
	char	timebuf[100];
	i4	newline;

        TMnow( &dbg_buf->db_resp_t );
        TMstr( &dbg_buf->db_resp_t, timebuf );
        newline = STlength( timebuf );
        if ( timebuf[ newline - 1 ] == '\n' )  timebuf[ newline - 1 ] = '\0';

        /* Call registered query trace output handler */
        if ( qrytrc->ii_tr_flags & II_TR_HDLR )
        {
	    char hdlr_buf[150];
	    char hdlr_cpu[10];

	    STpolycat( 3, ERx("Query Response Time:   "), 
		       timebuf, ERx("\n"), hdlr_buf );
	    IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, hdlr_buf, FALSE );

	    if ( ! qry_end )
	    {
	       STprintf( hdlr_cpu, ERx("%d"), TMcpu() );
	       STpolycat( 3, ERx("Response CPU Time:     "), 
			  hdlr_cpu, ERx("\n"), hdlr_buf );
	       IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, hdlr_buf, FALSE );
	    }
        }

        /* Print query timing information to printqry log file */
 	if ( qrytrc->ii_tr_flags & II_TR_FILE )
	{
	    FILE *trace_file;

	    /* 
	    ** Tracing is turned on but the file is closed - it may be 
	    ** that we are returning from a child process. The child 
	    ** process closed the file so we need to re-open it for the
	    ** parent.
	    */
 	    if ( ! qrytrc->ii_tr_file )  dbg_file( IIlbqcb, IITRC_ON );

	    if ( (trace_file = (FILE *)qrytrc->ii_tr_file) )
	    {
		/*
		** Check to see if the session has been switched
		** since the last trace was written.
		*/
	        if ( qrytrc->ii_tr_sid != IIlbqcb->ii_lq_sid )
		    dbg_file( IIlbqcb, IITRC_SWITCH );

		SIfprintf( trace_file , 
			   ERx("Query Response Time:   %s\n"), timebuf );
		if ( ! qry_end )  
		    SIfprintf( trace_file,
			       ERx("Response CPU Time:     %d\n"),TMcpu() );
		SIflush( trace_file );
	    }
	}

	dbg_buf->db_done_resp = TRUE;
    }

    if ( qry_end )
    {
	if ( qrytrc->ii_tr_flags & II_TR_HDLR )
	{
	    char hdlr_buf[150];
	    char hdlr_cpu[10];

	    STprintf( hdlr_cpu, ERx("%d"), TMcpu() ); 
	    STpolycat( 3, ERx("Qry End CPU Time:      "), 
		       hdlr_cpu, ERx("\n"), hdlr_buf );
	    IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, hdlr_buf, FALSE );
	    IILQcthCallTraceHdlrs( qrytrc->ii_tr_hdl_l, IITRCENDLN, FALSE );
	}

	if ( qrytrc->ii_tr_flags & II_TR_FILE )
        {
	    FILE *trace_file;

	    /* 
	    ** Tracing is turned on but the file is closed - it may be 
	    ** that we are returning from a child process. The child 
	    ** process closed the file so we need to re-open it for the
	    ** parent.
	    */
 	    if ( ! qrytrc->ii_tr_file )  dbg_file( IIlbqcb, IITRC_ON );

	    if ( (trace_file = (FILE *)qrytrc->ii_tr_file) )
	    {
		/*
		** Check to see if the session has been switched
		** since the last trace was written.
		*/
	        if ( qrytrc->ii_tr_sid != IIlbqcb->ii_lq_sid )
		    dbg_file( IIlbqcb, IITRC_SWITCH );

		SIfprintf( trace_file, 
			   ERx("Qry End CPU Time:      %d\n"), TMcpu() );
		SIfprintf( trace_file, IITRCENDLN );
		SIflush( trace_file );
	    }
	}

	dbg_buf->db_qry_done = TRUE;
    }

    MUv_semaphore( &qrytrc->ii_tr_sem );

    return;
}

/*{
**  Name: IIdbg_toggle - Toggle internal state of "printqry".
**
**  Description:
**	The SET_INGRES statement has been issued to turn on/off the tracking
**	of queries.  This allows the tracing to be done around an isolated
**	set of queries.  This routine is also called whenever a printqry
**	trace output handler has been registered/unregistered.  Note that
**	registration has no effect on the tracing currently active due to
**	II_EMBED_SET or SET_SQL.  If printqry tracing was turned on by both
**  	the user and the registration of an trace data handler, then trace
**	data will go to both the log file and the handler.
**
**	When the state is off (IITRC_OFF), we check:
**		if printqry then
**		   flush "-- printqry = off --";
**		   turn off printqry;
**		   turn off cgc_debug handler (IIdbg_gca);
**		   turn on "saved printqry";
**		end if;
**
**	When the state is on (IITRC_ON), we check:
**		if saved printqry then
**		   flush "-- printqry = on --";
**		   turn on printqry;
**		   turn on cgc_debug handler (IIdbg_gca);
**		   turn off "saved printqry";
**		else
**		   start up debugging;
**		end if;
**
**	When the state if on (IITRC_ON), we check:
**		  
**	If the session id passed in is not equal to zero, we include a
**	session id in the above banners. 
**
**	Note that setting tracing on or off takes no effect until 
**	a database connection has been made (ii_lq_gca will be null
**	until connection time).
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	flags
**	  IITRC_ON 	- Setting of new printqry state.
**	  IITRC_OFF	- Turn off printqry to file.
**	  IITRC_HDLON	- Turn on printqry to registered trace output handler.
**	  IITRC_HDLOFF	- Turn off printqry to registered trace output handler.
**	  IITRC_SWITCH
**	  IITRC_RESET	- Reset cgc_debug so that the state belonging to
**			  a multiple connection turns printqry/savequery 
**			  off.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	Resets state of some internal debug fields.
**
**  Notes:
**      The flags in the IILQ_TRACE structure of the global control 
**	block (IIglbcb->qrytrc.ii_tr_flags) specify how trace output 
**	should be handled.  Query text tracing is enabled when either
**	of the following flags is set.
**          II_TR_FILE  -  Set via II_EMBED_SET toggled via SET_SQL.
**                         Write trace information to specified log file.
**          II_TR_HDLR  -  Toggled via [de]registration of trace data handler.
**                         Invoke trace output handler with trace data.
-*
**  History:
**	06-sep-1988 - written (ncg)
**	19-may-1989 - Changed names of globals for multiple connects and. 
**		      added session id information. (bjb)
**	14-mar-1991 - Fixed "-d" flag (and ERRORQRY) so that DEBUG state,
**		      if set, doesn't get toggled off when session
**		      is switched. (teresal)
**	14-mar-1991 - Added savequery feature. (teresal)
**	01-nov-1991  (kathryn)
**          Made changes for trace file information now stored in new
**          IILQ_TRACE structure of IIglbcb rather than directly in IIglbcb.
**	15-dec-1991  (kathryn)
**	    Add support for W4GL debugger trace data handler.  This routine
**	    will be called whenever a handler is registered/un-registered.
*/

VOID
IIdbg_toggle( II_LBQ_CB *IIlbqcb, i4  flags )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    i4	gl_flags = IIglbcb->ii_gl_flags;

    /*
    ** This should work, so just ignore request if fails.
    */
    if ( MUp_semaphore( &qrytrc->ii_tr_sem ) != OK )  return;
	
    switch( flags )
    {
	case IITRC_ON :
	    qrytrc->ii_tr_flags |= II_TR_FILE;
	    MUp_semaphore( &IIglbcb->ii_gl_sem );
	    IIglbcb->ii_gl_flags &= ~II_G_DEBUG;	/* Override "-d" */
	    MUv_semaphore( &IIglbcb->ii_gl_sem );
	    if ( dbg_buf_get( IIlbqcb ) == OK )
	    {
		if ( IIlbqcb->ii_lq_gca )
		    IIlbqcb->ii_lq_gca->cgc_debug = IIdbg_gca;
	    }
	    else  if ( IIlbqcb->ii_lq_gca )
		    IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    break;

	case IITRC_OFF :
	    qrytrc->ii_tr_flags &= ~II_TR_FILE;
	    dbg_file( IIlbqcb, IITRC_OFF );

	    /* Turn off tracing only if savequery is off too */
	    if ( IIlbqcb->ii_lq_gca  &&  ! (gl_flags & II_G_SAVQRY)  &&
		 ! (qrytrc->ii_tr_flags & II_TR_HDLR) )
		IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    break;

	case IITRC_HDLON :
	    if ( qrytrc->ii_num_hdlrs )
	    {
                qrytrc->ii_tr_flags |= II_TR_HDLR;
		MUp_semaphore( &IIglbcb->ii_gl_sem );
		IIglbcb->ii_gl_flags &= ~II_G_DEBUG;	/* Override "-d" */
		MUv_semaphore( &IIglbcb->ii_gl_sem );
		if ( dbg_buf_get( IIlbqcb ) == OK )
		{
		    if ( IIlbqcb->ii_lq_gca )
			IIlbqcb->ii_lq_gca->cgc_debug = IIdbg_gca;
		}
		else  if ( IIlbqcb->ii_lq_gca )
			IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    }
	    break;

	case IITRC_HDLOFF :
	    if ( ! qrytrc->ii_num_hdlrs )  
	    {
		qrytrc->ii_tr_flags &= ~II_TR_HDLR;

		/* Turn off tracing only if savequery is off too */
		if ( IIlbqcb->ii_lq_gca  &&  ! (gl_flags & II_G_SAVQRY)  &&
		     ! (qrytrc->ii_tr_flags & II_TR_FILE) )
		    IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    }
	    break;

	case IITRC_SWITCH :
	    if ( qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR)  ||
		 gl_flags & II_G_SAVQRY )
	    {
		if ( dbg_buf_get( IIlbqcb ) == OK )
		{
		    if ( IIlbqcb->ii_lq_gca )
			IIlbqcb->ii_lq_gca->cgc_debug = IIdbg_gca;
		}
		else  if ( IIlbqcb->ii_lq_gca )
		    IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    }
	    else  
	    {
		if ( gl_flags & II_G_DEBUG  &&  dbg_buf_get( IIlbqcb ) == OK )
		{
		    DBG_BUF *dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;
		    *dbg_buf->db_file_name = EOS;
		    dbg_buf->db_line_num = 0;
		}

		if ( IIlbqcb->ii_lq_gca )
		    IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    }
	    break;

	case IITRC_RESET :
	    dbg_end( IIlbqcb );
	    dbg_file( IIlbqcb, IITRC_OFF );
	    break;
    }

    MUv_semaphore( &qrytrc->ii_tr_sem );

    return;
} /* IIdbg_toggle */

/*{
**  Name: IIdbg_save
**
**  Description:
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	flags
**			IITRC_ON 	- Setting of new printqry state.
**			IITRC_OFF	- Turn off printqry to file.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
**
**  Side Effects:
**	Resets state of some internal debug fields.
-*
**  History:
*/

VOID
IIdbg_save( II_LBQ_CB *IIlbqcb, i4  flags )
{
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;

    switch( flags )
    {
	case IITRC_ON :
	    MUp_semaphore( &IIglbcb->ii_gl_sem );
	    IIglbcb->ii_gl_flags |= II_G_SAVQRY;
	    IIglbcb->ii_gl_flags &= ~II_G_DEBUG;	/* Override "-d" */
	    MUv_semaphore( &IIglbcb->ii_gl_sem );
	    if ( dbg_buf_get( IIlbqcb ) == OK )
	    {
		if ( IIlbqcb->ii_lq_gca )
		    IIlbqcb->ii_lq_gca->cgc_debug = IIdbg_gca;
	    }
	    else  if ( IIlbqcb->ii_lq_gca )
		IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    break;
	
	case IITRC_OFF :
	    MUp_semaphore( &IIglbcb->ii_gl_sem );
	    IIglbcb->ii_gl_flags &= ~II_G_SAVQRY;
	    MUv_semaphore( &IIglbcb->ii_gl_sem );
	    if ( IIlbqcb->ii_lq_gca  &&
		 ! (qrytrc->ii_tr_flags & (II_TR_FILE | II_TR_HDLR)) )
		IIlbqcb->ii_lq_gca->cgc_debug = NULL;
	    break;
    }

    return;
} /* IIdbg_save */


/*
** Local routines
*/

/*{
+*  Name: dbg_add - Save a db_value piece of a query.
**
**  Description:
**	Convert to data value to a text string and save it as text.
**
**  Inputs:
**	IIlbqcbq	Current session control block.
**	dbv		- The data value to save.
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
**	18-nov-1986 - Written. (mrw)
**	26-aug-1987 - Modified for QTXT. (ncg)
**	22-may-1989 - Modified names of globals for multiple connects. (bjb)
**	06-feb-1991 - Modified to support decimal - fixed Bus error
**		      by declaring LTXT buffer on a aligned boundary. 
**		      (teresal)
**	11-feb-1991 - Took out ifdef BYTE_ALIGN for previous fix - not
**		      needed. (teresal)
**	29-nov-91 (seg)
**	    buf is declared as an ALIGN_RESTRICT, not char *.  Need to cast.
**	09-dec-92 (teresal)
**	    Modified ADF control block reference to be session specific
**	    instead of global (part of decimal changes).
**	15-jan-1993 (kathryn)
**	    If type is DB_LVCH_TYPE then return - just the ADP peripheral 
**	    header is being sent.  The value of the first segment will be
**	    logged later when it is transmitted as CHA/CHR/VCH type.
**	25-sep-1993 (kathryn)
**	    Add Printqry/Savequery support for DB_{BYTE,VBYTE,LBYTE}_TYPE.
**	    Added DB_LBYTE_TYPE which will behave the same as DB_LVCH_TYPE,
**	    defined above.  Added and DB_BYTE_TYPE and DB_VBYTE_TYPE which 
**	    will be traced like DB_CHA_TYPE, and  DB_VCH_TYPE respectively.
**	01-oct-1993 (kathryn)
**	    Values for long datatypes DB_L{BYTE,BIT,LVCH}_TYPE will no longer
**	    be traced. The string literal "<value not traced>" will be generated
**	    in place of the data value.
**      26-Feb-2010 (hanal04) Bug 123353
**          Add support for printing dates, intervals and timestamps.
*/

static VOID
dbg_add( II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *dbv )
{
    /*
    ** Declare buffer to store long text string for result of integer, 
    ** money, float, decimal number, date, interval, or timestamp
    ** conversion to ascii: make it big enough to fit maximum TSW
    ** string length.
    */
    ALIGN_RESTRICT	buf[(ADH_EXTLEN_TSW / sizeof(ALIGN_RESTRICT)) + 1];

    DB_DATA_VALUE	dbval, dbtmp, dbspace, dbquote, dbnumtxt;
    char		*quote = IIlbqcb->ii_lq_dml == DB_SQL ?
					ERx("'") : ERx("\"");
    DB_TEXT_STRING	*txt;		/* For type TEXT */

    if (IIDB_LONGTYPE_MACRO(dbv))
    {
	II_DB_SCAL_MACRO( dbval, DB_CHR_TYPE, 18, ERx("<value not traced>"));
	dbg_text_add( IIlbqcb, &dbval );
	return;
    }
    if (ADH_ISNULL_MACRO(dbv))
    {
	II_DB_SCAL_MACRO( dbval, DB_CHR_TYPE, 4, ERx("NULL ") );
	dbg_text_add( IIlbqcb, &dbval );
	return;
    }
    /* Do work for NULLS too */
    dbtmp.db_datatype = abs(dbv->db_datatype);
    dbtmp.db_prec = dbv->db_prec;
    dbtmp.db_length =
	    dbv->db_datatype > 0 ? dbv->db_length : dbv->db_length - 1;
    dbtmp.db_data = dbv->db_data;

    II_DB_SCAL_MACRO( dbspace, DB_CHR_TYPE, 1, ERx(" ") );
    II_DB_SCAL_MACRO( dbquote, DB_CHR_TYPE, 1, quote );

    switch (dbtmp.db_datatype)
    {
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
      case DB_MNY_TYPE:
      case DB_DEC_TYPE:
	/*
	** Set up result DB_DATA_VALUE to hold numeric text string
	** then convert numeric data to a text string via afe.
	*/
        II_DB_SCAL_MACRO( dbnumtxt, DB_LTXT_TYPE, sizeof(buf), buf );
	if (afe_dpvalue(IIlbqcb->ii_lq_adf, &dbtmp, &dbnumtxt) != OK)
	{
	    ((char *)buf)[0] = '?';
	    ((char *)buf)[1] = '\0';
	    break;
	}
	txt = (DB_TEXT_STRING *)dbnumtxt.db_data;
	II_DB_SCAL_MACRO( dbval, DB_CHR_TYPE, txt->db_t_count, txt->db_t_text );
	dbg_text_add( IIlbqcb, &dbval );
	dbg_text_add( IIlbqcb, &dbspace );
	return;
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_BYTE_TYPE:
	dbg_text_add( IIlbqcb, &dbquote );
	dbg_text_add( IIlbqcb, &dbtmp );
	dbg_text_add( IIlbqcb, &dbquote );
	dbg_text_add( IIlbqcb, &dbspace );
	return;
      case DB_DTE_TYPE:
      case DB_ADTE_TYPE:
      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
        II_DB_SCAL_MACRO( dbnumtxt, DB_LTXT_TYPE, sizeof(buf), buf );
        if (afe_dpvalue(IIlbqcb->ii_lq_adf, &dbtmp, &dbnumtxt) != OK)
        {
	    ((char *)buf)[0] = '?';
	    ((char *)buf)[1] = '\0';
            break;
        }
        txt = (DB_TEXT_STRING *)dbnumtxt.db_data;
        II_DB_SCAL_MACRO( dbval, DB_CHR_TYPE, txt->db_t_count, txt->db_t_text );
        dbg_text_add( IIlbqcb, &dbquote );
        dbg_text_add( IIlbqcb, &dbval );
        dbg_text_add( IIlbqcb, &dbquote );
        dbg_text_add( IIlbqcb, &dbspace );
        return;
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
	txt = (DB_TEXT_STRING *)dbtmp.db_data;
	dbg_text_add( IIlbqcb, &dbquote );
	II_DB_SCAL_MACRO( dbval, DB_CHR_TYPE, txt->db_t_count, txt->db_t_text );
	dbg_text_add( IIlbqcb, &dbval );
	dbg_text_add( IIlbqcb, &dbquote );
	dbg_text_add( IIlbqcb, &dbspace );
	return;
      case DB_QTXT_TYPE:
	dbg_text_add( IIlbqcb, &dbtmp );
	return;
      default:
	((char *)buf)[0] = '?';
	((char *)buf)[1] = '\0';
	break;
    }
    II_DB_SCAL_MACRO(dbval, DB_CHR_TYPE, STlength((char *)buf), buf);
    dbg_text_add( IIlbqcb, &dbval );
    dbg_text_add( IIlbqcb, &dbspace );
}

/*{
+*  Name: dbg_text_add - Save a textual piece of a query.
**
**  Description:
**	Store as much of the string as will fit in the current chunk.
**	If we've run out of room, allocate another chunk and continue.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	dbv		- The text to save.
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
**	18-nov-1986 - Written. (mrw)
**      27-Oct-2003 (hanal04) Bug 111162 INGCBT499
**          If we run out of buffer space allocate new bigger buffers,
**          copy the current contents of the db_inbuf to the new buffer and 
**          deallocate the old buffers.
**	02-May-2005 (bonro01)
**	    Need to add length of actual data into calculation of new
**	    data buffer size instead of just adding a fixed amount.
*/

static VOID
dbg_text_add( II_LBQ_CB *IIlbqcb, DB_DATA_VALUE *dbv )
{
    u_i4	len;
    i4		free;
    register DBG_BUF	*dbg_buf;	/* Pointer to our structure */

    dbg_buf = (DBG_BUF *)IIlbqcb->ii_lq_dbg;
  /* If we couldn't grab a buffer, forget it. */
    if (dbg_buf == NULL)
	return;

    len = dbv->db_length;
    free = dbg_buf->db_inbuf_size - (dbg_buf->db_in_cur - dbg_buf->db_inbuf);

    /* Not enough room! */
    if (len > free)
    {
	char         *new_dbg_buf, *new_out_buf;
	u_i4         copy_len;

	/* Get newer bigger buffers */
        new_dbg_buf = MEreqmem( (u_i4)0, 
				(u_i4)(dbg_buf->db_inbuf_size +
				len + DB_IBUF_SIZ), 
				TRUE, NULL );        
        new_out_buf = MEreqmem( (u_i4)0,
				(u_i4)(dbg_buf->db_outbuf_size +
				len + DB_OBUF_SIZ + DB_OBUF_RES),
				TRUE, NULL );
	if(new_dbg_buf && new_out_buf)
	{
	    /* Got new buffers */
	    copy_len = (u_i4)(dbg_buf->db_in_cur - dbg_buf->db_inbuf);
	    dbg_buf->db_in_cur = new_dbg_buf + copy_len;
	    dbg_buf->db_inbuf_size += len + DB_IBUF_SIZ;
	    dbg_buf->db_outbuf_size += len + DB_OBUF_SIZ + DB_OBUF_RES;

	    /* Transfer current contents */
            copy_len = CMcopy(dbg_buf->db_inbuf, copy_len, new_dbg_buf); 

	    /* Free current buffers */
            MEfree(dbg_buf->db_inbuf);
	    MEfree(dbg_buf->db_outbuf);

	    /* Set new buffer pointers */
	    dbg_buf->db_inbuf = new_dbg_buf;
	    dbg_buf->db_outbuf = new_out_buf;
	}
	else
	{
	    /* Free successful allocation if either failed */
	    if(new_dbg_buf)
	    {
                MEfree(new_dbg_buf);
	    }

	    if(new_out_buf)
	    {
		MEfree(new_out_buf);
	    }

	    /* If an allocation failed revert to previous behaviour */
	    if(free <= 0)
	    {
		/* No room */
                return;
	    }
	    /* Truncate it */
	    len = free;
        }
    }

    /* Copy the text. */
    len = CMcopy(dbv->db_data, len, dbg_buf->db_in_cur);
    dbg_buf->db_in_cur += len;
} /* dbg_text_add */

/*{
**  Name: dbg_buf_get - Dynamically allocate buffer for storing query text. 
**
**  Description:
**	Dynamically allocate local control block buffer for storing query text.
**	Moved from IIdbg_info() to this routine so it can also be called by
**	IIdbg_toggle.
**	This routine is called from IIdbg_info for the "savequery", "printqry"
**	and "EQUEL -d" features.  It is called from IIdbg_toggle() for the
**	"printqry" feature when a trace data handler is registered and has
**	turned "printqry" on, and "printqry" has not also been set on via 
**	II_EMBED_SET or SET_SQL.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**
**  Outputs:
**	None.
**	Returns:
**	    OK:		Buffer successfully allocated.
**	    FAIL:	Could not allocate memory for buffer.
**  Errors:
**
**  Side Effects:
**	Local control block debug buffer (IIlbqcb->ii_lq_dbg) is allocated.
**
**  History:
**	15-dec-1991 (kathryn) Written.
**      27-Oct-2003 (hanal04) Bug 111162 INGCBT499
**          Dynamically allocate the query text buffers. 
*/

static STATUS
dbg_buf_get( II_LBQ_CB *IIlbqcb )
{
    DB_STATUS 	status = E_DB_OK;

    /*
    ** Allocate debugging control block, but only for
    ** regular (non-default) sessions.
    */
    if ( IIlbqcb->ii_lq_flags & II_L_CURRENT  &&  ! IIlbqcb->ii_lq_dbg )
    {
	IIlbqcb->ii_lq_dbg = MEreqmem( (u_i4)0, 
				       (u_i4)sizeof(DBG_BUF), TRUE, NULL );
	if(IIlbqcb->ii_lq_dbg)
	{
            ((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_inbuf = MEreqmem( (u_i4)0,
				       (u_i4)DB_IBUF_SIZ, TRUE, NULL );
            if(((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_inbuf)
	    {
		((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_inbuf_size = 
					DB_IBUF_SIZ;

                ((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_outbuf = MEreqmem( 
				       (u_i4)0,
				       (u_i4)(DB_OBUF_SIZ + DB_OBUF_RES), 
				       TRUE, NULL );
                if(((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_outbuf == NULL)
		{
		    status = E_DB_ERROR;
		}
		else
		{
		    ((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_outbuf_size = 
					DB_OBUF_SIZ;
		}
	    }
	    else
	    {
		status = E_DB_ERROR;
	    }

	    if(status != E_DB_OK)
	    {
		if(((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_inbuf)
		{
                    MEfree(((DBG_BUF *)(IIlbqcb->ii_lq_dbg))->db_inbuf);
		}

		if(IIlbqcb->ii_lq_dbg)
		{
                    MEfree(IIlbqcb->ii_lq_dbg);
		}
		IIlbqcb->ii_lq_dbg = (PTR)NULL;
            }
	}
    }

    return( IIlbqcb->ii_lq_dbg ? OK : FAIL );
} /* dbg_buf_get */



/*{
+*  Name:  IILQqthQryTraceHdlr - Register query text trace output handler.
**
**  Description:
**      Register callback routine to handle printqry trace output.  This does 
**	NOT affect the logging which may be active due to II_EMBED_SET or 
**	SET_SQL.  Registration of a callback routine will turn "Printqry"
**      tracing on if it is not already on, and de-registration will turn
**	"printqry" tracing off if it has been turned on only via the handler
**	registration and not via II_EMBED_SET or SET_SQL.
**      This routine is called by Ingres products (W4GL Debugger and XA) to 
**	register printqry trace output handlers.  The routines registered 
**	via this function are stored in the global control block (IIglbcb) 
**	and will be invoked by IIdbg_repsonse() and IIdbg_dump() when they 
**	are called to handle query text and timing trace information.
**	
**	NOTE:   We now maintain an array of function handlers.  As
**		of this writing there are two products that use this 
**		facility; the maximum number of callbacks allowed is
**		set at three (3).  To add more than that, change the constant
**		II_TR_MAX_HDLR in the iilibq.h file.
**
**  Inputs:
**
**      rtn - callback routine to recieve future output (if active = TRUE).
**
**      cb - context block pointer to be passed to (*rtn)().  Opaque to libq.
**
**      active - if TRUE, we are registering a new callback.  If false,
**              we are unregistering an existing callback.  Done this way
**              rather than by a NULL rtn argument to allow future support
**              for multiple callbacks, although we only need one at present.
**              (currently, a new active callback may simply replace an
**              existing one).
**  Outputs:
**	None.
**	Returns:
**	    FAIL - Attempt to register a NULL callback routine.
**	    OK   
**	Errors:
**	    None.
**
**  Side Effects:
**	Printqry tracing may be turned on/off.
**
**  History:
**	01-nov-1991 (kathryn) Written.
**	31-dec-1992 (larrym) 
**	    Modified to maintain a list of handlers instead of just one.
**	    Stripped out common code amongst all three trace handler 
**	    routines (including this one) and placed it in a new function
**	    IILQathAdminTraceHdlr, which this routine now calls.
*/
STATUS
IILQqthQryTraceHdlr(rtn,cb,active)
VOID (*rtn)();
PTR cb;
bool active;
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    IILQ_TRACE	*qrytrc = &IIglbcb->iigl_qrytrc;
    STATUS	status;

    if ( MUp_semaphore( &qrytrc->ii_tr_sem ) != OK )
	return( FAIL );

    status = IILQathAdminTraceHdlr( qrytrc, rtn, cb, active );

    MUv_semaphore( &qrytrc->ii_tr_sem );

    if ( status != FAIL )
	IIdbg_toggle( IIlbqcb, active ? IITRC_HDLON : IITRC_HDLOFF );

    return status;
}

/*
+*  Name:  IILQathAdminTraceHdlr - Administer tracing callback routine list
**
**  Description:
**      Register callback routine to handle tracing trace output.  
**	This can be called by any of the (currently) 3 tracing functions.
** 
**  Inputs:
**
**	trc_struct - pointer to 1 of 3 IILQ_TRACE stuctures in IIglbcb.
**
**      rtn - callback routine to recieve future output (if active = TRUE).
**
**      cb - context block pointer to be passed to (*rtn)().  Opaque to libq.
**
**      active - if TRUE, we are registering a new callback.  If false,
**              we are unregistering an existing callback.  Done this way
**              rather than by a NULL rtn argument to allow future support
**              for multiple callbacks, although we only need one at present.
**              (currently, a new active callback may simply replace an
**              existing one).
**  Outputs:
**	None.
**	Returns:
**	    FAIL - Attempt to register a NULL callback routine.
**	    OK   
**	Errors:
**	    None.
**
**  Side Effects:
**	Printqry tracing may be turned on/off.
**
**  History:
**	31-dec-1992 (larrym) written (happy new year!).
*/
STATUS
IILQathAdminTraceHdlr( IILQ_TRACE *trc_struct, 
		       VOID (*rtn)(), PTR cb, bool active )
{

    IILQ_TRC_HDL *hdlr_p = trc_struct->ii_tr_hdl_l; 
    i4		  hdlr_ndx;
    i4		  added = FALSE;

    if (active)
    {
	/* registering a new callback */

	if (rtn == NULL)
	    return FAIL;

	/* add new function; find first slot available */
	for (hdlr_ndx = 0; hdlr_ndx < II_TR_MAX_HDLR; hdlr_ndx++)
	{
	    if ( (hdlr_p[hdlr_ndx].ii_tr_hdl) == 0)
	    {
        	hdlr_p[hdlr_ndx].ii_tr_hdl = rtn;
        	hdlr_p[hdlr_ndx].ii_tr_cb = cb;
		added = TRUE;
		break;
	    }
	}

	if (added)
	    ++trc_struct->ii_num_hdlrs; 
	else
	    return FAIL;
    }
    else
    {
	/* de-register callback.  Find function in array */
	for (hdlr_ndx = 0; hdlr_ndx < II_TR_MAX_HDLR; hdlr_ndx++)
	{
	    if (hdlr_p[hdlr_ndx].ii_tr_hdl == rtn)
	    {
		hdlr_p[hdlr_ndx].ii_tr_hdl = /* (VOID *) */ 0;
		hdlr_p[hdlr_ndx].ii_tr_cb = (PTR)0;
	        trc_struct->ii_num_hdlrs--;
		break;
	    }
	}
    }
    return OK;

}

/*{
+*  Name:  IILQcthCallTraceHdlrs - call trace output handlers.
**
**  Description:
**      call callback routines to handle trace output.  This is called by all
**	three (so far) trace "facilities", i.e., printtrace, gcatrace and
**	querytrace (It just seemed easy to put it in this file).  The handler
**	used to be an element on the global control block and we would call
**	the directly from whereever we needed to.  Now we call this function
**	instead which makes sure all the handlers get called.
**	
**  History:
**	30-dec-1992 (larrym) Written.
*/
void
IILQcthCallTraceHdlrs( IILQ_TRC_HDL *trace_hdlr_list, 
		       char *outbuf, 
		       i4  flag )
{

    IILQ_TRC_HDL	*hdlr_p = trace_hdlr_list;
    i4		  hdlr_ndx;

    for (hdlr_ndx = 0; hdlr_ndx < II_TR_MAX_HDLR; hdlr_ndx++)
    {
	if (hdlr_p[hdlr_ndx].ii_tr_hdl != 0)
	{
	    _VOID_ (*hdlr_p[ hdlr_ndx ].ii_tr_hdl)(hdlr_p[ hdlr_ndx ].ii_tr_cb, 
					           outbuf, flag);
	}
	else
	   break;
    }

}



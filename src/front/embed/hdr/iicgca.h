/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
+*  Name: iicgca.h - Header file for Client to GCA interface.
**
**  Description:
**	This file defines the structures and definitions for a front-end
-*  	interface to GCA (General Communications - Applications utility).
**	It is be included with <gca.h>.
**
**  Notes:
**	The GCA tracing information is included in this file.  Most of
**	the structures and constants should be moved into gca.h and
**	its internal files when the tracing utility is moved into
** 	GCA itself.  The structure types are prefixed with GCA_ rather
**	than the IICGC_ prefix for future updates.
**
**  History:
**	08-sep-1986	- Written for 6.0 front-ends (ncg)
**	15-jul-1987	- Rewritten for GCA. (ncg)
**	09-may-1988	- Modified to support DB procedures. (ncg)
**	16-sep-1988	- Added all the tracing information. (ncg)
**	24-may-1989	- Added field to CGC_DESC to keep track of number
**			  of connections. (bjb)
**	16-aug-1989	- Added GCA_ROWCOUNT_INIT define to aid LIBQ
**			  in setting transaction info. (bjb)
**	23-apr-1990	- Alterernate read buffer processing (ncg)
**	15-jun-1990 (barbara)
**	    Added FASSOC mask and changed handler flag from HQUIT to HFASSOC.
**	    (bug 21832).
**	27-apr-1990 (barbara)
**	    Modified alternate read buffer processing.  Fixed defines are
**	    now in terms of number of bytes instead of number of message
**	    buffers.
**	28-sep-1990 (barbara)
**	    Added FUNC_EXTERN declaration of IIcgc_alloc_cgc.
**	25-jul-1991 (teresal)
**	    Clean up comments for new DBEVENT syntax.
**	24-mar-92 (leighb) Desktop Porting Change:
**	    added conditional function pointer prototypes.
**	03-nov-1992 (larrym)
**	    Added SQLSTATE support.  Added DB_SQLSTATE element to IICGC_HARG.
** 	23-feb-1993 (mohan)
**	    Added tags to structures for makeing prototyes correctly.
**	10-mar-93 (fraser)
**	    Changed structure tag names so that they don't begin with
**	    underscore.
**      31-mar-1993 (smc)
**          Fixed up prototypes of function pointers in IICGC_DESC to
**          match calls. Moved typedef of IICGC_HARG ahead of one of 
**          IICGC_DESC so that it could be used in the latters function 
**          pointer prototype.
**	18-oct-1993 (kathryn)
**	    Correct prototypes for cgc_debug and cgc_handle.
**	    Correct arguments types for cgc_debug and cgc_handle prototypes.
**	4-nov-93 (robf)
**          Add Prompt handler (CGC_HPROMPT)
**	22-apr-96 (chech02)
**          Add function prototypes for windows 3.1 port.
**      05-apr-96 (loera01)
**          Add IIcgc_find_cvar_len to support compressed variable-length
**	    datatypes.
**	25-Jul-96 (gordy)
**	    Added local username and password parameters to IIcgc_connect().
**	16-aug-96 (thoda04)
**          Added function prototype for IIcgcp3_write_data().
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**      09-Jan-97 (mcgem01)
**          The use of DLL's on NT requires that IIgca_cb be GLOBALDLLREF'ed.
**      30-jul-1999 (stial01)
**          Added IICGC_FLUSHQTXT
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-nov-2000 (stial01)
**          Removed IICGC_FLUSHQTXT
**      11-may-2001 (loera01)
**          Added constants for varchar compression.
**/

/*}
+*  Name: IICGC_MSG - Client's header for GCA message buffer.
**
**  Description:
**	This is the client header for the GCA message buffer.  The header
**	includes only data block counters and pointers and no semantic
**	information such as message types or modifiers.  The actual buffers
**	themselves are allocated at startup time. 
**
**	A buffer includes a message header (only known by GCA), and a
**	client/server data area.  
**
**	Also included is the GCA parameter list argument.  These are
**	explicitly non-static and maintained inside this structure so as
**	to handle "double operations", such as reading an IACK while waiting
**	to read other data with an interrupt handler.
**
**	There is also a special "interrupt" GCA parameter argument.  This is
**	there so that we do not use the same GCA_PARM to send queries as we
**	do to send interrupts.  If we did, we could run into the following
**	situation:
**	    Send a query (GCA_SEND does not complete)
**	    .... interrupt arrives before GCA_SEND completes ...
**	    Send an interrupt (GCA_SEND for interrupt completes, but when
**		done GCA marks all outstanding GCA_SEND's as "purged" inside
**		their GCA_PARM).
**	    On return from GCA_SEND of the interrupt, we find that it looks
**	    like the "interrupt" was purged!  This is because the GCA_PARM
**	    for sending both the interrupt and the regular query are the same.
**	This can also happen for GCA_RECEIVE which may be waiting on a read
**	when an interrupt is hit.  It may be possible to get data that was
**	just about to be deposited into the result buffer, but with the "purge"
**	status set.
**	Large Objects: 05-oct-1992 (kathryn)
**          Large Objects are witten to the message buffer as an ADP_PERIPHERAL
**          followed by any number of segment strings, followed by the 
**	    null_byte when nullable.  A segment string consists of an integer 
**	    (1 = more segments, 0 = no more segments) followed by an
**          i2 whose value contains the length of the next data segment, then
**          followed by the actual data segment.
**          One Large object would be transmitted as follows:
**              ADP_PERIPHERAL,
**              1, cgc_seg_len, data_segment, 1, cgc_seg_len, data_segment ...
**              0, null_byte (if nullable).
**          Note that the "," is not part of the message, it is used here
**          only for readability.
**		
-*
**  History:
**	15-jul-1987	- Written (ncg)
**	09-oct-1987	- Added GCA_PARMLIST arguments (ncg)
**	05-oct-1992  (kathryn)
**	    Added cgc_seg_len and cgc_seg_used for use with large objects.
**	    These fields are used only when transmitting a large object.
**	    cgc_seg_len is the length in bytes of the current data segment 
**	    being transmitted.  cgc_seg_used is the number of bytes of the
**	    current data segment that has been read/written in the cgc_buffer.
**	01-nov-1992  (kathryn)
**	    Added cgc_more_segs - indicates whether or not there are more
**	    segements of a large object in GCA buffer.  This value is part of
**	    the segment string as described above.
**	23-apr-1993 (kathryn)
**	    Change cgc_more_segs from i4 to nat.
*/

typedef struct s_IICGC_MSG {
    GCA_PARMLIST cgc_parm;		/* GCA call parameter argument */
    GCA_PARMLIST cgc_intr_parm;		/* GCA call interrupt argument */
    i4	cgc_b_length;	  	/* Message buffer size is the size of
					** the message buffer with the message
					** header.
					*/
    char	*cgc_buffer;		/* Message buffer (header reserved by
					** GCA_FORMAT and GCA_RECEIVE).  This
					** buffer is allocated at startup.
					*/
    i4	cgc_d_length;		/* # of bytes allowed for data in
					** message buffer. This is set by
					** calling GCA_FORMAT and GCA_RECEIVE:
					** initialized to:
					**  cgc_b_length - size(gca_hdr)
					*/
    i4	cgc_d_used;		/* # of bytes used.  On input this is
					** the # of bytes that have been filled
					** by user data, on output this is the
					** number of data bytes read so far.
					** Note that:
					**  cgc_d_used <= cgc_d_length
					*/
    char	*cgc_data;		/* Where data starts inside message
					** buffer (cgc_buffer).  Note that data
					** is copied into and read from:
					**  cgc_data + cgc_d_used
					*/
    i4		cgc_message_type;	/* Current message type */

    i4		cgc_more_segs;		/* Are there more segments in the
					** GCA buffer. This value is sent from 
					** the DBMS.
					*/

    i2		cgc_seg_len;		/* Length in bytes of the large object
				        ** segment that is being transmitted.
					*/
    i2		cgc_seg_used;		/* The number of Bytes of the large 
					** object segment that has been read or
					** written in cgc_buffer.
					*/
					

} IICGC_MSG;

/*}
+*  Name: IICGC_QRY - Query text bufferer.
**
**  Description:
**	This structure buffers query text as it comes in from LIBQ.
**	When full the buffer is sent as a DB_QTXT_TYPE DB data value and
**	put into the general message buffer.  When the query is complete
**	a null is always added to the end of the query.  In order to ALWAYS
**	allow the addition of a null without sending an extra message,
**	we make sure that the maximum fillable length is one less than
**	the maximum length allocated for the buffer.
-*
**  History:
**	17-jul-1987	- Written (ncg)
*/

typedef struct s_IICGC_QRY {
    i4	cgc_q_max;		/* Maximum size that can be filled.
					** This is set to sizeof(buf) -1 in
					** order to always allow a terminating
					** null byte.
					*/
    i4	cgc_q_used;		/* # query text bytes in buffer */
    char	*cgc_q_buf;		/* Pointer to query text */
} IICGC_QRY;

/*
** Tracing Information:
**
**    Currently, all tracing is handled by LIBQGCA.  In the future tracing
** will be managed by a GCA service which can be turned on or off.
** When this is done the following service call and parameter should be
** introduced (note that the constant GCA_TRACE is already reserved so we
** will use another name):
**
**	    GCA_MSGTRACE	- Service call to turn on/off tracing.
**
**	    struct {		- Parm list to GCA_MSGTRACE service.
**		VOID (*gca_emit_exit)();  - If set resume tracing, else suspend.
**					  - Default is no tracing.  Arguments to
**					  - function are:
**						func(action, length, buffer);
**					  - See use of IILQgwtGcaWriteTrace.
**		i4  gca_line_length	  - Trace line length >= 80.
**	    } GCA_TR_PARMS
*/

/*}
+*  Name: GCA_TR_STATE - Internal trace state for trace generator.
**
**  Description:
**	This structure maintains internal information known within the trace
**	generator.  No one outside may update the internals of this structure.
**	The fields allow the trace generator to handle the printing, buffering
**	and processing of random GCA messages that are traced.
-*
**  History:
**	16-sep-1988	- Written (ncg)
**	07-dec-1988	- Added protocol level to allow tracing of multiple
**			  protocol formats (ncg)
**	01-mar-1993  (kathryn)
**	    Added GCA_TR_CONT, GCA_TR_INVAL and cgc_gcv_no.  
**	    These are used to keep the state of the trace of a GCA_QUERY 
**	    message. QUERY data that spans GCA messages will now be traced 
**	    as each message is sent. 
**	
*/

typedef struct s_GCA_TR_STATE {
    i4	gca_protocol;		/* GCA client/server protocol version
					** number.  This is saved to allow
					** the trace utility to vary output 
					** based on protocol levels.  When new 
					** protocol levels are introduced the
					** trace module should be made aware
					** of the currently running protocol.
					*/ 

    PTR		gca_trace_sem;		/* Semaphore to protect gca_emit_exit */

#ifdef CL_HAS_PROTOS
    VOID 	(*gca_emit_exit)(i4,i4,PTR);
#else									  
    VOID 	(*gca_emit_exit)(); 
#endif
	
					/* Saved trace emit function.  Routine
					** interface is:
					**   routine(action, length, buffer);
					** Action is one of:
					*/
# define	GCA_TR_WRITE		0
					/* Buffer includes data - write it.
					*/
# define	GCA_TR_FLUSH		1
					/* If buffer is not null then write
					** its data - flush any written trace.
					*/
    i4	 	gca_line_length;	/* Saved trace line length.  When
					** reaching this limit the exit function
					** will be called.
					*/
    i4	 	gca_message_type;	/* Message type of last message traced.
					** Saved to handle protocol errors.
					*/
    i4	gca_flags;		/* Status flags for current tracing
					** operation
					*/
# define	GCA_TR_IGNORE		0x001
					/* 
					** Because of internal error ignore
					** continuation of trace till end of
					** GCA message.
					*/
# define        GCA_TR_CONT             0x002
					/* The data in this GCA message buffer 
					** is continuation data of a GCA 
					** message rather than a new message.
					*/
# define	GCA_TR_INVAL		0x004
					/* We are currently in the middle
					** of tracing a data value - character
					** string values of query messages may
					** span several GCA message buffers.
					*/
    i4 	gca_l_continue;		/* How big is the continued buffer.
					** This allows the continuation protocol
					** to be traced via building one message
					** into the continued buffer.
					*/
    i4	gca_d_continue;		/* The amount of data used within the
					** continued buffer.
					*/
    char 	*gca_continued_buf;	/* Dynamically allocated continuation
					** protocol buffer.
					*/
# define	GCA_TR_MAXTRACE		10000
					/*
					** Maximum dynamic size of the continue
					** buffer.  If it grows larger than 
					** this then tracing is turned off for
					** the rest of the message.
					*/
    i4          gca_gcv_no;		/* The current gcv number of a QUERY 
					** message.
					*/
    DB_DATA_VALUE gca_dbv;		/* Data value tracing info */
    i4		gca_seg_no;		
    bool	gca_seg_more;
    bool	gca_seg_len;

} GCA_TR_STATE;



/*}
+*  Name: GCA_TR_BUF - Trace control message buffer.
**
**  Description:
**	This structure includes the trace control information provided
**	by the GCA_RECEIVE and GCA_SEND services.  These services call
**	the trace generator after filling in the fields of this trace
**	message buffer from their corresponding parameters.
-*
**  History:
**	16-sep-1988	- Written (ncg)
*/

typedef struct s_GCA_TR_BUF {
    i4	 	gca_service;		/* GCA_SEND or GCA_RECEIVE */
    i4	 	gca_association_id;	/* From original parm */
    i4	 	gca_message_type;	/* From original parm */
    i4 	gca_d_length;		/* From original parm */
    char 	*gca_data_area;		/* From original parm */
    PTR	 	gca_descriptor;		/* Form original parm */
    i4	 	gca_end_of_data;	/* From original parm */
} GCA_TR_BUF;

/*
** Alternate Read Buffer Processing:
**
**	These data structures provide the CGC caller with an alternate source
**	of data.  Typically the data buffers are pre-loaded from GCA and
**	then returned to the user attribute at a time.  These data structures
**	are manipulated in the module cgcaltrd.c.
*/

/*}
** Name: IICGC_ALTMSG - Alternate message buffer.
**
** Description:
**	This data structure defines an buffered message to be processed as an
**	alternate source for reading data.  This message is queued up in the
**	buffer queue of IICGC_ALTRD.  The message buffer is allocated after it
**	is read from GCA in IIcga3_load_stream and freed in IIcga5_get_data
**	(when the data in the buffer is exhausted) and in IIcga2_close_stream
**	(when explicitly deallocated due to client request).
**
** History:
**	23-apr-1990 (neil)
**	    Written for cursor performance project.
**      27-jul-1990 (barbara)
**	    Changed cursor prefetch defaults to specify number of bytes to
**	    prefetch instead of number of message buffers.  This is because
**	    the size of a message buffer varies widely between different
**	    systems.
**	24-Feb-2010 (kschendel) SIR 123275
**	    Revise size constant comments to reflect new usage.
*/

typedef struct _amsg_
{
    struct _amsg_ *cgc_anext;		/* Next message in queue */
    i4		cgc_amsg_type;		/* Message type of saved GCA message
					** buffer.  Currently set up to allow
					** GCA_TUPLES and GCA_ERROR messages
					** to be buffered as alternate sources.
					*/
    i4	cgc_aused;		/* # of bytes used in this buffer.
					** Must be <= cgc_adlen.  Once the
					** requested # of bytes reaches
					** cgc_adlen, then this buffer is freed
					** and we move on to the next buffer.
					*/
    i4	cgc_adlen;		/* # of bytes in following buffer */
    char	cgc_adata[1];		/* Data allocated and copied in-line
					** when message buffer is allocated.
					*/
} IICGC_ALTMSG;

/*
** Defaults and maximums to control memory allocation
*/
# define  IICGC_ADEF_BYTES	8192	/* Default stream size guideline.
					** This size, adjusted to be some
					** multiple of the GCA buffer size,
					** is the default prefetch size in
					** bytes.  (div by row size to get
					** default prefetch in rows.)
					*/
# define  IICGC_AMAX_BYTES	150000	/* A user PREFETCHROWS setting,
					** converted to bytes, is clipped to
					** this maximum before aligning to
					** the GCA buffer size and becoming
					** the prefetch stream size.
					** Unless the user row is VERY large,
					** this maximum ought to be well into
					** the realm of diminishing returns.
					*/

/*}
** Name: IICGC_ALTRD - Client GCA alternate read descriptor.
**
** Description:
**	This descriptor is used to process the pre-fetch operation for
**	cursors.  In LIBQGCA terms this descriptor, when allocated and
**	filled, will provide an alternate stream from which the client
**	may read data.  This descriptor is allocated in IIcga1_open_stream,
**	while the buffer queue is allocated and filled by IIcga3_load_stream.
**	Upon subsequent retrievals, when the caller recognizes that data 
**	should be read from the alternate stream (possibly one of many) the
**	IICGC_DESC cgc_alternate field is pointed at our descriptor and
**	attempts to read data will be routed through IIcga5_get_data.
**
**	This data structure is deallocated by IIcga2_close_stream.
**
** History:
**	23-apr-1990 (neil)
**	    Written for cursor performance project.
*/

typedef struct s_IICGC_ALTRD
{
    i4	cgc_aflags;		/* Status of current alternate stream */
#define	   IICGC_ALOADING	0x001	/* Stream is currently being loaded.
					** Set by IIcga3_load_stream.
					*/
#define	   IICGC_AREADING	0x002	/* Data is currently being retrieved
					** from alternate stream. Set by caller.
					*/
#define	   IICGC_AERRLOAD	0x004	/* While loading an error was read from
					** GCA and loaded into stream. Set by
					** IIcga4_load_error.
					*/
#define	   IICGC_ATRACE		0x008	/* Trace internal alternate stream 
					** operations.  Set by caller.
					*/
    char	cgc_aname[DB_MAXNAME];	/* Internal name for tracing.  This
					** may be smaller than the original
					** query name and will be truncated.
					*/
    i4	cgc_atupsize;		/* Tuple size for the query associated
					** with this read stream.  Set by
					** IIcga1_open_stream.
					*/
    i4		cgc_arows;		/* Based on original row descriptor
					** associated with this read-stream,
					** the number of rows to request on
					** a read.  Value may be result of
					** system defaults or user setting.
					** Set by IIcga1_open_stream.
					*/
    IICGC_ALTMSG *cgc_amsgq;		/* Pointer to first message in
					** alternate buffer queue.  If this is
					** NULL then there are none left.
					*/
    GCA_RE_DATA	cgc_aresp;		/* Saved response block after pre-fetch
					** operation to fill alternate buffer
					** queue.  This is saved so that
					** we can properly reconstruct the
					** current response block on a per-row
					** basis.  The rowcount field indicates
					** how many rows were returned for this
					** operation.
					*/
} IICGC_ALTRD;


/*
** Handler flags and argument types 
*/

/*}
+*  Name: IICGC_HARG - Structure to pass to cgc_handler.
**
**  Description:
**	This structure allows us to pass trace data, error messages, internal
**	error numbers and parameters, etc.  The cgc_handle field is called
**	with a flag and a parameter of this structure type.
-*
**  History:
**	21-jul-1987	- Written (ncg)
**	11-may-1988	- Modified with extra parameters for 6.1 (ncg)
**	09-dec-1988	- Field h_use now set to gca_severity. (ncg)
**	15-jun-1990 (barbara)
**	    Changed h_use flag HQUIT to HFASSOC.  Calling handler with
**	    this flag does not cause automatic quit now.  (bug 21832)
**	01-nov-1992 (larrym)
**		Added h_sqlstate field to allow passing back the SQLSTATE
**		returned from the DBMS.  
*/

typedef struct s_IICGC_HARDG {
    i4	h_errorno;	/* If an error occurs then this indicates the
				** error number.
				*/
    i4	h_local;	/* Local error from gateway.  This field is
				** only used when called with IICGC_HDBERROR.
				*/
    i4		h_use;		/* Flag error usage.  This field is only used 
				** when called with IICGC_HDBERROR and is set
				** to the same value as the gca_severity field
				** of a GCA_ERROR (default, warning, message,
				** formatted).
				*/
    DB_SQLSTATE	*h_sqlstate;	/* SQLSTATE returned from DBMS (or GW).	For
				** all handlers defined to date, this should
				** have a value (if current protocol level).
				** DB_SQLSTATE is defined in dbms.h.
				*/
    i2		h_numargs;	/* # of arguments in h_data */
    i2		h_length;	/* If there is only one parameter (in h_data)
				** and this is trace data, a CL error or a
				** server GCA_ERROR message then this length
				** should be used.  Otherwise the data in
				** h_data is null terminated C string data.
				*/ 
    char	*h_data[5];	/* Parameters to handler service */
} IICGC_HARG;

# define	IICGC_HTRACE	1	/* GCA_TRACE message.  Argument includes
					** h_length and h_data[0] being the
					** trace data.  If h_errorno is > 0 this
					** means first piece of trace data.
					** If h_errorno is 0 this means normal
					** trace.  If h_errorno is < 0 then
					** this means flush any trace data
					** buffered so far.  
					*/
# define	IICGC_HDBERROR	2	/* GCA_ERROR message received. Argument
					** has h_use set to error, warning or
					** user message.  
					**
					** 1. If a DB error or warning was
					** read, then argument has h_errorno
					** set to error #, h_length and h_data
					** being the error text. Usually
					** h_numargs is 1 (if it isn't then
					** there is no string).  This works
					** with the INGRES server that passes
					** errors back as strings.  If on
					** a gateway, then h_local is set to the
					** local error number.
					**
					** If the argument is null, then the
					** the client may choose to reset the
					** internal error status, as more data
					** follows (ie, a server warning).
					**
					** 2. If a user message was returned,
					** then argument has h_errorno set to
					** the message number (if specified),
					** h_length and h_data being the message
					** text (if specified).  One or both
					** must be specified.
					*/
# define	IICGC_HCLERROR	3	/* CL error message string.  Argument
					** has CL error status in h_errorno, 
					** h_data[0] is the null-terminated
					** routine name (ie, "IIcgc_connect").
					** Handler should call ERreport and
					** format.  
					*/
# define	IICGC_HGCERROR	4	/* GCA error.  Argument has IICGCA error
					** number in h_errorno, # or arguments
					** and null terminated parameters 
					** in h_data.   
					*/
# define	IICGC_HUSERROR	5	/* User error. Argument has IICGCA error
					** number in h_errorno, # or arguments
					** and null terminated parameters 
					** in h_data.  
					*/
# define	IICGC_HCOPY	6	/* Initiate COPY processing. Message 
					** type FROM or INTO is in h_errorno.
					** Rest of argument is ignored.
					*/
# define	IICGC_HFASSOC	7	/* Communications failure.  Quit
					** or continue, depending on II_EMBED_
					** SET or SET_INGRES setting.
					*/
# define	IICGC_HVENT	8	/* GCA_EVENT message.  Argument is
					** ignored.  Call routine to read
					** contents of message and save.
					*/
# define	IICGC_HPROMPT	9	/* GCA_PROMPT message.  Read reply
					** from session (if possible) and
					** return to partner
					*/

/*}
**  Name: IICGC_DESC - Client descriptor area for GCA communications interface.
**
**  Description:
**	The client GCA communications data structures are hidden from the 
**	callers (in LIBQ) via this descriptor.  This descriptor is allocated
**	and filled on session startup, and is always passed in with calls
**	to IIcgc routines.
**
**	This structure is split into 4 groups:
**	1. The "general" group consists of global message information, such as
**	   masks indicating what is going on.
**	2. The "writing" group contains any information required while writing
**	   data to the server.  There are one message buffer for writing
**	   data and a the query-text buffer for buffering query text as it
**	   comes in.  This query buffer is maintained so as not to have little
**	   pieces of queries mixed with binary values.  Note that this DOES
**	   require the copying of the query buffer into the data buffer when
**	   full.  This group also keeps track of the current message type.
**	3. The block "reading" group is made up of one result block, a 
**	   response, procedure return structure, and alternate buffer.
**	4. The last group is "miscellaneous" and contains I/NET descriptor
**	   information and utility handlers to call on various conditions.
**
**  History:
**	14-aug-1986	- Written for Jupiter FE-BE communications. (ncg)
**	14-jul-1987	- Modified for GCA. (ncg)
**	04-may-1988	- Added cgc_retproc for database procedures and
**			  added descriptor information for I/NET. (ncg)
**	01-sep-1988	- Added cgc_savestate for faster startup of
**			  spawned processes. (ncg)
**	16-sep-1988	- Added cgc_trace for tracing GCA. (ncg)
**	18-apr-1989	- Added state flag to describe whether association
**			  is heterogeneous. (bjb)
**	23-apr-1990	- Added cgc_alternate for cursor performance
**			  project. (ncg)
**	15-jun-1990 (barbara)
**	    Added state flag for failed association (FASSOC_MASK) so that
**	    disconnect doesn't attempt to send GCA_RELEASE. (bug 21832)
**	01-nov-1991 (kathryn) 
**	    Add defines used as cgc_debug() parameters for PRINTQRY handling.
**	    IIDBG_INIT - New query - re-initialize query trace buffer.
**          IIDBG_ADD  - Add text strings to current query trace buffer.
**          IIDBG_END  - Sending query to DBMS - print query trace to file.
*/

typedef struct s_IICGC_DESC {
				    /* General information */

    i4	cgc_assoc_id;		/* GCA association id assigned when
					** the initial request for association
					** is made.
					*/
    i4	cgc_version;		/* GCA client/server protocol version
					** number.  This is initially set to the
					** compile-time value, but can be
					** changed at connection-time to work
					** with an older version.
					*/
    i4		cgc_query_lang;		/* Query language currently running */
    i4		cgc_g_state;	  	/* State masks used on a per-session
					** basis.  These states are maintained
					** across message boundaries.
					*/

# define	IICGC_CHILD_MASK	0x001
					/* This session is a spawned child -
					** do not send a GCA_RELEASE message
					** when done.
					*/
# define	IICGC_QUIT_MASK		0x002
					/* Caller has quit session.  This may
					** be a normal exit, a GCA_RELEASE
					** message received from server, or an
					** "association failed" error from 
					** GCA.
					*/
# define	IICGC_COPY_MASK		0x004
					/* Currently within COPY handler */
# define	IICGC_HET_MASK		0x008
					/* Association is heterogeneous.
					** FEs need to know when encoded data 
					** must be transformed.
					*/
    i4		cgc_m_state;	  	/* State masks used on a per-message
					** basis.  For each new message the
					** state is cleared.
					*/

# define	IICGC_QTEXT_MASK	0x001
					/* Message has query text (this is
					** GCA_QUERY or GCA_DEFINE).
					*/
# define	IICGC_DBG_MASK		0x002
					/* Query debugging (query echoing on
					** errors) is set for this message.
					*/
# define	IICGC_LAST_MASK		0x004
					/* We have read a message buffer that
					** must be the last buffer in the
					** sequence.  Includes GCA_RESPONSE,
					** GCA_DONE, GCA_REFUSE, GCA_S_[E|B]TRAN
					*/
# define	IICGC_ERR_MASK		0x008
					/* Currently reading error message (so
					** as to avoid resignaling new error).
					*/
# define	IICGC_START_MASK	0x020
					/* In startup, so do not call the quit
					** handler. Even if you receive a
					** GCA_RELEASE message from server.
					** (Used to prevent cascading errors.)
					*/
# define	IICGC_EOD_MASK		0x040
					/* During reading the end_of_data
					** mask has been read for a particular
					** message.  This prevents the client
					** from reading too much on some
					** messages.
					*/
# define	IICGC_PROCEVENT_MASK	0x080
					/* LIBQ is reading GCA_EVENT data so
					** do not recursively call LIBQ
					** handler on subsequent GCA_EVENT
					** blocks.
					*/
# define	IICGC_EVENTGET_MASK	0x100
					/* Event is being read to process
					** a GET DBEVENT statement so do not
					** read more than one GCA_EVENT msg
					** and do not call DBEVENTHANDLER on
					** this event.
					*/
# define	IICGC_FASSOC_MASK	0x200
					/* Failed association state.  Set
					** after failed GCA_READ/RECEIVE.
					** Flag is checked in disconnect
					** code to prevent further errors.
					*/

				    /* Message writing information */

    IICGC_MSG	cgc_write_msg;		/* To put message data */
    IICGC_QRY	cgc_qry_buf;		/* Bufferer for query strings */

				    /* Data reading information */

    IICGC_MSG	cgc_result_msg;		/* Result data block */
    GCA_RE_DATA	cgc_resp;		/* Response structure */
					/* Rowcount field of response block
					** is initialized with this value
					** so LIBQ knows whether xact info
					** has been refreshed with info from
					** response block.
					*/
# define	GCA_ROWCOUNT_INIT	-99
    GCA_RP_DATA	cgc_retproc;		/* Procedure return structure */
    GCA_TX_DATA	cgc_xact_id;		/* Transaction id - ignored */
    IICGC_ALTRD	*cgc_alternate;		/* Alternate read descriptor for
					** reading from pre-fetched buffer
					** queue. If set then IIcgc_get_value
					** will call the alternate read routine
					** IIcga5_get_data.
					*/

				    /* Miscellaneous utilities */

    GCA_TD_DATA	*cgc_tdescr;		/* GCA tuple descriptor for 
					** GCA presentation layer for
					** tuple data.
					*/

    char	*cgc_savestate;		/* Saved state of callers application.
					** This is allocated and set by the
					** caller before spawning another DBMS
					** session, and saved together with 
					** GCA_SAVE.
					** In the child process this is 
					** restored and allocated from
					** GCA_RESTORE and can then be
					** retrieved by the caller.
					**
					** The callers saved state must be less
					** than the following constant.
					*/
# define	IICGC_SV_LEN		1000

    GCA_TR_STATE cgc_trace;		/* Internal trace descriptor to use
					** when tracing GCA_SEND and GCA_RECEIVE
					** service calls.  Tracing is on if the
					** gca_emit_exit of this structure is
					** not null.
					*/
    i4		(*cgc_handle)();
					/* Caller's function handler passed in
					** at startup.  This handler is called
					** with one parameter specifying the
					** object, and the second parameter
					** holds the actual value.  It should
					** be declared as:
					**    handle(flag, iicgc_harg)
					** The flags and argument are
					** specified below.
					*/
    VOID	(*cgc_debug)(i4 action, DB_DATA_VALUE *dbv);
					/* Callers debug handler.  This 
					** handler is called with: 
					**  debug(INIT_or_ADD_or_END,dbv)   
					*/
# define	IIDBG_INIT	1       /* debug(IIDBG_INIT,dbv)- dbv holds: 
					**       query text strings,
					**   or  dbv=null if not debug-able 
					*/
# define	IIDBG_ADD	2	/* debug(IIDBG_ADD, dbv) - dbv holds:
					**      query text strings,
					**   or db values; 		    
					*/
# define	IIDBG_END	3	/* debug(IIDBG_END,dbv) - dbv=NULL:	
					**	output query text.
					*/
} IICGC_DESC;


/* Various definitions for iicgc entry points */

/*
** The time out argument to GCA is always negative, indicating that we wait
** until completion.
*/
# define	IICGC_NOTIME	((i4) -1)

/*
** If iicgcread processes an error block while reading it returns a
** negative count (relative to a normally positive count).
*/
# define	IICGC_GET_ERR	(-1)

/*
** Flags for IIcgc_put_msg and IIcgc_get_value
*/

# define	IICGC_VQRY	0	/* Argument is DBV with query text. The
					** db_datatype is DB_QTXT_TYPE.  For
					** input only.
					*/
# define	IICGC_VID	1	/* Argument is a DBV pointing at a query
					** id.  The db_datatype is ignored. This
					** is used to write and read query ids.
					*/
# define	IICGC_VDBV 	2	/* Argument is a DBV whose value should
					** be sent as a GCA data value. For
					** input only.
					*/
# define	IICGC_VVAL	3	/* Argument is a DBV whose value should
					** be copied into the message buffer
					** without a GCA data value (for input),
					** or copied into a result buffer (for
					** output).
					*/
# define	IICGC_VNAME	4	/* Argument is a DBV whose value should
					** be copied into the message buffer
					** as a GCA_NAME (for input).  When 
					** used for output (future enhancement
					** to DB procedures), the name can be
					** copied into the result buffer.
					*/
# define        IICGC_VVCH      5       /* Argument is a DBV whose tuple
                                        ** contains compressed varchar
					** columns.
                                        */
#define         IICGC_NVCHR    -2       /* Nullable Unicode varchar */
#define         IICGC_VCH      -1       /* Nullable varchar */
#define         IICGC_NON_VCH  0        /* Non-varchar */
#define         IICGC_VCHN     1        /* Non-nullable varchar */
#define         IICGC_NVCHRN   2        /* Non-nullable Unicode varchar */


/*
** Globals
*/

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF PTR        IIgca_cb;       /* GCA control block */
# else
GLOBALREF PTR   IIgca_cb;               /* GCA control block */
# endif 


/*
** Entry points to iigca from the outside
*/

/* cgcassoc */					/* check-7 */
FUNC_EXTERN STATUS IIcgc_init( i4  (*)( i4, IICGC_HARG * ), i4  * );
FUNC_EXTERN VOID   IIcgc_term( VOID );
FUNC_EXTERN STATUS IIcgc_connect();			/* c */
FUNC_EXTERN STATUS IIcgc_tail_connect();		/* t */
FUNC_EXTERN VOID IIcgc_disconnect();			/* d */
FUNC_EXTERN STATUS IIcgc_alloc_cgc();			/* a */
FUNC_EXTERN VOID IIcgc_free_cgc( IICGC_DESC * );	/* f */
FUNC_EXTERN STATUS IIcgc_save();			/* s */
FUNC_EXTERN STATUS IIcc1_connect_params();		/* Internal calls */
FUNC_EXTERN STATUS IIcc2_connect_restore();
FUNC_EXTERN VOID IIcc3_discon_service();

/* cgcwrite */
FUNC_EXTERN VOID IIcgc_init_msg();			/* i */
FUNC_EXTERN VOID IIcgc_put_msg();			/* p */
FUNC_EXTERN VOID IIcgc_end_msg();			/* e */
FUNC_EXTERN VOID IIcgc_break();				/* b */
FUNC_EXTERN VOID IIcc1_put_qry();			/* Internal calls */
FUNC_EXTERN VOID IIcc2_put_gcv();
FUNC_EXTERN VOID IIcc3_put_bytes();
FUNC_EXTERN STATUS IIcc1_send();

/* cgcread */
FUNC_EXTERN i4  IIcgc_get_value();			/* g */
FUNC_EXTERN i4  IIcgc_more_data();			/* m */
FUNC_EXTERN i4  IIcgc_read_results();			/* r */
FUNC_EXTERN STATUS IIcgc_event_read();			/* e */
FUNC_EXTERN i4  IIcc1_read_bytes();			/* Internal calls */
FUNC_EXTERN i4  IIcgc_find_cvar_len();
FUNC_EXTERN VOID IIcc2_read_error();
FUNC_EXTERN VOID IIcc3_read_buffer();

/* cgcaltrd */
FUNC_EXTERN STATUS IIcga1_open_stream();
FUNC_EXTERN VOID   IIcga2_close_stream();
FUNC_EXTERN STATUS IIcga3_load_stream();
FUNC_EXTERN STATUS IIcga4_load_error();
FUNC_EXTERN i4     IIcga5_get_data();
FUNC_EXTERN VOID   IIcga6_reset_stream();
FUNC_EXTERN STATUS IIcga1_open_stream();

/* cgccopy */
FUNC_EXTERN VOID    IIcgcp1_write_ack();
FUNC_EXTERN VOID    IIcgcp2_write_response();
FUNC_EXTERN STATUS  IIcgcp3_write_data();
FUNC_EXTERN i4 IIcgcp4_write_read_poll();
FUNC_EXTERN i4      IIcgcp5_read_data();

/* cgcutils */
FUNC_EXTERN VOID  IIcc1_util_error();			/* Internal calls */
FUNC_EXTERN char *IIcc2_util_type_name( i4, char * );
FUNC_EXTERN VOID  IIcc3_util_status();

/* cgctrace */
FUNC_EXTERN VOID  IIcgct1_set_trace();
FUNC_EXTERN VOID  IIcgct2_gen_trace();			/* Internal calls */

# define	IICGC_ELEN	255			/* Length of status */

#ifdef WIN16
/* cgcassoc */					/* check-7 */
FUNC_EXTERN STATUS IIcgc_connect(i4 argc,char **argv , i4  query_lang,
            i4  hdrlen,i4 gateway,i4 sendzone,
            char *dbname,struct _ADF_CB *adf_cb, 
            i4	(*cgc_handle)(), struct _IICGC_DESC	*cgc,
	    char *lcl_username, char *lcl_passwd,
            char *rem_username, char *rem_passwd);
FUNC_EXTERN STATUS IIcgc_tail_connect(struct s_IICGC_DESC *cgc);
FUNC_EXTERN VOID   IIcgc_disconnect(struct s_IICGC_DESC *cgc,bool internal_quit);
FUNC_EXTERN STATUS IIcgc_alloc_cgc(struct s_IICGC_DESC **cgc_buf ,
            i4  (*cgc_handle)(int ,struct s_IICGC_HARDG *));
FUNC_EXTERN STATUS IIcc1_connect_params(struct s_IICGC_DESC *cgc,
            i4  argc,char **argv ,i4 gateway,
            i4  sendzone,struct _ADF_CB *adf_cb,char *dbname);
FUNC_EXTERN STATUS IIcgc_save(struct s_IICGC_DESC *cgc,char *dbname,
            char *save_name);
FUNC_EXTERN STATUS IIcc2_connect_restore(struct s_IICGC_DESC *cgc,
            char *save_name,i4 header_length,char *dbname);
FUNC_EXTERN VOID   IIcc3_discon_service(struct s_IICGC_DESC *cgc);

/* cgcwrite */
FUNC_EXTERN VOID IIcgc_init_msg(struct s_IICGC_DESC *cgc,
               i4  message_type,i4 query_lang,i4 modifier);	/* i */
FUNC_EXTERN VOID IIcgc_put_msg(struct s_IICGC_DESC *cgc,bool put_mark,
               i4  put_tag,struct _DB_DATA_VALUE *put_arg);			/* p */
FUNC_EXTERN VOID IIcgc_end_msg(struct s_IICGC_DESC *cgc);           /* e */
FUNC_EXTERN VOID IIcgc_break(struct s_IICGC_DESC *cgc,i4 breakval); /* b */
FUNC_EXTERN VOID IIcc1_put_qry(struct s_IICGC_DESC *cgc,
              struct _DB_DATA_VALUE *qry_dbv);			/* Internal calls */
FUNC_EXTERN VOID IIcc2_put_gcv(struct s_IICGC_DESC *cgc,bool convert_char,
              struct _DB_DATA_VALUE *put_dbv);
FUNC_EXTERN VOID IIcc3_put_bytes(struct s_IICGC_DESC *cgc,bool atomic,
              i4  bytes_len,char *bytes_ptr);
FUNC_EXTERN STATUS IIcc1_send(struct s_IICGC_DESC *cgc,bool end_of_data);

/* cgcread */
FUNC_EXTERN i4  IIcgc_get_value(struct s_IICGC_DESC *cgc,i4 message_type,
                       i4  get_tag, struct _DB_DATA_VALUE *get_arg);			/* g */
FUNC_EXTERN i4  IIcgc_more_data(struct s_IICGC_DESC *cgc,i4 message_type);			/* m */
FUNC_EXTERN i4  IIcgc_read_results(struct s_IICGC_DESC *cgc,
                  bool flushing_on, i4  message_type);			/* r */
FUNC_EXTERN STATUS IIcgc_event_read();			/* e */
FUNC_EXTERN i4  IIcc1_read_bytes(struct s_IICGC_DESC *cgc,i4 message_type,
                  bool atomic,i4 bytes_len,char *bytes_ptr);			/* Internal calls */
FUNC_EXTERN VOID IIcc2_read_error(struct s_IICGC_DESC *cgc);
FUNC_EXTERN VOID IIcc3_read_buffer(struct s_IICGC_DESC *cgc,
                   i4 time_out,i4 channel);
FUNC_EXTERN	STATUS IIcgc_event_read(struct s_IICGC_DESC *cgc,i4 timeout);

/* cgcaltrd */
FUNC_EXTERN STATUS IIcga1_open_stream(struct s_IICGC_DESC *cgc,
                    i4  flags,char *qid,i4 tup_size,
                    i4  usr_pfrows,struct s_IICGC_ALTRD **astream);
FUNC_EXTERN VOID   IIcga2_close_stream(struct s_IICGC_DESC *cgc,
                    struct s_IICGC_ALTRD *astream);
FUNC_EXTERN STATUS IIcga3_load_stream(struct s_IICGC_DESC *cgc,
                    struct s_IICGC_ALTRD *astream);
FUNC_EXTERN STATUS IIcga4_load_error(struct s_IICGC_DESC *cgc,
                    struct s_IICGC_HARDG *earg);
FUNC_EXTERN i4     IIcga5_get_data(struct s_IICGC_DESC *cgc,
                    i4  message_type,i4 bytes_len,char *bytes_ptr);
FUNC_EXTERN VOID   IIcga6_reset_stream(struct s_IICGC_DESC *cgc);

/* cgccopy */
FUNC_EXTERN VOID    IIcgcp1_write_ack(struct s_IICGC_DESC *cgc,
                     struct _GCA_AK_DATA *ack);
FUNC_EXTERN VOID    IIcgcp2_write_response(struct s_IICGC_DESC *cgc,
                     struct _GCA_RE_DATA *resp);
FUNC_EXTERN STATUS  IIcgcp3_write_data(struct s_IICGC_DESC *cgc,
                     bool end_of_data,
                     i4 bytes_used,
                     DB_DATA_VALUE *copy_dbv);
FUNC_EXTERN i4 IIcgcp4_write_read_poll(struct s_IICGC_DESC *cgc,
                     i4  message_type);
FUNC_EXTERN i4      IIcgcp5_read_data(struct s_IICGC_DESC *cgc,
                     struct _DB_DATA_VALUE *copy_dbv);

/* cgcutils */
FUNC_EXTERN VOID  IIcc1_util_error(struct s_IICGC_DESC *cgc,
                     bool user,u_i4 errorno,i4 numargs,
                     char *a1,char *a2,char *a3,char *a4); /* Internal calls */
FUNC_EXTERN VOID  IIcc3_util_status(STATUS gca_stat,struct _CL_ERR_DESC *os_error,
                      char *stat_buf);

/* cgctrace */
FUNC_EXTERN VOID  IIcgct1_set_trace( struct s_IICGC_DESC *cgc,
				     i4  line_len, PTR sem, 
				     void (*emit_exit)(void) );
FUNC_EXTERN VOID  IIcgct2_gen_trace(struct s_GCA_TR_STATE *trs,
                      struct s_GCA_TR_BUF *trb);			/* Internal calls */
FUNC_EXTERN	VOID  IIcgct3_trace_data(struct s_GCA_TR_STATE *trs,
                      struct s_GCA_TR_BUF *trb,char *start,i4 length);

#endif /* WIN16 */

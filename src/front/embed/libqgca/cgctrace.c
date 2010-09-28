/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/
# define	PRINTFFLOAT		


# include	<compat.h>
# include	<mh.h>			
# include	<er.h>
# include	<me.h>
# include	<mu.h>
# include	<st.h>
# include	<cm.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<adp.h>
# include	<gca.h>
# include	<iicgca.h>		/* For internal trace structures */
# include	<sqlstate.h>

/**
+*  Name: cgctrace.c - This file contains internal routines to trace GCA.
**
**  Description:
**	The routines in this file support the tracing of GCA messages.
**	They provide the trace control and the trace generation.  When this
**	module becomes part of GCA (rather than LIBQGCA) the routines should
**	be moved into GCA itself.  See the internal design document and
**	the header file iicgca.h for more details.
**
**  Defines:
**
**	IIcgct1_set_trace - 	 Resume/suspend tracing.
**	IIcgct2_gen_trace -	 Generate trace data from GCA_SEND/GCA_RECEIVE.
**
**  Locals:
**
**	IIcgct3_trace_data -	 High level internal data structure generator.
**
**  Low Level Trace Routines:
**
**	gtr_id - 		 Trace a GCA_ID.
**	gtr_response - 		 Trace a GCA_RE_DATA.
**	gtr_gname - 		 Trace a GCA_NAME.
**      gtr_gct -                Trace a GCA_XA_SECURE
**	gtr_gcv - 		 Trace a GCA_DATA_VALUE w/o value part.
**	gtr_tdesc - 		 Trace a GCA_TDESC.
**	gtr_cpmap - 		 Trace a GCA_CP_MAP.
**	gtr_att -		 Trace a GCA_COL_ATT.
**	gtr_i4 - 		 Trace a i4.
**	gtr_nat -		 Trace a nat.
**	gtr_name - 		 Trace a fixed length name.
**	gtr_sqlstate -		 Trace an encoded SQLSTATE.
**	gtr_type -		 Map to type names.
**	gtr_value -		 Trace value part of a GCA_DATA_VALUE/tuple.
-*
**  History:
**	19-sep-1988	- Written (ncg)
**	07-dec-1989	- Updated for Phoenix/Alerters. (bjb)
**	19-mar-1990	- Fixed up formatting of event trace. (bjb)
**	27-sep-1990 (barbara)
**	    Trace MD_ASSOC flags.
**	03-oct-1990 (barbara)
**	    Backed out time zone support until dbms is ready.
**	15-nov-1990 (barbara)
**	    Going ahead with time zone support.  Trace GCA_TIMEZONE.
**	02-aug-1991 (teresal)
**	    Add trace for new COPY map: GCA1_C_FROM GCA1_C_INTO
**	24-mar-92 (leighb) DeskTop Porting Change:
**	    defined PRINTFFLOAT so that STprintf() won't be function
**	    prototyped because floats are passed as args to it.
**	02-nov-1992 (larrym)
**	    Add trace for new SQLSTATEs in GCA_ERROR for GCA_PROTO >= 60.
**	    Add function gtr_sqlstate to decode SQLSTATE
**      10-dec-1992 (nandak,mani)
**          Added trace for GCA_XA_SECURE
**	23-nov-1992 (teresal)
**	    Improved trace for decimal data type: Added trace for
**	    decimal data value to gtr_value(); modified precision trace
**	    to print 'real' precision and scale values for decimal data type.
**	01-mar-1993 (kathryn)
**	    Do not buffer query data when it spans GCA messages, trace each
**	    message buffer as it is sent.  This allows for tracing queries
**	    which contain large objects, without requiring huge (2 gig)
**	    continuation buffers.
**      22-sep-1993 (iyer)
**          Modified trace to output two additional members of the GCA
**          message struct for the GCA_XA_SECURE message type. The is a
**          result of the change in GCA_XA_DIS_TRAN_ID struct.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	10-Nov-1994 (athda01)
**		Bug #60916
**		Correct the increment of pointer for multiple text value fields
**		within the trace structure passed to gtr_charval for moving to
**		the GCA trace message buffer.  Variable length query text value
**		elements were only positioning the pointer past the variable
**		text instead of at the end of the fixed field (max length), which
**		caused subsequent handling of query text to be incorrect.
**	14-nov-1995 (nick)
**	    Add trace support for GCA_TIMEZONE_NAME and GCA_YEAR_CUTOFF.
**	16-Nov-95 (gordy)
**	    Trace GCA1_FETCH messages.
**	26-jul-96 (chech02)
**	    Added function prototypes for windows 3.1 port.
**	    Cast STprintf() arguments to nat.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-mar-2001 (abbjo03)
**	    Add NCHR and NVCHR for Unicode support.
**	21-Dec-2001 (gordy)
**	    Replaced gtr_dbv with gtr_att: redefined GCA_COL_ATT structure.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**  16-Jun-2009 (thich01)
**      Add GEOM type to error messaging.
**      Treat GEOM type the same as LBYTE.
**  20-Aug-2009 (thich01)
**      Add all spatial types to error messaging.
**      Treat all spatial types the same as LBYTE.
**	 1-Oct-09 (gordy)
**	    New GCA2_INVPROC message.
**      14-oct-2009 (joea)
**          Add case for DB_BOO_TYPE in gtr_type.
*/
/*
** Local HEX flag (small names to fit on one line)
*/
# define        TN      0               /* Trace as number */
# define        TX      1               /* Trace as hex number */
/*
** Macros pertaining to tracing data values.
*/
# define  VAL_LINE_MAX  200
# define  VAL_NUL_MACRO(val, length)    (((u_char *)(val))[length-1] & 0x01)


VOID 		IIcgct3_trace_data();
static	VOID	gtr_id(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	VOID	gtr_response(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	VOID	gtr_gname(GCA_TR_STATE *, i4, char *, char *, char *, 
			char **, i4  *);
static  VOID    gtr_gct(GCA_TR_STATE *, i4, char *, char **,
			i4 *);         /* trace routine for XA */
static	VOID	gtr_gcv(GCA_TR_STATE *, i4, char *, char **, i4*);
static	VOID	gtr_tdesc(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	VOID	gtr_cpmap(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	VOID	gtr_att(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	VOID	gtr_i4(GCA_TR_STATE *, i4, char *, char **, 
			i4 *, i4, i4 *);
static	VOID	gtr_nat(GCA_TR_STATE *, i4, char *, char **, i4  *, 
			i4, i4  *);
static	VOID	gtr_name(GCA_TR_STATE *, i4, char *, char **, i4  *, i4);
static	VOID	gtr_sqlstate(GCA_TR_STATE *, i4, char *, char **, i4  *);
static	char	*gtr_type(i4, char *);
static	VOID	gtr_value(GCA_TR_STATE *, i4, char *, char **, i4  *,
			DB_DATA_VALUE *);
static  void	gtr_charval(GCA_TR_STATE *trs, i4  indent, i4  titlen,
		    i4  datlen, char **cp, i4  *left, DB_DATA_VALUE *dbv);
static  void	gtr_longval(GCA_TR_STATE *trs, i4  indent, char **cp,
			    i4  *left, DB_DATA_VALUE *dbv);

/*
** Local string data
*/
static char *spaces     = ERx("                                      ");
static char *not_enough = ERx("not enough data");
static char *trace_off  = ERx("-- Tracing turned off till End of Message --\n");
static char *eom_ignore =
		    ERx("-------- End of GCA Message (Untraced) -------\n\n");

/*{
+*  Name: IIcgct1_set_trace - Resume/suspend GCA message tracing.
**
**  Description:
**	This routine prepares the internal trace state for later message
**	tracing.  Tracing is resumed by specifying a non-null trace data emit
**	function, and suspended by specifying a null emit function.
**
**	Protocol Level:  This value is set to the running protocol of the
**	GCA client.  The value should be used for future differentiation
**	between different GCA message layouts that depend on the protocol
**	level.  For example assume message M has a layout L which in version
**	V introduced a new field F in the middle of the data.  The code to
**	trace the data of message M would be:
**		case M:
**		    trace start of L;
**		    if version >= V then
**			trace field F;
**		    trace end of L;
**
**  Notes:
**	The interface to this routine should be modified when collapsed into
**	a GCA trace service.
**
**  Inputs:
**	cgc		- Client general communications descriptor.
**	line_len	- Line length.  If < 80, set to 80.
**	sem		Semaphore to protect emit_exit;
**	emit_exit	- Routine to call when writing a trace line.  This
**		  	  routine interface is:
**				routine(action, length, buffer);
**			  If emit_exit is null then tracing is suspended.
**
**  Outputs:
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	09-dec-1988	- Now sets the protocol level (ncg)
**	01-mar-1993	- Initialize gca_gcv_no (kathryn)
*/

VOID
IIcgct1_set_trace( cgc, line_len, sem, emit_exit )
IICGC_DESC	*cgc;
i4		line_len;
PTR		sem;
VOID		(*emit_exit)();
{
    if ( cgc )
    {
	/* Initialize the trace state */
	cgc->cgc_trace.gca_protocol    = cgc->cgc_version;
	cgc->cgc_trace.gca_trace_sem   = sem;
	cgc->cgc_trace.gca_emit_exit   = emit_exit;
	cgc->cgc_trace.gca_line_length = line_len < 80 ? 80 : line_len;
	cgc->cgc_trace.gca_d_continue  = 0;
	cgc->cgc_trace.gca_flags       = 0;
	cgc->cgc_trace.gca_gcv_no      = 0;
    }

} /* IIcgct1_set_trace */

/*{
+*  Name: IIcgct2_gen_trace - Trace the current GCA message.
**
**  Description:
**	This routine executes the GCA message trace generation.  The routine
**	formats the GCA message buffer based on the message type and
**	continuation flag (end_of_data).  Each line is passed to the trace
**	emit exit routine to allow the data to be displayed.
**
**  Notes:
**	This routine will be completely hidden within GCA when a GCA trace
**	service is provided.
**
**  Some GCA messages are not yet traced:
**		GCA_TUPLES
**		GCA_CDATA
**		GCA_TRACE
**
**  Adding new GCA messages:
**	When new GCA messages are added an attempt should be made to trace
**	their contents.  The default will be to leave their contents untraced.
**	In order to trace a new message follow the following steps:
**	1. If the message has tuple data add it to the note above,  and make
**	   sure to check for it in the code that prohibits tracing.
**	2. Add the message type to the switch statement.
**	3. If necessary add the code to generate the contents.  If possible
**	   use pre-existing low level routines.
**
**  Pseudo Code for Trace Generation:
**
**	emit trace header (message type, data length, association id);
**
**	-- Check for continuation errors/interrupts
**	if (message_type != last_message_type) and (continued data)
**	    emit "Last message interrupted - not traced";
**	    continued data = 0;
**	endif
**
**	-- Handle continuation protocol
**	if (end_of_data) and (no continued data)
**	    start = new buffer;
**	    length = length of new data;
**	else				-- Not end of data or continued data
**	    copy new data into continued buffer;
**	    if (end_of_data)
**		start = continued buffer;
**		length = length of continued data;
**	    else			-- More continued data
**		start = null;
**		length = length of new data;
**	    endif
**	endif
**
**	if (not end_of_data)		-- Message continued
**	    emit "Tracing delayed until end of data";
**	    return;
**	endif
**
**	case (message_type)		-- Process/format message data
**	    ...
**	    emit trace data;
**	    ...
**	endcase
**	flush trace;			-- Close message tracing
**	continued data = 0;
**
**  Inputs:
**	trs		- Internal trace state.
**	trb		- GCA message trace buffer.
**
**  Outputs:
**	Returns:
**	    	None
**	Errors:
**		Continuation buffer is too large.
**		Error allocating continuation buffer.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	01-mar-1993 (kathryn)
**	    Do not buffer GCA_QUERY data when it spans GCA messages.
**	    Trace each message buffer as it is sent, but only generate
**	    header information for the first message buffer.
**	01-apr-1993 (kathryn)
**	    No longer buffer INVPROC data either. Trace each message as
**	    it is sent.  This allows for tracing database procedure parameters
**          which contain large objects, without requiring huge (2 gig)
**          continuation buffers.
**	10-apr-1993 (kathryn)
**	    Update gca_gcv_no correctly.
**	23-aug-1993 (larrym)
**	    Added support for GCA1_INVPROC tracing.
**	01-oct-1993 (kathryn)
**	    Turn off GCA_TR_INVAL/GCA_TR_CONT flags when GCA_ATTENTION is sent.
**	 1-Oct-09 (gordy)
**	    Added support for GCA2_INVPROC tracing.
*/

VOID
IIcgct2_gen_trace(trs, trb)
GCA_TR_STATE	*trs;
GCA_TR_BUF	*trb;
{
    char	buf[100];		/* Trace buffer */
    char	*start;			/* Start of trace data */
    i4		length;			/* Length of trace data */
    char	*new_buf;		/* New continued buffer */
    u_i4	new_len;		/* New continued buffer length */
    char	*new_err;		/* Error aloocating continue buffer */

    /*
    ** This should work, so just ignore request if fails.
    */
    if ( MUp_semaphore( (MU_SEMAPHORE *)trs->gca_trace_sem ) != OK )
	return;

    /* Trace banner  -
    ** Generate for each new message type sent - Do not generate for
    ** continuation messages.
    */
    if (trb->gca_message_type == GCA_ATTENTION)
    {
	trs->gca_flags &=  ~GCA_TR_CONT;
	trs->gca_flags &=  ~GCA_TR_INVAL;
    }

    if (!(trs->gca_flags &  GCA_TR_CONT))
    {
	char stat_buf3[30];
        STprintf(buf, ERx("GCA Service: %s\n"), trb->gca_service == GCA_SEND ?
		ERx("GCA_SEND") : ERx("GCA_RECEIVE"));
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
        STprintf(buf, ERx(" gca_association_id: %d\n"),
		trb->gca_association_id);
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
        STprintf(buf, ERx(" gca_message_type: %s\n"),
		IIcc2_util_type_name(trb->gca_message_type, stat_buf3));
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
        STprintf(buf, ERx(" gca_data_length: %d\n"), trb->gca_d_length);
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
        STprintf(buf, ERx(" gca_end_of_data: %s\n"),
		trb->gca_end_of_data ? ERx("TRUE") : ERx("FALSE"));
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }

    /* Check to see we did not turn tracing off for this message */
    if (trs->gca_flags & GCA_TR_IGNORE)
    {
	if (trb->gca_end_of_data)
	{
	    /* Completed GCA message */
	    trs->gca_d_continue = 0;
	    (*trs->gca_emit_exit)(GCA_TR_FLUSH, 0, eom_ignore);
	    trs->gca_flags &= ~GCA_TR_IGNORE;
	}
	else
	{
	    (*trs->gca_emit_exit)(GCA_TR_FLUSH, 0, trace_off);
	}
	MUv_semaphore( (MU_SEMAPHORE *)trs->gca_trace_sem );
	return;
    }

    /* Check for no illegal continuations */
    if (trs->gca_d_continue && trb->gca_message_type != trs->gca_message_type)
    {
	char stat_buf2[20], stat_buf3[30];
	STprintf(buf, ERx("-- Message %s interrupted with %s; not traced --\n"),
		      IIcc2_util_type_name(trs->gca_message_type, stat_buf2),
		      IIcc2_util_type_name(trb->gca_message_type, stat_buf3));
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	trs->gca_d_continue = 0;
    }

    trs->gca_message_type = trb->gca_message_type;

    /*
    ** Handle continuation protocol -
    ** If eod and no buffered data then the entire message fits in one buffer,
    ** and the buffer is traced.  If this is not eod then for some messages
    ** the data will be buffered until the end of the entire message.
    ** Tuple, Trace, Copy and Query data is not buffered.
    */
    if (   (trb->gca_end_of_data && trs->gca_d_continue == 0)
        || trb->gca_message_type == GCA_TUPLES
	|| trb->gca_message_type == GCA_CDATA
	|| trb->gca_message_type == GCA_QUERY
	|| trb->gca_message_type == GCA_INVPROC
        || trb->gca_message_type == GCA1_INVPROC
	|| trb->gca_message_type == GCA2_INVPROC
	|| trb->gca_message_type == GCA_TRACE)
    {
	if (!(trs->gca_flags & GCA_TR_CONT))
	    trs->gca_gcv_no = 0;
	start = trb->gca_data_area;
	length = trb->gca_d_length;
	/* Check for messages that are empty strings */
	if (length == 1 && *start == '\0')
	    length = 0;
    }
    else				/* Not end of data or continued data */
    {
	/*
	** Either buffer the current continued data, or add to the already
	** buffered data.  This buffering allows for simple tracing later on.
	** If data does not fit in continued buffer (if there is one) then we
	** need to allocate a larger (or new) continued buffer.
	*/
	if (trs->gca_l_continue < trs->gca_d_continue + trb->gca_d_length)
	{
	    /*
	    ** Allocate a new buffer (give it a bit of padding) and copy any
	    ** already buffered data.  Then copy the new data onto this.
	    ** Validate that the continuation buffer size is reasonable, and
	    ** if so make sure we do not run out of dynamic memory.  If we
	    ** do then turn off tracing for this message.
	    */
	    new_len = trs->gca_l_continue + trb->gca_d_length * 2;
	    if (new_len > GCA_TR_MAXTRACE)	/* Too large a buffer */
		new_err = ERx("-- Continuation buffer is too large --\n");
	    else
	    {
		new_buf = (char *)MEreqmem((u_i4)0, new_len,TRUE, (STATUS *)0);
		if (new_buf == (char *)0)	/* Could not allocate */
		    new_err =
			ERx("-- Error allocating continuation buffer --\n");
		else
		    new_err = (char *)0;
	    }

	    if (new_err != (char *)0)		/* Handle allocation errors */
	    {
		(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, new_err);
		if (trb->gca_end_of_data)
		    (*trs->gca_emit_exit)(GCA_TR_FLUSH, 0, eom_ignore);
		else
		{
		    /* Ignore remaining data till eom */
		    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, trace_off);
		    trs->gca_flags |= GCA_TR_IGNORE;
		}
		trs->gca_d_continue = 0;
		MUv_semaphore( (MU_SEMAPHORE *)trs->gca_trace_sem );
		return;
	    }

	    /* Is there any old data to add first */
	    if (trs->gca_d_continue)
		MEcopy((PTR)trs->gca_continued_buf, (u_i2)trs->gca_d_continue,
		       (PTR)new_buf);
	    /* Is there an old buffer to free */
	    if (trs->gca_l_continue)
		MEfree((PTR)trs->gca_continued_buf);
	    trs->gca_continued_buf = new_buf;
	    trs->gca_l_continue = new_len;
	    /* trs->gca_d_continue is left alone - may be zero */
	}
	/* Copy new message data into continued buffer */
	MEcopy((PTR)trb->gca_data_area, (u_i2)trb->gca_d_length,
	       (PTR)(trs->gca_continued_buf + trs->gca_d_continue));
	trs->gca_d_continue += trb->gca_d_length;

	if (trb->gca_end_of_data)		/* We're done with data */
	{
	    start = trs->gca_continued_buf;
	    length = trs->gca_d_continue;
	}
	else
	{
	    (*trs->gca_emit_exit)(GCA_TR_FLUSH, 0,
			ERx("-- Tracing delayed until End of Message --\n"));
	    MUv_semaphore( (MU_SEMAPHORE *)trs->gca_trace_sem );
	    return;
	}
    } /* If end of data and no continuation */

    IIcgct3_trace_data(trs, trb, start, length);	/* Do real tracing */

    /* Completed GCA message */
    if ( (!trb->gca_end_of_data)
	&& ((trb->gca_message_type == GCA_INVPROC)
           ||(trb->gca_message_type == GCA1_INVPROC)
           ||(trb->gca_message_type == GCA2_INVPROC)
	   ||(trb->gca_message_type == GCA_QUERY))
       )
       /* This data spans GCA messages buffers - trace will be generated as
       ** each buffer is sent, so set the flag indicating that the next
       ** message buffer is a continuation of this query data. */
    {
        trs->gca_flags |=  GCA_TR_CONT;
    }
    else
    {
	trs->gca_flags &=  ~GCA_TR_CONT;
	trs->gca_flags &= ~GCA_TR_INVAL;
        trs->gca_d_continue = 0;
        (*trs->gca_emit_exit)(GCA_TR_FLUSH, 0,
		ERx("-------------- End of GCA Message ------------\n\n"));
    }

    MUv_semaphore( (MU_SEMAPHORE *)trs->gca_trace_sem );
    return;
} /* IIcgct2_gen_trace */

/*{
+*  Name: IIcgct3_trace_data - Highest low level routine to trace GCA messages.
**
**  Description:
**	Based on the message type this routine executes the tracing.
**	All the low level data structures are traced via other routines
**	that walk along the message buffer copying and printing as they
**	go along.
**
**  Inputs:
**	trs		- Internal trace state.
**	trb		- GCA message trace buffer.  Currently unused, but
**			  will be required for its descriptor if we do trace
**			  tuple messages.
**	start		- Start of message data.
**	length		- Length of message data.
**
**  Outputs:
**	Returns:
**	    	None
**	Errors:
**		Internal trace errors written when trace is corrupted.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	02-nov-1988	- Printed number of unread bytes (ncg)
**	13-mar-1989	- GCA_SECURE now passes xids as data. (bjb)
**	07-dec-1989	- Updated for GCA_EVENT for Phoenix/Alerters. (bjb)
**	19-mar-1990	- Fixed up formatting of event trace. (bjb)
**	27-sep-1990 (barbara)
**	    Trace MD_ASSOC flags.
**	15-nov-1990 (barbara)
**	    Trace GCA_TIMEZONE session startup parameter.
**	08-mar-1991 (teresal)
**	    Trace GCA_MISC_PARM session startup parameter.
**	02-nov-1992 (larrym)
**	    Add trace for new SQLSTATEs in GCA_ERROR for GCA_PROTO >= 60.
**	    Add function gtr_sqlstate to decode SQLSTATE
**      10-dec-1992 (nandak,mani,larrym)
**          Add trace for GCA_XA_SECURE
**	01-mar-1993 (kathryn)
**	    Only generate header information if this is a new message type.
**	    Added functionality to trace query messages transparently
**	    across GCA message buffers as each buffer is sent.
**	24-may-1993 (kathryn)
**	    Add tracing for new GCA1_DELETE message.
**	23-Aug-1993 (larrym)
**	    Added support for new GCA1_INVPROC message.
**	15-sep-1993 (kathryn)
**	    Cast PTR arguments to MEcopy to match prototypes. 
**	3-oct-93 (robf)
**          Trace GCA_SL_TYPE modifier
**	8-nov-93 (robf)
**          Trace GCA_PROMPT, GCA_PRREPLY messages, GCA_CAN_PROMPT modifier
**	14-nov-95 (nick)
**	    Trace GCA_TIMEZONE_NAME & GCA_YEAR_CUTOFF
**	16-Nov-95 (gordy)
**	    Trace GCA1_FETCH messages.
**	13-feb-2007 (dougi)
**	    Trace GCA2_FETCH messages.
**	15-Mar-07 (gordy)
**	    Minor clean-up in GCA messages.
**	07-Aug-07 (gupsh01)
**	    Added GCA_DATE_ALIAS.
**	 1-Oct-09 (gordy)
**	    Added support for new GCA2_INVPROC message.
*/

VOID
IIcgct3_trace_data(trs, trb, start, length)
GCA_TR_STATE	*trs;
GCA_TR_BUF	*trb;			/* NOT YET USED */
char		*start;
i4		length;
{
    i4		i, j;			/* For varying length messages */
    i4	tln1, tln2;		/* Temp i4s */
    i4		tn;			/* Temp i4  */
    char	title[50];		/* Trace titles */
    char	*assoc_parm;		/* Modify assoc params */
    bool	default_param;		/* Default or unknown assoc param */
    bool	trace_value;		/* May trace value of assoc parm */
    GCA_DATA_VALUE gcv;			/* Used to skip assoc parm */
    i4		skip;			/*   "  "   "      "   " */

    if (!(trs->gca_flags & GCA_TR_CONT))
    	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx(" gca_data_area: "));
    switch (trs->gca_message_type)
    {
      case GCA_ACCEPT:
      case GCA_Q_ETRAN:
      case GCA_ABORT:
      case GCA_ROLLBACK:
      case GCA_COMMIT:
      case GCA_Q_STATUS:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("no data for message\n"));
	break;

      case GCA_MD_ASSOC:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_SESSION_PARMS\n"));
	gtr_i4(trs, 2, ERx("gca_l_user_data"), &start, &length, TN, &tln1);
	/* Trace fixed number of user parameters */
	for (i = 0; i < tln1 && length > 0; i++)
	{
	    default_param = FALSE;
	    trace_value = TRUE;
	    STprintf(title, ERx("  gca_user_data[%d]:\n"), i);
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, title);
	    MEcopy((PTR)start, (u_i2)sizeof(tln2), (PTR)&tln2);
	    switch(tln2) {
	      case GCA_VERSION:
		assoc_parm = ERx("GCA_VERSION\n");
		break;
	      case GCA_QLANGUAGE:
		assoc_parm = ERx("GCA_QLANGUAGE\n");
		break;
	      case GCA_TIMEZONE:
	        assoc_parm = ERx("GCA_TIMEZONE\n");
	        break;
	      case GCA_APPLICATION:
		assoc_parm = ERx("GCA_APPLICATION\n");
		break;
	      case GCA_CWIDTH:
		assoc_parm = ERx("GCA_CWIDTH\n");
		break;
	      case GCA_TWIDTH:
		assoc_parm = ERx("GCA_TWIDTH\n");
		break;
	      case GCA_I1WIDTH:
		assoc_parm = ERx("GCA_I1WIDTH\n");
		break;
	      case GCA_I2WIDTH:
		assoc_parm = ERx("GCA_I2WIDTH\n");
		break;
	      case GCA_I4WIDTH:
		assoc_parm = ERx("GCA_I4WIDTH\n");
		break;
	      case GCA_DECFLOAT:
		assoc_parm = ERx("GCA_DECFLOAT\n");
		break;
	      case GCA_RES_STRUCT:
		assoc_parm = ERx("GCA_RES_STRUCT\n");
		break;
	      case GCA_IDX_STRUCT:
		assoc_parm = ERx("GCA_IDX_STRUCT\n");
		break;
	      case GCA_EUSER:
		assoc_parm = ERx("GCA_EUSER\n");
		break;
	      case GCA_MATHEX:
		assoc_parm = ERx("GCA_MATHEX\n");
		break;
	      case GCA_EXCLUSIVE:
		assoc_parm = ERx("GCA_EXCLUSIVE\n");
		break;
	      case GCA_WAIT_LOCK:
		assoc_parm = ERx("GCA_WAIT_LOCK\n");
		break;
	      case GCA_XUPSYS:
		assoc_parm = ERx("GCA_XUPSYS\n");
		break;
	      case GCA_SUPSYS:
		assoc_parm = ERx("GCA_SUPSYS\n");
		break;
	      case GCA_GRPID:
		assoc_parm = ERx("GCA_GRPID\n");
		break;
	      case GCA_RUPASS:
		assoc_parm = ERx("GCA_RUPASS\n");
		trace_value = FALSE;
		break;
	      case GCA_APLID:
		assoc_parm = ERx("GCA_APLID\n");
		trace_value = FALSE;
		break;
	      case GCA_DBNAME:
		assoc_parm = ERx("GCA_DBNAME\n");
		break;
	      case GCA_F4STYLE:
		assoc_parm = ERx("GCA_F4STYLE\n");
		break;
	      case GCA_F8STYLE:
		assoc_parm = ERx("GCA_F8STYLE\n");
		break;
	      case GCA_F4WIDTH:
		assoc_parm = ERx("GCA_F4WIDTH\n");
		break;
	      case GCA_F8WIDTH:
		assoc_parm = ERx("GCA_F8WIDTH\n");
		break;
	      case GCA_F4PRECISION:
		assoc_parm = ERx("GCA_F4PRECISION\n");
		break;
	      case GCA_F8PRECISION:
		assoc_parm = ERx("GCA_F8PRECISION\n");
		break;
	      case GCA_NLANGUAGE:
		assoc_parm = ERx("GCA_NLANGUAGE\n");
		break;
	      case GCA_DATE_FORMAT:
		assoc_parm = ERx("GCA_DATE_FORMAT\n");
		break;
	      case GCA_MPREC:
		assoc_parm = ERx("GCA_MPREC\n");
		break;
	      case GCA_MLORT:
		assoc_parm = ERx("GCA_MLORT\n");
		break;
	      case GCA_MSIGN:
		assoc_parm = ERx("GCA_MSIGN\n");
		break;
	      case GCA_DECIMAL:
		assoc_parm = ERx("GCA_DECIMAL\n");
		break;
	      case GCA_GW_PARM:
		assoc_parm = ERx("GCA_GW_PARM\n");
		break;
	      case GCA_CAN_PROMPT:
		assoc_parm = ERx("GCA_CAN_PROMPT\n");
		break;
	      case GCA_SL_TYPE:
		assoc_parm = ERx("GCA_SL_TYPE\n");
		break;
	      case GCA_MISC_PARM:
	        assoc_parm = ERx("GCA_MISC_PARM\n");
	        break;
	      case GCA_TIMEZONE_NAME:
	        assoc_parm = ERx("GCA_TIMEZONE_NAME\n");
	        break;
	      case GCA_YEAR_CUTOFF:
	        assoc_parm = ERx("GCA_YEAR_CUTOFF\n");
	        break;
	      case GCA_DATE_ALIAS:
	        assoc_parm = ERx("GCA_DATE_ALIAS\n");
	        break;
	      default:
		default_param = TRUE;
	    }
	    if (default_param)
	    {
		gtr_i4(trs, 3, ERx("gca_p_index"), &start, &length,
				TX, &tln2);
	    }
	    else
	    {
		(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("   gca_p_index: "));
		(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, assoc_parm);
		start += sizeof(tln2);
		length -= sizeof(tln2);
	    }
	    if (trace_value)
		gtr_gcv(trs, 3, ERx("gca_p_value"), &start, &length);
	    else	
	    {
		(*trs->gca_emit_exit)(GCA_TR_WRITE, 0,
			ERx("   gca_p_value: GCA_USER_DATA not traced\n"));
		skip = sizeof(gcv.gca_type) +
			sizeof(gcv.gca_precision) +
			sizeof(gcv.gca_l_value);
		MEcopy((PTR)start, (u_i2)skip, (PTR)&gcv);
		skip += gcv.gca_l_value;
		length -= skip;
		start += skip;
	    }
	}
	break;
      case GCA_REJECT:
      case GCA_RELEASE:
      case GCA_ERROR:
	if (length == 0)
	{
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("no GCA_ER_DATA\n"));
	    break;
	}
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_ER_DATA\n"));
	gtr_i4(trs, 2, ERx("gca_l_e_element"), &start, &length, TN, &tln1);
	/* Trace fixed number of error elements */
	for (i = 0; i < tln1 && length > 0; i++)
	{
	    STprintf(title, ERx("  gca_e_element[%d]:\n"), i);
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, title);
	    /*
	    ** If proto > 60, then using SQLSTATEs
	    ** SQLSTATES are char[5], but are "packed" into an i4 because
	    ** of GCC issues.  Since SQLSTATES replace Generic Errors, the
	    ** "packed" i4 is sent in the gca_id_error field.  gtr_sqlstate
	    ** decodes the i4 back into a char[5].
	    */
	    if (trs->gca_protocol >= GCA_PROTOCOL_LEVEL_60)
	    {
		gtr_sqlstate(trs, 3, ERx("gca_sqlstate"), &start, &length );
	    }
	    else
	    {
		gtr_i4(trs, 3, ERx("gca_id_error"), &start, &length, TN,
			&tln2);
	    }
	    gtr_i4(trs, 3, ERx("gca_id_server"), &start, &length, TN,
			&tln2);
	    gtr_i4(trs, 3, ERx("gca_server_type"), &start, &length, TN,
			&tln2);
	    gtr_i4(trs, 3, ERx("gca_severity"), &start, &length, TX,
			&tln2);
	    gtr_i4(trs, 3, ERx("gca_local_error"), &start, &length, TN,
			&tln2);
	    gtr_i4(trs, 3, ERx("gca_l_error_parm"),&start, &length, TN,
			&tln2);
	    /* Trace fixed number of error parameters */
	    for (j = 0; j < tln2 && length > 0; j++)
	    {
		STprintf(title, ERx("gca_error_parm[%d]"), j);
		gtr_gcv(trs, 3, title, &start, &length);
	    }
	}
	break;
      case GCA_Q_BTRAN:
      case GCA_S_BTRAN:
	if (length == 0)
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("no GCA_TX_DATA\n"));
	else
	{
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_TX_DATA\n"));
	    gtr_id(trs, 2, ERx("gca_tx_id"), &start, &length);
	    gtr_i4(trs, 2, ERx("gca_tx_type"), &start, &length, TN, &tln1);
	}
	break;
      case GCA_S_ETRAN:
      case GCA_DONE:
      case GCA_REFUSE:
      case GCA_RESPONSE:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_RE_DATA\n"));
	gtr_response(trs, 2, (char *)0, &start, &length);
	break;
      case GCA_QUERY:
      case GCA_DEFINE:
      case GCA_CREPROC:
      case GCA_DRPPROC:
	if (!(trs->gca_flags & GCA_TR_CONT))	/* first message buffer */
	{
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_Q_DATA\n"));
	    gtr_i4(trs, 2, ERx("gca_language_id"), &start, &length,
			TN, &tln1);
	    gtr_i4(trs, 2, ERx("gca_query_modifier"), &start, &length, TX,
		    &tln1);
	}
	/* Trace varying number of query values -
	** Character datatype query values may span message buffers. This
	** message buffer could begin in middle of query value.(GCA_TR_INVAL)
	*/
	for (i = 0; length > 0; i++)
	{
	    if (!(trs->gca_flags & GCA_TR_INVAL))	/* new query value */
	        STprintf(title, ERx("gca_qdata[%d]"), trs->gca_gcv_no++);
	    gtr_gcv(trs, 2, title, &start, &length);
	}
	break;
      case GCA_INVOKE:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_IV_DATA\n"));
	gtr_id(trs, 2, ERx("gca_qid"), &start, &length);
	/* Trace varying number of repeat query parameters */
	for (i = 0; length > 0; i++)
	{
	    STprintf(title, ERx("gca_qparm[%d]"), i);
	    gtr_gcv(trs, 2, title, &start, &length);
	}
	break;
      case GCA_FETCH:
      case GCA_CLOSE:
      case GCA_QC_ID:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_ID\n"));
	gtr_id(trs, 2, (char *)0, &start, &length);
	break;
      case GCA1_FETCH:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA1_FT_DATA\n"));
	gtr_id(trs, 2, ERx("gca_cursor_id"), &start, &length);
   	gtr_nat(trs, 2, ERx("gca_rowcount"), &start, &length, TN, &tn);
	break;
      case GCA2_FETCH:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA2_FT_DATA\n"));
	gtr_id(trs, 2, ERx("gca_cursor_id"), &start, &length);
   	gtr_nat(trs, 2, ERx("gca_rowcount"), &start, &length, TN, &tn);
   	gtr_nat(trs, 2, ERx("gca_anchor"), &start, &length, TN, &tn);
   	gtr_nat(trs, 2, ERx("gca_offset"), &start, &length, TN, &tn);
	break;
      case GCA_DELETE:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_DL_DATA\n"));
	gtr_id(trs, 2, ERx("gca_cursor_id"), &start, &length);
	gtr_gname(trs, 2, ERx("gca_table_name"), (char *)0, (char *)0,
		  &start, &length);
	break;
      case GCA1_DELETE:
     	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA1_DL_DATA\n"));
	gtr_id(trs, 2, ERx("gca_cursor_id"), &start, &length);
	gtr_gname(trs, 2, ERx("gca_owner_name"), (char *)0, (char *)0,
				  &start, &length);
	gtr_gname(trs, 2, ERx("gca_table_name"), (char *)0, (char *)0,
			  &start, &length);
        break;
      case GCA_ATTENTION:
      case GCA_NP_INTERRUPT:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_AT_DATA\n"));
	gtr_i4(trs, 2, ERx("gca_itype"), &start, &length, TN, &tln1);
	break;
      case GCA_TDESCR:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_TD_DATA\n"));
	gtr_tdesc(trs, 2, (char *)0, &start, &length);
	break;
      case GCA_TUPLES:
      case GCA_CDATA:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_TU_DATA not traced\n"));
	length = 0;
	break;
      case GCA_C_INTO:
      case GCA_C_FROM:
      case GCA1_C_INTO:
      case GCA1_C_FROM:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_CP_MAP\n"));
	gtr_cpmap(trs, 2, (char *)0, &start, &length);
	break;
      case GCA_ACK:
      case GCA_IACK:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_AK_DATA\n"));
	gtr_i4(trs, 2, ERx("gca_ak_data"), &start, &length, TN, &tln1);
	break;
      case GCA_TRACE:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_TR_DATA not traced\n"));
	length = 0;
	break;
      case GCA_INVPROC:
      case GCA1_INVPROC:
      case GCA2_INVPROC:
	if (!(trs->gca_flags & GCA_TR_CONT))    /* first message buffer */
	{
            if (trs->gca_message_type == GCA2_INVPROC)
	    {
                (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA2_IP_DATA\n"));
                gtr_gname(trs, 2, ERx("gca_proc_name"), (char *)0, (char *)0,
                        &start, &length);
                gtr_gname(trs, 2, ERx("gca_owner_name"), (char *)0, (char *)0,
                        &start, &length);
	    }
            else  if (trs->gca_message_type == GCA1_INVPROC)
            {
                (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA1_IP_DATA\n"));
                gtr_id(trs, 2, ERx("gca_procid"), &start, &length);
                gtr_gname(trs, 2, ERx("gca_owner_name"), (char *)0, (char *)0,
                        &start, &length);
            }
            else
            {
	        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_IP_DATA\n"));
	        gtr_id(trs, 2, ERx("gca_procid"), &start, &length);
	    }
	    gtr_i4(trs, 2, ERx("gca_proc_mask"), &start, &length,
			TX, &tln1);
	}
	/* Trace varying number of procedure parameters */
	for (i = 0; length > 0; i++)
	{
	    if (!(trs->gca_flags & GCA_TR_INVAL))
	    {
	    	STprintf(title, ERx("  gca_param[%d]:\n"), trs->gca_gcv_no++);
	    	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, title);
	    	gtr_gname(trs, 3, ERx("gca_parname"), (char *)0, (char *)0,
		      &start, &length);
	    	gtr_i4(trs, 3, ERx("gca_parm_mask"), &start, &length, TX,
			&tln1);
	    }
	    gtr_gcv(trs, 3, ERx("gca_parvalue"), &start, &length);
	}
	break;
      case GCA_RETPROC:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_RP_DATA\n"));
	gtr_id(trs, 2, ERx("gca_procid"), &start, &length);
	gtr_i4(trs, 2, ERx("gca_retstat"), &start, &length, TN, &tln1);
	break;
      case GCA_SECURE:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_TX_DATA\n"));
	gtr_id(trs, 2, ERx("gca_tx_id"), &start, &length);
	gtr_i4(trs, 2, ERx("gca_tx_type"), &start, &length, TN, &tln1);
	break;
      case GCA_XA_SECURE:
 	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_XA_DIS_TRAN_ID\n"));
          gtr_gct(trs, 2, ERx("gca_xa_dis_tran_id"), &start, &length);
 	break;
      case GCA_EVENT:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_EV_DATA\n"));
	gtr_gname(trs, 2, ERx("gca_evname"), (char *)0, (char *)0,
		  &start, &length);
	gtr_gname(trs, 2, ERx("gca_evowner"), (char *)0, (char *)0,
		  &start, &length);
	gtr_gname(trs, 2, ERx("gca_evdb"), (char *)0, (char *)0,
		  &start, &length);
	gtr_gcv(trs, 2, ERx("gca_evtime"), &start, &length);
   	gtr_nat(trs, 2, ERx("gca_l_evvalue"), &start, &length, TN, &tn);
        for (i = 0; i < tn && length > 0; i++)
	{
	    STprintf(title, ERx("gca_evvalue[%d]"), i);
	    gtr_gcv(trs, 2, title, &start, &length);
	}
	break;

      case GCA_PROMPT:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_PROMPT_DATA\n"));
   	gtr_nat(trs, 2, ERx("gca_prflags"), &start, &length, TN, &tn);
   	gtr_nat(trs, 2, ERx("gca_prtimeout"), &start, &length, TN, &tn);
   	gtr_nat(trs, 2, ERx("gca_prmaxlen"), &start, &length, TN, &tn);
   	gtr_nat(trs, 2, ERx("gca_l_prmesg"), &start, &length, TN, &tn);
        for (i = 0; i < tn && length > 0; i++)
	{
	    STprintf(title, ERx("gca_prmesg[%d]"), i);
	    gtr_gcv(trs, 2, title, &start, &length);
	}
	break;

      case GCA_PRREPLY:
	{
	bool noecho;
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("GCA_PRREPLY_DATA\n"));
   	gtr_nat(trs, 2, ERx("gca_prflags"), &start, &length, TN, &tn);
	if(tn&GCA_PRREPLY_NOECHO)
		noecho=TRUE;
	else
		noecho=FALSE;
   	gtr_nat(trs, 2, ERx("gca_l_prvalue"), &start, &length, TN, &tn);
	/*
	** If noecho is set we don't print the reply to the trace log,
	** since this is likely to be something sensitive (like a password)
	*/
	if(noecho)
	{
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, 
			ERx("   NOECHO REPLY not traced.\n"));
	    length=0;
		
	}
	else
	{
            for (i = 0; i < tn && length > 0; i++)
	    {
	        STprintf(title, ERx("gca_prvalue[%d]"), i);
	        gtr_gcv(trs, 2, title, &start, &length);
	    }
	}
	break;
	}

      default:
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("unknown message format\n"));
	length = 0;
	break;
    } /* switch on message type */
    /* Is there garbage data left? */
    if (length > 0)
    {
	STprintf(title, ERx(" -- %d data bytes left --\n"), length);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, title);
    }
} /* IIcgct3_trace_data */

/*{
+*  Name: gtr_id - Low level trace routine to trace GCA_ID.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Not enough data for tracing object.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
*/

static VOID
gtr_id(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    i4	l;			/* Temp to dump from */
    char	buf[100];		/* Buffer for all data */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    if (*left >= sizeof(GCA_ID))
    {
	gtr_i4(trs, indent, ERx("gca_index[0]"), cp, left, TN, &l);
	gtr_i4(trs, indent, ERx("gca_index[1]"), cp, left, TN, &l);
	gtr_name(trs, indent, ERx("gca_name"), cp, left, GCA_MAXNAME);
    }
    else
    {
	STprintf(buf, ERx("%.*s-- %s --\n"), indent, spaces, not_enough);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left = 0;
    }
} /* gtr_id */



/*{
+*  Name: gtr_response - Low level trace routine to trace GCA_RE_DATA.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of message area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	08-mar-1991 (barbara)
**	    Trace new logical key fields.
**	20-mar-1991 (teresal)
**	    Modify trace of logical key value to print
**	    a 2 digit hex value for each byte (easier to read).
*/

static VOID
gtr_response(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    GCA_RE_DATA	r;			/* Local structure to dump from */
    char	buf[200];		/* Buffer for display data */
    i4		i;

    MEfill(sizeof(r), 0, (PTR)&r);
    MEcopy((PTR)*cp, (u_i2)(min(sizeof(r), *left)), (PTR)&r);
    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    STprintf(buf,
	ERx("%.*sgca_rid: %d\n%.*sgca_rqstatus: 0x0%x\n"),
	    indent, spaces, r.gca_rid,
	    indent, spaces, r.gca_rqstatus);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    STprintf(buf,
	ERx("%.*sgca_rowcount: %d\n%.*sgca_r[hi,lo]stamp: [%d, %d]\n"),
	    indent, spaces, r.gca_rowcount,
	    indent, spaces, r.gca_rhistamp, r.gca_rlostamp);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    STprintf(buf,
	ERx("%.*sgca_cost: %d\n%.*sgca_errd[0,1,4,5]: [%d, %d, %d, %d]\n"),
	    indent, spaces, r.gca_cost,
	    indent, spaces,
		r.gca_errd0, r.gca_errd1, r.gca_errd4, r.gca_errd5);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    STprintf(buf, ERx("%.*sgca_logkey[16]: [0x"), indent, spaces );
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    for (i = 0; i < 16; i++)
    {
	STprintf( buf, ERx("%02x"), (unsigned char) r.gca_logkey[i]);
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    STprintf( buf, ERx("]\n") );
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= sizeof(r);
    *cp += sizeof(r);
} /* gtr_response */



/*{
+*  Name: gtr_gname - Low level trace routine to trace GCA_NAME.
**
**  Description:
**	This routine may trace a GCA_NAME structure or any other fields
**	that consist of a length followed by a fixed length name.
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	ltitle		- Title of length field.  If null use "gca_l_name".
**	ntitle		- Title of name field.  If null use "gca_name".
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
*/

static VOID
gtr_gname(trs, indent, title, ltitle, ntitle, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title, *ltitle, *ntitle;
char		**cp;
i4		*left;
{
    i4		n;			/* Temp for name length */
    char	buf[50];		/* Buffer for display data */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    gtr_nat(trs, indent, ltitle ? ltitle : ERx("gca_l_name"), cp, left, TN, &n);
    gtr_name(trs, indent, ntitle ? ntitle : ERx("gca_name"), cp, left, n);
} /* gtr_gname */


/*{
+*  Name: gtr_gct - Low level trace routine to trace GCA_XA_DIS_TRAN_ID.
**
**  Description:
**      Trace routine for GCA_XA_DIS_TRAN_ID
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Not enough data for tracing object.
**
**  Side Effects:
-*	
**  History:
**	07-oct-1992	- Written for GCA tracing (nandak)
**      22-sep-1993     - Modified to output branch_seqnum and branch_flag:
**                        the two additional members of the extended XA XID
**                        structure.
*/
static VOID
gtr_gct(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    i4	l;			/* Temp to dump from */
    char	buf[512];		/* Buffer for all data */
    i4          i;
    i4          desc_size = (sizeof(i4) + sizeof(i4) + sizeof(i4));
    i4          gtrid_length;           /* length of GTRID in XA's XID */
    i4          bqual_length;           /* length of BQUAL in XA's XID */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    if (*left >= sizeof(GCA_XA_DIS_TRAN_ID)+desc_size)
    {
        /* Skip the descriptor */
        *cp += desc_size;
        *left -= desc_size;
        /* Print the transaction id */
        /* XA transaction */
        gtr_i4(trs, indent, ERx("gca_xa_dis_tran_id.XA.formatID"),
                    cp, left, TN, &l);
        gtr_i4(trs, indent, ERx("gca_xa_dis_tran_id.XA.gtrid_length"),
                    cp, left, TN, &l);
        gtrid_length = l;
        gtr_i4(trs, indent, ERx("gca_xa_dis_tran_id.XA.bqual_length"),
                    cp, left, TN, &l);
        bqual_length = l;
        for(i=0;
            i<(gtrid_length+bqual_length+sizeof(i4)-1);
            i=i+sizeof(i4))
	    gtr_i4(trs, indent, ERx("gca_xa_dis_tran_id.XA.data"),
                        cp, left, TX, &l);
        /* Skip over remaining portion of the structure */
        *left -= (DB_XA_XIDDATASIZE - i);
        *cp   += (DB_XA_XIDDATASIZE - i);
        gtr_nat(trs, indent, ERx("gca_xa_dis_tran_id.XA.branch_seqnum"),
		cp, left, TN, &i);
        gtr_nat(trs, indent, ERx("gca_xa_dis_tran_id.XA.branch_flag"),
		cp, left, TN, &i);
    }
    else
    {
	STprintf(buf, ERx("%.*s-- %s --\n"), indent, spaces, not_enough);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left = 0;
    }
} /* gtr_gct */


/*{
+*  Name: gtr_gcv - Low level trace routine to trace GCA_DATA_VALUE.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Not enough data for tracing object.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	01-mar-1993 (kathryn)
**	    Change dbv to static.  The db_data_value information (type/length)
**	    is at the beginning of the value in the GCA message buffer.
**	    Since values may now span GCA message buffers, the dbv info must
**	    be kept for tracing the remainder of a value if it is in a
**	    following message buffer.
**	    If the current message buffer begins with the remaining data from
**	    a previous value, then just generate the reaminder of the value
**	    and return. This routine will be invoked again to process the
**	    next query value.
*/

static VOID
gtr_gcv(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    GCA_DATA_VALUE	  gcv;			/* Local copy for tracing */
    i4			  gcv_head;		/* For quick copying */
    char		  buf[200];		/* Buffer for display data */
    char		  typename[100];   	/* For mapping type name */

    /* If we are mid-value (value spans messages) header is already printed
    ** and DBV contains the correct information.  Just generate the remainder
    ** of the value and return.  This routine will be called again for the
    ** next value.
    */
    if (trs->gca_flags & GCA_TR_INVAL)
    {
	indent++;
	gtr_value(trs, indent, ERx("gca_value"), cp, left, &trs->gca_dbv);
	return;
    }
    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    gcv_head = sizeof(gcv.gca_type) + sizeof(gcv.gca_precision) +
	       sizeof(gcv.gca_l_value);
    if (*left < gcv_head)
    {
	STprintf(buf, ERx("%.*s-- %s --\n"), indent, spaces, not_enough);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left = 0;
	return;
    }
    /* Trace first part */
    MEcopy((PTR)*cp, (u_i2)gcv_head, (PTR)&gcv);
    STprintf(buf,
	ERx("%.*sgca_type: %s\n"),
	  indent, spaces, gtr_type((i4)gcv.gca_type, typename));
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);

    /*
    ** For DECIMAL, unpack precision to get 'real' precision and scale
    ** and trace like so:
    **		"gca_precision: precision,scale".
    */
    if (abs(gcv.gca_type) == DB_DEC_TYPE)
    {
        STprintf(buf,
            ERx("%.*sgca_precision: %d,%d\n"), indent, spaces,
                   (i4)DB_P_DECODE_MACRO(gcv.gca_precision),
                   (i4)DB_S_DECODE_MACRO(gcv.gca_precision));
    }
    else
    {
        STprintf(buf,
            ERx("%.*sgca_precision: %d\n"), indent, spaces, gcv.gca_precision);
    }
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    STprintf(buf,
	ERx("%.*sgca_l_value: %d\n"), indent, spaces, gcv.gca_l_value);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);

    *left -= gcv_head;
    *cp += gcv_head;
    /* Trace value part */
    trs->gca_dbv.db_data = (PTR)0;
    trs->gca_dbv.db_datatype = gcv.gca_type;
    trs->gca_dbv.db_length = gcv.gca_l_value;
    trs->gca_dbv.db_prec = gcv.gca_precision;
    gtr_value(trs, indent, ERx("gca_value"), cp, left, &trs->gca_dbv);
} /* gtr_gcv */



/*{
+*  Name: gtr_tdesc - Low level trace routine to trace GCA_TD_DATA.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
*/

static VOID
gtr_tdesc(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    i4		l;		/* Temp values & # of columns */
    i4			i;		/* Index for column tracing */
    char		buf[50];	/* Local buffer for display */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    gtr_i4(trs, indent, ERx("gca_tsize"), cp, left, TN, &l);
    gtr_i4(trs, indent, ERx("gca_result_modifier"), cp, left, TX, &l);
    gtr_i4(trs, indent, ERx("gca_id_tdscr"), cp, left, TN, &l);
    gtr_i4(trs, indent, ERx("gca_l_col_desc"), cp, left, TN, &l);
    /* Trace the fixed number of column descriptors */
    for (i = 0; i < l && *left > 0; i++)
    {
	STprintf(buf, ERx("%.*sgca_col_desc[%d]:\n"), indent, spaces, i);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	gtr_att(trs, indent + 1, ERx("gca_attdbv"), cp, left);
    } /* For each column */
} /* gtr_tdesc */



/*{
+*  Name: gtr_cpmap - Low level trace routine to trace GCA_CP_MAP.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	15-aug-1989	- If nullen_cp > 0 read nuldata_cp (barbara)
**	02-aug-1991	- Trace new copy map format. (teresal)
*/

static VOID
gtr_cpmap(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    i4		l, l1;		/* Temps and # of column descriptors */
    i4			n;		/* Temp length */
    i4			i;		/* Column descriptor index */
    char		buf[50];	/* Temp display buffer */
    DB_DATA_VALUE	dbv;		/* For cp_nuldata */
    i4			rowtype;	/* For cp_nuldata */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    gtr_i4(trs, indent, ERx("gca_status_cp"), cp, left, TN, &l);
    gtr_i4(trs, indent, ERx("gca_direction_cp"), cp, left, TN, &l);
    gtr_i4(trs, indent, ERx("gca_maxerrs_cp"), cp, left, TN, &l);
    gtr_gname(trs, indent, (char *)0,
	      ERx("gca_l_fname_cp"), ERx("gca_fname_cp"), cp, left);
    gtr_gname(trs, indent, (char *)0,
	      ERx("gca_l_logname_cp"), ERx("gca_logname_cp"), cp, left);
    gtr_tdesc(trs, indent, ERx("gca_tup_desc_cp"), cp, left);
    gtr_i4(trs, indent, ERx("gca_l_row_desc_cp"), cp, left, TN, &l);
    /* Trace the fixed number of COPY column descriptors */
    for (i = 0; i < l && *left > 0; i++)
    {
	STprintf(buf, ERx("%.*sgca_row_desc_cp[%d]:\n"), indent, spaces, i);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	indent++;
	    gtr_name(trs, indent, ERx("gca_domname_cp"), cp, left, GCA_MAXNAME);
	    gtr_nat(trs, indent, ERx("gca_type_cp"), cp, left, TN, &n);
	    rowtype = n;
	    /* Protocol level 50 copy map added gca_precision_cp */
	    if (trs->gca_protocol >= GCA_PROTOCOL_LEVEL_50)
	    {
		gtr_nat(trs, indent, ERx("gca_precision_cp"), cp, left, TN, &n);
	    }
	    gtr_i4(trs, indent, ERx("gca_length_cp"), cp, left, TN, &l1);
	    gtr_nat(trs, indent, ERx("gca_user_delim_cp"), cp, left, TN, &n);
	    gtr_gname(trs, indent, (char *)0,
		      ERx("gca_l_delim_cp"), ERx("gca_delim_cp"), cp, left);
	    gtr_i4(trs, indent, ERx("gca_tupmap_cp"), cp, left, TN, &l1);
	    gtr_nat(trs, indent, ERx("gca_cvid_cp"), cp, left, TN, &n);
	    /* Protocol level 50 copy map added gca_cvprec_cp */
	    if (trs->gca_protocol >= GCA_PROTOCOL_LEVEL_50)
	    {
		gtr_nat(trs, indent, ERx("gca_cvprec_cp"), cp, left, TN, &n);
	    }
	    gtr_nat(trs, indent, ERx("gca_cvlen_cp"), cp, left, TN, &n);
	    gtr_nat(trs, indent, ERx("gca_withnull_cp"), cp, left, TN, &n);

	    /*
	    ** Check version level here to determine how to trace this:
	    ** Protocol level 50 copy map replaced gca_nuldata_cp
	    ** with gca_nullvalue_cp (a GCA_DATA_VALUE) and eliminated
	    ** gca_nullen_cp.
	    */
	    if (trs->gca_protocol >= GCA_PROTOCOL_LEVEL_50)
	    {
		/* Trace new copy map */

	    	gtr_nat(trs, indent, ERx("gca_l_nullvalue_cp"), cp, left, TN, &n);

		/* gca_nullvalue_cp only sent if gca_l_nullvalue_cp is TRUE */
		if (n)
		{
		    indent++;
	    	    gtr_gcv(trs, indent, ERx("gca_nullvalue_cp"), cp, left);
		    indent--;
		}
	    }
	    else /* Do the old way */
	    {
	    	gtr_nat(trs, indent, ERx("gca_nullen_cp"), cp, left, TN, &n);
	    	if (n > 0)		/* Get null data if there is any */
	    	{
		    dbv.db_length = n;
		    dbv.db_datatype = rowtype;
		    dbv.db_prec = 0;
		    dbv.db_data = NULL;
	 	    gtr_value( trs, indent, ERx("gca_nuldata_cp"), cp, left, &dbv );
	    	}
	    }
	indent--;
    } /* For each column */
} /* gtr_cpmap */



/*{
+*  Name: gtr_att - Low level trace routine to trace a GCA_COL_ATT w/o data.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Not enough data for tracing object.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	21-Dec-2001 (gordy)
**	    Renamed to gtr_att() and updated for changes in GCA_COL_ATT.
*/

static VOID
gtr_att(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    GCA_COL_ATT	dbv;			/* Local for quick copying */
    char	buf[100];		/* Buffer for all display */
    char	typename[50];		/* Type name buffer */

    if (title)
    {
	STprintf(buf, ERx("%.*s%s:\n"), indent++, spaces, title);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    if (*left < sizeof(dbv.gca_attdbv))
    {
	STprintf(buf, ERx("%.*s-- %s --\n"), indent, spaces, not_enough);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left = 0;
	return;
    }
    MEcopy((PTR)*cp, sizeof(dbv.gca_attdbv), (PTR)&dbv.gca_attdbv);
    STprintf(buf,
	ERx("%.*sdb_data: 0x0%p\n%.*sdb_length: %d\n"),
	  indent, spaces, dbv.gca_attdbv.db_data,
	  indent, spaces, dbv.gca_attdbv.db_length);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    STprintf(buf,
	ERx("%.*sdb_datatype: %s\n"),
	  indent, spaces, gtr_type((i4)dbv.gca_attdbv.db_datatype, typename));
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	
    /*
    ** For DECIMAL, unpack precision to get 'real' precision and scale.
    */
    if (abs(dbv.gca_attdbv.db_datatype) == DB_DEC_TYPE)
    {
        STprintf(buf,
            ERx("%.*sdb_prec: %d,%d\n"),
              indent, spaces, (i4)DB_P_DECODE_MACRO(dbv.gca_attdbv.db_prec),
                              (i4)DB_S_DECODE_MACRO(dbv.gca_attdbv.db_prec));
    }
    else
    {
        STprintf(buf,
            ERx("%.*sdb_prec: %d\n"),
              indent, spaces, (i4)dbv.gca_attdbv.db_prec);
    }
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= sizeof(dbv.gca_attdbv);
    *cp += sizeof(dbv.gca_attdbv);
    gtr_gname(trs, --indent, (char *)0,
	      ERx("gca_l_attname"), ERx("gca_attname"), cp, left);
} /* gtr_att */


/*{
+*  Name: gtr_i4 - Low level trace routine to trace a i4.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title - must NOT be null.
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**	hex		- Print data as hex?
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**	l		- Value of i4.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	05-oct-1988	- Added option for printing hex (ncg)
*/

static VOID
gtr_i4(trs, indent, title, cp, left, hex, l)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
i4		hex;
i4		*l;
{
    char	buf[100];		/* Buffer for display */

    *l = 0;
    MEcopy((PTR)*cp, (u_i2)sizeof(*l), (PTR)l);
    if (hex)
	STprintf(buf, ERx("%.*s%s: 0x0%x\n"), indent, spaces, title, *l);
    else
	STprintf(buf, ERx("%.*s%s: %d\n"), indent, spaces, title, *l);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= sizeof(*l);
    *cp += sizeof(*l);
} /* gtr_i4 */

/*{
+*  Name: gtr_sqlstate - Low level trace routine to trace an sqlstate.
**
**  Description:
**	SQLSTATES are char[5]'s but are sent over GCC as an i4 (because
**	of GCC issues).  This function calls the common!cuf routine that
*	decodes	the i4 back into a char[5] for printing to the log, but only
**	advances the cp pointer 4 bytes.
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title - must NOT be null.
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	03-nov-1992	- Written for GCA tracing (larrym)
*/

static VOID
gtr_sqlstate(trs, indent, title, cp, left)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
{
    char	buf[100];			/* Buffer for display */
    char	sqlst[DB_SQLSTATE_STRING_LEN];	/* buf for SQLSTATE */
    i4	l;				/* encoded SQLSTATE */

    MEcopy((PTR)*cp, (u_i2)sizeof(l), (PTR)&l);
    ss_decode(sqlst,l);	
    STprintf(buf, ERx("%.*s%s: %.*s\n"), indent, spaces, title,
	(i4)DB_SQLSTATE_STRING_LEN, sqlst);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= sizeof(l);
    *cp += sizeof(l);
} /* gtr_sqlstate */


/*{
+*  Name: gtr_nat - Low level trace routine to trace a nat.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title - must NOT be null.
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**	hex		- Print data as hex?
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**	n		- Value of nat.
**
**	Returns:
**	    	None
**	Errors:
**		None
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	05-oct-1988	- Added option for printing hex (ncg)
*/

static VOID
gtr_nat(trs, indent, title, cp, left, hex, n)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
i4		hex;
i4		*n;
{
    char	buf[100];		/* Buffer for display */

    *n = 0;
    MEcopy((PTR)*cp, (u_i2)sizeof(*n), (PTR)n);
    if (hex)
	STprintf(buf, ERx("%.*s%s: 0x0%x\n"), indent, spaces, title, *n);
    else
	STprintf(buf, ERx("%.*s%s: %d\n"), indent, spaces, title, *n);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= sizeof(*n);
    *cp += sizeof(*n);
} /* gtr_nat */



/*{
+*  Name: gtr_name - Low level trace routine to trace fixed length name.
**
**  Description:
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**	slen		- Length of name.  Must be smaller than the
**			  size of the local display buffer.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Invalid name length.
**		Not enough data for tracing name.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
*/

static VOID
gtr_name(trs, indent, title, cp, left, slen)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
i4		slen;
{
    char	buf[200];		/* Buffer for name display */
    char	name[100];
    char	*src, *dst;		/* Copy pointers */
    i4		i;			/* Char counter */

    STprintf(buf, ERx("%.*s%s: "), indent, spaces, title);
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);

    if (slen > sizeof(buf) - 2 || slen < 0)
    {
	STprintf(buf, ERx("invalid or large name length - %d\n"), slen);
    }
    else if (*left < slen)
    {
	STprintf(buf, ERx("%s - %d\n"), not_enough, *left);
    }
    else
    {
	/* Walk through the name copying chars and mapping control chars */
	dst = name;
	src = *cp;
	for (i = 0; i < slen && *src != EOS;)
	{
	    CMbyteinc(i, src);
	    if (CMprint(src))
		CMcpyinc(src, dst);
	    else
	    {
		*dst++ = '.';
		CMnext(src);
	    }
	}
	/* Trim as much as possible - names may be padded to GCA_MAXNAME */
	*dst = EOS;
	STtrmwhite(name);
	STprintf(buf, ERx("(%d) %s\n"), i, name);
    }
    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    *left -= slen;
    *cp += slen;
} /* gtr_name */


/*{
+*  Name: gtr_type - Trace type name mapping routine.
**
**  Description:
**
**  Inputs:
**	type		- Type id.
**
**  Outputs:
**	typename	- Type name mapped as:
**				type id : [nullable] type name
**
**	Returns:
**	    	Pointer to typename.
**	Errors:
**		If invalid type sets typename to DB_unknown_TYPE.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	19-oct-1992 (kathryn)
**	    Added DB_LVCH_TYPE and DB_LBIT_TYPE as valid types for new
**	    large object support.
**	26-jul-1993 (kathryn)
**	    Added DB_{BYTE|VBYTE|LBYTE}.
**	09-mar-2001 (abbjo03)
**	    Add DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/

static char *
gtr_type(type, typename)
i4	type;
char	*typename;
{
    char	*tn;

    STprintf(typename, ERx("%d : "), type);
    if (type < 0)
	STcat(typename, ERx("nullable "));
    switch (abs(type))
    {
      case DB_BOO_TYPE:
        tn = "DB_BOO_TYPE";
        break;
      case DB_INT_TYPE:
	tn = ERx("DB_INT_TYPE");
        break;
      case DB_FLT_TYPE:
	tn = ERx("DB_FLT_TYPE");
        break;
      case DB_CHR_TYPE:
	tn = ERx("DB_CHR_TYPE");
        break;
      case DB_TXT_TYPE:
	tn = ERx("DB_TXT_TYPE");
        break;
      case DB_MNY_TYPE:
	tn = ERx("DB_MNY_TYPE");
        break;
      case DB_DTE_TYPE:
	tn = ERx("DB_DTE_TYPE");
        break;
      case DB_ADTE_TYPE:
	tn = ERx("DB_ADTE_TYPE");
        break;
      case DB_TMWO_TYPE:
	tn = ERx("DB_TMWO_TYPE");
        break;
      case DB_TMW_TYPE:
	tn = ERx("DB_TMW_TYPE");
        break;
      case DB_TME_TYPE:
	tn = ERx("DB_TME_TYPE");
        break;
      case DB_TSWO_TYPE:
	tn = ERx("DB_TSWO_TYPE");
        break;
      case DB_TSW_TYPE:
	tn = ERx("DB_TSW_TYPE");
        break;
      case DB_TSTMP_TYPE:
	tn = ERx("DB_TSTMP_TYPE");
        break;
      case DB_INYM_TYPE:
	tn = ERx("DB_INYM_TYPE");
        break;
      case DB_INDS_TYPE:
	tn = ERx("DB_INDS_TYPE");
        break;
      case DB_CHA_TYPE:
	tn = ERx("DB_CHA_TYPE");
        break;
      case DB_VCH_TYPE:
	tn = ERx("DB_VCH_TYPE");
        break;
      case DB_NCHR_TYPE:
	tn = ERx("DB_NCHR_TYPE");
        break;
      case DB_NVCHR_TYPE:
	tn = ERx("DB_NVCHR_TYPE");
        break;
      case DB_DEC_TYPE:
	tn = ERx("DB_DEC_TYPE");
        break;
      case DB_LTXT_TYPE:
	tn = ERx("DB_LTXT_TYPE");
        break;
      case DB_DMY_TYPE:
	tn = ERx("DB_DMY_TYPE");
        break;
      case DB_DBV_TYPE:
	tn = ERx("DB_DBV_TYPE");
        break;
      case DB_QTXT_TYPE:
	tn = ERx("DB_QTXT_TYPE");
        break;
      case DB_LBIT_TYPE:
        tn = ERx("DB_LBIT_TYPE");
        break;
      case DB_BYTE_TYPE:
	tn = ERx("DB_BYTE_TYPE");
	break;
      case DB_VBYTE_TYPE:
	tn = ERx("DB_VBYTE_TYPE");
	break;
      case DB_LBYTE_TYPE:
	tn = ERx("DB_LBYTE_TYPE");
	break;
      case DB_GEOM_TYPE:
	tn = ERx("DB_GEOM_TYPE");
	break;
      case DB_POINT_TYPE:
	tn = ERx("DB_POINT_TYPE");
	break;
      case DB_MPOINT_TYPE:
	tn = ERx("DB_MPOINT_TYPE");
	break;
      case DB_LINE_TYPE:
	tn = ERx("DB_LINE_TYPE");
	break;
      case DB_MLINE_TYPE:
	tn = ERx("DB_MLINE_TYPE");
	break;
      case DB_POLY_TYPE:
	tn = ERx("DB_POLY_TYPE");
	break;
      case DB_MPOLY_TYPE:
	tn = ERx("DB_MPOLY_TYPE");
	break;
      case DB_GEOMC_TYPE:
	tn = ERx("DB_GEOMC_TYPE");
	break;
      case DB_LVCH_TYPE:
	tn = ERx("DB_LVCH_TYPE");
	break;
      default:
	tn = ERx("DB_unknown_TYPE");
	break;
    }
    STcat(typename, tn);
    return typename;
}

/*{
** Name:  gtr_charval - Trace character data values.
**
** Description:
**	This routine traces character datatype values. Given a descriptor
**	to the next data value, this routine traces the number of bytes
**	specified by the descriptor length. If the number of bytes left in
**	the message buffer is less than the length of the data value to be
**	traced, then this value spans message buffers. The number of bytes in
**	this message will be traced and the reamining bytes will be in a
**	following message.
**
** Inputs:
**	trs	- Structure containing trace state.
**	indent	- Number of spaces to indent trace line.
**	titlen	- Length of title preceding value.
**	datlen	- Length of value to trace.
**	cp	- Current position in gca message buffer.
**	left	- Number of bytes left in GCA message buffer.
**	dbv	- The current value being traced.
**
** Outputs:
**	cp      - Updated to next spot in message buffer.
**	left    - Decreased by number of bytes traced.
**
** Returns:
**	None.
**
** Side Effects:
**      Sets trace flag GCA_TR_INVAL when data value length is greater than
**	  the number of bytes left in message.
**	Query data value is written to the printgca log file.
**
** History:
**	10-mar-1993 (kathryn)
**	    Written.
**      18-aug-1993 (kathryn)
**          Correct "left" calculation for varying length datatypes.
**      10-nov-1994 (athda01)
**          Correct increment of pointer *cp and decrement of *left
**          and dbv->db_length to correctly position at end of text
**          field within the structure for subsequent entries, instead
**          of merely at the end of the variable length text data.
**
*/
static void
gtr_charval(GCA_TR_STATE *trs, i4  indent, i4  titlen, i4  datlen, char **cp,
	    i4  *left, DB_DATA_VALUE *dbv)
{

    i4          line_len;
    char        *src, *dst;		
    char        buf[VAL_LINE_MAX +3];   /* Kanji + \n + EOS */
    DB_DT_ID    type;			/* Data Type */
    i2          len;		   	/* number of bytes in string value */
    i2		tr_len;			/* actual bytes traced */

    type = abs(dbv->db_datatype);

    /*
    ** Strings need to be processed as streams of bytes.  Varying length
    ** strings require a count field traced, while all strings may
    ** require line truncation.
    */
    if ( (type == DB_VCH_TYPE || type == DB_TXT_TYPE
	 || type == DB_LTXT_TYPE || type == DB_VBYTE_TYPE)
      && !(trs->gca_flags & GCA_TR_INVAL))
    {
            /* Read the varying-length count and mark it */
            MEcopy((PTR)*cp, (u_i2)sizeof(len), (PTR)&len);
            src = *cp + sizeof(len);
            STprintf(buf, ERx("(%d) "), len);
	    dst = buf + STlength(buf);
            line_len = trs->gca_line_length - titlen - STlength(buf) - 2;
	    tr_len = (i2)sizeof(len);
    }
    else
    {
	    src = *cp;
	    dst = buf;
            line_len = trs->gca_line_length - titlen - 2;
	    len = datlen;
	    tr_len = 0;
    }

    /* The value is longer than the number of bytes in the gca message
    ** buffer. This value will be continued in the next buffer so set
    ** flag indicating that the next buffer begins in mid data value
    */
    if (len > (*left - tr_len))
    {
        trs->gca_flags |= GCA_TR_INVAL;
        len = *left - tr_len;
    }
    tr_len += len;

    while (len > 0)
    {
        /*
        ** Dump all characters (map control characters)
        ** Do not try to map some common control characters like new-line
        ** and tab.  This only extends tracing output.
        */
        while (line_len > 0 && len > 0)
        {
            CMbytedec(len, src);
            if (CMprint(src))
            {
                CMbytedec(line_len, src);
                CMcpyinc(src, dst);
            }
            else
            {
                CMnext(src);
                *dst++ = '.';
                line_len--;
            }
        }
        *dst++ = '\n';
        *dst = EOS;
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
        /* If there more data then indent to edge of title */
        if (len > 0)
        {
            (*trs->gca_emit_exit)(GCA_TR_WRITE, titlen, spaces);
            dst = buf;
            line_len = trs->gca_line_length - titlen - 2;
         }
    }
    /*If end of nullable value skip null indicator byte */           /*athda01*/
    /*if ((dbv->db_datatype < 0) && !(trs->gca_flags & GCA_TR_INVAL)) *athda01*/
    /*   tr_len += 1; */                                             /*athda01*/

    /*If all text successfully moved to message buffer, then update*//*athda01*/
    /*pointers and values by full length of text field regardless  *//*athda01*/
    /*of variable length.                                          *//*athda01*/

    if (!(trs->gca_flags & GCA_TR_INVAL))                            /*athda01*/
    {                                                                /*athda01*/
    /* update buffer pointer and # of bytes left to end of current *//*athda01*/
    /* text field.  All text now processed.                        *//*athda01*/
       *cp += dbv->db_length;                                        /*athda01*/
       *left -= dbv->db_length;                                      /*athda01*/
       dbv->db_length = 0;                                           /*athda01*/
    }                                                                /*athda01*/
    else                                                             /*athda01*/
    {                                                                /*athda01*/

    /* Update buffer pointer and number of bytes left */
       *cp += tr_len;
       *left -= tr_len;
       dbv->db_length -= tr_len;
    }                                                                /*athda01*/
}


/*{
** Name:  gtr_longval - Trace a Large Object.
**
** Description:
** 	This routine traces a large object data value when it is transmitted to
**	the DBMS.  Large objects are transmitted to the DBMS as an
**	ADP_PERIPHERAL_HDR  -- followed by any number of segments that consist
**	of a more indicator, the data length and the data -- followed by a
**	no_more indicator and the null_byte_indicator:
**	 	ADP_PERIPHERAL_HDR	
**	    	more, len, data, more, len, data .....
**	        nomore, null_byte_indicator.
**	Only by reading the "no_more" indicator do we know that we have
**	finished the value.  The next data value immediately follows it in
**	the same message buffer.
**
**	Large object values may span several physical gca messages, and
**	messages are traced as each message is sent to the DBMS. Only the
**      ADP_PERIPHERAL_HDR is guaranteed to be in one message. The large
**	object may be split anywhere from one message to another.
**	For example the more indicator may be in one message, but the
**	len value would not fit, so is in a following message.
**	The trace flag GCA_TR_INVAL specifies that the last message ended
**	in the middle of a value and the next message will contain the
**	remaining  data. Variables trs->gca_seg_len and trs->gca_seg_more
**	indicate what the next item in the message buffer is. In the
**	
**	Trace output for Large objects will consist of the same
**	header information as other data values, with the exception that
**	the gca_l_value will contain the length of the ADP_PERIPHERAL header,
**	rather than the entire length of the object.  Large objects are
**	transmitted a segment at a time, so the entire length is not known.
**
**	    Example Header for Long Varchar:
**	        gca_qdata[i]:
**	         gca_type: 22 : DB_LVCH_TYPE
**	         gca_precision: 0
**	         gca_l_value: 12
**	         gca_value:
**
**	TRACE OUTPUT:
**	 The gca_value will consist of the length of each segment transmitted
**	 to the DBMS. For Example, if a large object is transmitted as 5
**	 segments of 500 chars each then the trace output would be:
**   	     gca_value:
** 		segment[1]: 500
** 		segment[2]: 500
** 		segment[3]: 500
** 		segment[4]: 500
** 		segment[5]: 500
**
**	DEBUGGING:
**	  The boolean trace_val is available for Debugging.  Setting this
**        boolean to TRUE will cause the segment data to be traced as well.
**	
** Inputs:
**	trs	- Structure containing trace state.
**	indent	- Number of spaces to indent trace line.
**	cp	- Current position in gca message buffer.
**	left	- Number of bytes left in GCA message buffer.
**	dbv	- The current value being traced.
**
** Outputs:
**	cp	- Updated to next spot in message buffer.
**	left	- Decreased by number of bytes traced.
**
** Returns:
**	None.
**
** Side Effects:
**	Query data value is written to the printgca log file.
**
** History:
**	10-mar-1993 (kathryn)
**	    Written.
**	01-apr-1993 (kathryn)
**	    If this is the first segment then set readmore to TRUE.
**	    Fix coding error.
**	15-sep-1993 (kathryn)
**	    Ensure proper tracing when more-indicator and length are
**	    split across two GCA messages.
**	01-oct-1993 (kathryn)
**	    Check for error where no dataend is specified.
*/
static void
gtr_longval(GCA_TR_STATE *trs, i4  indent, char **cp, i4  *left,
	    DB_DATA_VALUE *dbv)
{
              i4      more;		 /* are there more segments to trace */
	      i2      len;	         /* len of segment to trace */
              char    titbuf[30];	 /* buffer containing trace title */
	      i4      tr_len;	         /* no of bytes traced so far */
	      bool    trace_val = FALSE; /* When TRUE - segment value traced */


    /* First time through for large object - skip ADP_PERIPHERAL header */

    if (!(trs->gca_flags & GCA_TR_INVAL))
    {
        trs->gca_flags |= GCA_TR_INVAL;
        *cp = *cp + ADP_HDR_SIZE;
        *left -= ADP_HDR_SIZE;
        dbv->db_length -= ADP_HDR_SIZE;
        trs->gca_seg_no = 1;
	trs->gca_seg_more = TRUE;
	trs->gca_seg_len = TRUE;
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("\n"));
    }
    else if (dbv->db_length > 0 && trace_val)
	(*trs->gca_emit_exit)(GCA_TR_WRITE, indent+4, spaces);

    /* There are more bytes in the current message buffer */
    while (*left)
    {
	if ((dbv->db_length <= 0) && !trs->gca_seg_more && !trs->gca_seg_len)
	{
	    trs->gca_flags &= ~GCA_TR_INVAL;
	    return;
	}

	/* We are in the middle of a segment value */
        if (dbv->db_length > 0)
        {
	    /* Trace or Skip the actual Segment Value */
	    if (trace_val)
                gtr_charval(trs, indent, indent+4, dbv->db_length, cp,
			    left, dbv);
	    else
	    {
		tr_len = (dbv->db_length > *left) ? *left : dbv->db_length;
		dbv->db_length = (dbv->db_length > *left) ?
			(dbv->db_length -= *left) : 0;
		*cp += tr_len;
		*left -= tr_len;
	    }
	    /* Did we complete the data for that segment? */
            if (dbv->db_length > 0)
            {
                trs->gca_seg_more = FALSE;
                trs->gca_seg_len = FALSE;
            }
            else
            {
                trs->gca_seg_more = TRUE;
                trs->gca_seg_no++;
            }
        }

	/* Next item in the message buffer is the more indicator */
        if (trs->gca_seg_more && (*left >= (i4)sizeof(i4)))
        {

            MEcopy((PTR)*cp, (u_i2)sizeof(i4),(PTR)&more);
            *cp += sizeof(i4);
            *left -= sizeof(i4);
            trs->gca_seg_len = TRUE;
	    trs->gca_seg_more = FALSE;

            /* There are no more segments so this large object value is
	    ** complete
	    */
            if (!more)
            {
                trs->gca_flags &= ~GCA_TR_INVAL;
                trs->gca_seg_len = FALSE;
                dbv->db_length = 0;

                /* Dispose of NULL indicator byte */
                if (dbv->db_datatype < 0)
                {
                    (*cp)++;
                    (*left)--;
                }
                break;
            }
        }

	/* Next item in the message buffer is the len of data segment */
        if (trs->gca_seg_len && (*left >= (i4)sizeof(i2)))
        {

            /* Get the length of the next segment */
	    MEcopy((PTR)*cp, (u_i2)sizeof(i2),(PTR)&len);
            dbv->db_length = len;
            *cp += sizeof(i2);
            *left -= sizeof(i2);

            /* Is the entire segment in this message? */
            STprintf(titbuf, ERx("%.*ssegment[%d]: %d\n"),
                indent + 1, spaces, trs->gca_seg_no, dbv->db_length);
	    (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, titbuf);
	    if (trace_val)
	        (*trs->gca_emit_exit)(GCA_TR_WRITE, indent+4, spaces);
	    trs->gca_seg_len = FALSE;
        }
    }
    return;
}



/*{
+*  Name: gtr_value - Low level trace routine to trace GCA values.
**
**  Description:
**	This routine, when given a descriptor to the next data value traces
**	the stream of bytes as specified by the descriptor.
**	
**	Long strings are traced over lines with an attempt at indentation.
**
**  Inputs:
**	trs		- Internal trace state.
**	indent		- Indentation level.
**	title		- Title (may be null if outermost level).
**	cp		- Start of data area.
**	left		- Number of bytes left in message.
**	dbv		- DB data value describing the next data value.
**			- All fields are set except that:
**	  .db_data	- Ignored as the data is pointed at by cp.
**
**  Outputs:
**	cp		- Updated to next spot in message buffer.
**	left		- Decreased by number of bytes traced.
**
**	Returns:
**	    	None
**	Errors:
**		Not enough data for tracing data value.
**		Data type not traced.
**		Invalid type ids and lengths.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	01-mar-1993    (kathryn)
**	    Replaced functionality for tracing character datatype values with
**	    calls to new functions; "gtr_longval" to trace large objects,
**	    and "gtr_charval" to trace other character datatypes.
**	26-jul-1993 (kathryn)
**	    Add tracing info for BYTE/VBYTE/LBYTE datatypes.
**	09-mar-2001 (abbjo03)
**	    Add minimal support for DB_NCHR_TYPE and DB_NVCHR_TYPE.
**	15-Jul-2004 (schka24)
**	    Add i8 type.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
*/

static VOID
gtr_value(trs, indent, title, cp, left, dbv)
GCA_TR_STATE	*trs;
i4		indent;
char		*title;
char		**cp;
i4		*left;
DB_DATA_VALUE	*dbv;
{

    f8		numbuf[10];			/* Aligned numeric buffer */
    char	titbuf[100];			/* Title buffer */
    i4		titlen;				/* Length of title */
    char	buf[VAL_LINE_MAX +3];		/* Kanji + \n + EOS */
    char	*src, *dst;			/* For copying */
    i4		type;				/* Local type and length */
    i2		len;
    i4		line_len;			/* For string dumping */
    i4		prec, scal, nchars;		/* For decimal to ascii conv */

    STprintf(titbuf, ERx("%.*s%s: "), indent, spaces, title);
    /* if we are mid value - don't re-write title */
    if (!(trs->gca_flags &  GCA_TR_INVAL))
    {
        (*trs->gca_emit_exit)(GCA_TR_WRITE, 0, titbuf);
    }
    if (dbv->db_length < 0)			/* Valid length ? */
    {
	STprintf(buf, ERx("invalid length = %ld!\n"), dbv->db_length);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left = 0;
	return;
    }

    /* Check for NULL values */
    if (   (dbv->db_datatype < 0 )
        && (dbv->db_datatype != -DB_LVCH_TYPE)
	&& (dbv->db_datatype != -DB_LBYTE_TYPE)
	&& (VAL_NUL_MACRO(*cp, dbv->db_length))
       )
    {
	STcopy(ERx("NULL value skipped\n"), buf);
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
	*left -= dbv->db_length;
	*cp += dbv->db_length;
	return;
    }

    type = abs(dbv->db_datatype);
    len = dbv->db_length;

    if (type == DB_INT_TYPE || type == DB_FLT_TYPE || type == DB_MNY_TYPE
	|| type == DB_DEC_TYPE)
    {
	if (dbv->db_datatype < 0)
            len -= 1;

	/* Numerics can be copied and cast with truncation */
	dst = (char *)numbuf;
	MEcopy((PTR)*cp, (u_i2)len, (PTR)dst);
	switch (type)
	{
	  case DB_INT_TYPE:
	    switch (len)
	    {
	      case 1: STprintf(buf, ERx("%d\n"),     (i4)(*(i1 *)dst)); break;
	      case 2: STprintf(buf, ERx("%d\n"),     (i4)(*(i2 *)dst)); break;
	      case 4: STprintf(buf, ERx("%d\n"),(i4)(*(i4 *)dst)); break;
	      case 8: STprintf(buf, ERx("%lld\n"),(i8)(*(i8 *)dst)); break;
	      default: STprintf(buf, ERx("intlen = %d!\n"), (i4)len); break;
	    }
	    break;
	  case DB_FLT_TYPE:
	    switch (len)
	    {
	      case 4: STprintf(buf, ERx("%g\n"), *(f4 *)dst, '.'); break;
	      case 8: STprintf(buf, ERx("%g\n"), *(f8 *)dst, '.'); break;
	      default: STprintf(buf, ERx("fltlen = %d!\n"), (i4)len); break;
	    }
	    break;
	  case DB_MNY_TYPE:
	    STprintf(buf, ERx("%g\n"), *(f8 *)dst, '.');
	    break;
	  case DB_DEC_TYPE:
            prec = DB_P_DECODE_MACRO(dbv->db_prec);
            scal = DB_S_DECODE_MACRO(dbv->db_prec);
            if (CVpka((PTR)dst, prec, scal, '.', sizeof(buf) - 2, scal,
                CV_PKLEFTJUST, buf, &nchars) != OK)
                STcopy(ERx("DECIMAL not traceable\n"), buf);
            else
                STcat(buf, ERx("\n"));
            break;
	} /* switch numeric types */
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, buf);
    }
    else if (   type == DB_CHA_TYPE   || type == DB_VCH_TYPE
	     || type == DB_QTXT_TYPE  || type == DB_CHR_TYPE
	     || type == DB_TXT_TYPE   || type == DB_LTXT_TYPE
	     || type == DB_BYTE_TYPE  || type == DB_VBYTE_TYPE
             || type == DB_GEOM_TYPE  || type == DB_POINT_TYPE
             || type == DB_MPOINT_TYPE || type == DB_LINE_TYPE
             || type == DB_MLINE_TYPE || type == DB_POLY_TYPE
             || type == DB_MPOLY_TYPE || type == DB_GEOMC_TYPE
	     || type == DB_LBYTE_TYPE || type == DB_LVCH_TYPE)
    {
	/* Make sure that client line length will not overflow local buffers */
	if (trs->gca_line_length > VAL_LINE_MAX)
	    trs->gca_line_length = VAL_LINE_MAX;

	/* Track title length for possible indentation */
	titlen = STlength(titbuf);
	if (type == DB_LVCH_TYPE   || type == DB_LBYTE_TYPE || 
            type == DB_GEOM_TYPE   || type == DB_POINT_TYPE ||
            type == DB_MPOINT_TYPE || type == DB_LINE_TYPE  ||
            type == DB_MLINE_TYPE  || type == DB_POLY_TYPE  ||
            type == DB_MPOLY_TYPE  || type == DB_GEOMC_TYPE )
                _VOID_ gtr_longval(trs, indent, cp, left, dbv);
        else
        {
            if (len <= *left && dbv->db_datatype < 0)
                len -= 1;
            _VOID_ gtr_charval(trs, indent, titlen, len, cp, left, dbv);
            if (dbv->db_length == 0) /* we have completed this value */
                trs->gca_flags &= ~GCA_TR_INVAL;
        }
        return;
    }
    else if ((type == DB_DTE_TYPE) ||
    	     (type == DB_ADTE_TYPE) ||
    	     (type == DB_TMWO_TYPE) ||
    	     (type == DB_TMW_TYPE) ||
    	     (type == DB_TME_TYPE) ||
    	     (type == DB_TSWO_TYPE) ||
    	     (type == DB_TSW_TYPE) ||
    	     (type == DB_TSTMP_TYPE) ||
    	     (type == DB_INYM_TYPE) ||
    	     (type == DB_INDS_TYPE)
	    )
    {
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0, ERx("DATE type not yet traced\n"));
    }
    else if (type == DB_NCHR_TYPE || type == DB_NVCHR_TYPE)
    {
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0,
	    ERx("NCHAR/NVARCHAR not yet traced\n"));
    }
    else
    {
	(*trs->gca_emit_exit)(GCA_TR_WRITE, 0,ERx("Unknown data not traced\n"));
    }
    trs->gca_flags &= ~GCA_TR_INVAL;
    *left -= dbv->db_length;
    *cp += dbv->db_length;
} /* gtr_gcv */

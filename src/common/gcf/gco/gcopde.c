/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <me.h>
# include <qu.h>

# include <dbms.h>
# include <gca.h>
# include <gccer.h>
# include <gcocomp.h>
# include <gcoint.h>
# include <gcxdebug.h>


/**
** 
** Name: GCOPDE.C - perform data encoding/decoding
**
** Description:
**
**	gco_decode() - decode a message for GCA_RECEIVE
**	gco_encode() - encode a message for GCA_SEND
**
**	These routines turn C structures containing message data into 
**	byte stream messages and back again.
**
**	The C structures are defined in GCD.H as variants of the 
**	original message definitions in GCA.H.  Because of the "variable 
**	length" components of the original structures, the original 
**	structures cannot actually be put to use in C programs.
**
**	These routines use the same compiled message descriptors that
**	gco_convert() does.  As such, they are somewhat tied into
**	the message compilation routines in gcocomp.c, because both tuple
**	data and GCA_DATA_VALUE's require on-the-fly compilation.  Regular
**	GCA messages are compiled in advance when gcocomp.c is initialized.
**
**	The compiled message descriptors contain special hints for 
**	gco_encode() and gco_decode() to know about the sizes of 
**	variable length components.
**
** History:
**	17-Mar-94 (seiwald)
**	    Written.
**	29-Mar-94 (seiwald)
**	    Added support for VARIMPAR in message encoding.
**	1-Apr-94 (seiwald)
**	    More comments, more meaningful error messages.
**	4-Apr-94 (seiwald)
**	    Spruce up tracing with OP_DEBUG_TYPE and OP_DEBUG_BUFS.
**	4-Apr-94 (seiwald)
**	    New OP_DV_BEGIN/END.
**	4-Apr-94 (seiwald)
**	    OP_MSG_BEGIN now includes the message type for tracing.
**	5-Apr-94 (seiwald)
**	    GCA_VARIMPAR tracing.
**	5-Apr-94 (seiwald)
**	    Allow for continuation of message after VARIMPAR subelement.
**	11-Apr-94 (seiwald)
**	    Allow for continuation of messages in perform_decoding().
**	24-apr-94 (seiwald)
**	    Use the new GCC_DOC->state flag to communicate a little
**	    more affirmatively about the results of perform_encoding
**	    and perform_decoding().
**	24-Apr-94 (seiwald)
**	    Perform_decoding() now recognises the instruction OP_BREAK
**	    to indicate the end of a row of tuple data: it returns to
**	    its caller with the current row of output.
**	04-May-94 (cmorris)
**	    Bump level at which plethora of trace output appears to 3.
**	06-Oct-94 (gordy)
**	    Use MECOPY macros for possibly unaligned transfers.
**	 9-Nov-94 (gordy)
**	    Added OP_MSG_RESTART for start of next tuple.
**	 1-May-95 (gordy)
**	    Make sure OP_VAR_BEGIN data is properly aligned when allocated
**	    from the user area (dst3 of perform_decoding()).
**	20-Jun-95 (gordy)
**	    Added missing OP_MSG_RESTART case for perform_encoding().
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Fixed processing of large messages which span buffers.
**	    Added OP_MARK, OP_REWIND, OP_JNOVR to detect output buffer
**	    overflow in gcd_decode() and reset the processing state
**	    so that only a complete C data structure is returned.  GCD
**	    messages structures were changed such that all variable
**	    length message end with VARIMPAR arrays which can easily
**	    be used as the split point when buffer overlow occurs.
**	 3-Apr-96 (gordy)
**	    Added casts to tracing.  Removed OP_JNOVR and added OP_CHKLEN
**	    and OP_DV_ADJUST.  OP_VAR_BEGIN and OP_DV_BEGIN once again
**	    can interrupt processing, but do so by issuing a OP_REWIND.
**	    Since BLOB segments in GCA_DATA_VALUES and do an OP_BREAK,
**	    need to carfully chack the psp stack during subsequent
**	    handling because traling segments are placed at the top
**	    of the buffer rather than indirectly in the buffer through
**	    a pointer as is done for all other GCA_DATA_VALUES.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG for messages which are not formatted into 
**	    elements.  The entire message is copied from input to output.
**	    We no longer process tuples as individual elements, so CALLTUP
**	    now uses the global program for tuple messages rather than
**	    the DOC program for the current tuples.
**	 5-Feb-97 (gordy)
**	    Added OP_CALL_DDT for pre-compiled DBMS datatype processing.
**	    OP_PUSH_VCH is now passed both the max and actual data lengths
**	    on the data stack.  OP_COPY_MSG now expects the element size
**	    as an argument.  Removed OP_CHKLEN, OP_COMP_PAD.
**	 7-Sep-97 (gordy)
**	    Skip double byte op-codes which are not relevant here.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       1-Mar-01 (gordy)
**          OP_PUSH_VCH now takes argument which is the size of the array
**          elements (to support Unicode).  New OP_DIVIDE operation.
**      21-June-2001 (wansh01) 
**          Changed gcxlevel to gcd_trace_level to seperate the gcc and gcd trace
**	22-Mar-02 (gordy)
**	    More nat to i4 cleanup.  Removed unneeded atomic types.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Renamed DOC->last_seg to eod to match message flag.
**	    DOC_DONE now distinct from DOC_IDLE for HET/NET processing.
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Renamed gcx_gcc_ops to gcx_gco_ops, as this referenced outside
**          of GCC.
**	30-Nov-05 (gordy)
**	    Fixed global declarations.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	22-Jan-10 (gordy)
**	    Renamed dst3 to rsrv to separate from external interface.
**	    Added operations for multi-byte charset processing.
**	 2-Mar-10 (gordy)
**	    Restore some code lost in preceding change.
*/

# define VEC_MACRO( vec, limit, sz ) \
	if ( ((limit) / (sz)) < (vec) ) { (vec) = (limit) / (sz); }

GLOBALREF GCXLIST gcx_gco_ops[];
GLOBALREF GCXLIST gcx_gcc_eop[];

/*
** Mini-program used to fake an OP_REWIND operation during
** CV_* operations which run out of input.
*/
static GCO_OP	op_rewind[2] = { OP_REWIND, OP_DONE };


/*
** Name: gco_decode() - decode a message for GCA_RECEIVE
**
** Description:
**
**	This function takes a byte stream containing a GCA message and
**	turns it into a C structure representing the message.  In doing
**	so gco_decode() used message descriptors compiled by gcocomp.c.
**	These compiled message descriptors contain instructions on how to
**	decode messages, and gco_decode() interprets these instructions.
**
**	Decoding messages involves five special cases:
**
**	1) Most elements are just copied from the message buffer to the
**	C structure, using alignment-safe copying.  These are carried out
**	by the OP_CV instructions.
**
**	2) Structure pads are skipped in the output as necessary.  This
**	is done by the OP_PAD_ALIGN instruction.
**
**	3) Substructures with array length of VARIMPAR are of a variable
**	count, which is determined by the input source length.  The 
**	OP_IMP_BEGIN instruction just makes sure that there is space
**	available at the end of the current output structure for an
**	entire substructure.
**
**	4) Substructures with array length of VAREXPAR are represented 
**	in the C structure by a pointer to the actual substructure array.
**	The OP_VAR_BEGIN instruction pushes the current source pointer
**	and uses the unused portion of the output buffer for the array.
**	When the substructure has been converted, the OP_VAR_END pops
**	the original source pointer.
**
**	5) GCA_DATA_VALUES are three numbers (type, precision, length)
**	followd by a VAREXPAR data value, buth that VAREXPAR value is
**	treated as a single substructure of array length 1.
**	
**	The use of src1, src2, dst1, dst2, rsrv, eod and state need
**	some explanation:
**
**	    At the start of a new message, src1 and src2 should 
**	    point to the start and end of the input buffer, dst1 
**	    and dst2 should point to the start and end of the 
**	    output buffer and state should be DOC_IDLE.  Set eod
**	    TRUE only if the last piece of the message is in the
**	    input buffer.  The caller should not set rsrv.
**
**	    Space for the output C structure is allocated in the
**	    output buffer as follows: space for the fixed portion
**	    of the structure and any VARIMPAR subelements comes
**	    from the start of the buffer (dst1) while VAREXPAR
**	    arrays are allocated from the end of the buffer (dst2).
**	    Space at the start of the buffer is reserved (rsrv),
**	    when the message or VARIMPAR subelement is begun, so
**	    that VAREXPAR allocation will not overlap with the
**	    portion of the structure being processed.
**
**	    In the simple case, where the output C structure, all
**	    VARIMPAR subelements and VAREXPAR arrays fit in the
**	    output buffer, dst1 will point just past the fixed
**	    portion of the structure or VARIMPAR subelement and
**	    dst2 will point to the start or the VAREXPAR data.
**	    The caller should loop (while state is DOC_MOREIN)
**	    until state is DOC_DONE, providing new input buffers 
**	    (src1 and src2) for each call.
**
**	    If there is not sufficient room in the output buffer
**	    for the entire C structure and related variable length
**	    subelements, the structure will be returned in pieces.
**	    However, the buffer must be large enough to hold the
**	    fixed portion of the message along with any associated
**	    VAREXPAR subelements, or one VARIMPAR subelement and
**	    associated VAREXPAR subelements.  
**
**	    On the first call, the fixed portion of the message 
**	    will be returned and any VARIMPAR subelements which 
**	    fit (the fixed portion of the message or VARIMPAR sub-
**	    element will not be split).  Subsequent calls return 
**	    arrays of VARIMPAR subelements.  The caller should loop 
**	    until state is DOC_DONE, and after each iteration decide 
**	    whether a new output buffer (state is DOC_MOREOUT) or 
**	    input buffer (state is DOC_MOREIN) is needed.
**
** Inputs:
**	doc->state		DOC_IDLE on first call
**	doc->src1		start of input data
**	doc->src2		end of input data 
**	doc->dst1		start of output buffer
**	doc->dst2		end of output buffer
**	doc->eod 		TRUE if message end-of-data
**	doc->message_type	GCA message type
**
** Outputs:
**	doc->src1		unused part of input buffer
**	doc->dst1		unused part of output buffer (upto dst2).
**	doc->dst2		start of VAREXPAR data
**	doc->state		DOC_MOREIN - more input required
**				DOC_MOREOUT - more output required
**				DOC_DONE - conversion complete
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Mar-96 (gordy)
**	    Fixed processing of large messages which span buffers.
**	    Added OP_MARK, OP_REWIND, OP_JNOVR to detect output buffer
**	    overflow in gcd_decode() and reset the processing state
**	    so that only a complete C data structure is returned.  GCD
**	    messages structures were changed such that all variable
**	    length message end with VARIMPAR arrays which can easily
**	    be used as the split point when buffer overlow occurs.
**	 3-Apr-96 (gordy)
**	    Added casts to tracing.  Removed OP_JNOVR and added OP_CHKLEN
**	    and OP_DV_ADJUST.  OP_VAR_BEGIN and OP_DV_BEGIN once again
**	    can interrupt processing, but do so by issuing a OP_REWIND.
**	    Since BLOB segments in GCA_DATA_VALUES and do an OP_BREAK,
**	    need to carfully chack the psp stack during subsequent
**	    handling because traling segments are placed at the top
**	    of the buffer rather than indirectly in the buffer through
**	    a pointer as is done for all other GCA_DATA_VALUES.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG for messages which are not formatted into 
**	    elements.  The entire message is copied from input to output.
**	    We no longer process tuples as individual elements, so CALLTUP
**	    now uses the global program for tuple messages rather than
**	    the DOC program for the current tuples.
**	 5-Feb-97 (gordy)
**	    Added OP_CALL_DDT for pre-compiled DBMS datatype processing.
**	    OP_PUSH_VCH is now passed both the max and actual data lengths
**	    on the data stack.  OP_COPY_MSG now expects the element size
**	    as an argument.  Removed OP_CHKLEN, OP_COMP_PAD.
**	 7-Sep-97 (gordy)
**	    Skip double byte op-codes which are not relevant here.
**	22-Mar-02 (gordy)
**	    More nat to i4 cleanup.  Removed unneeded atomic types.
**	12-May-03 (gordy)
**	    Renamed DOC->last_seg to eod to match message flag.
**	    DOC_DONE now distinct from DOC_IDLE for HET/NET processing.
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**	08-Mar-07 (gordy, gupsh01)
**	    Fixed the case OP_PUSH_LIT, to correctly obtain the values.
**	22-Jan-10 (gordy)
**	    Renamed dst3 to rsrv to separate from external interface.
**	    Added operations for multi-byte charset processing.
**	 2-Mar-10 (gordy)
**	    Restore some code lost in preceding change.
*/

STATUS
gco_decode( GCO_DOC *doc )
{
    register i4	i;		/* for counting converted items */
    GCO_OP		arg;		/* arguments in the program */
    GCA_ELEMENT		ela;		/* to hold GCA_DATA_VALUES */
    i4			item_size;
    bool		pop;
    STATUS		status;

    register char	*src = doc->src1;
    register char	*dst = doc->dst1;

    u_i2		t_ui2;
    i4			t_i4;

    /*
    ** Upon entry, there is no saved processing state.
    */
    doc->mark.pc = NULL;

    /* 
    ** If fresh message, load program according to message type.
    ** (DOC_IDLE and DOC_DONE used to be identical.  Changes in
    ** HET/NET processing resulted in a need to distinguish the
    ** two.  Treat them as the same until this code gets the
    ** same cleanup)
    */
    if ( doc->state == DOC_IDLE  ||  doc->state == DOC_DONE )
    {
	doc->state = DOC_MOREOUT;	/* Pick up rsrv below */
	gco_init_doc( doc, doc->message_type );
    } 

    if ( doc->state == DOC_MOREOUT )
    {
	/*
	** We have been given a fresh output buffer, either because
	** we are at the start of a new message or we returned part
	** of a message and are now being asked for the rest.  We
	** may be at the start of a message or VARIMPAR element, in
	** which case rsrv will be set shortly.  But we could have 
	** just executed OP_BREAK, in which case we need to set rsrv.  
	** In all cases, setting rsrv is a prudent thing to do.  In 
	** addition, since we have a new output buffer, any saved 
	** pointers are now invalid, so flush the pointer stack.
	*/
	doc->rsrv = doc->dst1;
	doc->psp = doc->ptr_stack;
    }

    /* 
    ** Step through instructions.
    */

top:

    /*
    ** Rather than checking the value stack in the many places
    ** where it is modified, check here between each operation.
    */
    if ( doc->vsp < &doc->val_stack[ 0 ]  ||
         doc->vsp > &doc->val_stack[ DOC_VAL_DEPTH ] )
    {
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pdd internal error: value stack under/over flow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( E_GC0050_PDD_ERROR );
    }

# ifdef GCXDEBUG
    if ( gco_trace_level >= 2 )
    {
    	switch( *doc->pc )
	{
	/*
	** The following operations are strictly for tracing
	** and do not need additional tracing.
	*/
	case OP_UP:
	case OP_DOWN:
	case OP_DEBUG_TYPE:
	case OP_DEBUG_BUFS:
	    break;

	/*
	** The following operations are NO-OPs and do not need to be traced.
	*/
	case OP_END_TUP:
	case OP_UPD_REFI2:
	case OP_UPD_REFI4:
	case OP_ADJUST_PAD:
	case OP_SAVE_MSG:
	case OP_RSTR_MSG:
	case OP_DV_REF:
	case OP_SEG_BEGIN:
	case OP_SEG_END:
	case OP_SEG_CHECK:
	case OP_PAD:
	case OP_SKIP_INPUT:
	case OP_SAVE_CHAR:
	case OP_RSTR_CHAR:
	    break;

	default:
	    if ( gco_trace_level >= 3 )
	    {
		i4 *i;

		TRdisplay( "%s-- %23s\t[", tabs( doc->depth ), 
			   gcx_look( gcx_gco_ops, (i4)*doc->pc ) );

		for( i = doc->val_stack; i < doc->vsp; i++ )
		    TRdisplay( "%d ", *i );

		TRdisplay( "]\n" );
	    }
	    else
	    {
	    	TRdisplay( "%s-- %s\n", tabs( doc->depth ),
			   gcx_look( gcx_gco_ops, (i4)*doc->pc ) );
	    }
	    break;
	}
    }
# endif /* GCXDEBUG */

    switch( *doc->pc++ )
    {
    case OP_ERROR:		/* error has occured */
	arg = *doc->pc++;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 1 )
	    TRdisplay( "pdd error: %s\n", gcx_look( gcx_gcc_eop, (i4)arg ) );
# endif /* GCXDEBUG */

	doc->state = DOC_DONE;
	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc -= 2;
	return( E_GC0050_PDD_ERROR );

    case OP_DONE:		/* complete msg converted */
	doc->state = DOC_DONE;
	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc--;
	return( OK );

    case OP_DOWN: 		/* decend into complex type */
	doc->depth++;
	break;

    case OP_UP: 		/* finish complex type */
	doc->depth--;
	break;

    case OP_DEBUG_TYPE:		/* trace the current type */
	TRdisplay( "%s== (%s, %d, %d)\n", tabs( doc->depth ),
		   gcx_look( gcx_datatype, (i4)doc->pc[0] ),
		   (i4)doc->pc[1], (i4)doc->pc[2] );
	doc->pc += 3;
	break;

    case OP_DEBUG_BUFS:		/* trace buffer info */
	TRdisplay( "%s++ optr %x ilen %d (%d frag)\n", 
		   tabs( doc->depth ), dst, (i4)(doc->src2 - src), 
		   doc->frag_bytes );
	break;

    case OP_JZ:			/* pop top-of-stack; branch if zero */
	arg = *doc->pc++;
	if ( ! *(--doc->vsp) )  doc->pc += arg;
	break;

    case OP_DJNZ:		/* if !top-of-stack pop, else decr & branch */
	arg = *doc->pc++;
	if ( doc->vsp[-1]-- )
	    doc->pc += arg;
	else
	    --doc->vsp;
	break;

    case OP_JNINP:		/* branch if input depleted */
	arg = *doc->pc++;
	if ( doc->eod  &&  src >= doc->src2 )  doc->pc += arg;
	break;

    case OP_GOTO: 		/* branch unconditionally */
	arg = *doc->pc++;
	doc->pc += arg;
	break;

    case OP_CALL_DDT:		/* call DBMS datatype program */
	arg = *doc->pc++;

	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pdd internal error: DDT call stack overflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc -= 2;
	    return( E_GC0050_PDD_ERROR );
	}

	*doc->csp++ = doc->pc;
	doc->pc = gco_ddt_map[ arg ];
	doc->depth++;
	break;

    case OP_CALL_DV: 		/* call data value program */
	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pdd internal error: DV call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0050_PDD_ERROR );
	}

	ela.tpl.length = *(--doc->vsp);
	ela.tpl.precision = *(--doc->vsp);
	ela.tpl.type = *(--doc->vsp);

	gco_comp_dv( &ela, doc->od_dv_prog );
	*doc->csp++ = doc->pc;
	doc->pc = doc->od_dv_prog;
	doc->depth++;
	break;

    case OP_CALL_TUP:		/* call tuple program */
	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pdd internal error: TUP call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0050_PDD_ERROR );
	}

	*doc->csp++ = doc->pc;
	doc->pc = doc->tod_prog ? (GCO_OP *)doc->tod_prog : gco_tuple_prog;
	doc->depth++;
	break;

    case OP_END_TUP:		/* tuple processing completed */
        break;

    case OP_RETURN:		/* return from program call */
	if ( doc->csp <= &doc->call_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pdd internal error: RETURN call stack underflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0050_PDD_ERROR );
	}

	doc->pc = *(--doc->csp);
	doc->depth--;
	break;

    case OP_PUSH_LIT:		/* push 2 byte signed quanity */
	*doc->vsp++ = (u_i2) *doc->pc++;
	break;

    case OP_PUSH_I2:		/* push i2 from data stream */
    case OP_PUSH_REFI2:		/* push i4 & reference from output stream */
	MECOPY_CONST_MACRO( dst - sizeof(t_ui2), sizeof(t_ui2), (PTR)&t_ui2 );
	*doc->vsp++ = t_ui2;
	break;

    case OP_PUSH_I4:		/* push i4 from data stream */
    case OP_PUSH_REFI4:		/* push i4 & reference from output stream */
	MECOPY_CONST_MACRO( dst - sizeof(t_i4), sizeof(t_i4), (PTR)&t_i4 );
	*doc->vsp++ = t_i4;
	break;

    case OP_UPD_REFI2:		/* update i2 reference in output stream */
    case OP_UPD_REFI4:		/* update i4 reference in output stream */
	/* Reference changes not needed! */
    	break;

    case OP_PUSH_VCH:		/* compute varchar padding length */
        /*
        ** Adjust lengths for processing data and padding.
	** The array element size is provided as argument.
        ** The stack contains the number of elements from
        ** the embedded length field just processed and 
        ** the maximum length of the object (includes the
        ** embedded length field which is removed).
        */
        arg = *doc->pc++;			/* Size of elements */
        t_ui2 = *(--doc->vsp);                   /* Number of elements */
        t_i4 = *(--doc->vsp) - sizeof( t_ui2 );	/* Max data length (bytes) */  

	/*
	** Length can be bogus on nullable variable length types.
	** Adjust if greater than max length of varchar.  Also
	** fix the output stream just to be a little bit safer.
	*/
	if ( (t_ui2 * arg) > (u_i2)t_i4 )
	{
	    t_ui2 = 0;
	    MECOPY_CONST_MACRO( (PTR)&t_ui2, 
				sizeof(t_ui2), (PTR)(dst - sizeof(t_ui2)) );
	}

	*doc->vsp++ = t_i4 - (t_ui2 * arg);	/* length of pad (bytes) */
	*doc->vsp++ = t_ui2;			/* Number of elements */
	break;

    case OP_CHAR_LIMIT:		/* character length is limited (0 pad) */
	/*
	** The stack contains the number of elements 
	** to be processed.  For limited character
	** values, the padding length (which in this
	** case is 0), must also be on the stack
	** below the length.
	*/
	t_ui2 = *(--doc->vsp);			/* Number of elements */
	*doc->vsp++ = 0;			/* length of pad (bytes) */
	*doc->vsp++ = t_ui2;			/* Number of elements */
    	break;

    case OP_ADJUST_PAD:		/* adjust padding for data length change */
	/* Reference changes not needed! */
    	break;

    case OP_POP:		/* discard top of stack */
    	doc->vsp--;
	break;

    case OP_MSG_BEGIN:		/* note beginning of message */
    	/* 
	** Beginning of a message: ensure there is room in the output
	** buffer for the fixed portion of the message.
	**
	** arg1 = message number for tracing
	** arg2 = size of fixed part of message structure 
	*/
	arg = *(doc->pc)++;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ message %s %d bytes olen %d\n", 
		       tabs( doc->depth ), gcx_look( gcx_gca_msg, (i4)arg ), 
		       (i4)*doc->pc, (i4)(doc->dst2 - dst) );
# endif /* GCXDEBUG */

	/*
	** Reserve space for the message.  If there is not
	** sufficient room for the fixed portion of the
	** message, call OP_REWIND to request more output
	** space and return processing to the last marked
	** position.
	*/
	doc->rsrv = dst + *(doc->pc++);
	if ( doc->rsrv > doc->dst2 )  doc->pc = &op_rewind[0];
	break;

    case OP_IMP_BEGIN:		/* note beginning of GCA_VARIMPAR */
	/* 
	** Beginning of a VARIMPAR array: make sure there is room
	** at the end of the current output structure for at least 
	** one element of the array.  If not enough output space
	** available, return current info and get more output.
	**
	** arg1 = size of substructure
	*/
	arg = *(doc->pc++);

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ impar %d bytes olen %d\n", 
		       tabs( doc->depth ), (i4)arg, (i4)(doc->dst2 - dst) );
# endif /* GCXDEBUG */

	/*
	** Reserve space for the array element and check for overflow.
	*/
	doc->rsrv = dst + arg;
	if ( doc->rsrv > doc->dst2 )  doc->pc = &op_rewind[0];
	break;

    case OP_COPY_MSG:		/* Copy remaining input to output */
	/*
	** Copy as much of the message to the output
	** buffer as possible.  If the output is full,
	** request more output space by rewinding to
	** the last marked position.  Since we are not 
	** at end-of-input, make sure we ask for at least
	** one element so that the conversion routines
	** will get more input if needed.
	*/
	arg = *(doc->pc)++;	/* Size of array element */

	if ( (dst + arg) > doc->dst2 )
	    doc->pc = &op_rewind[0];		/* Get more output */
	else
	    *doc->vsp++ = max( 1, (min( (doc->src2 - src) + doc->frag_bytes, 
					doc->dst2 - dst )) / arg );
	break;

    case OP_CHAR_MSG:		/* Copy character input to output */
	/*
	** Copy as much of the message to the output
	** buffer as possible.  If the output is full,
	** request more output space by rewinding to
	** the last marked position.  Since we are not 
	** at end-of-input, make sure we ask for at least
	** one element so that the conversion routines
	** will get more input if needed.
	*/
	if ( dst >= doc->dst2 )
	    doc->pc = &op_rewind[0];		/* Get more output */
	else
	    *doc->vsp++ = min( (doc->src2 - src) + doc->frag_bytes, 
			      doc->dst2 - dst );
	break;

    case OP_SAVE_MSG:		/* Save input */
    case OP_RSTR_MSG:		/* Restore input */
        break;

    case OP_VAR_BEGIN:		/* note beginning of GCA_VAREXPAR */
	/* 
	** Beginning of VAREXPAR array: consume from dst2 room for the
	** VAREXPAR data: the size of substructure multiplied by the array
	** count.  Stuff a pointer to this new data into the current output 
	** location, and load dst with the new pointer.  Make sure VAREXPAR 
	** data is properly aligned.
	**
	** arg1 = size of substructure
	** arg2 = alignment type
	** arg3 = alignment length
	*/

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ expar %d els * %d bytes align(%s, %d) olen %d\n", 
		       tabs( doc->depth ), doc->vsp[-1], (i4)doc->pc[0], 
		       gcx_look( gcx_atoms, (i4)doc->pc[1] ), 
		       (i4)doc->pc[2], (i4)(doc->dst2 - doc->rsrv) );
# endif /* GCXDEBUG */

	i = *(doc->pc++) * doc->vsp[-1];	/* Size of array */

	/*
	** Ensure proper alignment of array by adding
	** padding at the end to align the start.
	** Funky looking code but gco-align-pad only cares about the low bits
	** of the adjusted pointer.
	*/
	ela.tpl.type = *(doc->pc++);
	ela.tpl.length = *(doc->pc++);

	for( 
	     t_i4 = 0; 
	     gco_align_pad( &ela, (u_i2)(SCALARP)(doc->dst2 - i - t_i4) ); 
	     t_i4++ 
	   );

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3  &&  t_i4 )
	    TRdisplay( "%s++ align %d bytes\n", tabs( doc->depth ), t_i4 );
# endif /* GCXDEBUG */

	/*
	** Reserve space for the array.  If there is not
	** sufficient room for the array, call OP_REWIND 
	** to request more output space and return 
	** processing to the last marked position.
	*/
	doc->dst2 -= i + t_i4;

	if ( doc->rsrv > doc->dst2 )  
	    doc->pc = &op_rewind[0];
	else  if ( doc->psp >= &doc->ptr_stack[ DOC_PTR_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pdd internal error: VAR psp stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc -= 4;
	    return( E_GC0050_PDD_ERROR );
	}
	else
	{
	    *((char **)dst) = doc->dst2;
	    *(doc->psp++) = dst;
	    dst = doc->dst2;
	}
	break;

    case OP_VAR_END:		/* note end of GCA_VAREXPAR */
	/*
	** Pop the original destination and
	** step over the pointer placed there.
	*/
	if ( doc->psp <= &doc->ptr_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pdd internal error: VAR psp stack underflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0050_PDD_ERROR );
	}

	dst = *(--doc->psp) + sizeof(char *);
	break;

    case OP_DV_BEGIN:		/* note beginning of GCA_DATA_VALUE value */
	/* 
	** Beginning of GCA_DATA_VALUE value: consume from dst2 room 
	** for the data value length.  Stuff a pointer to this new data 
	** into the current output location, and load dst with the new 
	** pointer.
	**
	** arg1 = length
	*/
	arg = *(doc->pc++);

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ data value %d bytes (+ %d byte ptr) olen %d\n", 
		       tabs(doc->depth), (i4)arg, (i4)sizeof(char *), 
		       (i4)(doc->dst2 - doc->rsrv) );
# endif /* GCXDEBUG */

	/*
	** Reserve space for the data value.  If there is not
	** sufficient room for the data value, call OP_REWIND 
	** to request more output space and return processing 
	** to the last marked position.
	*/
	doc->dst2 -= arg;

	if ( doc->rsrv > doc->dst2 )  
	    doc->pc = &op_rewind[0];
	else  if ( doc->psp >= &doc->ptr_stack[ DOC_PTR_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pdd internal error: DV psp stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc -= 2;
	    return( E_GC0050_PDD_ERROR );
	}
	else
	{
	    *((char **)dst) = doc->dst2;
	    *(doc->psp++) = dst;
	    dst = doc->dst2;
	}
	break;

    case OP_DV_END:		/* note end of GCA_DATA_VALUE value */
	/* 
	** End of GCA_DATA_VALUE value: pop the dst pointer back
	**
	** BLOB segments are sent one GCA request at a time.  The
	** pointer stack is flushed upon re-entry after processing
	** the first segment.  If the pointer stack is empty, a
	** BLOB has been processed and the dst pointer restoration
	** cannot be done.  But we can't just place the next part
	** of the message following the BLOB segment, so we BREAK 
	** at the end of the BLOB, as was done for the intermediate 
	** segments, and upon re-entry will begin processing the 
	** rest of the GCA message.
	*/
	if ( doc->psp > doc->ptr_stack )
	{
	    /*
	    ** Pop the original destination and
	    ** step over the pointer placed there.
	    */
	    dst = *(--doc->psp) + sizeof(char *);
	}
	else
	{
	    /*
	    ** Return current output to caller.
	    */
	    doc->state = DOC_MOREOUT;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( OK );
	}
	break;

    case OP_DV_ADJUST:		/* Adjust buffer space for GCA_DATA_VALUE */
	/*
	** Some GCA object lengths are not known until the
	** actual data is being processed (BLOB segments for
	** example).  We were only able to allocate space
	** for the fixed portion of the object in DV_BEGIN.
	** We now need to adjust the allocation to allow
	** room for the variable length data.
	*/
	if ( doc->psp > doc->ptr_stack )
	{
	    /*
	    ** The length of the variable portion of the object
	    ** is on the stack.  Since we allocate from the end 
	    ** of the buffer backwards, we also need to fix the 
	    ** pointer we placed in the parent data object which 
	    ** points at the current object.
	    **
	    ** doc->vsp[-1]	Length of variable portion of object.
	    ** doc->dst2	Start of fixed portion of object.
	    ** dst		End of fixed portion of object.
	    ** doc->psp[-1]	Position of object pointer in parent.
	    **
	    ** NOTE: data length is in elements.  The following is 
	    ** not correct for all types, but should be correct for 
	    ** the types which are currently returned from a server 
	    ** in GCA_DATA_VALUE.
	    */

	    /*
	    ** Set parent object pointer to the new location.
	    */
	    *((char **)doc->psp[-1]) = doc->dst2 - doc->vsp[-1];

	    /*
	    ** Check for buffer overflow on the new data.
	    */
	    if ( doc->rsrv > *((char **)doc->psp[-1]) )  
		doc->pc = &op_rewind[0];
	    else
	    {
		/*
		** Copy fixed portion of the object and
		** reset the buffer positions.
		*/
		MEcopy( (PTR)doc->dst2, 
			dst - doc->dst2, (PTR)*((char **)doc->psp[-1]) );

		dst -= doc->vsp[-1];
		doc->dst2 -= doc->vsp[-1];
	    }
	}
	else
	{
	    /*
	    ** Object lists return list objects individually.
	    ** For all but the first object, the data is being
	    ** placed directly into the start of the buffer (no
	    ** indirection through a parent object).  There is
	    ** no allocation to adjust, but we do need to check 
	    ** for possible buffer overflow.
	    */
	    if ( dst + doc->vsp[-1] > doc->dst2 )  
		doc->pc = &op_rewind[0];
	}
	break;

    case OP_DV_REF:		/* save meta-data length reference */
    case OP_SEG_BEGIN:		/* note beginning of list segments */
    case OP_SEG_END:		/* note end of list segments */
    	break;

    case OP_SEG_CHECK:		/* check for end-of-segments */
	arg = *doc->pc++;
    	break;

    case OP_MARK:		/* save current processing state */
	/*
	** Save our current location and processing
	** state.  Buffer overflow conditions can use
	** the OP_REWIND operation to return to this
	** point.
	*/
	doc->mark.pc = doc->pc - 1;
	doc->mark.csp = doc->csp;
	doc->mark.vsp = doc->vsp;
	doc->mark.psp = doc->psp;
	doc->mark.depth = doc->depth;
	doc->mark.src1 = src;
	doc->mark.dst1 = dst;
	break;

    case OP_REWIND:		/* backup to saved processing state */
	if ( ! doc->mark.pc )
	{
	    /*
	    ** No saved processing state to restore.
	    */
	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    doc->state = DOC_DONE;
	    status = E_GC0050_PDD_ERROR;
	}
	else
	{
	    /*
	    ** Restore saved processing state.
	    */
	    doc->pc = doc->mark.pc;
	    doc->csp = doc->mark.csp;
	    doc->vsp = doc->mark.vsp;
	    doc->psp = doc->mark.psp;
	    doc->depth = doc->mark.depth;
	    src = doc->mark.src1;
	    dst = doc->mark.dst1;

	    /*
	    ** Get more output.
	    */
	    status = (dst == doc->dst1) ? E_GC0051_PDD_BADSIZE : OK;
	    doc->state = DOC_MOREOUT;
	    doc->src1 = src;
	    doc->dst1 = dst;
	}
	return( status );

    case OP_PAD_ALIGN:		/* alignment pad for structure */
	dst += *doc->pc++;
	break;

    case OP_DIVIDE:		/* Divide stack by argument */
	arg = *doc->pc++;				/* Divisor */
	t_i4 = *(--doc->vsp);			/* Dividend */
	*doc->vsp++ = (arg < 2) ? t_i4 : (t_i4 / arg);
	break;

    case OP_BREAK:		/* end of data segment */
	/*
	** Return current output to caller.
	*/
	doc->state = DOC_MOREOUT;
	doc->src1 = src;
	doc->dst1 = dst;
	return( OK );

    /*
    ** Start of conversion operations.
    **
    ** We only convert as many elements as currently available
    ** in the input buffer.  If there are additional elements
    ** to convert, we request more input and convert the 
    ** remaining elements once they are available.
    */

    case OP_CV_I1:   item_size = sizeof( i1 );	pop = TRUE;	goto cv;
    case OP_CV_I2:   item_size = sizeof( i2 );	pop = TRUE;	goto cv;
    case OP_CV_I4:   item_size = sizeof( i4 );	pop = TRUE;	goto cv;
    case OP_CV_I8:   item_size = sizeof( i8 );	pop = TRUE;	goto cv;
    case OP_CV_F4:   item_size = sizeof( f4 );	pop = TRUE;	goto cv;
    case OP_CV_F8:   item_size = sizeof( f8 );	pop = TRUE;	goto cv;
    case OP_CV_CHAR: item_size = sizeof(char);	pop = FALSE;	goto cv; 
    case OP_CV_BYTE: 
    case OP_CV_PAD:  item_size = 1;		pop = TRUE;	goto cv;

    case OP_PAD:		/* output padding */
    case OP_SKIP_INPUT:		/* Skip remaining input */
    case OP_SAVE_CHAR:		/* Save character input */
    	/*
	** OP_CV_CHAR processes all input, so there is 
	** nothing to do except discard the input length 
	** left on stack.
	*/
    	doc->vsp--;
	break;

    case OP_RSTR_CHAR:		/* Restore character input */
    	break;

    cv:
	/*
	** Limit vector count to input available.  OP_MSG_BEGIN,
	** OP_COPY_MSG, OP_IMP_BEGIN, OP_VAR_BEGIN and OP_DV_BEGIN 
	** made sure there was sufficient output space.  Consume 
	** any carry over in the fragment buffer.
	*/
	i = doc->vsp[-1];
	VEC_MACRO( i, (doc->src2 - src) + doc->frag_bytes, item_size );
	doc->vsp[-1] -= i;
	item_size *= i;

	if ( doc->frag_bytes )
	{
# ifdef GCXDEBUG
		    if ( gco_trace_level >= 3 )
			TRdisplay( "%s++ consuming %d fragment bytes\n", 
				   tabs( doc->depth ), doc->frag_bytes );
# endif /* GCXDEBUG */

	    MEcopy( doc->frag_buffer, doc->frag_bytes, dst );
	    dst += doc->frag_bytes;
	    item_size -= doc->frag_bytes;
	    doc->frag_bytes = 0;
	}

	MEcopy( src, item_size, dst );
	src += item_size;
	dst += item_size;

	if ( doc->vsp[-1] )	/* More elements to convert? */
	{
	    /*
	    ** We need more input, but we just can't go get more
	    ** because of a rather obscure problem with output
	    ** overflows.  If we get more input at this point, we
	    ** throw away the current marked processing point.
	    ** If the output buffer then overflows before we hit
	    ** the next OP_MARK instruction, we are hosed.  We
	    ** can avoid this problem by maximizing the output
	    ** space available.  If there is a marked processing
	    ** point with a non-empty output buffer, fake an
	    ** OP_REWIND instruction to clear the output buffer.
	    ** At some point we will reach this point again in
	    ** the processing, but the marked output buffer will
	    ** be empty.  If we then get more input and the output
	    ** buffer still overflows, then it is a buffer size
	    ** error and the only solution is for the caller to
	    ** use a larger output buffer.
	    */
	    if ( doc->mark.pc  &&  doc->mark.dst1 != doc->dst1 )
		doc->pc = &op_rewind[0];	/* Fake an OP_REWIND */
	    else
	    {
		/*
		** Get more input.  There may be a partial atomic
		** value in the input buffer.  If so, save it in
		** the fragment buffer.  Reset DOC to convert the
		** additional elements.
		*/
		if ( (item_size = doc->src2 - src) > 0  &&  
		     item_size <= sizeof( doc->frag_buffer ) )
		{
		    MEcopy( src, item_size, doc->frag_buffer );
		    src += item_size;
		    doc->frag_bytes = item_size;

# ifdef GCXDEBUG
		    if ( gco_trace_level >= 3 )
			TRdisplay( "%s++ saving %d fragment bytes\n", 
				   tabs( doc->depth ), doc->frag_bytes );
# endif /* GCXDEBUG */
		}

		doc->state = DOC_MOREIN;
		doc->src1 = src;
		doc->dst1 = dst;
		doc->pc--;
		return( OK );
	    }
	}

	/* 
	** All items converted - pop vector count
	*/
	if ( pop )  doc->vsp--;
	break;
    }

    goto top;
}


/*
** Name: gco_encode() - encode a message for GCA_SEND
**
** Description:
**	
**	Perform_encoding() takes a C structure containing a GCA message and
**	turns it into the byte stream representing the message.  In doing
**	so gco_encode() uses message descriptors compiled by gcocomp.c.
**	These compiled message descriptors contain instructions on how to
**	encode messages, and gco_encode() interprets these instructions.
**	
**	Encoding messages involves five special cases:
**	
**	1) Most elements are just copied from the C structure to the message
**	buffer, using alignment-safe copying.  These are carried out by
**	the OP_CV instructions.
**	
**	2) Structure pads are skipped in the input as necessary.  This is
**	done by the OP_PAD_ALIGN instruction.
**	
**	3) Substructures with array length of VARIMPAR are of a variable 
**	count, which is determined by the input source length.  The 
**	OP_IMP_BEGIN instruction just makes sure that the caller has 
**	provided a whole substructure in the input.
**
**	4) Substructures with array length of VAREXPAR are represented 
**	in the C structure by a pointer to the actual substructure array.
**	The OP_VAR_BEGIN instruction pushes the current source pointer 
**	and then deferences it to find the new source pointer.  When the
**	substructure has been converted, the OP_VAR_END pops the original
**	source pointer.
**
**	5) GCA_DATA_VALUES are three numbers (type, precision, length)
**	followed by a VAREXPAR data value, but that VAREXPAR value is 
**	treated as a single substructure of array length 1.
**	
**	The use of src1, src2, dst1, dst2, eod and state need some
**	explanation:
**
**	    In the simple case, where the input C structure does
**	    not end with a partial array of VARIMPAR subelements,
**	    src1 and src2 should be set to the beginning and end of
**	    the C structure, eod should be TRUE, and state should
**	    be DOC_IDLE.  The caller should loop (while state is
**	    DOC_MOREOUT) until state is DOC_DONE, providing new 
**	    output buffers (dst1, dst2) for each call.  The input 
**	    C structure must in this case be whole.
**
**	    In the complicated case, where the input C structure
**	    does end with a partial array of VARIMPAR sub-elements,
**	    src1 and src2 should be set to the beginning and end
**	    of the present part of the structure, eod should be
**	    FALSE, and state should be DOC_IDLE.  The caller should
**	    loop until state is DOC_DONE, and after each iteration 
**	    decide whether new output buffers (state is DOC_MOREOUT) 
**	    or new input (state is DOC_MOREIN) is needed.  When 
**	    providing the final VARIMPAR sub-elements, eod should 
**	    be TRUE.
**
** Inputs:
**	doc->state		DOC_IDLE on first call
**	doc->src1		start of input data
**	doc->src2		end of input data 
**	doc->dst1		start of output buffer
**	doc->dst2		end of output buffer
**	doc->eod 		TRUE if message end-of-data
**	doc->message_type	GCA message type
**
** Outputs:
**	doc->src1		unused part of input buffer
**	doc->dst1		unused part of output buffer
**	doc->state		DOC_MOREIN - more input required
**				DOC_MOREOUT - more output buffer needed
**				DOC_DONE - conversion complete
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 8-Mar-96 (gordy)
**	    Extended for new instructions added for gcd_decode().
**	 3-Apr-96 (gordy)
**	    Added casts to tracing.  Removed OP_JNOVR and added OP_CHKLEN
**	    and OP_DV_ADJUST.  OP_VAR_BEGIN and OP_DV_BEGIN once again
**	    can interrupt processing, but do so by issuing a OP_REWIND.
**	    Since BLOB segments in GCA_DATA_VALUES and do an OP_BREAK,
**	    need to carfully chack the psp stack during subsequent
**	    handling because traling segments are placed at the top
**	    of the buffer rather than indirectly in the buffer through
**	    a pointer as is done for all other GCA_DATA_VALUES.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG for messages which are not formatted into 
**	    elements.  The entire message is copied from input to output.
**	    We no longer process tuples as individual elements, so CALLTUP
**	    now uses the global program for tuple messages rather than
**	    the DOC program for the current tuples.
**	 5-Feb-97 (gordy)
**	    Added OP_CALL_DDT for pre-compiled DBMS datatype processing.
**	    OP_PUSH_VCH is now passed both the max and actual data lengths
**	    on the data stack.  OP_COPY_MSG now expects the element size
**	    as an argument.  Removed OP_CHKLEN, OP_COMP_PAD.
**	 7-Sep-97 (gordy)
**	    Skip double byte op-codes which are not relevant here.
**	22-Mar-02 (gordy)
**	    More nat to i4 cleanup.  Removed unneeded atomic types.
**	12-May-03 (gordy)
**	    Renamed DOC->last_seg to eod to match message flag.
**	    DOC_DONE now distinct from DOC_IDLE for HET/NET processing.
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**	22-Jan-10 (gordy)
**	    Renamed dst3 to rsrv to separate from external interface.
**	    Added operations for multi-byte charset processing.
*/

STATUS
gco_encode( GCO_DOC *doc )
{
    register i4	i;		/* for counting converted items */
    GCO_OP		arg;		/* arguments in the program */
    GCA_ELEMENT		ela;		/* to hold GCA_DATA_VALUES */
    i4			item_size;
    bool		pop;
    STATUS		status;

    register char	*src = doc->src1;
    register char	*dst = doc->dst1;

    u_i2		t_ui2;
    i4			t_i4;

    /*
    ** Upon entry, there is no saved processing state.
    */
    doc->mark.pc = NULL;

    /* 
    ** If fresh message, load program according to message type.
    ** (DOC_IDLE and DOC_DONE used to be identical.  Changes in
    ** HET/NET processing resulted in a need to distinguish the
    ** two.  Treat them as the same until this code gets the
    ** same cleanup)
    */
    if ( doc->state == DOC_IDLE  ||  doc->state == DOC_DONE )
    {
	doc->state = DOC_MOREIN;	/* Start of new message */
	gco_init_doc( doc, doc->message_type );
    }

    if ( doc->state == DOC_MOREIN )
    {
	/*
	** We have been given a fresh input buffer, either because
	** we are at the start of a new message or the application
	** split the message across multiple buffers.  In either
	** case, we can flush the pointer stack since any saved
	** pointers are now invalid.
	*/
	doc->psp = doc->ptr_stack;
    }

    /* 
    ** Step through instructions.
    */

top:

    /*
    ** Rather than checking the value stack in the many places
    ** where it is modified, check here between each operation.
    */
    if ( doc->vsp < &doc->val_stack[ 0 ]  ||
         doc->vsp > &doc->val_stack[ DOC_VAL_DEPTH ] )
    {
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pde internal error: value stack under/over flow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( E_GC0055_PDE_ERROR );
    }

# ifdef GCXDEBUG
    if ( gco_trace_level >= 2 )
    {
    	switch( *doc->pc )
	{
	/*
	** The following operations are strictly for tracing
	** and do not need additional tracing.
	*/
	case OP_UP:
	case OP_DOWN:
	case OP_DEBUG_TYPE:
	case OP_DEBUG_BUFS:
	    break;

	/*
	** The following operations are NO-OPs and do not need to be traced.
	*/
	case OP_END_TUP:
	case OP_UPD_REFI2:
	case OP_UPD_REFI4:
	case OP_ADJUST_PAD:
	case OP_SAVE_MSG:
	case OP_RSTR_MSG:
	case OP_DV_REF:
	case OP_SEG_BEGIN:
	case OP_SEG_END:
	case OP_SEG_CHECK:
	case OP_PAD:
	case OP_SKIP_INPUT:
	case OP_SAVE_CHAR:
	case OP_RSTR_CHAR:
	    break;


	default:
	    if ( gco_trace_level >= 3 )
	    {
		i4 *i;

		TRdisplay( "%s-- %23s\t[", tabs( doc->depth ), 
			   gcx_look( gcx_gco_ops, (i4)*doc->pc ) );

		for( i = doc->val_stack; i < doc->vsp; i++ )
		    TRdisplay( "%d ", *i );

		TRdisplay( "]\n" );
	    }
	    else
	    {
	    	TRdisplay( "%s-- %s\n", tabs( doc->depth ),
			   gcx_look( gcx_gco_ops, (i4)*doc->pc ) );
	    }
	    break;
	}
    }
# endif /* GCXDEBUG */

    switch( *doc->pc++ )
    {
    case OP_ERROR:		/* error has occured */
	arg = *doc->pc++;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 1 )
	    TRdisplay( "pde error: %s\n", gcx_look( gcx_gcc_eop, (i4)arg ) );
# endif /* GCXDEBUG */

	doc->state = DOC_DONE;
	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc -= 2;
	return( E_GC0055_PDE_ERROR );

    case OP_DONE:		/* complete msg converted */
	doc->state = DOC_DONE;
	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc--;
	return( OK );

    case OP_DOWN: 		/* decend into complex type */
	doc->depth++;
	break;

    case OP_UP: 		/* finish complex type */
	doc->depth--;
	break;

    case OP_DEBUG_TYPE:		/* trace the current type */
	TRdisplay( "%s== (%s, %d, %d)\n", tabs( doc->depth ),
		   gcx_look( gcx_datatype, (i4)doc->pc[0] ),
		   (i4)doc->pc[1], (i4)doc->pc[2] );
	doc->pc += 3;
	break;

    case OP_DEBUG_BUFS:		/* trace buffer into */
	TRdisplay( "%s++ iptr 0x%x olen %d\n", 
		   tabs( doc->depth ), src, (i4)(doc->dst2 - dst) );
	break;

    case OP_JZ:			/* pop top-of-stack; branch if zero */
	arg = *doc->pc++;
	if ( ! *(--doc->vsp) )  doc->pc += arg;
	break;

    case OP_DJNZ:		/* if !top-of-stack pop, else decr & branch */
	arg = *doc->pc++;
	if ( doc->vsp[-1]-- )
	    doc->pc += arg;
	else
	    --doc->vsp;
	break;

    case OP_JNINP:		/* branch if input depleted */
	arg = *doc->pc++;
	if ( doc->eod  &&  src >= doc->src2 )  doc->pc += arg;
	break;

    case OP_GOTO: 		/* branch unconditionally */
	arg = *doc->pc++;
	doc->pc += arg;
	break;

    case OP_CALL_DDT:		/* call DBMS datatype program */
	arg = *doc->pc++;

	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pde internal error: DDT call stack overflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc -= 2;
	    return( E_GC0055_PDE_ERROR );
	}

	*doc->csp++ = doc->pc;
	doc->pc = gco_ddt_map[ arg ];
	doc->depth++;
	break;

    case OP_CALL_DV: 		/* call data value program */
	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pde internal error: DV call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	ela.tpl.length = *(--doc->vsp);
	ela.tpl.precision = *(--doc->vsp);
	ela.tpl.type = *(--doc->vsp);

	gco_comp_dv( &ela, doc->od_dv_prog );
	*doc->csp++ = doc->pc;
	doc->pc = doc->od_dv_prog;
	doc->depth++;
	break;

    case OP_CALL_TUP:		/* call tuple program */
	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pde internal error: TUP call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	*doc->csp++ = doc->pc;
	doc->pc = doc->tod_prog ? (GCO_OP *)doc->tod_prog : gco_tuple_prog;
	doc->depth++;
	break;

    case OP_END_TUP:		/* tuple processing completed */
        break;

    case OP_RETURN:		/* return from program call */
	if ( doc->csp <= &doc->call_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("pde internal error: RETURN call stack underflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	doc->pc = *(--doc->csp);
	doc->depth--;
	break;

    case OP_PUSH_LIT:		/* push 2 byte signed quanity */
	*doc->vsp++ = (u_i2) *doc->pc++;
	break;

    case OP_PUSH_I2:		/* push i2 from data stream */
    case OP_PUSH_REFI2:		/* push i2 & reference from output stream */
	MECOPY_CONST_MACRO( src - sizeof(t_ui2), sizeof(t_ui2), (PTR)&t_ui2 );
	*doc->vsp++ = t_ui2;
	break;

    case OP_PUSH_I4:		/* push i4 from data stream */
    case OP_PUSH_REFI4:		/* push i4 & reference from output stream */
	MECOPY_CONST_MACRO( src - sizeof(t_i4), sizeof(t_i4), (PTR)&t_i4 );
	*doc->vsp++ = t_i4;
	break;

    case OP_UPD_REFI2:		/* update i2 reference in output stream */
    case OP_UPD_REFI4:		/* update i4 reference in output stream */
	/* Reference changes not needed! */
    	break;

    case OP_PUSH_VCH:		/* compute varchar padding length */
        /*
        ** Adjust lengths for processing data and padding.
	** The array element size is provided as argument.
        ** The stack contains the number of elements from
        ** the embedded length field just processed and 
        ** the maximum length of the object (includes the
        ** embedded length field which is removed).
        */
        arg = *doc->pc++;			/* Size of elements */
        t_ui2 = *(--doc->vsp);			/* Number of elements */
        t_i4 = *(--doc->vsp) - sizeof( t_ui2 );	/* Max data length (bytes) */  

	/*
	** Length can be bogus on nullable variable length types.
	** Adjust if greater than max length of varchar.  Also
	** fix the output stream just to be a little bit safer.
	*/
	if ( (t_ui2 * arg) > (u_i2)t_i4 )
	{
	    t_ui2 = 0;
	    MECOPY_CONST_MACRO( (PTR)&t_ui2,
				sizeof(t_ui2), (PTR)(dst - sizeof(t_ui2)) );
	}

	*doc->vsp++ = t_i4 - (t_ui2 * arg);	/* length of pad (bytes) */
	*doc->vsp++ = t_ui2;			/* Number of elements */
	break;

    case OP_CHAR_LIMIT:		/* character length is limited (0 pad) */
	/*
	** The stack contains the number of elements 
	** to be processed.  For limited character
	** values, the padding length (which in this
	** case is 0), must also be on the stack
	** below the length.
	*/
	t_ui2 = *(--doc->vsp);			/* Number of elements */
	*doc->vsp++ = 0;			/* length of pad (bytes) */
	*doc->vsp++ = t_ui2;			/* Number of elements */
    	break;

    case OP_ADJUST_PAD:		/* adjust padding for data length change */
	/* Reference changes not needed! */
    	break;

    case OP_POP:		/* discard top of stack */
    	doc->vsp--;
	break;

    case OP_MSG_BEGIN:		/* note beginning of message */
	/* 
	** Beginning of a message: ensure the source contains the fixed 
	** portion of the message. 
	**
	** arg1 = message number for tracing
	** arg2 = size of fixed part of message structure 
	*/
	arg = *(doc->pc)++;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ message %s %d bytes ilen %d\n", 
		       tabs( doc->depth ), gcx_look( gcx_gca_msg, (i4)arg ), 
		       (i4)*doc->pc, (i4)(doc->src2 - src) );
# endif /* GCXDEBUG */

	/*
	** If there is not sufficient data for the fixed
	** portion of the message, call OP_REWIND to
	** signal the error.
	*/
	if ( src + *(doc->pc++) > doc->src2 )  
	    doc->pc = &op_rewind[0];
	break;

    case OP_IMP_BEGIN:		/* note beginning of GCA_VARIMPAR */
	/* 
	** Beginning of a VARIMPAR array: the remainder of the input should
	** be a multiple of the substructure size. Since a previous call to
	** JNINP verified that more remains in this message, we now only
	** check that the remaining input size is at least big enough to
	** provide one element of the substructure.  Anything less is an
	** error.
	**
	** arg1 = size of substructure 
	*/
	arg = *(doc->pc++);

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ impar %d bytes ilen %d\n", 
		       tabs( doc->depth ), (i4)arg, (i4)(doc->src2 - src) );
# endif /* GCXDEBUG */

	/*
	** If we have exhausted the input buffer, we need more 
	** input.  Otherwise, if there is insufficient input for 
	** another element, we can't continue.  The following test 
	** and OP_REWIND operation handles both conditions.
	*/
	if ( src >= doc->src2  ||  src + arg > doc->src2 )
	    doc->pc = &op_rewind[0];
	break;

    case OP_COPY_MSG:		/* Copy remaining input to output */
	/*
	** Copy as much of the array to the output
	** buffer as possible.  If there isn't at
	** least one element in the input buffer,
	** rewind to get more input.  Otherwise, the
	** conversion routines will request more
	** output as needed.
	*/
	arg = *(doc->pc)++;	/* Size of array element */

	if ( (src + arg) > doc->src2 )
	    doc->pc = &op_rewind[0];			/* Get more input */
	else
	    *doc->vsp++ = (doc->src2 - src) / arg;	/* Number of elements */
	break;

    case OP_CHAR_MSG:		/* Copy character input to output */
	/*
	** Copy as much of the array to the output
	** buffer as possible.  If there isn't at
	** least one element in the input buffer,
	** rewind to get more input.  Otherwise, the
	** conversion routines will request more
	** output as needed.
	*/
	if ( src >= doc->src2 )
	    doc->pc = &op_rewind[0];			/* Get more input */
	else
	    *doc->vsp++ = doc->src2 - src;		/* Number of elements */
	break;

    case OP_SAVE_MSG:		/* Save input */
    case OP_RSTR_MSG:		/* Restore input */
        break;

    case OP_VAR_BEGIN:		/* note beginning of GCA_VAREXPAR */
	/* 
	** Beginning of VAREXPAR array: save the current src pointer on
	** the stack, and follow the pointer at the current src to the
	** actual array of elements.  We don't care about the size of
	** the array (we just hope the caller provided a pointer to real
	** data), so we ignore the size and alignment arguments.
	**
	** arg1 = size of substructure
	** arg2 = alignment type
	** arg3 = alignment length 
	*/

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ expar %d els * %d bytes\n", 
		       tabs( doc->depth ), doc->vsp[-1], (i4)doc->pc[0] );
# endif /* GCXDEBUG */

	if ( doc->psp >= &doc->ptr_stack[ DOC_PTR_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pde internal error: VAR psp stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	doc->pc += 3;
	*(doc->psp++) = src;
	src = *((char **)src);
	break;

    case OP_VAR_END:		/* note end of GCA_VAREXPAR */
	/*
	** Pop the original source and step
	** over the pointer placed there.
	*/
	if ( doc->psp <= &doc->ptr_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pde internal error: VAR psp stack underflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	src = *(--doc->psp) + sizeof(char *);
	break;

    case OP_DV_BEGIN:		/* note beginning of GCA_DATA_VALUE value */
	/* 
	** Beginning of GCA_DATA_VALUE value: save the current src pointer 
	** on the stack, and follow the pointer at the current src to the
	** actual array of elements.  We don't care about the size of
	** the array (we just hope the caller provided a pointer to real
	** data), so we ignore the size and alignment arguments.
	**
	** arg1 = length
	*/

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )
	    TRdisplay( "%s++ data value %d bytes\n", 
		       tabs( doc->depth ), (i4)*doc->pc );
# endif /* GCXDEBUG */

	if ( doc->psp >= &doc->ptr_stack[ DOC_PTR_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "pde internal error: DV psp stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC0055_PDE_ERROR );
	}

	doc->pc++;
	*(doc->psp++) = src;
	src = *((char **)src);
	break;

    case OP_DV_END:		/* note end of GCA_DATA_VALUE value */
	/* 
	** End of GCA_DATA_VALUE value: pop the src pointer back.
	**
	** BLOB segments are sent one GCA request at a time.  The
	** pointer stack is flushed upon re-entry after processing
	** the first segment.  If the pointer stack is empty, a
	** BLOB has been processed and the src pointer restoration
	** cannot be done.  But we can't just continue expecting
	** there to be a GCA_DATA_VALUE structure following the
	** BLOB segment, so we BREAK at the end of the BLOB, as was
	** done for the intermediate segments, and upon re-entry
	** will begin processing the rest of the GCA message.
	*/
	if ( doc->psp > doc->ptr_stack )
	{
	    /*
	    ** Pop the original source and step
	    ** over the pointer placed there.
	    */
	    src = *(--doc->psp) + sizeof(char *);
	}
	else
	{
	    /*
	    ** Return for more input.
	    */
	    doc->state = DOC_MOREIN;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( OK );
	}
	break;

    case OP_DV_ADJUST:		/* Adjust buffer space for GCA_DATA_VALUE */
	/*
	** During standard processing we assume the caller has
	** provided all the data needed (since there is no way
	** for us to check).  During list processing, trailing
	** list objects are read directly from the input buffer,
	** rather than indirectly through a pointer, so we can
	** check to make sure there is enough data.  We are
	** taking our input directly from the input buffer if
	** there is no saved input pointer on the stack.
	*/
	if ( doc->psp <= doc->ptr_stack )
	{
	    /*
	    ** The object length is on the data stack.
	    ** Make sure there is sufficient data in the
	    ** input buffer for the object.  Rewind to 
	    ** last valid location and return an error 
	    ** if not enough room.
	    **
	    ** TODO: length is in elements not bytes.
	    ** This calculation is not correct for
	    ** all types but for now we don't expect
	    ** to hit this condition anyway.
	    */
	    if ( (src + doc->vsp[-1]) > doc->src2 )
	    {
		/*
		** Not enough input for the object.  To
		** ensure an error, make sure src < src2.
		*/
		if ( src >= doc->src2 )  src = doc->src2 - 1;
		doc->pc = &op_rewind[0];
	    }
	}
	break;

    case OP_DV_REF:		/* save meta-data length reference */
    case OP_SEG_BEGIN:		/* note beginning of list segments */
    case OP_SEG_END:		/* note end of list segments */
    	break;

    case OP_SEG_CHECK:		/* check for end-of-segments */
	arg = *doc->pc++;
    	break;

    case OP_MARK:		/* save current processing state */
	/*
	** Save our current location and processing
	** state.  Buffer overflow conditions can use
	** the OP_REWIND operation to return to this
	** point.
	*/
	doc->mark.pc = doc->pc - 1;
	doc->mark.csp = doc->csp;
	doc->mark.vsp = doc->vsp;
	doc->mark.psp = doc->psp;
	doc->mark.depth = doc->depth;
	doc->mark.src1 = src;
	doc->mark.dst1 = dst;
	break;

    case OP_REWIND:		/* backup to saved processing state */
	if ( ! doc->mark.pc )
	{
	    /*
	    ** No saved processing state to restore.
	    */
	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    doc->state = DOC_DONE;
	    status = E_GC0055_PDE_ERROR;
	}
	else
	{
	    /*
	    ** Restore saved processing state.
	    */
	    doc->pc = doc->mark.pc;
	    doc->csp = doc->mark.csp;
	    doc->vsp = doc->mark.vsp;
	    doc->psp = doc->mark.psp;
	    doc->depth = doc->mark.depth;
	    src = doc->mark.src1;
	    dst = doc->mark.dst1;

	    /*
	    ** If we have consumed the input (src >= src2), need
	    ** more input.  Otherwise (src < src2), we must have
	    ** run into a input data length problem.
	    */
	    doc->state = DOC_MOREIN;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    status = (src < doc->src2) ? E_GC0056_PDE_BADSIZE : OK;
	}
	return( status );

    case OP_PAD_ALIGN:		/* alignment pad for structures */
	src += *doc->pc++;
	break;

    case OP_DIVIDE:		/* Divide stack by argument */
	arg = *doc->pc++;				/* Divisor */
	t_i4 = *(--doc->vsp);			/* Dividend */
	*doc->vsp++ = (arg < 2) ? t_i4 : (t_i4 / arg);
	break;

    case OP_BREAK:		/* end of data segment */
	/*
	** Return for more input.
	*/
	doc->state = DOC_MOREIN;
	doc->src1 = src;
	doc->dst1 = dst;
	return( OK );

    /*
    ** Start of conversion operations.
    **
    ** Since we are encoding a data structure, we assume the
    ** caller has provided sufficient input.  OP_MSG_BEGIN and
    ** OP_IMP_BEGIN also verify the input length.  We may not
    ** have room in the output buffer for all elements of an
    ** array.  We first convert what will fit in the output
    ** buffer.  If there are remaining elements to convert,
    ** we request more output space and then convert the
    ** remaining elements.
    */

    case OP_CV_I1:   item_size = sizeof( i1 );	pop = TRUE;	goto cv;
    case OP_CV_I2:   item_size = sizeof( i2 );	pop = TRUE;	goto cv;
    case OP_CV_I4:   item_size = sizeof( i4 );	pop = TRUE;	goto cv;
    case OP_CV_I8:   item_size = sizeof( i8 );	pop = TRUE;	goto cv;
    case OP_CV_F4:   item_size = sizeof( f4 );	pop = TRUE;	goto cv;
    case OP_CV_F8:   item_size = sizeof( f8 );	pop = TRUE;	goto cv;
    case OP_CV_CHAR: item_size = sizeof(char);	pop = FALSE;	goto cv; 
    case OP_CV_BYTE: 
    case OP_CV_PAD:  item_size = 1;		pop = TRUE;	goto cv;

    case OP_PAD:		/* output padding */
    case OP_SKIP_INPUT:		/* Skip remaining input */
    case OP_SAVE_CHAR:		/* Save character input */
    	/*
	** OP_CV_CHAR processes all input, so there is 
	** nothing to do except discard the input length 
	** left on stack.
	*/
    	doc->vsp--;
	break;

    case OP_RSTR_CHAR:		/* Restore character input */
	break;

    cv:
	/*
	** Limit vector count to the output buffer size.
	** OP_MSG_BEGIN and OP_IMP_BEGIN made sure there
	** was enough input data for the object being
	** processed.  For VAREXPAR arrays, the caller
	** must provide all the input since we have no
	** way to check the actual length provided.
	*/
	i = doc->vsp[-1];
	VEC_MACRO( i, doc->dst2 - dst, item_size );
	doc->vsp[-1] -= i;
	item_size *= i;

	MEcopy( src, item_size, dst );
	src += item_size;
	dst += item_size;

	if ( doc->vsp[-1] )	/* More elements to convert? */
	{
	    /*
	    ** Need more output.  Reset DOC to convert
	    ** remaining elements.
	    */
	    doc->state = DOC_MOREOUT;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( OK );
	}

	/* 
	** All items converted - pop vector count 
	*/
	if ( pop )  doc->vsp--;
	break;
    }

    goto top;
}

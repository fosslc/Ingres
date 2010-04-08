/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <gcccl.h>
#include    <cm.h>
#include    <cv.h>		 
#include    <cvgcc.h>
#include    <me.h>
#include    <qu.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcocomp.h>
#include    <gcoint.h>
#include    <gcc.h>
#include    <gccer.h>
#include    <gccpl.h>
#include    <gcxdebug.h>

/*
**
**  Name: GCOPDC.C - GCF Comm Server Presentation Data Conversion module
**
**  External entry points:
**	gco_convert		Convert messages using compiled descriptor
**	gco_xlt_ident		Indentity charset processing.
**	gco_xlt_sb		Single-byte charset processing.
**	gco_xlt_db		Double-byte charset processing.
**
**  History:
**      12-Feb-90 (seiwald)
**	    Collected from gccpl.c and gccutil.c.
**	16-Feb-90 (seiwald)
**	    Rebuilt upon CV conversion macros.
**	    Tuned somewhat.
**	    Modified gcc_pl_lod (load object descriptor) to compress
**	    runs of the same single element into an array.  In certain
**	    cases, this speeds up large data retrieves significantly.
**	15-Mar-90 (seiwald)
**	    Reworked gcc_adf_init to allocate properly sized od_ptr
**	    vectors for each triad_od.  Removed silly [1] subscript
**	    for the od array pointer in the GCA_TRIAD_OD.
**	15-Mar-90 (seiwald)
**	    Pass upper range bytes (128-255) through directly.
**	    This allows Kanji to indentify itself as ASCII (yuk).
**      23-Mar-90 (seiwald)
**          Byte align patch for gcc_pl_lod.
**	27-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	20-May-91 (seiwald)
**	    Changes for character set negotiation: removed gcc_load_av,
**	    gcc_chk_av, and cc_ctbl_init, which have been replaced by
**	    newer routines in gcccset.c.  Made perform_conversion and
**	    its underlings refer to the new per-connection character 
**	    set transliteration table.
**	17-aug-91 (leighb) DeskTop Porting Changes:
**	    added include of cv.h;
**	    corrected func decls & defs for gcc_save_int(), gcc_ld_od(), 
**	    and gcc_adf_init();
**	17-Jun-91 (seiwald)
**	    A little non-functional regrooving:
**	    -	Removed complex_type() and triad_od() and stuffed their
**	    	contents into gcc_ld_od().  
**	    -	Removed gcc_save_int() and stuffed its contents into 
**	    	perform_conversion().
**	    -   Removed queue of DOC_EL's and just put an array of them
**	        into a new DOC_STACK; extracted elements not specific to
**		each DOC_EL into the DOC_STACK.
**	    -	Spruced up tracing to be more compact.
**  	13-Aug-91 (cmorris)
**  	    Added include of tm.h.
**	14-Aug-91 (seiwald) bug #39172
**	    Gcc_ld_od returns a failure status for 0 length char types,
**	    as well as other bogus TPL's.  Gcc_pl_lod can now return a
**	    failure status.
**	14-Aug-91 (seiwald)
**	    Linted.
**	26-Dec-91 (seiwald)
**	    Gcc_conv_vec now zeros the PAD on inbound flow.
**	14-Aug-92 (seiwald)
**	    Support for compiled descriptors: message descriptors, tuple 
**	    descriptors, and GCA_DATA_VALUES are now compiled into simple 
**	    programs interpreted by perform_conversion().
**	14-Aug-92 (seiwald)
**	   Support for blobs, by way of a new array status "GCA_VARSEGAR" 
**	   to GCA_ELEMENT.  This array status indicates that the element 
**	   may be repeated indefinitely, as controlled by the previous 
**	   element.
**	20-Aug-92 (seiwald)
**	   New interface to adf decompose functions, adc_gcc_decompose,
**	   which simple returns a list of GCA_ELEMENT's that describe
**	   all the datatypes.  OP_ERROR now takes an argument for more
**	   descriptive tracing.
**	10-Sep-92 (seiwald)
**	   ANSI fixes: cast signed quantity to unsigned before comparing
**	   against unsigned (they are both semantically unsigned); rework
**	   tabs using a macro, rather than trying to add 8 to a static
**	   pointer; removed suspected trigraph.
**	26-sep-92 (seiwald)
**	    Moved fragment handling from perform_conversion to gcc_pdc_down.
**	28-Oct-92 (seiwald)
**	    Recoded DO_16SRC after assuming that mappings don't exist
**	    between single and double byte characters; fixed vector count
**	    adjustment in DO_16SRC to represent the actual number of source
**	    bytes used.
**	4-Dec-92 (seiwald) bug #47311
**	    Allow GC_QTXT_TYPE of 0 length for STAR generated queries.
**	16-Dec-92 (seiwald)
**	    GCA_VARSEGAR (blob types) tested and fixed.
**	5-Feb-93 (seiwald)
**	    Moved allocation of GCC_DOC up into caller of perform_conversion().
**	    Removed CCB's and MDE's from perform_conversion's interface: it
**	    just deals with buffers now.
**  	10-Feb-93 (cmorris)
**  	    Removed status from GCnadds and GCngets:- never used.
**	17-Apr-93 (seiwald)
**	    Another crack at doublebyte support: sometimes character data 
**	    coming across the wire is garbage.  It is possible for the last
**	    byte of a character field to look like the first byte of a 
**	    doublebyte character, and DO_16SRC has now been modified to
**	    limit itself to a single byte conversion if the vector count
**	    indicates only one byte remaining.  Embarrassingly enough, it
**	    is the GCA_OBJECT_DESC, generated by GCA_SEND, that contains a
**	    garbage character field - the unfilled data_object_name.
**	25-May-93 (seiwald)
**	    Moved non-conversion code out to gcccomp.c.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-Sep-93 (brucek)
**	    Cast args to MECOPY_CONST_MACRO to PTRs.
**	11-Jan-94 (mikehan)
**	    For doublebyte support: - perform_conversion() - assign the
**	    value of doc->sp[-1] to i as well before do DO_16SRC when
**	    it is OUTFLOW. For one more time, this will fix the 
**	    'No Connection' problem if there is charset conversion involved,
**	    notably that of Kanjieuc<-> Shiftjis.
**	1-Mar-94 (seiwald)
**	    Added dummy handling of new OP_VAR_BEGIN and OP_VAR_END
**	    op codes, which aren't needed by perform_conversion.
**	14-Mar-94 (seiwald)
**	    Extracted from gccpdc.c.
**	24-mar-95 (peeje01)
**	    Integrate doublebyte changes from 6500db_su4_us42
**	29-Mar-94 (seiwald)
**	    Added support for VARIMPAR in message encoding.
**	4-Apr-94 (seiwald)
**	    Spruce up tracing with OP_DEBUG_TYPE and OP_DEBUG_BUFS.
**	4-Apr-94 (seiwald)
**	    New OP_DV_BEGIN/END.
**	4-Apr-94 (seiwald)
**	    OP_MSG_BEGIN now includes the message type for tracing.
**	04-May-94 (cmorris)
**	    Bump level at which plethora of trace output appears to 3.
**	 1-May-95 (gordy)
**	    Added alignment type and length to OP_VAR_BEGIN arguments.
**	25-May-95 (gordy)
**	    Add entries for OP_BREAK and OP_MSG_RESTART which are ignored.
**	31-Aug-95 (gordy)
**	    Fix bad varchar lengths for null varchar's in OP_PUSH_VCH.
**	29-Nov-95 (gordy)
**	    Adjust previous fix.  Some 6.4 versions have bad lengths
**	    but still treat the data area as padding.  Replicating
**	    this behavior seems to work for everyone.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 1-Mar-96 (gordy)
**	    General clean-up to reconcile gcd_convert(), gcd_encode()
**	    and gcd_decode().
**	 3-Apr-96 (gordy)
**	    Removed OP_JNOVR and added OP_CHKLEN, OP_DV_ADJUST.  Added
**	    casts to tracing.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG for messages which are not formatted into 
**	    elements.  The entire message is copied from input to output.
**	 5-Feb-97 (gordy)
**	    Added OP_CALL_DDT for pre-compiled DBMS datatype processing.
**	    OP_PUSH_VCH is now passed both the max and actual data lengths
**	    on the data stack.  OP_COPY_MSG now expects the element size
**	    as an argument.  Removed OP_CHKLEN, OP_COMP_PAD.
**      31-Mar-98 (gordy)
**          Include long varchar in doublebyte special string handling.
**	03-apr-1998 (canor01)
**	    The padding information needs to be skipped on two other
**	    conditions--INFLOW and truncation.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-June-2001 (wansh01) 
**          Changed gcx_level to gcd_trace_level to seperate the gcc and gcd trace
**	22-Mar-02 (gordy)
**	    More nat to i4 cleanup.  Removed unneeded atomic types.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Made sure current processing state is returned in all instances.
**	    VEC_MACRO now returns an indication that limit was exceeded.
**	    DOC_DONE (complete) now distinct from DOC_IDLE (initialize).
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**	24-Sep-04 (gordy)
**	    Fixing a problem with character translation caused by changing
**	    u_char * types to char *.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Renamed gcx_gcc_ops to gcx_gco_ops, as this referenced outside
**          of GCC.
**	 4-Jan-5 (gordy)
**	    Use unsigned char's in gco_do_16sr csince they used as 
**	    indices in the translation arrays.
**	30-Nov-05 (gordy)
**	    Fixed global declarations.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code for multi-byte charsets.
*/

# define VEC_MACRO( vec, limit, sz ) \
	((((limit)/(sz)) < (vec)) ? (((vec) = (limit)/(sz)),TRUE) : FALSE)

GLOBALREF GCXLIST gcx_gco_ops[];
GLOBALREF GCXLIST gcx_gcc_eop[];



/*
** Name: gco_convert - convert messages using compiled descriptor
**
** Description:
**	Translates all or part of a GCA message from local to network
**	or network to local transfer syntax, using simple programs
**	produced by compiling message descriptors.
**
**	Returns successfully if input data depleted or output data area
**	full, expecting to be called again.  May return with input data
**	still present if input data contains only a fragment of an
**	atomic datatype.
**
** Input:
**	doc		GCO_DOC (state info for conversion)
**	    state		DOC_IDLE on first call
**	    src1		start of input data
**	    src2		end of input data 
**	    dst1		start of output buffer
**	    dst2		end of output buffer
**	    message_type	GCA message type
**	    eod 		this is the last segment of a message
**	    tod_prog 		program for GCA_TUPLE data
**	inout		INFLOW = convert to local; OUTFLOW = convert to network
**	xlt		Character set transliteration table
**
** Output:
**	doc->src1	unused part of input buffer
**	doc->src2	unused part of output buffer
**	doc->state	Processing state:
**				DOC_MOREIN - more input required
**				DOC_MOREOUT - more output required
**				DOC_MOREQOUT - more out req (queue current)
**				DOC_FLUSHQOUT - flush queued output
**				DOC_DONE - conversion complete
**
** Returns:
**	STATUS		OK or error code.
**
** History
**	?-?-88 (berrow)
**	    Written.
**	24-may-89 (seiwald)
**	    Revamped and made sane.
**	06-jun-89 (seiwald)
**	    Don't consider abrupt end of message fatal.  Just convert
**	    what data there is and leave the error control to the clients.
**	08-jun-89 (seiwald)
**	    When checking for abrupt end of message, check for fragment.
**	14-Aug-92 (seiwald)
**	    Rewritten to use compiled descriptors.
**  	11-Dec-92 (cmorris)
**  	    Renamed the confusingly named "mop2" argument to "mopend".
**  	    This is consistent with the naming of the p... variables in
**  	    an MDE:- p2 refers to the end of the _data_ whereas "pend" 
**  	    refers to the end of the available buffer for data. The latter
**  	    is what is implied by this argument.
**	5-Feb-93 (seiwald)
**	    Interface rid of references to MDE's or CCB's.
**	 1-May-95 (gordy)
**	    Added alignment type and length to OP_VAR_BEGIN arguments.
**	31-Aug-95 (gordy)
**	    Fix bad varchar lengths for null varchar's in OP_PUSH_VCH.
**	29-Nov-95 (gordy)
**	    Adjust previous fix.  Some 6.4 versions have bad lengths
**	    but still treat the data area as padding.  Replicating
**	    this behavior seems to work for everyone.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 1-Mar-96 (gordy)
**	    General clean-up to reconcile gcd_convert(), gcd_encode()
**	    and gcd_decode().
**	 3-Apr-96 (gordy)
**	    Removed OP_JNOVR and added OP_CHKLEN, OP_DV_ADJUST.  Added
**	    casts to tracing.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG for messages which are not formatted into 
**	    elements.  The entire message is copied from input to output.
**	 5-Feb-97 (gordy)
**	    Added OP_CALL_DDT for pre-compiled DBMS datatype processing.
**	    OP_PUSH_VCH is now passed both the max and actual data lengths
**	    on the data stack.  OP_COPY_MSG now expects the element size
**	    as an argument.  Removed OP_CHKLEN, OP_COMP_PAD.
**      31-Mar-98 (gordy)
**          Include long varchar in doublebyte special string handling.
**	 1-Mar-01 (gordy)
**	    OP_PUSH_VCH now takes argument which is the size of the array
**	    elements (to support Unicode).  New OP_DIVIDE operation.
**	12-May-03 (gordy)
**	    Made sure current processing state is returned in all instances.
**	    DOC_DONE (complete) now distinct from DOC_IDLE (initialize).
**	    May be called in DOC_DONE state (will return with no processing
**	    since already complete) but DOC_IDLE must be used for new message.
**	    OP_JNINP now returns DOC_MOREIN for case where input exhausted 
**	    but message continued (processing resumes with OP_JNINP).
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**	24-Sep-04 (gordy)
**	    A previous change made many of the binary pointers to be
**	    char * to match GCO_DOC definitions rather than u_char *.
**	    This causes problems when using the pointer to do character
**	    translation using the character as an index.  Signed chars
**	    will produce an invalid index when characters above 0x7F
**	    are translated.  All binary pointers should actually be
**	    of type u_i1 *, but for now just casting the x2y[] indexes.
**	08-Mar-07 (gordy, gupsh01)
**	    Fixed the case OP_PUSH_LIT, to correctly obtain the values.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code for multi-byte charsets.
**	    Check various value stacks for over/under flow.  Add new ops
**	    for updating string meta-data lengths.
*/

STATUS
gco_convert( GCO_DOC *doc, i4  inout, GCO_CSET_XLT *xlt )
{
    i4		i;			/* for counting converted items */
    GCO_OP	arg;			/* arguments in the program */
    GCA_ELEMENT	ela;			/* to hold GCA_DATA_VALUES */
    STATUS	status;			/* for conversion status */
    i4		t_i4;			/* for PUSH_I4 */
    u_i2	t_ui2;			/* for PUSH_I2 */
    char	*s;			/* for PUSH_* */
    char	*src = doc->src1;
    char	*dst = doc->dst1;
    i4		src_limit = doc->src2 - src;
    i4		dst_limit = doc->dst2 - dst;

# ifdef GCXDEBUG
    static GCXLIST gcx_doc_state[] =
    {
	{ DOC_IDLE,	"IDLE" },
	{ DOC_DONE,	"DONE" },
	{ DOC_MOREIN,	"INPUT" },
	{ DOC_MOREOUT,	"OUTPUT" },
	{ DOC_MOREQOUT,	"Q-OUTPUT" },
	{ DOC_FLUSHQOUT,"FLUSH-Q" },
	{ 0, NULL },
    };

    static GCXLIST gcx_gco_flow[] =
    {
	{ INFLOW,	"IN" },
	{ OUTFLOW,	"OUT" },
	{ 0, NULL }
    };

    if ( gco_trace_level >= 2 )
    {
	TRdisplay( "GCO convert: flow = %s, msg = %s, eod = %s\n",
		    gcx_look( gcx_gco_flow, inout ), 
		    gcx_look( gcx_gca_msg, doc->message_type ),
		    doc->eod ? "TRUE" : "FALSE" );

    	if ( gco_trace_level >= 4 )
    	    TRdisplay( "           : src-len = %d, dst-len = %d, save = %d, frag = %d\n",
	    	       src_limit, dst_limit, doc->text_len, doc->frag_bytes );
    }
# endif /* GCXDEBUG */

    /* 
    ** If fresh message, load program according to message type 
    */
    if ( doc->state == DOC_IDLE )
	gco_init_doc( doc, doc->message_type );
    else
    {
	doc->state = DOC_IDLE;	/* clear prior state */

	if ( doc->frag_bytes )
	{
	    /* 
	    ** Pre-pend atomic fragment left over 
	    ** from previous message segment.
	    */
	    src -= doc->frag_bytes;
	    src_limit = doc->src2 - src;
	    MEcopy( (PTR)doc->frag_buffer, doc->frag_bytes, (PTR)src );
	    doc->frag_bytes = 0;
	}
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
		TRdisplay("GCO internal error: value stack under/over flow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( E_GC2410_PERF_CONV );
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
	** The following operations are NO-OPs for het-net
	** processing and do not need to be traced.
	*/
	case OP_MSG_BEGIN:
	case OP_IMP_BEGIN:
	case OP_VAR_BEGIN:
	case OP_VAR_END:
	case OP_DV_ADJUST:
	case OP_MARK:
	case OP_REWIND:
	case OP_PAD_ALIGN:
	case OP_BREAK:
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
	    TRdisplay( "GCO Error: %s\n", gcx_look( gcx_gcc_eop, (i4)arg ) );
# endif /* GCXDEBUG */

	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc -= 2;
	return( E_GC2410_PERF_CONV );

    case OP_DONE:		/* complete msg converted */
	doc->src1 = src;
	doc->dst1 = dst;

	/*
	** Value stack should be empty at this point.
	*/
	if ( doc->vsp != &doc->val_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("GCO internal error: value stack not empty\n");
# endif /* GCXDEBUG */

	    return( E_GC2410_PERF_CONV );
	}

# ifdef GCXDEBUG
	if ( gco_trace_level >= 2 )
	    TRdisplay( "GCO convert: state = %s\n",
			gcx_look( gcx_doc_state, DOC_DONE ) );
# endif /* GCXDEBUG */

	doc->state = DOC_DONE;
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
	TRdisplay( "%s++ ilen %d olen %d\n", 
		   tabs( doc->depth ), src_limit, dst_limit );
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
	arg = *doc->pc++;		/* branch offset */

	if ( ! src_limit )		/* input buffer empty */
	    if ( doc->eod )		/* message end-of-data */
		doc->pc += arg;		/* branch */
	    else
	    {
		/*
		** The input buffer is empty but the end-of-data
		** flag is not set: request more input.  There
		** may be no additional data available, just
		** the end-of-data flag on an empty message
		** segment, so reset the PC to redo the test.
		*/
# ifdef GCXDEBUG
		if ( gco_trace_level >= 2 )
		    TRdisplay( "GCO convert: state = %s\n",
				gcx_look( gcx_doc_state, DOC_MOREIN ) );
# endif /* GCXDEBUG */

		doc->state = DOC_MOREIN;
		doc->src1 = src;
		doc->dst1 = dst;
		doc->pc -= 2;
		return( OK );
	    }
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
		TRdisplay("GCO internal error: DDT call stack overflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc -= 2;
	    return( E_GC2410_PERF_CONV );
	}

	*doc->csp++ = doc->pc;
	doc->pc = (GCO_OP *)gco_ddt_map[ arg ];
	doc->depth++;
	break;

    case OP_CALL_DV: 		/* call data value program */
	if ( doc->csp >= &doc->call_stack[ DOC_CALL_DEPTH ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay( "GCO internal error: DV call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC2410_PERF_CONV );
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
		TRdisplay( "GCO internal error: TUP call stack overflow\n" );
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC2410_PERF_CONV );
	}

	*doc->csp++ = doc->pc;
	doc->pc = doc->tod_prog ? (GCO_OP *)doc->tod_prog : gco_tuple_prog;
	doc->depth++;
	doc->flags |= DOC_TUP_FLG;
	break;

    case OP_END_TUP:		/* tuple processing completed */
	doc->flags &= ~DOC_TUP_FLG;
	break;

    case OP_RETURN:		/* return from program call */
	if ( doc->csp <= &doc->call_stack[ 0 ] )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 1 )
		TRdisplay("GCO internal error: RETURN call stack underflow\n");
# endif /* GCXDEBUG */

	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( E_GC2410_PERF_CONV );
	}

	doc->pc = *(--doc->csp);
	doc->depth--;
	break;

    case OP_PUSH_LIT:		/* push 2 byte signed quanity */
	*doc->vsp++ = (u_i2)*doc->pc++;
	break;

    case OP_PUSH_REFI2:		/* push i2 & reference from output stream */
	/*
	** Save reference to output value and initialize change tracking.
	*/
	doc->ref = dst - ((inout == INFLOW) ? CV_L_I2_SZ : CV_N_I2_SZ);
	doc->flags |= DOC_REF_FLG;
	doc->delta = 0;	

    	/*
	** Fall through to push value.
	*/

    case OP_PUSH_I2:		/* push i2 from data stream */
	/*
	** Retrieve value from input or output (which ever is platform format).
	*/
	s = (inout == INFLOW) ? dst : src;
	MECOPY_CONST_MACRO( s - CV_L_I2_SZ, CV_L_I2_SZ, (PTR)&t_ui2 );
	*doc->vsp++ = t_ui2;
	break;

    case OP_UPD_REFI2:		/* update i2 reference in output stream */
	/*
	** Only update if a change has occured and
	** the reference is still valid.
	*/
	if ( doc->flags & DOC_REF_FLG  &&  doc->delta )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 3 )
		TRdisplay("GCO update data length: delta = %d\n", doc->delta);
# endif /* GCXDEBUG */

	    if ( inout == INFLOW )
	    {
		MECOPY_CONST_MACRO( doc->ref, CV_L_I2_SZ, (PTR)&t_ui2 );
		t_ui2 += doc->delta;
		MECOPY_CONST_MACRO( (PTR)&t_ui2, CV_L_I2_SZ, doc->ref );
	    }
	    else
	    {
		CV2L_I2_MACRO( doc->ref, (PTR)&t_ui2, &status );
		t_ui2 += doc->delta;
		CV2N_I2_MACRO( (PTR)&t_ui2, doc->ref, &status );
	    }
	}

	/*
	** Clear the reference and flush queued output.
	*/
	doc->flags &= ~DOC_REF_FLG;
    	doc->ref = NULL;

	/*
	** Flush any queued output.  Do not flush
	** if DV reference is saved (flush will
	** occur when DV reference is updated).
	*/
	if ( doc->flags & DOC_OUTQ_FLG  &&  ! (doc->flags & DOC_DVREF_FLG) )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 2 )
		TRdisplay( "GCO convert: state = %s\n",
			    gcx_look( gcx_doc_state, DOC_FLUSHQOUT ) );
# endif /* GCXDEBUG */

	    doc->flags &= ~DOC_OUTQ_FLG;
	    doc->state = DOC_FLUSHQOUT;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( OK );
	}
	break;

    case OP_PUSH_REFI4:		/* push i4 & reference from output stream */
	/*
	** Save reference to output value and initialize change tracking.
	*/
	doc->ref = dst - ((inout == INFLOW) ? CV_L_I4_SZ : CV_N_I4_SZ);
	doc->flags |= DOC_REF_FLG;
	doc->delta = 0;	

    	/*
	** Fall through to push value.
	*/

    case OP_PUSH_I4:		/* push i4 from data stream */
	/*
	** Retrieve value from input or output (which ever is platform format).
	*/
	s = inout == INFLOW ? dst : src;
	MECOPY_CONST_MACRO( s - CV_L_I4_SZ, CV_L_I4_SZ, (PTR)&t_i4 );
	*doc->vsp++ = t_i4;
	break;

    case OP_UPD_REFI4:		/* update i4 reference in output stream */
	/*
	** Only update if a change has occured and
	** the reference is still valid.
	*/
	if ( doc->flags & DOC_REF_FLG  &&  doc->delta )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 3 )
		TRdisplay("GCO update data length: delta = %d\n", doc->delta);
# endif /* GCXDEBUG */

	    if ( inout == INFLOW )
	    {
		MECOPY_CONST_MACRO( doc->ref, CV_L_I4_SZ, (PTR)&t_i4 );
		t_i4 += doc->delta;
		MECOPY_CONST_MACRO( (PTR)&t_i4, CV_L_I4_SZ, doc->ref );
	    }
	    else
	    {
		CV2L_I4_MACRO( doc->ref, (PTR)&t_i4, &status );
		t_i4 += doc->delta;
		CV2N_I4_MACRO( (PTR)&t_i4, doc->ref, &status );
	    }
	}

	doc->flags &= ~DOC_REF_FLG;
    	doc->ref = NULL;

	/*
	** Flush any queued output.  Do not flush
	** if DV reference is saved (flush will
	** occur when DV reference is updated).
	*/
	if ( doc->flags & DOC_OUTQ_FLG  &&  ! (doc->flags & DOC_DVREF_FLG) )
	{
# ifdef GCXDEBUG
	    if ( gco_trace_level >= 2 )
		TRdisplay( "GCO convert: state = %s\n",
			    gcx_look( gcx_doc_state, DOC_FLUSHQOUT ) );
# endif /* GCXDEBUG */

	    doc->flags &= ~DOC_OUTQ_FLG;
	    doc->state = DOC_FLUSHQOUT;
	    doc->src1 = src;
	    doc->dst1 = dst;
	    return( OK );
	}
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
	t_i4 = *(--doc->vsp) - CV_L_I2_SZ;	/* Max data length (bytes) */

	/*
	** Length can be bogus on nullable variable length types.
	** Adjust if greater than max data length of varchar.  Also
	** fix the output stream just to be a little bit safer.
	*/
	if ( (t_ui2 * arg) > (u_i2)t_i4 )
	{
	    t_ui2 = 0;

	    if ( inout == INFLOW )
	    {
		MECOPY_CONST_MACRO( (PTR)&t_ui2, CV_L_I2_SZ, dst - CV_L_I2_SZ );
	    }
	    else
	    {
		CV2N_I2_MACRO( (PTR)&t_ui2, dst - CV_N_I2_SZ, &status );
	    }
	}

	*doc->vsp++ = t_i4 - (t_ui2 * arg);	/* length of pad (bytes) */
	*doc->vsp++ = t_ui2;			/* Number of elements */
	doc->flags |= DOC_LIMIT_FLG;
	doc->delta = 0;
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
	doc->flags |= DOC_LIMIT_FLG;
	doc->delta = 0;
    	break;

    case OP_ADJUST_PAD:		/* adjust padding for data length change */
	/*
	** NOTE: padding length (on stack) is in bytes while
	** delta is in elements.  Currently, only byte oriented 
	** data utilizes the change delta, so mixing the two is 
	** OK.  The element size will be required if ever delta 
	** is changed for non-byte data.
	*/

	/*
	** If data length decreased, increase padding length
	** accordingly (delta fully compensated by padding).  
	** Otherwise, padding is decreased but only to the 
	** extent that it is fully consumed (delta partially
	** compensated by padding, indicates data extension).
	*/
	if ( (i = min( doc->vsp[-1], doc->delta )) );
	{
	    /*
	    ** Outbound processing removes padding from the
	    ** data stream while inbound processing inserts
	    ** padding back into stream.  Outbound padding 
	    ** length is is not adjusted so that existing 
	    ** padding will be removed.  Inbound padding 
	    ** length is adjusted to compensate for change
	    ** in data length so that correct amount will
	    ** be inserted.  
	    */
	    if ( inout == INFLOW )   doc->vsp[-1] -= i;

	    /*
	    ** Data change delta is compensated by the 
	    ** (logical) change in padding length.
	    */
	    doc->delta -= i;
	}

	doc->flags &= ~DOC_LIMIT_FLG;
	break;

    case OP_POP:		/* discard top of stack */
    	doc->vsp--;
	break;

    case OP_MSG_BEGIN:		/* note beginning of message */
    	/* 
	** Beginning of a message: we do nothing.
	**
	** arg1 = message number for tracing
	** arg2 = size of fixed part of message structure 
	*/
	doc->pc += 2;
	break;

    case OP_IMP_BEGIN:		/* note beginning of GCA_VARIMPAR */
	/* 
	** Beginning of a VARIMPAR array: we do nothing.
	**
	** arg1 = size of substructure 
	*/
	doc->pc++;
	break;

    case OP_COPY_MSG:		/* Copy remaining input to output */
	/*
	** Copy as much of the message to the output 
	** buffer as possible.  Only process whole
	** atomic objects.
	*/
	arg = *(doc->pc)++;	/* Size of array element */
	*doc->vsp++ = (doc->src2 - src) / arg;
	break;

    case OP_CHAR_MSG:		/* Copy character input to output */
	/*
	** Set/clear flag indicating if additional text segments
	** are available.  
	*/
	if ( doc->eod )  
	    doc->flags &= ~DOC_SEG_FLG;
	else
	    doc->flags |= DOC_SEG_FLG;

	*doc->vsp++ = (doc->src2 - src);
	doc->delta = 0;	
	break;

    case OP_SAVE_MSG:		/* Save input */
	if ( src < doc->src2 )
	{
	    doc->text_len = (u_i2)(doc->src2 - src);

	    if ( doc->text_len > sizeof( doc->text_buffer ) )
	    {
# ifdef GCXDEBUG
		if ( gco_trace_level >= 1 )
		    TRdisplay( "GCO save buffer overflow: %d bytes\n",
				(i4)doc->text_len );
# endif /* GCXDEBUG */

		return( E_GC2410_PERF_CONV );
	    }

	    MEcopy( src, doc->text_len, doc->text_buffer );
	    src = doc->src2;
	    src_limit = 0;
	}

	/*
	** Turn off segments flag at end-of-message.
	*/
	if ( doc->eod )  doc->flags &= ~DOC_SEG_FLG;
        break;

    case OP_RSTR_MSG:		/* Restore input */
	if ( doc->text_len > 0 )
	{
	    src -= doc->text_len;
	    src_limit = doc->src2 - src;
	    MEcopy( doc->text_buffer, doc->text_len, src );
	    doc->text_len = 0;
	}
        break;

    case OP_VAR_BEGIN:		/* note beginning of GCA_VAREXPAR */
	/* 
	** Beginning of VAREXPAR array: we do nothing
	**
	** Note: the number of array elements is on the top of the
	** stack.  It is updated/popped by a OP_DJNZ operation.
	**
	** arg1 = size of substructure
	** arg2 = alignment type
	** arg3 = alignment length
	*/
	doc->pc += 3;
	break;

    case OP_VAR_END:		/* note end of GCA_VAREXPAR */
	break;

    case OP_DV_BEGIN:		/* note beginning of GCA_DATA_VALUE value */
	/* 
	** Clear DV meta-data length reference.  The
	** reference was either saved in OP_DV_REF or
	** is not applicable to the data type being
	** processed.  In either case, the reference
	** should now be dropped.  It will be restored
	** by OP_DV_END if previously saved.
	**
	** arg1 = length (ignored)
	*/
	doc->flags |= DOC_DV_FLG;
	doc->flags &= ~DOC_REF_FLG;
	doc->ref = NULL;
	doc->pc++;
	break;

    case OP_DV_END:		/* note end of GCA_DATA_VALUE value */
	doc->flags &= ~DOC_DV_FLG;

	/*
	** Restore meta-data length reference if saved.
	*/
	if ( doc->flags & DOC_DVREF_FLG )
	{
	    doc->ref = doc->dvref;
	    doc->flags |= DOC_REF_FLG;
	    doc->flags &= ~DOC_DVREF_FLG;
	    doc->dvref = NULL;
	}
	break;

    case OP_DV_ADJUST:		/* Adjust buffer space for GCA_DATA_VALUE */
	break;

    case OP_DV_REF:		/* save meta-data length reference */
	if ( doc->flags & DOC_REF_FLG )
	{
	    doc->dvref = doc->ref;
	    doc->flags |= DOC_DVREF_FLG;
	}
        break;

    case OP_SEG_BEGIN:		/* note beginning of list segments */
	doc->flags |= DOC_SEG_FLG;
        break;

    case OP_SEG_END:		/* note end of list segments */
    	doc->flags &= ~DOC_SEG_FLG;
	break;

    case OP_MARK:		/* save current processing state */
	/*
	** Conversion does not overflow.
	*/
	break;

    case OP_REWIND:		/* backup to saved processing state */
	/*
	** No saved processing state to restore.
	*/
# ifdef GCXDEBUG
	if ( gco_trace_level >= 1 )
	    TRdisplay( "GCO internal error: REWIND with no MARK\n" );
# endif /* GCXDEBUG */

	doc->src1 = src;
	doc->dst1 = dst;
	doc->pc--;
	return( E_GC2410_PERF_CONV );

    case OP_PAD_ALIGN:		/* alignment pad for structures */
	doc->pc++;
	break;

    case OP_DIVIDE:		/* Divide stack by argument */
	arg = *doc->pc++;			/* Divisor */
	t_i4 = *(--doc->vsp);			/* Dividend */
	*doc->vsp++ = (arg < 2) ? t_i4 : (t_i4 / arg);
	break;

    case OP_BREAK:		/* end of data segment */
	break;

    /*
    ** Start of conversion operations.
    **
    ** Array conversions are limited to the available
    ** space in our input and output buffers.  Remaining
    ** elements are converted after requesting more
    ** input or output space.
    */

    case OP_CV_I1:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_I1_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_I1_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_I1_MACRO( src, dst, &status );
		src += CV_N_I1_SZ;
		dst += CV_L_I1_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_I1_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_I1_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_I1_MACRO( src, dst, &status );
		src += CV_L_I1_SZ;
		dst += CV_N_I1_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_I2:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_I2_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_I2_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_I2_MACRO( src, dst, &status );
		src += CV_N_I2_SZ;
		dst += CV_L_I2_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_I2_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_I2_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_I2_MACRO( src, dst, &status );
		src += CV_L_I2_SZ;
		dst += CV_N_I2_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_I4:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_I4_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_I4_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_I4_MACRO( src, dst, &status );
		src += CV_N_I4_SZ;
		dst += CV_L_I4_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_I4_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_I4_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_I4_MACRO( src, dst, &status );
		src += CV_L_I4_SZ;
		dst += CV_N_I4_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_I8:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_I8_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_I8_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_I8_MACRO( src, dst, &status );
		src += CV_N_I8_SZ;
		dst += CV_L_I8_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_I8_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_I8_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_I8_MACRO( src, dst, &status );
		src += CV_L_I8_SZ;
		dst += CV_N_I8_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_F4:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_F4_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_F4_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_F4_MACRO( src, dst, &status );
		src += CV_N_F4_SZ;
		dst += CV_L_F4_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_F4_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_F4_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_F4_MACRO( src, dst, &status );
		src += CV_L_F4_SZ;
		dst += CV_N_F4_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_F8:
	if ( inout == INFLOW )
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_L_F8_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_N_F8_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2L_F8_MACRO( src, dst, &status );
		src += CV_N_F8_SZ;
		dst += CV_L_F8_SZ;
	    }
	}
	else
	{
	    i = doc->vsp[-1];
	    if (VEC_MACRO(i, dst_limit, CV_N_F8_SZ)) doc->state = DOC_MOREOUT;
	    if (VEC_MACRO(i, src_limit, CV_L_F8_SZ)) doc->state = DOC_MOREIN;
	    doc->vsp[-1] -= i;

	    while( i-- )
	    {
		CV2N_F8_MACRO( src, dst, &status );
		src += CV_L_F8_SZ;
		dst += CV_N_F8_SZ;
	    }
	}

	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

    case OP_CV_BYTE:
	i = doc->vsp[-1];
	if ( VEC_MACRO( i, dst_limit, 1 ) )  doc->state = DOC_MOREOUT;
	if ( VEC_MACRO( i, src_limit, 1 ) )  doc->state = DOC_MOREIN;
	doc->vsp[-1] -= i;

	MEcopy( (PTR)src, i, (PTR)dst );
	src += i;
	dst += i;
	if ( doc->state == DOC_IDLE )  doc->vsp--;
	goto converted;

	case OP_CV_CHAR:		/* Convert characters */
	{
	    i4		len, limit, *lptr = NULL;
	    bool	inflow = (inout == INFLOW);
	    bool	eod = ! (doc->flags & DOC_SEG_FLG);

	    /*
	    ** Input length is on the stack.  If the value
	    ** has a max limit then the padding length is
	    ** also on the stack.  Output length limit is 
	    ** initially the combined length of input and 
	    ** padding.  As characters are processed, output 
	    ** limit drops in relation to the amount of input 
	    ** processed.  The relative change delta tracks 
	    ** the relationship between input and output 
	    ** lengths.  If there is a DV meta-data length 
	    ** reference, then output is not limited.
	    */
	    len = doc->vsp[-1];		/* input length */

	    if ( doc->flags & DOC_LIMIT_FLG && ! (doc->flags & DOC_DVREF_FLG) )
	    {
		limit = len + doc->vsp[-2] - doc->delta;
		lptr = &limit;
	    }

	    if ( inflow )
		doc->state = (*xlt->n2l.xlt)( xlt->n2l.map, eod,
					      (u_i1 **)&src, src_limit,
					      (u_i1 **)&dst, dst_limit,
					      &len, lptr, &doc->delta );
	    else
		doc->state = (*xlt->l2n.xlt)( xlt->l2n.map, eod,
					      (u_i1 **)&src, src_limit,
					      (u_i1 **)&dst, dst_limit,
					      &len, lptr, &doc->delta );

	    /*
	    ** Update input length.  Padding does not change.
	    ** Output limit is updated (calculated) via changes
	    ** in input input length and relative change delta.
	    ** Unlike other CV operations, input length is not
	    ** popped from the stack when processing complete.
	    */
	    doc->vsp[-1] = len;
	    goto converted;
	}
	case OP_CV_PAD:
	    /*
	    ** Padding does not flow across the net.  Outbound the input
	    ** padding is skipped.  Inbound the padding is reinserted in
	    ** output.
	    */
	    if ( inout == INFLOW )
	    {
		/*
		** Relative change in data length is compensated
		** with padding.  If data decreased, padding will 
		** increase.  Otherwise, padding will decrease 
		** but only to zero (possibly leaving delta > 0). 
		**
		** Note that this operation is repeatable (for
		** the case where DOC_MOREOUT occurs) since
		** either the padding length or change delta
		** will go to zero with the other positive.
		** Subsequently, no compensation will occur.
		*/
		if ( (i = min( doc->vsp[-1], doc->delta )) )
		{
		    doc->vsp[-1] -= i;
		    doc->delta -= i;
		}

		if ( (i = doc->vsp[-1]) )
		{
		    if ( VEC_MACRO(i, dst_limit, CV_L_CHAR_SZ) ) 
			doc->state = DOC_MOREOUT;

		    if ( i )
		    {
			doc->vsp[-1] -= i;

			/* Create padding in output stream */ 
			MEfill( i, (u_char)0, (PTR)dst );
			dst += i;
		    }
		}
	    }
	    else
	    {
		if ( (i = doc->vsp[-1]) )
		{
		    if ( VEC_MACRO( i, src_limit, CV_L_CHAR_SZ ) ) 
			doc->state = DOC_MOREIN;

		    if ( i )
		    {
			doc->vsp[-1] -= i;

			/* Skip padding in input stream */
			src += i;
		    }
		}

		/*
		** Since padding is being removed from input 
		** stream, can't directly compensate relative 
		** data change with padding.  However, the
		** change delta can be logically compensated.
		** If data decreases, additional padding can
		** fully compensate.  If data increases,
		** removing padding can compensate but only
		** upto the padding length.  Logically, a
		** negative delta is forced to 0 while a
		** positive delta is decreased by the pad
		** length but only to 0.
		** 
		*/
		doc->delta -= min( doc->delta, i );
	    }

	    doc->flags &= ~DOC_LIMIT_FLG;
	    if ( doc->state == DOC_IDLE )  doc->vsp--;
	    goto converted;

	case OP_PAD:		/* output padding */
	    /*
	    ** When a data value reference is saved, 
	    ** the meta-data length can be adjusted 
	    ** rather than padding the string value.
	    ** Explicit pad length is consumed by
	    ** adjusting the relative change delta.
	    */
	    if ( doc->flags & DOC_DVREF_FLG )
		doc->delta -= doc->vsp[-1];
	    else
	    {
		/*
		** Relative change in data length is compensated
		** with padding.  If data decreased, padding will 
		** increase.  Otherwise, padding will decrease 
		** but only to zero (possibly leaving delta > 0). 
		**
		** Note that this operation is repeatable (for
		** the case where DOC_MOREOUT occurs) since
		** either the padding length or change delta
		** will go to zero with the other positive.
		** Subsequently, no compensation will occur.
		*/
		if ( (i = min( doc->vsp[-1], doc->delta )) )
		{
		    doc->vsp[-1] -= i;
		    doc->delta -= i;
		}

		if ( inout == INFLOW  ||  xlt->type == GCO_XLT_IDENTITY )
		{
		    if ( (i = doc->vsp[-1]) )
		    {
			if ( VEC_MACRO( i, dst_limit, CV_L_CHAR_SZ ) ) 
			    doc->state = DOC_MOREOUT;

			if ( i )
			{
			    doc->vsp[-1] -= i;

			    /*
			    ** Output stream is in local charset,
			    ** so can space fill directly.
			    */
			    MEfill( i, ' ', (PTR)dst );
			    dst += i;
			}
		    }
		}
		else  if ( doc->vsp[-1] )
		{
		    i4		len = doc->vsp[-1];
		    i4		limit = len - doc->delta;
		    u_i1	pad = (u_i1)' ';

		    /*
		    ** Output space padding converted to network charset.
		    */
		    while( len > 0  &&  (doc->state == DOC_IDLE  ||  
					 doc->state == DOC_MOREIN) )
		    {
			u_i1	*padptr = &pad;

			doc->state = (*xlt->l2n.xlt)( xlt->l2n.map, TRUE,
						      &padptr, sizeof( pad ),
						      (u_i1 **)&dst, dst_limit,
						      &len, &limit, 
						      &doc->delta );

			dst_limit = doc->dst2 - dst;
		    }

		    doc->vsp[-1] = len;
		}
	    }

	    doc->flags &= ~DOC_LIMIT_FLG;
	    if ( doc->state == DOC_IDLE )  doc->vsp--;
	    goto converted;

	case OP_SKIP_INPUT:		/* Skip remaining input */
	    /*
	    ** Note: length is assumed to be in bytes (except for
	    ** extended network character set).  If this is not the
	    ** case, element size will need to be compiled as argument.
	    */
	    if ( doc->vsp[-1] )
	    {
		i = doc->vsp[-1];
		arg = (inout != INFLOW) ? CV_L_CHAR_SZ : CV_N_CHAR_SZ;
		if ( VEC_MACRO( i, src_limit, arg ) ) doc->state = DOC_MOREIN;

		if ( i )
		{
		    src += i;
		    doc->vsp[-1] -= i;

		    /*
		    ** Update relative change delta for input processed
		    ** with no corresponding output.  Pop vector count 
		    ** when complete.
		    */
		    doc->delta -= i;
		}
	    }

	    if ( doc->state == DOC_IDLE )  doc->vsp--;
	    goto converted;

	case OP_SAVE_CHAR:		/* Save character input */
	    if ( (i = doc->vsp[-1]) )
	    {
		arg = (inout != INFLOW) ? CV_L_CHAR_SZ : CV_N_CHAR_SZ;
		if ( VEC_MACRO( i, src_limit, arg ) )  doc->state = DOC_MOREIN;

		if ( i )
		{
		    if ( (doc->text_len + i) > sizeof( doc->text_buffer ) )
		    {
# ifdef GCXDEBUG
			if ( gco_trace_level >= 1 )
			    TRdisplay( "GCO save buffer overflow: %d bytes\n",
					(i4)(doc->text_len + i) );
# endif /* GCXDEBUG */

			return( E_GC2410_PERF_CONV );
		    }

		    MEcopy( src, i, &doc->text_buffer[ doc->text_len ] );
		    src += i;
		    doc->text_len += i;

		    /*
		    ** Update input length and relative change delta 
		    ** for input processed with no corresponding output.  
		    */
		    doc->vsp[-1] -= i;
		    doc->delta -= i;
		}
	    }

	    /*
	    ** Pop vector count when complete.
	    */
	    if ( doc->state == DOC_IDLE )  doc->vsp--;
	    goto converted;

	case OP_RSTR_CHAR:		/* Restore character input */
	    if ( doc->text_len > 0 )
	    {
		i = doc->text_len;
		src -= doc->text_len;
		src_limit = doc->src2 - src;
		MEcopy( doc->text_buffer, doc->text_len, src );
		doc->text_len = 0;

		/*
		** Update input length and relative change delta
		** for carryover input.
		*/
		doc->vsp[-1] += i;
		doc->delta += i;
	    }
	    break;

	case OP_SEG_CHECK:		/* Check for end-of-segments */
	    /*
	    ** Saved character input must be processed if the current 
	    ** segment is the last segment.  Branch if there is saved 
	    ** input and the next segment indicator is end-of-segments.
	    */
	    arg = *doc->pc++;	/* Branch offset */

	    if ( doc->text_len > 0 )
	    {
		/*
		** Bit of a kludge here: the segment indicator
		** is assumed to be next in input stream and is
		** an I4.  It would be better to generate the
		** correct operations from the VARLSTAR object
		** descriptor, but at that processing level it
		** isn't currently possible to insert operations
		** into the segment processing compilation.
		*/
		i = 1;

		if ( inout == INFLOW )
		{
		    if ( VEC_MACRO( i, src_limit, CV_N_I4_SZ ) ) 
		    {
			doc->state = DOC_MOREIN;
			doc->pc--;
			goto converted;
		    }

		    CV2L_I4_MACRO( src, (char *)&t_i4, &status );
		}
		else
		{
		    if ( VEC_MACRO( i, src_limit, CV_L_I4_SZ ) ) 
		    {
			doc->state = DOC_MOREIN;
			doc->pc--;
			goto converted;
		    }

		    MECOPY_CONST_MACRO( src, CV_L_I4_SZ, (PTR)&t_i4 );
		}

		/*
		** Branch if end-of-segments to finish processing
		** final segment.
		*/
		if ( t_i4 == 0 )
		{
		    /*
		    ** There are no more segments, so turn off
		    ** the segments flag to ensure the saved
		    ** character input will be fully consumed.
		    ** Also need to initialize the input length
		    ** prior to restoring the saved input (which
		    ** is another bit of a kludge requiring the
		    ** branch to target OP_RSTR_CHAR).
		    */
		    doc->flags &= ~DOC_SEG_FLG;
		    *doc->vsp++ = 0;
		    doc->pc += arg;
		}
	    }
	    break;

	converted:

	src_limit = doc->src2 - src;
	dst_limit = doc->dst2 - dst;

	if ( doc->state != DOC_IDLE )	/* Need more input/output? */
	{
	    STATUS status = OK;

	    switch( doc->state )
	    {
	    case DOC_MOREIN :
		/* 
		** Save unprocessed atomic fragment for later processing 
		*/
		if ( src < doc->src2 )
		{
		    doc->frag_bytes = (u_i2)(doc->src2 - src);

		    if ( doc->frag_bytes > sizeof( doc->frag_buffer ) )
		    {
# ifdef GCXDEBUG
			if ( gco_trace_level >= 1 )
			    TRdisplay( "GCO atom fragment overflow: %d bytes\n",
			    		(i4)doc->frag_bytes );
# endif /* GCXDEBUG */

			status = E_GC2410_PERF_CONV;
			break;
		    }

		    MEcopy( (PTR)src, doc->frag_bytes, (PTR)doc->frag_buffer );
		    src = doc->src2;
		}
	        break;

	    case DOC_MOREOUT :
		/*
		** If the output reference flags are set, convert to 
		** DOC_MOREQOUT to keep referenced buffer from being 
		** flushed.
		*/
		if ( doc->flags & (DOC_REF_FLG | DOC_DVREF_FLG | DOC_OUTQ_FLG) )
		{
		    doc->state = DOC_MOREQOUT;
		    doc->flags |= DOC_OUTQ_FLG;
		}
	        break;
	    }

# ifdef GCXDEBUG
	    if ( gco_trace_level >= 2  &&  status == OK )
		TRdisplay( "GCO convert: state = %s\n",
			    gcx_look( gcx_doc_state, doc->state ) );

	    if ( gco_trace_level >= 4 )
		TRdisplay( "           : src-len = %d, dst-len = %d, save = %d, frag = %d\n",
			   doc->src2 - src, doc->dst2 - dst, 
			   doc->text_len, doc->frag_bytes );
# endif /* GCXDEBUG */

	    /*
	    ** Return current input/output positions and
	    ** set PC to re-execute current operation.
	    */
	    doc->src1 = src;
	    doc->dst1 = dst;
	    doc->pc--;
	    return( status );
	}
	break;
    }

    goto top;
}



/*
** Name: gco_xlt_ident
**
** Description:
**	Copy character data from source (src) to destination (dst).
**	Updates the source and destination data positions, input 
**	data length (inLength) and relative data length change 
**	(delta).  Returns the processing state.
**
**	Optionally, the output data length may be limited by 
**	providing the space available (outLength) for output.  
**	Set to NULL otherwise.
**
**	NOTE: assumes that the size of local and network
**	characters are one byte.
**
** Input:
**	map		Character translation map (ignored).
**	eod		TRUE if more input than inLength is available.
**	src		Source data position.
**	srcLimit	Input buffer length (bytes).
**	dst		Destination data position.
**	dstLimit	Output buffer space (bytes).
**	inLength	Input character length (elements).
**	outLength	Output character space (elements), may be NULL.
**	delta		Relative change in data length.
**
** Output:
**	src		Position of remaining input.
**	dst		Position for subsequent output.
**	inLength	Remaining input character length.
**	outLength	Remaining output character space, if not NULL.
**	delta		Cumulative change in character length.
**
** Returns:
**	i1	Processing state:
**		    DOC_IDLE	Processing complete.
**		    DOC_MOREIN	More input needed.
**		    DOC_MOREOUT	More output space needed.
**
** History:
**	22-Jan-10 (gordy)
**	    Created.
*/	

i1
gco_xlt_ident
(
    PTR		map,
    bool	eod,
    u_i1	**src,
    i4		srcLimit,
    u_i1	**dst,
    i4		dstLimit,
    i4		*inLength,
    i4		*outLength,
    i4		*delta
)
{
    i1 state = DOC_IDLE;
    i4 length = *inLength;

    if ( outLength  &&  *outLength < length )  length = *outLength;

    if ( length > 0 )
    {
	if ( VEC_MACRO( length, dstLimit, 1 ) )  state = DOC_MOREOUT;
	if ( VEC_MACRO( length, srcLimit, 1 ) )  state = DOC_MOREIN;

	MEcopy( (PTR)*src, length, (PTR)*dst );
	*src += length;
	*dst += length;
	*inLength -= length;
	if ( outLength )  *outLength -= length;
    }

    return( state );
}



/*
** Name: gco_xlt_sb
**
** Description:
**	Translates character data using a single byte translation
**	table (map).  Updates the source (src) and destination (dst) 
**	data positions, input data length (inLength) and relative 
**	data length change (delta).  Returns the processing state.
**
**	Optionally, the output data length may be limited by 
**	providing the space available (outLength) for output.  
**	Set to NULL otherwise.
**
**	NOTE: assumes that the size of local and network
**	characters are one byte.
**
** Input:
**	map		Character translation map.
**	eod		TRUE if more input than inLength is available.
**	src		Source data position.
**	srcLimit	Input buffer length (bytes).
**	dst		Destination data position.
**	dstLimit	Output buffer space (bytes).
**	inLength	Input character length (elements).
**	outLength	Output character space (elements), may be NULL.
**	delta		Relative change in data length.
**
** Output:
**	src		Position of remaining input.
**	dst		Position for subsequent output.
**	inLength	Remaining input character length.
**	outLength	Remaining output character space, if not NULL.
**	delta		Cumulative change in character length.
**
** Returns:
**	i1	Processing state:
**		    DOC_IDLE	Processing complete.
**		    DOC_MOREIN	More input needed.
**		    DOC_MOREOUT	More output space needed.
**
** History:
**	22-Jan-10 (gordy)
**	    Created.
*/	

i1
gco_xlt_sb
(
    PTR		map,
    bool	eod,
    u_i1	**src,
    i4		srcLimit,
    u_i1	**dst,
    i4		dstLimit,
    i4		*inLength,
    i4		*outLength,
    i4		*delta
)
{
    i1 state = DOC_IDLE;
    i4 length = *inLength;

    if ( outLength  &&  *outLength < length )  length = *outLength;

    if ( length > 0 )
    {
	u_i1 *x2y = (u_i1 *)map;

	if ( VEC_MACRO( length, dstLimit, 1 ) )  state = DOC_MOREOUT;
	if ( VEC_MACRO( length, srcLimit, 1 ) )  state = DOC_MOREIN;

	*inLength -= length;
	if ( outLength )  *outLength -= length;
	while( length-- )  *(*dst)++ = x2y[ *(*src)++ ];
    }

    return( state );
}



/*
** Name: gco_xlt_db
**
** Description:
**	Translates character data using a double byte translation
**	table (map).  Updates the source (src) and destination (dst) 
**	data positions, input data length (inLength) and relative 
**	data length change (delta).  Returns the processing state.
**
**	Optionally, the output data length (outLength) may be 
**	limited by providing the space available for output.  
**	Set to NULL otherwise.
**
**	There are three types of errors which can occur during
**	character translation.  The manner in which these errors
**	are handled here is compatibile with past versions of
**	this algorithm.
**
**	    Truncated Input: 
**		Input ends with lead byte of a two-byte character (*).
**		A single 0 byte is output.
**
**	    No character translation:
**		Input (two-byte) character has no mapped output.
**		Two 0 bytes are output.
**		
**	    Truncated Output:
**		Insufficient space to output a character.
**		Lead (high order) byte is output if possible.
**
**	(*) Note: a single-byte character (excluding EOS) without 
**	a mapping is assumed to be the lead byte of a double-byte 
**	character.
**
** Input:
**	map		Character translation map.
**	eod		TRUE if more input than inLength is available.
**	src		Source data position.
**	srcLimit	Input buffer length (bytes).
**	dst		Destination data position.
**	dstLimit	Output buffer space (bytes).
**	inLength	Input character length (elements).
**	outLength	Output character space (elements), may be NULL.
**	delta		Relative change in data length.
**
** Output:
**	src		Position of remaining input.
**	dst		Position for subsequent output.
**	inLength	Remaining input character length.
**	outLength	Remaining output character space, if not NULL.
**	delta		Cumulative change in character length.
**
** Returns:
**	i1	Processing state:
**		    DOC_IDLE	Processing complete.
**		    DOC_MOREIN	More input needed.
**		    DOC_MOREOUT	More output space needed.
**
** History:
**	22-Jan-10 (gordy)
**	    Created.
*/	

i1
gco_xlt_db
(
    PTR		map,
    bool	eod,
    u_i1	**src,
    i4		srcLimit,
    u_i1	**dst,
    i4		dstLimit,
    i4		*inLength,
    i4		*outLength,
    i4		*delta
)
{
    u_i2 *x2y16 = (u_i2 *)map;

    /*
    ** Process characters while there is input and
    ** output space available.
    */
    while( *inLength > 0  &&
    	   (outLength == NULL  ||  *outLength > 0) )
    {
	u_i2 val;
	u_i1 inSize, outSize;

	/*
	** There are more characters to process 
	** but input buffer has been exhausted.
	*/
    	if ( srcLimit <= 0 )  return( DOC_MOREIN );

	/*
	** Determine size of the next input character.  
	** It is single byte if there is an entry in the 
	** translation table, otherwise it is double byte. 
	** 
	** The first byte of double byte characters are
	** flagged with 0 in the translation table.  An 
	** input of 0 also always maps to 0, so this case 
	** must be caught as well.
	**
	** Note that for our purposes, a character element 
	** is one byte while an actual character can be one 
	** or two bytes in length.
	*/
    	val = x2y16[ (*src)[0] ];
	inSize = (val || ! (*src)[0]) ? 1 : 2;

        if ( inSize > *inLength )
	{
	    /*
	    ** Partial character cannot be processed,
	    ** but may be combined with subsequent
	    ** available input.  This is different
	    ** than the MOREIN condition because the
	    ** current text segment is complete.
	    */
	    if ( ! eod )  return( DOC_IDLE );

	    /*
	    ** The input is malformed (last byte is first
	    ** byte of a double byte character).  This
	    ** condition should be treated as an input
	    ** character error, but for compatibility
	    ** the input byte is simply translated by
	    ** itself.
	    **
	    ** Force size to single byte since input 
	    ** length is positive but less than two at 
	    ** this point.
	    */
	    inSize = 1;
	}

        if ( inSize > srcLimit )  return( DOC_MOREIN );

	/* 
	** Translate double byte characters.
	**
	** Note that a 0 result indicates a translation error
	** and should be handled as such.  For compatibility
	** the 0 value is output (take note of the additional
	** comment for determining output size below).
	*/
	if ( inSize == 2 )  val = x2y16[ ((*src)[0] << 8) + (*src)[1] ];

	/*
	** Make sure there is room for the output.
	**
	** Note that an output value of 0 either indicates
	** an input byte of 0 or a translation failure.  For
	** compatibility, the convention is to output the
	** same number of bytes as the input in both cases.
	*/
    	outSize = (val > 0xff) ? 2 : ((! val) ? inSize : 1);

	if ( outLength  &&  *outLength < outSize )
	{
	    /*
	    ** There isn't space for a two byte character.
	    ** This condition should be treated as a
	    ** truncation error but for compatibility
	    ** a partial character is output.  Note
	    ** that the high-order byte is to be output
	    ** so it must be shifted to the location of
	    ** a single output byte.
	    */
	    outSize = 1;
	    val >>= 8;
	}

        if ( outSize > dstLimit )  return( DOC_MOREOUT );

	/*
	** Finally, output the translated character
	** and consume the input character.
	*/
	switch( outSize )
	{
	case 1 :  (*dst)[0] = (u_i1)val;		break;

	case 2 :  (*dst)[0] = (u_i1)(val >> 8);
		  (*dst)[1] = (u_i1)val;		break;
	}

	*src += inSize;
	srcLimit -= inSize;
	*dst += outSize;
	dstLimit -= outSize;
	*inLength -= inSize;
	if ( outLength )  *outLength -= outSize;
	*delta += outSize - inSize;
    }

    return( DOC_IDLE );
}


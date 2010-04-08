/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>

#include    <gcccl.h>
#include    <cv.h>		 
#include    <cvgcc.h>
#include    <er.h>
#include    <gc.h>
#include    <me.h>
#include    <qu.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>

#include    <gca.h>
#include    <gcaint.h>
#include    <gcocomp.h>
#include    <gcoint.h>
#include    <gccer.h>
#include    <gcxdebug.h>

/*
**
**  Name: GCOCOMP.C - GCF Data Conversion module
**
**  External entry points:
**	gco_init 	- initialize data conversion structures
**	gco_init_doc  	- initialize GCO_DOC
**	gco_comp_dv 	- compile a data value descriptor
**	gco_comp_tod 	- compile a tuple descriptor
**	gco_comp_tdescr - compile a GCA_TDESCR-type tuple descriptor
**	gco_align_pad 	- calculate padding for element alignment
**
**  Internal routines:
**
**	gco_map_ddt	- Map DBMS type ID to GCO internal ID.
**	gco_comp_msg	- compile GCA message descriptor
**	gco_comp_ddt	- compile DBMS datatype descriptor
**	gco_comp_tpl	- compile TPL for DBMS datatype
**	gco_comp_od 	- compile a GCA_OBJECT_DESC
**	gco_comp_el 	- compile a GCA_ELEMENT
**	gco_comp_atomic - compile an atomic value
**	gco_push_numeric- push a numeric value on stack
**	gco_upd_ref	- update a numeric value via reference
**	gco_dumpobjdesc - trace an object descriptor for debugging
**
**  History:
**      25-May-93 (seiwald)
**	    Extracted from gccpdc.c.
**	3-Jun-93 (seiwald)
**	    Eliminate the age old GCA_TRIAD_OD - when trying to find the
**	    GCA_OBJECT_DESC for a complex type, we now just directly scan 
**	    the table handed back by adc_gcc_decompose().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	20-sep-93 (seiwald)
**	    Revert to GC_PAD_ATOMIC's having an explicit array status
**	    GCA_VARPADAR, which tells gcc_comp_el() that the size of the
**	    pad is already on the stack from the previous GCA_VARVCHAR.
**	    The previous logic assumed that only GC_CHAR_ATOMIC's could
**	    precede pads, and this was broken with the new "nullable
**	    byte varying" type.
**	1-mar-94 (seiwald)
**	    Added hints for message encoding/decoding.  Surround processing
**	    of elements that have array status = GCA_VAREXPAR with 
**	    OP_VAR_BEGIN and OP_VAR_END.  The OP_VAR_BEGIN indicates the
**	    size of the individual GCA_ELEMENT, as calculated by the new
**	    flags to the gcc_comp*() functions.
**	17-mar-94 (seiwald)
**	    Add size calculations and VAR_BEGIN/VAR_END for GCA_DATA_VALUEs.
**	29-Mar-94 (seiwald)
**	    Added support for VARIMPAR in message encoding.
**	1-Apr-94 (seiwald)
**	    Zero sz_struct when computing size of GCA_DATA_VALUE.
**	4-Apr-94 (seiwald)
**	    Spruce up tracing with OP_DEBUG_TYPE and OP_DEBUG_BUFS.
**	4-Apr-94 (seiwald)
**	    New OP_DV_BEGIN/END.
**	4-Apr-94 (seiwald)
**	    OP_MSG_BEGIN now includes the message type for tracing.
**	5-Apr-94 (seiwald)
**	    Allocate programs for compiled messages piecemeal.
**	18-Apr-94 (seiwald)
**	    New gcc_td_comp() to compile GCA_TDESCR messages.
**	24-Apr-94 (seiwald)
**	    Put new OP_BREAK instruction at the end of tuple data to
**	    indicate the end of a row.
**	04-May-94 (cmorris)
**	    Bump level at which plethora of trace output appears to 3.
**	 7-Nov-94 (gordy)
**	    Add new OP_MSG_RESTART instruction after OP_BREAK similar
**	    to OP_MSG_BEGIN except for start of next tuple.
**	24-mar-95 (forky01)
**	    Added STATUS and error msg/code E_GC2414_PL_COMP_MSGS_EXCEED
**	 1-May-95 (gordy)
**	    Made gcc_align_pad() non-static.  Added alignment type and
**	    length to OP_VAR_BEGIN arguments.
**	25-May-95 (gordy)
**	    Tuples are still processed as a data stream rather than
**	    structured.  Added a structure/stream parameter to various
**	    compilation functions to enable the correct padding behaviour.
**	    Cleaned up GCA_VARSEGAR.  
**	13-May-95 (gordy)
**	    Cleaned up naming conventions.
**	20-Dec-95 (gordy)
**	    Added support for incremental initialization.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Fixed processing of large messages which can span buffers.
**	    Added OP_MARK/OP_REWIND to split C data structures at
**	    VARIMPAR boundaries.  Combined gcd_size_atomic() into
**	    gcd_comp_atomic().
**	19-Mar-96 (gordy)
**	    Instead of treating tuples as a single VARIMPAR, treat each
**	    element as its own VARIMPAR.  This permits the processing
**	    of larger tuples and/or elements.
**	 3-Apr-96 (gordy)
**	    OP_MARK now precedes the location to be marked.  OP_VAR_BEGIN
**	    and OP_DV_BEGIN now emulate OP_REWIND rather than explicitly
**	    coding a jump and rewind operation.  Tuples no longer returned
**	    one at a time (OP_BREAK removed after end of TUPLE).  BLOB
**	    segments may now be processed in GCA_DATA_VALUES as well as
**	    in GCA_TUPLES.
**	21-Jun-96 (gordy)
**	    Added GCA_VARMSGAR for messages which are processed in bulk
**	    rather than as formatted elements.  Moved the special process
**	    for tuple messages from the object descriptor level to the
**	    message level so that we can still compile the descriptor.
**	    The program for tuple messages is placed in a global so 
**	    encode/decode can use it when CALLTUP is executed.
**	30-sep-1996 (canor01)
**	    Move global data definitions to gcdod.c.
**	 5-Feb-97 (gordy)
**	    DBMS datatype object descriptors are now pre-compiled.
**	    Removed gcd_ld_od().  Added gcd_map_ddt() to map DBMS 
**	    datatype ID's to internal GCD datatype ID's.  Added 
**	    gcd_comp_ddt() to pre-compile datatype descriptors.  
**	    Added gcd_comp_tpl() to compile OP_CALL_DDT programs 
**	    for datatype processing of tuples and GCA_DATA_VALUE's.
**	    Generalized BLOB segment handling into list processing 
**	    and renamed gcd_comp_segment() to gcd_comp_list().  
**	    Removed GCD_TUPLE_MASK since encode/decode no longer
**	    process individual tuple attributes.  Removed GCD_DV_MASK
**	    since its compilation differed from the default only in
**	    operations which are NO-OPs in gcd_convert().  Combined 
**	    and simplified the array status types.  VARMSGAR replaced 
**	    by VARIMPAR for atomic type.  VARLSTAR added for general 
**	    list processing replacing VARVSEGAR for BLOB segments.
**	    VARSEGAR generalized for BLOB segments and compressed 
**	    variable length types.  VARTPLAR and VARVCHAR generalized 
**	    and replace VAR1TPLAR, VAR1VCHAR and VARPADAR.  
**	24-Feb-97 (gordy)
**	    Changed gcd_push_numeric() parameter to GCA_ELEMENT since 
**	    we are always pushing a preceding numeric element.  Added
**	    previous element argument to gcd_comp_el() for array types
**	    which are preceded by a numeric length count.
**	 4-Mar-98 (gordy)
**	    Alignment is required for data structures as well as data
**	    elements.  Structures also require padding for proper size.
**	    Certain array types (VAREXPAR and Data Values) are stored
**	    separate from data structures and replaced by pointers,
**	    so base alignment on the pointer not the extracted element.
**	31-Mar-98 (gordy)
**	    Include long varchar in doublebyte special string handling.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	 1-Mar-01 (gordy)
**	    Added support for new DBMS Unicode datatypes.
**	    OP_PUSH_VCH now takes size of array element as argument.
**	    For TPL arrays, if the element size is not 1, need to
**	    convert TPL byte length to number of elements.
**      21-June-2001 (wansh01) 
**          Changed gcxlevel to gcd_trace_level to seperate the gcc and gcd trace
**	22-Mar-02 (gordy)
**	    More nat to i4 cleanup.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Use macro to initialize DOC.
**	12-Jan-04 (gordy)
**	    Use semaphore to protect static storage during compilation.
**	15-Mar-04 (gordy)
**	    Support eight-byte integers.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time types.
**	24-Oct-06 (gordy)
**	    Added support for Blob/Clob locator types.
**	 9-Oct-09 (gordy)
**	    Added support for Boolean type.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
*/

/*
** Compilation options.
*/

# define GCO_ALIGN_MASK		0x01	/* Pad for alignment */
# define GCO_COMP_MASK		0x02	/* Compressed VARCHAR */



/*
** Determine length from TPL, adjusting for
** NULL byte as appropriate.
*/

# define GCO_TPL_LEN( tpl )	(((tpl)->type < 0) \
				? (tpl)->length - 1 : (tpl)->length)

/*
** Local variables.
*/
static bool		gco_initialized = FALSE;
static MU_SEMAPHORE	gco_semaphore;
static u_i2		*gco_ddt_len = NULL;
static GCO_OP		prog_buffer[ GCO_L_MSG_MAX ];


/*
** Default programs to produce error for
** invalid messages, datatypes.
*/
static GCO_OP	gco_prog_no_msg[] = { OP_ERROR, EOP_NOMSGOD };
static GCO_OP	gco_prog_no_ddt[] = { OP_ERROR, EOP_NOTPLOD };



/*
**  Forward and/or External function references.
*/

GLOBALREF GCXLIST gcx_gco_ops[];

static	bool	gco_map_ddt( GCA_ELEMENT *, i4 *, i4 );
static	STATUS	gco_comp_msg( i4, i4 );
static	STATUS	gco_comp_ddt( i4, i4 );
static	VOID	gco_comp_tpl( GCA_ELEMENT *, GCO_OP **, u_i2  *, i4 );
static	u_i2	gco_comp_list( GCA_ELEMENT *, GCO_OP **, i4 );
static	VOID	gco_comp_od( GCA_OBJECT_DESC *, 
			     GCO_OP **, u_i2 *, i4, GCA_ELEMENT * );
static	VOID	gco_comp_el( GCA_ELEMENT *, GCO_OP **, 
			     u_i2  *, i4, GCA_ELEMENT *, GCA_ELEMENT * );
static	u_i2	gco_comp_atomic( GCA_ELEMENT *, GCO_OP ** );
static	GCO_OP	gco_push_numeric( GCA_ELEMENT *, bool );
static	GCO_OP	gco_update_ref( GCA_ELEMENT * );
static	VOID	gco_cmp_align( GCA_ELEMENT *, GCA_ELEMENT *, GCA_ELEMENT * );

# ifdef GCXDEBUG
static	VOID	gco_dumpobjdesc( GCA_OBJECT_DESC *, i4, i4  );
static	VOID	gco_dumpprog( char *, GCO_OP *, i4, i4 );
# endif /* GCXDEBUG */



/*
** Name: gco_init
**
** Description:
**	Initialize the object desciptor sub-system.  A full or 
**	incremental initialization may be requested.  The incremental
**	initialization delays compilation of object descriptors for
**	individual GCA message until each is first processed.
**
** Input:
**	None
**
** Output:
**	None
**
** Returns:
**	STATUS
**
** History:
**	13-Sep-95 (gordy)
**	    Renamed to gcd_init().
**	20-Dec-95 (gordy)
**	    Added parameter for full/incremental initialization.
**	 5-Feb-97 (gordy)
**	    Allocate and pre-compile datatype object descriptors.
**      06-07-01 (wansh01) 
**          moved init gcd_trace_level from gcx_init to gcd_init. 
**	12-Jan-04 (gordy)
**	    Only allow single initialization for new semaphore.
**	22-Jan-10 (gordy)
**	    Check some initialization on every call in case a full
**	    init follows a partial init.
*/

STATUS
gco_init( bool full_init )
{
    i4		i;
    STATUS	status = OK;
    char        *level = NULL;

    if ( ! gco_initialized )	/* Only initialize once */
    {
        gco_initialized = TRUE;

	/* Initialize gco_semaphore to protet static storage */
	MUi_semaphore( &gco_semaphore );
	MUn_semaphore( &gco_semaphore, ERx("GCD") );

	/* Initialize trace */
	gcu_get_tracesym( "II_GCO_TRACE", "!.gco_trace_level", &level );
	if ( level  &&  *level )  CVan( level, &gco_trace_level );
    }

    /*
    ** Initialize the message and datatype mapping tables.
    */
    if ( ! gco_msg_map )
	gco_msg_map = (GCO_OP **)MEreqmem( (u_i2)0, 
					   (u_i4)( gco_l_msg_ods * 
						    sizeof( *gco_msg_map ) ),
					   TRUE, &status );

    if ( ! gco_ddt_map )
	gco_ddt_map = (GCO_OP **)MEreqmem( (u_i2)0, 
					   (u_i4)( gco_l_ddt_ods * 
					            sizeof( *gco_ddt_map ) ),
					   TRUE, &status );

    if ( ! gco_ddt_len )
	gco_ddt_len = (u_i2 *)MEreqmem((u_i2)0, 
				       (u_i4)( gco_l_ddt_ods * 
						sizeof( *gco_ddt_len ) ),
				       TRUE, &status );

    /*
    ** In certain situations, such as the standalone Comm Server,
    ** it is beneficial to compile all the GCA message descriptors
    ** at initialization time when it will have the least impact.
    ** At other times, we want to keep initialization overhead to
    ** a minimum while enduring a small run-time penalty for each 
    ** new message processed.
    */
    if ( full_init  &&  gco_msg_map  &&  ! gco_msg_map[ 0 ] )  
	for( i = 0; i < gco_l_msg_ods; i++ )
	    if ( (status = gco_comp_msg( i, 4 )) != OK )  break;

    if ( full_init  &&  gco_ddt_map  &&  ! gco_ddt_map[ 0 ] )  
	for( i = 0; i < gco_l_ddt_ods; i++ )
	    if ( (status = gco_comp_ddt( i, 4 )) != OK )  break;

    return( status );
}


/*
** Name: gco_init_doc - initialize GCO_DOC
**
** Description:
**	Initializes a GCO_DOC by mapping the given GCA message number into 
**	the compiled descriptor program and setting the program counter.
**
** Inputs:
**	doc		data object context
**	msg_type	GCA message number
**
** History:
**	5-Feb-93 (seiwald)
**	    Caller now allocates the GCO_DOC - we just initialize it.
**	24-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42
**	20-Dec-95 (gordy)
**	    Compile the object descriptor for the current GCA message
**	    if it has not yet been done.
**	 5-Feb-97 (gordy)
**	    Tracing moved to gcd_comp_msg().
**	12-May-03 (gordy)
**	    Use macro to initialize DOC.
*/

VOID
gco_init_doc( GCO_DOC *doc, i4  msg_type )
{
    DOC_CLEAR_MACRO( doc );

    /*
    ** Compile the object descriptor for the current
    ** GCA message if it has not already been done.
    ** Note, errors are ignored here and will be
    ** detected as a result of processing the compiled
    ** output.
    */
    if ( ! gco_msg_map[ msg_type ] )  gco_comp_msg( msg_type, 3 );
    doc->pc = gco_msg_map[ msg_type ];

    return;
}


/*
** Name: gco_comp_msg
**
** Description:
**	Compiles the descriptor for a single GCA message and adds the
**	the program to the message mapping table.  Any undefined messages 
**	(those with null descriptors) compile into a program that cause 
**	graceful failure.
**
**	Caller must provide a GCA message number in the valid range.
**
** Inputs:
**	msg_type	GCA message number.
**	dump_level	Trace level to dump object descriptor.
**
** Outputs:
**	None.
**
** History:
**	24-mar-95 (forky01)
**	    Added STATUS and error msg/code E_GC2414_PL_COMP_MSGS_EXCEED
**	25-May-95 (gordy)
**	    Use structure alignment for messages in gcc_comp_od().
**	20-Dec-95 (gordy)
**	    Only compile a single message.  Renamed to gcd_comp_msg().
**	 8-Mar-96 (gordy)
**	    Added OP_MARK/OP_REWIND to properly return only complete
**	    C data structures.
**	 3-Apr-96 (gordy)
**	    OP_MARK now precedes the point to be marked.
**	21-Jun-96 (gordy)
**	    Special processing for tuple messages is now done here.  We
**	    also compile the object descriptor for tuple messages and
**	    save it in a global variable for use by OP_CALLTUP in encode,
**	    decode.
**	 5-Feb-97 (gordy)
**	    Added dump_level argument to control tracing level.
**	12-Jan-04 (gordy)
**	    Semphore protect statics used during compilation.
**	22-Jan-10 (gordy)
**	    Added OP_END_TUP operation.
*/

static STATUS
gco_comp_msg( i4  msg_type, i4  dump_level )
{
    GCO_OP	*size_pc, *pc = prog_buffer;
    STATUS	status = OK;
    u_i2	length = 0;
    i4		i;

    /*
    ** Check to see if the message has already been compiled.
    ** Note: for the un-compiled condition, this test is not
    ** thread-safe.  A subsequent thread-safe test is made
    ** below to catch race conditions.
    */
    if ( gco_msg_map[ msg_type ] )  return( OK );

    /*
    ** Provide default for messages with no descriptors.
    */
    if ( ! gco_msg_ods[ msg_type ] )
    {
	gco_msg_map[ msg_type ] = gco_prog_no_msg;
	return( OK );
    }

    MUp_semaphore( &gco_semaphore );

    /*
    ** Check to see if the message has already 
    ** been compiled (catch race conditions).
    */
    if ( gco_msg_map[ msg_type ] )  
    {
	MUv_semaphore( &gco_semaphore );
	return( OK );
    }

# ifdef GCXDEBUG
    if ( gco_trace_level >= dump_level )
	gco_dumpobjdesc( gco_msg_ods[ msg_type ], 0, 0 );
# endif /* GCXDEBUG */

    /*
    ** Compile the message descriptor.
    */
    *pc++ = OP_MARK;		/* Mark object boundary */
    *pc++ = OP_MSG_BEGIN;	/* Start of message */
    *pc++ = msg_type;		    /* arg1: message ID */
    size_pc = pc++;		    /* arg2: length of fixed part of object */

    /*
    ** Message containing tuples require special processing.
    */
    if ( msg_type != GCA_TUPLES  &&  msg_type != GCA_CDATA )
	gco_comp_od(gco_msg_ods[msg_type], &pc, &length, GCO_ALIGN_MASK, NULL);
    else
    {
	GCO_OP	*top = pc;

	/*
	** Tuple messages are processed by a CALL_TUP loop.  A gco
	** client can compile programs for tuple sets from GCA_TDESCR
	** messages which will be called from the CALL_TUP loop.  We 
	** also compile a generic tuple program directly from the 
	** GCA_TU_DATA object descriptor and save it so that it can 
	** be used if no tuple set specific program is provided.
	*/

	/*
	** Build the tuple processing loop.
	*/
	*pc++ = OP_JNINP;	/* Exit loop at end-of-input */
	*pc++ = 0;		    /* arg1: pc offset */
	*pc++ = OP_CALL_TUP;	/* Call tuple processing program */
	*pc++ = OP_END_TUP;	/* Tuple processing completed */
	*pc++ = OP_GOTO;	/* loop for next tuple */
	*pc++ = 0;		    /* arg1: pc offset */

	top[1] = pc - (top + 2);
	pc[-1] = top - pc;

	/*
	** Now compile the tuple message object descriptor into
	** a generic program.  The program is first built in the 
	** unused portion of the current program buffer and then
	** saved for global access.
	*/
	if ( ! gco_tuple_prog )
	{
	    top = pc;	/* save current position */

	    gco_comp_od( gco_msg_ods[msg_type], 
			 &pc, &length, GCO_ALIGN_MASK, NULL );

	    *pc++ = OP_RETURN;		/* Return to calling program */

	    gco_tuple_prog = (GCO_OP *)
		    MEreqmem( (u_i2)0, (u_i4)((pc - top) * sizeof(GCO_OP)),
			      FALSE, &status );

	    if ( gco_tuple_prog )
		MEcopy( (PTR)top, (pc - top) * sizeof( GCO_OP ),
			(PTR)gco_tuple_prog );

	    pc = top;	/* restore original program position */
	    length = 0;	/* Length indeterminate until run-time */
	}
    }

    *size_pc = (GCO_OP)length;

    /* 
    ** Fail if input remains after descriptor depleted.
    */
    *pc++ = OP_JNINP;		/* Exit at end-of-input */
    *pc++ = 2;			    /* arg1: pc offset */
    *pc++ = OP_ERROR;		/* Return error */
    *pc++ = EOP_DEPLETED;	    /* arg1: error ID */
    *pc++ = OP_DONE;		/* Processing competed */

    length = pc - prog_buffer;

# ifdef GCXDEBUG
    if ( gco_trace_level >= 3 )
	TRdisplay( "gco_comp_msg: message %s, compiled length %d\n", 
		   gcx_look( gcx_gca_msg, msg_type ), (i4)length );
# endif /* GCXDEBUG */

    if ( length > GCO_L_MSG_MAX )
    {
	status = E_GC2414_PL_COMP_MSGS_EXCEED;
	gco_msg_map[ msg_type ] = gco_prog_no_msg;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 1 )
	    TRdisplay( "gco_comp_msg: message %s, length %d too long!\n", 
		       gcx_look( gcx_gca_msg, msg_type ), (i4)length );
# endif /* GCXDEBUG */
    }
    else
    {
	/* 
	** Copy program from temp buffer 
	*/
	gco_msg_map[ msg_type ] = 
	    (GCO_OP *)MEreqmem( (u_i2)0, (u_i4)(length * sizeof(GCO_OP)),
				FALSE, &status );

	if ( gco_msg_map[ msg_type ] )
	    MEcopy( (PTR)prog_buffer, length * sizeof( GCO_OP ),
		    (PTR)gco_msg_map[ msg_type ] );

# ifdef GCXDEBUG
	if ( gco_trace_level >= dump_level )
		gco_dumpprog( "MSG", prog_buffer, length, 0 );
# endif /* GCXDEBUG */
    }

    MUv_semaphore( &gco_semaphore );
    return( status );
}


/*
** Name: gco_comp_ddt
**
** Description:
**	Compiles the descriptor for a single DBMS datatype and adds the
**	program to the TPL mapping table.  Any undefined types (those
**	with null descriptors) compile into a program that produces a
**	runtime error.
**
**	Caller must provide an internal gco datatype ID in the valid range.
**
** Inputs:
**	gco_type	Internal gco datatype ID.
**	dump_level	Trace level to dump object descriptors.
**
** Outputs:
**	None.
**
** History:
**	28-Jan-97 (gordy)
**	    Created.
**	31-Mar-98 (gordy)
**	    Include long varchar in doublebyte special string handling.
**	12-Jan-04 (gordy)
**	    Semphore protect statics used during compilation.
**	22-Jan-10 (gordy)
**	    Generalized double-byte specific code.
*/

static STATUS
gco_comp_ddt( i4  gco_type, i4  dump_level )
{
    GCO_OP	*pc = prog_buffer;
    GCO_OP	*size_pc;
    STATUS	status = OK;
    u_i2	length = 0;

    /*
    ** Check to see if already been compiled.
    ** Note: for the un-compiled condition, this 
    ** test is not thread-safe.  A subsequent 
    ** thread-safe test is made below to catch 
    ** race conditions.
    */
    if ( gco_ddt_map[ gco_type ] )  return( OK );

    /*
    ** Provide default for those with no descriptors.
    */
    if ( ! gco_ddt_ods[ gco_type ] )
    {
	gco_ddt_map[ gco_type ] = gco_prog_no_ddt;
	return( OK );
    }

    MUp_semaphore( &gco_semaphore );

    /*
    ** Check to see if already been compiled 
    ** (catch race conditions).
    */
    if ( gco_ddt_map[ gco_type ] )  
    {
	MUv_semaphore( &gco_semaphore );
	return( OK );
    }

# ifdef GCXDEBUG
    if ( gco_trace_level >= dump_level )
	gco_dumpobjdesc( gco_ddt_ods[ gco_type ], 0, 0 );
# endif /* GCXDEBUG */

    /*
    ** Compile the TPL descriptor.
    */
    gco_comp_od( gco_ddt_ods[ gco_type ], &pc, &length, 0, NULL );
    *pc++ = OP_RETURN;		/* Return to calling program */
    gco_ddt_len[ gco_type ] = length;

# ifdef GCXDEBUG
    if ( gco_trace_level >= 3 )
	TRdisplay( "gco_comp_ddt: type %s, size %d, compiled length %d\n", 
		   gcx_look( gcx_gco_type, gco_type ), 
		   (i4)length, pc - prog_buffer );
# endif /* GCXDEBUG */

    if ( (length = pc - prog_buffer) > GCO_L_MSG_MAX )
    {
	status = E_GC2414_PL_COMP_MSGS_EXCEED;
	gco_ddt_map[ gco_type ] = gco_prog_no_ddt;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 1 )
	    TRdisplay( "gco_comp_ddt: type %s, length %d too long!\n", 
		       gcx_look( gcx_gco_type, gco_type ), (i4)length );
# endif /* GCXDEBUG */
    }
    else
    {
	/* 
	** Copy program from temp buffer 
	*/
	gco_ddt_map[ gco_type ] = 
	    (GCO_OP *)MEreqmem( (u_i2)0, (u_i4)(length * sizeof( GCO_OP )),
				FALSE, &status );

	if ( gco_ddt_map[ gco_type ] )
	    MEcopy( (PTR)prog_buffer, length * sizeof( GCO_OP ),
		    (PTR)gco_ddt_map[ gco_type ] );

# ifdef GCXDEBUG
	/*
	** If the DDT length is significant, it will be pushed
	** on the value stack prior to calling the DDT program,
	** so set the initial argument length accordingly.
	*/
	if ( gco_trace_level >= dump_level )
	    gco_dumpprog( "DDT", prog_buffer, length,
		! (gco_ddt_ods[ gco_type]->flags & GCA_IGNLENGTH) ? 1 : 0 );
# endif /* GCXDEBUG */

    }

    MUv_semaphore( &gco_semaphore );
    return( status );
}


/*
** Name: gco_comp_dv - compile a data value descriptor
**
** Description:
**	Compile a GCA_ELEMENT describing any INGRES datatype contained
**	in a GCA_DATA_VALUE object.
**
**	A maximum of GCO_DV_OP_MAX operations (5 + GCO_TPL_OP_MAX) will 
**	be appended to the compiled program (in_pc).
**
** Inputs:
**	ela	GCA_ELEMENT with TPL of INGRES datatype
**	in_pc	data area for compiled program
**
** History:
**	25-May-95 (gordy)
**	    The raw data value is not structured, so tell gcc_comp_el()
**	    not to do any alignment padding.
**	 8-Mar-96 (gordy)
**	    Added OP_MARK/OP_REWIND to properly return only complete
**	    C data structures.
**	 3-Apr-96 (gordy)
**	    DV_BEGIN now simulates REWIND, explicit operation not required.
**	 5-Feb-97 (gordy)
**	    Call pre-compiled datatype programs using gco_comp_tpl().
**	22-Jan-10 (gordy)
**	    Add OP_DV_REF operation to permit meta-data length to be
**	    updated for certain string types whose length can be
**	    changed during multi-byte processing.
*/

VOID
gco_comp_dv( GCA_ELEMENT *ela, GCO_OP *in_pc )
{
    GCO_OP	*pc = in_pc;
    u_i2	sz_struct = 0;
    GCO_OP	*size_pc;

    /*
    ** Save meta-data length reference for certain
    ** character based types which may need to extend 
    ** the meta-data length to compensate for changes
    ** during character set translation.
    **
    ** This determination would best be made from the
    ** object descriptor information (GCO_ATOM_CHAR
    ** combined with GCA_VARTPLAR, GCA_VARVCHAR, or
    ** GCA_VARPADAR), but that info is not readily
    ** available at this level.  Since it can also
    ** be determined by the raw dbms type, it does
    ** not seem worthwhile to do anything more 
    ** complicated
    */
    switch( abs( ela->tpl.type ) )
    {
    case GCA_TYPE_C :
    case GCA_TYPE_CHAR :
    case GCA_TYPE_VCHAR :
    case GCA_TYPE_TXT :
    case GCA_TYPE_LTXT :	
    case GCA_TYPE_QTXT :
	*pc++ = OP_DV_REF;	/* Save meta-data length reference */
	break;
    }

    *pc++ = OP_DV_BEGIN;	/* Begin processing fixed GCA_DATA_VALUE */
    size_pc = pc++;		    /* arg1: object size */

    gco_comp_tpl( ela, &pc, &sz_struct, 0 );

    *size_pc = (GCO_OP)sz_struct;
    *pc++ = OP_DV_END;		/* Done processing GCA_DATA_VALUE */
    *pc++ = OP_RETURN;		/* Return to calling program */
	
# ifdef GCXDEBUG
    if ( gco_trace_level >= 1 )
    {
        if ( (pc - in_pc) > GCO_DV_OP_MAX )
	    TRdisplay( "gco_comp_dv: length %d too long!\n", (i4)(pc - in_pc) );
	else  if ( gco_trace_level >= 3 )
	    TRdisplay( "gco_comp_dv: %d instructions %d bytes\n", 
		       (i4)(pc - in_pc), (i4)sz_struct );
    }
# endif /* GCXDEBUG */

    return;
}


/*
** Name: gco_comp_tod - compile a tuple descriptor
**
** Description:
**	Compiles a GCA_OBJECT_DESC message object describing a row 
**	of tuple data.
**
**	A maximum of GCO_TUP_OP_MAX( attr-count ) operations will 
**	be appended to the compiled program (in_pc) (GCO_TPL_OP_MAX
**	operations per attribute plus an additional single op for 
**	the entire tuple).
**
** Inputs:
**	od	GCA_OBJECT_DESC describing tuple row
**	in_pc	data area for compiled program
**
** History:
**	25-May-95 (gordy)
**	    Tuples are processed as byte streams, so tell gcc_comp_od()
**	    not to add alignment padding.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 5-Feb-97 (gordy)
**	    Call pre-compiled datatype programs using gco_comp_tpl().
**	22-Mar-02 (gordy)
**	    Decompose message object using 'safe' macros.
**	22-Jan-10 (gordy)
**	    Check that compiled length does not exceed expected maximum.
*/

VOID
gco_comp_tod( PTR ptr, GCO_OP *in_pc )
{
    u_char	*od = (u_char *)ptr;
    GCO_OP	*pc = in_pc;
    i4		i, flags, count;
    u_i2	sz_struct = 0;

    od += GCA_OBJ_NAME_LEN;				/* data_object_name */
    od += GCA_GETI4_MACRO( od, &flags );		/* flags */
    od += GCA_GETI4_MACRO( od, &count );		/* el_count */

    flags = (flags & GCA_COMP_MASK) ? GCO_COMP_MASK : 0;

    for( i = 0; i < count; i++ )
    {
	GCA_ELEMENT	ela;
	i4		tmpi4;

	od += GCA_GETI2_MACRO( od, &ela.tpl.type );		/* type */
	od += GCA_GETI2_MACRO( od, &ela.tpl.precision );	/* precision */
	od += GCA_GETI4_MACRO( od, &ela.tpl.length );		/* length */
	od += GCA_GETI4_MACRO( od, &tmpi4 );			/* od_ptr */
	od += GCA_GETI4_MACRO( od, &ela.arr_stat );		/* arr_stat */

	ela.od_ptr = NULL;
	gco_comp_tpl( &ela, &pc, &sz_struct, flags );
    }

    *pc++ = OP_RETURN;		/* Return to calling program */

# ifdef GCXDEBUG
    if ( gco_trace_level >= 1 )
    {
        if ( (pc - in_pc) > GCO_TUP_OP_MAX(count) )
	    TRdisplay("gco_comp_tod: length %d too long!\n", (i4)(pc - in_pc));
	else if ( gco_trace_level >= 3 )
	    TRdisplay( "gco_comp_tod: %d instructions %d bytes\n", 
		       (i4)(pc - in_pc), (i4)sz_struct );
    }
# endif /* GCXDEBUG */

    return;
}


/*
** Name: gco_comp_tdescr - compile a GCA_TDESCR-type tuple descriptor
**
** Description:
**	Compile a GCA_TUP_DESCR message object describing a row 
**	of tuple data.
**
**	A maximum of ((3 + GCO_TPL_OP_MAX) * attr-count + 1) 
**	operations will be appended to the compiled program (in_pc).
**
** Inputs:
**	modifier	GCA tuple descriptor flags (gca_result_modifier).
**	count		Number of column descriptors.
**	desc		Array of column descriptors.
**	eod		End of data: TRUE if last descriptor, FALSE otherwise.
**	in_pc		Start/continuation of compiled program.
**
** Output:
**	in_pc		End of current program.
**
** Returns:
**	bool		TRUE if successful, FALSE if error.
**
** History:
**	25-May-95 (gordy)
**	    Tuples are still processed as a byte stream.  Tell
**	    gcc_comp_el() not to add alignment padding.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Extended interface so that large messages split across
**	    buffers can be processed is segments.
**	19-Mar-96 (gordy)
**	    Each element of a tuple is now treated as its own VARIMPAR,
**	    rather than the entire tuple.  This allows the processing
**	    of larger tuples/elements.  Note that the input loop normally
**	    associated with a VARIMPAR is kept at the tuple level.
**	 3-Apr-96 (gordy)
**	    We no longer BREAK after a tuple since caller must handle
**	    tuples spanning buffers and handling multiple tuples is
**	    hardly any additional work.
**	 5-Feb-97 (gordy)
**	    Call pre-compiled datatype programs using gcd_comp_tpl().
**	22-Mar-02 (gordy)
**	    Decompose message object using 'safe' macros.
**	22-Jan-10 (gordy)
**	    Check that compiled length does not exceed expected maximum.
*/

bool
gco_comp_tdescr
(
    i4		modifier, 
    i4		count, 
    PTR		ptr, 
    bool	eod,
    GCO_OP	**in_pc
)
{
    GCO_OP	*pc = *in_pc;
    u_i2	sz_struct = 0;
    i4		i;

    for( i = 0; i < count; i++ )
    {
	GCA_ELEMENT	ela;
	GCO_OP		*size_pc;
	u_i2		sz_sub_struct;
	i4		flags = (modifier & GCA_COMP_MASK) ? GCO_COMP_MASK : 0;
	i4		tmpi4;
	u_char		*col = (u_char *)ptr;

	col += GCA_GETI4_MACRO( col, &tmpi4 );		    /* db_data */
	col += GCA_GETI4_MACRO( col, &ela.tpl.length );	    /* db_length */
	col += GCA_GETI2_MACRO( col, &ela.tpl.type );	    /* db_datatype */
	col += GCA_GETI2_MACRO( col, &ela.tpl.precision );  /* db_prec */
	col += GCA_GETI4_MACRO( col, &tmpi4 );		    /* gca_l_attname */
	col += tmpi4;					    /* gca_attname */

	*pc++ = OP_MARK;	/* Mark object boundary */
	*pc++ = OP_IMP_BEGIN;	/* Process array element */
	size_pc = pc++;		    /* arg1: object size */
	sz_sub_struct = 0;

	ela.od_ptr = NULL;
	ela.arr_stat = GCA_NOTARRAY;
	gco_comp_tpl( &ela, &pc, &sz_sub_struct, flags );

	*size_pc = (GCO_OP)sz_sub_struct;
	sz_struct += sz_sub_struct;
    }

    if ( eod )  *pc++ = OP_RETURN;	/* Return to calling program */

# ifdef GCXDEBUG
    if ( gco_trace_level >= 1 )
    {
        if ( (pc - *in_pc) > ((count * (GCO_TPL_OP_MAX + 3)) + 1) )
	    TRdisplay( "gco_comp_tdescr: length %d too long!\n", 
	    		(i4)(pc - *in_pc));
	else  if ( gco_trace_level >= 3 )
	    TRdisplay("gco_comp_tdescr: %d columns %d bytes, %d instructions\n",
		      count, (i4)sz_struct, (i4)(pc - *in_pc));
    }
# endif /* GCXDEBUG */

    /*
    ** Update the original PC so that if
    ** the tuple descriptor is split, we
    ** can resume at the correct point in
    ** the program.
    */
    *in_pc = pc;

    return( TRUE );
}


/*
** Name: gco_comp_tpl
**
** Description:
**	Compiles the TPL of a GCA_ELEMENT describing a DBMS datatype.  
**	Determines the array status and object descriptor for the 
**	element based on the TPL values.
**
**	A maximum of GCO_TPL_OP_MAX operations (8) will be appended
**	to the compiled program (in_pc).
**
** Input:
**	ela		GCA_ELEMENT to compile
**	in_pc		Data area for compiled program
**	sz_struct	Size of containing object.
**	flags		Compilation options
**
** Output:
**	in_pc		End of compiled program.
**	sz_struct	Increased by size of object compiled.
**
** Returns:
**	VOID
**
** History:
**	28-Jan-97 (gordy)
**	    Created.
**	31-Mar-98 (gordy)
**	    Include long varchar in doublebyte special string handling.
**	22-Mar-02 (gordy)
**	    Don't need to check for atomic types.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code.  Don't return the 
**	    TPL length for truly variable length types (flagged with 
**	    GCA_VARLENGTH).
*/

static VOID
gco_comp_tpl( GCA_ELEMENT *ela, GCO_OP **in_pc, u_i2 *sz_struct, i4 flags )
{
    GCO_OP	*pc = *in_pc;
    i4		gco_type;

# ifdef GCXDEBUG
    if ( gco_trace_level >= 3 )
    {
	TRdisplay( "gco_comp_tpl: (%s, %d, %d)\n",
		   gcx_look( gcx_datatype, (i4)ela->tpl.type ),
		   (i4)ela->tpl.precision, ela->tpl.length );

	*pc++ = OP_DEBUG_TYPE;			/* Trace TPL info */
	*pc++ = (GCO_OP)ela->tpl.type;		    /* arg1: datatype (i2) */
	*pc++ = (GCO_OP)ela->tpl.precision;	    /* arg2: precision (i2) */
	*pc++ = (GCO_OP)ela->tpl.length;	    /* arg3: length (i4) */
    }
# endif /* GCXDEBUG */

    /* 
    ** Check for valid datatype.
    */
    if ( 
	 ! gco_map_ddt( ela, &gco_type, flags )	||
	 gco_type < 0  ||  gco_type >= gco_l_ddt_ods	||
	 ! gco_ddt_ods[ gco_type ] 
       )
    {
	*pc++ = OP_ERROR;	/* Return error */
	*pc++ = EOP_NOTPLOD;	    /* arg1: error ID */
	goto done;
    }

# ifdef GCXDEBUG
    if ( gco_trace_level >= 3 )
	TRdisplay( "gco_comp_tpl: %s\n",
		   gco_ddt_ods[ gco_type ]->data_object_name );
# endif /* GCXDEBUG */

    /*
    ** Check for precompiled program for this 
    ** object (compile if necessary).
    */
    if ( ! gco_ddt_map[ gco_type ]  &&
	 gco_comp_ddt( gco_type, 3 ) != OK )
    {
	*pc++ = OP_ERROR;	/* Return error */
	*pc++ = EOP_NOTPLOD;    /* arg1: error ID */
	goto done;
    }

    /*
    ** Fixed size types and truly variable length
    ** data types return the compiled object length.  
    ** Variable sized objects return the TPL length.  
    */
    if ( gco_ddt_ods[ gco_type ]->flags & (GCA_IGNLENGTH | GCA_VARLENGTH) )
	*sz_struct += gco_ddt_len[ gco_type ];
    else
	*sz_struct += (u_i2)ela->tpl.length;

    /*
    ** Push TPL length  of variable sized objects 
    ** (don't include the NULL byte).
    */
    if ( ! (gco_ddt_ods[ gco_type ]->flags & GCA_IGNLENGTH) )
    {
	*pc++ = OP_PUSH_LIT;			/* Push literal */
	*pc++ = GCO_TPL_LEN( &ela->tpl );	/* arg1: value */
    }

    *pc++ = OP_CALL_DDT;	/* Call datatype program */
    *pc++ = gco_type;		    /* arg1: internal type ID */

  done:

# ifdef GCXDEBUG
    if ( gco_trace_level >= 1  &&  (pc - *in_pc) > GCO_TPL_OP_MAX )
	TRdisplay( "gco_comp_tpl: %s, length %d too long!\n", 
		   gcx_look( gcx_datatype, (i4)ela->tpl.type ),
		   pc - *in_pc );
# endif /* GCXDEBUG */

    *in_pc = pc;
    return;
}


/*
** Name: gco_comp_od - compile a GCA_OBJECT_DESC
**
** Description:
**	Compiles a GCA_OBJECT_DESC by stepping through each element and
**	calling gco_comp_el().  Compresses runs of the elements of the
**	same type.  Handles the special cases for tuple data and
**	GCA_DATA_VALUES.
**
**	Data alignment is based on original offset of the complex object.
**	If the caller aligns the object, an offset of 0 may be used.
**
** Inputs:
**	od		GCA_OBJECT_DESC describing any complex object
**	in_pc		Data area for compiled program
**	offset		Offset of object, 0 if aligned.
**	flags		Compilation options.
**
** Output:
**	in_pc		End of compiled program.
**	offset		Increased by size of object compiled.
**	obj_align	Alignment type for object, may be NULL.
**
** History:
**	24-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42
**	25-May-95 (gordy)
**	    Pass alignment padding to gcc_comp_el().
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	19-Mar-96 (gordy)
**	    Add an input loop around tuple processing.  Tuples are no
**	    longer treated as a single VARIMPAR, which contained the
**	    input loop.
**	21-Jun-96 (gordy)
**	    Moved special processing for tuples to gcd_comp_msg().
**	 5-Feb-97 (gordy)
**	    Removed TPL length argument.  Length is now pushed on the
**	    data stack.  Use gcd_push_numeric() to generalize handling
**	    or GCA_DATA_VALUE.
**	 4-Mar-98 (gordy)
**	    Determine worst case alignment for object.  Add padding at
**	    end of object to ensure proper size.  Data values are stored
**	    separately and replaced by a pointer which is used for
**	    alignment rather than the data value.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code.  Save reference to
**	    data value length and update after processing.
*/

static VOID
gco_comp_od
( 
    GCA_OBJECT_DESC	*od, 
    GCO_OP		**in_pc, 
    u_i2		*offset, 
    i4			flags, 
    GCA_ELEMENT		*obj_align 
)
{
    GCA_ELEMENT *ela, *prev, *next;
    GCA_ELEMENT	align_type, ela_align;
    GCO_OP	*pc = *in_pc;
    u_i2	sz_struct = *offset;	/* include orig offset for alignment */

    align_type.tpl.type = GCO_ATOM_CHAR;	/* default alignment */
    align_type.tpl.length = 1;

    if ( od == gco_od_data_value )
    {
	/*
	** Type
	*/
	gco_comp_el( &od->ela[0], &pc, &sz_struct, flags, NULL, &ela_align );
	gco_cmp_align( &align_type, &ela_align, &align_type );

	if ( (*pc++ = gco_push_numeric( &od->ela[0], FALSE )) == OP_ERROR )
	    *pc++ = EOP_BADDVOD;	/* arg1: error ID */

	/*
	** Precision
	*/
	gco_comp_el( &od->ela[1], &pc, &sz_struct, flags, NULL, &ela_align );
	gco_cmp_align( &align_type, &ela_align, &align_type );

	if ( (*pc++ = gco_push_numeric( &od->ela[1], FALSE )) == OP_ERROR )
	    *pc++ = EOP_BADDVOD;	/* arg1: error ID */

	/*
	** Length
	*/
	gco_comp_el( &od->ela[2], &pc, &sz_struct, flags, NULL, &ela_align );
	gco_cmp_align( &align_type, &ela_align, &align_type );

	if ( (*pc++ = gco_push_numeric( &od->ela[2], TRUE )) == OP_ERROR )
	    *pc++ = EOP_BADDVOD;	/* arg1: error ID */

	/*
	** Value: VAREXPAR CHAR array is replaced by pointer.
	*/
	ela_align.tpl.type = GCO_ATOM_PTR;
	ela_align.tpl.length = 0;
	ela_align.tpl.precision = 0;
	gco_cmp_align( &align_type, &ela_align, &align_type );

	if ( flags & GCO_ALIGN_MASK )
	{
	    u_i2 pad;

	    if ( (pad = gco_align_pad( &ela_align, sz_struct )) )
	    {
		*pc++ = OP_PAD_ALIGN;		/* Add padding for alignment */
		*pc++ = pad;			    /* arg1: padding length */
		sz_struct += pad;
	    }
	}

	sz_struct += sizeof( char * );				/* varexpar */
	*pc++ = OP_CALL_DV;	/* Compile and call GCA_DATA_VALUE program */

	/*
	** Update actual length via reference.
	*/
	if ( (*pc++ = gco_update_ref( &od->ela[2] )) == OP_ERROR )
	    *pc++ = EOP_BADDVOD;	/* arg1: error ID */
    }
    else 
    {
	for( 
	     ela = od->ela, prev = NULL; 
	     ela < od->ela + od->el_count; 
	     prev = ela, ela = next 
	   )
	{
	    int n = 1;
	    next = ela + 1;

	    if ( ela->arr_stat == GCA_NOTARRAY  &&  ! ela->od_ptr )
		while( 
		       next < od->ela + od->el_count  &&
		       next->arr_stat == GCA_NOTARRAY  &&
		       next->tpl.type == ela->tpl.type  &&
		       next->tpl.precision == ela->tpl.precision  &&
		       next->tpl.length == ela->tpl.length 
		     )
		{
		    next++;
		    n++;
		}
		
	    if ( n == 1 )
		gco_comp_el( ela, &pc, &sz_struct, flags, prev, &ela_align );
	    else
	    {
		ela->arr_stat = n;
		gco_comp_el( ela, &pc, &sz_struct, flags, prev, &ela_align );
		ela->arr_stat = GCA_NOTARRAY;
	    }

	    gco_cmp_align( &align_type, &ela_align, &align_type );
	}
    }

    sz_struct -= *offset;	/* remove original offset */

    if ( flags & GCO_ALIGN_MASK )
    {
	u_i2 pad;

	/*
	** Structure alignment is based on the worst case alignment
	** of the structure elements.  Structures must also be padded
	** so that an array of structures will be aligned properly.
	** Since the following structure must be aligned for the worst
	** case element, we add padding to align such an element as
	** if it followed the current structure.
	*/
	if ( (pad = gco_align_pad( &align_type, sz_struct )) )
	{
	    *pc++ = OP_PAD_ALIGN;		/* Add padding for structure */
	    *pc++ = pad;			    /* arg1: padding length */
	    sz_struct += pad;
	}
    }

    if ( obj_align )  STRUCT_ASSIGN_MACRO( align_type, *obj_align );
    *offset += sz_struct;
    *in_pc = pc;
    return;
}


/*
** Name: gco_comp_list
**
** Description:
**	Compiles a list object.  A list is made up of one or more
**	elements.  Elements contain an indicator and an object
**	(atomic or complex).  The last element (and only the last
**	element) has an indicator value of zero and the object
**	is assumed to have a length of 0.  All elements but the
**	last have a non-zero indicator and a non-zero length object.
**
** Input:
**	ela		GCA_ELEMENT to compile.
**	in_pc		Data area for compiled program.
**	flags		GCO processing flags.
**
** Output:
**	in_pc		New position in program.
**
** Returns:
**	u_i2		Length of segment for parent object.
**
** History:
**	 3-Apr-96 (gordy)
**	    Created.
**	29-Jan-97 (gordy)
**	    Removed the TUPLE compilation scheme since the formatted
**	    interface no longer processes individual attributes of a
**	    tuple.  Removed the default compilation scheme since it 
**	    was only used by gcd_comp_tod() for HET processing and the
**	    only differences between the default and DATA VALUE schemes 
**	    are no-op's in HET conversion code.
**	30-Jan-97 (gordy)
**	    Generalized and renamed to gcd_comp_list().  Removed segment
**	    specific processing.  List element may be any object.
**	22-Jan-10 (gordy)
**	    Bracket segment processing with OP_SEG_BEG and OP_SEG_END
**	    operations.
*/

static u_i2
gco_comp_list( GCA_ELEMENT *ela, GCO_OP **in_pc, i4  flags )
{
    GCO_OP	*pc = *in_pc;
    GCO_OP	*top, *offset;
    u_i2	sz_struct = 0;

    /*
    ** A list is made up of an indicator (zero for 
    ** end-of-list, non-zero for object present) 
    ** and an object.
    */
    if ( ! ela->od_ptr  ||  ela->od_ptr->el_count != 2 )
    {
	*pc++ = OP_ERROR;	/* Return error */
	*pc++ = EOP_VARLSTAR;	    /* arg1: error ID */
	*in_pc = pc;
	return( 0 );
    }

    *pc++ = OP_SEG_BEGIN;	/* Begin list segments */

    /*
    ** Process the indicator.
    */
    top = pc;
    gco_comp_el( &ela->od_ptr->ela[0], &pc, &sz_struct, flags, NULL, NULL );

    /*
    ** If the indicator is 0 we have reached
    ** the end of the list: branch to end.
    */
    if ( (*pc++ = gco_push_numeric( &ela->od_ptr->ela[0], FALSE )) == OP_ERROR )
	*pc++ = EOP_VARLSTAR;	    /* arg1: error ID */

    *pc++ = OP_JZ;	/* Exit if indicator is top-of-stack is zero */
    offset = pc++;	    /* arg1: pc offset */

    /*
    ** Process the list object.
    */
    gco_comp_el( &ela->od_ptr->ela[1], &pc, &sz_struct, flags, NULL, NULL );

    /*
    ** List objects are returned one per GCA call 
    ** during formatted processing.  To do this, 
    ** we insert a BREAK at the end of the list.  
    */
    *pc++ = OP_BREAK;		/* Return object, reset pointers */
    *pc++ = OP_MARK;		/* Mark object boundary */
    *pc++ = OP_GOTO;		/* Process next object */
    *pc++ = 0;			    /* arg1: pc offset */

    *offset = pc - (offset + 1);
    pc[-1] = top - pc;
    *pc++ = OP_SEG_END;		/* End of list segments */

    *in_pc = pc;
    return( sz_struct );
}



/*
** Name: gco_comp_el - compile a GCA_ELEMENT
**
** Description:
**	Compiles a GCA_ELEMENT.  Calls gco_comp_atomic for atomic 
**	elements (with null od_ptr's) and calls gco_comp_od() for
**	complex elements.  Brackets both with whatever array 
**	handling instructions are necessary.
**
** Input:
**	ela		GCA_ELEMENT to compile
**	in_pc		Data area for compiled program
**	sz_struct	Size of containing object.
**	flags		Compilation options
**	prev		Preceding GCA_ELEMENT.
**
** Output:
**	in_pc		End of compiled program.
**	sz_struct	Increased by size of object compiled.
**	align_type	Alignment datatype for element, may be NULL.
**
** History:
**	24-mar-95 (peeje01)
**	    Cross integration from doublebyte label 6500db_su4_us42
**	25-May-95 (gordy)
**	    Long segment indicator now an element of GCA_VARSEGAR.
**	    GCA_VARSEGAR elements processed directly to handle skip
**	    logic after indicator.  Segment data now GCA_VARVSEGAR
**	    which is similar to GCA_VARVCHAR but with no padding.
**	    Only add alignment padding when requested.
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Added OP_MARK/OP_REWIND to properly return only complete
**	    C data structures.
**	 3-Apr-96 (gordy)
**	    Extracted BLOB segment processing to gcd_comp_segment().
**	    MARK now precedes location to be marked.  VAR_BEGIN now
**	    emulates REWIND so explicit usage removed.
**	21-Jun-96 (gordy)
**	    Added GCA_VARMSGAR for messages which are processed in bulk
**	    rather than as formatted elements.
**	 5-Feb-97 (gordy)
**	    Removed TPL length argument.  Length now pushed on the
**	    data stack.  Combined/simplified the array status types.
**	24-Feb-97 (gordy)
**	    Added previous element argument for array types which are
**	    preceded by a length count.
**	 4-Mar-98 (gordy)
**	    Return alignment of elements (for structures, the worst
**	    case alignment is returned).  Data structures require
**	    padding for alignment similar to atomic elements.  For
**	    VAREXPAR elements, alignment is based on the embedded
**	    pointer to the element, not the element itself.  If any
**	    padding is added for the VAREXPAR pointer, be sure the
**	    array length (integer element preceding VAREXPAR array)
**	    is pushed prior to padding being added.
**	 1-Mar-01 (gordy)
**	    OP_PUSH_VCH now takes size of array element as argument.
**	    For TPL arrays, if the element size is not 1, need to
**	    convert TPL byte length to number of elements.
**	 3-May-01 (gordy)
**	    OP_DIVIDE must be done prior to conversion of the data.
**	22-Jan-10 (gordy)
**	    Added VARPADAR array type.  Added special character
**	    processing to handle multi-byte charsets.
*/

static VOID
gco_comp_el( GCA_ELEMENT *ela, GCO_OP **in_pc, u_i2  *sz_struct, 
	     i4  flags, GCA_ELEMENT *prev, GCA_ELEMENT *obj_align )
{
    GCA_ELEMENT	align_type;
    GCO_OP	*top, *size_pc;
    GCO_OP	*pc = *in_pc;
    u_i2	sz_sub_struct = 0;
    u_i2	mul_sub_struct = 0;
    bool	length_pushed = FALSE;

    if ( ela->od_ptr )			/* Complex type */
    {
	GCO_OP	*type_pc, *len_pc, *align_pad = NULL;

	align_type.tpl.type = GCO_ATOM_CHAR;	/* Default alignment */
	align_type.tpl.length = 1;

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )  *pc++ = OP_DOWN;	/* Descend into complex type */
# endif /* GCXDEBUG */

	/*
	** Structure may need to be aligned.  Alignment
	** must be determined from elements in structure,
	** so reserve place to add alignment later.
	*/
	if ( flags & GCO_ALIGN_MASK )
	    switch( ela->arr_stat )
	    {
		case GCA_VARZERO:	/* No alignment needed */
		case GCA_VARLSTAR:
		case GCA_VARSEGAR:
		case GCA_VARTPLAR: 
		case GCA_VARVCHAR:
		case GCA_VARPADAR:
		    break;

		case GCA_VAREXPAR:
		    /*
		    ** Need to push the array length 
		    ** prior to adding padding.
		    */
		    length_pushed = TRUE;
		    if ( (*pc++ = gco_push_numeric( prev, FALSE )) == OP_ERROR )
			*pc++ = EOP_NOT_INT;	/* arg1: error ID */

		    /*
		    ** Fall through for padding
		    */

		case GCA_NOTARRAY:	/* Align object (reserve space) */
		case GCA_VARIMPAR: 
		default:
		    *pc++ = OP_PAD_ALIGN;
		    align_pad = pc++;
		    break;
	    }

	switch( ela->arr_stat )
	{
	case GCA_VARZERO:
	    /*
	    ** Do nothing.
	    */
	    sz_sub_struct = 0;
	    mul_sub_struct = 0;
	    break;
	    
	case GCA_NOTARRAY:
	    /*
	    ** Process single object.
	    */
	    gco_comp_od(ela->od_ptr, &pc, &sz_sub_struct, flags, &align_type);
	    mul_sub_struct = 1;
	    break;

	case GCA_VARIMPAR: 
	    /*
	    ** Array of objects, length determined
	    ** by length of message.  Loop processing
	    ** each element until input is depleted.
	    */
	    top = pc;
	    *pc++ = OP_JNINP;		/* Exit at end-of-input */
	    *pc++ = 0;			    /* arg1: pc offset */
	    *pc++ = OP_MARK;		/* Mark object boundary */
	    *pc++ = OP_IMP_BEGIN;	/* Process array element */
	    size_pc = pc++;		    /* arg1: object size */

	    gco_comp_od(ela->od_ptr, &pc, &sz_sub_struct, flags, &align_type);
	    *size_pc = (GCO_OP)sz_sub_struct;

	    *pc++ = OP_GOTO;		/* Loop for next array element */
	    *pc++ = 0;			    /* arg1: pc offset */
	    top[1] = pc - (top + 2);
	    pc[-1] = top - pc;
	    sz_sub_struct = 0;		/* Not part of fixed length of object */
	    mul_sub_struct = 0;
	    break;

	case GCA_VAREXPAR:
	    /*
	    ** This array must be preceded by an atomic 
	    ** integer which specifies the length of the 
	    ** array.  This array is not contained in data 
	    ** structures but is replaced by a pointer to
	    ** the actual array (the array IS embedded
	    ** directly in the data stream).  The array 
	    ** length is pushed (if not done above).
	    */
	    if ( ! length_pushed  && 
		 (*pc++ = gco_push_numeric( prev, FALSE )) == OP_ERROR )
		*pc++ = EOP_NOT_INT;	/* arg1: error ID */

	    *pc++ = OP_VAR_BEGIN;	/* Process array element */
	    size_pc = pc++;		    /* arg1: object size */
	    type_pc = pc++;		    /* arg2: Worst case align type */
	    len_pc = pc++;		    /* arg3: size of align type */
	    top = pc;
	    *pc++ = OP_GOTO;		/* Test element count (allow for 0) */
	    *pc++ = 0;			    /* arg1: pc offset */

	    gco_comp_od(ela->od_ptr, &pc, &sz_sub_struct, flags, &align_type);
	    *size_pc = (GCO_OP)sz_sub_struct;
	    *type_pc = align_type.tpl.type;
	    *len_pc = align_type.tpl.length;

	    *pc++ = OP_DJNZ;		/* Decrement top-of-stack, loop if !0 */
	    *pc++ = 0;			    /* arg1: pc offset */
	    top[1] = (pc - 2) - (top + 2);
	    pc[-1] = (top + 2) - pc;
	    *pc++ = OP_VAR_END;		/* End of array */

	    /*
	    ** Structure array replaced by pointer.
	    */
	    align_type.tpl.type = GCO_ATOM_PTR;
	    align_type.tpl.length = 0;
	    sz_sub_struct = sizeof(char *);
	    mul_sub_struct = 1;
	    break;

	case GCA_VARLSTAR:
	    /*
	    ** Array of complex objects comprised of
	    ** an indicator and sub-object (atomic or
	    ** complex).  The length of the list is
	    ** determined at runtime.
	    */
	    sz_sub_struct = gco_comp_list( ela, &pc, flags );
	    mul_sub_struct = 1;		/* One element returned per GCA call */
	    break;
	

	case GCA_VARSEGAR:
	case GCA_VARTPLAR: 
	case GCA_VARVCHAR:
	case GCA_VARPADAR:
	    /*
	    ** Not allowed for complex objects.
	    */
	    *pc++ = OP_ERROR;		/* Return error */
	    *pc++ = EOP_ARR_STAT;	    /* arg1: error ID */
	    sz_sub_struct = 0;
	    mul_sub_struct = 0;
	    break;

	default:
	    /*
	    ** Process object array (arr_stat >= 2).
	    */
	    *pc++ = OP_PUSH_LIT;	/* Push literal */
	    *pc++ = (GCO_OP)ela->arr_stat;  /* arg1: value (i4) */
	    top = pc;
	    *pc++ = OP_GOTO;		/* Test count (allow for 0 */
	    *pc++ = 0;			    /* arg1: pc offset */

	    gco_comp_od(ela->od_ptr, &pc, &sz_sub_struct, flags, &align_type);

	    *pc++ = OP_DJNZ;		/* Decrement top-of-stack, loop if !0 */
	    *pc++ = 0;			    /* arg1: pc offset */
	    top[1] = ( pc - 2 ) - ( top + 2 );
	    pc[-1] = ( top + 2 ) - pc;
	    mul_sub_struct = (u_i2)ela->arr_stat;
	    break;
	}

	/*
	** If a place has been reserved for structure
	** alignment padding, determine what padding
	** is needed (may be 0).
	*/
	if ( align_pad )
	{
	    /*
	    ** Structure alignment is based on the worst case
	    ** alignment of the structure elements.
	    */
	    u_i2 pad = gco_align_pad( &align_type, *sz_struct );

	    *align_pad = pad;
	    *sz_struct += pad;
	}

# ifdef GCXDEBUG
	if ( gco_trace_level >= 3 )  *pc++ = OP_UP;	/* Ascend from complex object */
# endif /* GCXDEBUG */
    }
    else				/* Atomic type */
    {
	/*
	** Charset processing can change the data
	** length of character types requiring
	** special processing.
	*/
	bool	is_char = (ela->tpl.type == GCO_ATOM_CHAR);

	STRUCT_ASSIGN_MACRO( *ela, align_type );

	/*
	** If processing a data structure, check
	** to see if need to do data alignment.
	** Stream data is not aligned.
	*/
	if ( flags & GCO_ALIGN_MASK )
	{
	    u_i2 pad;

	    if ( ela->arr_stat != GCA_VAREXPAR )
		pad = gco_align_pad( ela, *sz_struct );
	    else
	    {
		/*
		** VAREXPAR arrays are replaced by a pointer to
		** the actual storage, so need to align a pointer.
		** If padding is required, the array length must
		** be pushed first.
		*/
		align_type.tpl.type = GCO_ATOM_PTR;
		align_type.tpl.length = 0;

		if ( (pad = gco_align_pad( &align_type, *sz_struct )) )
		{
		    length_pushed = TRUE;
		    if ( (*pc++ = gco_push_numeric(prev, is_char)) == OP_ERROR )
			*pc++ = EOP_NOT_INT;	/* arg1: error ID */
		}
	    }

	    if ( pad )
	    {
		*pc++ = OP_PAD_ALIGN;		/* Add padding for alignment */
		*pc++ = pad;			    /* arg1: padding length */
		*sz_struct += pad;
	    }
	}

	switch( ela->arr_stat )
	{
	case GCA_VARZERO:
	    /*
	    ** Do nothing.
	    */
	    sz_sub_struct = 0;
	    mul_sub_struct = 0;
	    break;

	case GCA_VARIMPAR:
	    /*
	    ** Message length determines array length.
	    ** Process array elements in current buffer
	    ** as a group and loop to get additional
	    ** buffers until end of message.
	    */
	    top = pc;
	    *pc++ = OP_JNINP;		/* Exit at end-of-input */
	    *pc++ = 0;			    /* arg1: pc offset */
	    *pc++ = OP_RSTR_MSG;	/* Restore unprocessed input */
	    *pc++ = OP_MARK;		/* Mark object boundary */

	    if ( is_char )
	    {
		*pc++ = OP_CHAR_MSG;	/* Copy character message */
		gco_comp_atomic( ela, &pc );

		/*
		** The input length pushed by OP_{CHAR,COPY}_MSG is not
		** consumed during character processing.  We don't need
		** the input length since OP_SAVE_MSG will handle any
		** remaining input and we don't want to discard the input
		** itself as would be done with OP_SKIP_INPUT, so just
		** discard (pop) the input length.
		*/
		*pc++ = OP_POP;		/* Discard input length */
	    }
	    else
	    {
		*pc++ = OP_COPY_MSG;	/* Copy message objects */
		size_pc = pc++;		    /* arg1: object size */
		*size_pc = (GCO_OP)gco_comp_atomic( ela, &pc );
	    }

	    *pc++ = OP_SAVE_MSG;	/* Save unprocessed input */
	    *pc++ = OP_GOTO;		/* Loop until end-of-input */
	    *pc++ = 0;			    /* arg1: pc offset */
	    top[1] = pc - (top + 2);
	    pc[-1] = top - pc;

	    /*
	    ** Restore any unprocessed input so that the
	    ** enclosing message processing can detect
	    ** the excess.
	    */
	    *pc++ = OP_RSTR_MSG;	/* Restore unprocessed input */

	    sz_sub_struct = 0;
	    mul_sub_struct = 0;
	    break;

	case GCA_VAREXPAR:
	    /*
	    ** This array must be preceded by an atomic
	    ** integer which specifies the length of the 
	    ** array.  This array is not contained in data 
	    ** structures but is replaced by a pointer to
	    ** the actual array (the array IS embedded
	    ** directly in the data stream).  Push the
	    ** length of the array on the stack and save
	    ** a reference (for character data) to the
	    ** array length.
	    */
	    if ( ! length_pushed  &&
		 (*pc++ = gco_push_numeric( prev, is_char )) == OP_ERROR )
		*pc++ = EOP_NOT_INT;	/* arg1: error ID */

	    *pc++ = OP_VAR_BEGIN;	/* Process array element */
	    size_pc = pc++;		    /* arg1: object size */
	    *pc++ = ela->tpl.type;	    /* arg2: alignment type */
	    *pc++ = ela->tpl.length;	    /* arg3: length of align type */

	    *size_pc = (GCO_OP)gco_comp_atomic( ela, &pc );

	    if ( is_char )
	    {
		/*
		** Input length is not consumed during character
		** processing.  This type should not leave any
		** remaining input, but skipping input, just in 
		** case, will also consume the input length.
		*/
		*pc++ = OP_SKIP_INPUT;

		/*
		** Update actual length via reference.
		*/
		if ( (*pc++ = gco_update_ref( prev )) == OP_ERROR )
		    *pc++ = EOP_NOT_INT;	/* arg1: error ID */
	    }

	    *pc++ = OP_VAR_END;		/* End of array */
	    align_type.tpl.type = GCO_ATOM_PTR;
	    align_type.tpl.length = 0;
	    sz_sub_struct = sizeof( char * );
	    mul_sub_struct = 1;
	    break;

	case GCA_VARTPLAR: 
	    /*
	    ** The caller is responsible for pushing the
	    ** array length on the stack before calling us
	    ** (generally the TPL length).  Adjust byte
	    ** length to number of elements and push the
	    ** padding length (0).
	    */
	    *pc++ = OP_DIVIDE;		/* Calculate number of elements */
	    size_pc = pc++;			/* arg1: element size */
	    if ( is_char )  *pc++ = OP_CHAR_LIMIT;	/* No padding */

	    sz_sub_struct = gco_comp_atomic( ela, &pc );
	    mul_sub_struct = 1;		/* size of 1 element, caller has TPL */
	    *size_pc = (GCO_OP)sz_sub_struct;

	    if ( is_char )
	    {
		/*
		** Skip remaining input resulting from
		** expanding character data to fill output.
		** If character processing has reduced the
		** output length, compensate by padding to
		** the fixed string length.
		*/
		*pc++ = OP_SKIP_INPUT;
		*pc++ = OP_PAD;
	    }
	    break;

	case GCA_VARVCHAR:
	    /*
	    ** The caller is responsible for pushing the
	    ** array length on the stack before calling us
	    ** (generally the TPL length).  This element
	    ** must be preceded by an atomic integer which
	    ** specifies the actual length of the data.
	    ** Push the actual data length on the stack
	    ** and save a reference (for character data)
	    ** to the data length.
	    */
	    if ( (*pc++ = gco_push_numeric( prev, is_char )) == OP_ERROR )
		*pc++ = EOP_NOT_INT;	/* arg1: error ID */

	    /*
	    ** Since the size is not known until runtime, 
	    ** buffer allocations need to be adjusted
	    ** once the array length is available.
	    */
	    *pc++ = OP_DV_ADJUST;	/* Adjust buffer allocation */

	    /*
	    ** Pop data length and max length from stack. 
	    ** Compute length of padding.  Push padding 
	    ** length and data length back on stack.
	    */
	    *pc++ = OP_PUSH_VCH;	/* Calculate lengths for varchar */
	    size_pc = pc++;			/* arg1: element size */

	    *size_pc = gco_comp_atomic( ela, &pc );
	    sz_sub_struct = 0;		/* size not known until runtime */
	    mul_sub_struct = 0;

	    if ( is_char )
	    {
		/*
		** Discard any remaining input resulting
		** from expanding characters to fill output.
		*/
		*pc++ = OP_SKIP_INPUT;

		/*
		** Update actual length via reference.
		*/
		if ( (*pc++ = gco_update_ref( prev )) == OP_ERROR )
		    *pc++ = EOP_NOT_INT;	/* arg1: error ID */

		/*
		** While compressed types are unpadded, it is still
		** necessary to compensate for the change in data
		** length.  This ensures that changes in meta-data 
		** length are calculated correctly.  
		*/
		*pc++ = OP_ADJUST_PAD;
	    }

	    /*
	    ** Discard pad length.
	    */
	    *pc++ = OP_POP;
	    break;

	case GCA_VARPADAR:
	    /*
	    ** The caller is responsible for pushing the
	    ** array length on the stack before calling us
	    ** (generally the TPL length).  This element
	    ** must be preceded by an atomic integer which
	    ** specifies the actual length of the data.
	    ** Push the actual data length on the stack
	    ** and save a reference (for character data)
	    ** to the data length.
	    */
	    if ( (*pc++ = gco_push_numeric( prev, is_char )) == OP_ERROR )
		*pc++ = EOP_NOT_INT;	/* arg1: error ID */

	    /*
	    ** Pop data length and max length from stack. 
	    ** Compute length of padding.  Push padding 
	    ** length and data length back on stack.
	    */
	    *pc++ = OP_PUSH_VCH;	/* Calculate lengths for varchar */
	    size_pc = pc++;			/* arg1: element size */

	    /*
	    ** Process the data. Data and pad length 
	    ** are on stack.  Data length is removed.
	    */
	    sz_sub_struct = gco_comp_atomic( ela, &pc );
	    *size_pc = (GCO_OP)sz_sub_struct;
	    mul_sub_struct = 1;		/* size of 1 element, caller has TPL */

	    if ( is_char )
	    {
		/*
		** Discard any remaining input resulting
		** from expanding characters to fill output.
		*/
		*pc++ = OP_SKIP_INPUT;

		/*
		** Update actual length via reference
		** and compensate by adjusting padding.
		*/
		if ( (*pc++ = gco_update_ref( prev )) == OP_ERROR )
		    *pc++ = EOP_NOT_INT;	/* arg1: error ID */
	    }

	    /*
	    ** Process the padding (length on stack).
	    */
	    *pc++ = OP_CV_PAD; 
	    break;

	case GCA_VARSEGAR:
	    /*
	    ** This element must be preceded by an atomic 
	    ** integer which specifies the length of the 
	    ** array.  Push the array length on the stack
	    ** and save a reference (for character data)
	    ** to the array length.
	    */
	    if ( (*pc++ = gco_push_numeric( prev, is_char )) == OP_ERROR )
		*pc++ = EOP_NOT_INT;	/* arg1: error ID */

	    /*
	    ** Since the size is not known until runtime, 
	    ** buffer allocations need to be adjusted
	    ** once the array length is available.
	    */
	    *pc++ = OP_DV_ADJUST;	/* Adjust buffer allocation */

	    /*
	    ** Restore input carried over from previous segment.
	    */
	    if ( is_char )
	    {
		top = pc;	/* Branch target to finish processing */
		*pc++ = OP_RSTR_CHAR;
	    }

	    gco_comp_atomic( ela, &pc );
	    sz_sub_struct = 0;		/* size not known until runtime */
	    mul_sub_struct = 0;

	    if ( is_char )
	    {
		/*
		** Save any remaining input for next segment.
		** Check for end-of-segments and branch to
		** finish processing final segment.
		**
		** There's a bit of a kludge here.  At the
		** segment level we really don't know anything
		** about the list indicator but we have to
		** check it so that final processing can be
		** done.  At the list level, we can't properly
		** insert the check in the midst of the segment
		** processing.
		*/
		*pc++ = OP_SAVE_CHAR;
		*pc++ = OP_SEG_CHECK;
		*pc++ = 0;		    /* arg1: pc offset */
		pc[-1] = top - pc;

		/*
		** Update actual length via reference.
		*/
		if ( (*pc++ = gco_update_ref( prev )) == OP_ERROR )
		    *pc++ = EOP_NOT_INT;    /* arg1: error ID */
	    }
	    break;

	case GCA_VARLSTAR:
	    /*
	    ** Not allowed for atomic objects.
	    */
	    *pc++ = OP_ERROR;		/* Return error */
	    *pc++ = EOP_ARR_STAT;	    /* arg1: error ID */
	    sz_sub_struct = 0;
	    mul_sub_struct = 0;
	    break;

	case GCA_NOTARRAY:		/* Process single atom. */
	default:
	    *pc++ = OP_PUSH_LIT;			/* Push literal */
	    *pc++ = (GCO_OP)ela->arr_stat;		/* arg1: value (i4) */
	    if ( is_char )  *pc++ = OP_CHAR_LIMIT;	/* No padding */

	    sz_sub_struct = gco_comp_atomic( ela, &pc );
	    mul_sub_struct = (u_i2)ela->arr_stat;

	    if ( is_char )  
	    {
		/*
		** Discard any remaining input resulting
		** from expanding characters to fill output.
		** If character processing has reduced the
		** output length, compensate by padding to
		** the fixed string length.
		*/
		*pc++ = OP_SKIP_INPUT;
	    	*pc++ = OP_PAD;
	    }
	    break;
	}
    }

    if ( obj_align )  STRUCT_ASSIGN_MACRO( align_type, *obj_align );
    *sz_struct += mul_sub_struct * sz_sub_struct;
    *in_pc = pc;

    return;
}


/*
** Name: gco_comp_atomic
**
** Description:
**	Compiles an atomic element by generating the appropriate
**	intructions and returns the size of the element.
**
** Input:
**	ela		GCA_ELEMENT to compile
**	in_pc		Data area for compiled program
**
** Output:
**	none
**
** Returns:
**	u_i2		Size of element.
**
** History:
**	 1-Feb-96 (gordy)
**	    Support compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Added functionality of gcd_size_atomic().
**	 5-Feb-97 (gordy)
**	    OP_COMP_PAD no longer required.  Compressed variable
**	    length datatypes now represented by their own object
**	    descriptors.
**	22-Mar-02 (gordy)
**	    Removed unneeded atomic types.
**	15-Mar-04 (gordy)
**	    Support eight-byte integers.
**	22-Jan-10 (gordy)
**	    Generalized double-byte processing code.
*/

static u_i2
gco_comp_atomic( GCA_ELEMENT *ela, GCO_OP **in_pc )
{
    GCO_OP	*pc = *in_pc;
    GCO_OP	*top;
    u_i2	size;

# ifdef GCXDEBUG
    if ( gco_trace_level >= 3 )  *pc++ = OP_DEBUG_BUFS;	/* Trace buffers */
# endif /* GCXDEBUG */

    switch( ela->tpl.type )
    {
	case GCO_ATOM_INT:
	    size = (u_i2)ela->tpl.length;
	    switch( ela->tpl.length )
	    {
		case CV_L_I1_SZ:	*pc++ = OP_CV_I1; break;
		case CV_L_I2_SZ:	*pc++ = OP_CV_I2; break;
		case CV_L_I4_SZ:	*pc++ = OP_CV_I4; break;
		case CV_L_I8_SZ:	*pc++ = OP_CV_I8; break;
	    }
	    break;

	case GCO_ATOM_FLOAT:
	    size = (u_i2)ela->tpl.length;
	    switch( ela->tpl.length )
	    {
		case CV_L_F4_SZ:	*pc++ = OP_CV_F4; break;
		case CV_L_F8_SZ:	*pc++ = OP_CV_F8; break;
	    }
	    break;

	case GCO_ATOM_BYTE:	
	    size = sizeof( char );
	    *pc++ = OP_CV_BYTE; 
	    break;

	case GCO_ATOM_CHAR:
	    size = sizeof( char );
	    *pc++ = OP_CV_CHAR;
	    break;

	default:		
	    if ( gco_trace_level >= 1 )
		TRdisplay( "gco_comp_atomic: unknown atomic type 0x%x\n", 
			   ela->tpl.type );
	    size = 0;
	    *pc++ = OP_ERROR;		/* Return error */
	    *pc++ = EOP_NOT_ATOM;	    /* arg1: error ID */
	    break;
    }

    *in_pc = pc;
    return( size );
}



/*
** Name: gco_push_numeric
**
** Description:
**	Returns the OP code for pushing a numeric value
**	based on its type and size.  In addition to the
**	numeric value, a reference to the value may also
**	be saved to permit updating the value in a later
**	operation.
**
** Input:
**	ela		Element containing TPL of object to push.
**	ref		Save reference to numeric value.
**
** Output:
**	none
**
** Returns:
**	GCO_OP		OP_PUSH_* or OP_ERROR.
**
** History:
**	25-May-95 (gordy)
**	    Created.
**	24-Feb-97 (gordy)
**	    Changed parameter to GCA_ELEMENT since we are always
**	    pushing a preceding numeric element.
**	22-Jan-10 (gordy)
**	    Added ref parameter to select reference operations.
*/

static GCO_OP
gco_push_numeric( GCA_ELEMENT *ela, bool ref )
{
    GCO_OP op = OP_ERROR;

    if ( ela )
	switch( ela->tpl.type )
	{
	    case GCO_ATOM_INT :
		switch( ela->tpl.length )
		{
		case 2 : op = ref ? OP_PUSH_REFI2 : OP_PUSH_I2;	break;
		case 4 : op = ref ? OP_PUSH_REFI4 : OP_PUSH_I4;	break;
		}
		break;
	}

    return( op );
}


/*
** Name: gco_update_ref
**
** Description:
**	Returns the OP code for updating a numeric value
**	via reference based on its type and size.
**
** Input:
**	ela		Element containing TPL of object to push.
**
** Output:
**	none
**
** Returns:
**	GCO_OP		OP_UPD_* or OP_ERROR.
**
** History:
**	22-Jan-10 (gordy)
**	    Created.
*/

static GCO_OP
gco_update_ref( GCA_ELEMENT *ela )
{
    GCO_OP op = OP_ERROR;

    if ( ela )
	switch( ela->tpl.type )
	{
	    case GCO_ATOM_INT :
		switch( ela->tpl.length )
		{
		case 2 : op = OP_UPD_REFI2;	break;
		case 4 : op = OP_UPD_REFI4;	break;
		}
		break;
	}

    return( op );
}


/*
** Name: gco_map_ddt
**
** Description:
**	Return the internal GCO datatype for a supported DBMS datatype.
**
** Input:
**	ela		A GCA element.
**	flags		Compilation flags.
**
** Ouptut:
**	gco_type	The GCO datatype
**
** Returns:
**	bool		TRUE if successful, FALSE if unsupported datatype.
**
** History:
**	27-Jan-97 (gordy)
**	    Created.
**	 1-Mar-01 (gordy)
**	    Added support for new DBMS Unicode datatypes.
**	15-Mar-04 (gordy)
**	    Added support for eight-byte integers.
**	31-May-06 (gordy)
**	    Added support for ANSI date/time types.
**	24-Oct-06 (gordy)
**	    Added support for Blob/Clob locator types.
**	 9-Oct-09 (gordy)
**	    Added support for Boolean type.
*/

static bool	
gco_map_ddt( GCA_ELEMENT *ela, i4  *gco_type, i4  flags )
{
    i4		type;
    i4		length = ela->tpl.length;
    bool	nullable = (ela->tpl.type < 0);
    bool	success = TRUE;

    /*
    ** Nullable types include the NULL indicator
    ** byte as part of the length.
    */
    if ( nullable )  length--;

    switch( abs( ela->tpl.type ) )
    {
	/* Numeric types */

	case GCA_TYPE_INT :
	    switch( length )
	    {
		case 1 :	type = GCO_DT_INT1;	break;
		case 2 :	type = GCO_DT_INT2;	break;
		case 4 :	type = GCO_DT_INT4;	break;
		case 8 :	type = GCO_DT_INT8;	break;

		default :
		    /*
		    ** Unsupported integer size.
		    */
		    success = FALSE;
		    break;
	    }
	    break;

	case GCA_TYPE_FLT :
	    switch( length )
	    {
		case 4 :	type = GCO_DT_FLT4;	break;
		case 8 :	type = GCO_DT_FLT8;	break;

		default :
		    /*
		    ** Unsupported float size.
		    */
		    success = FALSE;
		    break;
	    }
	    break;

	case GCA_TYPE_DEC :	type = GCO_DT_DEC;	break;
	case GCA_TYPE_MNY :	type = GCO_DT_MONEY;	break;

	/* String types */

	case GCA_TYPE_C :	type = GCO_DT_C;	break;
	case GCA_TYPE_CHAR :	type = GCO_DT_CHAR;	break;
	case GCA_TYPE_LCHAR :	type = GCO_DT_LCHR;	break;
	case GCA_TYPE_LCLOC :	type = GCO_DT_LCLOC;	break;
	case GCA_TYPE_BYTE :	type = GCO_DT_BYTE;	break;
	case GCA_TYPE_LBYTE :	type = GCO_DT_LBYT;	break;
	case GCA_TYPE_LBLOC :	type = GCO_DT_LBLOC;	break;
	case GCA_TYPE_QTXT :	type = GCO_DT_QTXT;	break;
	case GCA_TYPE_NCHR :	type = GCO_DT_NCHAR;	break;
	case GCA_TYPE_LNCHR :	type = GCO_DT_NLCHR;	break;
	case GCA_TYPE_LNLOC :	type = GCO_DT_LNLOC;	break;
	case GCA_TYPE_NQTXT :	type = GCO_DT_NQTXT;	break;

	case GCA_TYPE_VCHAR :
	    type = (flags & GCO_COMP_MASK) ? GCO_DT_VCHR_C : GCO_DT_VCHR;
	    break;

	case GCA_TYPE_VBYTE :
	    type = (flags & GCO_COMP_MASK) ? GCO_DT_VBYT_C : GCO_DT_VBYT;
	    break;

	case GCA_TYPE_TXT :
	    type = (flags & GCO_COMP_MASK) ? GCO_DT_TEXT_C : GCO_DT_TEXT;
	    break;

	case GCA_TYPE_LTXT :	
	    type = (flags & GCO_COMP_MASK) ? GCO_DT_LTXT_C : GCO_DT_LTXT;
	    break;

	case GCA_TYPE_VNCHR :
		type = (flags & GCO_COMP_MASK) ? GCO_DT_NVCHR_C : GCO_DT_NVCHR;
		break;

	/* Abstract types */

	case GCA_TYPE_BOOL :	type = GCO_DT_BOOL;	break;
	case GCA_TYPE_DATE :	type = GCO_DT_IDATE;	break;
	case GCA_TYPE_ADATE :	type = GCO_DT_ADATE;	break;
	case GCA_TYPE_TIME :	type = GCO_DT_TIME;	break;
	case GCA_TYPE_TMWO :	type = GCO_DT_TMWO;	break;
	case GCA_TYPE_TMTZ :	type = GCO_DT_TMTZ;	break;
	case GCA_TYPE_TS :	type = GCO_DT_TS;	break;
	case GCA_TYPE_TSWO :	type = GCO_DT_TSWO;	break;
	case GCA_TYPE_TSTZ :	type = GCO_DT_TSTZ;	break;
	case GCA_TYPE_INTYM :	type = GCO_DT_INTYM;	break;
	case GCA_TYPE_INTDS :	type = GCO_DT_INTDS;	break;
	case GCA_TYPE_LKEY :	type = GCO_DT_LKEY;	break;
	case GCA_TYPE_TKEY :	type = GCO_DT_TKEY;	break;

	default :
	    /*
	    ** Unsupported type.
	    */
	    success = FALSE;
	    break;
    }

    if ( success )
    {
	/*
	** Internally, nullable types are numerically 1 greater
	** than the non-nullable types.
	*/
	if ( nullable )  type++;
	*gco_type = type;
    }

    return( success );
}


/*
** Name: gco_cmp_align
**
** Description:
**	Compares alignment of two atomic datatypes and returns
**	the type with the worst case alignment.
**
** Input:
**	src1		Atomic datatype.
**	src2		Atomic datatype.
**
** Output:
**	dst		Atomic datatype with worst case alignment.
**
** Returns:
**	VOID
**
** History:
**	 4-Mar-98 (gordy)
**	    Created.
*/

static VOID
gco_cmp_align( GCA_ELEMENT *src1, GCA_ELEMENT *src2, GCA_ELEMENT *dst )
{
    /*
    ** Worst case alignment is determined by which datatype
    ** requires the most padding when placed 1 byte out of
    ** alignment.
    */
    if ( gco_align_pad( src1, 1 ) >= gco_align_pad( src2, 1 ) )
	STRUCT_ASSIGN_MACRO( *src1, *dst );
    else
	STRUCT_ASSIGN_MACRO( *src2, *dst );

    return;
}


/*
** Name: gco_align_pad()
**
** Description:
**	Compute the pad needed for alignment of a given atomic type.
**
** Input:
**	ela		GCA_ELEMENT atomic type.
**	offset		Current offset.
**
** Output:
**	None
**
** Returns:
**	u_i2		Number of bytes of padding required.
**
** History:
**	 4-Mar-98 (gordy)
**	    Use Ingres datatypes rather rather than native C types.
**	22-Mar-02 (gordy)
**	    Removed unneeded atomic types.
**	15-Mar-04 (gordy)
**	    Support eight-byte integers.
*/

/* Character alignment */

struct charalign0 { char member; } ;
struct charalign1 { char pad[1]; char member; } ;
struct charalign2 { char pad[2]; char member; } ;
struct charalign3 { char pad[3]; char member; } ;
struct charalign4 { char pad[4]; char member; } ;
struct charalign5 { char pad[5]; char member; } ;
struct charalign6 { char pad[6]; char member; } ;
struct charalign7 { char pad[7]; char member; } ;

/* i1 alignment */

struct i1align0 { i1 member; } ;
struct i1align1 { char pad[1]; i1 member; } ;
struct i1align2 { char pad[2]; i1 member; } ;
struct i1align3 { char pad[3]; i1 member; } ;
struct i1align4 { char pad[4]; i1 member; } ;
struct i1align5 { char pad[5]; i1 member; } ;
struct i1align6 { char pad[6]; i1 member; } ;
struct i1align7 { char pad[7]; i1 member; } ;

/* i2 alignment */

struct i2align0 { i2 member; } ;
struct i2align1 { char pad[1]; i2 member; } ;
struct i2align2 { char pad[2]; i2 member; } ;
struct i2align3 { char pad[3]; i2 member; } ;
struct i2align4 { char pad[4]; i2 member; } ;
struct i2align5 { char pad[5]; i2 member; } ;
struct i2align6 { char pad[6]; i2 member; } ;
struct i2align7 { char pad[7]; i2 member; } ;

/* i4 alignment */

struct i4align0 { i4 member; } ;
struct i4align1 { char pad[1]; i4 member; } ;
struct i4align2 { char pad[2]; i4 member; } ;
struct i4align3 { char pad[3]; i4 member; } ;
struct i4align4 { char pad[4]; i4 member; } ;
struct i4align5 { char pad[5]; i4 member; } ;
struct i4align6 { char pad[6]; i4 member; } ;
struct i4align7 { char pad[7]; i4 member; } ;

/* i8 alignment */

struct i8align0 { i8 member; } ;
struct i8align1 { char pad[1]; i8 member; } ;
struct i8align2 { char pad[2]; i8 member; } ;
struct i8align3 { char pad[3]; i8 member; } ;
struct i8align4 { char pad[4]; i8 member; } ;
struct i8align5 { char pad[5]; i8 member; } ;
struct i8align6 { char pad[6]; i8 member; } ;
struct i8align7 { char pad[7]; i8 member; } ;

/* f4 alignment */

struct f4align0 { f4 member; } ;
struct f4align1 { char pad[1]; f4 member; } ;
struct f4align2 { char pad[2]; f4 member; } ;
struct f4align3 { char pad[3]; f4 member; } ;
struct f4align4 { char pad[4]; f4 member; } ;
struct f4align5 { char pad[5]; f4 member; } ;
struct f4align6 { char pad[6]; f4 member; } ;
struct f4align7 { char pad[7]; f4 member; } ;

/* f8 alignment */

struct f8align0 { f8 member; } ;
struct f8align1 { char pad[1]; f8 member; } ;
struct f8align2 { char pad[2]; f8 member; } ;
struct f8align3 { char pad[3]; f8 member; } ;
struct f8align4 { char pad[4]; f8 member; } ;
struct f8align5 { char pad[5]; f8 member; } ;
struct f8align6 { char pad[6]; f8 member; } ;
struct f8align7 { char pad[7]; f8 member; } ;

/* ptr alignment */

struct ptralign0 { PTR member; } ;
struct ptralign1 { char pad[1]; PTR member; } ;
struct ptralign2 { char pad[2]; PTR member; } ;
struct ptralign3 { char pad[3]; PTR member; } ;
struct ptralign4 { char pad[4]; PTR member; } ;
struct ptralign5 { char pad[5]; PTR member; } ;
struct ptralign6 { char pad[6]; PTR member; } ;
struct ptralign7 { char pad[7]; PTR member; } ;

# define ALIGNED( s ) CL_OFFSETOF( struct s, member )

static char
charaligns[] =
{
    ALIGNED( charalign0 ), ALIGNED( charalign1 ), ALIGNED( charalign2 ),
    ALIGNED( charalign3 ), ALIGNED( charalign4 ), ALIGNED( charalign5 ),
    ALIGNED( charalign6 ), ALIGNED( charalign7 )
};

static char
i1aligns[] =
{
    ALIGNED( i1align0 ), ALIGNED( i1align1 ), ALIGNED( i1align2 ),
    ALIGNED( i1align3 ), ALIGNED( i1align4 ), ALIGNED( i1align5 ),
    ALIGNED( i1align6 ), ALIGNED( i1align7 )
} ;

static char
i2aligns[] =
{
    ALIGNED( i2align0 ), ALIGNED( i2align1 ), ALIGNED( i2align2 ),
    ALIGNED( i2align3 ), ALIGNED( i2align4 ), ALIGNED( i2align5 ),
    ALIGNED( i2align6 ), ALIGNED( i2align7 )
} ;

static char
i4aligns[] =
{
    ALIGNED( i4align0 ), ALIGNED( i4align1 ), ALIGNED( i4align2 ),
    ALIGNED( i4align3 ), ALIGNED( i4align4 ), ALIGNED( i4align5 ),
    ALIGNED( i4align6 ), ALIGNED( i4align7 )
} ;

static char
i8aligns[] =
{
    ALIGNED( i8align0 ), ALIGNED( i8align1 ), ALIGNED( i8align2 ),
    ALIGNED( i8align3 ), ALIGNED( i8align4 ), ALIGNED( i8align5 ),
    ALIGNED( i8align6 ), ALIGNED( i8align7 )
} ;

static char
f4aligns[] =
{
    ALIGNED( f4align0 ), ALIGNED( f4align1 ), ALIGNED( f4align2 ),
    ALIGNED( f4align3 ), ALIGNED( f4align4 ), ALIGNED( f4align5 ),
    ALIGNED( f4align6 ), ALIGNED( f4align7 )
} ;

static char
f8aligns[] =
{
    ALIGNED( f8align0 ), ALIGNED( f8align1 ), ALIGNED( f8align2 ),
    ALIGNED( f8align3 ), ALIGNED( f8align4 ), ALIGNED( f8align5 ),
    ALIGNED( f8align6 ), ALIGNED( f8align7 )
} ;

static char
ptraligns[] =
{
    ALIGNED( ptralign0 ), ALIGNED( ptralign1 ), ALIGNED( ptralign2 ),
    ALIGNED( ptralign3 ), ALIGNED( ptralign4 ), ALIGNED( ptralign5 ),
    ALIGNED( ptralign6 ), ALIGNED( ptralign7 )
} ;

#define	PAD( arr, offset )	(arr[ offset ] - offset)

u_i2
gco_align_pad( GCA_ELEMENT *ela, u_i2  offset )
{
    u_i2  pad = 0;

    /* Assume that nothing (for now) has worse than 8 byte alignement */
    offset %= 8;

    switch( ela->tpl.type )
    {
	case GCO_ATOM_CHAR:
	case GCO_ATOM_BYTE:	
	    pad = PAD( charaligns, offset );
	    break;

	case GCO_ATOM_INT:
	    switch( ela->tpl.length )
	    {
		case CV_L_I1_SZ:	
		    pad = PAD( i1aligns, offset );
		    break;

		case CV_L_I2_SZ:	
		    pad = PAD( i2aligns, offset );
		    break;

		case CV_L_I4_SZ:	
		    pad = PAD( i4aligns, offset );
		    break;

		case CV_L_I8_SZ:	
		    pad = PAD( i8aligns, offset );
		    break;

		default:
		    pad = PAD( charaligns, offset );
		    break;
	    }
	    break;

	case GCO_ATOM_FLOAT:	
	    switch( ela->tpl.length )
	    {
		case CV_L_F4_SZ:	
		    pad = PAD( f4aligns, offset );
		    break;

		case CV_L_F8_SZ:	
		    pad = PAD( f8aligns, offset );
		    break;

		default:
		    pad = PAD( charaligns, offset );
		    break;
	    }
	    break;
    
	case GCO_ATOM_PTR:	
	    pad = PAD( ptraligns, offset );
	    break;

	default:
	    pad = PAD( charaligns, offset );
	    break;
    }

    return( pad );
}


/*
** Name: gco_dumpobjdesc 
**
** Description:
**	Trace an object descriptor for debugging.
**
** Input:
**	od		Object descriptor.
**	depth		Current tracing depth.
**	arr_stat	Array size/type for object, 0 for unknown.
**
** Output:
**	None
**
** Returns
**	VOID
**
** History:
**	 4-Mar-98 (gordy)
**	    Remove extraneous trace for complex objects by moving
**	    array status info to nested trace.
*/

# ifdef GCXDEBUG
static VOID
gco_dumpobjdesc( GCA_OBJECT_DESC *od, i4  depth, i4  arr_stat )
{
    GCA_ELEMENT	*ela;

    if ( arr_stat )
	TRdisplay( "%sobject %s array %s\n", tabs( depth ), 
		   od->data_object_name, gcx_look(gcx_gca_elestat, arr_stat) );
    else
	TRdisplay( "%sobject %s\n", tabs( depth ), od->data_object_name );

    if ( od->flags != GCA_IGNPRCLEN )
	TRdisplay( "%s    flags %x\n", tabs( depth ), od->flags );

    for( ela = od->ela; ela < od->ela + od->el_count; ela++ )
	if ( ela->od_ptr )
	    gco_dumpobjdesc( ela->od_ptr, depth + 1, ela->arr_stat );
	else
	    TRdisplay( "%s    -- (%s, %d, %d) array %s\n", tabs( depth ), 
		gcx_look( gcx_atoms, (i4)ela->tpl.type ),
		(i4)ela->tpl.precision, ela->tpl.length,
		gcx_look( gcx_gca_elestat, ela->arr_stat ) );

    return;
}


/*
** Name: gco_dumpprog
**
** Description:
**	Trace an objects compiled program.
**
** Input:
**	id	Object ID
**	pc	Compiled program
**	length	Length of compiled program
**	args	Number of argument values
**
** Output:
**	
** Returns:
**	VOID
**
** History:
**	22-Jan-10 (gordy)
**	    Added stack depth checking.
*/

static VOID
gco_dumpprog( char *id, GCO_OP *pc, i4 length, i4 args )
{
    i4 depth = 0;
    i4 psp_depth = 0;
    i4 psp_max = 0;
    i4 val_depth = 0;
    i4 val_max = 0;

    val_depth += args;
    if ( val_depth > val_max )  val_max = val_depth;

    for( ; length > 0; length-- )
    {
	char *op = gcx_look( gcx_gco_ops, (i4)*pc );

	switch( *pc++ )
	{
	case OP_ERROR : 
	case OP_JNINP : 
	case OP_GOTO : 
	case OP_CALL_DDT :
	case OP_PUSH_VCH :
	case OP_IMP_BEGIN :
	case OP_SEG_CHECK :
	case OP_PAD_ALIGN :
	case OP_DIVIDE :
	    TRdisplay( "%s    %s %d\n", tabs( depth ), op, (i4)pc[0] );
	    pc++;
	    length--;
	    break;

	case OP_PUSH_LIT :
	case OP_COPY_MSG :
	    TRdisplay( "%s    %s %d\n", tabs( depth ), op, (i4)pc[0] );
	    pc++;
	    length--;
	    val_depth++;	/* push */
	    if ( val_depth > val_max )  val_max = val_depth;
	    break;
		
	case OP_JZ : 
	case OP_DJNZ : 
	    TRdisplay( "%s    %s %d\n", tabs( depth ), op, (i4)pc[0] );
	    pc++;
	    length--;
	    val_depth--;	/* pop */
	    break;

	case OP_DV_BEGIN :
	    TRdisplay( "%s    %s %d\n", tabs( depth ), op, (i4)pc[0] );
	    pc++;
	    length--;
	    psp_depth++;	/* push */
	    if ( psp_depth > psp_max )  psp_max = psp_depth;
	    break;

	case OP_DV_END :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    psp_depth--;	/* pop */
	    break;

	case OP_MSG_BEGIN :
	    TRdisplay( "%s    %s %d %d\n", tabs( depth ), op, 
	    				   (i4)pc[0], (i4)pc[1] );
	    pc += 2;
	    length -= 2;
	    break;

	case OP_DEBUG_TYPE : 
	    TRdisplay( "%s    %s %d %d %d\n", tabs( depth ), op, 
	    				      (i4)pc[0], (i4)pc[1], (i4)pc[2] );
	    pc += 3;
	    length -= 3;
	    break;

	case OP_VAR_BEGIN :
	    TRdisplay( "%s    %s %d %d %d\n", tabs( depth ), op, 
	    				      (i4)pc[0], (i4)pc[1], (i4)pc[2] );
	    pc += 3;
	    length -= 3;
	    psp_depth++;	/* push */
	    if ( psp_depth > psp_max )  psp_max = psp_depth;
	    break;

	case OP_VAR_END :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    psp_depth--;	/* pop */
	    break;

	case OP_CALL_DV :
	    /*
	    ** OP_DV_BEGIN does not appear in compilations
	    ** as it can be dynamically compiled as part of
	    ** OP_CALL_DV.  We account for one push (and pop)
	    ** level here.  If OP_DV_BEGIN does actually
	    ** materialize, it will duplicate, with no ill
	    ** effects, what is done here.  Data values do
	    ** not have nested DV or VAREXPAR references, so
	    ** the pop action can be handled at the same time
	    ** as the push.
	    */
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    psp_depth++;	/* push */
	    if ( psp_depth > psp_max )  psp_max = psp_depth;
	    psp_depth--;	/* pop */
	    val_depth -= 3;
	    break;

	case OP_PUSH_I2 :
	case OP_PUSH_REFI2 :
	case OP_PUSH_I4 :
	case OP_PUSH_REFI4 :
	case OP_CHAR_LIMIT :
	case OP_CHAR_MSG :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    val_depth++;	/* push */
	    if ( val_depth > val_max )  val_max = val_depth;
	    break;

	case OP_POP :
	case OP_CV_I1 :
	case OP_CV_I2 :
	case OP_CV_I4 :
	case OP_CV_I8 :
	case OP_CV_F4 :
	case OP_CV_F8 :
	case OP_CV_BYTE :
	case OP_CV_PAD :
	case OP_PAD :
	case OP_SKIP_INPUT :
	case OP_SAVE_CHAR : 
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    val_depth--;	/* pop */
	    break;

	case OP_DOWN :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    depth++;
	    break;

	case OP_UP :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    depth--;
	    break;

	default :
	    TRdisplay( "%s    %s\n", tabs( depth ), op );
	    break;
        }
    }

    if ( psp_max )  TRdisplay( "%s: PTR depth %d\n", id, psp_max );
    if ( psp_depth )
        TRdisplay( "WARNING: PTR stack not empty: depth %d\n", psp_depth );

    if ( val_max )  TRdisplay( "%s: VAL depth %d\n", id, val_max );
    if ( val_depth )
        TRdisplay( "WARNING: VAL stack not empty: depth %d\n", val_depth );

    return;
}
# endif /* GCXDEBUG */



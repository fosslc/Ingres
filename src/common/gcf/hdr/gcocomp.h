/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: gcocomp.h
**
** Description:
**	GCO internal interface and definitions.
**
** History:
**	12-May-89  (GordonW)
**	   added GLOBALDEF and #define for sizes.
**	16-May-89 (GordonW)
**	   I didn't want to get rid  of the READONLYs.
**	12-Feb-90 (seiwald)
**	    Moved het support in from gccpl.h.
**	16-Feb-90 (seiwald)
**	    Removed defunct static variables.
**	    Removed GCA_TPL from DOC_EL: gcc_save_int() builds directly
**	    into the fourth element of the triad.
**	15-Mar-90 (seiwald)
**	    Reworked gcc_adf_init to allocate properly sized od_ptr
**	    vectors for each triad_od.  Removed silly [1] subscript
**	    for the od array pointer in the GCA_TRIAD_OD.
**	20-May-91 (seiwald)
**	    Excised old definitions for het connections.  They are now
**	    in gccpl.h.
**	13-Jun-91 (seiwald)
**	    Excised much fluff:
**	    -	A complex GCA_ELEMENT has a TPL and an od_ptr, though once
**	        the od_ptr is known, the TPL is not needed.  For complex
**	        GCA_ELEMENTs defined in this file, the TPL is now {0,0,0},
**		and all GCAT_ and GCTR_ defines have been removed.  This 
**	        saves the trouble of making up unused type numbers.
**	    -	All GC_..._TR defines have been replaced with their actual
**		values (a TPL initializer).
**	    -	Removed gca_objects[] - no one ever references GCA messages
**		by their (entirely arbitrary) TPL.
**	    -	Removed user_objects[] - we don't generate them yet, so why
**		bother.
**	    -	Use GCA_VARxxxx from gca.h instead of constants for arr_stat.
**	    -	Redefined OD_INST so that some casting can be avoided.
**	    -	Removed unenlightening comments.
**	20-Jun-91 (seiwald)
**	    Added descriptions for Name Server messages, so that Name Server
**	    operations can be carried out across het-net.  This is for 
**	    testing only: any actual use will have to be keyed to a change
**	    in the GCA_PROTOCOL_LEVEL.
**	17-Jul-91 (seiwald) bug #37564
**	    New messages GCA1_C_FROM, GCA1_C_INTO with corrected copy 
**	    row domain descriptor structures GCA1_CPRDD.  The old descriptor
**	    used a character array as the 'with null' nulldata, even though
**	    its type varied.  The null value is now a GCA_DATA_VALUE.
**	    Introduced at (new) GCA_PROTOCOL_LEVEL_50.
**	12-Aug-91 (seiwald)
**	   A few adjustments to GCA1_CPRDD requested by the front ends.
**	28-Jan-92 (seiwald)
**	    Support for installation passwords: new messages GCN_NS_AUTHORIZE
**	    and GCN_AUTHORIZED to request and receive authorization tickets
**	    from remote Name Servers.
**	13-Aug-92 (seiwald)
**	    Moved structure definitions out into gccod.c; excised history
**	    comments only relevant to that file.
**	13-Aug-92 (seiwald)
**	    Support for compiled message descriptors: GCC_DOC replaces
**	    GCC_DOC_STACK as the holder of perform_conversion()'s state;
**	    perform_conversion()'s (now simple) operations enumerated.
**	14-Aug-92 (seiwald)
**	    New OP_JZ for blob support.
**	14-Sep-92 (seiwald)
**	    Declare gca_msg_ods to be READONLY, to be consistent with its
**	    definition in gccod.c.
**	16-Dec-92 (seiwald)
**	    Rename OP_PUSH_UI2 to OP_PUSH_VCH make room for a new OP_PUSH_UI2
**	    that isn't specific to varchars.  Increase GCC_L_OP_MAX to 60,
**	    because long varchars programs are more than 40 (with debugging
**	    in them).
**      22-Mar-93 (huffman)
**          Change "GLOBALREF READONLY" to "GLOBALREF"
**      24-mar-93 (sweeney)
**          Changed forward ref to gca_msg_ods to be type GLOBALCONSTREF.
**          See companion change to gccod.c for justification.
**	29-mar-93 (kirke)
**	    Removed useless typedefs from struct _GCA_OBJHDR_DESC, struct
**	    _GCA_TRIAD_OD, and struct _DOC_EL.
**	25-May-93 (seiwald)
**	    Moved definitions for OP_ERROR argument in from gccpdc.c.
**	3-Jun-93 (seiwald)
**	    Eliminate the age old GCA_TRIAD_OD - when trying to find the
**	    GCA_OBJECT_DESC for a complex type, we now just directly scan 
**	    the table handed back by adc_gcc_decompose().
**	22-Jun-93 (seiwald)
**	    Removed reference to now defunct gcc_eop_msg.
**	14-Mar-94 (seiwald)
**	    Renamed from gccod.h.
**	    Support for message encoding/decoding: pointer stack for 
**	    VAREXPAR handling added to GCC_DOC; a few new OP_'s.
**	29-Mar-94 (seiwald)
**	    Added support for VARIMPAR in message encoding.
**	1-Apr-94 (seiwald)
**	    Split OP_DEBUG into OP_DEBUG_TYPE and OP_DEBUG_BUFS.
**	4-Apr-94 (seiwald)
**	    New OP_DV_BEGIN/END.
**	11-Apr-94 (seiwald)
**	    New dst3 in GCC_DOC for perform_decoding().
**	24-Apr-94 (seiwald)
**	    Collect flags in GCC_DOC into "state".
**	24-Apr-94 (seiwald)
**	    New OP_BREAK to inidicate that perform_decoding() should
**	    indicate a break in the tuple stream (i.e. after a row).
**	 9-Nov-94 (gordy)
**	    New OP_MSG_RESTART similar to OP_MSG_BEGIN but used after
**	    OP_BREAK to start next tuple.
**	27-mar-95 (peeje01)
**	    Crossintegration from doublebyte label 6500db_su4_us42:
**	    13-jul-94 (kirke)
**	        Added new elements to GCC_DOC struct for varying length
**	        character conversion.  Also added defines for OP_CHAR_TYPE
**	        and OP_DATA_LEN.
**	25-May-95 (gordy)
**	    Added EOP_VARSEGAR for GCA_VARSEGAR segment arrays.
**	20-Jun-95 (gordy)
**	    Bump GCC_L_OP_MAX to 100 for BLOBs which can exceed 60.
**	20-Dec-95 (gordy)
**	    Added incremental initialization support to gcd_init().
**	 1-Feb-96 (gordy)
**	    Added support for compressed VARCHARs.
**	 8-Mar-96 (gordy)
**	    Fixed processing of large messages which span input or
**	    or output buffers.  Added OP_MARK, OP_REWIND to properly
**	    split C structures at VARIMPAR boundaries.  Changed
**	    gcd_convert() interface to match gcd_encode(), gcd_decode().
**	 3-Apr-96 (gordy)
**	    OP_JNOVR changed to OP_CHKLEN.  Added OP_DV_ADJUST.
**	21-Jun-96 (gordy)
**	    Added OP_COPY_MSG and gcd_tuple_prog, removed gcd_od_tuple.
**	 5-Feb-97 (gordy)
**	    Object descriptors for DBMS datatypes are now pre-compiled.
**	    Compilation of tuple descriptors or GCA_DATA_VALUE objects
**	    now just produce a OP_CALL_DDT to the pre-compiled program 
**	    greatly reducing the memory requirements for compilation.
**	    Removed OP_CHKLEN (tuple attributes no longer processed
**	    individually by encode/decode) and OP_COMP_PAD (compressed
**	    variable length types now represented by their own object
**	    descriptor).  Added internal GCD datatype ID's and reworked
**	    the global structures for datatype object descriptors.
**	24-Feb-97 (gordy)
**	    Added EOP_NOT_INT.
**	 1-Mar-01 (gordy)
**	    Added new GCD types to represent the new DBMS Unicode data
**	    types.  Added OP_DIVIDE to handle length conversions for
**	    Unicode arrays.
**       06-07-01 (wansh01) 
**          Added new GLOBALREF gcd_trace_level. 
**	22-Mar-02 (gordy)
**	    Moved OP codes, internal datatype representations and gcd
**	    internal function prototypes to gcdint.h.  Moved object
**	    descriptor structures from gca.h to here.
**	12-Dec-02 (gordy)
**	    Renamed gcd component to gco.
**	12-May-03 (gordy)
**	    Renamed DOC->last_seg to eod to match message flag.  Made
**	    DOC_DONE distinct from DOC_IDLE so HET/NET processing can
**	    detect difference between completion and initialization.
**	    Define DOC_CLEAR_MACRO to initialize DOC.
**	22-Jan-10 (gordy)
**	    DOUBLEBYTE code generalized for multi-byte charset processing.
**	    GCA_VARVCHAR array type is now used for compressed varchars
**	    (rather than GCA_VARSEGAR) while the new GCA_VARPADAR is used
**	    for padded varchars.  Added definitions for empirically sized
**	    arrays, DOC_MOREQOUT and DOC_FLUSHQOUT states, and a set of
**	    processing flags.  GCO_CSET_XLT defines charset translation 
**	    mode and provides translation tables and routines.  Max atomic 
**	    fragment size is actually 10 bytes for the network standard 
**	    float format.  Extend value stack by one to allow overflow 
**	    test to occur after pushing a value without stepping on 
**	    following data.
*/

#ifndef GCOCOMP_H_INCLUDED
#define GCOCOMP_H_INCLUDED

/*
** The following definitions are used to size various arrays.
** The sizes are determined from actual compilations of the
** various GCO objects, as described below.
**
** GCO_TUP_OP_MAX( count )
**	Maximum operations produced by compilation of a tuple 
**	object descriptor for a given number of attributes. 
**	Determined by examination of gco_comp_tod() (with 
**	nested call to gco_comp_tpl()).
**
** GCO_DV_OP_MAX
**	Maximum operations produced by compilation of a GCA
**	data value.  Determined by examination of gco_comp_dv()
**	(with nested call to gco_comp_tpl()).
**
** GCO_TPL_OP_MAX
**	Maximum operations produced by compilation of a call
**	to a pre-compiled data type descriptor program.
**	Determined by examination of gco_comp_tpl().
**
** GCO_L_MSG_MAX
**	Maximum compiled length of the GCO message and data 
**	type object descriptors.  The compiled length of each 
**	object is traced at level 3.
**
** DOC_CALL_DEPTH
**	Size of the program counter stack used by OP_CALL_TUP, 
**	OP_CALL_DV, and OP_CALL_DDT (restored by OP_RETURN). 
**	Tuples and data values have nested calls for ddt's, 
**	hence a stack depth of 2.
**
** DOC_VAL_DEPTH
**	Size of the runtime value stack used to pass arguments
**	to various operations.  The VAL depth of each object 
**	is traced when compiled (full init, trace level 4).  
**	The max depth is the sum of the max MSG and DDT value 
**	depths (single nesting of a data value in a message).
**
** DOC_PTR_DEPTH
**	Size of the data pointer stack used by OP_VAR_BEGIN 
**	and OP_DV_BEGIN (restored by OP_VAR_END and OP_DV_END).  
**	The PSP depth of each object is traced when compiled 
**	(trace level 4 during full init).
**
** DOC_MAX_FRAG_LEN
**	Size of an atomic data value.  Atomic values may be 
**	split between message segments and atomic fragments 
**	must be saved and combined for processing as a single 
**	atomic entity.  An f8 has the max atomic length which
**	is 10 bytes in network standard format.
**
** DOC_MAX_CHAR_LEN
**	Maximum length of character data which may be combined
**	to form a single character.  Used by OP_SAVE_MSG and
**	OP_SAVE_CHAR (restored by OP_RSTR_MSG and OP_RSTR_CHAR).
**	Multi-byte characters may be split across text segments 
**	and partial character fragments must be saved and 
**	combined for translation of the entire character.  
**	Currently, the maximum character length is a double-byte 
**	character (2).  This length must also be large enough to 
**	hold a partial atomic fragment (see DOC_MAX_FRAG_LEN).
*/

# define GCO_TPL_OP_MAX		8
# define GCO_DV_OP_MAX		(5 + GCO_TPL_OP_MAX)
# define GCO_TUP_OP_MAX(count)	(((count) * GCO_TPL_OP_MAX) + 1)
# define GCO_L_MSG_MAX		215
# define DOC_CALL_DEPTH		2
# define DOC_VAL_DEPTH		6
# define DOC_PTR_DEPTH		2
# define DOC_MAX_FRAG_LEN	10
# define DOC_MAX_CHAR_LEN	DOC_MAX_FRAG_LEN

/*
** GCO op codes are all small integers, so i2 is easily
** sufficient.  However, the OP codes also hold various
** data values.  This may need to be larger if data value
** lengths expand.
**
** TODO: should this be larger?, unsigned?
*/

typedef i2 GCO_OP;

typedef struct _GCO_DOC GCO_DOC;
typedef struct _GCO_CSET_XLT GCO_CSET_XLT;
typedef struct _GCA_TPL GCA_TPL;
typedef struct _GCA_ELEMENT GCA_ELEMENT;
typedef struct _GCA_OBJECT_DESC GCA_OBJECT_DESC;


/*
** Name: GCO_DOC - data object context 
**
** Contains the state info needed to translate a GCA message, 
** possibly fragmented across many message segments.
*/

struct _GCO_DOC
{
    QUEUE	q;			/* Free chain */

    /* I/O Section */

    char	*src1;			/* Start of input data */
    char	*src2;			/* End of input data */
    char	*dst1;			/* Start of output buffer */
    char	*dst2;			/* End of output buffer */
    PTR		tod_prog;		/* Compiled tuple descriptor */
    i4		message_type;		/* GCA message type */
    bool	eod;			/* Message end-of-data */
    i1		state;			/* Conversion state */

# define DOC_IDLE	(-1)		/* Conversion not started */
# define DOC_DONE	0		/* Conversion complete */
# define DOC_MOREIN	1		/* Need more input */
# define DOC_MOREOUT	2		/* Need more output space */
# define DOC_MOREQOUT	3		/* Need more output, queue current */
# define DOC_FLUSHQOUT	4		/* Flush queued output */

    /******************************************/
    /* Private conversion interpreter section */
    /******************************************/

    u_i2	flags;		/* Processing flags */

# define DOC_TUP_FLG	0x01	/* Processing a Tuple */
# define DOC_DV_FLG	0x02	/* Processing a Data Value */
# define DOC_DVREF_FLG	0x04	/* DV meta-data length reference is set */
# define DOC_REF_FLG	0x08	/* Numeric output reference is set */
# define DOC_OUTQ_FLG	0x10	/* Output is queued (DOC_MOREQOUT) */
# define DOC_SEG_FLG	0x20	/* More segments available */
# define DOC_LIMIT_FLG	0x40	/* Character input has max limit */

    GCO_OP	*pc;		/* Program counter */
    i4		depth;		/* Tracing obj descr depth */
    char	*dvref;		/* DV meta-data length reference */
    char	*ref;		/* Numeric output reference */
    i4		delta;		/* Relative change in numeric value */
    char	*rsrv;		/* Reserve output buffer space for decoding */

    /*
    ** Stack for program counters to permit calling other
    ** compiled objects.  Data type descriptors (DDT) are
    ** pre-compiled while tuples and data values are
    ** dynamically compiled.
    */
    GCO_OP	*call_stack[ DOC_CALL_DEPTH ];	/* pushed pc's */
    GCO_OP	**csp;				/* top of call stack */

    /*
    ** Stack for dynamically produced values used as arguments 
    ** of various operations.  An additional entry is added 
    ** because overflow is tested after values are pushed.
    */
    i4		val_stack[ DOC_VAL_DEPTH + 1 ];	/* value stack */
    i4		*vsp;				/* top of value stack */

    /*
    ** Stack for input/output pointers during encoding/decoding
    ** of data structures.
    */
    char	*ptr_stack[ DOC_PTR_DEPTH ];	/* Data pointer stack */
    char	**psp;				/* top of ptr_stack */

    /*
    ** Storage area to dynamical compile a GCA data value
    ** (OP_CALL_DV).
    */
    GCO_OP	od_dv_prog[ GCO_DV_OP_MAX ];	/* compiling GCA_DATA_VALUES */

    /* 
    ** Buffer for atomic fragments.  An atomic value may be split
    ** across message segments.  Any unused input must be saved
    ** when DOC_MOREIN is requested and pre-pended to the next
    ** message segment.
    */
    u_i2	frag_bytes;				/* Fragment size */
    u_i1	frag_buffer[ DOC_MAX_FRAG_LEN ];	/* Fragment storage */

    /*
    ** Buffer for text processing.  Save partial characters split
    ** across text segments.  Character data is different than 
    ** atomic data in that text segments may be separated by other 
    ** atomic values which themselves may be split.  
    */
    u_i2	text_len;				/* Character length */
    u_i1	text_buffer[ DOC_MAX_CHAR_LEN ];	/* Saved characters */

    /*
    ** OP_MARK/OP_REWIND saved values
    */
    struct
    {
	char		*src1;		/* input position */
	char		*dst1;		/* output position */
	GCO_OP		*pc;		/* program counter */
	i4		depth;		/* object descriptor depth */
	GCO_OP		**csp;		/* top of call stack */
	i4		*vsp;		/* top of value stack */
	char		**psp;		/* top of ptr_stack */
    } mark;

};

#define	DOC_CLEAR_MACRO( doc_ptr ) ((doc_ptr)->state = DOC_IDLE, \
				    (doc_ptr)->depth = 0, \
				    (doc_ptr)->frag_bytes = 0, \
				    (doc_ptr)->text_len = 0, \
				    (doc_ptr)->csp = (doc_ptr)->call_stack, \
				    (doc_ptr)->vsp = (doc_ptr)->val_stack, \
				    (doc_ptr)->psp = (doc_ptr)->ptr_stack, \
				    (doc_ptr)->flags = 0, \
				    (doc_ptr)->delta = 0)


/*
** Name: GCO_CSET_XLT
*/

# define GCO_L_SB_CSET		256
# define GCO_L_DB_CSET		65536

typedef	i1 GCO_XLT_FUNC(PTR, bool, u_i1 **, i4, u_i1 **, i4, i4 *, i4 *, i4 *);

struct _GCO_CSET_XLT
{
	u_i2	type;		/* Mapping type */

# define GCO_XLT_IDENTITY	0
# define GCO_XLT_SINGLE		1
# define GCO_XLT_DOUBLE		2

	struct			/* Local to net mappings */
	{
	    PTR			map;	/* Character map */
	    GCO_XLT_FUNC	*xlt;	/* Mapping function */
	} l2n;

	struct			/* Net to local mappings */
	{
	    PTR			map;	/* Character map */
	    GCO_XLT_FUNC	*xlt;	/* Mapping function */
	} n2l;
};


/*
** GCO Object definitions
**
** These objects are also used as message objects
** for the Het/Net internal message GCA_TO_DESCR.
*/

/*
** Name: GCA_TPL - Object Type Triad.
**
** Description:
**      Three integers which, when taken together, define 
**	a unique (and possibly complex) data object.
**
** History:
**      05-aug-88 (thurston)
**          Initial creation.
**	05-Oct-89 (seiwald)
**	    Moved GCA_QTXT definition in from gca.h.
*/

struct _GCA_TPL
{
    i2		type;		/* Data type id */
    i2		precision;	/* Precision/scale (hi/lo bytes) */
    i4		length;		/* Length in bytes */
};

/*
** GCO Atomic data object types
*/

#define     GCO_ATOM_CHAR	(i2)0xA0		/* character */
#define     GCO_ATOM_BYTE	(i2)0xA1		/* byte	*/
#define     GCO_ATOM_INT	(i2)0xA2		/* integer (1,2,4) */
#define     GCO_ATOM_FLOAT	(i2)0xA3		/* float (4,8) */

/*
** The following atomic type is used internally for
** structure alignment of pointers.  It may not be
** used as a type for data objects.
*/

#define     GCO_ATOM_PTR	(i2)0xAF		/* pointer	*/


/*
** Name: GCA_ELEMENT - Object Type Element.
**
** Description:
**      This structure defines an array (with a possible 
**	length of one) of object types.  The object types 
**	may be atomic OR complex.
*/

struct _GCA_ELEMENT
{
    GCA_TPL         tpl;                /* Type, precision, length */
    GCA_OBJECT_DESC *od_ptr;		/* Object Descriptor */
    i4		    arr_stat;		/* Array count or type */
};

/*
** Actual array lengths.  Any non-negative value
** is permitted.  Negative values reserved for
** special cases defined below.
*/
#define GCA_VARZERO	((i4)  0)	/* Zero length array */
#define GCA_NOTARRAY	((i4)  1)	/* Single object (non-array) */

/*
** VARIMPAR: Implicit array 
**
** Length determined by the length of the 
** message.  The elements in the array 
** (atomic or complex) repeat until the 
** end of the message.  Objects of this 
** type may only occur as the last element 
** of a complex top-level object (can not 
** be nested in other complex objects).
*/
#define GCA_VARIMPAR	((i4) -1)

/*
** VAREXPAR: Explicit array
**
** Length determined by preceding element.  
** The elements in the array (atomic or 
** complex) repeat the indicated number of 
** times.  Must be preceded by an element 
** of type integer.  Array is replaced in 
** data structures by a pointer to the 
** actual array, but in the data stream 
** the array is fully embedded in place 
** (see GCA_VARSEGAR).
*/
#define GCA_VAREXPAR	((i4) -2)

/*
** VARLSTAR: List array
**
** Length determined by embedded indicators.  
** The elements in the array (complex only) 
** repeat until an end-of-list indicator.  
** Each element must be a complex object 
** consisting of an atomic numeric (the 
** indicator) and a list object (atomic 
** or complex).  The end of the list is 
** marked by an indicator with value 0.  
** No list object accompanies the end-of-
** list indicator.  Zero or more elements 
** precede the end-of-list indicator, each 
** with a non-zero indicator.  Each non-
** zero indicator is accompanied by a single 
** instance of the list object.
*/
#define GCA_VARLSTAR	((i4) -3)

/*
** VARSEGAR: Explicit array (segment) 
**
** Length determined by preceding element.  
** The elements in the array (atomic only) 
** repeat the indicated number of times.  
** Must be preceded by an atomic element 
** of type integer.  Objects of this type 
** may only be used to build datatype 
** object descriptors and can not be used 
** in message object descriptors.  (see  
** GCA_VAREXPAR).
*/
#define	GCA_VARSEGAR	((i4) -4)

/*
** VARTPLAR: Explicit array (triad)
**
** Length determined by external triad description.
** The elements in the array (atomic only) repeat
** the indicated number of times.  Objects of this
** type may only be used to build datatype object
** descriptors and can not be used in message
** object descriptors.  Must be accompanied by a
** triad descriptor (GCA_DATA_VALUE or GCA_TUP_DESCR)
** to determine actual length.
*/
#define GCA_VARTPLAR	((i4) -5)

/*
** VARVCHAR: Explicit array (varchar)
**
** Length determined by preceding element and
** external triad description.  The elements in
** the array (atomic only) repeat the indicated
** number of times, limited by the triad length.
** Must be preceded by an element of type integer.
** Objects of this type may only be used to build
** datatype object descriptors and can not be 
** used in message object descriptors.  Must 
** be accompanied by a triad descriptor 
** (GCA_DATA_VALUE or GCA_TUP_DESCR) to
** determine actual length.
*/
#define GCA_VARVCHAR	((i4) -6)

/*
** VARPADAR: Explicit padded array (varchar)
**
** Length determined by preceding element and
** external triad description.  The elements in 
** the array (atomic only) repeat the indicated 
** number of times, followed by padding to the
** triad length.  Must be preceded by an element 
** of type integer.  Objects of this type may 
** only be used to build datatype object 
** descriptors and can not be used in message 
** object descriptors.  Must be accompanied 
** by a triad descriptor (GCA_DATA_VALUE or 
** GCA_TUP_DESCR) to determine actual length.
*/
#define GCA_VARPADAR	((i4) -7)


/*
** Name: GCA_OBJECT_DESC - Complex Object Descriptor.
**
** Description:
**      This data structure describes a complex data object 
**	in terms of other (known) complex data objects or 
**	atomic data objects.  It is composed of a variable 
**	length array of GCA_ELEMENT structures along with 
**	some supplementary information.
*/

#define	GCA_OBJ_NAME_LEN	32
#define	GCA_OBJ_HDR_LEN		(GCA_OBJ_NAME_LEN + (sizeof(i4) * 2))
#define	GCA_OBJ_ELE_LEN		((sizeof(i2) * 2) + (sizeof(i4) * 3))
#define	GCA_OBJ_SIZE( el )	(GCA_OBJ_HDR_LEN + (el * GCA_OBJ_ELE_LEN))

struct _GCA_OBJECT_DESC
{
    char	data_object_name[ GCA_OBJ_NAME_LEN ];
    i4		flags;

#define		GCA_IGNPRCSN	1	/* Ignore precision */
#define		GCA_IGNLENGTH	2	/* Ignore length */
#define		GCA_IGNPRCLEN	3	/* Ignore precision & length */
#define		GCA_VARLENGTH	4	/* Variable length */

    i4		el_count;		/* Length of following array */
    GCA_ELEMENT	ela[1];			/* Variable length */
};


/*
** External Functions
*/

FUNC_EXTERN	i4	gca_to_len( bool fmt, PTR desc );
FUNC_EXTERN	VOID	gca_to_fmt( bool fmt, PTR desc, GCA_OBJECT_DESC *tod );

FUNC_EXTERN	STATUS	gco_init( bool );
FUNC_EXTERN	VOID	gco_init_doc( GCO_DOC *doc, i4  msg_type );
FUNC_EXTERN	STATUS	gco_encode( GCO_DOC *doc );
FUNC_EXTERN	STATUS	gco_decode( GCO_DOC *doc );

FUNC_EXTERN	STATUS	gco_convert( GCO_DOC *, i4, GCO_CSET_XLT * );

FUNC_EXTERN	VOID    gco_comp_tod( PTR ptr, GCO_OP *in_pc );

FUNC_EXTERN	i1	gco_xlt_ident( PTR, bool, u_i1 **, i4,
					u_i1 **, i4, i4 *, i4 *, i4 * );

FUNC_EXTERN	i1	gco_xlt_sb( PTR, bool, u_i1 **, i4,
				    u_i1 **, i4, i4 *, i4 *, i4 * );

FUNC_EXTERN	i1	gco_xlt_db( PTR, bool, u_i1 **, i4, 
				    u_i1 **, i4, i4 *, i4 *, i4 * );

#endif


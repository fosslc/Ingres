/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cv.h>
#include	<ex.h>
#include	<er.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fdesc.h>
#include	<fe.h>
#ifndef _FEalloc
#define _FEalloc(size, status) FEreqmem(0, (size), FALSE, (status))
#endif
#include	<ilops.h>
#include	<iltypes.h>
#include	"igrdesc.h"
#include	<adf.h>

/**
** Name:    igalloc.c -	OSL Interpreted Frame Object Generator
**					Variable Allocation Module.
** Description:
**	Contains routines that allocate variables for an interpreted frame
**	object and output their data descriptors.  Defines:
**
**	IGallocRef()	allocate IL variable reference.
**	IGsymbol()	allocate IL symbol table entry.
**	iiIGspStartProc() start a local procedure or top routine.
**	iiIGepEndProc()	end a local procedure or top routine.
**
**	(ILG internal)
**	IGgenRefs()	output interpreted frame object data descriptors.
**
**	***NOTE*** When a new datatype is to be supported, it's essential that
**	a new case be added for it in the static function align_restrict.
**	Otherwise, when compiling a 4GL program that contains a variable
**	of the new datatype, no space will be allocated for it, and it will
**	overlay other variables at run-time.
**
** History:
**	Revision 5.1  86/10/16  20:26:07  wong
**	Initial revision.  86/09/11  wong
**
**	Revision 6.0  87/06  wong
**	Added support for ADF types including Nulls.
**
**	Revision 6.3  89/08/21  jmorris
**	Align BYTE stack, which contains Nullable types that require alignment.
**
**	Revision 6.3/03/00  90/07  wong
**	Made allocation of place-holder references explicit.
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added procedures iiIGspStartProc and iiIGepEndProc;
**		modified IGgenRefs (some of its logic is now in iiIGepEndProc).
**		Changed alloc_list from a circular list to a null-terminated
**		list.
**	05/07/91 (emerson)
**		Added logic in iiIGspStartProc to generate NEXTPROC statement
**		(for codegen).
**	08/17/91 (emerson)
**		Added missing #include <cv.h>.
**
**	Revision 6.4/02
**	12/15/91 (emerson)
**		Fix for bug 40662 in align_restrict.
**		Also added cautionary note above.
**
**	Revision 6.5
**	18-jun-92 (davel)
**		added decimal datatype support.
**	15-nov-93 (robf)
**           Add Security label support  (DB_SEC_TYPE)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	aug-2009 (stephenb)
**		Prototyping front-ends
*/

/*
** Name:    DBREF -		Variable Reference Structure (internal.)
*/
typedef struct _dbref {
    i4	    db_offs;
    i4		    ir_sect;
    i4		    ir_type;
    i4		    ir_len;
    i4		    ir_prec;
    struct _dbref   *ir_next;
    i4		    db_value;	/* debug info */
} DBREF;

/*
** Name:    SYMTAB -	Internal Symbol Table Entry Structure.
**
** Description:
**	This structure maintains a symbol table entry during code generation.
*/
typedef struct _symtab {
    char	*ir_name;
    char	*ir_parent;
    u_i4	ir_ref : 15;
    char	ir_visible;
    i4	    	ir_flags;
    char	*ir_typename;
    struct _symtab  *ir_snext;
} SYMTAB;

/* Free list */

#define LIST_SIZE	(max(sizeof(DBREF),sizeof(SYMTAB)))

static	FREELIST	*listp = NULL;
static	SYMTAB		*symbol_table = NULL;
static	DBREF		*alloc_list = NULL;
static	DBREF		**alloc_list_tail = &alloc_list;

# define BYTES 0
# define WORDS 1
# define DOUBLES 2

static i4	stack_size[3];
static i4	db_values = 0;
static i4	vdesc_size = 0;
static i4	n_syms = 0;

/*{
** Name:	IGallocRef() -	Allocate An IL Variable Reference.
**
** Description:
**	Allocates an IL variable with the type and length specified
**	and returns its IL reference.
**
** Inputs:
**	place	{bool}  Whether the reference is for a place-holder or not.
**	type	{DB_DT_ID}  Data type of symbol (e.g., DB_INT_TYPE.)
**	len	{nat}  Length of data.
**	prec	{nat}  Precision scale for decimal types.
**
** Returns:
**	{ILALLOC}  IL variable reference.
**
** History:
**	08/86 (jhw) -- Written.
**	07/90 (jhw) -- Made place-holder references explicit by adding 'place'
**		parameter.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added statement to bump new static var vdesc_size.
**		Also added statements to copy prec and db_values into ref.
**		Zero-length items now have their db_offs set to -1 instead of 0.
**		(It was getting changed to -1 anyway in IGgenRefs).
**	15-Sep-2008 (jonj)
**	    SIR 120874: adu_error() is prototyped in adfint.h, not available here.
**	    Changed to conform to new aduErrorFcn() prototype.
*/
static i4	align_restrict();

ILALLOC
IGallocRef ( place, type, len, prec )
bool		place;
DB_DT_ID	type;
i4		len;
i4		prec;
{
    register DBREF  *ref;
    register i4     align;

    if (type == 0 || type >= ADI_MXDTS || type <= -ADI_MXDTS)
    {
	ADF_CB	*cb;

	ADF_CB	*FEadfcb();

        _VOID_ aduErrorFcn(cb=FEadfcb(), E_AD2004_BAD_DTID, __FILE__, __LINE__, 0);
	FEafeerr(cb);
	EXsignal(EXFEBUG, 1, ERx("IGallocRef()"));
    }

    while ((listp == NULL && (listp = FElpcreate(LIST_SIZE)) == NULL) ||
		(ref = (DBREF *)FElpget(listp)) == NULL)
	EXsignal(EXFEMEM, 1, ERx("IGallocRef"));

    vdesc_size++;
    db_values++;

    ref->db_value = db_values;
    ref->ir_type = type == DB_DYC_TYPE ? DB_VCH_TYPE : type;
    ref->ir_len = len;
    ref->ir_prec = prec;

    /*
    ** Set temporary data reference and increment stack size.
    */
    align = !place && len != 0 ? align_restrict(type, len) : 0;
    if (align != 0  &&  (align <= sizeof(i2) || (len % align) != 0))
    {
	i4	pad;

	ref->ir_sect = BYTES;
	if ((pad = (stack_size[BYTES] % align)) != 0)
	    pad = align - pad;
	ref->db_offs = stack_size[BYTES] + pad;
	stack_size[BYTES] += ref->ir_len + pad;
    }
    else if (align == sizeof(i4))
    {
	ref->ir_sect = WORDS;
	ref->db_offs = stack_size[WORDS];
	stack_size[WORDS] += ref->ir_len;
    }
    else if (align == sizeof(f8))
    {
	ref->ir_sect = DOUBLES;
	ref->db_offs = stack_size[DOUBLES];
	stack_size[DOUBLES] += ref->ir_len;
    }
    else
    {
	ref->ir_sect = -1;
	ref->db_offs = -1;
    }

    /* Add to list */

    *alloc_list_tail = ref;
    alloc_list_tail = &ref->ir_next;
    ref->ir_next = NULL;

    return db_values;
}

/*
** Name:    align_restrict() -    Determine Alignment Restriction for Type.
**
** Description:
**	Determines the alignment restriction of the base input type and length.
**
**	***NOTE*** When a new datatype is to be supported,
**	it's essential that a new case be added for it in this function.
**	Otherwise, when compiling a 4GL program that contains a variable
**	of the new datatype, no space will be allocated for it, and it will
**	overlay other variables at run-time.
**
** History:
**	12/15/91 (emerson)
**		Fix for bug 40662: Added cases for logical key datatypes
**		(DB_LOGKEY_TYPE and DB_TABKEY_TYPE): they get aligned on
**		an i4 boundary (per John Black and Mike Matrigali).
**		Also added note to description.
**	18-jun-92 (davel)
**		add DB_DEC_TYPE as byte-aligned.
**	15-nov-93 (robf)
**           Add Security label support  (DB_SEC_TYPE)
**	11-sep-06 (gupsh01)
**	     Added ANSI date/time types.
*/
static i4
align_restrict (type, len)
DB_DT_ID    type;
i4	    len;
{
    if (type < 0)
    {
	type = -type;
	len -= 1;
    }
    switch (type)
    {
      case DB_FLT_TYPE:
      case DB_MNY_TYPE:
	return (len == sizeof(f8)) ? sizeof(f8) : sizeof(f4);
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
      case DB_LOGKEY_TYPE:
      case DB_TABKEY_TYPE:
	return sizeof(i4);
      case DB_INT_TYPE:
	return len;
      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
	return sizeof(i2);
      case DB_CHR_TYPE:
      case DB_CHA_TYPE:
      case DB_DEC_TYPE:
	return sizeof(char);
      default:
	return 0;
    }
}

/*{
** Name:    IGsymbol() -	Allocate IL Symbol Table Entry.
**
** Description:
**
** Inputs:
**	name	{char *}  	Variable name.
**	parent	{char *}  	Parent name.
**	dbref	{ILALLOC}  	IL DB data reference.
**	visible	{char}    	Is this a form field, a global, or a hidden var.
**	flags	{nat}		other info.
**	typename {char*}	Name of the type, if this is a complex datatype.
**
**	Note: all character pointers must be non-null, except when creating
**	a NULL FDESC entry (which is done only in this source module).
**
** History:
**	08/86 (jhw) -- Written.
*/
VOID
IGsymbol (name, parent, dbref, visible, flags, typename)
char	*name;
char	*parent;
ILALLOC	dbref;
char	visible;
i4	flags;
char	*typename;
{
    register SYMTAB	*sym;

    while ((sym = (SYMTAB *)FElpget(listp)) == NULL)
	EXsignal(EXFEMEM, 1, ERx("IGsymbol"));

    sym->ir_name = name;
    sym->ir_parent = parent;
    sym->ir_ref = dbref;
    sym->ir_visible = visible;
    sym->ir_flags = flags;
    sym->ir_typename = typename;

    /* Add to list */

    if (symbol_table == NULL)
    {
	sym->ir_snext = sym;
	symbol_table = sym;
    }
    else
    {
	sym->ir_snext = symbol_table->ir_snext;
	symbol_table->ir_snext = sym;
	symbol_table = sym;
    }
    ++n_syms;
}

/*{
** Name:	iiIGspStartProc() - Start a Local Procedure or Top Routine
**
** Description:
**	Sets ILG-internal information and generates appropriate IL
**	at the beginning of a local procedure or top routine
**	(a frame or a non-local procedure).
**
**	The first call to this routine should be for the top routine.
**	Subsequent calls, if any, should be for local procedures.
**
**	The IL generated for the top routine is:
**
**	MAINPROC VALUE1 VALUE2 VALUE3 VALUE4
**	NEXTPROC VALUE5
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the frame or procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the frame or procedure.
**	VALUE3	The number of VDESC entries for the frame or procedure.
**	VALUE4	The total number of entries in the dbdv array
**		for the frame or procedure and any local procedures it may call.
**	VALUE5	The SID of the first IL_LOCALPROC statement (0 if none).
**
**	The IL generated for a local routine is:
**
**	LOCALPROC VALUE1 VALUE2 VALUE3 VALUE4 VALUE5 VALUE6 VALUE7 VALUE8
**	NEXTPROC VALUE9
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the local procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the local procedure.
**	VALUE3	The number of VDESC entries for the local procedure.
**	VALUE4	The offset of the FDESC entries for the local procedure
**		(within the composite FDESC for the entire frame or procedure).
**	VALUE5	The offset of the VDESC entries for the local procedure
**		(within the composite VDESC array for the entire frame or proc).
**	VALUE6	The offset of the dbdv's for the local procedure
**		(within the composite dbdv array for the entire frame or proc).
**	VALUE7	A reference to a char constant containing the name of
**		the local procedure.
**	VALUE8	The SID of the IL_MAINPROC or IL_LOCALPROC statement
**		of the "parent" main or local procedure (i.e, the procedure
**		in which this procedure was declared).
**	VALUE9	The SID of the next IL_LOCALPROC statement (0 if none).
**
**	See IGoutStmts in igstmt.c for the logic that generates
**	MAINPROC and LOCALPROC statements from IGRDESC structures.
**
**	The NEXTPROC statement is used only by codegen.  (Codegen generates
**	STATIC declarations for all the local procedures near the top of
**	the C source file).  The NEXTPROC statement simply contains the
**	SID of the next LOCALPROC (or 0 if none).  This SID was put into
**	a separate IL opcode rather than being made another operand of
**	LOCALPROC because LOCALPROC already has a SID (VALUE8), and longsid
**	logic doesn't easily handle opcodes that have more than one SID
**	(there's a bit in the opcode saying whether "the" SID is long or short).
**
**	Implementation note:  next_proc_sid is set to point to a new IGSID
**	each time a routine is started.  This IGSID is always initially set
**	to contain the IGSID of the NEXTPROC statement for the routine being
**	started.  It will ultimately be overridden to contain the IGSID
**	of the LOCALPROC statement for the *next* routine, except for the
**	last routine.  For the last routine, the IGSID in the NEXTPROC statement
**	will be left pointing to itself, which will result in a value of 0
**	in the generated IL operand.
**
** Inputs:
**	name_ref	{ILREF}	char const containing name of routine
**				(0 for the top routine).
**	parent		{PTR}	pointer to IGRDESC structure for routine
**				in which the local procedure was declared
**				(NULL for the top routine).
**
** Returns:
**	{PTR}	pointer to IGRDESC structure created for local procedure
**		or top routine being started.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedure support.
**	05/07/91 (emerson)
**		Added logic to generate NEXTPROC statement (for codegen).
**		Added description of generated IL code.
*/
static	i2	proc_tag = 0;
static	IGRDESC	*top_rdesc = NULL;
static	DBREF	**alloc_list_proc_head = &alloc_list;
static	IGSID	*next_proc_sid;

PTR
iiIGspStartProc( name_ref, parent )
ILREF	name_ref;
PTR	parent;
{
	IGRDESC	*rdesc;
	ILOP	op;

	if ( proc_tag == 0 )
	{
		proc_tag = FEgettag( );
	}
	rdesc = (IGRDESC *)FEreqmem( proc_tag, sizeof( IGRDESC ),
					(bool)TRUE, (STATUS *)NULL );
	if ( parent == NULL )
	{
		top_rdesc = rdesc;
		top_rdesc->igrd_dbdv_size = 0;
		rdesc->igrd_parent_sid = NULL;
		op = IL_MAINPROC;
	}
	else
	{
		db_values = ( (IGRDESC *)parent )->igrd_dbdv_end;
		rdesc->igrd_parent_sid = ( (IGRDESC *)parent )->igrd_sid;
		op = IL_LOCALPROC;
		IGsetSID( next_proc_sid );
	}
	rdesc->igrd_fdesc_off = n_syms;
	rdesc->igrd_vdesc_off = vdesc_size;
	rdesc->igrd_dbdv_off  = db_values;
	rdesc->igrd_name_ref  = name_ref;
	rdesc->igrd_sid       = IGinitSID( );
	IGsetSID( rdesc->igrd_sid );

	alloc_list_proc_head = alloc_list_tail;

	stack_size[ BYTES ] = stack_size[ WORDS ] = stack_size[ DOUBLES ] = 0;

	IGgenStmt( op, (IGSID *)rdesc, 0 );

	next_proc_sid = IGinitSID( );
	IGsetSID( next_proc_sid );

	IGgenStmt( IL_NEXTPROC, next_proc_sid, 0 );

	return (PTR)rdesc;
}

/*{
** Name:	iiIGepEndProc() - End a Local Procedure or Top Routine
**
** Description:
**	Sets ILG-internal information and generates appropriate IL
**	at the end of a local procedure or top routine
**	(a frame or a non-local procedure).
**
** Inputs:
**	routine	{PTR}	pointer to IGRDESC structure for routine being ended.
**
** Returns:
**	VOID
**
** History:
**	04/07/91 (emerson)
**		Created for local procedure support.
**		The logic that rounds up the word stack size
**		has been moved here from IGgenRefs.
*/

VOID
iiIGepEndProc( routine )
PTR	routine;
{
	IGRDESC	*rdesc = (IGRDESC *)routine;
	DBREF	*ref;
	i4	align;
	i4 stacksize, byte_stack_off, word_stack_off;
	char	buf[ 16 ];

	/*
	** Put a NULL FDESC entry at the end of the routine's FDESC entries
	** (because there are several places in abfrt where FDESC searches
	** are terminated by a null entry).
	*/
	IGsymbol((char *)NULL, (char *)NULL, (ILALLOC)0, (char)0, 0,
		 (char *)NULL);

	/*
	** Round up size of the WORD stack so BYTE stack will be aligned.
	** (Needed since BYTE stack may contain Nullable floats or other
	** types that must be aligned; use 'f8' as the most restrictive
	** alignment for IL portability across a network boundary.)
	*/
	align = stack_size[ WORDS ]  % sizeof( f8 );
	if ( align != 0 )
	{
		stack_size[ WORDS ]  += sizeof( f8 ) - align;
	}

	stacksize      = stack_size[ DOUBLES ] + stack_size[ WORDS ]
		       + stack_size[ BYTES ];
	byte_stack_off = stack_size[ DOUBLES ] + stack_size[ WORDS ];
	word_stack_off = stack_size[ DOUBLES ];

	for ( ref = *alloc_list_proc_head; ref != NULL; ref = ref->ir_next )
	{
		if ( ref->ir_sect == BYTES )
		{
			ref->db_offs += byte_stack_off;
		}
		else if ( ref->ir_sect == WORDS )
		{
			ref->db_offs += word_stack_off;
		}
	}

	CVla( stacksize, buf );
	rdesc->igrd_stacksize_ref =
				IGsetConst( DB_INT_TYPE, iiIG_string( buf ) );
	rdesc->igrd_fdesc_end = n_syms;
	rdesc->igrd_vdesc_end = vdesc_size;
	rdesc->igrd_dbdv_end  = db_values;

	if ( top_rdesc->igrd_dbdv_size < db_values )
	{
		top_rdesc->igrd_dbdv_size = db_values;
	}

	return;
}

/*{
** Name:    IGgenRefs() -	Output Interpreted Frame Object Data Descriptors
**
** Description:
**	This routine outputs both symbol table entries and data descriptors
**	for all allocated IL variables for an interpreted frame object.
**
** Input:
**	dfile_p		{FILE *}  Debug file pointer.
**
** History:
**	08/86 (jhw) -- Written.
**	21-Aug-89 (John Morris of HP)
**	Round the stack size for the WORD stack so that the BYTE stack is
**	aligned.  The problem was the BYTE stack was not always aligned so
**	a Nullable float located in it was also not aligned.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Moved the logic to round up the WORD stack size and the logic
**		to adjust offsets of items in the BYTE and WORD stacks
**		to the new function iiIGepEndProc.
**		Set stacksize to 0 in call to IIAMisIndirSect.
**		Change handling of null FDESC entries: they can now be
**		embedded in the FDESC (in addition to appearing at the end), so
**		we now let IGsymbol generate them: IGgenRefs just copies them.
**		We also now assume that all character pointers in non-null
**		FDESC entries have already be set to non-null pointers.
**		Also added statement to copy prec from ref into VDESC.
**		Also prettified the dump of the VDESC array.
**	18-jun-92 (davel)
**		changed format of IL debug output for db_precision to %04x.
**	26-jun-92 (davel)
**		do not set VDESC's length for place-holder temps. This routine
**		was setting the length for integer or float VDESC's.
*/

VOID
IGgenRefs (dfile_p)
FILE	*dfile_p;
{
    register DBREF		*ref;
    register IL_DB_VDESC	*dbp;
    register IL_DB_VDESC	*db_array;
    FDESCV2			*sym_tab;
    STATUS			status;

    if (vdesc_size == 0 || alloc_list == NULL)
	return;

    /* allocate. note we allocate n+1 FDESCs, the extra will be empty */
    while ((db_array =
	(IL_DB_VDESC *)_FEalloc(vdesc_size*sizeof(*db_array), &status)) == NULL 
      || (sym_tab = (FDESCV2*)_FEalloc((n_syms + 1) * sizeof(*sym_tab), &status))
			== NULL)
    {
	EXsignal(EXFEMEM, 1, ERx("IGgenRef"));
    }

    /* Build DB Data Value Descriptors */

    if (dfile_p != NULL)
    { /* print debug info */
	SIfprintf(dfile_p, ERx("ALLOCATION\n"));
    }
    for (ref = alloc_list, dbp = db_array; ref != NULL;
	 ref = ref->ir_next, ++dbp)
    {
	dbp->db_datatype = ref->ir_type;
	dbp->db_length = ref->ir_len;
	dbp->db_prec = ref->ir_prec;
	dbp->db_offset = ref->db_offs;
        if (dfile_p != NULL)
	{ /* print debug info */
	    SIfprintf(dfile_p, ERx("%6d,%6d:%6d(%6d,0x%04x) @%9d\n"),
			dbp - db_array + 1, ref->db_value,
			dbp->db_datatype, dbp->db_length, dbp->db_prec,
			dbp->db_offset
	    );
	}
    }
    alloc_list = NULL;

    IIAMisIndirSect(db_array, vdesc_size, 0);

    /* Build symbol table */

    if (n_syms > 0)
    {
	register SYMTAB	*sym;
	register FDESCV2 *stp;

	/*
	** Put another NULL FDESC entry at the end of the FDESC entries.
	*/
	IGsymbol((char *)NULL, (char *)NULL, (ILALLOC)0, (char)0, 0,
		 (char *)NULL);

	sym = symbol_table->ir_snext;
	symbol_table->ir_snext = NULL;
	symbol_table = NULL;
	for (stp = sym_tab ; sym != NULL ; sym = sym->ir_snext, ++stp)
	{
	    stp->fdv2_name = sym->ir_name; 
	    stp->fdv2_tblname = sym->ir_parent; 
	    stp->fdv2_visible = sym->ir_visible;
	    stp->fdv2_flags = sym->ir_flags;
	    stp->fdv2_dbdvoff = sym->ir_ref - 1;
	    stp->fdv2_order = 0;
	    stp->fdv2_typename = sym->ir_typename; 

	} /* end symbol table for */

	IIAMstSymTab(sym_tab, n_syms - 1);
    }

    FElpdestroy(listp);
    listp = NULL;
}

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ADEINT.H - Internal structures and constants needed for ADE.
**
** Description:
**      This file holds typedefs and constant definitions needed internally
**	by ADF's Compiled Expression Module, ADE.  Throughout the file you
**	will see "compiled expression" abbreviated as "CX".  This file depends
**	on <compat.h>, <dbms.h>, and <ade.h> already being #included.
**
** History: $Log-for RCS$
**	17-jun-86 (thurston)
**          Initial creation.
**	10-jul-86 (thurston)
**	    Changes to the ADE_INSTRUCTION typedef:  Removed the .ins_ops[1]
**          member, since some instructions will not have any operands, and this
**          would be misleading, as well as a little difficult to code around.
**          Also removed the .ins_rsvd member. 
**	16-jul-86 (thurston)
**	    Changed the ADE_CONSTANT typedef:  Made .con_rsvd an array to
**	    round structure up to 16 bytes.
**	21-jul-86 (thurston)
**	    Added the ADE_CXINPROGRESS constant.
**	18-nov-86 (thurston)
**	    Added some pre-compiler lines to assure that either ADE_BACKEND
**	    or ADE_FRONTEND (and not both) are defined.
**	25-mar-87 (thurston)
**	    In ADE_CXHEAD, I changed f4 cx_version to be i2's cx_hi_version and
**	    cx_lo_version.  Also tweaked the version constants.
**	06-may-87 (thurston & wong)
**	    Integrated FE changes.
**	13-may-87 (thurston)
**	    Added ADE_END_OFFSET_LIST.
**	14-may-87 (thurston)
**	    Increased max # VLT's in any CX (ADE_MXVLTS) from 25 to 100.
**	10-feb-88 (thurston)
**	    Used the reserved space at the end of the CX header struct for the
**	    new fields .cx_n_vi, .cx_vi_1st_offset, and .cx_fi_last_offset which
**	    are needed in support of the new VIRGIN segment.
**	05-apr-89 (jrb)
**	    Added opr_prec and opr_1rsvd to ADE_I_OPERAND for decimal project.
**	    Added con_prec to ADE_CONSTANT.
**	05-jan-98 (nicph02)
**	    Increased max # VLT's in any CX (ADE_MXVLTS) from 100 to 500.
**	    (bug 48794)
**      22-Nov-1999 (hweho01)
**          Changed opr_offset from int to long for avoiding the truncation  
**          of address, since pointer is 8-byte for 64-bit platforms.
**          
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	02-aug-2003 (somsa01)
**	    Updated opr_offset to be of type OFFSET_TYPE.
**	14-Feb-2005 (schka24)
**	    Additions for subroutine-threaded code (STCODE).
**	29-Aug-2005 (schka24)
**	    Add a couple more CX-segment flags.
**	04-Aug-2010 (kiria01) b124160
**	    Raised limit handling on vlts. Now we have a ADE_MXVLTS_SOFT
**	    upto which the stack will be used and beyond, the limit can
**	    get to ADE_MXVLTS which is the max supported in the CXHEAD.
**	    To handle this memory will be allocated temporarily to extend
**	    the workspace. Based on this the soft limit bas been lowered
**	    from what it was as it no longer needs to represent the hard
**	    limit.
**/


/*
[@forward_type_references@]
[@forward_function_references@]
[@global_variable_references@]
*/


/*
**      The following pre-compiler lines assure that either ADE_BACKEND or
**      ADE_FRONTEND have been #define'd, since this header file depends
**	on one or the other of them being defined.  If both are defined, or
**	neither are defined, the code including this header file will not
**	compile.
*/

#ifdef ADE_BACKEND
#   ifdef ADE_FRONTEND
	@@@  Both ADE_BACKEND and ADE_FRONTEND are defined.
	@@@  Your <adefebe.h> file is probably screwy.
#   endif
#else
#   ifndef ADE_FRONTEND
	@@@  Neither ADE_BACKEND nor ADE_FRONTEND is defined.
	@@@  You probably need to include <adefebe.h>.
#   endif
#endif


/*
**      This macro is used internally to round a number up
**      to the nearest multiple of some other number.
**
**	( AT SOME POINT, A PROPOSAL SHOULD BE MADE TO PUT
**	SUCH A MACRO INTO THE CL. )
*/

#ifndef    ADE_FRONTEND

# define	ADE_ROUNDUP_MACRO(x,s)  if ((x)%(s)) (x) += (s) - ((x)%(s))

#else   /* ADE_FRONTEND */

# define	ADE_ROUNDUP_MACRO(x,s)	x = x

#endif /* ADE_FRONTEND */


/*
**      The following constants specify information
**      about the ADE system as a whole.
*/

#define		    ADE_CXHIVERSION ((i2) 1)	/* Major version # of ADE */
#define		    ADE_CXLOVERSION ((i2) 0)	/* Minor version # of ADE */

#define		    ADE_CXINPROGRESS ((i2) -1114)   /* Flag to show CX is in */
                                                    /* the process of being  */
                                                    /* compiled.  -1114 is   */
                                                    /* not magic in any way. */
                                                    /* I just chose my       */
                                                    /* favorite number.      */

#define		    ADE_MXVLTS		MAXI2	    /* Max # VLTs in any CX */
#define		    ADE_MXVLTS_SOFT	100	    /* Max # VLTs that we handle
						    ** directly on the stack */
#define		    ADE_END_OFFSET_LIST	    (-1)    /* Offset list terminater */
						    /* for CX's.              */

/*
[@defined_constants@]...
*/

/*
[@group_of_defined_constants@]...
*/


/*}
** Name: ADE_I_OPERAND - Internal representation of a CX instruction's operand.
**
** Description:
**      This describes the internal structure (i.e. what is actually compiled
**      into the CX) of an operand for an instruction in a CX.  This differs
**      from the external ADE_OPERAND typedef found in <ade.h> for two reasons.
**      First, it gives us the flexibility to change the internal format of a CX
**      without affecting the interface to the ADE routines.  Second, since the
**      Front End and PC groups are more concerned about space then they are
**      about speed, and since the bulk of a CX will be made up of operands,
**      they wish to have some of the information normally contained in an
**      operand "hidden" in a array of DB_DATA_VALUEs that they will have,
**      anyway.  Therefore, because of this, there will actually be two formats
**      of an ADE_I_OPERAND.  One will be used when compiling ADE for the Back
**      End, and the other will be used when compiling for the Front End or PC
**      group. MACROS will be used to get an ADE_OPERAND from an ADE_I_OPERAND
**      and to put and ADE_OPERAND into an ADE_I_OPERAND so as to maintain one
**      set of source code. Both the MACRO definitions and a #define for either
**	ADE_BACKEND or ADE_FRONTEND will be in the single header file
**	<adefebe.h>.  This will be the only source file that two versions must
**      be kept for. 
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	05-apr-89 (jrb)
**	    Added opr_prec and opr_1rsvd for decimal project.
**	02-aug-2003 (somsa01)
**	    Updated opr_offset to be of type OFFSET_TYPE.
**	20-dec-04 (inkdo01)
**	    Added opr_collID for collations.
**	10-Feb-2005 (schka24)
**	    Make ADE_I_OPERAND a synonym for ADE_OPERAND for backend,
**	    since the two are identical anyway.
*/

#ifdef ADE_BACKEND

    /* In the backend, use the standard ADE operand definition */

#define ADE_I_OPERAND ADE_OPERAND

#else

    /* -------------------------------------------------------------------- */
    /* This structure for an operand will be used by Front Ends and PC      */
    /* since they are more concerned about space than they are about speed. */
    /* -------------------------------------------------------------------- */

typedef i2 ADE_I_OPERAND;	/* The index into the array of
				** DB_DATA_VALUEs pointed to by
				** base[ADE_DVBASE] where the data
				** value to use as this operand can
				** be found.
			 	*/

#endif


/*}
** Name: ADE_INSTRUCTION - A CX instruction.
**
** Description:
**      This describes the structure of an instruction in a CX.  It is not
**      visable outside of the ADE code.  A CX will keep four ordered lists
**	of instructions; the VIRGIN, INIT, MAIN, and FINIT lists.  These will
**	also be referred to as the VIRGIN, INIT, MAIN, and FINIT "segments"
**	of the CX.
**
** History:
**      17-jun-86 (thurston)
**          Initial creation.
**	10-jul-86 (thurston)
**	    Removed the .ins_ops[1] member, since some instructions will not
**	    have any operands, and this would be misleading, as well as a little
**	    difficult to code around.  Also removed the .ins_rsvd member.
*/
typedef struct
{

#ifdef    ADE_BACKEND
    i4              ins_offset_next;	/* Offset from base[ADE_CXBASE] of next
					** instruction in the segment.
					*/
#endif /* ADE_BACKEND */

    i2              ins_icode;          /* The instruction code.  This will
                                        ** either be one of the ADF function
                                        ** instance IDs, or one of the CX
                                        ** special instructions. 
					*/
    i2              ins_nops;           /* # operands in the following array. 
					*/

    /* The instructions operands (if any) will follow this structure. */

}   ADE_INSTRUCTION;


/*}
** Name: ADE_CONSTANT - A CX constant.
**
** Description:
**      This describes the structure of a constant that has been compiled
**      into a CX.  It is not visable outside of the ADE code.  A CX will keep
**      a list of constants.  This typedef actually only describes the header
**	of a CX constant.  The actual data of the constant will follow this
**	header immediately.
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	16-jul-86 (thurston)
**	    Made .con_rsvd an array to round structure up to 16 bytes.
**	05-apr-89 (jrb)
**	    Added con_prec by taking an i2 out of the rsvd array.
**	20-dec-04 (inkdo01)
**	    Added con_collID to ADE_CONSTANT for collations.
*/
typedef struct
{
    i4              con_offset_next;	/* Offset from base[ADE_CXBASE] of
                                        ** previous constant.
                                        */
    i4              con_len;            /* Length of constant's data.
                                        */
    DB_DT_ID        con_dt;             /* Datatype of constant.
                                        */
    i2		    con_prec;		/* Precision/Scale of constant.
					*/
    i2              con_collID;		/* Collation ID of constant.
					*/

    /* The constant's actual data will immediately follow this structure. */

}   ADE_CONSTANT;


/*}
** Name: ADE_CXSEG - CX segment info for header
**
** Description:
**	Each CX consists of up to 4 different code segments:  the
**	MAIN segment, the INIT segment, the FINIT segment, and the
**	VIRGIN segment.  (There's also constants, but they aren't a
**	code segment as such.)
**
**	The VIRGIN segment is executed once (by QEF) at the start of a
**	query execution, to evaluate expressions that are actually query
**	constants.  While the need for VIRGIN segments is partly due to
**	the current parser's lack of constant folding, we also need them
**	to evaluate query constants that involve repeat query parameters,
**	which aren't known until query execution time.
**	If the CX operates on VLT/VLUPs,  the ADE_CALCLEN instructions
**	needed are found (only) in the VIRGIN segment.
**
**	The INIT segment is for aggregates;  it's executed at the start of
**	a new group to initialize the aggregate value(s) for that group.
**
**	The MAIN segment is the normal code; whatever is to be evaluated
**	for each execution of the CX.  For aggregates, it's the code
**	to first check that the current row is part of the current group,
**	and then it includes the current row into the aggregate(s).
**	(The comparing part might be done elsewhere, for some kinds of
**	aggregates.)
**
**	The FINIT segment is executed when a group is finished;  if there is
**	additional execution needed to finish evaluating an aggregate, like
**	e.g. AVG, it's executed in the FINIT.  FINIT also copies the
**	aggregate results to the output.
**
**	Since keeping track of the various CX segments is pretty much
**	identical for each segment type, we'll define a structure that
**	tracks the segment info.
**
** History:
**	14-Feb-2005 (schka24)
**	    Gather up common segment stuff from CXHEAD.
**	22-Feb-2007 (kschendel)
**	    i2 for instruction count isn't large enough for some of the
**	    ghastly experian selects.  Make it a u_i4.
**
*/

typedef struct _ADE_CXSEG {

	struct	_STCODE_OP *seg_stcode;	/* Address of start of STCODE, if
					** generated for this seg
					*/
	i4	seg_1st_offset;		/* Offset from start of CX to first
					** instruction of this segment;
					** ADE_END_OFFSET_LIST if none
					*/
	i4	seg_last_offset;	/* Offset from start of CX to last
					** instruction of this segment
					*/
	u_i4	seg_n;			/* Number of ADE instructions in seg */
	u_i2	seg_flags;		/* Flags for this CX segment */

#define ADE_CXSEG_PMQUEL	0x0001	/* Set if CX needs to start with the
					** Quel pattern-match flag TRUE.
					*/
#define ADE_CXSEG_LABELS	0x0002	/* Set if CX has branches in it */
#define ADE_CXSEG_FANCY		0x0004	/* Set if CX has "fancy" stuff in it,
					** meaning blobs or VLT/VLUP that
					** require some extra work at runtime.
					*/

} ADE_CXSEG;

/*}
** Name: ADE_CXHEAD - A CX header.
**
** Description:
**      This describes the header portion of a CX.  One of these structures
**      will appear at the top of each CX, and will be used to control the CX
**      both during compilation and during execution.  As with all of the other
**      structures that can appear in a CX, once the CX is compiled this will
**      be a read only structure.  This structure will not be visable outside of
**      the ADE code.
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	25-mar-87 (thurston)
**	    Changed f4 cx_version to be i2's cx_hi_version and cx_lo_version.
**	10-feb-88 (thurston)
**	    Used the reserved space at the end of this structure for the new
**	    fields .cx_n_vi, .cx_vi_1st_offset, and .cx_fi_last_offset which
**	    are needed in support of the new VIRGIN segment.
**	14-Feb-2005 (schka24)
**	    Extract segment stuff into ADE_CXSEG.
*/
typedef struct
{
    i2              cx_hi_version;      /* Major version of CX this one is.
                                        */
    i2              cx_lo_version;      /* Minor version of CX this one is.
                                        */
    i4              cx_allocated;       /* When inited, this is set to the size
                                        ** of the buffer supplied to hold the
                                        ** CX.  The CX will not be allowed to
                                        ** grow larger than this. 
                                        */
    i4              cx_bytes_used;      /* The actual # bytes used by the CX.
                                        ** During compilation, this will be used
                                        ** to find the next available location
                                        ** for an instruction or constant to be
                                        ** compiled into the CX, then updated to
                                        ** reflect the amount of memory used by
                                        ** that instruction or constant. 
                                        */
    i4              cx_k_last_offset;   /* Offset from base[ADE_CXBASE] of the
                                        ** last constant.
                                        */
    ADE_CXSEG	    cx_seg[ADE_SMAX+1];	/* Segment info.
					** FIXME note that ade.h defines the
					** segment numbers as 1-origin, hence
					** the +1.  The [0] slot is wasted
					** at present.
					*/
    i2              cx_n_k;             /* The # of constants.
                                        */
    i2              cx_n_vlts;          /* The # of VLTs.
                                        */
    i2              cx_hi_base;         /* The highest numbered base used.
                                        */
    i2              cx_alignment;       /* The alignment used to compile this CX
                                        ** with.  At ade_bgn_comp() time this
                                        ** will be set to sizeof(ALIGN_RESTRICT)
                                        ** and never changed.  If this CX gets
                                        ** moved to another machine with
                                        ** different alignment requirements,
                                        ** this can be checked against that
                                        ** machine's sizeof(ALIGN_RESTRICT) to
                                        ** see if the CX will be valid on that
                                        ** machine.  ADE will not initially
                                        ** provide this checking, but this
                                        ** typedef member is here in case we
                                        ** want to do something like this in the
                                        ** future. 
                                        */
}   ADE_CXHEAD;


/*}
** Name: ADE_VLT_WS_STRUCT - Used to estimate VLT space, and for ADE_CALCLEN.
**
** Description:
[@comment_line@]...
**
** History:
**	22-jul-86 (thurston)
**	    Initial creation.
*/
typedef struct
{
    i4              vlt_base;		/* Holds the base for the
                                        ** corresponding VLT.
                                        */
    i4         vlt_len;            /* Holds the # bytes required in the VLT
                                        ** space for the corresponding VLT.
                                        */
}   ADE_VLT_WS_STRUCT;

/*
** Name: Subroutine-Threaded Code (STCODE) definitions
**
** Description:
**	In an attempt to make CX execution run faster, its final executable
**	form will be what's often call subroutine-threaded code (STCODE
**	for short).  STCODE is basically a list of routine addresses
**	and operands, such that the main execution loop can be a very
**	tight loop:
**		while (more code) do
**		    resolve operand addresses
**		    call (*routine-addr)(operands)
**		endwhile
**
**	This transformation by itself is of minimal benefit;  but we'll
**	also take the opportunity to resolve the called routine for
**	commonly used operands.  So, instead of calling an "integer add"
**	FI routine that has to figure out the input and result integer
**	lengths, we'll resolve all the way to calling "add i4 and i2
**	giving an i4".  Obviously this means lots more little routines
**	to do the work, but the routines are short and easy to write;
**	and the savings can be significant.
**
**	Some interpreters go even farther, and attempt to arrange for
**	each little called-routine to do some kind of jump (eg an indirect
**	jump) to the next routine in the code sequence.  Unfortunately,
**	that's hard to do in a portable-C manner.  If I'm going to make
**	that sort of effort, we're going all the way to compiling native
**	code on popular platforms.
**
**	Each STCODE operation contains a call type (there are a few different
**	ways of calling the handler routine);  the original ADE_xxx opcode,
**	so that we can write more generic handlers for less time critical
**	operations;  the number of operands, and a list of operands following.
**
**	For the first try, we aren't generating STCODE for front-ends,
**	nor for any CX that contains VLT/VLUPs or large-object code.
**	These restrictions are merely to simplify the initial version.
**
**	The business of first generating a traditional CX, and then
**	generating STCODE from the CX, may sound stupid -- why not just
**	generate the STCODE directly?  That's probably where we want to
**	end up, but to minimize the impact on the rest of the back-end
**	(and to avoid having to figure out how to deal with front-ends),
**	we'll keep all the traditional CX machinery around.  (Note that
**	compiled ABF programs and possibly compiled forms may have
**	traditional CX code wired into them, so removing the traditional
**	stuff is not to be undertaken lightly.)
**
** History:
**	14-Feb-2005 (schka24)
**	    Take a swing at writing an STC interpreter in hopes that it's
**	    faster than the current ade-execute stuff.
*/

/* Define the longest operand list allowed in one operation.  Allow
** for MECOPY, which combines a series of ADE_MECOPY pairs into one
** operation.  Allow for 10 pairs (20 operands), which is probably well
** into the point of dimishing returns.
*/
#define MAX_STCODE_OPERANDS	20

typedef struct _STCODE_OP {
	void (*handler)();		/* Handler to call */
	i2		opcode;		/* ADE_XXXX or ADFI_xxx opcode.
					** In a few instances, special stuff
					** is placed here, e.g. a testing
					** operator code.
					*/
	i1		call_type;	/* Handler call type, see below */
	i1		num_operands;	/* Number of operands 0..n */
	ADE_OPERAND	operand[MAX_STCODE_OPERANDS]; /* Operands here */
} STCODE_OP;

/* Call types: */

#define STC_CALL_STATE	0		/* handler(&execution_state) */
#define STC_CALL_DATA	1		/* handler(&result, &op1, &op2) */
#define STC_CALL_CMP	2		/* cmpval = handler(&op1, &op2)
					** and then set excb_cmp, return if
					** handler returned not-equal.
					** (this call type for ADE_COMPARE
					** and ADE_BYCOMPARE fastpath)
					*/
#define STC_CALL_TEST	3		/* cmpval = handler(&op1, &op2)
					** and test return based on opcode.
					** (this call type for testing-ops)
					*/
#define STC_CALL_SCMP	4		/* cmpval = handler(&execution_state)
					** and then as for CALL_CMP
					*/
#define STC_CALL_STEST	5		/* cmpval = handler(&execution_state)
					** and then as for CALL_TEST
					*/


/* Testing operator codes stuck into opcode for STC_CALL_TEST and
** STC_CALL_STEST types.
** IMPORTANT: these code are defined such that if you take the
** result of a compare handler (-1 = a<b; 0 = a=b; 1 = a>b), and
** test testcode & (1<<(result+1)), the answer is nonzero if the testing
** operator is TRUE.
** Also note that for LT/LE/GE/GT, you can swap operands by XOR'ing the
** testing-operator code with 7.
*/
#define	STC_TESTOP_LT	1
#define STC_TESTOP_LE	3
#define STC_TESTOP_EQ	2
#define STC_TESTOP_NE	5
#define STC_TESTOP_GE	6
#define STC_TESTOP_GT	4

/* A couple potentially useful macros for computing the actual size of
** an STCODE_OP.  (The structdef is the maximum size, to make defining
** the execution state struct simple.)
*/

/* Size of STCODE operation pointed to by op. */
#define STCODE_OP_SIZE_MACRO(op) (sizeof(STCODE_OP) - (STCODE_MAX_OPERANDS - (op)->num_operands)*sizeof(ADE_OPERAND))

/* Size of an STCODE operation with N operands */
#define STCODE_OP_N_SIZE_MACRO(n) (sizeof(STCODE_OP) - (STCODE_MAX_OPERANDS - (n))*sizeof(ADE_OPERAND))


/*
** Name: STCODE_STATE -- STCODE execution state holder
**
** Description:
**	As execution of an STCODE compiled expression proceeds, we
**	need to keep around various bits of state information.
**	Often, an STCODE handler routine is interested in some of
**	that state (e.g. the next PC, or the current boolean expr value).
**	So, it's convenient to bundle up the state into a structure.
**
**	The STCODE operation list is read-only (multiple sessions
**	could be sharing it), but the execution state is local.  So,
**	this is where we hold resolved operand addresses.  For extreme
**	cases, like unknown-length operands, a duplicate of the entire
**	operation can be resolved into the execution state area, and
**	the PC temporarily pointed to that duplicate.
**
** History:
**	14-Feb-2004 (schka24)
**	    Initial version.
*/

typedef struct _STCODE_STATE {
    struct _ADF_CB *adf_scb;		/* ADF session control block addr */

    PTR		*base_array;		/* Pointer to operand base addresses,
					** to be indexed by an operand's
					** opr_base
					*/

    STCODE_OP	*pc;			/* Current address within STCODE */
    STCODE_OP	*newpc;			/* Next address within STCODE
					** Normally it's the current op plus
					** its size, but branches can change
					** that.
					*/
    char	*addr[MAX_STCODE_OPERANDS];  /* Operand resolved addresses */
    STCODE_OP	resolved_op;		/* Copy of operation (and operands)
					** with all lengths resolved; only
					** needed for VLT/VLUP, which may
					** not be implemented for the first
					** pass.
					*/

    i4		cx_value;		/* Value of conditional expr:
					** one of ADE_TRUE, ADE_NOT_TRUE,
					** or ADE_UNKNOWN.  Important!
					** ADE_TRUE is C's true (1), and
					** ADE_NOT_TRUE is C's false (0).
					** This value ends up in excb_value.
					*/

    i4		cx_cmp;			/* Value of last-executed ADE_COMPARE
					** or ADE_BYCOMPARE operation:
					** ADE_1LT2 (must be -1)
					** ADE_1EQ2 (must be 0)
					** ADE_1GT2 (must be 1)
					** or null indicators:
					** ADE_1ISNULL, _2ISNULL, or _BOTHNULL.
					** This ends up in excb_cmp.
					*/

    DB_STATUS	call_stat;		/* Status for current STCODE op,
					** if it calls other ADF routines.
					*/
    DB_STATUS	final_stat;		/* Status to return from the CX */

    UCS2	*uesc_ptr;		/* Same thing, for Unicode */
    u_char	*esc_ptr;		/* Escape char ptr for LIKE */
    UCS2	uesc_char;		/* Escape char for LIKE, Unicode */
    u_char	esc_char;		/* Escape char for LIKE */

    i2		num_operands;		/* Copy of # of operands for current
					** STCODE op, for convenience
					*/
    i2		opcode;			/* Copy of opcode for current op,
					** again for convenience
					*/
    i2		cx_seg;			/* The CX segment we're executing:
					** ADE_SMAIN, ADE_SINIT, etc etc
					*/
    bool	nullskip;		/* Initially FALSE, set to TRUE if
					** an operation was skipped because
					** of an operand being NULL (only
					** for aggregates, the 2CXI_SKIP in
					** particular, and all it does is set
					** a status for the embedded sql
					** SQLCA.)
					** Ends up in excb_nullskip.
					*/

    bool	pmquel;			/* Initially set from the cx seg
					** flag ADE_CXSEG_PMQUEL, tracks
					** whether we want Quel string testing
					** operator semantics (implicit LIKE)
					*/
} STCODE_STATE;



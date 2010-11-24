/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <dmucb.h>


/**
** Name: DM1U.H - Definitions for check/patch table utility routines.
**
** Description:
**      This file provides structures/constants for use by the routines that
**	check or patch a table's disk file.
**
** History:
**      16-Aug-89 (teg)
**          Initial Creation.
**	31-oct-89 (teg)
**	    additional fields/flags in DM1U_CB for check table development.
**	15-mar-1991 (Derek)
**	    Re-organized to add support for allocation bitmaps.
**	07-july-1992 (jnash)
**	    Add DMF Function prototypes.
**	23-oct-92 (teresa)
**	    added DM1U_VERBOSE_FL for sir 42498.
**	30-feb-1993 (rmuth)
**	    Get verifydb working changes.
**	      - Add DM1U_10ISAM_INDEX.
**	      - Move dm1u_talk into here.
**	30-apr-1993 (rmuth)
**	    Changed dm1u_chain and dm1u_map to u_char from char to avoid
**	    overflow problem.
**	06-mar-1996 (stial01 for bryanp)
**	    Just a comment: the DM1U_CB and DM1U_ADF control blocks are VERY
**		large and cannot safely be allocated on the stack. You must
**		dynamically allocate and free these control blocks, or you will
**		get stack overflow problems.
**      06-mar-1996 (stial01)
**          Add tuple buffer pointer to DM1U_CB
**	13-apr-99 (stephenb)
**	    Add DM1U_PERIPHERAL modifier to check validity of peripheral
**	    tables and/or coupons.
**      19-jan-2000 (stial01)
**          Added DM1U_KPERPAGE
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Mar-2004 (schka24)
**	    Try to cut down on massive flag confusion some.
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1u_? functions converted to DB_ERROR *
**	15-Oct-2010 (kschendel)
**	    Grrrr ... another instance of two names for the same thing,
**	    in this case the VERBOSE flag.  Define in terms of the
**	    definition in dmucb.h.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    dm1u_talk() rewritten with variable parameters, prototyped.
**/

/*
**  This section contains constants used by the DM1U.C code.
*/

/*  Operation codes for dm1u_verify(). */
/*  It makes sense to use the same flag and type definitions as the
**  original DMU caller, with the one exception that the DMU caller
**  will pass a separate dmu-characteristics entry for modifiers;  while
**  dm1u (and indeed everything below the dmumodify logical level)
**  wants everything in a single operator word.
*/

#define	    DM1U_VERIFY	    DMU_V_VERIFY
#define	    DM1U_REPAIR	    DMU_V_REPAIR
#define	    DM1U_PATCH	    DMU_V_PATCH
#define	    DM1U_FPATCH	    DMU_V_FPATCH
#define	    DM1U_DEBUG	    DMU_V_DEBUG

#define	    DM1U_OPMASK	    0x07	/* Ideally would be in dmucb.h */

/* This modifier is actually part of the "verify" characteristics entry */
#define	    DM1U_VERBOSE_FL DMU_V_VERBOSE  /* if set, display informative msgs */

/*  Operation modifiers for dm1u_verify(). */
/*  These originate with DMU_VOPTION;  just shift things over a bit. */

#define     DM1U_MODSHIFT   4		/* From 0x000x to 0x00x0 */
#define	    DM1U_BITMAP	    (DMU_T_BITMAP << DM1U_MODSHIFT)
#define	    DM1U_LINK	    (DMU_T_LINK << DM1U_MODSHIFT)
#define	    DM1U_RECORD	    (DMU_T_RECORD << DM1U_MODSHIFT)
#define	    DM1U_ATTRIBUTE  (DMU_T_ATTRIBUTE << DM1U_MODSHIFT)
#define	    DM1U_PERIPHERAL (DMU_T_PERIPHERAL << DM1U_MODSHIFT)
#define     DM1U_KPERPAGE   0x400

/*
**  Forward and/or External typedef/struct references.
*/

    /* return status */
#define		DM1U_INVALID	    -1
#define		DM1U_GOOD	    1
#define		DM1U_BAD	    0
#define		DM1U_FIXED	    -1

    /* other */
#define		DM1U_MSGBUF_SIZE    500

/* btree constants */
#define		BTREE_START	    2	/* page 0 is btree root, page 1 is
					** btree free list header, and page 2
					** is the 1st non-designated page */
#define		BTREE_FREEHDR	    1
#define		BTREE_ROOT	    0

#define		DM1U_1DATAPAGE	    1
#define		DM1U_2FREEPAGE	    2
#define		DM1U_3LEAFPAGE	    3
#define		DM1U_4LEAFOVFL	    4
#define		DM1U_5INDEXPAGE	    5
#define		DM1U_6FHDRPAGE	    6
#define		DM1U_7FMAPPAGE	    7
#define		DM1U_8UNKNOWN	    8
#define		DM1U_9EMPTYDATA	    9
#define		DM1U_10ISAM_INDEX   10




/*}
** Name:  DM1U_ADF - ADF Data Value Blocks for attributes in the tuple.
**
** Description:
**      This structure is a "prebuilt" adf_dv for each attribute in the tuple.
**	It is used for calls to adc_valchk on a record returned from dm1c_get.
**
** History:
**      29-Aug-89 (teg)
**          Initial Creation.
**	7-aug-1991 (bryanp) -- B37626, B38241
**	    Added dm1u_colbuf to the DM1U_ADF for alignment-sensitive machines.
**	17-apr-97 (pchang)
**	    Moved dm1u_colbuf to the beginning of the structure to eliminate
**	    the need to do ALIGN_RESTRICT as part of the fix for bug 79871.
**	18-Oct-2001 (inifa01) bug 106094, INGSRV 1576.
**	    Upgradedb from II 2.0 to II 2.5 causes server crash with SEGV in
**	    memcpy() (called from data_page()) and E_SC0206, E_LQ002E.  verifydb
**	    also causes same SEGV with error E_QE009C.
**	    The record holder buffer (adf->dm1u_record[]) is allocated a 
**	    max_tuple_length of 2008 which doesn't account for > 2k pages. 
**	    max_tuple_length is in range 0 - 32767, changed buffer allocation to
**	    size DM_MAXTUP (32767) instead of DB_MAXTUP (2008).
**      01-Nov-2010 (frima01) BUG 124670
**	    Enforce 8 Byte alignment for dm1u_colbuf to avoid BUS errors.
*/

typedef struct _DM1U_ADF
{
#ifdef BYTE_ALIGN
    union {
	i8          align_enforcement; /* enforce 8 Byte alignment */
        char	    colbuf[DB_MAXTUP]; /* aligned buffer into which column
					    ** values are copied before they are
					    ** passed to ADF.  Do not move this
					    ** field; use ALIGN_RESTRICT if you
					    ** need to otherwise.
					*/
	} dm1u_colbuf;
#endif
    i4	    dm1u_numdv; 	    /* number of DVs for this tuple */
    char	    dm1u_record[DM_MAXTUP]; /* buffer to hold a record */
    ADF_CB	    dm1u_cbadf; 	    /* ADF session control block */
    DB_DATA_VALUE   dm1u_dv[DB_MAX_COLS];   /* array of DVs - 1 for each
					    ** attribute
					    */
} DM1U_ADF;

/*}
** Name:  DM1U_CB - Description of control block for check/patch table.
**
** Description:
**      This structure is the control block that dm1u routines use to keep 
**	track of allocated memory specific to the check/patch table operations.
**	Note: memory is allocated separately, rather than as 1 large block.  
**	      This is because the dm1u_map, dm1u_chain and dm1u_adf are varaible
**	      length, and can get quite large.  There is more chance of getting
**	      three separate blocks in a stressed system than getting 1 large
**	      contigious block.
**
**	The relationship between the allocated memory is as follows:
**  
**	 DM1U_CB		     ALLOCATED MEMORY
**	:-------------:             :---------------:
**	: dm1u_xchain :------------>: DMP_MISC      :
**	:	      :		    :---------------:
**	: dm1u_chain  :------------>:allocated mem  :
**	:	      :		    :for chain map  :
**	:	      :		    :---------------:
**	:	      :
**	:	      :		    :---------------:
**	: dm1u_xmap   :------------>: DMP_MISC	    :
**	:	      :		    :---------------:
**	: dm1u_map    :------------>:allocated mem  :
**	:	      :		    : for page map  :
**	:-------------:		    :---------------:
**
**
** History:
**      16-Aug-89 (teg)
**          Initial Creation.
**	31-oct-89 (teg)
**	    additional flags in DM1U_CB for check table development.
**	22-feb-89 (teg)
**	    added new fields dm1u_broken_chain, dm1u_bufa and dm1u_bufb for
**	    checking ISAM tables.
**	23-oct-92 (teresa)
**	    added dm1u_flags and DM1U_VERBOSE for sir 42498.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO
**	    structures.
*/
struct _DM1U_CB
{
    i4	dm1u_op;	    /* Operation */
    i4     dm1u_errcode;
    i4	dm1u_e_cnt;	    /* Count of errors found. */
    i4	dm1u_lastpage;	    /* page number of last page in file */
    i4	dm1u_state;	    /* State of processing. */
#define		    DM1U_S_ORPHAN   0x01
    DMP_RCB	*dm1u_rcb;	    /* Copy of RCB pointer. */
    char        *dm1u_tupbuf;       /* tuple buffer if needed */
    DMP_PINFO	dm1u_index; 	    /* place to fix index pages to */
    DMP_PINFO	dm1u_data;  	    /* place to fix data pages to */
    DMP_PINFO	dm1u_leaf;  	    /* place to fix leaf pages to */
    DMP_PINFO	dm1u_ovfl;	    /* place to fix leaf overflow pages to */
    DMP_PINFO	dm1u_temp;  	    /* temp place to fix pages to */
    u_char 	*dm1u_chain;	    /* map of pages referenced in this chain */
    u_char	*dm1u_map;	    /* map of refreneced referenced */
    DM_PAGENO	dm1u_1stleaf;	    /* page number of leaf with -infinity marked
				    ** on it -- ie, leftmost leaf in tree */
    DM_PAGENO	dm1u_lastleaf;	    /* page number of leaf with +infinity marked
				    ** on it -- ie, rightmost leaf in tree */
    u_i4	dm1u_numleafs;	    /* number of leaf pages in this btree */
    i4	dm1u_size;	    /* number of bytes in dm1u_chain & dm1u_map*/
    DM1U_ADF	dm1u_adf;	    /* ADF info. */
    DMP_MISC	*dm1u_xmap;	    /* memory allocated for map of referenced pages */
    DMP_MISC	*dm1u_xchain;	    /* memory allocated for map of pgs referenced
				    ** in this chain */
    i4	dm1u_flags;	    /* Instuction Flags: */
#define		    DM1U_VERBOSE    0x1L
				    /*    set   -> output informative messages
				    **    unset -> suppress informative messages
				    */
};

/*
**  External function references.
*/

FUNC_EXTERN DB_STATUS dm1u_verify(
			DMP_RCB         *rcb,
			DML_XCB         *xcb,
			i4         type,
			DB_ERROR        *dberr );


FUNC_EXTERN VOID dm1u_talk(
			DM1U_CB		*dm1u,
			i4		msg_id,
			i4		count,
			... );

/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef ADE_H_INCLUDED
#define ADE_H_INCLUDED

/**
** Name: ADE.H - External structures and constants needed for ADE.
**
** Description:
**      This file holds typedefs and constant definitions needed to use ADF's
**      Compiled Expression Module, ADE.  Throughout the file you will see
**      "compiled expression" abbreviated as "CX".  This file depends on
**	<compat.h> and <dbms.h> already being #included.
**
** History:
**      17-jun-86 (thurston)
**          Initial creation.
**	16-jul-86 (thurston)
**	    Added the ADE_EXCB typedef and the ADE_TRUE and ADE_NOT_TRUE
**	    constants.
**	25-jul-86 (thurston)
**	    Removed the .excb_adf_scb member from the ADE_EXCB typedef.
**	25-jul-86 (thurston)
**	    Added FUNC_EXTERNs for all external interface routines.
**	11-sep-86 (thurston)
**	    Added the .excb_cmp field to the ADE_EXCB structure, and a #define
**	    for the ADE_COMPARE instruction.
**	28-dec-86 (thurston)
**	    Added the ADE_LEN_UNKNOWN constant.
**	29-jan-87 (thurston)
**	    Added the .excb_nullskip field to the ADE_EXCB structure.
**	23-feb-87 (thurston)
**	    Added FUNC_EXTERN for new ade_cxinfo() routine, and its associated
**	    request codes.
**	12-mar-87 (thurston)
**	    Added the ADE_NAGEND CX special instruction, as well as the new
**	    group of internal CX special instructions to handle nullable
**	    datatypes.
**	16-mar-87 (thurston)
**	    Added three CX special instructions to deal with QUEL PM semantics:
**	    ADE_PMQUEL, ADE_NO_PMQUEL, and ADE_PMFLIPFLOP.
**	17-mar-87 (thurston)
**	    Added the CX special instruction, ADE_3CXI_CMP_SET_SKIP.
**	25-mar-87 (thurston)
**	    Added some new request codes for ade_cxinfo().
**	29-mar-87 (thurston)
**	    Added the CX special instruction, ADE_PMENCODE.
**	1-apr-87 (agh)
**	    Added typedef for AFC_VERALN structure.
**	13-may-87 (thurston)
**	    Added request codes ADE_14REQ_LAST_INSTR_OFFSETS and
**	    ADE_15REQ_LAST_INSTR_ADDRS for ade_cxinfo().
**	10-jun-87 (thurston)
**	    Added ADE_NULLBASE.
**	16-sep-87 (thurston)
**	    Converted the PM flags into RANGEKEY flags.
**	26-oct-87 (thurston)
**	    Added the ADE_BYCOMPARE instruction which differs from the
**	    ADE_COMPARE instruction only in that NULL values will compare as
**	    equal to one another.  This is to allow `BY' clauses of aggregates
**	    to function properly.  ADE_COMPARE must compare NULL values as never
**	    being equal to one another for keyed joins.
**	10-feb-88 (thurston)
**	    Added ADE_SVIRGIN.
**	09-jun-88 (thurston)
**	    Added the ADE_ESC_CHAR instruction.  This will be used for the LIKE
**	    and NOT LIKE operators when an ESCAPE clause is in use.  The code
**	    that compiles a CX with one of these operators will first compile
**	    the ADE_ESC_CHAR instruction with one operand (must be a character
**	    string of length 1, any INGRES string type will do), and then go
**	    ahead and compile the LIKE or NOT LIKE operator as usual.  After the
**	    LIKE or NOT LIKE instruction is processed, the escape character will
**	    be turned back off.
**	07-oct-88 (thurston)
**	    Added the following constants:  ADE_1LT2, ADE_1EQ2, ADE_1GT2, and
**	    ADE_1UNK2.
**	22-nov-88 (eric)
**	    Added ADE_SMAX because OPC has an array of info for each segment
**	    of a cx.
**	19-feb-89 (paul)
**	    Added special operators ADE_PNAME and ADE_DBDV in support of the
**	    CALLPROC operation for rules.
**	    ADE_PNAME computes the address of the second parameter and places
**	    it at the location described by the first parameter. Followed by
**	    the length of the second parameter. AN assumption is made that
**	    the first parameter address represents the beginning of a
**	    QEF_USR_PARAM structure.
**	    ADE_DBDV constructs a DB_DATA_VALUE for the second parameter at
**	    the location described by the first parameter.
**      27-mar-89 (jrb)
**	    Added opr_prec and opr_1rsvd to ADE_OPERAND for decimal project.
**	23-may-89 (jrb)
**	    Added ADE_1ISNULL, ADE_2ISNULL, and ADE_BOTHNULL values to those
**	    possible as return values from an ade_compare instruction.  Removed
**	    ADE_1UNK2 since no instruction returns it any longer.
**	28-nov-89 (fred)
**	    Added support for peripheral datatypes.
**      29-dec-1992 (stevet)
**          Added function prototype.
**	13-jul-1995 (ramra01)
**	    Aggf optimization for count(*), new function to examine 
**	    cx instructions. Potential for future aggf optimization.
**	3-jul-96 (inkdo01)
**	    Added ADE_4CXI_CMP_SET_SKIP for predicate function support.
**	21-mar-97 (inkdo01)
**	    Added ADE_ANDL, ADE_ORL as part of dropping CNF requirement for
**	    Boolean expressions. Also added ADE_LABBASE for identifying label
**	    operands.
**	8-sep-99 (inkdo01)
**	    Added ADE_BRANCH &ADE_BRANCHF to do (un)conditional branching 
**	    within an ADF expression (for the case function).
**      22-Nov-1999 (hweho01)
**          Changed opr_offset from int to long for avoiding the truncation
**          of address, since pointer is 8-byte for 64-bit platforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-nov-00 (inkdo01)
**	    Added ADE_MECOPYN to MECOPY and do null test, and ADE_5CXI_CLR_SKIP.
**	15-dec-00 (inkdo01)
**	    Added ADE_AVG to compute final average for new inline aggregates.
**	3-jan-01 (inkdo01)
**	    Added ADE_NULLCHECK to verify null status of agg result.
**	9-jan-01 (inkdo01)
**	    Added ADE_OLAP to compute final result of OLAP agg funcs and 
**	    ADE_MECOPYQA, ADE_AVGQ for Quel aggregate processing to check 
**	    null first.
**	9-apr-01 (inkdo01)
**	    Added ADE_UNORM to normalize raw Unicode values (from INSERT, UPDATE)
**	30-aug-01 (inkdo01)
**	    Restore AGBGN, NAGEND and add AGNEXT for FRS (still uses old agg 
**	    techniques) and add OLAGBGN and OLAGEND for binary aggs. 
**	02-aug-2003 (somsa01)
**	    Updated opr_offset to be of type OFFSET_TYPE.
**	03-sep-2003 (abbjo03)
**	    Correct prototypes of ade_lab_resolve and afc_countstar_loc.
**	14-nov-03 (inkdo01)
**	    Added ADE_IBASE as preallocated DSH row buffer for shared
**	    intermediate results buffer.
**	15-dec-03 (inkdo01)
**	    Added ADE_QENADF to store QEN_ADF addr for thread init in || q's.
**	24-aug-04 (inkdo01)
**	    Added ade_global_bases().
**	5-nov-04 (inkdo01)
**	    Added ADE_NOOP.
**	28-Mar-2005 (schka24)
**	    Add ADE_6CXI_CLR_OP0NULL.
**	25-Jul-2005 (schka24)
**	    Add ADE_SETTRUE.
**	26-Jun-2008 (kiria01) SIR120473
**	    Slight change to meaning of ADE_ESC_CHAR
**	12-Oct-2008 (kiria01) b121011
**	    Added ADE_SETFALSE.
**	22-Jan-2009 (kibro01) b120461
**	    Add ADE_MECOPY_MANY to turn off attempt to optimise adjacent
**	    MECOPY instructions if many will be produced in one go.
**      27-Mar-2009 (hanal04) Bug 121857
**          Correct FUNC_EXTERN for ade_global_bases().
**	24-Aug-2009 (kschendel) 121804
**	    Allow multiple inclusions by e.g. the front-end rats nest.
**	01-Nov-2009 (kiria01) b122822
**	    Added support for ADE_COMPAREN & ADE_NCOMPAREN instructions
**	    for big IN lists.
**	18-Mar-2010 (kiria01) b123438
**	    Added ADE_SINGLETON as FINIT support for inline SINGLETON
**	    aggregate for scalar sub-query support.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@global_variable_references@]
*/


/*
**  Defines of other constants.
*/

/*: CX Reserved Bases
**      These are the symbolic constants to use for the
**      CX reserved bases:
*/

#define			ADE_NOBASE	(-1)/* Refers to no base at all */

#define                 ADE_CXBASE      0   /* base[ADE_CXBASE] must be set   */
                                            /* up to point to the CX itself.  */

#define                 ADE_VLTBASE     1   /* base[ADE_VLTBASE] must be set  */
                                            /* up to point to enough scratch  */
                                            /* space for the VLTs.  This can  */
                                            /* change each time the VLUPs do. */

#define                 ADE_DVBASE      2   /* For the Front Ends and PC only.*/
                                            /* base[ADE_DVBASE] must be set   */
                                            /* up to point to the array of    */
                                            /* DB_DATA_VALUEs used by the FE  */
                                            /* and PC groups.                 */

#define                 ADE_NULLBASE    3   /* For DBMS only.                 */
                                            /* base[ADE_NULLBASE] must be set */
                                            /* up to be NULL.  This will be   */
                                            /* used by OPC as a mechanism to  */
                                            /* avoid having ADE_KEYBLD fill   */
                                            /* in the high or the low key.    */

#define			ADE_LABBASE	4   /* base[ADE_LABBASE] is same as   */
					    /* ADE_CXBASE. It's base for label*/
					    /* operands and is diff from      */
					    /* CXBASE to allow adecompile to  */
					    /* identify & resolve 'em without */
					    /* confusing them with constants. */

#define			ADE_QENADF	5   /* For backend only.              */
					    /* Holds addr of corresponding    */
					    /* QEN_ADF structure. Used for    */
					    /* thread initialization in       */
					    /* parallel queries. Not the best */
					    /* place to stuff it.             */

#define			ADE_GLOBALBASE	6   /* For backend only.              */
					    /* Holds addr of the global base  */
					    /* array, if it is being used.    */
					    /* Otherwise, it is NULL.         */

#define			ADE_IBASE	7   /* this is the intermediate	      */
					    /* results buffer shared by all   */
					    /* CXs in an executed plan. It    */
					    /* corresponds to a DSH row       */
					    /* buffer (buffer 7). Pre-alloc'd */					    /* DSH buffs are new with         */
					    /* parallel query changes         */

#define                 ADE_ZBASE       8   /* base[ADE_ZBASE] is the first   */
                                            /* non reserved base.  This is    */
                                            /* where the ADE callers should   */
                                            /* start to allocate bases from.  */


/*: CX Special Instructions
**      These are the CX special instruction codes that can be compiled
**      into a CX in addition to all of the ADF function instance IDs:
*/

#define                 ADE_AND         (-1001)	/* logical AND                */
#define                 ADE_OR          (-1002)	/* logical OR                 */
#define                 ADE_NOT         (-1003)	/* logical NOT                */
#define                 ADE_MECOPY      (-1004)	/* move bytes of data         */
#define                 ADE_AGBGN       (-1005)	/* ag-begin                   */
#define                 ADE_AGEND       (-1006)	/* ag-end w/def for empty set */
#define                 ADE_KEYBLD      (-1007)	/* build key & do rng-key chk */
#define                 ADE_CALCLEN     (-1008)	/* calculate length for VLT   */
#define                 ADE_COMPARE     (-1009)	/* cmp 2 values (NULL != NULL)*/
#define                 ADE_NAGEND      (-1010)	/* ag-end w/NULL for empty set*/
#define                 ADE_PMQUEL      (-1011)	/* turns on QUEL PM semantics */
#define                 ADE_NO_PMQUEL   (-1012)	/* turns off QUEL PM semantics*/
#define                 ADE_PMFLIPFLOP  (-1013)	/* toggles QUEL PM semantics  */
#define                 ADE_PMENCODE    (-1014)	/* encodes QUEL PM chars      */
#define                 ADE_BYCOMPARE   (-1015)	/* cmp 2 values (NULL == NULL)*/
#define                 ADE_ESC_CHAR    (-1016)	/* set esc characteristics for LIKE      */
#define			ADE_PNAME	(-1017)	/* compute physical address */
#define			ADE_DBDV	(-1018)	/* build DB_DATA_VALUE for  */
#define			ADE_TMCVT	(-1019) /* CX version of adc_tmcvt */
#define			ADE_REDEEM	(-1020) /* Like MEcopy() but to
						** materialize peripheral
						** datatypes.
						*/
#define                 ADE_ANDL        (-1021)	/* logical AND with label.    */
#define                 ADE_ORL         (-1022)	/* logical OR with label.     */
						/* Labelled AND/OR are used in
						** non-conjunctive normal form
						** Boolean expressions (i.e.,
						** AND's can be nested in OR's).
						** Labels are for true/false
						** paths through expression */
#define			ADE_BRANCH	(-1023) /* simple branch (for case function) */
#define			ADE_BRANCHF	(-1024) /* conditional (on FALSE) branch */
#define			ADE_MECOPYN	(-1025) /* MEcopy which does null check */
#define			ADE_AVG		(-1026) /* FINIT avg computation for
						** inline aggregates */
#define			ADE_NULLCHECK	(-1027)	/* Checks for null agg result
						** assigned to non-nullable field */
#define			ADE_OLAP	(-1028)	/* FINIT computation for OLAP
						** aggregates */
#define			ADE_MECOPYQA	(-1029)	/* FINIT MECOPY of Quel agg results
						** uses 2CXI_SKIP to check nulls */
#define			ADE_AVGQ	(-1030)	/* FINIT avg computation for 
						** Quel (doesn't set null) */
#define			ADE_UNORM	(-1031) /* normalization of Unicode 
						** source values */
#define			ADE_AGNEXT	(-1032) /* detail agg exec for FRS only */
#define			ADE_OLAGBGN	(-1033)	/* AGBGN for binary OLAP aggs */
#define			ADE_OLAGEND	(-1034) /* AGEND for binary OLAP aggs */
#define			ADE_NOOP	(-1035) /* NOOP to replace collapsed
						** MECOPY's (or for whatever
						** other purpose they suit) */
#define			ADE_SETTRUE	(-1036)	/* Reset CX result to TRUE */
#define			ADE_SETFALSE	(-1037)	/* Reset CX result to FALSE */
#define			ADE_MECOPY_MANY (-1038)	/* MECOPY, but not the last */
#define			ADE_COMPAREN	(-1039)	/* Bulk compare operator - true vararg */
#define			ADE_NCOMPAREN	(-1040)	/* Bulk not compare operator - true vararg */
#define			ADE_SINGLETON	(-1041) /* FINIT computation for
						** singleton inline aggregate */



/*: Internal ADF CX Special Instructions
**      These are the CX special instruction codes that ADE's instruction
**      generator may automatically compile prior to any instruction whose
**      input operands contain nullable datatypes:
*/

#define             ADE_0CXI_IGNORE	    (-2000)
	/* This will never be compiled, but exists as a value
	** to place in the function instance table for those
	** function instances that want to handle the null values
	** themselves; such as the ifnull() function.
	*/

#define             ADE_1CXI_SET_SKIP	    (-2001)
	/* Test for a null in any input operand.  If found, set
	** result to null, and skip the next instruction.  If no
	** nulls are found, clear the null value bit in the result,
	** and proceed to execute the next instruction as though
	** all input and output datatypes were non-nullable.  This
	** is the most common of this set.  Most function instances
	** will use this when one of their inputs is nullable.
	** Only generated when the result and at least one input are
	** nullable, otherwise see 6CXI and 7CXI below.
	*/

#define             ADE_3CXI_CMP_SET_SKIP   (-2003)
	/* If not in a CX, (i.e. adf_func()) this instruction
        ** functions exactly like ADE_1CXI_SET_SKIP.  If in a CX,
        ** however, (since comparison function instances do not have
        ** result operands in a CX) it works like this:  Test for a
        ** null in any input operand.  If found, set the value of the
        ** CX to `UNKNOWN', and skip the next instruction. If no
        ** nulls are found, just proceed to execute the next
        ** instruction as though all input and output datatypes were
        ** non-nullable.  This will be used by most comparison
        ** function instances when one of their inputs is nullable. 
	*/

#define             ADE_2CXI_SKIP	    (-2002)
	/* Test for a null in any input operand.  If found,
	** skip the next instruction.  This will be used by
	** most aggregate function instances.
	*/

#define		    ADE_4CXI_CMP_SET_SKIP   (-2004)
	/* Combination of 1CXI and 3CXI which both sets the result 
	** operand null indicator, and assigns UNKNOWN to the CX
	** value, if a null operand is detected. Finally, the 
	** following instruction is skipped if any operand is null.
	** This was implemented for ADI_PRED_FUNC operations - 
	** predicate functions which can be used either in infix
	** notation or function notation. 
	** Only generated when the result and at least one input are
	** nullable, otherwise see 6CXI and 7CXI below.
	*/

#define		    ADE_5CXI_CLR_SKIP       (-2005)
	/* A 1CXI which doesn't set the null indicator, only clears it.
	** If source operand is null, this just skips next instruction. 
	** If source operand is not null, it clears the null indidcator 
	** of the result operand and executes next instruction (used
	** in aggregate processing).
	** Only generated when the result and at least one input are
	** nullable, otherwise see 6CXI and 7CXI below.
	*/

#define		    ADE_6CXI_CLR_OP0NULL	(-2006)
	/* When 1CXI, 4CXI, or 5CXI is generated for a nullable result
	** operand but no nullable input operands, all we need to do is
	** to clear the result's null indicator.  So we generate this
	** instruction instead.
	*/

#define		    ADE_7CXI_CKONLY		(-2007)
	/* When a 1CXI, 4CXI, or 5CXI is generated for a non-nullable
	** result operand that has nullable inputs, all we need to do is
	** to check the inputs (without fooling around with the result).
	** So, we generate this check-only instruction instead.
	*/


/*: Range-type Key Options for ADE_KEYBLD
**      These are the range-type key options that can be compiled
**      into the ADE_KEYBLD instruction to test for the existence
**	or non-existence of range-type keys caused by the input:
*/

#define		ADE_1RANGE_DONTCARE  1  /* Does not matter; don't check */
#define         ADE_2RANGE_YES       2  /* Input must have range-type key */
#define		ADE_3RANGE_NO        3  /* Input must not have range-type key */


/*: Comparison Relationship Constants for ADE_COMPARE and ADE_BYCOMPARE
**      These are the constants that get placed in the .excb_cmp field of
**	the execution control block passed to ade_execute_cx() by the two
**	comparison instructions, ADE_COMPARE and ADE_BYCOMPARE.
*/

#define		ADE_1LT2           (-1) /* 1st value was <  2nd value */
#define         ADE_1EQ2             0  /* 1st value was == 2nd value */
#define		ADE_1GT2             1  /* 1st value was >  2nd value */
/* These values are returned only when executing the ade_compare instruction */
#define		ADE_1ISNULL	     3  /* 1st value was NULL */
#define		ADE_2ISNULL	     4  /* 2nd value was NULL */
#define		ADE_BOTHNULL	     5  /* both 1st and 2nd values were NULL */
#define		ADE_NOTIN	     6  /* 1st valu not in 2nd..nth value */


/*: CX Segments
**      These are the symbolic constants to use to refer to the
**      four segments of a CX when compiling and executing.  Note
**	that the VIRGIN segment is reserved for ADE_CALCLEN
**	instructions ONLY.
**
**	ADE_SMAX should be the largest number of the ADE_S* series of
**	constants
*/

#define                 ADE_SVIRGIN	4   /* The VIRGIN segment */
#define                 ADE_SINIT	1   /* The INIT   segment */
#define                 ADE_SMAIN       2   /* The MAIN   segment */
#define                 ADE_SFINIT      3   /* The FINIT  segment */
#define			ADE_SMAX	4   /* The largest ADE_S* constant */


/*: CX Request Codes
**      These are the symbolic constants to use to request various pieces
**      of information about a given CX via the ade_cxinfo() routine:
*/

#define     ADE_01REQ_BYTES_ALLOCATED   ((i4) 1)   /* Mem alloc'ed to CX     */
						    /* Returns:  i4      */

#define     ADE_02REQ_BYTES_USED        ((i4) 2)   /* Mem used by CX         */
						    /* Returns:  i4      */

#define     ADE_03REQ_NUM_INSTRS        ((i4) 3)   /* # instrs in each seg   */
						    /* Returns:  i4[3]   */
						    /*   where:     [0]=INIT  */
						    /*              [1]=MAIN  */
						    /*              [2]=FINIT */

#define     ADE_04REQ_NUM_CONSTANTS     ((i4) 4)   /* # constants            */
						    /* Returns:  i4      */

#define     ADE_05REQ_NUM_VLTS          ((i4) 5)   /* # variable length tmps */
						    /* Returns:  i4           */

#define     ADE_06REQ_HIGH_BASE         ((i4) 6)   /* Highest #'ed base used */
						    /* Returns:  i4           */

#define     ADE_07REQ_ALIGNMENT_IN_USE  ((i4) 7)   /* Alignment used by CX   */
						    /* Returns:  i4           */

#define     ADE_08REQ_VERSION	        ((i4) 8)   /* Version # of CX.	      */
						    /* Returns:  i2[2]        */

#define     ADE_09REQ_SIZE_OF_CXHEAD    ((i4) 9)   /* # bytes in a CX header.*/
						    /* Returns:  i4           */
    /*	*** NOTE: ptr to CX can be NULL ***				      */

#define     ADE_10REQ_1ST_INSTR_OFFSETS ((i4)10)   /* Offsets to 1st instrs. */
						    /* Returns:  i4[3]   */
						    /*   where:     [0]=INIT  */
						    /*              [1]=MAIN  */
						    /*              [2]=FINIT */

#define     ADE_11REQ_1ST_INSTR_ADDRS   ((i4)11)   /* Addrs of instr lists.  */
						    /* Returns:  PTR[3]       */
    /*	*** FRONTENDS TAKE NOTE!!! ***		         where:     [0]=INIT  */
    /*	    This request will only be valid as long as              [1]=MAIN  */
    /*	    the CX header is immediately followed by                [2]=FINIT */
    /*	    the instructions/constants. (i.e. They are			      */
    /*	    contiguous.)						      */

#define     ADE_12REQ_FE_INSTR_LIST_I2S ((i4)12)   /* # i2s in FE instr lists*/
						    /* Returns:  nat[3]       */
    /*	*** FRONTENDS TAKE NOTE!!! ***		         where:     [0]=INIT  */
    /*	    This request will only be valid as long as              [1]=MAIN  */
    /*	    the CX header is immediately followed by                [2]=FINIT */
    /*	    the instructions/constants. (i.e. They are			      */
    /*	    contiguous.)						      */
    /*	*** FURTHER ASSUMPTION!!! ***					      */
    /*	    This will only be used by FE's, and all			      */
    /*	    instructions in each list (i.e. segment)			      */
    /*	    are contiguous.						      */

#define     ADE_13REQ_ADE_VERSION        ((i4)13)  /* Version # of ADE       */
						    /* Returns:  i2[2]        */
    /*	*** NOTE: ptr to CX can be NULL ***				      */

#define     ADE_14REQ_LAST_INSTR_OFFSETS ((i4)14)  /* Offsets to last instrs.*/
						    /* Returns:  i4[3]   */
						    /*   where:     [0]=INIT  */
						    /*              [1]=MAIN  */
						    /*              [2]=FINIT */

#define     ADE_15REQ_LAST_INSTR_ADDRS   ((i4)15)  /* Addrs end of ins lists.*/
						    /* Returns:  PTR[3]       */
    /*	*** FRONTENDS TAKE NOTE!!! ***		         where:     [0]=INIT  */
    /*	    This request will only be valid as long as              [1]=MAIN  */
    /*	    the CX header is immediately followed by                [2]=FINIT */
    /*	    the instructions/constants. (i.e. They are			      */
    /*	    contiguous.)						      */


/*: Unknown Length Constant for VLTs and VLUPs
**      This constant is to be used when compiling an instruction either VLTs
**      or VLUPs as operands where the length is unknown at compile time:
*/

#define                 ADE_LEN_UNKNOWN -1  /* Unknown length for VLT or VLUP */
#define			ADE_LEN_LONG	(-2)	/*
						**  Longer than tuple size,
						**  exact size unknown.  These
						**  are used for peripheral
						**  datatypes.
						*/


/*}
** Name: ADE_OPERAND - A CX instruction's operand.
**
** Description:
**      This describes an operand for an instruction in a CX.  An array of these
**	will be passed into ade_instr_gen() for each instruction to be compiled
**	into the CX.  There will be one element in this array for each operand
**	required by the instruction being compiled.
**
** History:
**     17-jun-86 (thurston)
**          Initial creation.
**     27-mar-89 (jrb)
**	    Added opr_prec and opr_1rsvd for decimal project.
**	10-sep-99 (inkdo01)
**	    Added OPR_NO_OFFSET #define for case functions.
**	02-aug-2003 (somsa01)
**	    Updated opr_offset to be of type OFFSET_TYPE.
**	9-dec-04 (inkdo01)
**	    Replaced opr_1rsvd by opr_collID (for character operands).
**	10-Feb-2005 (schka24)
**	    Put offset back to i4; we currently don't support having anything
**	    that a CX might point to being larger than 2 Gb.  (Even 32K
** 	    would probably suffice.)  Note that it's an offset, not a ptr.
**	23-Jun-2005 (schka24)
**	    Arrgh.  Put offset back, this time with a proper explanation of
**	    why it has to be larger than just i4.
*/

/* Notice that opr_offset is "ssize_t", i.e. "signed size of something".
** (Another option would be ptrdiff_t.)
** It's changed back and forth.  I (schka24) most recently changed it
** back to i4, thinking that the operand is at offset opr_offset within
** base opr_base.  And, since we don't generate CX areas larger than 2 Gb,
** an i4 would do.
**
** Unfortunately, that's backend-think, although correct in that context.
** For front-end CX's, the CX actually contains indexes into an array
** of DB_DATA_VALUE's for its operands instead of ADE_OPERAND structs.
** In order to share code, afc_execute_cx wants to generate an array
** of ADE_OPERAND's from the DB_DATA_VALUE's.  It does this by taking
** each DB_DATA_VALUE and generating a completely bogus offset, 
** computing (in effect) dv[n]->db_data - &dv[0].
** *This is completely illegal* according to strict C, but it does it
** anyway.  Since the DB_DATA_VALUE array may be somewhere totally
** unrelated to the db_data value, arbitrary offsets can be generated.
** (Indeed, the DB_DATA_VALUE array is usually on the stack, and the
** data value might be in static memory somewhere.  That's why the
** pointer subtraction is illegal -- theoretically you can only subtract
** pointers that point within the same "object".)
** Eventually, the operand offset is added back to the "base" (ie &dv[0])
** to recalculate the data value address, which we had in the first place.
**
** So, we need opr_offset to be something that can hold an arbitrary,
** signed, offset so that front-end CX's on 64-bit machines work properly.
**
** As a side note, a better fix would probably be to make opr_offset a
** union of a pointer (for FE's) and an offset (for the BE).  It wouldn't
** be any smaller, but it would be more correct and portable C code.
*/

typedef struct
{
    i4              opr_len;            /* Length of operand's data.  A length
                                        ** of zero will tell the CX evaluator
                                        ** that the length is in a two byte
                                        ** count field at the front of the data
                                        ** pointed to by the base/offset below.
                                        ** This two byte count field will not
                                        ** include itself in that length. 
                                        */
    DB_DT_ID        opr_dt;             /* Datatype of operand.
                                        */
    i2              opr_base;           /* The base number ...
                                        */
    SSIZE_TYPE	    opr_offset;		/* ... and the offset from that base to
                                        ** use in order to calculate the actual
                                        ** memory address of the operand's data.
                                        */
#define		OPR_NO_OFFSET	-1	/* indicates base/offset not set yet */
    i2		    opr_prec;		/* Precision and scale for DECIMAL
					** values.
					*/
    i2		    opr_collID;		/* collation ID (for character operands)
					*/
}    ADE_OPERAND;


/*}
** Name: ADE_EXCB - CX execution control block.
**
** Description:
**      This structure is passed into the ade_execute_cx() routine.  It carries
**      all of the information required to execute a CX, and supplies a place
**      for returning any information from the routine.
**
** History:
**      16-jul-86 (thurston)
**          Initial creation.
**	25-jul-86 (thurston)
**	    Removed the .excb_adf_scb member.
**	11-sep-86 (thurston)
**	    Added the .excb_cmp field to this structure.
**	29-jan-87 (thurston)
**	    Added the .excb_nullskip field to this structure.
**	28-nov-89 (fred)
**	    Added the excb_continuation and excb_size fields for large datatype
**	    support.
**	06-nov-03 (hayke02)
**	    Added ADE_UNKNOWN for completeness - ADE_NOT_TRUE does not represent
**	    a cx_value of UNKNOWN.
**	18-Jan-2006 (kschendel)
**	    Added struct tag for pointer forward references.
*/

typedef struct _ADE_EXCB
{
    PTR             excb_cx;            /* Pointer to the CX.
                                        */
    i4              excb_seg;           /* Segment of the CX to execute.  This
                                        ** must be one of the symbolic constants
                                        ** defined above for this purpose.
                                        */
    i4              excb_nbases;        /* # of bases supplied in the base
                                        ** address array, ade_bases[].
                                        */
    i4              excb_cmp;           /* If one or more ADE_COMPARE or
					** ADE_BYCOMPARE instructions were 
					** executed, this will be set according
					** to the last one of these to be
					** executed as follows:
					**	ADE_1LT2:  Compare showed "<"
					**	ADE_1EQ2:  Compare showed "="
					**	ADE_1GT2:  Compare showed ">"
					** and for only ADE_COMPARE three more:
					**	ADE_1ISNULL:  1st value was NULL
					**	ADE_2ISNULL:  2nd value was NULL
					**	ADE_BOTHNULL: Both values were NULL
					** For ADE_COMPARE with NULL values, one
					** of the last three values will be set;
					** without NULLs one of the first three
					** will be returned.  However, for
					** ADE_BYCOMPARE, a NULL value will
					** compare equal to another NULL value
					** (giving ADE_1EQ2) and a NULL value is
					** greater than any non-NULL value.  If
					** no ADE_COMPARE or ADE_BYCOMPARE
					** instruction was executed, this field
					** will be undefined.
                                        */
    i4              excb_nullskip;      /* At the begining of execution of any
                                        ** segment of a CX, this field will be
                                        ** set to zero.  During execution, if
                                        ** one or more NULL values are skipped
                                        ** by any aggregate operation, then this
                                        ** field will be set to 1.
                                        */
    i4              excb_value;         /* The value of the CX will be returned
                                        ** here.  This will be one of:
                                        */

#define                 ADE_TRUE	1   /* CX was TRUE */
#define                 ADE_NOT_TRUE	0   /* CX was FALSE */
#define                 ADE_UNKNOWN	-1  /* CX was UNKNOWN */

    i4		    excb_limited_base;
					/*
					**  The base id whose size is
					**  limited.  Set to ADE_NOBASE if
					**  no base is limited (i.e. the
					**  caller "is sure" that all bases are
					**  large enough to handle the output
					**  requested.  (Normally, all bases
					**  are large enough.  However, with
					**  large datatype support, "large
					**  enough" is undefined.  ADE
					**  (implicitly, by having only a
					**  single limited base, currently
					**  restricts a single base with
					**  receiving large datatype output.)
					*/
    i4	    excb_continuation;	/*
					** Instruction number from which to
					** resume when processing peripheral,
					** multipass function instances.
					*/
    i4	    excb_size;		/*
					** On input, this is the size of the
					** excb_constrained_base mentioned
					** above.  On output, this is the amount
					** of that base actually used.
					*/

    PTR             excb_bases[1];      /* Array of base addresses to be used
                                        ** when calculating actual memory
                                        ** locations from the base/offsets
                                        ** in the CX's operands.  This is
                                        ** actually a variable length array.
                                        ** The 1 is only here so as to be able
                                        ** to reference this member as an array.
                                        */
}   ADE_EXCB;


/*}
** Name: AFC_VERALN - Contains ADF's version and alignment information
**
** Description:
**      This structure is passed into the afc_cxhead_init() routine.
**      It carries information about a CX's version and alignment.
**      This structure also appears as part of FRMALIGN structure
**      used by the ABF facility.
**
** History:
**      1-apr-87 (agh)
**          Initial creation.
*/

typedef struct
{
    i2    afc_hi_version;    /* Major version of CX.
                             */
    i2    afc_lo_version;    /* Minor version of CX.
                             */
    i2    afc_max_align;     /* Alignment used in building CX.
                             */
}   AFC_VERALN;


/*
**  Forward and/or External function references.
*/
# ifdef ADF_BUILD_WITH_PROTOS
FUNC_EXTERN DB_STATUS ade_cx_space(ADF_CB   *adf_scb,
				   i4       ade_ni,
				   i4       ade_nop_tot,
				   i4       ade_nk,
				   i4  ade_ksz_tot,
				   i4  *ade_est_cxsize);
FUNC_EXTERN DB_STATUS ade_bgn_comp(ADF_CB   *adf_scb,
				   PTR      ade_cx,
				   i4  ade_cxsize);
FUNC_EXTERN DB_STATUS ade_end_comp(ADF_CB   *adf_scb,
				   PTR      ade_cx,
				   i4  *ade_cxrsize);
FUNC_EXTERN DB_STATUS ade_const_gen(ADF_CB         *adf_scb,
				    PTR            ade_cx,
				    DB_DATA_VALUE  *ade_dv,
				    ADE_OPERAND    *ade_op);
FUNC_EXTERN VOID ade_global_bases(ADF_CB   *adf_scb,
				   PTR      ade_cx,
				   i4		*basemap);
FUNC_EXTERN DB_STATUS ade_instr_gen(ADF_CB       *adf_scb,
				    PTR          ade_cx,
				    i4           ade_icode,
				    i4           ade_seg,
				    i4           ade_nops,
				    ADE_OPERAND  ade_ops[],
				    i4           *ade_qconst_map,
				    i4           *ade_unaligned);
FUNC_EXTERN void      ade_lab_resolve(ADF_CB     *adf_scb,
				    PTR          ade_cx,
				    ADE_OPERAND	 *ade_labop);
FUNC_EXTERN VOID      ade_verify_labs(ADF_CB     *adf_scb,
				    PTR          ade_cx);
FUNC_EXTERN DB_STATUS ade_cxinfo(ADF_CB  *adf_scb,
				 PTR     ade_cx,
				 i4      ade_request,
				 PTR     ade_result);
FUNC_EXTERN DB_STATUS ade_inform_space(ADF_CB   *adf_scb,
				       PTR      ade_cx,
				       i4  ade_new_cxsize);

FUNC_EXTERN DB_STATUS ade_vlt_space(ADF_CB  *adf_scb,
				    PTR     ade_cx,
				    i4      ade_nbases,
				    PTR     ade_bases[],
				    i4      *ade_needed);
FUNC_EXTERN DB_STATUS ade_execute_cx(ADF_CB    *adf_scb,
				     ADE_EXCB  *ade_excb);
FUNC_EXTERN DB_STATUS ade_countstar_loc(ADF_CB    *adf_scb,
				        ADE_EXCB  *ade_excb,
					PTR       *loc,
                                        i4        *instr_cnt);
FUNC_EXTERN VOID      ade_cx_print(PTR  cxbases[],
				   PTR  cx);
FUNC_EXTERN VOID      ade_cxbrief_print(PTR   cxbases[],
					PTR   cx,
					char  *cxname);

/*: CX Front-End Interface
**	These are the function interfaces for the FE version of ADE.
*/
FUNC_EXTERN DB_STATUS afc_cx_space(ADF_CB   *adf_scb,
				   i4       ade_ni,
				   i4       ade_nop_tot,
				   i4       ade_nk,
				   i4  ade_ksz_tot,
				   i4  *ade_est_cxsize);
FUNC_EXTERN DB_STATUS afc_bgn_comp(ADF_CB   *adf_scb,
				   PTR      ade_cx,
				   i4  ade_cxsize);
FUNC_EXTERN DB_STATUS afc_end_comp(ADF_CB   *adf_scb,
				   PTR      ade_cx,
				   i4  *ade_cxrsize);
FUNC_EXTERN DB_STATUS afc_instr_gen(ADF_CB       *adf_scb,
				    PTR          ade_cx,
				    i4           ade_icode,
				    i4           ade_seg,
				    i4           ade_nops,
				    ADE_OPERAND  ade_ops[],
				    i4           *ade_qconst_map,
				    i4           *ade_unaligned);
FUNC_EXTERN DB_STATUS afc_cxinfo(ADF_CB  *adf_scb,
				 PTR     ade_cx,
				 i4      ade_request,
				 PTR     ade_result);
FUNC_EXTERN DB_STATUS afc_inform_space(ADF_CB   *adf_scb,
				       PTR      ade_cx,
				       i4  ade_new_cxsize);
FUNC_EXTERN DB_STATUS afc_cxhead_init(ADF_CB      *adf_scb,
				      PTR         afc_cxhd,
				      AFC_VERALN  *afc_veraln,
				      i4          *offset_array);
FUNC_EXTERN DB_STATUS afc_execute_cx(ADF_CB    *adf_scb,
				     ADE_EXCB  *ade_excb);
FUNC_EXTERN DB_STATUS afc_countstar_loc(ADF_CB    *adf_scb,
					ADE_EXCB  *ade_excb, 
                                        PTR       *loc,
                                        i4        *instr_cnt);
FUNC_EXTERN VOID      afc_cx_print(PTR  cxbases[],
				   PTR  cx);
FUNC_EXTERN VOID      afc_cxbrief_print(PTR   cxbases[],
					PTR   cx,
					char  *cxname);
# else  /* ADF_BUILD_WITH_PROTOS */
FUNC_EXTERN DB_STATUS ade_cx_space();
FUNC_EXTERN DB_STATUS ade_bgn_comp();
FUNC_EXTERN DB_STATUS ade_end_comp();
FUNC_EXTERN DB_STATUS ade_const_gen();
FUNC_EXTERN DB_STATUS ade_instr_gen();
FUNC_EXTERN DB_STATUS ade_cxinfo();
FUNC_EXTERN DB_STATUS ade_inform_space();
FUNC_EXTERN DB_STATUS ade_vlt_space();
FUNC_EXTERN DB_STATUS ade_execute_cx();
FUNC_EXTERN DB_STATUS ade_countstar_loc();
FUNC_EXTERN VOID      ade_cx_print();
FUNC_EXTERN VOID      ade_cxbrief_print();
FUNC_EXTERN DB_STATUS afc_cx_space();
FUNC_EXTERN DB_STATUS afc_bgn_comp();
FUNC_EXTERN DB_STATUS afc_end_comp();
FUNC_EXTERN DB_STATUS afc_instr_gen();
FUNC_EXTERN DB_STATUS afc_cxinfo();
FUNC_EXTERN DB_STATUS afc_inform_space();
FUNC_EXTERN DB_STATUS afc_cxhead_init();
FUNC_EXTERN DB_STATUS afc_execute_cx();
FUNC_EXTERN DB_STATUS afc_countstar_loc();
FUNC_EXTERN VOID      afc_cx_print();
FUNC_EXTERN VOID      afc_cxbrief_print();
# endif /* ADF_BUILD_WITH_PROTOS */


#endif /* ADE_H_INCLUDED */

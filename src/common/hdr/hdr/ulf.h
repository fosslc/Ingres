/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: ULF.H - Utility library definitions.
**
** Description:
**      This file defines types, constants and function defined in the
**	utility library for the dbms.
**
** History: $Log-for RCS$
**      29-oct-1985 (derek)
**          Created new for 5.0.
**	22-Apr-1986 (fred)
**	    Added ULT section of file (looks like entire file in format)
**	    to define trace vectors and macros which operate upon them.
**	17-jun-86 (ericj)
**	    Added ULE_LOOKUP definition.
**	21-jun-87 (thurston)
**	    Added E_UL0014_BAD_ULM_PARM to avoid confusion with E_UL0003_BADPARM
**	    which is used as one of the ule_format() error code.  Also, had to
**	    add ULH error codes, since nobody bothered with CMS when they added
**	    them the first time.
**	17-jul-87 (thurston)
**	    Added E_UL0015_BAD_STREAM_MARK.
**	20-jul-87 (daved)
**	    Added E_UL0201_ULM_FAILED.
**	27-feb-88 (puree)
**	    Added E_UL0122_NAME_TOO_LONG for ULH.
**	11-apr-88 (seputis)
**	    Added query tree to text conversion routines interface/errors
**	24-jan-89 (jrb)
**	    Added forward type declares for some functions missing here.
**	11-jan-91 (stevet)
**	    Added E_UL0016_CHAR_INIT for read-in character set support.
**	29-aug-1992 (rog)
**	    Prototype ULF.
**	20-sep-1993 (andys)
**	    Add dummy prototype for ule_format call.
**	    This can be used to compile and check that ule_format is called
**	    with correct types for its first nine args.
**	13-oct-93 (swm)
**	    Bug #56448
**	    Altered uld_prnode calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to uld_prnode.
**	    This works because STprintf is a varargs function in the CL.
**	    Defined ULD_PRNODE_LINE_WIDTH for uld_prnode() callers which
**	    need to declare an STprintf buffer.
**	16-feb-94 (swm)
**	    Bug #59611
**	    Calls to STprintf required in previous change should use TRformat
**	    which is safer because it checks for buffer overrun. However,
**	    TRformat semantics make it necessary to null-terminate the
**	    formatted buffer in its callback and to preserve trailing newlines
**	    when requested. Added uld_tr_callback() to implement these
**	    operations.
**	29-jan-1994 (rog)
**	    Correct the ULBTM macro so that it doesn't silently overflow.
**	    (Bug 62552)
**	07-may-1997 (canor01)
**	    Add ULE_OPER flag to log messages to operating system log.
**	20-May-1998 (hanch04)
**	    Replace ule_format with a macro to print the file and line
**	    of the source code from the error.  ule_format is now ule_doformat
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (hweho01)
**          Listed the prototype of ule_doformat() as variable-arg function. 
**	21-jun-2001 (devjo01)
**	    Include tr.h to provide TRdisplay prototype to ule_format macro.
**      09-jul-2003 (stial01)
**          Added E_UL0017_DIAGNOSTIC
**	10-Sep-2008 (jonj)
**	    Sir 120874: Replace ule_doformat() with uleFormatFcn(), ule_format
**	    macro with uleFormat macro. ule_format macro rewritten to provide
**	    a transparent bridge while converting all macro uses to uleFormat
**	    form which includes DB_ERROR *dberror as its first call parameter.
**	24-Oct-2008 (jonj)
**	    VMS compiler does not like ule_doformat defined as ule_format macro.
**	    Deleted ule_doformat define and change sources to use ule_format
**	    instead.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	14-oct-2009 (maspa05) Sir 122725
**	    Add constants for use with SC930 tracing and move function defines
**	    here
**       2-Dec-2009 (hanal04) Bug 122137
**          Bump sc930 version number for format change.
**      15-feb-2010 (maspa05) Sir 123293
**          Bump sc930 version number for format change - added server_class
**           output.
**	25-May-2010 (kschendel)
**	    Add ult-trace-qep decl.
**/
#ifndef TR_HDR_INCLUDED
#include <tr.h>
#endif

/*
**  Forward and/or External typedef/struct references.
*/


/*
**  Defines of other constants.
*/

/*
** Maximum TRformat buffer size, for calls that use TRformat calls prior to
** uld_prnode() calls
*/
#define	ULD_PRNODE_LINE_WIDTH	1024

/*
**      Constants used in calls the ulf_e_format().
*/

/*}
** Name: ULF_LOG - type for calling ULE_FORMAT
**
** Description:
**      This type should be used to call ule_format for logging directive
**
** History:
**      11-apr-88 (seputis)
**	07-may-1997 (canor01)
**	    Add ULE_OPER flag to log messages to operating system log.
[@history_template@]...
*/
typedef i4 ULF_LOG;
#define			ULE_LOOKUP	(ULF_LOG)0L
#define                 ULE_LOG         (ULF_LOG)1L
#define			ULE_MESSAGE	(ULF_LOG)2L
#define			ULE_OPER	(ULF_LOG)3L

/*
**  Error Codes for ULF.
*/

#define			E_UL_MASK	0x000A0000L
#define                 E_UL0000_OK		    0L
#define			E_UL0001_BAD_SYSTEM_LOOKUP (E_UL_MASK + 0x0001L)
#define			E_UL0002_BAD_ERROR_LOOKUP  (E_UL_MASK + 0x0002L)
#define			E_UL0003_BADPARM	   (E_UL_MASK + 0x0003L)
#define			E_UL0004_CORRUPT	   (E_UL_MASK + 0x0004L)
#define			E_UL0005_NOMEM		   (E_UL_MASK + 0x0005L)
#define			E_UL0006_BADPTR		   (E_UL_MASK + 0x0006L)
#define			E_UL0007_CANTFIND	   (E_UL_MASK + 0x0007L)
#define			E_UL0008_MEM_SEMINIT	   (E_UL_MASK + 0x0008L)
#define		        E_UL0009_MEM_SEMWAIT	   (E_UL_MASK + 0x0009L)
#define		        E_UL000A_MEM_SEMRELEASE	   (E_UL_MASK + 0x000AL)
#define		        E_UL000B_NOT_ALLOCATED 	   (E_UL_MASK + 0x000BL)
#define			E_UL000C_BAD_ACK_MODE	   (E_UL_MASK + 0x000CL)
#define			E_UL000D_NEWBLOCK	   (E_UL_MASK + 0x000DL)
#define			E_UL000E_BAD_DO_BLOCK	   (E_UL_MASK + 0x000EL)
#define			E_UL000F_BAD_BLOCK_TYPE    (E_UL_MASK + 0x000FL)
#define			E_UL0010_NEED_MORE	   (E_UL_MASK + 0x0010L)
#define			E_UL0011_MORE_TO_SEND	   (E_UL_MASK + 0x0011L)
#define			E_UL0012_NETWORK_UNKNOWN   (E_UL_MASK + 0x0012L)
#define			E_UL0013_IN_BLOCK_FULL	   (E_UL_MASK + 0x0013L)
#define			E_UL0014_BAD_ULM_PARM	   (E_UL_MASK + 0x0014L)
#define			E_UL0015_BAD_STREAM_MARK   (E_UL_MASK + 0x0015L)
#define			E_UL0016_CHAR_INIT	   (E_UL_MASK + 0x0016L)
#define			E_UL0017_DIAGNOSTIC	   (E_UL_MASK + 0x0017L)
/*
**  Error codes for ULH
*/
#define			E_UL0101_TABLE_ID	   (E_UL_MASK + 0x0101L)
#define			E_UL0102_TABLE_SIZE	   (E_UL_MASK + 0x0102L)
#define			E_UL0103_NOT_EMPTY	   (E_UL_MASK + 0x0103L)
#define			E_UL0104_FULL_TABLE	   (E_UL_MASK + 0x0104L)
#define			E_UL0105_FULL_CLASS	   (E_UL_MASK + 0x0105L)
#define			E_UL0106_OBJECT_ID	   (E_UL_MASK + 0x0106L)
#define			E_UL0107_ACCESS		   (E_UL_MASK + 0x0107L)
#define			E_UL0108_DUPLICATE	   (E_UL_MASK + 0x0108L)
#define			E_UL0109_NFND		   (E_UL_MASK + 0x0109L)
#define			E_UL010A_MEMBER		   (E_UL_MASK + 0x010AL)
#define			E_UL010B_NOT_MEMBER	   (E_UL_MASK + 0x010BL)
#define			E_UL010C_UNKNOWN	   (E_UL_MASK + 0x010CL)
#define			E_UL010D_TABLE_MEM	   (E_UL_MASK + 0x010DL)
#define			E_UL010E_TCB_MEM	   (E_UL_MASK + 0x010EL)
#define			E_UL010F_BUCKET_MEM	   (E_UL_MASK + 0x010FL)
#define			E_UL0110_CHDR_MEM	   (E_UL_MASK + 0x0110L)
#define			E_UL0111_OBJ_MEM	   (E_UL_MASK + 0x0111L)
#define			E_UL0112_INIT_SEM	   (E_UL_MASK + 0x0112L)
#define			E_UL0113_TABLE_SEM	   (E_UL_MASK + 0x0113L)
#define			E_UL0114_BUCKET_SEM	   (E_UL_MASK + 0x0114L)
#define			E_UL0115_CLASS_SEM	   (E_UL_MASK + 0x0115L)
#define			E_UL0116_LRU_SEM	   (E_UL_MASK + 0x0116L)
#define			E_UL0117_OBJECT_SEM	   (E_UL_MASK + 0x0117L)
#define			E_UL0118_NUM_ALIASES	   (E_UL_MASK + 0x0118L)
#define			E_UL0119_ALIAS_MEM	   (E_UL_MASK + 0x0119L)
#define			E_UL0120_DUP_ALIAS	   (E_UL_MASK + 0x0120L)
#define			E_UL0121_NO_ALIAS	   (E_UL_MASK + 0x0121L)
#define			E_UL0122_ALIAS_REDEFINED   (E_UL_MASK + 0x0122L)
#define			E_UL0123_NAME_TOO_LONG	   (E_UL_MASK + 0x0123L) 

/*
**  Error codes for ULD
*/
#define			E_UL0201_ULM_FAILED	   (E_UL_MASK + 0x0201L)

/*
**  Error codes for query tree to text routines
*/
#define			E_UL0300_BAD_ADC_TMLEN	   (E_UL_MASK + 0x0300L)
#define			E_UL0301_BAD_ADC_TMCVT	   (E_UL_MASK + 0x0301L)
#define			E_UL0302_BUFFER_OVERFLOW   (E_UL_MASK + 0x0302L)
#define			E_UL0303_BQN_BAD_QTREE_NODE (E_UL_MASK + 0x0303L)
#define			E_UL0304_ADI_OPNAME	    (E_UL_MASK + 0x0304L)
#define			E_UL0305_NO_MEMORY	    (E_UL_MASK + 0x0305L)

/*
**  Error codes for exception handler routines
*/
#define			E_UL0306_EXCEPTION	    (E_UL_MASK + 0x0306L)
#define			E_UL0307_EXCEPTION	    (E_UL_MASK + 0x0307L)
#define			E_UL0308_ADF_EXCEPTION	    (E_UL_MASK + 0x0308L)
#define			E_UL0309_ADF_EXCEPTION	    (E_UL_MASK + 0x0309L)

/*
** More error codes for query tree to text routines
*/
#define			E_UL030A_BAD_LANGUAGE	    (E_UL_MASK + 0x030AL)
#define			E_UL030B_BAD_QUERYMODE	    (E_UL_MASK + 0x030BL)

/*
[@type_definitions@]
*/

#define             ULD_TSIZE           256
/* default max length of string in tree to text conversion,
** size could be larger for long character strings however, this
** constant is used to determine whether a "new line" is needed
** but longer strings should be expected
*/


/*}
** Name: ULD_LENGTH - type used to defined text string lengths
**
** Description:
**	Length type for a text string, used in query tree to text
**      conversion routines.
** History:
**      31-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef u_i2        ULD_LENGTH;


/*}
** Name: ULD_TSTRING - linked list of null terminated strings
**
** Description:
**      Linked list of null terminated strings used to contain the
**      output of a query tree to text conversion routine.
**
** History:
**      31-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _ULD_TSTRING
{
    struct _ULD_TSTRING *uld_next;	/* ptr to next element or NULL */
    ULD_LENGTH	    uld_length;		/* length of null terminated
                                        ** string */
    char            uld_text[ULD_TSIZE]; /* contains null terminated 
					** text string */
} ULD_TSTRING;


/**
** Name: ULT portion - Includes macros and values for tracing in Jupiter
**
** Description:
**      This section contains the ult_ macros used to define and use
**      trace flags in the Jupiter DBMS.
**
** History: $Log-for RCS$
**      15-Apr-86 (fred)
**          Created for Jupiter.
**/

/*}
** Name: ULT_BTM - trace bit mask
**
** Description:
**      This structure describes a bit mask for trace flags.
**      It exists primarily to keep the compiler and lint happy;
**      its actual size varies with usage.
**
**
** History:
**     15-Apr-86 (fred)
**          Created on Jupiter
*/
typedef struct _ULT_BTM
{
    i4         btm_elem[1];           /* a word in the mask */
}   ULT_BTM;

/*}
** Name: ULT_ELEMENT - a set of values for some trace flag
**
** Description:
**      This structure holds the values for one particular trace flag.
**      An array of these (see below) is kept in the trace vector.
**
** History:
**     15-Apr-86 (fred)
**          Created on Jupiter.
*/
typedef struct _ULT_ELEMENT
{
    i4         elem_flag;          /* which flag are these values for */
    i4	    elem_v1;		/* the first value */
    i4	    elem_v2;		/* the second value */
}   ULT_ELEMENT;

/*}
** Name: ULT_VAL - the value array
**
** Description:
**      This structure is used to hold the values used in tracing.
**      See comment for above.
**
** History:
**     15-Apr-86 (fred)
**          Created on Jupiter.
**
*/
typedef struct _ULT_VAL
{
    ULT_ELEMENT	    val_elem[1];     /* an array of values */
}   ULT_VAL;

/*}
** Name: ULT_TVECT - A trace vector
**
** Description:
**      This structure implements a trace vector.  It contains fields for 
**      the highest bit number which will fit, for the number of bits with
**	which values are associated, and for the number of value pairs  
**      which are to be kept at a time.  Following these fields, 
**      an array of i4s is kept in which the bit mask is stored. 
**      Following this is the array of (trace number, value1, value2) 
**      items. 
** 
**      SPECIAL NOTE:  This structure must be kept in sync with the structure 
**      defined by the ULT_VECTOR_MACRO macro.  This is used to construct 
**      bit vectors in client space which are the correct size.
**
** History:
**     15-Apr-86 (fred)
**          Created on Jupiter
*/
typedef struct _ULT_TVECT
{
    i4              ulhb;               /* highest bit number */
    i4              ulvc;               /* bit number of first flag with a value */
    i4		    ulvao;		/* number of values to hold at once */
    ULT_BTM	    *ulbp;		/* pointer to the actual bit mask */
    ULT_VAL	    *ulvp;		/* pointer to the value array */
} ULT_TVECT;

/*
**  Defines of other constants.
**  The following are some simple defines to help keep the macro lengths
**  under 256, a limit imposed by the VMS compiler.
*/

#define			ULBPL	(sizeof(i4) * BITSPERBYTE)
#define			ULBTM(b)    (1L << ((b) % ULBPL))    /* to make things smaller */


/*{
** Name: ULT_VECTOR_MACRO	- Define a trace vector of the appropriate size
**
** Description:
**      This macro defines a trace vector of the appropriate size.
**	It must be defined as a macro in order to work in the declaration
**	section of the C program in which it is embedded.  The parameters
**	have short names on all of these macros in order to keep the length
**	of the macro down to one line of length 256 bytes or less.  This
**	restriction is imposed by the Whitesmith C compiler on VM/CMS.
**
** Inputs:
**      nb                              Number of flags/bits all total.
**      nvao                            Number of values to hold at once.
**
** Outputs:
**      (builds a structure of the appropriate size)
**  Usage:
**	ULT_VECTOR_MACRO(25, 8)	vect;	This declaration will produce a trace
**					vector structure which will hold 25 flags,
**					8 of which can have values at one time.
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Apr-1986 (fred)
**          Created on Jupiter.
*/

#define                 ULT_VECTOR_MACRO(nb,nvao) struct { ULT_TVECT vect; i4 bv[((nb)/ULBPL) + 1]; ULT_ELEMENT vv[(nvao)]; }

/*{
** Name: ult_init_macro	- Initialize (runtime) a vector defined above.
**
** Description:
**      This macro (which could be a routine) initializes the vector defined 
**      by ULT_VECTOR_MACRO for use by the rest of the trace system.  It fills 
**      in the description information defined below, and clears all the flags 
**      and bit masks.  It should only be called at the beginning of a 
**      session/server since it will clear all trace flags (which, I suppose, is 
**      a legitimate use of it).
**
**	NOTE:  Expect one "illegal structure pointer combination" lint error
**	from the use of this macro.  It is harmless.
**
** Inputs:
**      v                               the vector to initialize.  This is the address
**					of the vector built with ULT_VECTOR_MACRO.
**      nb                              number of bits/flags (include those which
**					may have values.  This number must match
**					the `nb' supplied above.
**      nvp                             number of value pairs.  This is the number
**					of flags which have values associated with 
**					them.  This number should not be confused
**					with ....
**      nvao                            number of values at once.  This is the number
**					of values which can held at once.  This number
**					number must match the `nvao' given at once.
**
** Outputs:
**      none
** Usage:
**	ult_init_macro(&vector, 250, 10, 3);
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Apr-86 (fred)
**          Created on Jupiter.
**      1-dec-87 (seputis)
**          Added casting to MEfill for lint
**
*/

#define ult_init_macro(v, nb, nvp, vao) (MEfill((u_i2)sizeof(*(v)),(u_char)0,(PTR)(v)),(v)->vect.ulbp = (ULT_BTM *)(v)->bv, (v)->vect.ulvp = (ULT_VAL *) (v)->vv,(v)->vect.ulvao = (vao), (v)->vect.ulvc = (nvp), (v)->vect.ulhb = (nb)-1) 


/*{
** Name: ult_check_macro	- check state of trace flag
**
** Description:
**      This macro (could be routine) checks the state of the specified 
**      trace flag, returning true if it is set, false if not.  If the 
**      flag should have values, these are retrieved.  The act of retrieving 
**      values will entail a procedure call;  simply checking a flag 
**      will not. 
**
**
** Inputs:
**      v                               address of the vector which contains the info
**      b                               the flag in question
**      v1, v2                          pointers to i4's to receive the
**					values.
**
** Outputs:
**      *v1, *v2                        if appropriate.
** Usage:
**	if (ult_check_macro(&vector, 22, &v1, &v2)
**	    TRdisplay("Useful information regarding trace flag 22\n");
**
**	Returns:
**	    bool (zero or nonzero)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Apr-86 (fred)
**          Created on Jupiter.
*/

#define ult_check_macro(v, b, v1, v2) ((v)->bv[(b)/ULBPL] & ULBTM((b)) ? ((b) > ((v)->vect.ulhb - (v)->vect.ulvc) ? ult_getval(&(v)->vect,(b),(v1),(v2)) : (bool) 1) : (bool) 0)


/*{
** Name: ult_set_macro	- set a particular trace flag.
**
** Description:
**      This macro (could be routine) sets a particular trace flag.  If 
**      this flag should have values associated with it, a routine is 
**      called to place these values in the vector.  Should this fail, 
**      a status of E_DB_ERROR is returned.  This should fail only if the trace 
**      vector value area is full.  E_DB_ERROR is also returned if an attempt
**	is made to set a flag which is invalid (larger than declared)
**
** Inputs:
**      v                               address of the vector in question
**      b                               the bit/flag to set
**      v1, v2                          i4's specifying the value.
**
** Outputs:
**	none.
**
** Usage:
**	if (ult_macro(&vector, 22, 0, 0) != E_DB_OK)
**	    TRdisplay("Unable to set trace flag 22\n");
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	16-Apr-86 (fred)
**          Created on Jupiter.
*/

#define	ult_set_macro(v, b, v1, v2) (((v)->vect.ulhb >= (b)) ? (((v)->bv[(b)/ULBPL] |= ULBTM((b)), (((b) > ((v)->vect.ulhb - (v)->vect.ulvc)) ?ult_putval(&(v)->vect,(b),(v1),(v2)) : E_DB_OK))) : E_DB_ERROR) 


/*{
** Name: ult_clear_macro	- clear a trace flag & its values
**
** Description:
**      This macro (could be a routine) clears the specified trace flag.  If 
**      there were values associated with this flag, these are removed as well. 
**      E_DB_ERROR is returned if the flag is out of range.
**
** Inputs:
**      v                               address of the trace vector on which to operate
**      b                               the flag to clear
**
** Outputs:
**	none.
**
** Usage:
**	ult_clear_macro(&vector, 22);
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**	16-Apr-86 (fred)
**          Created on Jupiter.
*/

#define	ult_clear_macro(v, b) (((v)->vect.ulhb >= (b)) ? ((v)->bv[(b)/ULBPL] &= ~(ULBTM((b))), ((b) > ((v)->vect.ulhb - (v)->vect.ulvc) ?ult_clrval(&(v)->vect,(b)) : 0)) : E_DB_ERROR) 


/*
**  Forward and/or External function references.
*/
FUNC_EXTERN VOID      uld_prtree      ( PTR root, VOID (*printnode)(),
					PTR (*leftson)(), PTR (*rightson)(),
					i4 indent, i4  lbl, i4  facility );

FUNC_EXTERN VOID      uld_prnode( PTR control, char *frmt );

FUNC_EXTERN i4	      uld_tr_callback( PTR nl, i4 length,
					char *buffer );

FUNC_EXTERN VOID      ule_initiate( char *node_name, i4  l_node_name,
				    char *server_name, i4  l_server_name );

/*
** uleFormatFcn takes a variable number of arguments.
**
** This prototype is here for checking that uleFormatFcn is called 
** in a way that is type correct for the first twelve arguments.
*/

FUNC_EXTERN DB_STATUS uleFormatFcn(
        DB_ERROR       *dberror,
	i4	       error_code,
        CL_ERR_DESC    *clerror,
        i4             flag,
        DB_SQLSTATE    *sqlstate,
        char           *msg_buffer,
        i4             msg_buf_length,
        i4             *msg_length,
        i4	       *uleError,
	PTR	       FileName,
	i4	       LineNumber,
	i4	       num_parms,
        ...);

#ifdef HAS_VARIADIC_MACROS

/* The preferred macro, requiring an explicit DB_ERROR pointer as first arg */
#define uleFormat(DBerror,error_code,clerror,flag,sqlstate,msg_buffer, \
                   msg_buf_length,msg_length,uleError, ...) \
            uleFormatFcn(DBerror,error_code,clerror,flag,sqlstate,msg_buffer, \
                           msg_buf_length,msg_length,uleError, \
                           __FILE__, \
                           __LINE__, \
                           __VA_ARGS__)

/* Provide bridge to extant uses of ule_format */
#define ule_format(error_code,clerror,flag,sqlstate,msg_buffer, \
                   msg_buf_length,msg_length,uleError, ...) \
            uleFormatFcn(NULL,error_code,clerror,flag,sqlstate,msg_buffer, \
                           msg_buf_length,msg_length,uleError, \
                           __FILE__, \
                           __LINE__, \
                           __VA_ARGS__)
#else

/* Variadic macros not supported */
#define uleFormat uleFormatNV

FUNC_EXTERN DB_STATUS uleFormatNV(
        DB_ERROR       *dberror,
	i4	       error_code,
        CL_ERR_DESC    *clerror,
        i4             flag,
        DB_SQLSTATE    *sqlstate,
        char           *msg_buffer,
        i4             msg_buf_length,
        i4             *msg_length,
        i4	       *uleError,
	i4	       num_parms,
        ...);

#define ule_format ule_formatNV

FUNC_EXTERN DB_STATUS ule_formatNV(
	i4	       error_code,
        CL_ERR_DESC    *clerror,
        i4             flag,
        DB_SQLSTATE    *sqlstate,
        char           *msg_buffer,
        i4             msg_buf_length,
        i4             *msg_length,
        i4	       *uleError,
	i4	       num_parms,
        ...);

#endif /* HAS_VARIADIC_MACROS */


FUNC_EXTERN bool      ult_getval( ULT_TVECT *vector, i4  bit, i4 *v1,
				  i4 *v2 );
FUNC_EXTERN DB_STATUS ult_putval( ULT_TVECT *vector, i4  flag, i4 v1,
				  i4 v2 );
FUNC_EXTERN DB_STATUS ult_clrval( ULT_TVECT *vector, i4  flag );

/*
** SC930 trace functions
**
**   ult_always_trace - check if SC930 tracing is set
**   ult_open_tracefile - open the session specific trace file
**   ult_print_tracefile - print to the trace file
**   ult_close_tracefile - close the trace file
*/

FUNC_EXTERN bool ult_always_trace(void);
FUNC_EXTERN void *ult_open_tracefile(void *);
FUNC_EXTERN void ult_print_tracefile(void *,i2 ,char *);
FUNC_EXTERN void ult_close_tracefile(void *);
FUNC_EXTERN bool ult_trace_qep(void);

#define	SC930_VERSION		6	/* version of SC930 output */
/* SC930 output line types */
#define SC930_LTYPE_UNKNOWN		0
#define SC930_LTYPE_ADDCURSORID		1
#define SC930_LTYPE_ENDTRANS		2
#define SC930_LTYPE_COMMIT		3
#define SC930_LTYPE_SECURE		4
#define SC930_LTYPE_ABORT		5
#define SC930_LTYPE_ROLLBACK		6
#define SC930_LTYPE_ABSAVE		7
#define SC930_LTYPE_RLSAVE		8
#define SC930_LTYPE_BGNTRANS		9
#define SC930_LTYPE_SVEPOINT		10
#define SC930_LTYPE_AUTOCOMMIT		11
#define SC930_LTYPE_DDLCONCUR		12
#define SC930_LTYPE_CLOSE		13
#define SC930_LTYPE_FETCH		14
#define SC930_LTYPE_DELCURSOR		15
#define SC930_LTYPE_EXEPROCEDURE	16
#define SC930_LTYPE_EXECUTE		17
#define SC930_LTYPE_TDESC_HDR		18
#define SC930_LTYPE_TDESC_COL		19
#define SC930_LTYPE_PARMEXEC		20
#define SC930_LTYPE_PARM		21
#define SC930_LTYPE_QEP			22
#define SC930_LTYPE_BEGINTRACE		23
#define SC930_LTYPE_QUEL		24
#define SC930_LTYPE_QUERY		25
#define SC930_LTYPE_REQUEL		26
#define SC930_LTYPE_REQUERY		27

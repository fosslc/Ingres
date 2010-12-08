/*
** Copyright (c) 2004, 2008, 2010 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <me.h>
#include    <mh.h>
#include    <ex.h>
#include    <st.h>
#include    <cm.h>
#include    <tr.h>
#include    <sl.h>
#include    <clfloat.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <add.h>
#include    <adfops.h>
#include    <ade.h>
#include    <adefebe.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adeint.h>
#include    <aduint.h>
#include    <adumoney.h>
#include    <adudate.h>
#include    <adffiids.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    <adp.h>

/*
NO_OPTIM = rs4_us5 su4_u42 su4_cmw i64_aix r64_us5
*/

#undef  COUNTS 


/**
**
**  Name: ADEEXECUTE.C - Execution phase of ADF's Compiled Expression Module.
**
**  Description:
**      This file contains all of the routines necessary to execute ADF
**      expressions.  Throughout this file, an ADF compiled expression
**      will be abbreviated as, "CX".
**
**	This file contains the following externally visible routines:
**	------------------------------------------------------------
**          ade_vlt_space() - Calculate scratch space needed for VLTs.
**          ade_execute_cx() - Execute a specified segment of a CX.
**
**	This file also contains the following static routines:
**	-----------------------------------------------------
**	    ad0_opr2dv() - Build a DB_DATA_VALUE from an operand.
**          ad0_fill_vltws() - Build an ADE_VLT_WS_STRUCT array from the CX.
**          ad0_fltchk() - Check and report floating point overflow.
**
**
**  History:
**      10-jun-86 (thurston)
**          Initial creation.  Split off from ADE.C (which is now called
**	    ADECOMPILE.C).
**	16-jul-86 (thurston)
**	    Coded ade_execute_cx(), and changed its argument list into a
**	    control block.
**	19-jul-86 (thurston)
**	    Split the ade_*_print() routines off into the ADEDEBUG.C file.
**	22-jul-86 (thurston)
**	    Wrote the ad0_fill_vltws() static routine.
**	25-jul-86 (thurston)
**          In the ade_execute() routine, I moved the pointer to ADF session
**          control block out of the ADE_EXCB and made it a seperate argument to
**          be consistent with the rest of ADF. 
**	04-aug-86 (thurston)
**	    Added code to ade_execute_cx() to handle zero argument f.i.'s.
**	    Also, I added an xDEBUG test to make sure the function instance ID
**          is a valid one before using it to reference the f.i. lookup table.
**          Also, made the value of the CX always return ADE_NOT_TRUE for error
**          conditions. 
**	13-aug-86 (thurston)
**	    Removed the #define for NEG_I1 since it is already done in
**	    <compat.h>.  Also, removed the static function ade_i1_to_i4().
**	    In its place, the routine now uses the I1_CHECK_MACRO defined
**	    in <compat.h>.
**	14-aug-86 (thurston)
**	    Had to add an extra parameter to ADE_GET_OPERAND_MACRO() uses to
**	    satisfy the FRONTEND_AND_PC version.
**	02-sep-86 (thurston)
**	    In ade_execute_cx(), if the function instance is an aggregate, the
**	    low level aggregate routine was being called incorrectly (the last
**	    two args were reversed).  This has been fixed.
**	01-oct-86 (thurston)
**	    Added code to ade_execute_cx() to support the ADI_LIKE_OP operation.
**	28-dec-86 (thurston)
**	    In the ade_execute_cx() routine, I modified the code that performs
**	    the ADE_CALCLEN instruction to use the ADE_LEN_UNKNWON constant.
**	05-jan-87 (thurston)
**          In the ade_execute_cx() routine, I commented out the "#ifdef xDEBUG"
**          from around the safety check which guards against using either a
**          f.i. that is illegal, or any f.i. this routine doesn't know how to
**          perform but has a NULL pointer to its routine.  This way, these
**          safety checks will always be compiled in. 
**	28-jan-87 (thurston)
**	    In ade_execute_cx(), I have added code to better handle warnings
**	    that could have possibly occurred in the middle of executing a CX.
**	    Previously, execution was halted at this point.  Now, execution will
**	    continue, and when it is time to return, either E_DB_OK or E_DB_WARN
**	    will be returned, depending on whether any warnings occurred.  Note
**	    that if more than one warning happened, the error block in the ADF
**	    session control block will only reflect the last one.
**	03-feb-87 (thurston)
**	    Corrected a bug in ade_execute_cx() whereby all of the comparison
**          operators between "i" and "f" were assuming that both inputs were of
**          type "f".  Turned out that this was a left-over from the 5.0 code,
**          where a "hack" had been put into PNODERSLV.C that forced coercions
**          of "i"s to "f"s if one of these mixed comparisons was being done. 
**	24-feb-87 (thurston)
**	    Moved definitions of forward static functions out of the routines
**	    that use them to the top of the file.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	10-mar-87 (thurston)
**	    Added code to ade_execute_cx() to perform the `is null' and
**	    `is not null' functions.  Also added code to perform the CX
**	    special instructions:  ADE_1CXI_SET_SKIP and ADE_2CXI_SKIP.
**	17-mar-87 (thurston)
**	    Added code to ade_execute_cx() to execute the ADE_NAGEND CX
**	    instruction, and the ADE_3CXI_CMP_SET_SKIP instruction.
**	19-mar-87 (thurston)
**	    Converted all occurances of NEG_I1 into equivalent I1_CHECK_MACRO's.
**	25-mar-87 (thurston)
**	    In ade_execute_cx(), I made sure instruction offsets are calclulated
**	    with reference to ade_excb->excb_bases[ADE_CXBASE] instead of
**	    ade_excb->excb_cx, since for frontends, the CX header may not be
**	    contiguous with the list of instructions.
**	05-apr-87 (thurston)
**	    In ade_execute_cx(), made the ADE_1CXI_SET_SKIP instruction return
**	    error if output is non-nullable and a null value input is found.
**	08-apr-87 (thurston)
**	    Fix bug in ade_execute_cx() where `is null' and `is not null' where
**	    always assuming the datatype was nullable.
**	13-apr-87 (thurston)
**          Added code to ade_execute_cx() that should allow the S/E/A functions
**          to return proper values.  In the future, however, we must do these
**          differently since, (1) the way I have it coded right now is a hog,
**          and (2) several of the S/E/A functions should be ADF constants (i.e.
**          constant for the life of the query. 
**	15-apr-87 (thurston)
**	    Fixed bug in ade_execute_cx() with the ADE_NAGEND instruction; it
**	    was passing nullable datatype to low-level ag-end function.
**	21-apr-87 (thurston)
**	    Added the ADE_PMQUEL, ADE_NO_PMQUEL, and ADE_PMFLIPFLOP instructions
**	    to ade_execute_cx().
**	28-apr-87 (thurston)
**	    In ade_execute_cx(), I fixed ADE_KEYBLD instruction to handle the
**	    LIKE operator pattern match semantics.  Also fixed the ADE_COMPARE
**	    instruction to compare text values properly.
**	29-apr-87 (thurston)
**	    Added the ADE_PMENCODE instruction to ade_execute_cx().
**	29-apr-87 (thurston)
**	    In ade_execute_cx(), I added specific check to `f ** f' so that
**	    it returns an error if arg 1 is negative, or if arg 1 is zero and
**	    arg 2 is non-positive.
**	06-may-87 (thurston & wong)
**	    Integrated FE changes.
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
**	14-may-87 (thurston)
**	    In ade_execute_cx(), I fixed ADE_NAGEND so that it returns a zero
**	    value for `count' and `count*' when aggregated over an empty set.
**	15-may-87 (thurston)
**	    In ade_execute_cx(), I got rid of the initializations of automatic
**	    arrays that were being done for the S/E/A functions.  (Unix compiler
**	    bitched about it.)
**	10-jun-87 (thurston)
**	    Added code to ade_execute_cx() to perform the four new function
**	    instances:
**		f  :  f * i			f  :  f / i
**		f  :  i * f			f  :  i / f
**	13-jul-87 (thurston)
**	    Made fix in ade_execute_cx() to the ADE_1CXI_SET_SKIP instruction:
**	    When no null values were found in the inputs, the code had been
**	    clearing the null indicator bit in the output ... EVEN IF THE OUTPUT
**	    WAS NON-NULLABLE!
**	28-jul-87 (thurston)
**	    In ade_execute_cx(), I upgraded the ADE_COMPARE instruction to
**          handle nullables and null values with the semantics necessary to do
**          keyed joins. 
**	24-aug-87 (thurston)
**	    In ade_execute_cx(), I removed the specific check for a bad math arg
**          (see history comment by me dated 29-apr-87).  Instead, we now depend
**          on the MH routines to signal MH_BADARG, and adx_handler() to deal
**          with it.
**	16-sep-87 (thurston)
**	    Re-coded the ADE_KEYBLD instruction in ade_execute_cx() to do `range
**          key' failure checking instead of `pattern match' failure checking.
**          The new plan, talked about at length between OPC (Eric Lundblad) and
**          ADE (Gene Thurston), and is a lot more consistent with what is truly
**          needed to keep or throw away query plans. 
**	26-oct-87 (thurston)
**	    In ade_execute_cx(), I added code to execute the new ADE_BYCOMPARE
**	    instruction, where NULL values compare equal to other NULL values.
**	05-dec-87 (thurston)
**	    In ade_execute_cx(), I fixed the ADE_COMPARE and ADE_BYCOMPARE
**	    instructions so they do floating point compares in f4 if EITHER
**	    argument is an f4.
**	16-jan-88 (thurston)
**	    Removed all of the string comparison function instance cases where
**	    the datatypes of the two operands was different.  (See LRC proposal
**	    for new string semantics, 11-jan-88 by Gene Thurston.)
**	05-feb-88 (thurston)
**	    Did MAJOR work on VLTs.  Aparently, these had never been tested
**	    since there were about 20 things wrong with them.  Not the least of
**	    which is that we need VLT `c' and `char' as well as VLT `text' and
**	    `varchar'.  With the removal of all of the function instance cases
**	    for the LRC proposal (see above), and the reordering of the datatype
**	    resolution hierarchy, VLTs are MUCH more common, thus the problems
**	    arose.  Work was done on ade_execute_cx(), ad0_fill_vltws(), and
**	    ade_vlt_space().
**	10-feb-88 (thurston)
**	    In ade_execute_cx(), I did away with using the AD_BIT_TYPE for the
**	    NPI's since it didn't work for VLUP's VLT's.
**	10-feb-88 (thurston)
**	    Changed all occurrences of `#ifndef ADE_FRONTEND' to be
**	    `#ifdef ADE_BACKEND'.  This is much easier to read.
**	10-feb-88 (thurston)
**	    Added code to handle the new ADE_SVIRGIN segment.
**	08-jun-88 (thurston)
**	    Fixed bug with the count(*) aggregate in ade_execute_cx() ... all
**	    aggregates had been assuming that there would always be exactly one
**	    input argument ...  count(*) has zero.  Fixed by adding a `case'
**	    specifically for ADFI_396_COUNTSTAR.
**	09-jun-88 (thurston)
**	    Added code to ade_execute_cx() to handle the new ADE_ESC_CHAR
**	    instruction, and modified the LIKE and NOT LIKE function instances
**	    so that the escape character will be used.
**	21-jun-88 (thurston)
**	    Added code to ade_execute_cx() so that the escape character gets
**	    used for the ADE_KEYBLD instruction.
**	07-oct-88 (thurston)
**	    Altered the ADE_COMPARE and ADE_BYCOMPARE instructions in
**	    ade_execute_cx() so that they fill in the .excb_cmp field with
**	    ADE_1LT2, ADE_1EQ2, ADE_1GT2, or ADE_1UNK2 instead of -1, 0, 1.
**	23-oct-88 (thurston)
**	    Beefed up performance of ADE_MECOPY instruction in
**	    ade_execute_cx() by using a loop with I4ASSIGN_MACRO's
**	    and a second loop copying u_char's for the remainder.  This special
**	    processing will only take effect if the size of the data to be
**	    copied is < ADE_CUTOFF_LEN_4_MECOPY ...  currently set at 40.
**	09-nov-88 (thurston)
**	    Changed all of the E_AD0FFF_NOT_IMPLEMENTED_YET returns in
**	    ade_execute_cx() to return E_AD8999_FUNC_NOT_IMPLEMENTED instead.
**	19-feb-89 (paul)
**	    Added new instructiosn to support the creation of QEF_USR_PARAM
**	    structures for the CALLPROC statement.
**	    ADE_PNAME computes the address of parameter 1 and places it at the
**	    location described by parameter 0 and places the length of
**	    parameter 1 in the location following the parameter 0 pointer.
**	    ADE_DBDV creates a DB_DATA_VALUE for parameter 1 at the location
**	    described by parameter 0.
**      28-feb-89 (fred)
**          Changed references to Adi_* global variables to be referenced as
**	    pointers thru the server control block (Adf_globs) for user defined
**	    ADT support.
**	17-apr-89 (paul)
**	    Corrected unix compiler warning.
**	02-may-89 (mikem)
**	    Added cases to handle ISNULL and ISNOTNULL for table_key and 
**	    object_key.
**	10-may-89 (anton)
**	    Added local collation support
**	23-may-89 (jrb)
**	    Changed ADE_COMPARE instr to return three new values: ADE_1ISNULL,
**	    ADE_2ISNULL, and ADE_BOTHNULL instead of ADE_1UNK2.  This was to fix
**	    a problem executing an FSM in QEF.  Also changed ADE_BYCOMPARE to
**	    stop returning ADE_1UNK2 since it shouldn't according to the ADE
**	    design spec.
**	07-jun-89 (jrb)
**	    Fix bug introduced by my fix above where ade_bycompare should set
**	    the CX's value to ADE_NOT_TRUE whenever the comparison result is not
**	    equality.
**	17-Jun-89 (anton)
**	    Moved local collation routines to mainline from CL
**	22-jan-90 (jrb)
**	    Alignment fix for VLUPs.
**      14-aug-91 (jrb)
**          Added inbuf and outbuf as aligned buffers in ade_execute_cx.
**      11-dec-91 (fred)
**	    Added support for large objects.  This includes checking and
**	    providing the size used in a specified base (one per expression)
**	    and the returning of partial completion of an expression -- with
**	    the expectation that this routine will be called again later with
**	    the excb_continuation field unaltered (and, of course, the basic CX
**	    also unaltered).
**      29-sep-1992 (stevet)
**          Added DB_ALL_TYPE for isnull and isnotnull.  Removed isnull and
**          isnotnull entries for 65 datatypes.  Added support for
**          ii_row_count, which is computed in line withint ade_execute_cx.
**      29-dec-1992 (stevet)
**          Added function prototype.
**      08-mar-1993 (stevet)
**          Call MHi4 routines to handle arithematic calculations for INTEGER.
**          Added check for divide-by-zero error for FLOAT and signal 
**          EXFLTDIV excpetion.
**      06-apr-1993 (stevet)
**          Set return value of divide-by-zero to zero.
**      08-apr-1993 (jrb, stevet)
**	    Bug 39071.  For some VLT's the count would be different from its
**	    VLUP/VLT input, and the NULL byte would get misplaced.  The fix was
**	    to move the NULL byte to just after the count field by using
**	    the ADF_VLUP_SETNULL macro on two argument functions that
**          returns a nullable character types.
**      20-apr-1993 (stevet)
**          Fixed type mismatch for ME routines.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      15-Jul-1993 (fred)
**          Added handling for DB_VBYTE_TYPE VLUP/VLT's.
**	08-Aug-1993 (shailaja)
**	     Unnested comments.
**      16-agu-1993 (stevet)
**          Fixed type mismatch when calling CVlpk().
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      17-jan-1994 (stevet)
**          Added ad0_fltchk() to check for floating point overflow (B58853).
**	27-jan-94 (rickh)
**	    Only emit AD0002_INCOMPLETE code if we're processing a BLOB.
**	    This will  prevent the INCOMPLETE handshake (with QEF/SCF) from
**	    looping infinitely if OPC mis-generates a compiled expression.
**	25-apr-1994 (stevet/rog)
**	    Turn off optimization for su4_u42 because we get better performance
**	    with it off (due to a big switch statement).
**	28-apr-94 (vijay)
**	    Change abs to fabs when acting on floats.
**  	07-Oct-94 (ramra01)
**      BLOBs followed by other fields could potentially run out of OUTBUFFER
**      and cause transmission error. By making space available for known
**      data lengths past the BLOB, it ensures that all of the fields go thru
**      successfully
**	6-may-94 (robf)
**          Propagate 25-apr-94 change to su4_cmw as well.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch for c89
**	13-jul-1995 (ramra01)
**	    count(*) optimization implementation. 
**	25-oct-95 (inkdo01)
**	    A wide variety of CPU performance enhancements were coded.
**	08-nov-1995 (wilri01)
**	    Fast string compare
**	21-nov-95 (emmag)
**	    Backed out one of inkdo01's changes which has broken
**	    ABF.
**      10-jan-1996 (toumi01; from 1.1 axp_osf port)
**          Added kchin's change for axp_osf.
**          05-jul-1993 (kchin)
**          Need to adjust pointer: dbv to point to right structure element
**          this is to avoid extra padding introduced by compiler to align
**          pointer structure elements to 8-byte boundary. See comment
**          in routine ade_execute_cx(), at case ADE_DBDV:
**	26-jan-1996 (toumi01)
**	    In eight places that do address calculation by adding an offset
**	    to an address cast to an i4 or an int, make the cast instead to
**	    long to avoid truncation on platforms like axp_osf where a PTR
**	    is longer than an int.
**	26-feb-96 (thoda04)
**	    Added adp.h to include function prototype for adu_redeem.
**	3-jul-96 (inkdo01)
**	    Added ADE_4CXI_CMP_SET_SKIP operator for support of predicate
**	    function operation types.
**	23-Sep-1997 (mosjo01)
**	    Forced to NO_OPTIM rs4_us5 inorder to process "select _version()"
**	    and "select user_name, dba_name from iidbconstants" as used in 
** 	    upgradefe (iisudbms).
**	26-Mar-98 (shust01)
**	    Added additional I1_CHECK_MACRO statements (by do_adccompare
**	    label, in DB_INT_TYPE case). Needed for platforms (such as AIX)
**	    that only has unsigned chars.  Problem when joining tables by
**	    integer1 fields when some of the data is negative. Bug 45988.
**	06-Apr-1998 (hanch04)
**	    Reorder switch statement.  Common ADE cases are moved to the
**	    beginning.  For ADE_MECOPY always use MEcopy call for performance.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	23-Feb-1999 (matbe01)
**	    Moved "include ex.h" after "include mh.h".  The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
**      13-Mar-1999 (hanal04)
**          In ad0_fill_vltws() initialise dv elements to ensure bad
**          addresses are not passed to adi_0calclen(). b96344.
**      07-Nov-2001 (horda03)
**          Fix for Bug 85284 only dealt with F4 = F8 comparisons, the
**          same is true for all other F4 and F8 comparisons. Now we
**          can get the situation where a query using the WHERE CLAUSE
**          F4 = F8 returns the same rows as the query F4 != F8.
**          This change causes all F4 to F8 comparisons to be made with
**          the F8 value being converted to an F4 value. Bug 85284.
**	09-Apr-1999 (shero03)
**	    Support multivariate functions.
**	    Add the #ifdef from 05-jan-87 (thurston) - this should never occur
**	13-mar-2001 (stial01)
**	    case ADFI_500_MAX_TABKEY, adjust dv db_length, db_datatype 
**	29-mar-2001 (gupsh01)
**	    Added the nchar and nvarchar comparison cases, for unicode support.
** 	06-Apr-2001 (gupsh01)
**	    Added support for max, min, and count function for nchar and 
**	    nvarchar.
**	16-apr-2001 (abbjo03)
**	    Add support for LIKE for NCHAR/NVARCHAR.
**	17-Jul-2001 (hanje04)
**	    Bug 105257
**	    Added check for null datatype before MEcopy of opr_len bytes
**	    into tempd1. Nullable decimals have length greater than 
**	    sizeof(tempd1) and causes SEGV on Linux.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**	15-dec-2001 (devjo01)
**	    Add ADE_COMPARE support for unicode.
**      19-feb-2002 (stial01)
**          Changes for unaligned NCHR,NVCHR data (b107084)
**	04-apr-2003 (gupsh01)
**	    Renamed the constants ADFI_998_NCHAR_LIKE_NCHAR to
**	    ADFI_998_NCHAR_LIKE_NVCHAR, and ADFI_999_NCHAR_NOTLIKE_NCHAR
**	    to ADFI_999_NCHAR_NOTLIKE_NVCHAR. This will remove ambiguity 
**	    between the parameters in adgfitab.roc and the function 
**	    instance ID.
**      28-apr-2003 (gupsh01)
**          Removed the case for ADFI_998_NCHAR_LIKE_NVCHAR and 
**          ADFI_999_NCHAR_NOTLIKE_NVCHAR, these cases should be dealt 
**          by coercion from case ADFI_1000_NVCHAR_LIKE_NVCHAR and 
**	    ADFI_1001_NVCHAR_NOTLIKE_NVCHAR, respectively.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	17-mar-04 (inkdo01)
**	    Changed "longlong" to "i8" for bigint support.
**      25-mar-2004 (stial01)
**           ADE_LEN_LONG: Restrict blob output to MAXI2
**      20-Oct-2004 (hweho01)
**           Turned off optimization for AIX 64-bit, avoid runtime
**           error during the upgradefe of createdb process.
**           Compiler: VisualAge C 6.006.
**	06-dec-2004 (gupsh01)
**	    For computation of stddev_pop and stddev_samp only take 
**	    absolute value of the arguments before taking the square
**	    root. This will avoid errors due to negative values 
**	    that may result due to float division in case that all the 
**	    entries are same and standard deviation is 0 (approx). 
**	29-aug-2006 (gupsh01)
**	    Added support for ANSI datetime system constants
**	    CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP, LOCAL_TIME,
**	    LOCAL_TIMESTAMP.
**	08-Feb-2008 (kiria01) b119885
**	    Change dn_time to dn_seconds to avoid the inevitable confusion with
**	    the dn_time in AD_DATENTRNL.
**	19-Feb-2008 (kiria01) b119943
**	    Correct SUM and AVG on Dates and intervals.
**	18-Feb-2008 (kiria01) b120004
**	    Consolidate timezone handling - error handing for adu_6to_dtntrnl
**	    and remove the associated MEfill.
**      10-sep-2008 (gupsh01,stial01)
**          Test for new UNORM FI's, fixed calclen for NVCHR vlups
**	27-Oct-2008 (kiria01) SIR120473
**	    Correct the pulling apart of the ADE_ESC_CHAR
**	11-Nov-2008 (kiria01) SIR120473
**	    Added explicit pattern compiler hooks.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      07-oct-2009 (joea)
**          Add cases for BOO_ISFALSE, BOO_NOFALSE, BOO_ISTRUE, BOO_NOTRUE,
**          BOO_EQ_BOO, BOO_NE_BOO, BOO_GT_BOO, BOO_LE_BOO, BOO_LT_BOO,
**          BOO_GE_BOO, BOO_ISUNKN and BOO_NOUNKN in ade_execute_cx.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
**	04-Aug-2010 (kiria01) b124160
**	    Raised limit handling on vlts. Now we have a ADE_MXVLTS_SOFT
**	    upto which the stack will be used and beyond, the limit can
**	    get to ADE_MXVLTS which is the max supported in the CXHEAD.
**	    To handle this memory will be allocated temporarily to extend
**	    the workspace.
**	09-nov-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
**/


/*
**  Defines of other constants.
*/

#define                 UNKNOWN         (-1)


/*
**  Definition of static variables and forward static functions.
*/

#ifdef    ADE_BACKEND
static  DB_STATUS   ad0_fill_vltws(ADF_CB             *adf_scb,
				   PTR                cx,
				   i4                 nbases,
				   PTR                bases[],
				   i4                 *nvlts,
				   ADE_VLT_WS_STRUCT  **vlt_ws);

#ifdef COUNTS
static i4  adfi_counts[1027] = {0};

FUNC_EXTERN void ade_barf_counts();

static VOID	   adf_printf();
#endif

#endif /* ADE_BACKEND */

static	VOID        ad0_opr2dv(PTR            bases[],
			       ADE_OPERAND    *op,
			       DB_DATA_VALUE  *dv);

static	VOID        ad0_fltchk(f8             tempf,
			       ADE_OPERAND    *op);

#ifdef    ADE_BACKEND

/*{
** Name: ade_vlt_space() - Calculate scratch space needed for VLTs.
**
** Description:
**      This routine will be used in order to calculate the amount of scratch
**      space that will be needed for all "Variable Length Temporaries" (VLTs)
**      used by the CX.  A VLT is a temporary result of an instruction whose
**      length depends on either another VLT's length or a "Variable Length
**      User Parameter's" (VLUP's) length.  Refer to the ADE Detailed Design
**      Specification document for more detail.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      ade_cx                          Pointer to the CX.
**      ade_nbases                      The # of base addresses in the ade_bases
**					array.
**      ade_bases                       Array of base addresses.  All base
**					addresses except those for the VLTs must
**					be set at this point.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      ade_needed                      The # of bytes of scratch space needed
**                                      for the VLTs in this CX.  This amount of
**                                      memory should be allocated, and the PTR
**                                      ade_bases[ADE_VLTBASE] should be set to
**                                      point to it before execution of the CX.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD2005_BAD_DTLEN		Bad length for a datatype.
**	    E_AD2020_BAD_LENSPEC	Unknown length spec.
**	    E_AD2021_BAD_DT_FOR_PRINT	Bad datatype for the print stlye
**					length specs.
**
**		The following returns will only happen when the system is
**		compiled with the xDEBUG option:
**
**          E_AD550E_TOO_MANY_VLTS      This CX has more than the max # of VLTs.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	05-feb-88 (thurston)
**	    Did MAJOR work on VLTs.  Aparently, these had never been tested
**	    since there were about 20 things wrong with them.  Not the least of
**	    which is that we need VLT `c' and `char' as well as VLT `text' and
**	    `varchar'.  With the removal of all of the function instance cases
**	    for the LRC proposal (see above), and the reordering of the datatype
**	    resolution hierarchy, VLTs are MUCH more common, thus the problems
**	    arose.
*/

DB_STATUS
ade_vlt_space(
ADF_CB             *adf_scb,
PTR                ade_cx,
i4                 ade_nbases,
PTR                ade_bases[],
i4                 *ade_needed)
{
    ADE_CXHEAD          *cxhead = (ADE_CXHEAD *) ade_cx;
    ADE_VLT_WS_STRUCT   _vlt_ws[ADE_MXVLTS_SOFT];
    ADE_VLT_WS_STRUCT	*vlt_ws = _vlt_ws;
    i4                  nvlts;
    i4                  i;
    DB_STATUS           db_stat = E_DB_OK;


    *ade_needed = 0;

    if (    cxhead->cx_n_vlts
	&&  !(db_stat = ad0_fill_vltws(adf_scb, ade_cx, ade_nbases, ade_bases,
				       &nvlts, &vlt_ws)
	     )
       )
    {
	for (i=0; i<nvlts; i++)
	{
	    *ade_needed += vlt_ws[i].vlt_len;
	    ADE_ROUNDUP_MACRO(*ade_needed, sizeof(ALIGN_RESTRICT));
	}
	if (vlt_ws != _vlt_ws)
	    MEfree(vlt_ws);
    }

    return(db_stat);
}
#endif /* ADE_BACKEND */


/*{
** Name: ade_execute_cx() - Execute a specified segment of a CX.
**
** Description:
**      This is the routine to use to actually execute the INIT, MAIN, or FINIT
**      segment of a CX.  By the time this is called, the array of base
**      addresses must be all set up, and the set of data to use must be in
**      place.  If there are any VLTs in the CX, enough scratch space to satisfy
**      them must be pointed to by the ADE_VLTBASE reserved base.
**
**	In MAIN segment, one of the bases (identified in the excb_limited_base
**	field) may be of limited size (size specified in the excb_size field).
**	When these are specified, then this routine will keep track of the
**	space used in that base.  If the space is exhausted before the
**	expression's execution is completed, this routine will return with an
**	incomplete status (E_DB_INFO/E_AD0002_INCOMPLETE), with status
**	maintained in the ade_excb parameter (excb_continuation field).  When
**	this is returned, the base in question should be replenished (either by
**	moving the current data somewhere else, or by providing a new buffer),
**	and this routine should be called again with the same excb (and with
**	the expression, excb_value field, and excb_continuation field intact).
**	Thus, this routine can be used to process expressions which produce
**	more data than it is desirable (or possible) to buffer.  This feature
**	is used in (at least) the processing of large datatypes.
**
**	    *** NOTICE -- POSSIBLY UNEXPECTED BEHAVIOR ***
**	    
**	For convenience in converting to the new form of ade_excb argument,
**	execution of any segment other than the MAIN segment will set the
**	following values in the ade_excb control block.  These settings
**	correspond to the normal (and only available in previous releases)
**	settings:
**
**	    ade_excb->excb_continuation = 0;
**	    ade_excb->excb_limited_base = ADE_NOBASE;
**	    ade_excb->excb_size = 0;
**
**	This means that these fields, if required, should be set immediately
**	preceding a the call the ade_execute_cx() for the MAIN segment.  Setting
**	things up early will be overridden if a VIRGIN, INIT, or FINIT segment
**	is executed using the same excb.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	ade_excb			Pointer to CX execution control block.
**	    .excb_cx                    Pointer to the CX.
**	    .excb_seg			Which segment of the CX to execute.
**                                      This will be either ADE_SVIRGIN,
**                                      ADE_SINIT, ADE_SMAIN, or ADE_SFINIT.
**	    .excb_nbases                The # of base addresses in the
**					.excb_bases array.
**	    .excb_limited_base		Number of the base which may be size
**					limited.  May be ADE_NOBASE if no
**					base is size limited (i.e. all bases
**					are known to be big enough).
**	    .excb_size			Size of the base identified as
**					limited.
**	    .excb_continuation		Indicates that this is a continuation of
**					previous execution.  If this is
**					non-zero, then this field contains an
**					indicator of which instruction is to be
**					continued.  That being the case, this
**					field is expected to be unmodified.
**					This field should be zero for the first
**					execution of any CX.
**		(.excb_value)		This field may have been altered, and
**					be preserved if a call is made with a
**					nonzero excb_continuation field.
**	    .excb_bases                 Array of base addresses.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	ade_excb
**	    .excb_cmp			If one or more ADE_COMPARE or
**                                      ADE_BYCOMPARE instructions were
**                                      executed, this will be set according to
**                                      the last one of these to be executed as
**                                      follows:
**					    ADE_1LT2:  Compare showed "<"
**					    ADE_1EQ2:  Compare showed "="
**					    ADE_1GT2:  Compare showed ">"
**					and for only ADE_COMPARE three more:
**					    ADE_1ISNULL:  1st value was NULL
**					    ADE_2ISNULL:  2nd value was NULL
**					    ADE_BOTHNULL: Both values were NULL
**					For ADE_COMPARE with NULL values, one of
**					the last three values will be set;
**					without NULLs one of the first three
**					will be returned.  However, for
**					ADE_BYCOMPARE, a NULL value will
**					compare equal to another NULL value
**					(giving ADE_1EQ2) and a NULL value is
**					greater than any non-NULL value.  If no
**					ADE_COMPARE or ADE_BYCOMPARE
**					instruction was executed, this will be
**					undefined.  This means that the routine
**					that calls to have the CX executed must
**					know whether or not one of these
**					instructions is in the CX if they are
**					interested in this field.
**	    .excb_nullskip		At the beginning of execution of any
**                                      segment of a CX, this field will be
**                                      set to zero.  During execution, if
**                                      one or more NULL values are skipped
**                                      by any aggregate operation, then this
**                                      field will be set to 1.
**	    .excb_value			The "value" of the CX.  When a segment
**                                      of a CX is executed it always evaluates
**                                      to either ADE_TRUE or ADE_NOT_TRUE.
**                                      This is called the value of the CX.
**					If an error condition occurs during CX
**					execution, this will always be returned
**                                      as ADE_NOT_TRUE.
**	    .excb_size			Size used of the excb_limited_base base
**					if excb_limited_base is not ADE_NOBASE.
**	    .excb_continuation		An indicator for ade_execute_cx of where
**					execution of this CX is to continue on
**					recall.  This field should not be
**					altered if continuation is necessary.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_INFO, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0002_INCOMPLETE		The expression has not yet completed
**					execution, but the base indicated by
**					excb_limited_base has been exhausted.
**					Replenish this base and recall the CX
**					with the excb_continuation field
**					unaltered.  (DB_STATUS code returned for
**					this case will be E_DB_INFO.)
**	    E_AD5500_BAD_SEG		Unknown CX segment.
**	    E_AD550A_RANGE_FAILURE	Range-key failure occurred.  This
**                                      error comes from executing an ADE_KEYBLD
**                                      instruction where a range-type key is
**                                      either specifically asked for or not,
**                                      and the data did not agree.
**
**		The following returns will only happen when the system is
**		compiled with the xDEBUG option:
**
**          E_AD8999_FUNC_NOT_IMPLEMENTED   The routine to execute some function
**                                      instance compiled into the CX has not
**                                      yet been implemented into the system.
**	    E_AD2010_BAD_FIID		Illegal function instance ID was found
**					in the CX.
**	    E_AD550B_TOO_FEW_BASES	Not as many base addresses as were
**                                      expected have been supplied.
**	    E_AD550D_WRONG_CX_VERSION	The CX has a version number that this
**					routine does not know how to execute.
**					Typically, this is because the compile
**					of the CX has never been ended.
**
**	    { Other returns may occur if an indirect call to some low level
**	      ADF routine results in an error. }
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    Depending on the instructions compiled into the CX, some memory
**	    locations referenced by offsets from the various base addresses in
**	    the .excb_bases[] array will be modified.  Hopefully, the compiler
**	    and the routine calling this routine were in sync, and these
**	    "results" will be put in the expected places.
**
** History:
**	17-jun-86 (thurston)
**          Initial creation.
**	16-jul-86 (thurston)
**	    Coded, and changed argument list into a control block.
**	25-jul-86 (thurston)
**          Moved the pointer to ADF session control block out of the ADE_EXCB
**          and made it a seperate argument to be consistent with the rest of
**          ADF.
**	04-aug-86 (thurston)
**	    Added code for zero argument f.i.'s.  Also, I added an xDEBUG test
**	    to make sure the function instance ID is a valid one before using
**	    it to reference the f.i. lookup table.  Also, made the value of the
**	    CX always return ADE_NOT_TRUE for error conditions.
**	14-aug-86 (thurston)
**	    Had to add an extra parameter to ADE_GET_OPERAND_MACRO() uses to
**	    satisfy the FRONTEND_AND_PC version.
**	02-sep-86 (thurston)
**	    In ade_execute_cx(), if the function instance is an aggregate, the
**	    low level aggregate routine was being called incorrectly (the last
**	    two args were reversed).  This has been fixed.
**	01-oct-86 (thurston)
**	    Added code to support the ADI_LIKE_OP operation.
**	28-dec-86 (thurston)
**	    Modified the code that performs the ADE_CALCLEN instruction to
**	    use the ADE_LEN_UNKNWON constant.
**	05-jan-87 (thurston)
**	    Commented out the "#ifdef xDEBUG" from around the safety check
**	    which guards against using either a f.i. that is illegal, or any
**	    f.i. this routine doesn't know how to perform but has a NULL pointer
**	    to its routine.  This way, these safety checks will always be
**	    compiled in.
**	28-jan-87 (thurston)
**	    I have added code to better handle warnings
**	    that could have possibly occurred in the middle of executing a CX.
**	    Previously, execution was halted at this point.  Now, execution will
**	    continue, and when it is time to return, either E_DB_OK or E_DB_WARN
**	    will be returned, depending on whether any warnings occurred.  Note
**	    that if more than one warning happened, the error block in the ADF
**	    session control block will only reflect the last one.
**	03-feb-87 (thurston)
**	    Corrected a bug whereby all of the comparison operators between
**	    "i" and "f" were assuming that both inputs were of type "f".  Turned
**	    out that this was a left-over from the 5.0 code, where a "hack" had
**	    been put into PNODERSLV.C that forced coercions of "i"s to "f"s if
**	    one of these mixed comparisons was being done.
**	10-mar-87 (thurston)
**	    Added code to perform the `is null' and `is not null' functions.
**	    Also added code to perform the CX special instructions:
**	    ADE_1CXI_SET_SKIP and ADE_2CXI_SKIP.
**	17-mar-87 (thurston)
**	    Added code to execute the ADE_NAGEND CX instruction, and the
**	    ADE_3CXI_CMP_SET_SKIP instruction.
**	25-mar-87 (thurston)
**	    Made sure instruction offsets are calclulated with reference to
**          ade_excb->excb_bases[ADE_CXBASE] instead of ade_excb->excb_cx, since
**          for frontends, the CX header may not be contiguous with the list of
**          instructions. 
**	05-apr-87 (thurston)
**	    Made the ADE_1CXI_SET_SKIP instruction return error if output is
**	    non-nullable and a null value input is found.
**	08-apr-87 (thurston)
**	    Fix bug where `is null' and `is not null' where always assuming the
**	    datatype was nullable.
**	13-apr-87 (thurston)
**	    Added code that should allow the S/E/A functions to return proper
**	    values.  In the future, however, we must do these differently since,
**	    (1) the way I have it coded right now is a hog, and (2) several of
**	    the S/E/A functions should be ADF constants (i.e. constant for the
**	    life of the query.
**	15-apr-87 (thurston)
**	    Fixed bug in ADE_NAGEND instruction; it was passing nullable
**	    datatype to low-level ag-end function.
**	21-apr-87 (thurston)
**	    Added the ADE_PMQUEL, ADE_NO_PMQUEL and ADE_PMFLIPFLOP instructions.
**	28-apr-87 (thurston)
**	    Fixed ADE_KEYBLD instruction to handle the LIKE operator pattern
**	    match semantics.  Also fixed the ADE_COMPARE instruction to compare
**	    text values properly.
**	29-apr-87 (thurston)
**	    Added the ADE_PMENCODE instruction.
**	29-apr-87 (thurston)
**	    Added specific check to `f ** f' so that it returns an error if
**	    arg 1 is negative, or if arg 1 is zero and arg 2 is non-positive.
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
**	14-may-87 (thurston)
**	    Fixed ADE_NAGEND so that it returns a zero value for `count' and
**	    `count*' when aggregated over an empty set.
**	15-may-87 (thurston)
**	    Got rid of the initializations of automatic arrays that were being
**	    done for the S/E/A functions.  (Unix compiler bitched about it.)
**	05-jun-87 (thurston)
**	    Made fix to the `_version' function ... it was being supplied to
**	    adu_dbmsinfo() as `version'.
**	10-jun-87 (thurston)
**	    Added code to perform the four new function instances:
**		f  :  f * i			f  :  f / i
**		f  :  i * f			f  :  i / f
**	13-jul-87 (thurston)
**          Made fix to the ADE_1CXI_SET_SKIP instruction:  When no null values
**          were found in the inputs, the code had been clearing the null
**          indicator bit in the output ... EVEN IF THE OUTPUT WAS NON-NULLABLE!
**	28-jul-87 (thurston)
**	    Upgraded the ADE_COMPARE instruction to handle nullables and null
**	    values with the semantics necessary to do keyed joins.
**	24-aug-87 (thurston)
**	    Removed the specific check for a bad math arg (see history comment
**	    by me dated 29-apr-87).  Instead, we now depend on the MH routines
**	    to signal MH_BADARG, and adx_handler() to deal with it.
**	16-sep-87 (thurston)
**	    Re-coded the ADE_KEYBLD instruction to do `range key' failure check-
**	    ing instead of `pattern match' failure checking.  The new plan,
**	    talked about at length between OPC (Eric Lundblad) and ADE (Gene
**	    Thurston), and is a lot more consistent with what is truly needed
**	    to keep or throw away query plans.
**	26-oct-87 (thurston)
**	    Added code to execute the new ADE_BYCOMPARE instruction, where NULL
**	    values compare equal to other NULL values.
**	05-dec-87 (thurston)
**	    Fixed the ADE_COMPARE and ADE_BYCOMPARE instructions so they do
**          floating point compares in f4 if EITHER argument is an f4. 
**	16-jan-88 (thurston)
**	    Removed all of the string comparison function instance cases where
**	    the datatypes of the two operands was different.  (See LRC proposal
**	    for new string semantics, 11-jan-88 by Gene Thurston.)
**	05-feb-88 (thurston)
**	    Did MAJOR work on VLTs.  Aparently, these had never been tested
**	    since there were about 20 things wrong with them.  Not the least of
**	    which is that we need VLT `c' and `char' as well as VLT `text' and
**	    `varchar'.  With the removal of all of the function instance cases
**	    for the LRC proposal (see above), and the reordering of the datatype
**	    resolution hierarchy, VLTs are MUCH more common, thus the problems
**	    arose.
**	10-feb-88 (thurston)
**	    Did away with using the AD_BIT_TYPE for the NPI's since it didn't
**	    work for VLUP's VLT's.
**	10-feb-88 (thurston)
**	    Modified code to allow execution of the new VIRGIN segmentof a CX.
**	08-jun-88 (thurston)
**	    Fixed bug with the count(*) aggregate ... all aggregates had been
**	    assuming that there would always be exactly one input argument ...
**	    count(*) has zero.  Fixed by adding a `case' specifically for
**	    ADFI_396_COUNTSTAR.
**	09-jun-88 (thurston)
**	    Added code to handle the new ADE_ESC_CHAR instruction, and modified
**	    the LIKE and NOT LIKE function instances so that the escape
**	    character will be used.
**	21-jun-88 (thurston)
**	    Modified the ADE_KEYBLD instruction so that the escape character
**	    will be used.
**	07-oct-88 (thurston)
**	    Altered the ADE_COMPARE and ADE_BYCOMPARE instructions so that they
**	    fill in the .excb_cmp field with ADE_1LT2, ADE_1EQ2, ADE_1GT2, or
**	    ADE_1UNK2 instead of -1, 0, 1.
**	23-oct-88 (thurston)
**	    Beefed up performance of ADE_MECOPY instruction by using a loop with
**	    I4ASSIGN_MACRO's and a second loop copying u_char's for the
**	    remainder.  This special processing will only take effect if the
**	    size of the data to be copied is < ADE_CUTOFF_LEN_4_MECOPY ...
**	    currently set at 40.
**	09-nov-88 (thurston)
**	    Changed all of the E_AD0FFF_NOT_IMPLEMENTED_YET returns to return
**	    E_AD8999_FUNC_NOT_IMPLEMENTED instead.
**	19-feb-89 (paul)
**	    Enhanced to execute ADE_PNAME and ADE_DBDV instructions.
**	02-may-89 (mikem)
**	    Added cases to handle ISNULL and ISNOTNULL for table_key and 
**	    object_key.
**	11-may-89 (anton)
**	    Added local collation support
**	23-may-89 (jrb)
**	    Changed ADE_COMPARE instr to return three new values: ADE_1ISNULL,
**	    ADE_2ISNULL, and ADE_BOTHNULL instead of ADE_1UNK2.  This was to fix
**	    a problem executing an FSM in QEF.  Also changed ADE_BYCOMPARE to
**	    stop returning ADE_1UNK2 since it shouldn't according to the ADE
**	    design spec.
**      16-may-89 (eric)
**          Added support for the new instruction ADE_TMCVT, the CX version
**          of ADC_TMCVT. In fact, the instruction just calls ADC_TMCVT.
**	07-jun-89 (jrb)
**	    Fix bug introduced by my fix above where ade_bycompare should set
**	    the CX's value to ADE_NOT_TRUE whenever the comparison result is not
**	    equality.
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL.
**	14-jul-89 (jrb)
**	    Added code to perform 23 of the 66 new function instances for
**	    decimal here.  Also set db_prec wherever it was not being set.
**      04-dec-89 (fred)
**	    Added support for large objects.  This includes checking and
**	    providing the size used in a specified base (one per expression)
**	    and the returning of partial completion of an expression -- with
**	    the expectation that this routine will be called again later with
**	    the excb_continuation field unaltered (and, of course, the basic CX
**	    also unaltered).
**	22-jan-90 (jrb)
**	    Alignment fix for VLUPs.
**      14-aug-91 (jrb)
**          Added inbuf and outbuf as aligned buffers for use by dba, username,
**          and _version functions; previously these were unaligned buffers
**          which caused problems on some systems.
**	04-dec-92 (andre)
**	    add support for CURRENT_USER, SESSION_USER, SYSTEM_USER, and
**	    INITIAL_USER
**      21-Dec-1992 (fred)
**          Added support for setting up the workspace necessary for
**          function instances marked with the ADI_F4_WORKSPACE.
**          These are primarily used with large objects, where there
**          is some context area necessary.
**      15-Jul-1993 (fred)
**          Added handling for DB_VBYTE_TYPE VLUP/VLT's.
**	08-Aug-1993 (shailaja)
**	     Unnested comments.
**	27-jan-94 (rickh)
**	    Only emit AD0002_INCOMPLETE code if we're processing a BLOB.
**	    This will  prevent the INCOMPLETE handshake (with QEF/SCF) from
**	    looping infinitely if OPC mis-generates a compiled expression.
**      06-apr-1994 (stevet)
**          Replace the use of sf0 and sf1, which are f4 values,  
**          with f0 and f1, which are f8 values.  Some sf0 and sf1 
**          are used to store f8 values can cause overflow error on
**          VMS. (B62041)
**  	07-Oct-94 (ramra01)
**      If we are dealing with a BLOB(s) always adjust the buffer to REDEEM
**      the BLOB coupons so that if it is the last buffer associated with the
**      BLOB then all the remaining fields can be fetched in that loop
**      (b63487)
**	25-oct-95 (inkdo01)
**	    Various CPU performance enhancements were coded, including:
**		- modified ADE_MECOPY routine which now does fullword
**		moves of aligned operands
**		- modified operand preparation logic at head of 
**		instruction loop
**		- "switch"s which were used to load integer operands
**		of differing lengths have been changed to "if then else"
**		- Quel pattern matching semantics are no longer default
**		- ADE_OR now looks for an ADE_AND or end of expression.
**	08-nov-1995 (wilri01)
**	    Fast string compare for non-DOUBLEBYTE, non-national coalating
**	    sequence SQL CHAR and VARCHAR (CHA & VCH) strings in WHERE clause
**	    and key compares.
**	19-feb-96 (inkdo01)
**	    Refine fast string compare to exclude cases where Quel pattern 
**	    matching may be needed.
**	3-jul-96 (inkdo01)
**	    Added ADE_4CXI_CMP_SET_SKIP operator for support of predicate
**	    function operation types, as well as logic to load PRED_FUNC 
**	    result into CXvalue.
**	9-jan-97 (inkdo01)
**	    Made very small change to ramra01 blob fix, above, to compute 
**	    intrinsic result col lens in ALL cases (not just during 
**	    continuation processing).
**	27-mar-97 (inkdo01)
**	    Add support of ANDL/ORL operators - the branching AND/OR which 
**	    are used in non-CNF Boolean expressions (which need to branch 
**	    around through the expression).
**	10-sep-97 (i4jo01)
**	    Alteration of float4 to float8 comparison. Convert float8s	
**	    to 4 byte floats to avoid expansion errors which occur
**	    converting float4 to float8s. 
**	26-Mar-98 (shust01)
**	    Added additional I1_CHECK_MACRO statements (by do_adccompare
**	    label, in DB_INT_TYPE case). Needed for platforms (such as AIX)
**	    that only has unsigned chars.  Problem when joining tables by
**	    integer1 fields when some of the data is negative. Bug 45988.
**	16-jul-99 (thaju02)
**	    changed check of lbase_size from <= to <.
**	    (b98103)
**      07-Nov-2001 (horda03)
**          Above change 10-sep-97 only handled the F4 = F8 comparison,
**          this leads to incorrect behavior when performing other type
**          of F4 to F8 comparisons. Extended change to all other cases.
**          (b85284).
**	8-sep-99 (inkdo01)
**	    Added ADE_BRANCH, ADE_BRANCHF operators to do branching for 
**	    case functions.
**	8-sep-00 (inkdo01)
**	    Change BRANCHF to be effectively branch on false or unknown, to
**	    handle nulls in case expressions.
**	24-nov-00 (inkdo01)
**	    Add support for ADE_MECOPYN and ADE_5CXI_CLR_SKIP.
**	15-dec-00 (inkdo01)
**	    New case's for inline aggregate computation.
**	3-jan-01 (inkdo01)
**	    Add ADE_NULLCHECK to assure agg results are ok.
**	9-jan-01 (inkdo01)
**	    Added ADE_OLAP, ADE_MECOPYQA, ADE_AVGQ for inline aggregate 
**	    processing.
**	15-jan-01 (inkdo01)
**	    General cleanup/consolidation. Remove superfluous case's,
**	    ADE_AGBGN, ADE_AGEND, ADE_NAGEND.
**	13-mar-2001 (stial01)
**	    case ADFI_500_MAX_TABKEY, adjust dv db_length, db_datatype 
**	19-mar-01 (inkdo01)
**	    Fix to SUM_M (sum(money)) to fix US1132 error. Also applied stial01 
**	    fix to min(log. key), too.
**	21-mar-01 (inkdo01)
**	    Fix sum, avg(float) to properly coerce f8 to f4.
**	9-apr-01 (inkdo01)
**	    Add ADE_UNORM to normalize Unicode input strings.
**	21-june-01 (inkdo01)
**	    Update binary aggregate support, re-adding ADE_AGBGN and ADE_AGEND
**	    in the process (still does func calls for these).
**	30-aug-01 (inkdo01)
**	    The long saga of changing aggregates continues - since FRS still
**	    does it the old fashioned way, the binary aggregate versions of 
**	    AGBGN and AGEND are renamed OLAGBGN and OLAGEND, respectively. The
**	    original AGBGN AND NAGEND are restored and AGNEXT is added to 
**	    execute the "next" functions of aggregation (all for FRS only).
**	15-dec-2001 (devjo01)
**	    Add support for NVCHR & NCHR comparisons.  Combine temp buffers
**	    for "inbuf", "outbuf", and decimal types, into a union to
**	    allow largish buffers for the various disjoint operations
**	    without increasing the overall stack requirements.
**      22-Feb-02 (zhahu02)
**          Updated case ADE_BRANCHF (b107195/ingsrv1706).
**	4-mar-02 (inkdo01)
**	    Move Unicode comparison f.i.'s out of DOUBLEBYTE if[n]def's.
**	5-mar-02 (inkdo01)
**	    Fix above properly - moved cases needed to be interleaved with 
**	    DOUBLEBYTE logic.
**      10-Apr-02 (zhahu02)
**          Updated for abs(int1()),etc for integer types. (b107557/ingsrv1753).
**      10-may-02 (inkdo01)
**          Fix ADE_UNORM to decrement lengths for nullable operands.
**	13-may-02 (inkdo01)
**	    Don't move null ind. in max/min date code.
**	31-Jan-2003 (jenjo02)
**	    If ADE_UNORM returns an error, that's ok, not an
**	    "internal error".
**	18-nov-03 (inkdo01)
**	    If adf_base_array is non-null, use it as base array for 
**	    operand addr resolution, not excb_bases[].
**	23-dec-03 (inkdo01)
**	    Add int to int coercion (used to be in adu!?) as test case
**	    for bigint introduction, plus various other inline ops.
**	24-mar-04 (inkdo01)
**	    Add more bigint support - all (?) integer arithmetic/comparison
**	    F.I.'s.
**	    (schka24) typo fix (ba->bo), also it turns out that it's best
**	    to simply do (most) int arithmetic in i8 rather than jumping
**	    around.
**	24-mar-04 (inkdo01)
**	    i8 support for MAX incorrectly overwrote cx_value.
**	30-mar-04 (inkdo01)
**	    Add bigint support for ADE_COMPARE, BYCOMPARE and fix MIN(int).
**	31-mar-04 (inkdo01)
**	    Minor fix to MAX(int).
**	15-apr-04 (inkdo01)
**	    Fix SIGBUS on 64 bit machines executing sum(int).
**	19-apr-04 (inkdo01)
**	    Fix MAX/SUM_I - bigint rewrites neglected nullable operands.
**	19-apr-04 (inkdo01)
**	    Fix moronic change to SUM(i) just made.
**	23-july-04 (inkdo01)
**	    Minor fixes to stddev_samp, var_samp.
**	25-aug-04 (inkdo01)
**	    Pick up global base array addr from excb_bases[ADE_GLOBALBASE].
**	27-aug-04 (inkdo01 & stephenb)
**	    Merge double/single byte logic.
**	17-sep-04 (inkdo01)
**	    Fix Unicode, text compares after double byte changes.
**	5-nov-04 (inkdo01)
**	    Add support for ADE_NOOP (no-op).
**	19-Nov-2004 (jenjo02)
**	    Add ADE_BACKEND wrapper around ADE_GLOBALBASE check
**	    to silence a myriad of front-end bus/segvs.
**	16-dec-04 (inkdo01)
**	    Add collation ID parm to aduucmp() call.
**	13-Jan-2005 (jenjo02)
**	    Changed BACKEND version of ADE_GET_OPERAND_MACRO
**	    to return pointer to compiled read-only ADE_I_OPERAND
**	    rather than copying it to the stack. Quantify
**	    showed that 27% of the in time here was spent copying
**	    the operands.
**	14-jan-05 (inkdo01)
**	    Fix the fix to 113580 for bad stddev funcs. Apparently, abs() 
**	    produces an integer result when we needed float.
**	17-feb-05 (inkdo01)
**	    Fix dv[i] assignments for "other" FI's (fix bug 113914).
**	17-Mar-2005 (schka24)
**	    Combine nearly-identical dbmsinfo, query constant cases.
**	28-Mar-2005 (schka24)
**	    Implement 6CXI, 7CXI.
**      8-apr-05 (hayke02)
**        Set excb_nullskip to 1 for ADE_5CXI_CLR_SKIP if we find a NULL.
**        This matches the logic for ADE_2CXI_SKIP. This change fixes bug
**        114091, problem INGSRV 3207.
**	23-Jun-2005 (schka24)
**	    Fix error in 7CXI, wasn't checking operand 0.
**	    Reinstate op0 nullability-check for front-end only, in case we
**	    run into any CX's compiled prior to the 6CXI/7CXI change.
**	    Don't deference result oprP for blob offset check (at end of
**	    big loop) if operation has no operands (causes segv).
**	25-Jul-2005 (schka24)
**	    Implement ADE_SETTRUE to reset CX result so that agg driver
**	    isn't confused by CX true-ness changes caused by CASE expr
**	    inside an agg.
**	29-Aug-2005 (schka24)
**	    Fix abs in wrong place in operand setup loop (no symptoms seen).
**	    Avoid some blob/vlt stuff if "fancy" flag isn't set.
**	    Do a wee bit of code bumming whilst in here, to make float
**	    operations more SPARC-branch-friendly.
**	6-Apr-2006 (kschendel)
**	    Implement COUNTVEC style function call (cb, count, dvp[]).
**	17-Oct-2006 (kiria01) b116858
**	    Pass an indication to adu_ulike whether we are in SQL or Quel
**	    context. For Quel we will set the escape parameter to null,
**	    for SQL it will point to either the chosen escape char or
**	    to a U_NULL if no escape char is to be used.
**	31-Oct-2006 (gupsh01)
**	    Fixed the aggregate calculations for date functions.
**	20-Apr-2007 (kiria01) b118105
**	    Ensure C datatype always in lexcomp for ignoring spaces.
**	3-may-2007 (dougi - with assistance from kiria01)
**	    Rewrite adccompare and strCompare to use MEcmp and be more
**	    efficient.
**	4-may-2007 (dougi)
**	    Add F.I.s for tan(), atan2(), acos(), asin(), pi() and sign().
**	8-may-2007 (dougi)
**	    Add logic to process comparisons of c/text/char/varchar operands
**	    in UTF8 server.
**	09-May-2007 (gupsh01)
**	    Added nfc_flag to adu_unorm to force normalization to NFC
**	    for UTF8 character sets.
**	14-May-2007 (gupsh01)
**	    Call adu_utf8_unorm for non unicode input datatypes. 
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
**	21-aug-2007 (gupsh01)
**	    Depricate 246_C_LIKE_C and 247_C_LIKE_CHAR etc in favor of 
**	    ADFI_1316_C_LIKE_VARCHAR and ADFI_1317_CHAR_LIKE_VARCHAR.
**	27-aug-2007 (gupsh01)
**	    Remove C_LIKE_VARCHAR and CHAR_LIKE_VARCHAR as it causes
**	    ambiguity with text_like_varchar case.
**	03-Nov-2007 (kiria01) b119410
**	    Add support for 'numeric' string comparisons.
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**      21-Feb-2008 (hanal04) Bug 119937
**          Use same code for UDF aggregates as UDT aggregates. Note
**          that a UDF aggregate function may be using non-UDTs.
**	11-Apr-2008 (wanfr01) Bug 120251
**	    Do not restrict blob operator length to 32767 until after
**	    you have adjusted the length.
**	16-Apr-2008 (kschendel)
**	    Count up any-warnings when counting warnings.
**	02-Jun-2008 (kiria01) SIR120473
**	    Added ALL-ALL varieties of LIKE/ILIKE
**	01-Oct-2008 (gupsh01)
**	    In a UTF8 installation make sure that we check that the char
**	    varchar strings are not longer than half of maximum string 
**	    length allowed to account for expansion in conversion to 
**	    nchar/nvarchar for internal processing.
**	12-Oct-2008 (kiria01) b121011
**	    Added ADE_SETFALSE and corrected the symbol names for return
**	    state to match docs.
**	27-Nov-2008 (kiria01) b121289
**	    Cure SEGV in adi_0calclen() due to uninitialised data.
**	06-Jan-2009 (kiria01) SIR120473
**	    Handle the escape char as a proper DBV so that Unicode endian
**	    issues are avoided.
**       1-May-2009 (hanal04) Bug 122002
**          Fix ADFI_109_SUM_I logic which would allow an i8 input to
**          write an i8 value to an i4 result.
**      23-Jun-2009 (hanal04) Bug 122206
**          If ADFI_464_AVG_DATE is dealing with interval ingres dates
**          set AD_DN_LENGTH in the accumulator so that it is picked up
**          in the result value under ADE_AVG.
**      29-Jul-2009 (hanal04) Bug 122368
**          Add DB_NCHR_TYPE to ensure the correct data length is calculated.
**          The default does not divide by sizeof(UCS2) and this allowed
**          ADE to think we had double the space allocated, leading to
**          ULM memory over-run and spurious exceptions.
**      26-Aug-2009 (hanal04) Bug 122400
**          NCHR VLUP processing needs to account for the sizeof(UCS2)
**          multiplier AND the prepended u_i2 length that gets added
**          as part of VLUP processing. Add/correct NCHR processing
**          to avoid treating NCHR like NVCHR or CHR.
**      03-Sep-2009 (coomi01) b122483
**          Abs(float) corrected to be careful about "negative" zero.
**	01-Nov-2009 (kiria01) b122822
**	    Added support for ADE_COMPAREN & ADE_NCOMPAREN instructions
**	    for big IN lists.
**      17-dec-2009 (huazh01) 
**          at the end of the while loop, don't update offset_accumulator 
**          if it is either ADE_COMPAREN or ADE_NCOMPAREN. oprP[] won't 
**          be set up, and random values of oprP[] may cause segv. 
**          (b123074)
**       8-Mar-2010 (hanal04) Bug 122974
**          Removed nfc_flag from adu_nvchr_fromutf8() and adu_unorm().
**          A standard interface is expected by fcn lookup / execute
**          operations. Force NFC normalization is now achieved by temporarily
**          updating the adf_uninorm_flag in the ADF_CB.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
**	14-Jul-2010 (kschendel) b123104
**	    Replace settrue/false with ii_true and ii_false.  Same execution,
**	    different FI numbers.
**	28-Jul-2010 (kiria01) b124142
**	    Split aggregate SINGLETON function into two deferring the error
**	    check into SINGLECHK. This allows the aggregate to summarise the
**	    record state whilst in derived tables and hence not cause errors
**	    to be raised with records that will not be part of the result set.
**	    The method of flagging the potential error is to use ADF_SING_BIT
**	    alongside ADF_NVL_BIT to be picked up by SINGLECHK.
**	    Tighten the access to ADF_NVL_BIT.
**       7-Sep-2010 (hanal04) Bug 124384
**          Add missing break to NCHR ADE_LEN_UNKNOWN case.
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. Don't allow UTF8 strings with it
**	    to use UCS2 CEs for comparison related actions.
*/


/* Quickie routine to signal floating point overflow */
static void fltovf() {EXsignal(EXFLTOVF, 0);}


DB_STATUS
ade_execute_cx(
ADF_CB             *adf_scb,
ADE_EXCB           *ade_excb)
{
    i4			cx_value = ADE_TRUE;
    i4			pm_quel;
    DB_DATA_VALUE	esc_char;
    DB_DATA_VALUE	*esc_ptr = NULL;
    u_i2		pat_flags = 0;
    i4			*dcmp;
    i4			cmp;
    i4			i;
    i4                  rangekey_flag;
    u_char              *null_byte;
    i4             next_instr_offset;
    i4             last_instr_offset;
    i4		this_instr_offset;
    i4			seg;
    DB_STATUS		final_stat = E_DB_OK;
    DB_STATUS		db_stat;
    DB_STATUS		(*func)();
    i4			bdt;
    i4                  bytes_needed;
    i4                  calced_len;
    i4			a0;
    i4			a1;
    i8		ba0;
    i8		ba1;
    bool		bo0;
    bool		bo1;
    bool		is_fancy;
    i4			tempi;
    i8		tempbi;
    f8			f0;
    f8			f1;
    f8			tempf, tempf1;
    f4			f4_0;
    f4			f4_1;
    AD_NEWDTNTRNL 	dnag1; /* Date aggregate variables */
    AD_NEWDTNTRNL 	dnag2; /* Date aggregate variables */

    /* declare aligned buffers for dba, username, session_user, system_user,
    ** initial_user, and _version functions; inbuf must be 12 + DB_CNTSIZE + 1
    ** bytes long (to hold "..session_user" which is 15 bytes) and outbuf must
    ** be 32 + DB_CNTSIZE bytes long; the following declarations guarantee this
    */
    union aligned_data {
      u_char		dec[DB_MAX_DECLEN];
      ALIGN_RESTRICT	inbuf[(13 + DB_CNTSIZE) / sizeof(ALIGN_RESTRICT) + 1];
      ALIGN_RESTRICT	outbuf[(31 + DB_CNTSIZE) / sizeof(ALIGN_RESTRICT) + 1];
    }			tempd1, tempd2;
    i4			tprec1;
    i4			tscale1;
    i4			tprec2;
    i4			tscale2;
    ADE_CXHEAD          *cxhead = (ADE_CXHEAD *) ade_excb->excb_cx;
    PTR                 cxbase = ade_excb->excb_bases[ADE_CXBASE];
    ADE_CXSEG		*cxseg;
    PTR			*base_array;
    ADE_INSTRUCTION *f;
    ADE_I_OPERAND       *opr_internal;
    /* General note on size, ADI_MAX_OPERANDS+1 is enough if it's at least 5.
    ** ADI_MAX_OPERANDS already includes the result, may need +1 for workspace.
    */
    ADE_OPERAND     oprs[ADI_MAX_OPERANDS+3];    /* +3 because result and work*/
    ADE_OPERAND		*oprP[ADI_MAX_OPERANDS+3];
    PTR         data[ADI_MAX_OPERANDS];    /* Must be at least 5 */
    DB_DATA_VALUE	dv[ADI_MAX_OPERANDS+2]; /* May need 1 res 1 workspace */
    DB_DATA_VALUE	*opptrs[ADI_MAX_OPERANDS];
    ADI_FI_DESC		*fdesc;
    ADF_AG_STRUCT       *agstruct_ptr;
    ADF_AG_OLAP		*olapstruct_ptr;
    ADC_KEY_BLK         kblk;
    ADI_LENSPEC         lenspec;
    PTR                 vltptr = NULL;
    ADI_OP_ID		key_op;

    i4		lbase_size;
    i4		old_lbase_size;
    i4			lbase;
    i4		lo_correction;	/* Large Object Correction */
    i4		offset_accumulator;
    i4			element_length = 0;
    i4			strOp;		/* 1=LT, 2=EQ, 4=GT */
    u_char		*c1;
    u_char		*c2;
    u_i4		 ln1;
    u_i4		 ln2;
    i4			diff;

    ade_excb->excb_value = ADE_NOT_TRUE;    /* Set to NOT_TRUE in case of err */
    ade_excb->excb_nullskip = 0;	    /* Init to no NULL values skipped */

#ifdef    xDEBUG
    if (    cxhead->cx_hi_version != ADE_CXHIVERSION
        ||  cxhead->cx_lo_version != ADE_CXLOVERSION
       )
	return(adu_error(adf_scb, E_AD550D_WRONG_CX_VERSION, 0));
#endif /* xDEBUG */

#ifdef    ADE_BACKEND
    /* Copy a couple of local CX addresses to the global array, if it is
    ** being used. Those entries in the global array have been reserved
    ** for the purpose. 
    */
    if ((base_array = (PTR *)ade_excb->excb_bases[ADE_GLOBALBASE]) != NULL)
    {
	base_array[ADE_CXBASE] = ade_excb->excb_bases[ADE_CXBASE];
	base_array[ADE_VLTBASE] = ade_excb->excb_bases[ADE_VLTBASE];
	base_array[ADE_NULLBASE] = NULL;
	base_array[ADE_LABBASE] = ade_excb->excb_bases[ADE_LABBASE];
    }
    else 
#endif /* ADE_BACKEND */
	base_array = ade_excb->excb_bases;

    seg = ade_excb->excb_seg;
    cxseg = &cxhead->cx_seg[seg];
    if (seg == ADE_SMAIN)
    {
	if (ade_excb->excb_continuation)
	{
	    next_instr_offset = ade_excb->excb_continuation;
	}
	else
	{
	    next_instr_offset = cxseg->seg_1st_offset;
	}
    }
    else  /* if (seg != ADE_SMAIN) */
    {
	/* Initialize the field in case the user doesn't */
	ade_excb->excb_continuation = 0;
	ade_excb->excb_limited_base = ADE_NOBASE;
	ade_excb->excb_size = 0;
	next_instr_offset = cxseg->seg_1st_offset;
    }
    last_instr_offset = cxseg->seg_last_offset;
#ifdef ADE_BACKEND
    pm_quel = (cxseg->seg_flags & ADE_CXSEG_PMQUEL) != 0;
    is_fancy = (cxseg->seg_flags & ADE_CXSEG_FANCY) != 0;
#else
    pm_quel = TRUE;
    is_fancy = TRUE;
#endif

    lbase_size = ade_excb->excb_size;	    /* obtain the original size */
    lbase = ade_excb->excb_limited_base;    /* of the limited base */
    lo_correction = 0;			    /* No correction to make yet */
    offset_accumulator = 0;


/* {@fix_me@} */
#ifdef    ADE_BACKEND
    EXmath((i4) EX_ON);
#endif /* ADE_BACKEND */

    while (next_instr_offset != ADE_END_OFFSET_LIST)
    {
        f = (ADE_INSTRUCTION *) ((char *)cxbase + next_instr_offset);

#ifdef COUNTS
#ifdef ADE_BACKEND
if (f->ins_icode >= 0) adfi_counts[f->ins_icode]++;
 else if (f->ins_icode <= -1001 && f->ins_icode >= -1022)
	adfi_counts[-f->ins_icode]++;
 else if (f->ins_icode <= -2000 && f->ins_icode >= -2003)
	adfi_counts[-f->ins_icode-977]++;
#endif
#endif
	this_instr_offset = next_instr_offset;

	/*
	**	Compute next instruction so that we know where to skip to in
	**	case of NULL data values.
	*/
	
	next_instr_offset = 
	    ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);

	/* Removed early PMQUEL test, we now try to avoid emitting it
	** at all.
	*/

	/* This loop sets up all the needed operands and */
	/* pointers to the oprs data into local arrays   */
	/* --------------------------------------------- */
	opr_internal = (ADE_I_OPERAND *) ((char *)f + sizeof(ADE_INSTRUCTION));
	old_lbase_size = 0;

	if (f->ins_icode == ADE_COMPAREN ||
	    f->ins_icode == ADE_NCOMPAREN)
	{
	    /*
	    ** True VARARGS instructions - skip general arg processing
	    ** as it is artificially bounded.
	    */

	    /* Do nothing */;
	}
	else if (!is_fancy)
	{
	    /* This loop for the normal (backend only!) case */
	    for (i=0; i < f->ins_nops; i++)
	    {
		/*
		** On the BACKEND, this simply sets oprP[i] to 
		** read-only opr_internal.
		*/
		oprP[i] = (ADE_OPERAND *) opr_internal;
		data[i] = (PTR) ((char *) base_array[oprP[i]->opr_base]
						   + oprP[i]->opr_offset);
		opr_internal++;
	    }
	}
	else
	{
	    for (i=0; i < f->ins_nops; i++)
	    {
		/*
		** On the BACKEND, this simply sets oprP[i] to 
		** read-only opr_internal.
		** On the FRONTEND, the operand is constructed in oprs[i]
		** and oprP[i] points to it, and it's writable.
		*/
		ADE_GET_OPERAND_MACRO(base_array, opr_internal, &oprs[i], &oprP[i]);
		    
#ifdef    ADE_FRONTEND
		if (    f->ins_icode >= 0
		    &&  f->ins_icode <= Adf_globs->Adi_fi_biggest
		    &&
		    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(f->ins_icode)].adi_fi
				!= NULL
		   )
		{
		    register ADI_FI_DESC *fdesc;

		    fdesc = (ADI_FI_DESC*)Adf_globs->
			    Adi_fi_lkup[ADI_LK_MAP_MACRO(f->ins_icode)].adi_fi;
		    if (	fdesc->adi_npre_instr != ADE_0CXI_IGNORE

			&&	oprP[i]->opr_dt < 0
		       )
		    { /* un-null nullable operand */
			oprP[i]->opr_dt = -oprP[i]->opr_dt;
			oprP[i]->opr_len -= 1;
		    }
		}
#endif /* ADE_FRONTEND */
#ifdef    xDEBUG
		/* Assure that we won't try using a base that was not supplied */
		if (    (f->ins_icode != ADE_KEYBLD  ||  i != 4)
		    &&  oprP[i]->opr_base >= ade_excb->excb_nbases
		   )
		    return(adu_error(adf_scb, E_AD550B_TOO_FEW_BASES, 0));
#endif /* xDEBUG */
		if ((!ade_excb->excb_continuation) || (i != 0))
		    data[i] = (PTR) ((char *) base_array[oprP[i]->opr_base]
						       + oprP[i]->opr_offset);
		else
		    data[i] = (PTR) ((char *) base_array[oprP[i]->opr_base]);
		/* Get the real length from the data for VLUPs and VLTs */

		if (    oprP[i]->opr_len == ADE_LEN_UNKNOWN
		    &&  f->ins_icode != ADE_CALCLEN
		   )
		{
		    u_i2		c;

		    /* We need a writable copy of the opr */
		    if ( oprP[i] != &oprs[i] )
		    {
			STRUCT_ASSIGN_MACRO(*oprP[i], oprs[i]);
			oprP[i] = &oprs[i];
		    }

		    oprP[i]->opr_prec = 0;
		    
		    switch (abs(oprP[i]->opr_dt))
		    {
		      case DB_VCH_TYPE:
		      case DB_TXT_TYPE:
		      case DB_LTXT_TYPE:
		      case DB_VBYTE_TYPE:
			/* The variable length types */
			I2ASSIGN_MACRO(((DB_TEXT_STRING *)data[i])->db_t_count, c);
			oprP[i]->opr_len = c + DB_CNTSIZE;
			break;

		      case DB_NVCHR_TYPE:
			/* unicode variable string */
			I2ASSIGN_MACRO(((DB_NVCHR_STRING *)data[i])->count, c);
			oprP[i]->opr_len = c * sizeof(UCS2) + DB_CNTSIZE;
			break;

		      case DB_NCHR_TYPE:
			/* unicode fixed length string */
			I2ASSIGN_MACRO(*(u_i2 *)((char *)data[i] - DB_CNTSIZE), c);
			oprP[i]->opr_len = c * sizeof(UCS2);
			break;

		      default:
			/* all other types */
			I2ASSIGN_MACRO(*(u_i2 *)((char *)data[i] - DB_CNTSIZE), c);
			oprP[i]->opr_len = c;
			break;
		    }
		    if (oprP[i]->opr_dt < 0)	    /* if nullable */
		    {
			oprP[i]->opr_len++;
		    }
		}
		    
		if (oprP[i]->opr_base == lbase)
		{
		    if (oprP[i]->opr_offset == ADE_LEN_UNKNOWN)
		    {
			lo_correction += offset_accumulator;
			offset_accumulator = 0;
			data[i] = (PTR) (
			    (char *) base_array[oprP[i]->opr_base]
					    + lo_correction);
		    }
		    else
		    {
			data[i] = (PTR) ((char *) data[i] + lo_correction);
		    }
		    element_length = oprP[i]->opr_len;
		    if ((i == 0) && (oprP[i]->opr_len == ADE_LEN_LONG))
		    {
			/*
			**  If this is a peripheral datatype, and, therefore,
			**  of unknown length at this point, set the output length
			**  to what remains in the base.  Once the function is complete,
			**  we will adjust back to what is really needed.
			*/

			/* We need a writable copy of the opr */
			if ( oprP[i] != &oprs[i] )
			{
			    STRUCT_ASSIGN_MACRO(*oprP[i], oprs[i]);
			    oprP[i] = &oprs[i];
			}
		    
			oprP[i]->opr_len = old_lbase_size = lbase_size;

			if (lbase_size < 0)
			   oprP[i]->opr_len = 1;    /* Force incomplete status below */
		    }

		    if (    (i == 0)	    /* this is a result */
			    &&  (abs(oprP[i]->opr_dt) != DB_BOO_TYPE))	
			    /* which is not a comparison */
			    /* and we care about this one */
		    {
			if ((lbase_size - oprP[i]->opr_len) < 0) /* And it won't fit! */
			{
			/*
			** This is a sanity check to prevent us from mistakenly
			** starting the BLOB handshake with QEF/SCF and
			** looping infinitely.  We only emit E_AD0002_INCOMPLETE
			** if the operator is a BLOB.  Otherwise, we confess
			** an OPC code generation error.
			*/

			    if ( oprP[i]->opr_len != ADE_LEN_LONG )
			    { return(adu_error(adf_scb, E_AD2005_BAD_DTLEN, 0)); }

			    ade_excb->excb_continuation = this_instr_offset;
			    if (lbase_size > 0)
				ade_excb->excb_size -= lbase_size;
			    return(adu_error(adf_scb, E_AD0002_INCOMPLETE, 0));
			}
			else
			{
			    lbase_size -= oprP[i]->opr_len;
			}
		    }
		}

		opr_internal++;
	    }
	} /* if fancy */

        switch(f->ins_icode)
        {
	  /* CX SPECIAL INSTRUCTIONS */
	  /* ----------------------- */

          case ADE_NOT:
	    if      (cx_value == ADE_TRUE) cx_value = ADE_NOT_TRUE;
	    else if (cx_value == ADE_NOT_TRUE) cx_value = ADE_TRUE;
	    break;

	  case ADE_ANDL:	/* labelled version of AND */
	    /* Labelled AND takes branch if cx_value is FALSE, else 
	    ** keep going. */
	    if (cx_value != ADE_TRUE)
	    {
		next_instr_offset = (i4)(SCALARP)data[0];
					/* operand[0] is false label */
		if (next_instr_offset > last_instr_offset)
		{
		    /* FALSE and at end of expr means we're done. */
		    next_instr_offset = ADE_END_OFFSET_LIST;
		    ade_excb->excb_value = ADE_NOT_TRUE;
		    return(final_stat);
		}
	    }
	    break;

          case ADE_AND:		/* unlabelled AND */
	    /* Unlabelled AND assumes expression is in conjunctive normal 
	    ** form (classic Ingres). As such, cx_value being ADE_NOT_TRUE means 
	    ** the whole expression is FALSE (ANDs are never nested under 
	    ** ORs). */
            if (cx_value != ADE_TRUE)
            {       
                /* First clause is not true -- let's return */
/* {@fix_me@} */
#ifdef    ADE_BACKEND
                EXmath((i4) EX_OFF);
#endif /* ADE_BACKEND */
		ade_excb->excb_value = ADE_NOT_TRUE;
                return(final_stat);
            }
            break;

	  case ADE_ORL:		/* labelled OR */
	    /* Labelled OR takes branch if cx_value is ADE_TRUE, else keep going */
	    if (cx_value == ADE_TRUE)
	    {
		next_instr_offset = (i4)(SCALARP)data[0];
					/* operand[0] is true label */
		if (next_instr_offset > last_instr_offset)
			next_instr_offset = ADE_END_OFFSET_LIST;
					/* assure graceful exit from big loop */
	    }
	    break;

          case ADE_OR:		/* unlabelled OR */
            if (cx_value == ADE_TRUE)
            {
		/*
                ** First clause is true, no reason to interpret second clause
		** So, skip to the instruction following the next AND. This
		** logic is based on the assumption of conjunctive normal
		** form (classic Ingres). Any true OR triggers jump to next
		** AND. */
                while (f->ins_icode != ADE_AND 
			&& next_instr_offset != ADE_END_OFFSET_LIST)
		{
		    f = (ADE_INSTRUCTION *)((char *)cxbase + next_instr_offset);
		    next_instr_offset = 
			ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);
		}
            }
            break;

	  case ADE_BRANCH:	/* unconditional branch (for case function) */
	    /* Unconditional branch simply sets next instruction offset. */
	    next_instr_offset = (i4)(SCALARP)data[0];
	    if (next_instr_offset > last_instr_offset)
		next_instr_offset = ADE_END_OFFSET_LIST;
					/* assure graceful exit from big loop */
	    break;

	  case ADE_BRANCHF:	/* branch if false (for case function) */
	    /* Test if FALSE and set next offset. */
	    if (cx_value == ADE_NOT_TRUE || cx_value == ADE_UNKNOWN)
	    {
		next_instr_offset = (i4)(SCALARP)data[0];
		if (next_instr_offset > last_instr_offset)
		    next_instr_offset = ADE_END_OFFSET_LIST;
					/* assure graceful exit from big loop */
	    }
            cx_value = ADE_TRUE;
	    break;

          case ADE_1CXI_SET_SKIP:   /* If null found, set the null bit in the
				    ** null byte pointed to by operand 0, then
				    ** skip next instruction.  If no nulls, then
				    ** clear the null bit and proceed to next
				    ** instruction.
				    */
	  case ADE_5CXI_CLR_SKIP:   /* Same as 1CXI, except we don't set the 
				    ** null byte, only clear it. Used in 
				    ** aggregates. */
			/* In both cases, we know that both some input
			** as well as the output is nullable, else we
			** would be doing 6CXI or 7CXI instead.
			** For front-end ONLY, check the result operand
			** for nullability (compatability with existing CX's).
			*/
	    {
	        i4		no_nulls = TRUE;
	        for (i=1; i < f->ins_nops; i++)	/* assume oprP[0] is output */
	        {
		    null_byte = (u_char *)data[i] + oprP[i]->opr_len - 1;
		    if (*null_byte & ADF_NVL_BIT)
		    {
		        no_nulls = FALSE;
			if (f->ins_icode == ADE_1CXI_SET_SKIP)
			{
#ifdef ADE_FRONTEND
			  if (oprP[0]->opr_dt < 0)
#endif
			    /* Set NULL bit & implicitly clear SING bit */
			    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
			}
			else /* 5CXI_CLR_SKIP */
			    ade_excb->excb_nullskip = 1; 
		        f = (ADE_INSTRUCTION *)((char *)cxbase
						+ next_instr_offset);
		        next_instr_offset =
			    ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);
		        break;
		    }
	        }
	        if (no_nulls)
		{   /* Clear null bit if no null values found in inputs.
		    ** Output is known to be nullable else we don't get here.
		    ** (Check anyway for front-end for compatability.)
		    */

#ifdef ADE_FRONTEND
		  if (oprP[0]->opr_dt < 0)
#endif
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
                }
		break;
	    }

          case ADE_3CXI_CMP_SET_SKIP:	/* If null found, set the CX value
				    ** to UNKNOWN, then skip next instruction.
				    ** If no nulls, then proceed to next
				    ** instruction.
				    */
	    {
	        for (i=0; i < f->ins_nops; i++)	/* No output operand here */
	        {
		    null_byte = (u_char *)data[i] + oprP[i]->opr_len - 1;
		    if (*null_byte & ADF_NVL_BIT)
		    {
		        cx_value = ADE_UNKNOWN;
		        f = (ADE_INSTRUCTION *)((char *)cxbase
						+ next_instr_offset);
		        next_instr_offset =
			    ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);
		        break;
		    }
	        }
                break;
	    }

          case ADE_4CXI_CMP_SET_SKIP: /* If null found, set the null bit in the
				    ** null byte pointed to by operand 0, then
				    ** set the CX value to UNKNOWN and skip
				    ** the next instruction.  If no nulls, then
				    ** clear the null bit and proceed to next
				    ** instruction.
				    ** NOTE: this is a combination of the 1CXI
				    ** and 3CXI instructions initially intended
				    ** for use by predicate functions which can
				    ** be coded using infix or function syntax.
				    */
	    {
	        i4		no_nulls = TRUE;
	        for (i=1; i < f->ins_nops; i++)	/* assume oprP[0] is output */
	        {
		    null_byte = (u_char *)data[i] + oprP[i]->opr_len - 1;
		    if (*null_byte & ADF_NVL_BIT)
		    {
			/* op0 is always nullable, else we would have generated
			** a 7CXI_CKONLY instead.
			** For front-end compatability with existing CX's,
			** check op0 nullability for front-end only.
			*/
			cx_value = ADE_UNKNOWN;
		        no_nulls = FALSE;
#ifdef ADE_FRONTEND
			if (oprP[0]->opr_dt < 0)
#endif
			    /* Set NULL bit & implicitly clear SING bit */
			    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		        f = (ADE_INSTRUCTION *)((char *)cxbase
						+ next_instr_offset);
		        next_instr_offset =
			    ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);
		        break;
		    }
	        }
	        if (no_nulls)
		{   /* Clear null bit if no null values found in inputs.
		    ** Output must be nullable, else we wouldn't be here.
		    */

#ifdef ADE_FRONTEND
		    if (oprP[0]->opr_dt < 0)
#endif
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
                }
		break;
	    }

          case ADE_2CXI_SKIP:	    /* If null found, skip next instruction */
	    for (i=0; i < f->ins_nops; i++)	/* No output operand here */
	    {
		null_byte = (u_char *)data[i] + oprP[i]->opr_len - 1;
		if (*null_byte & ADF_NVL_BIT)
		{
		    ade_excb->excb_nullskip = 1;
		    f = (ADE_INSTRUCTION *)((char *)cxbase + next_instr_offset);
		    next_instr_offset =
			ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);
		    break;
		}
	    }
            break;

	  case ADE_6CXI_CLR_OP0NULL:
	    /* This operator says unconditionally clear the nullable
	    ** indicator in operand 0.  It's used when the result is nullable
	    ** but none of the operands are, so there's no point in testing
	    ** operands for nullability.
	    */
	    *(u_char *)(data[0] + oprP[0]->opr_len - 1) = 0;
	    break;

	  case ADE_7CXI_CKONLY:
	    /* This operator says to check the inputs for null, if there are
	    ** any then issue an error.  It's used in place of 1CXI, 4CXI, or
	    ** 5CXI when an input is nullable but the output isn't.
	    ** The operands are all the nullable inputs.
	    */
	    for (i=0; i < f->ins_nops; i++)
	    {
		null_byte = (u_char *)data[i] + oprP[i]->opr_len - 1;
		if (*null_byte & ADF_NVL_BIT)
		{
		    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
		}
	    }
	    break;

          case ADE_MECOPY:
          case ADE_MECOPYN:
          case ADE_MECOPYQA:
		/* MECOPYN/QA are identical to MECOPY except for null testing
		** pre-instructions. MECOPY has none, MECOPYN uses 1CXI to 
		** skip MECOPY but set null ind. (for hash computations) and
		** MECOPYQA uses 2CXI to simply skip MECOPY (for Quel 
		** aggregate results). */
		MEcopy(data[1], oprP[0]->opr_len, data[0]);
            break;

	  case ADFI_1388_UNORM_UTF8:
	  case ADFI_1389_UNORM_UNICODE:
	  case ADE_UNORM:
		/* Make DV's from operands and call normalize function. */
		ad0_opr2dv(base_array, oprP[0], &dv[0]);
		ad0_opr2dv(base_array, oprP[1], &dv[1]);
		if (oprP[0]->opr_dt < 0)
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
							/* clear null ind. */
		if (dv[0].db_datatype < 0) 
		{
		    dv[0].db_datatype = -dv[0].db_datatype;
		    dv[0].db_length--;
		}
		if (dv[1].db_datatype < 0) 
		{
		    dv[1].db_datatype = -dv[1].db_datatype;
		    dv[1].db_length--;
		}

		if ((dv[0].db_datatype == DB_NVCHR_TYPE) || 
		    (dv[0].db_datatype == DB_NCHR_TYPE))
		    db_stat = adu_unorm(adf_scb, &dv[1], &dv[0]);
		else
		    db_stat = adu_utf8_unorm(adf_scb, &dv[1], &dv[0]);

		if (db_stat != E_DB_OK)
		  return(db_stat);
		break;

#ifdef    ADE_BACKEND
	  case ADE_AGBGN:
	    /* With the new aggregate code structure, this is only used by
	    ** FRS apps. They work the old fashioned (slow) way. */
	    agstruct_ptr = (ADF_AG_STRUCT *) data[0];
	    func = Adf_globs->Adi_fi_lkup[
		ADI_LK_MAP_MACRO(agstruct_ptr->adf_agfi)].adi_agbgn;
	    if (db_stat = (*func)(adf_scb, agstruct_ptr))
	    {
		if (db_stat == E_DB_WARN)
		    final_stat = E_DB_WARN;
		else
		    return(db_stat);
	    }
	    break;

	  case ADE_AGNEXT:
	    /* With the new aggregate code structure, this is used to execute 
	    ** the "next" F.I. code for FRS (since the F.I.'s are now case's
	    ** in the "big switch". There's enough stuff in the ag-struct that
	    ** the "next" function can be called from here. */

	    agstruct_ptr = (ADF_AG_STRUCT *) data[0];
	    ad0_opr2dv(base_array, oprP[1], &dv[1]);
	    func = agstruct_ptr->adf_agnx;
	    db_stat = (*func)(adf_scb, &dv[1], agstruct_ptr);
	    if (db_stat)
	     if (db_stat == E_DB_WARN) final_stat = E_DB_WARN;
		 else return(db_stat);
	    break;

	  case ADE_AGEND:
	    /* With the new aggregate code structure, this is not used at all. */
	    agstruct_ptr = (ADF_AG_STRUCT *) data[1];
	    func = Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(agstruct_ptr->adf_agfi)].adi_agend;
	    ad0_opr2dv(base_array, oprP[0], &dv[0]);
	    if (db_stat = (*func)(adf_scb, agstruct_ptr, &dv[0]))
	    {
		if (db_stat == E_DB_WARN)
		    final_stat = E_DB_WARN;
		else
		    return(db_stat);
	    }
	    break;

	  case ADE_NAGEND:
	    /* With the new aggregate code structure, this is only used by
	    ** FRS apps. They work the old fashioned (slow) way. */
	    agstruct_ptr = (ADF_AG_STRUCT *) data[1];
	    fdesc = (ADI_FI_DESC*)Adf_globs->Adi_fi_lkup[
				ADI_LK_MAP_MACRO(agstruct_ptr->adf_agfi)
							    ].adi_fi;
	    if (    agstruct_ptr->adf_agcnt == 0
		&&  fdesc->adi_fiopid != ADI_CNT_OP
		&&  fdesc->adi_fiopid != ADI_CNTAL_OP
	       )
	    {
		if (oprP[0]->opr_dt < 0)	    /* nullable result */
		    /* Set NULL bit & implicitly clear SING bit */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		else
		    return(adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
	    }
	    else
	    {
		func = Adf_globs->Adi_fi_lkup[
				ADI_LK_MAP_MACRO(agstruct_ptr->adf_agfi)
						].adi_agend;
		ad0_opr2dv(base_array, oprP[0], &dv[0]);
		if (dv[0].db_datatype < 0)	    /* nullable result */
		{
		    ADF_CLRNULL_MACRO(&dv[0]);
		    dv[0].db_datatype = -dv[0].db_datatype;
		    dv[0].db_length--;
		}
		if (db_stat = (*func)(adf_scb, agstruct_ptr, &dv[0]))
		{
		    if (adf_scb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
		    {
			/* Some aggs return NULL this way. */
			if (oprP[0]->opr_dt < 0)	    /* nullable result */
			{
			    /* Set NULL bit & implicitly clear SING bit */
			    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
			    db_stat = E_DB_OK;
			}
			else return(db_stat);
		    }
		    else if (db_stat == E_DB_WARN)
			final_stat = E_DB_WARN;
		    else
			return(db_stat);
		}
	    }
	    break;

	  case ADE_OLAGBGN:
	    /* With 2.6 aggregate enhancements in place, OLAGBGN is only used
	    ** for binary aggregates to initialize the "OLAP" work structure. */
	    olapstruct_ptr = (ADF_AG_OLAP *)data[0];
	    olapstruct_ptr->adf_agcnt = 0;
	    for (i = 0; i < 5; i++) olapstruct_ptr->adf_agfrsv[i] = 0.0;
	    break;

	  case ADE_OLAGEND:
	    /* With 2.6 aggregate enhancements, ADE_OLAGEND is only used for 
	    ** binary aggregates, though it does more or less what previous
	    ** ADE_AGEND did (with null support like ADE_NAGEND). oprP[0] is
	    ** result operand, oprP[1] is OLAP agg work structure and oprP[2]
	    ** is the OLAP agg. F.I. index. This code looks up "end" function
	    ** using index and calls it. */

	    /* Set up operands and find "end" function. */
	    olapstruct_ptr = (ADF_AG_OLAP *)data[1];
	    a0 = *(i4 *)data[2];
	    func = Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(a0)].adi_agend;
	    ad0_opr2dv(base_array, oprP[0], &dv[0]);

	    if (dv[0].db_datatype < 0)	/* nullable result? */
	    {
		ADF_CLRNULL_MACRO(&dv[0]);
		dv[0].db_datatype = -dv[0].db_datatype;
		dv[0].db_length--;
	    }

	    /* Call "end" function and process possible errors. */
	    if (db_stat = (*func)(adf_scb, olapstruct_ptr, &dv[0]))
	    {
		if (adf_scb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
		{
		    /* Process null result. */
		    if (oprP[0]->opr_dt < 0)
		    {
			/* Set NULL bit & implicitly clear SING bit */
			*((u_char *)data[0] + oprP[0]->opr_len-1) = ADF_NVL_BIT;
			db_stat = E_DB_OK;
		    }
		    else return(db_stat);

		}
		else if (db_stat == E_DB_WARN)
		    final_stat = E_DB_WARN;
		else return(db_stat);
	    }

	    break;

	  case ADE_AVG:		/* final average computation for inline aggs */
	  case ADE_AVGQ:
	    if (*(i4 *)data[2] == 0)
	    {
		/* Count is zero - result is null except for Quel, in which
		** case we've already set the result to 0. */
		if (f->ins_icode == ADE_AVGQ) break;

		if (oprP[0]->opr_dt > 0)
		{
		    return(adu_error(adf_scb,
			E_AD1012_NULL_TO_NONNULL, 0));
		}
		/* Set NULL bit & implicitly clear SING bit */
		*((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		break;
	    }
	    else if (oprP[0]->opr_dt < 0)
	    {
		/* Clear the null indicator. */
		*((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
	    }

	    /* Compute final average (differs by data type, of course). */
	    switch (abs(oprP[0]->opr_dt)) {
	      case DB_FLT_TYPE:
	      case DB_INT_TYPE:
		f0 = (*(f8 *)data[1]) / ((f8)(*(i4 *)data[2]));
		if (oprP[0]->opr_len <= 5) 
		    *(f4 *)data[0] = f0;
		else *(f8 *)data[0] = f0;
		break;
	      case DB_MNY_TYPE:
		((AD_MONEYNTRNL *)data[0])->mny_cents =
			((AD_MONEYNTRNL *)data[1])->mny_cents / 
							((f8)(*(i4 *)data[2]));
		db_stat = adu_11mny_round(adf_scb, (AD_MONEYNTRNL *)data[0]);
		if (db_stat != E_DB_OK) return(db_stat);
		break;
	      case DB_DEC_TYPE:
		/* Convert counter to decimal, copy sum to temp, then
		** do the division. */
		if (CVlpk(*(i4 *)data[2],
                        AD_I4_TO_DEC_PREC,
                        AD_I4_TO_DEC_SCALE,
                        (PTR)tempd1.dec) != E_DB_OK)
		    return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 
				2, 0, "ade decimal avg"));

		tprec1 = DB_MAX_DECPREC;
		tscale1 = (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec);
		MHpkdiv((PTR)data[1], 
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
		    (PTR)tempd1.dec,
			(i4)AD_I4_TO_DEC_PREC,
			(i4)AD_I4_TO_DEC_SCALE,
		    (PTR)data[0],
			&tprec1,
			&tscale1);
		break;

	      case DB_DTE_TYPE:
	      case DB_INYM_TYPE:
	      case DB_INDS_TYPE:
		/* P1=data[0]=dv[1] is out-DATE */
		/* P2=data[1] is AD_AGGDATE */
		/* P3=data[2] is count */
           	ad0_opr2dv(base_array, oprP[0], &dv[1]);
            	if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[1], &dnag1))
		    return (db_stat);

		/* If any input value was the stupid null date,
		** the result is also the stupid null date. */
	 	if (!(((AD_AGGDATE *)data[1])->tot_empty))
		{
		    /* Cloned code from adu_E3d_avg_date. */
		    i4	mremain, count, days;

		    count = *(i4 *)data[2];
		    mremain = ((AD_AGGDATE *)data[1])->tot_months % count;

		    dnag1.dn_month = ((AD_AGGDATE *)data[1])->tot_months / count;

		    if (dnag1.dn_month)
			((AD_AGGDATE *)data[1])->tot_status |= AD_DN_MONTHSPEC;
		    f1 = (mremain * AD_13DTE_DAYPERMONTH * AD_40DTE_SECPERDAY +
			((AD_AGGDATE *)data[1])->tot_secs +
			((AD_AGGDATE *)data[1])->tot_nsecs/AD_50DTE_NSPERS) / (f8)count;
		    /* Get days from seconds, then let function normalize
		    ** the rest. */
		    days = (i4)(f1 / AD_40DTE_SECPERDAY);
		    if (days != 0)
			((AD_AGGDATE *)data[1])->tot_status |= AD_DN_DAYSPEC;
		    f1 -= days * AD_40DTE_SECPERDAY;
		    dnag1.dn_seconds = (i4)f1;
		    dnag1.dn_nsecond = (i4)((f1 - dnag1.dn_seconds)*AD_50DTE_NSPERS);
		    if (dnag1.dn_seconds | dnag1.dn_nsecond)
			((AD_AGGDATE *)data[1])->tot_status |= AD_DN_TIMESPEC;
		    dnag1.dn_day = days;
		    dnag1.dn_status = ((AD_AGGDATE *)data[1])->tot_status;
		    dnag1.dn_year = 0;
		    /* Normalize to get final result. */
		    adu_2normldint(&dnag1);
		}
	    	if (db_stat = adu_7from_dtntrnl (adf_scb, &dv[1], &dnag1))
		    return (db_stat);
		break;
	    }

	    break;

	  case ADE_SINGLETON:	/* final computation for inline SINGLETON */
	    /* Also see ADFI_1463_SINGLETON_ALL and ADFI_1474_SINGLECHK */
	    if (*(i4*)data[2] == 0)
	    {
		/* Count is zero - result is null */
		if (oprP[0]->opr_dt > 0)
		{
		    /* Ouput should be nullable */
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0,
			    "ADE_SINGLETON not nullable"));
		}
		/* Set NULL bit & implicitly clear SING bit */
		*((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
	    }
	    else
	    {
		/* Copy input including any null/flag byte */
		MEcopy(data[1], oprP[0]->opr_len, data[0]);
		/* If count is more than one - set SING bit */
		if (*(i4 *)data[2] > 1)
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) |= ADF_SING_BIT;
	    }
	    break;

	  case ADE_NULLCHECK:	/* checks for null agg result */
	    if (oprP[0]->opr_dt < 0) break;
	    if (oprP[1]->opr_dt > 0) break;

	    /* If result is not nullable and source is nullable, check
	    ** for error condition. */
	    {
		char	*nullptr;
		nullptr = (char *)data[1] + oprP[1]->opr_len - 1;
		if ((*nullptr) & ADF_NVL_BIT)
		    return(adu_error(adf_scb,
			E_AD1012_NULL_TO_NONNULL, 0));
	    }
	    break;

	  case ADE_NO_PMQUEL:
	    pm_quel = FALSE;
	    break;

          case ADE_PMQUEL:
	    pm_quel = TRUE;
	    break;

          case ADE_PMFLIPFLOP:
	    pm_quel = !pm_quel;
	    break;
#endif /* ADE_BACKEND */

	  case ADE_PMENCODE:
	    {
		bool	ignore;
		i4	lnum = 0;
		i4	pmreqs = ADI_DO_PM;
		
		ad0_opr2dv(base_array, oprP[0], &dv[0]);
		if (db_stat = adi_pm_encode(adf_scb, pmreqs, &dv[0],
					    &lnum, &ignore)
		   )
		    return(db_stat);
	    }
	    break;

	  case ADE_ESC_CHAR:
	    /* set the escape character for LIKE and NOT LIKE */
	    ad0_opr2dv(base_array, oprP[0], &esc_char);
	    pat_flags = AD_PAT_NO_ESCAPE;
	    if (oprP[0]->opr_dt == DB_CHA_TYPE)
	    {
		if (esc_char.db_data[0])
		    pat_flags = AD_PAT_HAS_ESCAPE;
	    }
	    else if (esc_char.db_data[0] || esc_char.db_data[1])
		pat_flags = AD_PAT_HAS_UESCAPE;
	    esc_ptr  = &esc_char;

	    if (f->ins_nops >= 2)
	    {
		I2ASSIGN_MACRO(*(u_i2 *)data[1], pat_flags);
		if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_NO_ESCAPE)
		    esc_ptr = NULL;
	    }
	    break;

#ifdef    ADE_BACKEND
	  case ADE_KEYBLD:
	    bdt = abs(oprP[3]->opr_dt);
	    rangekey_flag = oprP[4]->opr_len;	/* the length of operand #4
						** is used to represent the
						** "range-key flag"
						*/
	    key_op = (ADI_OP_ID) oprP[4]->opr_dt;   /* the datatype of operand
                                                    ** #4 is used to represent
                                                    ** the "key operator" 
                                                    */
	    /* Now build the key */
	    ad0_opr2dv(base_array, oprP[3], &kblk.adc_kdv);
	    kblk.adc_opkey = key_op;
	    ad0_opr2dv(base_array, oprP[1], &kblk.adc_lokey);
	    ad0_opr2dv(base_array, oprP[2], &kblk.adc_hikey);
	    kblk.adc_pat_flags = pat_flags;
	    /* With the use of the pattern compiler, ADE_KEYBLD has little use
	    ** for .adc_escape as the compiled pattern has already 'escaped'
	    ** all it needed to. At some point this can probably be dropped */
	    if (esc_ptr != NULL)
		I2ASSIGN_MACRO(*(u_i2*)esc_ptr->db_data, kblk.adc_escape);
            if (db_stat = adc_keybld(adf_scb, &kblk))
		return(db_stat);

	    /* Now, check for range-key failure */
	    if (kblk.adc_tykey != ADC_KNOMATCH)
	    {
		switch (rangekey_flag)
		{
		  case ADE_1RANGE_DONTCARE:	/* always OK */
		    break;

		  case ADE_2RANGE_YES:	/* err if non range-type key found */
		    if (kblk.adc_tykey == ADC_KEXACTKEY)
			return (adu_error(adf_scb, E_AD550A_RANGE_FAILURE, 0));
		    break;

		  case ADE_3RANGE_NO:	/* err if range-type key found */
		    if (    kblk.adc_tykey == ADC_KRANGEKEY
			||  kblk.adc_tykey == ADC_KHIGHKEY
			||  kblk.adc_tykey == ADC_KLOWKEY
			||  kblk.adc_tykey == ADC_KALLMATCH
		       )
			return (adu_error(adf_scb, E_AD550A_RANGE_FAILURE, 0));
		    break;

		  default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
				2,0,"adex keybld"));
		}
	    }

	    /* Now put type of key built into destination:  oprP[0] */
	    *(i2 *)data[0] = kblk.adc_tykey;
	    esc_ptr = NULL;	/* turn off esc char if there was one */
	    pat_flags = 0;
	    break;

	  case ADE_CALCLEN:
	  {
	    i4	li;

	    if (vltptr == NULL)
	    {
		vltptr = base_array[ADE_VLTBASE];
	    }

	    /* First, set the base address for ADE_CALCLEN's VLT */
	    /* ------------------------------------------------- */
	    bdt = abs(oprP[1]->opr_dt);
	    if (    bdt != DB_VCH_TYPE
		&&  bdt != DB_TXT_TYPE
		&&  bdt != DB_VBYTE_TYPE
		&&  bdt != DB_LTXT_TYPE
		&&  bdt != DB_NVCHR_TYPE
	       )
	    {
		/* all other types */
		vltptr = (PTR)((char *)vltptr + DB_CNTSIZE);
	    }
	    base_array[oprP[0]->opr_base] = data[0] = vltptr;

	    /* Now, calculate the length for the VLT ... the datatype of */
            /* operand #0 is used to represent the "length spec code"    */
            /* and the length of operand #0 is used to represent the     */
            /* "fixed length size" if the length spec code is ADI_FIXED  */
	    /* --------------------------------------------------------- */
	    for (i=0, li=0; i < f->ins_nops; i++)
	    {
		/* Loop on the operands for this ADE_CALCLEN instruction */
		/* ----------------------------------------------------- */
		if (i == 0)	/* If operand #0 ... */
		{
		    lenspec.adi_lncompute = oprP[0]->opr_dt;
		    lenspec.adi_fixedsize = oprP[0]->opr_len;
		    lenspec.adi_psfixed   = oprP[0]->opr_prec;
		}
		else		/* ... else; operand #1, #2, etc. */
		{
		    i4	    im1 = i - 1;

		    /* set datatype and prec for operand #i */
		    dv[im1].db_datatype = oprP[i]->opr_dt;
		    dv[im1].db_prec	= oprP[i]->opr_prec;
		    dv[im1].db_collID	= oprP[i]->opr_collID;
		    /* Also clear db_data as will be likely to be dereferenced
		    ** otherwise by adi_0calclen */
		    dv[im1].db_data	= NULL;
		    if (im1 != 0)
		    {
	    	        opptrs[li++] = &dv[im1];
			/* Set length for operand #i, if not the output ...
			** If length is ADE_LEN_UNKNOWN then get the length from
			** the data ... do not forget to add DB_CNTSIZE for the
			** DB_CNTSIZE byte count field at the front of the data,
			** if the type is a variable length type.
			*/
			if (oprP[i]->opr_len != ADE_LEN_UNKNOWN)
			{
			    dv[im1].db_length = oprP[i]->opr_len;
			}
			else
			{
			    switch (abs(oprP[i]->opr_dt))
			    {
			      case DB_VCH_TYPE:
			      case DB_VBYTE_TYPE:
			      case DB_TXT_TYPE:
			      case DB_LTXT_TYPE:
				/* The variable length types */
				dv[im1].db_length =
					(*(i2 *)data[i]) + DB_CNTSIZE;
				break;
			    
			      case DB_NVCHR_TYPE:
				dv[im1].db_length = (*(i2 *)data[i]) * 
				 sizeof(UCS2) + DB_CNTSIZE;
				break;

                              case DB_NCHR_TYPE:
                                dv[im1].db_length = (*(i2 *)((char *)data[i] - 
                                        DB_CNTSIZE)) * sizeof(UCS2) ; 
                                break;

			      default:
				/* all other types */
				dv[im1].db_length =
					(*(i2 *)((char *)data[i] - DB_CNTSIZE));
				break;
			    }
			    if (dv[im1].db_datatype < 0)
				dv[im1].db_length++;
			}
		    }
		}
	    }	/* End of for loop for the ADE_CALCLEN operands */

	    if (db_stat = adi_0calclen(adf_scb, &lenspec, f->ins_nops - 2,
	    		 opptrs, &dv[0]))
		return(db_stat);

	    bytes_needed = calced_len = dv[0].db_length;
	    if (dv[0].db_datatype < 0)
		calced_len--;

	    /* Now put the VLT length at the front of the VLT data ... do
	    ** not forget to subtract off the DB_CNTSIZE byte count field for
	    ** variable length types.
	    */
	    switch (bdt)
	    {
	      case DB_VCH_TYPE:
	      case DB_VBYTE_TYPE:
	      case DB_TXT_TYPE:
	      case DB_LTXT_TYPE:
		/* The variable length types */
		*(i2 *)data[0] = calced_len - DB_CNTSIZE;
		ADE_ROUNDUP_MACRO(bytes_needed, sizeof(ALIGN_RESTRICT));
		break;

	     case DB_NVCHR_TYPE:
		*(i2 *)data[0] = (calced_len - DB_CNTSIZE)/ sizeof(UCS2);
		ADE_ROUNDUP_MACRO(bytes_needed, sizeof(ALIGN_RESTRICT));
		break;

             case DB_NCHR_TYPE:
                *(i2 *)(data[0] - DB_CNTSIZE) = calced_len / sizeof(UCS2);
                bytes_needed += DB_CNTSIZE;
                ADE_ROUNDUP_MACRO(bytes_needed, sizeof(ALIGN_RESTRICT));
                bytes_needed -= DB_CNTSIZE;
                break;

	      default:
		/* all other types */
		*(i2 *)((char *)data[0] - DB_CNTSIZE) = calced_len;
		bytes_needed += DB_CNTSIZE;
		ADE_ROUNDUP_MACRO(bytes_needed, sizeof(ALIGN_RESTRICT));
		bytes_needed -= DB_CNTSIZE;
		break;
	    }

	    /* Now increment the # bytes used in the scratch    */
	    /* space, and then move the VLT pointer accordingly */
	    /* ------------------------------------------------ */

	    vltptr = (PTR) ((char *) vltptr + bytes_needed);

	    break;
	  }

	  case ADE_BYCOMPARE:
	    /*
	    ** Handle NULL values and nullable types with semantics needed for
	    ** `BY' clauses.  This means, a NULL compared with another NULL will
	    ** call them equal; a NULL compared with anything non-NULL will call
	    ** the NULL value greater;  all other comparisons will be done via
	    ** adc_compare() semantics.
	    */
	    dcmp = &ade_excb->excb_cmp;
	    *dcmp = ADE_1EQ2;

	    if (oprP[0]->opr_dt < 0)
	    {
		if (*((u_char *)data[0] + oprP[0]->opr_len - 1) & ADF_NVL_BIT)
		{
		    /* First value is NULL; check 2nd value */
		    if (    oprP[1]->opr_dt < 0
			&&  (*((u_char *)data[1] + oprP[1]->opr_len - 1)
								& ADF_NVL_BIT)
		       )
		    {
			/* Second value is also NULL; so 1st == 2nd */
			*dcmp = ADE_1EQ2;
			break;	    /* OK, go to next instruction */
		    }
		    else
		    {
			/* Second value is not NULL; so 1st > 2nd */
			*dcmp = ADE_1GT2;
			ade_excb->excb_value = ADE_NOT_TRUE;
			if ((lbase != ADE_NOBASE) && (lbase_size > 0))
			    ade_excb->excb_size -= lbase_size;
			return(E_DB_OK);
		    }
		}
		else
		{
		    /* We need a writable copy of the opr */
		    if ( oprP[0] != &oprs[0] )
		    {
			STRUCT_ASSIGN_MACRO(*oprP[0], oprs[0]);
			oprP[0] = &oprs[0];
		    }
		    oprP[0]->opr_dt = -(oprP[0]->opr_dt);
		    oprP[0]->opr_len--;
		}
	    }

	    if (oprP[1]->opr_dt < 0)
	    {
		if (*((u_char *)data[1] + oprP[1]->opr_len - 1) & ADF_NVL_BIT)
		{
		    /* Second value is NULL, first not; so 1st < 2nd */
		    *dcmp = ADE_1LT2;
		    ade_excb->excb_value = ADE_NOT_TRUE;
		    if ((lbase != ADE_NOBASE) && (lbase_size > 0))
			ade_excb->excb_size -= lbase_size;
		    return(E_DB_OK);
		}
		else
		{
		    /* We need a writable copy of the opr */
		    if ( oprP[1] != &oprs[1] )
		    {
			STRUCT_ASSIGN_MACRO(*oprP[1], oprs[1]);
			oprP[1] = &oprs[1];
		    }
		    oprP[1]->opr_dt = -(oprP[1]->opr_dt);
		    oprP[1]->opr_len--;
		}
	    }
	    
	    goto do_adccompare;

	  case ADE_COMPAREN:
	  case ADE_NCOMPAREN:
	    {
		ADE_OPERAND *oprnd = (ADE_OPERAND *)opr_internal;
		i4 lo = 0, hi = f->ins_nops, mid;
		/*
		** Handle NULL values and nullable types with semantics needed for
		** keyed joins, et. al.  For NULL value, if the first value is NULL,
		** we return ADE_1ISNULL; All other comparisons will be done via
		** adc_compare() semantics.
		*/
		dcmp = &ade_excb->excb_cmp;
		*dcmp = ADE_NOTIN;

		/* Load dv[0] once now */
		dv[0].db_datatype = oprnd[0].opr_dt;
		dv[0].db_prec = oprnd[0].opr_prec;
		dv[0].db_length = oprnd[0].opr_len;
		dv[0].db_data = (PTR)((char*)base_array[oprnd[0].opr_base]+
					oprnd[0].opr_offset);
		dv[0].db_collID = oprnd[0].opr_collID;
		if (dv[0].db_datatype < 0)
		{
		    dv[0].db_length--;
		    if (*((u_char *)dv[0].db_data + dv[0].db_length) & ADF_NVL_BIT)
		    {
			*dcmp = ADE_1ISNULL;
			cx_value = ADE_UNKNOWN;
			break;
		    }
		    dv[0].db_datatype = -dv[0].db_datatype;
		}
		while (mid = (hi - lo) / 2)
		{
		    mid += lo;
		    dv[1].db_datatype = oprnd[mid].opr_dt;
		    dv[1].db_prec = oprnd[mid].opr_prec;
		    dv[1].db_length = oprnd[mid].opr_len;
		    dv[1].db_data = (PTR)((char*)base_array[oprnd[mid].opr_base]+
					oprnd[mid].opr_offset);
		    dv[1].db_collID = oprnd[mid].opr_collID;
		    if (db_stat = adc_compare(adf_scb, &dv[0], &dv[1], &cmp))
			return(db_stat);
		    if (cmp < 0)
		    {
			hi = mid;
			if (mid == 1)
			{
			    *dcmp = ADE_1LT2;
			    break;
			}
		    }
		    else if (cmp > 0)
		    {
			lo = mid;
			if (mid == f->ins_nops-1)
			{
			    *dcmp = ADE_1GT2;
			    break;
			}
		    }
		    else
		    {
			*dcmp = ADE_1EQ2;
			break;
		    }
		}
		if ((f->ins_icode == ADE_COMPAREN) ? *dcmp : !*dcmp)
		    cx_value = ADE_NOT_TRUE;
		else
		    cx_value = ADE_TRUE;
	    }
	    break;
	  case ADE_COMPARE:
	    /*
	    ** Handle NULL values and nullable types with semantics needed for
	    ** keyed joins, subselect joins, et. al.  For NULL values, if only
	    ** the first value is NULL, we return ADE_1ISNULL; if only the
	    ** second value is NULL, we return ADE_2ISNULL; if both are NULL we
	    ** return ADE_BOTHNULL.   All other comparisons will be done via
	    ** adc_compare() semantics.
	    */
	    dcmp = &ade_excb->excb_cmp;
	    *dcmp = ADE_1EQ2;

	    if (oprP[0]->opr_dt < 0)
	    {
		if (*((u_char *)data[0] + oprP[0]->opr_len - 1) & ADF_NVL_BIT)
		{
		    /* First value is NULL; check 2nd value */
		    if (    oprP[1]->opr_dt < 0
			&&  (*((u_char *)data[1] + oprP[1]->opr_len - 1)
								& ADF_NVL_BIT)
		       )
		    {
			/* Second value is also NULL; so return ADE_BOTHNULL */
			*dcmp = ADE_BOTHNULL;
			ade_excb->excb_value = ADE_NOT_TRUE;
			if ((lbase != ADE_NOBASE) && (lbase_size > 0))
			    ade_excb->excb_size -= lbase_size;
			return (E_DB_OK);
		    }
		    else
		    {
			/* Second value is not NULL; so only 1st ISNULL */
			*dcmp = ADE_1ISNULL;
			ade_excb->excb_value = ADE_NOT_TRUE;
			if ((lbase != ADE_NOBASE) && (lbase_size > 0))
			    ade_excb->excb_size -= lbase_size;
			return (E_DB_OK);
		    }
		}
		else
		{
		    /* We need a writable copy of the opr */
		    if ( oprP[0] != &oprs[0] )
		    {
			STRUCT_ASSIGN_MACRO(*oprP[0], oprs[0]);
			oprP[0] = &oprs[0];
		    }
		    oprP[0]->opr_dt = -(oprP[0]->opr_dt);
		    oprP[0]->opr_len--;
		}
	    }

	    if (oprP[1]->opr_dt < 0)
	    {
		if (*((u_char *)data[1] + oprP[1]->opr_len - 1) & ADF_NVL_BIT)
		{
		    /* Second value is NULL; therefore only 2nd ISNULL */
		    *dcmp = ADE_2ISNULL;
		    ade_excb->excb_value = ADE_NOT_TRUE;
		    if ((lbase != ADE_NOBASE) && (lbase_size > 0))
			ade_excb->excb_size -= lbase_size;
		    return (E_DB_OK);
		}
		else
		{
		    /* We need a writable copy of the opr */
		    if ( oprP[1] != &oprs[1] )
		    {
			STRUCT_ASSIGN_MACRO(*oprP[1], oprs[1]);
			oprP[1] = &oprs[1];
		    }
		    oprP[1]->opr_dt = -(oprP[1]->opr_dt);
		    oprP[1]->opr_len--;
		}
	    }


do_adccompare:
	    switch (oprP[0]->opr_dt)
	    {
	      case DB_INT_TYPE:
		bo0 = bo1 = FALSE;
		switch (oprP[0]->opr_len)
		{
		  case 1:
		    a0 = I1_CHECK_MACRO(*(i1 *)data[0]);
		    break;
		  case 2:
		    a0 = *(i2 *)data[0];
		    break;
		  case 4:
		    a0 = *(i4 *)data[0];
		    break;
		  case 8:
		    ba0 = *(i8 *)data[0];
		    bo0 = TRUE;
		    break;
		}
		switch (oprP[1]->opr_len)
		{
		  case 1:
		    a1 = I1_CHECK_MACRO(*(i1 *)data[1]);
		    break;
		  case 2:
		    a1 = *(i2 *)data[1];
		    break;
		  case 4:
		    a1 = *(i4 *)data[1];
		    break;
		  case 8:
		    ba1 = *(i8 *)data[1];
		    bo1 = TRUE;
		    break;
		}
		if (!bo0 && !bo1)
		{
		    if      (a0 < a1) *dcmp = ADE_1LT2;
		    else if (a0 > a1) *dcmp = ADE_1GT2;
		    else              *dcmp = ADE_1EQ2;
		    break;
		}
		if (bo0 && bo1)
		{
		    if      (ba0 < ba1) *dcmp = ADE_1LT2;
		    else if (ba0 > ba1) *dcmp = ADE_1GT2;
		    else              *dcmp = ADE_1EQ2;
		    break;
		}
		if (bo0)
		{
		    if      (ba0 < a1) *dcmp = ADE_1LT2;
		    else if (ba0 > a1) *dcmp = ADE_1GT2;
		    else              *dcmp = ADE_1EQ2;
		    break;
		}
		if      (a0 < ba1) *dcmp = ADE_1LT2;
		else if (a0 > ba1) *dcmp = ADE_1GT2;
		else              *dcmp = ADE_1EQ2;
		break;
	      case DB_FLT_TYPE:
	      case DB_MNY_TYPE:
		if (oprP[0]->opr_len == 4 || oprP[1]->opr_len == 4)
		{
		    if (oprP[0]->opr_len == 4)
			f0 = *(f4 *)data[0];
		    else
			f0 = *(f8 *)data[0];
		    if (oprP[1]->opr_len == 4)
			f1 = *(f4 *)data[1];
		    else
			f1 = *(f8 *)data[1];
		    if      (f0 < f1) *dcmp = ADE_1LT2;
		    else if (f0 > f1) *dcmp = ADE_1GT2;
		    else	        *dcmp = ADE_1EQ2;
		}
		else
		{
		    /* both are f8, do f8 comparison */
		    f0 = *(f8 *)data[0];
		    f1 = *(f8 *)data[1];
		    if      (f0 < f1) *dcmp = ADE_1LT2;
		    else if (f0 > f1) *dcmp = ADE_1GT2;
		    else	      *dcmp = ADE_1EQ2;
		}
		break;
	      case DB_TXT_TYPE:
		{
		    u_char  *tc1;
		    u_char  *tc2;
		    u_char  *endtc1;
		    u_char  *endtc2;

		   if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
				oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
				oprP[1]->opr_collID != DB_UCS_BASIC_COLL)
			goto utf8_compare;
		    tc1 = (u_char *)data[0] + DB_CNTSIZE;
		    tc2 = (u_char *)data[1] + DB_CNTSIZE;
		    endtc1 = tc1 + ((DB_TEXT_STRING *)data[0])->db_t_count;
		    endtc2 = tc2 + ((DB_TEXT_STRING *)data[1])->db_t_count;
		    
		    if (adf_scb->adf_collation)
		    {
			cmp = adugcmp((ADULTABLE *)adf_scb->adf_collation,
				     0, tc1, endtc1, tc2, endtc2);
		    }
		    else
		    {
			cmp = 0;
			while (tc1 < endtc1  &&  tc2 < endtc2)
			{
			    if (cmp = CMcmpcase(tc1, tc2))
				break;
			    CMnext(tc1);
			    CMnext(tc2);
			}
			if (!cmp)		/* If equal up to shorter, */
						/* short strings < long    */
			    cmp = (endtc1 - tc1) - (endtc2 - tc2);
		    }

		    if      (cmp < 0) *dcmp = ADE_1LT2;
		    else if (cmp > 0) *dcmp = ADE_1GT2;
		    else              *dcmp = ADE_1EQ2;
		    break;
		}
	      case DB_CHR_TYPE:
		if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
			oprP[1]->opr_collID != DB_UCS_BASIC_COLL)
		    goto utf8_compare;
		if (adf_scb->adf_collation)
		{
		    cmp = adugcmp((ADULTABLE *)adf_scb->adf_collation,
				 ADUL_SKIPBLANK, (u_char *)data[0],
				 (u_i4) oprP[0]->opr_len + (u_char *)data[0],
				 (u_char *)data[1],
				 (u_i4) oprP[1]->opr_len + (u_char *)data[1]);
		}
		else
		{
		    cmp = STscompare((char *) data[0], (u_i4) oprP[0]->opr_len,
				     (char *) data[1], (u_i4) oprP[1]->opr_len);
		}
		if      (cmp < 0) *dcmp = ADE_1LT2;
		else if (cmp > 0) *dcmp = ADE_1GT2;
		else              *dcmp = ADE_1EQ2;
		break;
	      case DB_NVCHR_TYPE:
	      case DB_NCHR_TYPE:
		{
		    STATUS	cl_stat;
		    PTR		dp1, dp2;
		    u_i2	len1, len2;
		    UCS2	*ncs1, *ncs2;

		    dp1 = data[0];
		    dp2 = data[1];

		    if ( DB_NCHR_TYPE == oprP[0]->opr_dt )
		    {
			ncs1 = (UCS2*)dp1;
			len1 = oprP[0]->opr_len / sizeof(UCS2);
		    }
		    else
		    {
			ncs1 = ((DB_NVCHR_STRING*)dp1)->element_array;
			len1 = ((DB_NVCHR_STRING*)dp1)->count;
		    }
			
		    if ( DB_NCHR_TYPE == oprP[1]->opr_dt )
		    {
			ncs2 = (UCS2*)dp2;
			len2 = oprP[1]->opr_len / sizeof(UCS2);
		    }
		    else if ( DB_NVCHR_TYPE == oprP[1]->opr_dt )
		    {
			ncs2 = ((DB_NVCHR_STRING*)dp2)->element_array;
			len2 = ((DB_NVCHR_STRING*)dp2)->count;
		    }
		    else
			return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
					2,0,"adex nvtype"));
			
		    db_stat = aduucmp(adf_scb, ADUL_BLANKPAD,
			    ncs1, ncs1 + len1, ncs2, ncs2 + len2, &cmp,
			    oprP[1]->opr_collID);

		    if (db_stat != E_DB_OK)
			return (db_stat);

		    if      (cmp < 0) *dcmp = ADE_1LT2;
		    else if (cmp > 0) *dcmp = ADE_1GT2;
		    else              *dcmp = ADE_1EQ2;
		}
		break;
	      case DB_CHA_TYPE:
	      case DB_VCH_TYPE:
		if ((adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
			oprP[1]->opr_collID != DB_UCS_BASIC_COLL)
		    goto utf8_compare;
		if (!(adf_scb->adf_collation || 
				Adf_globs->Adi_status & ADI_DBLBYTE))
		{
		  /* Neither double byte, nor alternate collation - following
		  ** code is ADE_COMPARE fast path. */

		  /* Setup operands */
	          c1 = (u_char*)data[0];
	          c2 = (u_char*)data[1];

	          if (oprP[0]->opr_dt == DB_VCH_TYPE)
	          {
	              ln1 = *(u_i2*)c1;
	              c1 += 2;
		      if (ln1 > adf_scb->adf_maxstring)
			  return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	          }
	          else
	              ln1 = oprP[0]->opr_len;

	          if (oprP[1]->opr_dt == DB_VCH_TYPE)
	          {
	              ln2 = *(u_i2*)c2;
	              c2 += 2;
		      if (ln2 > adf_scb->adf_maxstring)
			  return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	          }
	          else
	              ln2 = oprP[1]->opr_len;

		  /* Prepare to do MEcmp() of the common length prefixes. */
		  diff = ln1 - ln2;
		  if (diff > 0)
			cmp = MEcmp(c1, c2, ln2);
		  else cmp = MEcmp(c1, c2, ln1);

		  if (cmp < 0)
			*dcmp = ADE_1LT2;
		  else if (cmp > 0)
			*dcmp = ADE_1GT2;
		  else if (diff == 0)
			*dcmp = ADE_1EQ2;
		  else
		  {
			/* One is longer than the other and the prefixes are 
			** equal. Check the remainder of the longer operand. */
			*dcmp = ADE_1EQ2;		/* default */
			if (diff > 0)
			{
			    /* c1 is longer, check for trailing blanks. */
			    c1 += ln2;
			    c2 = c1 + diff;	/* loop terminator */
			    while (c1 < c2 && *c1 == ' ')
				c1++;

			    if (c1 < c2)
				*dcmp = (*c1 > ' ') ? ADE_1GT2 : ADE_1LT2;
			}
			else
			{
			    /* c2 is longer, check for trailing blanks. */
			    c2 += ln1;
			    c1 = c2 - diff;	/* loop terminator */
			    while (c2 < c1 && *c2 == ' ')
				c2++;

			    if (c2 < c1)
				*dcmp = (*c2 > ' ') ? ADE_1LT2 : ADE_1GT2;
			}
		  }

	          break;
		}
		/* Double byte and alternate collation VCH/CHA drop through
		** here to execute "slow" path version of ADE_COMPARE. */
	      case DB_DTE_TYPE:
	      default:	/* Will have to do it the slow way */
		dv[0].db_datatype = dv[1].db_datatype = oprP[0]->opr_dt;
		dv[0].db_prec   = oprP[0]->opr_prec;
		dv[0].db_length = oprP[0]->opr_len;
		dv[0].db_data   = data[0];
		dv[1].db_prec   = oprP[1]->opr_prec;
		dv[1].db_length = oprP[1]->opr_len;
		dv[1].db_data   = data[1];
		if (db_stat = adc_compare(adf_scb, &dv[0], &dv[1], &cmp))
		    return(db_stat);
		if      (cmp < 0) *dcmp = ADE_1LT2;
		else if (cmp > 0) *dcmp = ADE_1GT2;
		else              *dcmp = ADE_1EQ2;
		break;
	    }

	    if (*dcmp)
	    {
		ade_excb->excb_value = ADE_NOT_TRUE;
		if ((lbase != ADE_NOBASE) && (lbase_size > 0))
		    ade_excb->excb_size -= lbase_size;
		return (E_DB_OK);
	    }

	    break;

utf8_compare:
	    /* UTF8 server with c/text/char/varchar comparands. Make 
	    ** DB_DATA_VALUEs and call function to do it. */
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);

	    if      (cmp < 0) *dcmp = ADE_1LT2;
	    else if (cmp > 0) *dcmp = ADE_1GT2;
	    else              *dcmp = ADE_1EQ2;

	    if (*dcmp)
	    {
		ade_excb->excb_value = ADE_NOT_TRUE;
		if ((lbase != ADE_NOBASE) && (lbase_size > 0))
		    ade_excb->excb_size -= lbase_size;
		return (E_DB_OK);
	    }

	    break;

	 case ADE_DBDV:
	 {
	    DB_DATA_VALUE	*dbv;
	    
	    /* Create a DB_DATA_VALUE for oprP[1] at the location described */
	    /* by oprP[0]. This is used to create QEF_USR_PARAM entries at  */
	    /* runtime. Note that although opr_dt and opr_len are not used  */
	    /* for oprP[0], they should be set to the same values as	    */
	    /* oprP[1]. This will keep all the compilation code from	    */
	    /* building type conversion instructions for the operands.      */
	    /* On entry:						    */
	    /*	    data[0]	location for DB_DATA_VALUE		    */
	    /*	    data[1]	address for db_data			    */
	    /*	    oprP[1]->opr_dt  type for db_datatype		    */
	    /*	    oprP[1]->opr_prec precision for db_prec		    */
	    /*	    oprP[1]->opr_len length for db_length		    */
	    /*	    db_prec is set to zero for now. */

            /*
            ** Note: structure QEF_USR_PARAM has elements
            **              {
            **                 char *parm_name;
            **                 i4   parm_nlen;
            **                 struct DB_DATA_VALUE {
            **                      PTR db_data;
            **                      i4 db_length;
            **                      DB_DT_ID db_datatype;
            **                      i2 db_prec;
            **                 } parm_dbv;
            **              }
            ** at this point, data[0] should point to db_data, but
            ** on 64-bit platform like Alpha, due to pointer elements are
            ** aligned to 8-byte boundary, there is a 4-byte pad
            ** after parm_nlen (since i4  is i4), so data[0] is in
            ** fact point to the beginning of the pad, which is wrong !
            ** Need to adjust the dbv pointer to make it point to db_data.
            **                                      - [kchin] 03/04/93
            */
	    dbv = (DB_DATA_VALUE *)data[0];
	    dbv->db_datatype = oprP[1]->opr_dt;
	    dbv->db_prec   = oprP[1]->opr_prec;
	    dbv->db_length = oprP[1]->opr_len;
	    dbv->db_data = data[1];
	    break;
	 }
	 
	 case ADE_PNAME:
	 {
	    /* We assume this structure is the same as the first two fields */
	    /* of the QEF_USR_PARAM structure */
	    struct _cpp
	    {
		    char    *cp_name;
		    i4	    cp_length;
	    } *cpp;
	    
	    /* Move a pointer for oprP[1] to the location described by	    */
	    /* oprP[0]. This is used in generating the pointers to the	    */
	    /* parameter names in a QEF_USR_PARAM structure.		    */
	    /* data[0]	    location in which to place pointer.		    */
	    /* data[1]	    pointer to the data value.			    */
	    /* Note that although types and lengths are not used, they	    */
	    /* should be set to identical values for oprP[0] and oprP[1] so */
	    /* that the compilation routines do not try to create type	    */
	    /* conversion instructions for the operands. */
	    cpp = (struct _cpp *)data[0];
	    cpp->cp_name = (char *)data[1];

	    /* We also need the length of this string in the next location  */
	    cpp->cp_length = oprP[1]->opr_len;
	    break;
	 }
	 	    
#endif /* ADE_BACKEND */

	  case ADE_TMCVT:
	  {
		i4	outlen;

		/* set up the output dataval */
		dv[0].db_datatype = oprP[0]->opr_dt;
		dv[0].db_prec	  = oprP[0]->opr_prec;
		dv[0].db_length	  = oprP[0]->opr_len;
		dv[0].db_data	  = data[0];

		/* set up the input dv */
		dv[1].db_datatype = oprP[1]->opr_dt;
		dv[1].db_prec	  = oprP[1]->opr_prec;
		dv[1].db_length	  = oprP[1]->opr_len;
		dv[1].db_data	  = data[1];

		/* Convert the input data into a printable format and put it
		** into a buffer.
		*/
		adc_tmcvt(adf_scb, &dv[1], &dv[0], &outlen);
		break;
	  }

	  case ADE_REDEEM:
	  {
	    ADE_INSTRUCTION *CA_f;
	    i4         CA_next_instr_offset;
	    i4         CA_this_instr_offset;
	    i4         CA_last_instr_offset;
	    i4              CA_len = 0;
	    ADE_OPERAND     CA_oprs;
	    ADE_OPERAND	*CA_oprP;
	    ADE_I_OPERAND   *CA_opr_internal;

	    /* This is new Code for BLOB... CA  -- Ranjit 06/10/94          */
	    /* The reason for this allowance is that with BLOBs there is no */
	    /* indication of the length and therefore with the buffer being */
	    /* of finite limit, logic is built in to get the BLOB in smaller*/
	    /* chunks. The only problem is that the last chunk could gobble */
	    /* most of the buffer and leaving the remaining instructions not*/
	    /* enough space, which will fail the transaction.               */
	    /* This is optimised to the point of determining the length of  */
	    /* remaining instructions, thus still giving buffer space with  */
	    /* minimal trade off for UNKNOWN LENGTH. Also, code is localised*/
	    /* to affect only Tables with BLOBs and past a full buffer(2008)*/

	    {
		CA_next_instr_offset = next_instr_offset;
		CA_last_instr_offset = cxhead->cx_seg[ADE_SMAIN].seg_last_offset;

		while (CA_next_instr_offset != ADE_END_OFFSET_LIST)
		{
		    CA_f = (ADE_INSTRUCTION *) ((char *)cxbase +
						CA_next_instr_offset);
		    CA_this_instr_offset = CA_next_instr_offset;
		    CA_opr_internal = (ADE_I_OPERAND *) ((char *)CA_f +
					 sizeof(ADE_INSTRUCTION));
		    ADE_GET_OPERAND_MACRO(base_array,
					CA_opr_internal, &CA_oprs,
					&CA_oprP);
		    if ((CA_oprP->opr_base == lbase)
			&& (CA_oprP->opr_len != ADE_LEN_LONG))
		    {
			CA_len += CA_oprP->opr_len;
		    }
		    CA_next_instr_offset =
		    ADE_GET_OFFSET_MACRO(CA_f, CA_next_instr_offset,
					    CA_last_instr_offset);
		}
	    }
		/* set up the output dataval */
	    dv[0].db_datatype = oprP[0]->opr_dt;
	    dv[0].db_prec     = oprP[0]->opr_prec;
	    dv[0].db_length   = oprP[0]->opr_len - CA_len;

	    /* Restrict output to MAXI2 */
	    if (dv[0].db_length > MAXI2)
		dv[0].db_length = MAXI2;
	    dv[0].db_data     = data[0];

	    /* set up the input dv */
	    dv[1].db_datatype = oprP[1]->opr_dt;
	    dv[1].db_prec	  = oprP[1]->opr_prec;
	    dv[1].db_length   = oprP[1]->opr_len;
	    dv[1].db_data     = data[1];

	    /* set up the workspace dv */
	    dv[2].db_datatype = oprP[2]->opr_dt;
	    dv[2].db_prec	  = oprP[2]->opr_prec;
	    dv[2].db_length	  = oprP[2]->opr_len;
	    dv[2].db_data	  = data[2];
		
	    final_stat = adu_redeem(adf_scb,
				    &dv[0],
				    &dv[1],
				    &dv[2],
				    ade_excb->excb_continuation);
	    
	    /*
	    ** Since we have just processed a large object-type datatype,
	    ** then we must see if we are done, incomplete, or just generally
	    ** happy.
	    **
	    ** If we are complete, then readjust our offset pointers and keep
	    ** going.  Otherwise, keep going.  The stuff at the beginning of
	    ** this loop (waaaaaaaaaaaaaaaaaaaaaaaaaay back there) will finish
	    ** up correctly.
	    */
	    if (DB_FAILURE_MACRO(final_stat))
		return(final_stat);
	    else
		lbase_size = old_lbase_size - dv[0].db_length;
		
	    if (final_stat == E_DB_OK)
	    {
		element_length = dv[0].db_length;
		ade_excb->excb_continuation = 0;
	    }
	  }
	  break;

	  case ADFI_895_II_TRUE:
	    /* Reset CX value to TRUE and CX compare to 1EQ2.  This is used
	    ** by MAIN segment of aggregate computation, because the raggf
	    ** driver uses the BYCOMPARE results to decide whether to emit
	    ** a new group or not.  But, CASE expressions inside the agg
	    ** accumulation code can change the CX truth value and confuse
	    ** the driver.  If we get as far as the aggregation code in
	    ** the CX, we know that the driver wants to see TRUE, which is
	    ** what this operator does.  In an ideal world, ADE_SETTRUE
	    ** would not be generated into an agg CX that did not involve
	    ** CASE expressions.
	    ** ii_true and ii_false are also used to present the results
	    ** of a constant-folded expression such as "where 1=0".
	    */
	    cx_value = ADE_TRUE;
	    ade_excb->excb_cmp = ADE_1EQ2;
	    break;
	  case ADFI_894_II_FALSE:
	    cx_value = ADE_NOT_TRUE;
	    ade_excb->excb_cmp = ADE_1LT2;
	    break;

#ifdef ADE_BACKEND
	  /* Inline aggregates - replace old function calling mechanism. */
	  /* ----------------------------------------------------------- */

	  case ADFI_396_COUNTSTAR:
	  case ADFI_090_COUNT_C_ALL:
	    (*(i4 *)data[0])++;
	    break;

	  case ADFI_1463_SINGLETON_ALL:
	    /* Also see ADFI_1474_SINGLECHK_ALL aggregate and ADE_SINGLETON */
	    if (oprP[0]->opr_dt > 0)
		/* Ouput should be nullable */
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0,
			"singleton not nullable"));
	    /* Set count (=data[2]) to 1 or 2 depending on whether unique.
	    ** We don't just increment due to overflow */
	    if (*(i4 *)data[2] == 0)
	    {
		*(i4 *)data[2] = 1;
		/* Pass through data. As lenspec ADI_O1 is uesd the
		** lengths match unless input is not nullable in which
		** case we need to zero the NULL & SING bits */
		MEmove(oprP[1]->opr_len, data[1], 0,
			oprP[0]->opr_len, data[0]);
	    }
	    else
		*(i4 *)data[2] = 2;

	    break;

	  case ADFI_081_ANY_C_ALL:
	    if (oprP[0]->opr_len >= 4) (*(i4 *)data[0]) = 1;
	    if (oprP[0]->opr_len == 3 || 
		oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) (*(i2 *)data[0]) = 1;
	    else  (*(i1 *)data[0]) = 1;
	    break;

	  case ADFI_087_AVG_F:
	    /* Increment counter and roll the total. */
	    EXmath ((i4)EX_ON);
	    (*(i4 *)data[2])++;
	    if (oprP[1]->opr_len >= 8) f0 = *(f8 *)data[1];
	    else f0 = *(f4 *)data[1];
	    *(f8 *)data[0] += f0;
	    EXmath ((i4)EX_OFF);
	    break;

	  case ADFI_088_AVG_I:
	    /* Increment counter, convert integer value to float and roll the total. */
	    EXmath ((i4)EX_ON);
	    (*(i4 *)data[2])++;
	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            *(f8 *)data[0] += (f8)ba1;
	    EXmath ((i4)EX_OFF);
	    break;

	  case ADFI_089_AVG_MONEY:
	    /* Increment counter and roll the total. */
	    EXmath ((i4)EX_ON);
	    (*(i4 *)data[2])++;
	    *(f8 *)data[0] += ((AD_MONEYNTRNL *)data[1])->mny_cents;
	    EXmath ((i4)EX_OFF);
	    break;

	  case ADFI_464_AVG_DATE:
	    /* Accumulate sum in special work structure (data[1]). But ignore 
	    ** absolute date values in computation. */

	    /* If this is a null date, or if there's already been a null date, 
	    ** we don't do the average. */
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[2], &dnag2))
		return (db_stat);

	    if (((AD_AGGDATE *)data[0])->tot_empty || dnag2.dn_status == AD_DN_NULL)
	    {
		((AD_AGGDATE *)data[0])->tot_empty = 1;
		break;
	    }

	    if (dnag2.dn_status & AD_DN_ABSOLUTE)
	    {
		adf_scb->adf_warncb.ad_anywarn_cnt++;
		adf_scb->adf_warncb.ad_agabsdate_cnt++;
		break;
	    }

	    /* Increment counter and roll totals. This code was cloned from
	    ** old aggregation functions. */
	    (*(i4 *)data[2])++;
	    if (dnag2.dn_status & AD_DN_YEARSPEC)
		((AD_AGGDATE *)data[0])->tot_months += dnag2.dn_year * 12;
	    if (dnag2.dn_status & AD_DN_MONTHSPEC)
		((AD_AGGDATE *)data[0])->tot_months += dnag2.dn_month;
	    if (dnag2.dn_status & AD_DN_DAYSPEC)
		((AD_AGGDATE *)data[0])->tot_secs += dnag2.dn_day * AD_40DTE_SECPERDAY;
	    if (dnag2.dn_status & AD_DN_TIMESPEC)
	    {
		i4 over;
		((AD_AGGDATE *)data[0])->tot_nsecs += dnag2.dn_nsecond;
		if (over = ((AD_AGGDATE *)data[0])->tot_nsecs / AD_49DTE_INSPERS)
		    ((AD_AGGDATE *)data[0])->tot_nsecs -= over*AD_49DTE_INSPERS;
		((AD_AGGDATE *)data[0])->tot_secs += dnag2.dn_seconds + over;
	    }

            if (dnag2.dn_status & AD_DN_LENGTH)
            {
                ((AD_AGGDATE *)data[0])->tot_status |= AD_DN_LENGTH;
            }
	    break;

	  case ADFI_533_AVG_DEC:
	    /* Increment counter and roll the total. This is done by
	    ** copying out current total, adding new value and placing result
	    ** back in running total. */
	    (*(i4 *)data[2])++;
	
	/* copy one less byte if datatype nullable */

	    MEcopy((PTR)data[0], oprP[0]->opr_dt < 0 ? \
                oprP[0]->opr_len - 1 : \
                oprP[0]->opr_len, (PTR)tempd1.dec);

	    tprec1 = DB_MAX_DECPREC;
	    tscale1 = (i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec);
	    MHpkadd((PTR)data[1],
                    (i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
                    (i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
                (PTR)tempd1.dec,
                    (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
                    (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
                (PTR)data[0],
                    &tprec1,
                    &tscale1);
	    break;

	  case ADFI_097_MAX_DATE:
	    /* This is executed using the same logic as in the 
	    ** dedicated aggregate routine. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_4date_cmp. If the current max value is the "null" date,
	    ** the new value is copied to the result without comparing. */

	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[1], &dnag1))
		return (db_stat);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[2], &dnag2))
		return (db_stat);

	    cmp = 0;
	    if (dnag1.dn_status != AD_DN_NULL && dnag2.dn_status != AD_DN_NULL)
	     if (db_stat = adu_4date_cmp(adf_scb, &dv[1], &dv[2], &cmp))
		return(db_stat);

	    if (cmp < 0 || (dnag1.dn_status == AD_DN_NULL && dnag2.dn_status != AD_DN_NULL))
		MEcopy(data[1], oprP[1]->opr_len, data[0]);
	    break;

	  case ADFI_098_MAX_F:
	    if (oprP[0]->opr_len >= 8) f0 = *(f8 *)data[0];
	    else f0 = *(f4 *)data[0];

	    if (oprP[1]->opr_len >= 8) f1 = *(f8 *)data[1];
	    else f1 = *(f4 *)data[1];

	    if (f0 < f1)
	     if (oprP[0]->opr_len >= 8) *(f8 *)data[0] = f1;
	     else *(f4 *)data[0] = f1;
	    break;

	  case ADFI_099_MAX_I:
#ifdef LP64
	    if (oprP[0]->opr_len >= 8) ba0 = *(i8 *)data[0];
	     else if (oprP[0]->opr_len >= 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
			ba0 = *(i2 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len >= 8) ba1 = *(i8 *)data[1];
	     else if (oprP[1]->opr_len >= 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 3 ||
		    oprP[1]->opr_len == 2 && oprP[1]->opr_dt > 0) 
			ba1 = *(i2 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            if (ba1 > ba0)
	     if (oprP[0]->opr_len >= 8) *(i8 *)data[0] = ba1;
	     else if (oprP[0]->opr_len >= 4) *(i4 *)data[0] = ba1;
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
			*(i2 *)data[0] = ba1;
	     else *(i1 *)data[0] = I1_CHECK_MACRO(ba1);
#else
	    bo0 = bo1 = FALSE;
	    if (oprP[0]->opr_len >= 8)
	    {
		ba0 = *(i8 *)data[0];
		bo0 = TRUE;
	    }
	     else if (oprP[0]->opr_len >= 4) a0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
			a0 = *(i2 *)data[0];
	     else a0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len >= 8)
	    {
		ba1 = *(i8 *)data[1];
		bo1 = TRUE;
	    }
	     else if (oprP[1]->opr_len >= 4) a1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 3 ||
		    oprP[1]->opr_len == 2 && oprP[1]->opr_dt > 0) 
			a1 = *(i2 *)data[1];
	     else a1 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    /* Check for new max value. */
	    if (!bo0 && !bo1)
              cmp = (a0 < a1);
	    else if (bo0 && bo1)
	      cmp = (ba0 < ba1);
	    else if (bo0)
	      cmp = (ba0 < a1);
	    else cmp = (a0 < ba1);
	    if (cmp)
	     if (!bo1)
	     {
		if (oprP[0]->opr_len >= 8) *(i8 *)data[0] = a1;
		else if (oprP[0]->opr_len >= 4) *(i4 *)data[0] = a1;
		else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
						*(i2 *)data[0] = a1;
		else *(i1 *)data[0] = I1_CHECK_MACRO(a1); 
	     }
	     else
	     {
		if (oprP[0]->opr_len >= 8) *(i8 *)data[0] = ba1;
		else if (oprP[0]->opr_len >= 4) *(i4 *)data[0] = ba1;
		else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
						*(i2 *)data[0] = ba1;
		else *(i1 *)data[0] = I1_CHECK_MACRO(ba1); 
	     }
#endif
	    break;

	  case ADFI_100_MAX_MONEY:
	    if (((AD_MONEYNTRNL *)data[0])->mny_cents <
		((AD_MONEYNTRNL *)data[1])->mny_cents)
	     ((AD_MONEYNTRNL *)data[0])->mny_cents =
		((AD_MONEYNTRNL *)data[1])->mny_cents;
	    break;

	  case ADFI_096_MAX_C:
	  case ADFI_101_MAX_TEXT:
	  case ADFI_262_MAX_CHAR:
	  case ADFI_263_MAX_VARCHAR:
	  case ADFI_1032_MAX_NCHAR:
	  case ADFI_1033_MAX_NVCHAR:
	    /* All these are executed using the same logic as in the 
	    ** dedicated aggregate routines. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_lexcomp. */

	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    if (dv[1].db_datatype < 0)
	    {
		dv[1].db_datatype = -dv[1].db_datatype;
		dv[1].db_length--;
	    }
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (dv[2].db_datatype < 0)
	    {
		dv[2].db_datatype = -dv[2].db_datatype;
		dv[2].db_length--;
	    }
            if (db_stat = adu_lexcomp(adf_scb, (i4)FALSE, &dv[1], &dv[2], &cmp))
		return(db_stat);
            if (cmp > 0) break;

	    /* New max - just copy into place. */
	    MEcopy(data[1], min(oprP[0]->opr_len, oprP[1]->opr_len),
			data[0]);
            break;

	  case ADFI_481_MAX_LOGKEY:
	  case ADFI_500_MAX_TABKEY:
	    /* All these are executed using the same logic as in the 
	    ** dedicated aggregate routines. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_1logkey_cmp. */

	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    if (dv[1].db_datatype < 0)
	    {
		dv[1].db_datatype = -dv[1].db_datatype;
		dv[1].db_length--;
	    }
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (dv[2].db_datatype < 0)
	    {
		dv[2].db_datatype = -dv[2].db_datatype;
		dv[2].db_length--;
	    }
            if (db_stat = adu_1logkey_cmp(adf_scb, &dv[1], &dv[2], &cmp))
		return(db_stat);
            if (cmp > 0) break;

	    /* New max - just copy into place. */
	    MEcopy(data[1], min(oprP[0]->opr_len, oprP[1]->opr_len),
		data[0]);
            break;

	  case ADFI_535_MAX_DEC:
	    if (MHpkcmp(data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
		      data[0],
			(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec)) > 0)
		MEcopy(data[1], oprP[1]->opr_len, data[0]);
	    break;

	  case ADFI_103_MIN_DATE:
	    /* This is executed using the same logic as in the 
	    ** dedicated aggregate routine. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_4date_cmp. If the new value is the "null" date, it
	    ** is copied to the result without comparing. */
	    
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    MEfill(sizeof(dnag1), (u_char) '\0', (char *)&dnag1);
	    MEfill(sizeof(dnag2), (u_char) '\0', (char *)&dnag2);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[1], &dnag1))
		return (db_stat);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[2], &dnag2))
		return (db_stat);
	    cmp = 0;
	    if (dnag1.dn_status != AD_DN_NULL && dnag2.dn_status != AD_DN_NULL)
	     if (db_stat = adu_4date_cmp(adf_scb, &dv[1], &dv[2], &cmp))
		return(db_stat);
	    if (cmp > 0 || (dnag1.dn_status == AD_DN_NULL && dnag2.dn_status != AD_DN_NULL))
	    {
		MEcopy(data[1], oprP[1]->opr_len, data[0]);
	    }
	    break;

	  case ADFI_104_MIN_F:
	    if (oprP[0]->opr_len >= 8) f0 = *(f8 *)data[0];
	    else f0 = *(f4 *)data[0];

	    if (oprP[1]->opr_len >= 8) f1 = *(f8 *)data[1];
	    else f1 = *(f4 *)data[1];

	    if (f1 < f0)
	     if (oprP[0]->opr_len >= 8) *(f8 *)data[0] = f1;
	     else *(f4 *)data[0] = f1;
	    break;

	  case ADFI_105_MIN_I:
	    if (oprP[0]->opr_len >= 8) ba0 = *(i8 *)data[0];
	     else if (oprP[0]->opr_len >= 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
			ba0 = *(i2 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len >= 8) ba1 = *(i8 *)data[1];
	     else if (oprP[1]->opr_len >= 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 3 ||
		    oprP[1]->opr_len == 2 && oprP[1]->opr_dt > 0) 
			ba1 = *(i2 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            if (ba1 < ba0)
	     if (oprP[0]->opr_len >= 8) *(i8 *)data[0] = ba1;
	     else if (oprP[0]->opr_len >= 4) *(i4 *)data[0] = ba1;
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
				*(i2 *)data[0] = ba1;
	     else *(i1 *)data[0] = I1_CHECK_MACRO(ba1);
	    break;

	  case ADFI_106_MIN_MONEY:
	    if (((AD_MONEYNTRNL *)data[0])->mny_cents >
		((AD_MONEYNTRNL *)data[1])->mny_cents)
	     ((AD_MONEYNTRNL *)data[0])->mny_cents =
		((AD_MONEYNTRNL *)data[1])->mny_cents;
	    break;

	  case ADFI_102_MIN_C:
	  case ADFI_107_MIN_TEXT:
	  case ADFI_264_MIN_CHAR:
	  case ADFI_265_MIN_VARCHAR:
	  case ADFI_1034_MIN_NCHAR:
	  case ADFI_1035_MIN_NVCHAR:
	    /* All these are executed using the same logic as in the 
	    ** dedicated aggregate routines. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_lexcomp. */

	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    if (dv[1].db_datatype < 0)
	    {
		dv[1].db_datatype = -dv[1].db_datatype;
		dv[1].db_length--;
	    }
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (dv[2].db_datatype < 0)
	    {
		dv[2].db_datatype = -dv[2].db_datatype;
		dv[2].db_length--;
	    }
            if (db_stat = adu_lexcomp(adf_scb, (i4)FALSE, &dv[1], &dv[2], &cmp))
		return(db_stat);
            if (cmp < 0) break;

	    /* New min - just copy into place. */
	    MEcopy(data[1], min(oprP[0]->opr_len, oprP[1]->opr_len),
			data[0]);
            break;

	  case ADFI_482_MIN_LOGKEY:
	  case ADFI_501_MIN_TABKEY:
	    /* All these are executed using the same logic as in the 
	    ** dedicated aggregate routines. DB_DATA_VALUEs are made from
	    ** data[0] and data[1], and the comparison is done by 
	    ** adu_1logkey_cmp. */

	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    if (dv[1].db_datatype < 0)
	    {
		dv[1].db_datatype = -dv[1].db_datatype;
		dv[1].db_length--;
	    }
	    if (dv[2].db_datatype < 0)
	    {
		dv[2].db_datatype = -dv[2].db_datatype;
		dv[2].db_length--;
	    }
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_1logkey_cmp(adf_scb, &dv[1], &dv[2], &cmp))
		return(db_stat);
            if (cmp < 0) break;

	    /* New min - just copy into place. */
	    MEcopy(data[1], min(oprP[0]->opr_len, oprP[1]->opr_len),
		data[0]);
            break;

	  case ADFI_536_MIN_DEC:
	    if (MHpkcmp(data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
		      data[0],
			(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec)) < 0)
		MEcopy(data[1], oprP[1]->opr_len, data[0]);
	    break;

	  case ADFI_844_STDDEV_POP_FLT:
	    /* stddev_pop() aggregate. Logic is split between SMAIN and 
	    ** SFINIT. */
	    if (seg == ADE_SMAIN)
	    {
		(*(i4 *)data[3])++;		/* increment counter */
		if (oprP[1]->opr_len > 5) f1 = *(f8 *)data[1];
		 else f1 = *(f4 *)data[1];
		*(f8 *)data[0] += f1;
		*(f8 *)data[2] += f1*f1;
		break;
	    }
	    else if (seg == ADE_SFINIT) /* actually, it must be SFINIT */
	    {
		/* Check nullability, then compute result value. */
		if (*(i4 *)data[3] == 0)
		{
	    	    /* Count is zero - result is null. */
		    if (oprP[0]->opr_dt > 0)
		    {
			return(adu_error(adf_scb,
			    E_AD1012_NULL_TO_NONNULL, 0));
		    }
		    /* Set NULL bit & implicitly clear SING bit */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		    break;
		}
		else if (oprP[0]->opr_dt < 0)
		{
		    /* Clear the null indicator. */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
		}

		f0 = *(f8 *)data[1];
		f1 = *(f8 *)data[2];
		tempf = (f8)(*(i4 *)data[3]);

		/* stddev_pop = 
		**   sqrt((sum(x**2)-sum(x)**2/count(x))/count(x)). */
		tempf1 = f1 - f0*f0/tempf;
		if (tempf1 < 0)
		    tempf1 = -tempf1;
		*(f8 *)data[0] = sqrt(tempf1 / tempf);
		break;
	    }

	  case ADFI_845_STDDEV_SAMP_FLT:
	    /* stddev_samp() aggregate. Logic is split between SMAIN and 
	    ** SFINIT. */
	    if (seg == ADE_SMAIN)
	    {
		(*(i4 *)data[3])++;		/* increment counter */
		if (oprP[1]->opr_len > 5) f1 = *(f8 *)data[1];
		 else f1 = *(f4 *)data[1];
		*(f8 *)data[0] += f1;
		*(f8 *)data[2] += f1*f1;
		break;
	    }
	    else if (seg == ADE_SFINIT) /* actually, it must be SFINIT */
	    {
		/* Check nullability, then compute result value. */
		if (*(i4 *)data[3] <= 1)
		{
	    	    /* Count is zero - result is null. */
		    if (oprP[0]->opr_dt > 0)
		    {
			return(adu_error(adf_scb,
			    E_AD1012_NULL_TO_NONNULL, 0));
		    }
		    /* Set NULL bit & implicitly clear SING bit */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		    break;
		}
		else if (oprP[0]->opr_dt < 0)
		{
		    /* Clear the null indicator. */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
		}

		f0 = *(f8 *)data[1];
		f1 = *(f8 *)data[2];
		tempf = (f8)(*(i4 *)data[3]);

		/* stddev_samp = 
		**   sqrt((sum(x**2)-sum(x)**2/count(x))/(count(x)-1)). */
		tempf1 = f1 - f0*f0/tempf;
		if (tempf1 < 0)
		    tempf1 = -tempf1;
		*(f8 *)data[0] = sqrt(tempf1 / (tempf-1));
		break;
	    }

	  case ADFI_846_VAR_POP_FLT:
	    /* var_pop() aggregate. Logic is split between SMAIN and 
	    ** SFINIT. */
	    if (seg == ADE_SMAIN)
	    {
		(*(i4 *)data[3])++;		/* increment counter */
		if (oprP[1]->opr_len > 5) f1 = *(f8 *)data[1];
		 else f1 = *(f4 *)data[1];
		*(f8 *)data[0] += f1;
		*(f8 *)data[2] += f1*f1;
		break;
	    }
	    else if (seg == ADE_SFINIT) /* actually, it must be SFINIT */
	    {
		/* Check nullability, then compute result value. */
		if (*(i4 *)data[3] == 0)
		{
	    	    /* Count is zero - result is null. */
		    if (oprP[0]->opr_dt > 0)
		    {
			return(adu_error(adf_scb,
			    E_AD1012_NULL_TO_NONNULL, 0));
		    }
		    /* Set NULL bit & implicitly clear SING bit */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		    break;
		}
		else if (oprP[0]->opr_dt < 0)
		{
		    /* Clear the null indicator. */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
		}

		f0 = *(f8 *)data[1];
		f1 = *(f8 *)data[2];
		tempf = (f8)(*(i4 *)data[3]);

		/* varpop = (sum(x**2)-sum(x)**2/count(x))/count(x). */
		*(f8 *)data[0] = (f1 - f0*f0/tempf) / tempf;
		break;
	    }

	  case ADFI_847_VAR_SAMP_FLT:
	    /* var_samp() aggregate. Logic is split between SMAIN and 
	    ** SFINIT. */
	    if (seg == ADE_SMAIN)
	    {
		(*(i4 *)data[3])++;		/* increment counter */
		if (oprP[1]->opr_len > 5) f1 = *(f8 *)data[1];
		 else f1 = *(f4 *)data[1];
		*(f8 *)data[0] += f1;
		*(f8 *)data[2] += f1*f1;
		break;
	    }
	    else if (seg == ADE_SFINIT) /* actually, it must be SFINIT */
	    {
		/* Check nullability, then compute result value. */
		if (*(i4 *)data[3] <= 1)
		{
	    	    /* Count is zero - result is null. */
		    if (oprP[0]->opr_dt > 0)
		    {
			return(adu_error(adf_scb,
			    E_AD1012_NULL_TO_NONNULL, 0));
		    }
		    /* Set NULL bit & implicitly clear SING bit */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = ADF_NVL_BIT;
		    break;
		}
		else if (oprP[0]->opr_dt < 0)
		{
		    /* Clear the null indicator. */
		    *((u_char *)data[0] + oprP[0]->opr_len - 1) = 0;
		}

		f0 = *(f8 *)data[1];
		f1 = *(f8 *)data[2];
		tempf = (f8)(*(i4 *)data[3]);

		/* var_samp = (sum(x**2)-sum(x)**2/count(x))/(count(x)-1). */
		*(f8 *)data[0] = (f1 - f0*f0/tempf) / (tempf-1);
		break;
	    }

	  case ADFI_108_SUM_F:
	    /* Just roll the total. */
	    EXmath ((i4)EX_ON);
	    if (oprP[1]->opr_len >= 8) 
		f0 = *(f8 *)data[1];
	    else f0 = *(f4 *)data[1];
	    if (oprP[0]->opr_len >= 8)
		*(f8 *)data[0] += f0;
	    else *(f4 *)data[0] += f0;
	    EXmath ((i4)EX_OFF);
	    break;

	  case ADFI_109_SUM_I:
	    /* Roll the total and check for overflow. */
#ifdef LP64
	    if (oprP[0]->opr_len >= 8) ba0 = *(i8 *)data[0];
	     else if (oprP[0]->opr_len >= 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 3 ||
		    oprP[0]->opr_len == 2 && oprP[0]->opr_dt > 0) 
			ba0 = *(i2 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len >= 8) ba1 = *(i8 *)data[1];
	     else if (oprP[1]->opr_len >= 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 3 ||
		    oprP[1]->opr_len == 2 && oprP[1]->opr_dt > 0) 
			ba1 = *(i2 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    EXmath ((i4)EX_ON);
	    ba0 = MHi8add(ba0, ba1);
	    if (oprP[0]->opr_len >= 8)
		*(i8 *)data[0] = ba0;
	    else *(i4 *)data[0] = ba0;
	    EXmath((i4)EX_OFF);
#else
	    bo0 = bo1 = FALSE;
	    if (oprP[0]->opr_len >= 8)
		bo0 = TRUE;
	    if (oprP[1]->opr_len >= 8)
	    {
		ba1 = *(i8 *)data[1];
		bo1 = TRUE;
	    }
	     else if (oprP[1]->opr_len >= 4) a1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 3 ||
		    oprP[1]->opr_len == 2 && oprP[1]->opr_dt > 0) 
			a1 = *(i2 *)data[1];
	     else a1 = I1_CHECK_MACRO(*(i1 *)data[1]);
	    if (bo0 && !bo1)
	    {
		ba1 = (i8)a1;
		bo1 = TRUE;
	    }

	    EXmath ((i4)EX_ON);
	    if (!bo0 && bo1)
	    {
                /* Downgrading to i4 from i8 */
                if(ba1 > MAXI4 || ba1 < MINI4)
                    EXsignal(EXINTOVF,  0);
		a1 = (i4)ba1;
		bo1 = FALSE;
	    }
	    if (!bo1)
		*(i4 *)data[0] = MHi4add(*(i4 *)data[0], a1);
	    else *(i8 *)data[0] = MHi8add(*(i8 *)data[0], ba1);
	    EXmath((i4)EX_OFF);
#endif
	    break;

	  case ADFI_110_SUM_MONEY:
	    /* Just roll the total. */
	    EXmath ((i4)EX_ON);
	    *(f8 *)data[0] += ((AD_MONEYNTRNL *)data[1])->mny_cents;
	    EXmath ((i4)EX_OFF);
	    if (((AD_MONEYNTRNL *)data[0])->mny_cents > AD_1MNY_MAX_NTRNL)
		return (adu_error(adf_scb, E_AD5031_MAXMNY_OVFL, 0));
	    else if (((AD_MONEYNTRNL *)data[0])->mny_cents < AD_3MNY_MIN_NTRNL)
		return (adu_error(adf_scb, E_AD5032_MINMNY_OVFL, 0));
	    break;

	  case ADFI_465_SUM_DATE:
	    /* Individual date fields must be accumulated, then the result
	    ** normalized. */
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[1], &dnag1))
		return (db_stat);
	    if (db_stat = adu_6to_dtntrnl (adf_scb, &dv[2], &dnag2))
		return (db_stat);

	    if (dnag2.dn_status & AD_DN_ABSOLUTE)
	    {
		/* Can only sum intervals, not absolute dates. */
		adf_scb->adf_warncb.ad_anywarn_cnt++;
		adf_scb->adf_warncb.ad_agabsdate_cnt++;
		break;
	    }

	    dnag1.dn_status |= dnag2.dn_status;
	    i = dnag2.dn_status;
	    if (i & AD_DN_YEARSPEC) 
		dnag1.dn_year += dnag2.dn_year;
	    if (i & AD_DN_MONTHSPEC) 
		dnag1.dn_month += dnag2.dn_month;
	    if (i & AD_DN_DAYSPEC) 
		dnag1.dn_day += dnag2.dn_day;
	    if (i & AD_DN_TIMESPEC) 
	    {
		i4 over;
		dnag1.dn_nsecond += dnag2.dn_nsecond;
		if (over = dnag1.dn_nsecond / AD_49DTE_INSPERS)
		    dnag1.dn_nsecond -= over * AD_49DTE_INSPERS;
		dnag1.dn_seconds += dnag2.dn_seconds + over;
	    }
	    /* Normalize running total. */
	    adu_2normldint(&dnag1);
	    if (db_stat = adu_7from_dtntrnl (adf_scb, &dv[1], &dnag1))
		return (db_stat);
	    break;

	  case ADFI_537_SUM_DEC:
	    /* Copy out current total. Then add new value and place result
	    ** back in running total. */

	/* copy one less byte if datatype nullable */
	    MEcopy((PTR)data[0], oprP[0]->opr_dt < 0 ? \
                (oprP[0]->opr_len - 1) : \
                oprP[0]->opr_len, (PTR)tempd1.dec);

	    tprec1 = DB_MAX_DECPREC;
	    tscale1 = (i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec);
	    MHpkadd((PTR)data[1],
                    (i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
                    (i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
                (PTR)tempd1.dec,
                    (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
                    (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
                (PTR)data[0],
                    &tprec1,
                    &tscale1);
	    break;
#endif   /* ADE_BACKEND */

	  /* COMPARISONS -- Note that for the comparisons, there is no output */
	  /* operand, so the 1 or 2 input operands are oprP[0], and oprP[1].  */
	  /* ---------------------------------------------------------------- */

          case ADFI_006_I_EQ_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 == ba1);
	    break;

          case ADFI_014_I_NE_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 != ba1);
	    break;

          case ADFI_030_I_GT_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 > ba1);
	    break;

          case ADFI_038_I_GE_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 >= ba1);
	    break;

          case ADFI_046_I_LT_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 < ba1);
	    break;

          case ADFI_022_I_LE_I:
	    if (oprP[0]->opr_len == 4) ba0 = *(i4 *)data[0];
	     else if (oprP[0]->opr_len == 2) ba0 = *(i2 *)data[0];
	     else if (oprP[0]->opr_len == 8) ba0 = *(i8 *)data[0];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[0]);

	    if (oprP[1]->opr_len == 4) ba1 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba1 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba1 = *(i8 *)data[1];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (ba0 <= ba1);
	    break;

	  case ADFI_510_DEC_EQ_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) == 0);
	    break;
	    
	  case ADFI_511_DEC_NE_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) != 0);
	    break;
	    
	  case ADFI_515_DEC_LT_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) < 0);
	    break;
	    
	  case ADFI_512_DEC_LE_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) <= 0);
	    break;
	    
	  case ADFI_513_DEC_GT_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) > 0);
	    break;
	    
	  case ADFI_514_DEC_GE_DEC:
	    cx_value = (MHpkcmp((PTR)data[0],
				(i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
				(PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec)) >= 0);
	    break;
	    
          case ADFI_003_F_EQ_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 == f1);
            }
            else
            {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
                    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
		    fltovf();
                cx_value = (f4_0 == f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_004_F_EQ_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 == f1);
            break;

          case ADFI_005_I_EQ_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 == f1);
            break;

          case ADFI_011_F_NE_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 != f1);
            }
	    else
            {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
                    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
                    fltovf();
                cx_value = (f4_0 != f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_012_F_NE_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 != f1);
            break;

          case ADFI_013_I_NE_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 != f1);
            break;

          case ADFI_043_F_LT_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 < f1);
            }
	    else
            {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
		    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
		    fltovf();
                cx_value = (f4_0 < f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_044_F_LT_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 < f1);
            break;

          case ADFI_045_I_LT_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 < f1);
            break;

          case ADFI_019_F_LE_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 <= f1);
            }
	    else
            {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
                    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
                    fltovf();
                cx_value = (f4_0 <= f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_020_F_LE_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 <= f1);
            break;

          case ADFI_021_I_LE_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 <= f1);
            break;

          case ADFI_027_F_GT_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 > f1);
            }
            else
            {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
                    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
                    fltovf();
                cx_value = (f4_0 > f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_028_F_GT_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 > f1);
            break;

          case ADFI_029_I_GT_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 > f1);
            break;

          case ADFI_035_F_GE_F:
            if (oprP[0]->opr_len != 4 && oprP[1]->opr_len != 4)
            {
                /* both are f8, do f8 comparison */
                f0 = *(f8 *)data[0];
                f1 = *(f8 *)data[1];
                cx_value = (f0 >= f1);
            }
            else
	    {
                EXmath(EX_ON);
                if (oprP[0]->opr_len == 4)
                    f4_0 = (f4) *(f4 *)data[0];
                else if (CV_ABS_MACRO(*(f8 *)data[0]) <= FLT_MAX)
                    f4_0 = (f4) *(f8 *)data[0];
                else
                    fltovf();
                if (oprP[1]->opr_len == 4)
                    f4_1 = (f4) *(f4 *)data[1];
                else if (CV_ABS_MACRO(*(f8 *)data[1]) <= FLT_MAX)
                    f4_1 = (f4) *(f8 *)data[1];
                else
                    fltovf();
                cx_value = (f4_0 >= f4_1);
                EXmath(EX_OFF);
            }
            break;

          case ADFI_036_F_GE_I:
            if (oprP[0]->opr_len == 4) f0 = *(f4 *)data[0];
	    else f0 = *(f8 *)data[0];

	    if (oprP[1]->opr_len == 4) f1 = *(i4 *)data[1];
	   	 else if (oprP[1]->opr_len == 2) f1 = *(i2 *)data[1];
	   	 else if (oprP[1]->opr_len == 8) f1 = *(i8 *)data[1];
	   	 else f1 = I1_CHECK_MACRO(*(i1 *)data[1]);

            cx_value = (f0 >= f1);
            break;

          case ADFI_037_I_GE_F:
	    if (oprP[0]->opr_len == 4) f0 = *(i4 *)data[0];
	   	 else if (oprP[0]->opr_len == 2) f0 = *(i2 *)data[0];
	   	 else if (oprP[0]->opr_len == 8) f0 = *(i8 *)data[0];
	   	 else f0 = I1_CHECK_MACRO(*(i1 *)data[0]);

            if (oprP[1]->opr_len == 4) f1 = *(f4 *)data[1];
	    else f1 = *(f8 *)data[1];

            cx_value = (f0 >= f1);
            break;

          case ADFI_001_TEXT_EQ_TEXT:
          case ADFI_234_CHAR_EQ_CHAR:
          case ADFI_235_VARCHAR_EQ_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL
		** pattern matching allows use of fast path comparison. */
		strOp = 2;
		goto strCompare;
	    }

          case ADFI_000_C_EQ_C:
          case ADFI_983_NCHAR_EQ_NCHAR:
          case ADFI_985_NVCHAR_EQ_NVCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
            break;

	  case ADFI_1347_NUMERIC_EQ:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
            break;

          case ADFI_1425_BOO_EQ_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = ( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype ==
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_1426_BOO_NE_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = !( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype ==
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_1427_BOO_LE_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = ( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype <=
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_1428_BOO_GT_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = ( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype >
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_1429_BOO_GE_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = ( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype >=
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_1430_BOO_LT_BOO:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            ad0_opr2dv(base_array, oprP[1], &dv[2]);
            cx_value = ( ((DB_ANYTYPE *)dv[1].db_data)->db_booltype <
                         ((DB_ANYTYPE *)dv[2].db_data)->db_booltype );
            break;

          case ADFI_009_TEXT_NE_TEXT:
	  case ADFI_236_CHAR_NE_CHAR:
          case ADFI_237_VARCHAR_NE_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL, no UTF8
		** pattern matching allows use of fast path comparison. */
		strOp = 5;
		goto strCompare;
	    }

          case ADFI_008_C_NE_C:
          case ADFI_982_NCHAR_NE_NCHAR:
          case ADFI_984_NVCHAR_NE_NVCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
            break;

	  case ADFI_1348_NUMERIC_NE:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
            break;

          case ADFI_041_TEXT_LT_TEXT:
          case ADFI_244_CHAR_LT_CHAR:
          case ADFI_245_VARCHAR_LT_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL, no UTF8
		** pattern matching allows use of fast path comparison. */
		strOp = 1;
		goto strCompare;
	    }

          case ADFI_040_C_LT_C:
          case ADFI_986_NCHAR_LT_NCHAR:
          case ADFI_988_NVCHAR_LT_NVCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp < 0);
            break;

	  case ADFI_1349_NUMERIC_LT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp < 0);
            break;

          case ADFI_017_TEXT_LE_TEXT:
          case ADFI_238_CHAR_LE_CHAR:
          case ADFI_239_VARCHAR_LE_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL, no UTF8
		** pattern matching allows use of fast path comparison. */
		strOp = 3;
		goto strCompare;
	    }

          case ADFI_016_C_LE_C:
          case ADFI_992_NVCHAR_LE_NVCHAR:
          case ADFI_990_NCHAR_LE_NCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp <= 0);
            break;

	  case ADFI_1352_NUMERIC_LE:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp <= 0);
            break;

          case ADFI_025_TEXT_GT_TEXT:
          case ADFI_240_CHAR_GT_CHAR:
          case ADFI_241_VARCHAR_GT_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL, no UTF8
		** pattern matching allows use of fast path comparison. */
		strOp = 4;
		goto strCompare;
	    }

          case ADFI_024_C_GT_C:
          case ADFI_993_NVCHAR_GT_NVCHAR:
          case ADFI_991_NCHAR_GT_NCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp > 0);
            break;

	  case ADFI_1351_NUMERIC_GT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
            cx_value = (cmp > 0);
            break;

          case ADFI_033_TEXT_GE_TEXT:
          case ADFI_242_CHAR_GE_CHAR:
          case ADFI_243_VARCHAR_GE_VARCHAR:
	    if (!(adf_scb->adf_collation || pm_quel ||
			Adf_globs->Adi_status & ADI_DBLBYTE ||
			(adf_scb->adf_utf8_flag & AD_UTF8_ENABLED) &&
			oprP[0]->opr_collID != DB_UCS_BASIC_COLL &&
                        oprP[1]->opr_collID != DB_UCS_BASIC_COLL))
	    {
		/* Single byte, no alternate collation, no QUEL, no UTF8
		** pattern matching allows use of fast path comparison. */
		strOp = 6;
		goto strCompare;
	    }

          case ADFI_032_C_GE_C:
          case ADFI_989_NVCHAR_GE_NVCHAR:
          case ADFI_987_NCHAR_GE_NCHAR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
	    cx_value = (cmp >= 0);
            break;

	  case ADFI_1350_NUMERIC_GE:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_lexnumcomp(adf_scb, pm_quel, &dv[1], &dv[2], &cmp))
		return(db_stat);
	    cx_value = (cmp >= 0);
            break;


strCompare:
	    /* Setup operands */
	    c1 = (u_char*)data[0];
	    c2 = (u_char*)data[1];

	    if (oprP[0]->opr_dt == DB_VCH_TYPE ||
		oprP[0]->opr_dt == DB_TXT_TYPE)
	    {
	        ln1 = *(u_i2*)c1;
	        c1 += 2;
		if (ln1 > adf_scb->adf_maxstring)
		    return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    }
	    else
	        ln1 = oprP[0]->opr_len;

	    if (oprP[1]->opr_dt == DB_VCH_TYPE ||
		oprP[1]->opr_dt == DB_TXT_TYPE)
	    {
	        ln2 = *(u_i2*)c2;
	        c2 += 2;
		if (ln2 > adf_scb->adf_maxstring)
		    return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 0));
	    }
	    else
	        ln2 = oprP[1]->opr_len;

	    /* Prepare to do MEcmp() of the common length prefixes. */
	    diff = ln1 - ln2;
	    if (diff > 0)
		cmp = MEcmp(c1, c2, ln2);
	    else cmp = MEcmp(c1, c2, ln1);

	    if (cmp < 0)
		cmp = 1;
	    else if (cmp > 0)
		cmp = 4;
	    else 
	    {
		/* Prefixes were equal - check the rest (if any). */
		cmp = 2;

		if (diff > 0)
		{
		    /* c1 is longer - check it for trailing blanks. */
		    c1 += ln2;
		    c2 = c1 + diff;	/* loop terminator */
		    while (c1 < c2 && *c1 == ' ')
			c1++;

		    if (c1 < c2)
			cmp = (*c1 > ' ') ? 4 : 1;
		}
		else
		{
		    /* c2 is longer - check it for trailing blanks. */
		    c2 += ln1;
		    c1 = c2 - diff;	/* loop terminator */
		    while (c2 < c1 && *c2 == ' ')
			c2++;

		    if (c2 < c1)
			cmp = (*c2 > ' ') ? 1 : 4;
		}
	    }

	    cx_value = ((cmp & strOp) > 0);	
	    break;	/* end of fast path character comparisons */

          case ADFI_246_C_LIKE_C:
          case ADFI_247_CHAR_LIKE_CHAR:
          case ADFI_248_TEXT_LIKE_TEXT:
          case ADFI_249_VARCHAR_LIKE_VARCHAR:
          case ADFI_1000_NVCHAR_LIKE_NVCHAR:
          case ADFI_1376_LVCH_LIKE_VCH:
	  case ADFI_1378_LNVCHR_LIKE_NVCHR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
	    if (db_stat = adu_like_all(adf_scb, &dv[1], &dv[2],
			esc_ptr, pat_flags, &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
	    esc_ptr = NULL;	/* turn off esc char if there was one */
	    pat_flags = 0;
            break;

          case ADFI_250_C_NOTLIKE_C:
          case ADFI_251_CHAR_NOTLIKE_CHAR:
          case ADFI_252_TEXT_NOTLIKE_TEXT:
          case ADFI_253_VARCHAR_NOTLIKE_VARCHA:
          case ADFI_1001_NVCHAR_NOTLIKE_NVCHAR:
	  case ADFI_1377_LVCH_NOTLIKE_VCH:
	  case ADFI_1379_LNVCHR_NOTLIKE_NVCHR:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
	    ad0_opr2dv(base_array, oprP[1], &dv[2]);
            if (db_stat = adu_like_all(adf_scb, &dv[1], &dv[2],
			esc_ptr, pat_flags, &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
	    esc_ptr = NULL;	/* turn off esc char if there was one */
	    pat_flags = 0;
            break;

	  case ADFI_1393_PATCOMP_CHA:
	  case ADFI_1392_PATCOMP_VCH:
	    ad0_opr2dv(base_array, oprP[0], &dv[0]);
	    ad0_opr2dv(base_array, oprP[1], &dv[1]);
	    if (db_stat = adu_like_comp(adf_scb, &dv[1], &dv[0],
			esc_ptr, pat_flags))
		return(db_stat);
	    esc_ptr = NULL;	/* turn off esc char if there was one */
	    pat_flags = 0;
            break;

	  case ADFI_1394_PATCOMP_NVCHR:
	    ad0_opr2dv(base_array, oprP[0], &dv[0]);
	    ad0_opr2dv(base_array, oprP[1], &dv[1]);
            if (db_stat = adu_like_comp_uni(adf_scb, &dv[1], &dv[0],
			esc_ptr, pat_flags))
		return(db_stat);
	    esc_ptr = NULL;	/* turn off esc char if there was one */
	    pat_flags = 0;
            break;

	  case ADFI_1431_BOO_ISUNKN:
	  case ADFI_516_ALL_ISNULL:
	    if (oprP[0]->opr_dt < 0)
		cx_value = ((*((u_char *)data[0] + oprP[0]->opr_len - 1)
				    & ADF_NVL_BIT) != 0);
	    else
		cx_value = ADE_NOT_TRUE;
            break;

	  case ADFI_1432_BOO_NOUNKN:
	  case ADFI_517_ALL_ISNOTNULL:
	    if (oprP[0]->opr_dt < 0)
		cx_value = ((*((u_char *)data[0] + oprP[0]->opr_len - 1)
				    & ADF_NVL_BIT) == 0);
	    else
		cx_value = ADE_TRUE;
            break;

	  case ADFI_1357_ALL_ISINT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isinteger(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
            break;

	  case ADFI_1358_ALL_ISNOTINT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isinteger(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
            break;

	  case ADFI_1359_ALL_ISDEC:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isdecimal(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
            break;

	  case ADFI_1360_ALL_ISNOTDEC:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isdecimal(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
            break;

	  case ADFI_1361_ALL_ISFLT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isfloat(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp != 0);
            break;

	  case ADFI_1362_ALL_ISNOTFLT:
	    ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_isfloat(adf_scb, &dv[1], &cmp))
		return(db_stat);
            cx_value = (cmp == 0);
            break;

         case ADFI_1418_BOO_ISFALSE:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_bool_isfalse(adf_scb, &dv[1], &cmp))
                return db_stat;
            cx_value = (cmp != 0);
            break;

         case ADFI_1419_BOO_NOFALSE:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_bool_isfalse(adf_scb, &dv[1], &cmp))
                return db_stat;
            cx_value = (cmp == 0);
            break;

         case ADFI_1420_BOO_ISTRUE:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_bool_istrue(adf_scb, &dv[1], &cmp))
                return db_stat;
            cx_value = (cmp != 0);
            break;

         case ADFI_1421_BOO_NOTRUE:
            ad0_opr2dv(base_array, oprP[0], &dv[1]);
            if (db_stat = adu_bool_istrue(adf_scb, &dv[1], &cmp))
                return db_stat;
            cx_value = (cmp == 0);
            break;


	  /* MATH FUNCTIONS */
	  /* -------------- */

          case ADFI_124_COS_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHcos(f0);
	    if (oprP[0]->opr_len == 4)
		*(f4 *)data[0] = f0;
	    else
		*(f8 *)data[0] = f0;
	    break;

          case ADFI_1308_ACOS_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHacos(f0);
	    if (oprP[0]->opr_len == 4)
		*(f4 *)data[0] = f0;
	    else
		*(f8 *)data[0] = f0;
	    break;

          case ADFI_1310_TAN_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHtan(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_120_ATAN_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHatan(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_1315_ATAN2_F_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
            f0 = MHatan2(f0, f1);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_166_LOG_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHln(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_179_SIN_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHsin(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_1309_ASIN_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHasin(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_182_SQRT_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHsqrt(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

          case ADFI_134_EXP_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            f0 = MHexp(f0);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

	  case ADFI_1311_PI:
	    f0 = 3.14159265358979323846;
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = f0;
            else
                *(f8 *)data[0] = f0;
            break;

	  case ADFI_1312_SIGN_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (ba0 < 0)
		tempi = -1;
	    else if (ba0 == 0)
		tempi = 0;
	    else tempi = 1;

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempi;
	     else *(i1 *)data[0] = tempbi;
	    break;

	  case ADFI_1313_SIGN_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];

	    if (f0 < 0)
		tempi = -1;
	    else if (f0 == 0)
		tempi = 0;
	    else tempi = 1;

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempi;
	     else *(i1 *)data[0] = tempbi;
	    break;

	  case ADFI_1314_SIGN_DEC:
	    {
	      char temp_char = 0x0c;
	      tempi= MHpkcmp((PTR)data[1],
				(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
				(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
				&temp_char, 1, 0);
	    }
	    if (tempi < 0)
		tempi = -1;
	    else if (tempi > 0)
		tempi = 1;

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempi;
	     else *(i1 *)data[0] = tempbi;
	    break;

          case ADFI_111_ABS_F:
            if (oprP[1]->opr_len == 4)
            {
                if (*(f4 *)data[1] < 0)
                    *(f4 *)data[0] = -(*(f4 *)data[1]);
                else if ( *(f4 *)data[1] == 0 )
                {
                    /* Bug 122482 
                    ** The float in data[1] may have the negative bit set
                    ** even though it is zero. To prevent it being copied
                    ** over, use a fresh 0.
                    */
                    *(f4 *)data[0] = 0.0;
                }
                else
                    *(f4 *)data[0] = *(f4 *)data[1];
            }
            else
            {
                if (*(f8 *)data[1] < 0)
                    *(f8 *)data[0] = -(*(f8 *)data[1]);
                else if (*(f8 *)data[1] == 0)
                {
                    /* Bug 122482 
                    ** The float in data[1] may have the negative bit set
                    ** even though it is zero. To prevent it being copied
                    ** over, use a fresh 0.
                    */
                    *(f8 *)data[0] = 0;
                }
                else
                    *(f8 *)data[0] = *(f8 *)data[1];
            }
            break;

          case ADFI_075_PLUS_F:	    /* This case is probably avoided by OPF */
            if (oprP[1]->opr_len == 4)
                *(f4 *)data[0] = *(f4 *)data[1];
            else
                *(f8 *)data[0] = *(f8 *)data[1];
            break;

          case ADFI_078_MINUS_F:
            if (oprP[1]->opr_len == 4)
                *(f4 *)data[0] = -(*(f4 *)data[1]);
            else
                *(f8 *)data[0] = -(*(f8 *)data[1]);
            break;

	  case ADFI_538_ABS_DEC:
	    MHpkabs(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[0]);
	    break;

	  case ADFI_530_PLUS_DEC:
	    MEcopy((PTR)data[1], oprP[1]->opr_len, (PTR)data[0]);
	    break;

	  case ADFI_531_MINUS_DEC:
	    MHpkneg(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[0]);
	    break;

          case ADFI_112_ABS_I:
	    if (oprP[1]->opr_len == 4)
		{
                if (*(i4 *)data[1] < 0)
                    *(i4 *)data[0] = -(*(i4 *)data[1]);
                else
                    *(i4 *)data[0] = *(i4 *)data[1];
                if (*(i4 *)data[0] < 0)
                 EXsignal(EXINTOVF, (i4) 0);
		}
	     else if (oprP[1]->opr_len == 2)
		{
                if (*(i2 *)data[1] < 0)
                    *(i2 *)data[0] = -(*(i2 *)data[1]);
                else
                    *(i2 *)data[0] = *(i2 *)data[1];
                if (*(i2 *)data[0] < 0)
                 EXsignal(EXINTOVF, (i4) 0);
		}
	     else if (oprP[1]->opr_len == 8)
		{
                if (*(i8 *)data[1] < 0)
                    *(i8 *)data[0] = -(*(i8 *)data[1]);
                else
                    *(i8 *)data[0] = *(i8 *)data[1];
                if (*(i8 *)data[0] < 0)
                 EXsignal(EXINTOVF, (i4) 0);
		}
	     else
		{
                /*
                ** can't just check for < 0 and negate
                ** on 3b5 as it has unsigned chars.
                */

                if (I1_CHECK_MACRO(*(i1 *)data[1]) < 0)
                    *(i1 *)data[0] = ~(*(i1 *)data[1]) + 1;
                else
                    *(i1 *)data[0] = *(i1 *)data[1];
                if (I1_CHECK_MACRO(*(i1 *)data[0]) < 0)
                 EXsignal(EXINTOVF, (i4) 0);
		}

            break;

          case ADFI_076_PLUS_I:	    /* This case is probably avoided by OPF */
	    if (oprP[1]->opr_len == 4) *(i4 *)data[0] = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) *(i2 *)data[0] = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) *(i8 *)data[0] = *(i8 *)data[1];
	     else
                /*
                ** Is this OK on 3b5 ?
                */

                *(i1 *)data[0] = *(i1 *)data[1];
            break;

          case ADFI_079_MINUS_I:
	    if (oprP[1]->opr_len == 4) *(i4 *)data[0] = -(*(i4 *)data[1]);
	     else if (oprP[1]->opr_len == 2) *(i2 *)data[0] = -(*(i2 *)data[1]);
	     else if (oprP[1]->opr_len == 8) *(i8 *)data[0] = -(*(i8 *)data[1]);
	     else
                /*
                ** can't just negate on 3b5
                */

                *(i1 *)data[0] = ~(*(i1 *)data[1]) + 1;
            break;



	  /* ARITHMETIC OPERATIONS */
	  /* --------------------- */

          case ADFI_049_F_ADD_F:
            if (oprP[1]->opr_len == 8)
                f0 = *(f8 *)data[1];
            else
                f0 = *(f4 *)data[1];
            if (oprP[2]->opr_len == 8)
                f1 = *(f8 *)data[2];
            else
                f1 = *(f4 *)data[2];
            tempf = f0 + f1;
            if (oprP[0]->opr_len == 8)
	    {
		if (!MHdfinite(tempf)) fltovf();
                *(f8 *)data[0] = tempf;
	    }
            else
	    {
		ad0_fltchk( tempf, oprP[0]);
                *(f4 *)data[0] = tempf;
	    }
            break;

          case ADFI_056_F_SUB_F:
            if (oprP[1]->opr_len == 8)
                f0 = *(f8 *)data[1];
            else
                f0 = *(f4 *)data[1];
            if (oprP[2]->opr_len == 8)
                f1 = *(f8 *)data[2];
            else
                f1 = *(f4 *)data[2];
            tempf = f0 - f1;
            if (oprP[0]->opr_len == 8)
	    {
		if (!MHdfinite(tempf)) fltovf();
                *(f8 *)data[0] = tempf;
	    }
            else
	    {
		ad0_fltchk( tempf, oprP[0]);
                *(f4 *)data[0] = tempf;
	    }
            break;

          case ADFI_059_F_MUL_F:
            if (oprP[1]->opr_len == 8)
                f0 = *(f8 *)data[1];
            else
                f0 = *(f4 *)data[1];
            if (oprP[2]->opr_len == 8)
                f1 = *(f8 *)data[2];
            else
                f1 = *(f4 *)data[2];
            tempf = f0 * f1;
            if (oprP[0]->opr_len == 8)
	    {
		if (!MHdfinite(tempf)) fltovf();
                *(f8 *)data[0] = tempf;
	    }
            else
	    {
		ad0_fltchk( tempf, oprP[0]);
                *(f4 *)data[0] = tempf;
	    }
            break;

	  case ADFI_521_F_MUL_DEC:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
	    CVpkf(  (PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
		    &f1);
            tempf = f0 * f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_410_F_MUL_I:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];

   	    if (oprP[2]->opr_len == 4) f1 = *(i4 *)data[2];
   	     else if (oprP[2]->opr_len == 2) f1 = *(i2 *)data[2];
   	     else if (oprP[2]->opr_len == 8) f1 = *(i8 *)data[2];
             else f1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempf = f0 * f1;
	    ad0_fltchk( tempf, oprP[0]);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_066_F_DIV_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
	    if( f1 == 0.0)
	    {
		EXsignal(EXFLTDIV, 0);
		tempf = 0.0;
	    }
	    else
		tempf = f0 / f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

	  case ADFI_526_F_DIV_DEC:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
	    CVpkf(  (PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
		    &f1);
	    if( f1 == 0.0)
	    {
		EXsignal(EXFLTDIV, 0);
		tempf = 0.0;
	    }
	    else
		tempf = f0 / f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_412_F_DIV_I:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];

   	    if (oprP[2]->opr_len == 4) f1 = *(i4 *)data[2];
   	     else if (oprP[2]->opr_len == 2) f1 = *(i2 *)data[2];
   	     else if (oprP[2]->opr_len == 8) f1 = *(i8 *)data[2];
             else f1 = I1_CHECK_MACRO(*(i1 *)data[2]);

	    if( f1 == 0.0)
	    {
		EXsignal(EXFLTDIV, 0);
		tempf = 0.0;
	    }
	    else
		tempf = f0 / f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_073_F_POW_F:
            if (oprP[1]->opr_len == 4)
                f0 = *(f4 *)data[1];
            else
                f0 = *(f8 *)data[1];
            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];

/*******************************************************************************
**  Depend on MH to signal MH_BADARG, and adx_handler() to deal with it.
**
**	    if (f0 <  0.0  ||  (f0 == 0.0  &&  f1 <= 0.0))
**		return(adu_error(adf_scb, E_AD6000_BAD_MATH_ARG, 0));
*******************************************************************************/

            tempf = MHpow(f0,f1);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

	  case ADFI_518_DEC_ADD_DEC:
	    MHpkadd(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)data[0],
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    break;

	  case ADFI_519_DEC_SUB_DEC:
	    MHpksub(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)data[0],
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    break;

	  case ADFI_520_DEC_MUL_DEC:
	    MHpkmul(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)data[0],
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    break;

	  case ADFI_522_DEC_MUL_F:
	    CVpkf(  (PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
		    &f0);
            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
            tempf = f0 * f1;
	    ad0_fltchk( tempf, oprP[0]);
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_524_DEC_MUL_I:
	    bo1 = FALSE;
	    if (oprP[2]->opr_len == 4)
		{
                a1 = *(i4 *)data[2];
		tprec1  = AD_I4_TO_DEC_PREC;
		tscale1 = AD_I4_TO_DEC_SCALE;
		}
	     else if (oprP[2]->opr_len == 2)
		{
                a1 = *(i2 *)data[2];
		tprec1  = AD_I2_TO_DEC_PREC;
		tscale1 = AD_I2_TO_DEC_SCALE;
		}
	     else if (oprP[2]->opr_len == 8)
		{
                ba1 = *(i8 *)data[2];
		bo1 = TRUE;
		tprec1  = AD_I8_TO_DEC_PREC;
		tscale1 = AD_I8_TO_DEC_SCALE;
		}
	     else
		{
                a1 = I1_CHECK_MACRO(*(i1 *)data[2]);
		tprec1  = AD_I1_TO_DEC_PREC;
		tscale1 = AD_I1_TO_DEC_SCALE;
		}
	    /* Don't check status since we know it will fit */
	    if (!bo1)
	      _VOID_ CVlpk(a1, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	      else _VOID_ CVl8pk(ba1, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	    MHpkmul(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)tempd1.dec,
			(i4)tprec1,
			(i4)tscale1,
			(PTR)tempd2.dec,
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    if (CVpkpk((PTR)tempd2.dec,
		       (i4)tprec2,
		       (i4)tscale2,
		       (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
		       (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
		       (PTR)data[0]) == CV_OVERFLOW)
	    {
		EXsignal(EXDECOVF, 0);
	    }
            break;

	  case ADFI_525_DEC_DIV_DEC:
	    MHpkdiv(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)data[0],
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    break;

	  case ADFI_527_DEC_DIV_F:
	    CVpkf(  (PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
		    &f0);
            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
	    if( f1 == 0.0)
	    {
		EXsignal(EXFLTDIV, 0);
		tempf = 0.0;
	    }
	    else
		tempf = f0 / f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_529_DEC_DIV_I:
	    bo1 = FALSE;
	    if (oprP[2]->opr_len == 4)
		{
                a1 = *(i4 *)data[2];
		tprec1  = AD_I4_TO_DEC_PREC;
		tscale1 = AD_I4_TO_DEC_SCALE;
		}
	     else if (oprP[2]->opr_len == 2)
		{
                a1 = *(i2 *)data[2];
		tprec1  = AD_I2_TO_DEC_PREC;
		tscale1 = AD_I2_TO_DEC_SCALE;
		}
	     else if (oprP[2]->opr_len == 8)
		{
                ba1 = *(i8 *)data[2];
		bo1 = TRUE;
		tprec1  = AD_I8_TO_DEC_PREC;
		tscale1 = AD_I8_TO_DEC_SCALE;
		}
	     else
		{
                a1 = I1_CHECK_MACRO(*(i1 *)data[2]);
		tprec1  = AD_I1_TO_DEC_PREC;
		tscale1 = AD_I1_TO_DEC_SCALE;
		}
	    /* Don't check status since we know it will fit */
	    if (!bo1)
	      _VOID_ CVlpk(a1, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	      else _VOID_ CVl8pk(ba1, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	    MHpkdiv(	(PTR)data[1],
			(i4)DB_P_DECODE_MACRO(oprP[1]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[1]->opr_prec),
			(PTR)tempd1.dec,
			(i4)tprec1,
			(i4)tscale1,
			(PTR)tempd2.dec,
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    if (CVpkpk((PTR)tempd2.dec,
		       (i4)tprec2,
		       (i4)tscale2,
		       (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
		       (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
		       (PTR)data[0]) == CV_OVERFLOW)
	    {
		EXsignal(EXDECOVF, 0);
	    }
            break;

          case ADFI_169_MOD_I_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (oprP[2]->opr_len == 4) ba1 = *(i4 *)data[2];
	     else if (oprP[2]->opr_len == 2) ba1 = *(i2 *)data[2];
	     else if (oprP[2]->opr_len == 8) ba1 = *(i8 *)data[2];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempbi = ba0 % ba1;

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempbi;
	     else *(i1 *)data[0] = tempbi;
            break;

          case ADFI_050_I_ADD_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (oprP[2]->opr_len == 4) ba1 = *(i4 *)data[2];
	     else if (oprP[2]->opr_len == 2) ba1 = *(i2 *)data[2];
	     else if (oprP[2]->opr_len == 8) ba1 = *(i8 *)data[2];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempbi = MHi8add(ba0, ba1);

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempbi;
	     else *(i1 *)data[0] = tempbi;
            break;

          case ADFI_057_I_SUB_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (oprP[2]->opr_len == 4) ba1 = *(i4 *)data[2];
	     else if (oprP[2]->opr_len == 2) ba1 = *(i2 *)data[2];
	     else if (oprP[2]->opr_len == 8) ba1 = *(i8 *)data[2];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempbi = MHi8sub(ba0, ba1);

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempbi;
	     else *(i1 *)data[0] = tempbi;
            break;

          case ADFI_060_I_MUL_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (oprP[2]->opr_len == 4) ba1 = *(i4 *)data[2];
	     else if (oprP[2]->opr_len == 2) ba1 = *(i2 *)data[2];
	     else if (oprP[2]->opr_len == 8) ba1 = *(i8 *)data[2];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempbi = MHi8mul(ba0, ba1);

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempbi;
	     else *(i1 *)data[0] = tempbi;
            break;

          case ADFI_523_I_MUL_DEC:
	    bo0 = FALSE;
	    if (oprP[1]->opr_len == 4)
		{
                a0 = *(i4 *)data[1];
		tprec1  = AD_I4_TO_DEC_PREC;
		tscale1 = AD_I4_TO_DEC_SCALE;
		}
	     else if (oprP[1]->opr_len == 2)
		{
                a0 = *(i2 *)data[1];
		tprec1  = AD_I2_TO_DEC_PREC;
		tscale1 = AD_I2_TO_DEC_SCALE;
		}
	     else if (oprP[1]->opr_len == 8)
		{
                ba0 = *(i8 *)data[1];
		bo0 = TRUE;
		tprec1  = AD_I8_TO_DEC_PREC;
		tscale1 = AD_I8_TO_DEC_SCALE;
		}
	     else
		{
                a0 = I1_CHECK_MACRO(*(i1 *)data[1]);
		tprec1  = AD_I1_TO_DEC_PREC;
		tscale1 = AD_I1_TO_DEC_SCALE;
		}
	    /* Don't check status since we know it will fit */
	    if (!bo0)
	      _VOID_ CVlpk(a0, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	      else _VOID_ CVl8pk(ba0, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	    MHpkmul(	(PTR)tempd1.dec,
			(i4)tprec1,
			(i4)tscale1,
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)tempd2.dec,
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    if (CVpkpk((PTR)tempd2.dec,
		       (i4)tprec2,
		       (i4)tscale2,
		       (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
		       (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
		       (PTR)data[0]) == CV_OVERFLOW)
	    {
		EXsignal(EXDECOVF, 0);
	    }
            break;

          case ADFI_411_I_MUL_F:
   	    if (oprP[1]->opr_len == 4) f0 = *(i4 *)data[1];
   	     else if (oprP[1]->opr_len == 2) f0 = *(i2 *)data[1];
   	     else if (oprP[1]->opr_len == 8) f0 = *(i8 *)data[1];
             else f0 = I1_CHECK_MACRO(*(i1 *)data[1]);

            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
            tempf = f0 * f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

          case ADFI_067_I_DIV_I:
	    if (oprP[1]->opr_len == 4) ba0 = *(i4 *)data[1];
	     else if (oprP[1]->opr_len == 2) ba0 = *(i2 *)data[1];
	     else if (oprP[1]->opr_len == 8) ba0 = *(i8 *)data[1];
	     else ba0 = I1_CHECK_MACRO(*(i1 *)data[1]);

	    if (oprP[2]->opr_len == 4) ba1 = *(i4 *)data[2];
	     else if (oprP[2]->opr_len == 2) ba1 = *(i2 *)data[2];
	     else if (oprP[2]->opr_len == 8) ba1 = *(i8 *)data[2];
	     else ba1 = I1_CHECK_MACRO(*(i1 *)data[2]);

            tempbi = MHi8div(ba0, ba1);

	    if (oprP[0]->opr_len == 8) *(i8 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 4) *(i4 *)data[0] = tempbi;
	     else if (oprP[0]->opr_len == 2) *(i2 *)data[0] = tempbi;
	     else *(i1 *)data[0] = tempbi;
            break;

          case ADFI_528_I_DIV_DEC:
	    bo0 = FALSE;
	    if (oprP[1]->opr_len == 4)
		{
                a0 = *(i4 *)data[1];
		tprec1  = AD_I4_TO_DEC_PREC;
		tscale1 = AD_I4_TO_DEC_SCALE;
		}
	     else if (oprP[1]->opr_len == 2)
		{
                a0 = *(i2 *)data[1];
		tprec1  = AD_I2_TO_DEC_PREC;
		tscale1 = AD_I2_TO_DEC_SCALE;
		}
	     else if (oprP[1]->opr_len == 8)
		{
                ba0 = *(i8 *)data[1];
		bo0 = TRUE;
		tprec1  = AD_I8_TO_DEC_PREC;
		tscale1 = AD_I8_TO_DEC_SCALE;
		}
	     else
		{
                a0 = I1_CHECK_MACRO(*(i1 *)data[1]);
		tprec1  = AD_I1_TO_DEC_PREC;
		tscale1 = AD_I1_TO_DEC_SCALE;
		}
	    /* Don't check status since we know it will fit */
	    if (!bo0)
	      _VOID_ CVlpk(a0, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	      else _VOID_ CVlpk(ba0, (i4)tprec1, (i4)tscale1, (PTR)tempd1.dec);
	    MHpkdiv(	(PTR)tempd1.dec,
			(i4)tprec1,
			(i4)tscale1,
			(PTR)data[2],
			(i4)DB_P_DECODE_MACRO(oprP[2]->opr_prec),
			(i4)DB_S_DECODE_MACRO(oprP[2]->opr_prec),
			(PTR)tempd2.dec,
			(i4 *)&tprec2,
			(i4 *)&tscale2);
	    if (CVpkpk((PTR)tempd2.dec,
		       (i4)tprec2,
		       (i4)tscale2,
		       (i4)DB_P_DECODE_MACRO(oprP[0]->opr_prec),
		       (i4)DB_S_DECODE_MACRO(oprP[0]->opr_prec),
		       (PTR)data[0]) == CV_OVERFLOW)
	    {
		EXsignal(EXDECOVF, 0);
	    }
            break;

          case ADFI_413_I_DIV_F:
   	    if (oprP[1]->opr_len == 4) f0 = *(i4 *)data[1];
   	     else if (oprP[1]->opr_len == 2) f0 = *(i2 *)data[1];
   	     else if (oprP[1]->opr_len == 8) f0 = *(i8 *)data[1];
             else f0 = I1_CHECK_MACRO(*(i1 *)data[1]);

            if (oprP[2]->opr_len == 4)
                f1 = *(f4 *)data[2];
            else
                f1 = *(f8 *)data[2];
	    if( f1 == 0.0)
	    {
		EXsignal(EXFLTDIV, 0);
		tempf = 0.0;
	    }
	    else
		tempf = f0 / f1;
	    ad0_fltchk( tempf, oprP[0]);	    
            if (oprP[0]->opr_len == 4)
                *(f4 *)data[0] = tempf;
            else
                *(f8 *)data[0] = tempf;
            break;

	  case ADFI_1474_SINGLECHK_ALL:
	    /* Also see ADFI_1463_SINGLETON_ALL aggregate and ADE_SINGLETON */
	    if (oprP[1]->opr_dt > 0)
		/* Input should be nullable */
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0,
			"singlechk p1 not nullable"));

	    /* Raise error if SING bit set */
	    if ((data[1])[oprP[1]->opr_len - 1] & ADF_SING_BIT)
		return db_stat = adu_error(adf_scb, E_AD1028_NOT_ZEROONE_ROWS, 0);
	    /* Otherwise we need to copy the input. Note that output might be non-nullable */
	    if (oprP[0]->opr_dt > 0)
	    {
		if ((data[1])[oprP[1]->opr_len - 1] & ADF_NVL_BIT)
		    return db_stat = adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0);
	    }
	    /* As lenspec ADI_O1 is used, the output length should be used
	    ** - if not nullable, will simply drop the flags byte */
	    MEcopy(data[1], oprP[0]->opr_len, data[0]);
	    break;

	  case ADFI_595_ROWCNT_I:        /* ii_row_count */
	  {
            /* Get input values */
	    switch (oprP[1]->opr_len)
	    {
		case 1:
		    a0 = I1_CHECK_MACRO(*(i1 *)data[1]);
		    break;
		case 2: 
		    a0 = *(i2 *)data[1];
		    break;
		case 4:
		    a0 = *(i4 *)data[1];
		    break;
		case 8:
		    tempbi  = *(i8 *)data[1];

		    /* limit to i4 values */
		    if ( tempbi > MAXI4 || tempbi < MINI4LL )
			return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount p1 overflow"));

		    a0 = (i4) tempbi;
		    break;
		default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount p1 length"));
	    }

	    switch (oprP[2]->opr_len)
	    {
		case 1:
		    a1 = I1_CHECK_MACRO(*(i1 *)data[2]);
		    break;
		case 2: 
		    a1 = *(i2 *)data[2];
		    break;
		case 4:
		    a1 = *(i4 *)data[2];
		    break;
		case 8:
		    tempbi  = *(i8 *)data[2];

		    /* limit to i4 values */
		    if ( tempbi > MAXI4 || tempbi < MINI4LL )
			return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount p2 overflow"));

		    a1 = (i4) tempbi;
		    break;
		default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount p2 length"));
	    }

            /* If a1 is TRUE, then we want to initialize the value to 0 */
            if( a1 == TRUE)
                tempi = 0;
            else
                tempi = a0 + 1;

            /* Write output to both oprP[0] and oprP[1] */
	    switch (oprP[0]->opr_len)
	    {
		case 1:
		    *(i1 *)data[0] = tempi;
		    break;
		case 2: 
		    *(i2 *)data[0] = tempi;
		    break;
		case 4:
		    *(i4 *)data[0] = tempi;
		    break;
		case 8:
		    *(i8 *)data[0] = tempi;
		    break;
		default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount output0 length"));
	    }

	    switch (oprP[1]->opr_len)
	    {
		case 1:
		    *(i1 *)data[1] = tempi;
		    break;
		case 2: 
		    *(i2 *)data[1] = tempi;
		    break;
		case 4:
		    *(i4 *)data[1] = tempi;
		    break;
		case 8:
		    *(i8 *)data[1] = tempi;
		    break;
		default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "rowcount output1 length"));
	    }

            break;
	  }

	  /* Statistics, Environment, etc. ... (non-query constants) */
	  /* ------------------------------------------------------- */
          case ADFI_211_DBA:
          case ADFI_596_SYSUSER:
          case ADFI_618_SESSUSER:
          case ADFI_651_INITUSER:
          case ADFI_212_USERNAME:
          case ADFI_210__VERSION:
	    {
		DB_DATA_VALUE	pdv;
		DB_DATA_VALUE	rdv;
		i4 opcode = f->ins_icode;
		DB_TEXT_STRING *str = (DB_TEXT_STRING *) &tempd1.inbuf[0];

		pdv.db_datatype = DB_VCH_TYPE;
		pdv.db_prec   = 0;
		if (opcode == ADFI_211_DBA)
		    STcopy("dba",(char *) &str->db_t_text[0]);
		else if (opcode == ADFI_596_SYSUSER)
		    STcopy("system_user",(char *) &str->db_t_text[0]);
		else if (opcode == ADFI_618_SESSUSER)
		    STcopy("session_user",(char *) &str->db_t_text[0]);
		else if (opcode == ADFI_651_INITUSER)
		    STcopy("initial_user",(char *) &str->db_t_text[0]);
		else if (opcode == ADFI_212_USERNAME)
		    STcopy("username",(char *) &str->db_t_text[0]);
		else if (opcode == ADFI_210__VERSION)
		    STcopy("_version",(char *) &str->db_t_text[0]);
		str->db_t_count = STlength((char *)str->db_t_text);
		pdv.db_length = str->db_t_count + DB_CNTSIZE;
		pdv.db_data     = (PTR) str;

		rdv.db_datatype = DB_VCH_TYPE;
		rdv.db_prec	= 0;
		rdv.db_length   = sizeof(tempd2.outbuf);
		rdv.db_data     = (PTR) &tempd2.outbuf[0];

		if (db_stat = adu_dbmsinfo(adf_scb, &pdv, &rdv))
		    return(db_stat);

		ad0_opr2dv(base_array, oprP[0], &pdv);
		if (db_stat = adu_ascii(adf_scb, &rdv, &pdv))
		    return(db_stat);
	    }
            break;

	  /* Statistics, Environment, etc. ... (query constants) */
	  /* --------------------------------------------------- */
          case ADFI_194__BINTIM_I:
          case ADFI_442__BINTIM:
          case ADFI_197__CPU_MS:
          case ADFI_198__ET_SEC:
          case ADFI_199__DIO_CNT:
          case ADFI_200__BIO_CNT:
          case ADFI_201__PFAULT_CNT:
          case ADFI_1233_CURRENT_DATE:
          case ADFI_1234_CURRENT_TIME:
          case ADFI_1235_CURRENT_TMSTMP:
          case ADFI_1236_LOCAL_TIME:
          case ADFI_1237_LOCAL_TMSTMP:
	    {
		ADK_MAP *qconst;
		i4 opcode = f->ins_icode;

		ad0_opr2dv(base_array, oprP[0], &dv[0]);
		if (opcode == ADFI_194__BINTIM_I || opcode == ADFI_442__BINTIM)
		    qconst = &Adf_globs->Adk_bintim_map;
		else if (opcode == ADFI_197__CPU_MS)
		    qconst = &Adf_globs->Adk_cpu_ms_map;
		else if (opcode == ADFI_198__ET_SEC)
		    qconst = &Adf_globs->Adk_et_sec_map;
		else if (opcode == ADFI_199__DIO_CNT)
		    qconst = &Adf_globs->Adk_dio_cnt_map;
		else if (opcode == ADFI_200__BIO_CNT)
		    qconst = &Adf_globs->Adk_bio_cnt_map;
		else if (opcode == ADFI_201__PFAULT_CNT)
		    qconst = &Adf_globs->Adk_pfault_cnt_map;
		else if (opcode == ADFI_1233_CURRENT_DATE)
		    qconst = &Adf_globs->Adk_curr_date_map;
		else if (opcode == ADFI_1234_CURRENT_TIME)
		    qconst = &Adf_globs->Adk_curr_time_map;
		else if (opcode == ADFI_1235_CURRENT_TMSTMP)
		    qconst = &Adf_globs->Adk_curr_tstmp_map;
		else if (opcode == ADFI_1236_LOCAL_TIME)
		    qconst = &Adf_globs->Adk_local_time_map;
		else if (opcode == ADFI_1237_LOCAL_TMSTMP)
		    qconst = &Adf_globs->Adk_local_tstmp_map;

		if ( (db_stat = adu_dbconst(adf_scb, qconst, &dv[0])) )
		    return(db_stat);
	    }
            break;
  
	  /* Statistics, Environment, etc. ... (no longer supported) */
	  /* ------------------------------------------------------- */
          case ADFI_202__WS_PAGE:
          case ADFI_203__PHYS_PAGE:
          case ADFI_204__CACHE_REQ:
          case ADFI_205__CACHE_READ:
          case ADFI_206__CACHE_WRITE:
          case ADFI_207__CACHE_RREAD:
          case ADFI_208__CACHE_SIZE:
          case ADFI_209__CACHE_USED:
            *(i4 *)data[0] = 0;		/* No longer supported */
	    break;
  

#ifndef ADE_BACKEND
	  /* Special case for count(*) aggregate */
	  /* ----------------------------------- */

          case ADFI_396_COUNTSTAR:
	    agstruct_ptr = (ADF_AG_STRUCT *) data[0];
	    agstruct_ptr->adf_agcnt++; /* Increment counter in the ag_struct */
	    break;
#endif

	  default:

	    /*========================================================*/
	    /*                                                        */
	    /*   At this point, we have exhausted all instructions    */
	    /*   that this routine knows how to perform.  Therefore,  */
	    /*   the instruction to be executed must be some other    */
	    /*   ADF function instance.  Now we find it and execute   */
	    /*   the appropriate routine.                             */
	    /*                                                        */
	    /*========================================================*/

#ifdef    xDEBUG
	    if (    f->ins_icode < 0
		||  f->ins_icode > Adf_globs->Adi_fi_biggest
		||
		Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(f->ins_icode)].adi_fi
				    == NULL
	       )
		return(adu_error(adf_scb, E_AD2010_BAD_FIID, 0));
#endif  /* xDEBUG */

	    if (Adf_globs->Adi_fi_lkup[
			    ADI_LK_MAP_MACRO(f->ins_icode)
					    ].adi_func == NULL)
	    {
		/* find function name from optab */
		ADI_OP_NAME	tmp_opname;

		fdesc = (ADI_FI_DESC*)Adf_globs->Adi_fi_lkup[
						ADI_LK_MAP_MACRO(f->ins_icode)
								].adi_fi;
		_VOID_ adi_opname(adf_scb, fdesc->adi_fiopid, &tmp_opname);

		return(adu_error(adf_scb, E_AD8999_FUNC_NOT_IMPLEMENTED, 2,
				    (i4) 0,
				    (i4 *) &tmp_opname.adi_opname[0]
				)
		      );
	    }
/*  #endif /* xDEBUG */


	    /* Get the function instance description for this instruction */
	    /* ---------------------------------------------------------- */

	    fdesc = (ADI_FI_DESC*)Adf_globs->Adi_fi_lkup[
					ADI_LK_MAP_MACRO(f->ins_icode)
							    ].adi_fi;
	    func = Adf_globs->Adi_fi_lkup[
				ADI_LK_MAP_MACRO(f->ins_icode)
					    ].adi_func;

	    /* Is the f.i. a comparison? */
	    /* ------------------------- */

	    if (fdesc->adi_fitype == ADI_COMPARISON)
	    {
		ad0_opr2dv(base_array, oprP[0], &dv[1]);
		ad0_opr2dv(base_array, oprP[1], &dv[2]);
		if (db_stat = (*func)(adf_scb, &dv[1], &dv[2], &cmp))
		{
		    if (db_stat == E_DB_WARN)
			final_stat = E_DB_WARN;
		    else
			return(db_stat);
		}
		switch (fdesc->adi_fiopid)
		{
		  case ADI_EQ_OP:
		  case ADI_LIKE_OP:
		    cx_value = (cmp == 0);
		    break;
		  case ADI_NE_OP:
		  case ADI_NLIKE_OP:
		    cx_value = (cmp != 0);
		    break;
		  case ADI_LT_OP:
		    cx_value = (cmp < 0);
		    break;
		  case ADI_LE_OP:
		    cx_value = (cmp <= 0);
		    break;
		  case ADI_GT_OP:
		    cx_value = (cmp > 0);
		    break;
		  case ADI_GE_OP:
		    cx_value = (cmp >= 0);
		    break;
		}
	    }

#ifdef    ADE_BACKEND
	    /* Else, is the f.i. an aggregate? */
	    /* ------------------------------- */

	    else if (fdesc->adi_fitype == ADI_AGG_FUNC)
		if (fdesc->adi_fiflags & ADI_F16_OLAPAGG)
	    {
		olapstruct_ptr = (ADF_AG_OLAP *) data[0];
		/* These are all binary aggregates and have 2 arguments. */
		ad0_opr2dv(base_array, oprP[1], &dv[1]);
		ad0_opr2dv(base_array, oprP[2], &dv[2]);
		db_stat = (*func)(adf_scb, &dv[1], &dv[2], olapstruct_ptr);
		if (db_stat)
		 if (db_stat == E_DB_WARN) final_stat = E_DB_WARN;
		 else return(db_stat);
	    }
		else if ((abs(oprP[0]->opr_dt) >= ADI_DT_UDT_MIN) ||
                         (fdesc->adi_finstid >= ADD_USER_FISTART))
	    {
		/* UDT aggregates. For now we assume result is not
		** the dopey aggregate work structure. */
		ad0_opr2dv(base_array, oprP[0], &dv[0]);
		ad0_opr2dv(base_array, oprP[1], &dv[1]);
		db_stat = (*func)(adf_scb, &dv[0], &dv[1]);
		if (db_stat)
		 if (db_stat == E_DB_WARN) final_stat = E_DB_WARN;
		 else return(db_stat);
	    }
		else	/* Agg, but not OLAP */
	    {
		/* This should not actually be executed since all non-OLAP
		** aggregates have inline "case"s. But for historic or
		** possibly unanticipated future reasons ... */
		agstruct_ptr = (ADF_AG_STRUCT *) data[0];
		ad0_opr2dv(base_array, oprP[1], &dv[1]);
		db_stat = (*func)(adf_scb, &dv[1], agstruct_ptr);
		if (db_stat)
		 if (db_stat == E_DB_WARN) final_stat = E_DB_WARN;
		 else return(db_stat);
	    }
#endif /* ADE_BACKEND */
	    else
	    {
		/*
		** Else, we do have a function to call "normally."
		**
		** To cater to varargs user functions, and functions
		** that require a workspace, assume that the ADE
		** instruction is correct -- i.e. has the proper number
		** of arguments.  The 0th arg is the result.
		*/

		i4      numargs = f->ins_nops;
		i4	li;
		DB_DATA_VALUE * dvp = &dv[0];
		DB_DATA_VALUE *dvpp[ADI_MAX_OPERANDS+2];
		DB_DATA_VALUE **p_dvpp = &dvpp[0];

		/* Fill in dv[] array, and build dvpp[] array of pointers
		** to dv's (only needed if COUNTVEC but easier to do than
		** to test).
		*/
		for (li = 0; li < numargs; li++)
		{
		    ad0_opr2dv(base_array, oprP[li], dvp);
		    *p_dvpp++ = dvp;
		    ++dvp;
		}

		if (fdesc->adi_fiflags & ADI_F2_COUNTVEC)
		    db_stat = (*func)(adf_scb, numargs, &dvpp[0]);
		else
		    switch (numargs)
		    {
		    case 1:
		        db_stat = (*func)(adf_scb, &dv[0]);
			break;
		    case 2:
		        db_stat = (*func)(adf_scb, &dv[1], &dv[0]);
			break;
		    case 3:
		        db_stat = (*func)(adf_scb, &dv[1], &dv[2], &dv[0]);
		    
	    /* {@fix_me@} */
	    /* There is a problem with VLTs.  When executing an ADE_CALCLEN to
	    ** generate the result length for a VLT, it is sometimes greater
	    ** than the VLUP/VLT input's length (depending on the lenspec of
	    ** the operator about to be executed).  When this happens, the NULL
	    ** byte (assuming the result VLT is nullable) is put in the normal
	    ** place for NULL bytes: db_data[db_length-1].  But the rule for
	    ** VLUPs/VLTs is that the NULL byte should be placed after the end
	    ** of the string (which is indicated by the count field): count +
	    ** size of count (currently = 2) + 1 bytes from the base address.
	    ** A temporary fix to this problem is implemented below; the idea
	    ** here is to move the NULL byte from the normal place to the
	    ** position expected for VLTs.  The correct way to fix this problem
	    ** is to re-architect ADE to use self-describing operands rather
	    ** than the LEN_UNKNOWN mechanism.
	    */
		        if ( ( db_stat == E_DB_OK || db_stat == E_DB_WARN) &&
			     (ADI_ISNULL_MACRO(&dv[0]) == FALSE) )
			    adg_vlup_setnull(&dv[0]);
			break;
		    case 4:
		        db_stat = (*func)(adf_scb, &dv[1], &dv[2],
						&dv[3], &dv[0]);
			break;
		    case 5:
		        db_stat = (*func)(adf_scb, &dv[1], &dv[2],
						&dv[3], &dv[4], &dv[0]);
			break;
		    default:
		        return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR,
						2,0,"adex nops"));
		    }
		if (db_stat)
		{	
		    if (db_stat == E_DB_WARN)
		        final_stat = E_DB_WARN;
		    else
		        return(db_stat);
		}

		/* check for infix predicate "function" operation */
		/* There aren't any of these at present, ifdef it out */
#ifdef ADZ_PRED_FUNC_TY_FIIDX
		if (fdesc->adi_fitype == ADI_PRED_FUNC &&
			dv[0].db_datatype == DB_INT_TYPE)
		 switch (dv[0].db_length)
		 {
		  case 2:
		    cx_value = *((i2 *)dv[0].db_data);
		    break;
		  case 4:
		    cx_value = *((i4 *)dv[0].db_data);
		    break;
		 }
#endif

	    }
	    /*
	    ** If we have just processed a large object-type datatype,
	    ** then we must see if we are done, incomplete, or just generally
	    ** happy.
	    **
	    ** If we are complete, then readjust our offset pointers and keep
	    ** going.  Otherwise, keep going.  The stuff at the beginning of
	    ** this loop (waaaaaaaaaaaaaaaaaaaaaaaaaay back there) will finish
	    ** up correctly.
	    */

	    if (old_lbase_size)
	    {
		lbase_size = old_lbase_size - dv[0].db_length;
	    
		if (final_stat == E_DB_OK)
		{
		    element_length = dv[0].db_length;
		    ade_excb->excb_continuation = 0;
		}
	    }
        }	/* END OF SWITCH STATEMENT */
	if ((final_stat == E_DB_INFO)
		&& (adf_scb->adf_errcb.ad_errcode == E_AD0002_INCOMPLETE))
	{
	    break;
	}

        if (is_fancy && f->ins_nops > 0 && 
            f->ins_icode != ADE_COMPAREN  &&
            f->ins_icode != ADE_NCOMPAREN && 
            oprP[0]->opr_base == lbase)
	{
	    offset_accumulator += element_length;
	}
    }	    /* END OF WHILE LOOP */


    /* Since we have fallen through, we must be TRUE -- let's return */
    /* ------------------------------------------------------------- */
/* {@fix_me@} */
#ifdef    ADE_BACKEND
    EXmath((i4) EX_OFF);
#endif /* ADE_BACKEND */

    ade_excb->excb_value = cx_value;
    ade_excb->excb_continuation = 0;
    if (is_fancy)
    {
	if ((final_stat == E_DB_INFO)
	    && (adf_scb->adf_errcb.ad_errcode == E_AD0002_INCOMPLETE))
	{
	    ade_excb->excb_continuation = this_instr_offset;
	}
	if ((lbase != ADE_NOBASE) && (lbase_size > 0))
	    ade_excb->excb_size -= lbase_size;
    }
    return(final_stat);
}


/*
** Name: ad0_opr2dv() - Build a DB_DATA_VALUE from an operand.
**
** Description:
**      Takes an ADE_OPERAND and build a corresponding DB_DATA_VALUE from it.
**
** Inputs:
**      bases                           Arrary of base addresses being used to
**					calculate actual memory addresses from
**					the operands base and offset.
**      op                              Ptr to the operand.
**
** Outputs:
**      dv                              Ptr to the DB_DATA_VALUE built.
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-jul-86 (thurston)
**          Initial creation and coding.
**	14-jul-89 (jrb)
**	    Set db_prec to correct precision.
**	20-dec-04 (inkdo01)
**	    Init db_collID.
*/

static VOID
ad0_opr2dv(
PTR                bases[],
ADE_OPERAND        *op,
DB_DATA_VALUE      *dv)
{
    dv->db_datatype = op->opr_dt;
    dv->db_prec	    = op->opr_prec;
    dv->db_length   = op->opr_len;
    dv->db_collID   = op->opr_collID;
    dv->db_data	    = (PTR) ((char *) bases[op->opr_base] + op->opr_offset);
}


/*
** Name: ad0_fltchk() - Check and report floating point overflow.
**
** Description:
**      Takes an f8 and ADE_OPERAND and determine if we have a floating
**      point overflow.
**
** Inputs:
**      tempf                           Floating point value
**      op                              Ptr to the operand.
**
** Outputs:
**          none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    EXFLTOVF
**
** Side Effects:
**	    none
**
** History:
**      17-jul-1994 (stevet)
**          Initial creation and coding.
**	28-apr-94 (vijay)
**	    change abs to fabs.
**      01-sep-1994 (mikem)
**          bug #64261
**          Changed calls to fabs() to calls to CV_ABS_MACRO()
**          On some if not all UNIX's fabs() is an actual
**          routine call which takes a double as an argument and returns a
**          double.  Calls to this code did not have proper includes to
**          define the return type so the return code was assumed to be int
**          (basically returning random data), in adumoney.c this caused bug
**          #64261, in other files garbage return values from fabs would cause
**          other problems.
*/
static VOID
ad0_fltchk(
f8                 tempf,
ADE_OPERAND        *op)
{
    if (!MHdfinite (tempf) || 
	(op->opr_len == 4 && CV_ABS_MACRO(tempf) > FLT_MAX))
	EXsignal (EXFLTOVF, 0);
}


#ifdef    ADE_BACKEND

/*
** Name: ad0_fill_vltws() - Build an ADE_VLT_WS_STRUCT array from the CX.
**
** Description:
**      This routine is used to construct the ADE_VLT_WS_STRUCT array from 
**      the ADE_CALCLEN instructions found in the VIRGIN segment of the CX. 
**      Each of these instructions corresponds to one VLT in the CX.  The 
**      array built here will be used by ade_vlt_space() to calculate the 
**      amount of scratch space needed for the VLTs at run time.
**
** Inputs:
**      adf_scb                         Ptr to ADF session control block.
**      cx                              Ptr to the CX.
**      nbases                          Number of base addressess in the
**					bases[] array.
**      bases                           Array of base addresses.
**
** Outputs:
**      nvlts                           Number of VLTs in the vlt work space
**					array.
**      vlt_ws                          VLT work space array pointer address.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD2005_BAD_DTLEN		Bad length for a datatype.
**	    E_AD2020_BAD_LENSPEC	Unknown length spec.
**	    E_AD2021_BAD_DT_FOR_PRINT	Bad datatype for the print stlye
**					length specs.
**
**		The following returns will only happen when the system is
**		compiled with the xDEBUG option:
**
**          E_AD550E_TOO_MANY_VLTS      This CX has more than the max # of VLTs.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-jul-86 (thurston)
**          Initial creation and coding.
**	13-may-87 (thurston)
**	    Changed all end of offset list terminaters to be ADE_END_OFFSET_LIST
**	    instead of zero, since zero is a legal instruction offset for the
**	    FE's.
**	05-feb-88 (thurston)
**	    Did MAJOR work on VLTs.  Aparently, these had never been tested
**	    since there were about 20 things wrong with them.  Not the least of
**	    which is that we need VLT `c' and `char' as well as VLT `text' and
**	    `varchar'.  With the removal of all of the function instance cases
**	    for the LRC proposal (see above), and the reordering of the datatype
**	    resolution hierarchy, VLTs are MUCH more common, thus the problems
**	    arose.
**	10-feb-88 (thurston)
**	    Modified to work off the VIRGIN segment instead of the INIT segment.
**	22-jan-90 (jrb)
**	    Alignment fix from FredP.
**	05-mar-93 (rog)
**	    Fix typo where we were incrementing the datatype if the VLT was
**	    nullable when we should have been incrementing the length. (48645)
**      15-Jul-1993 (fred)
**          Added DB_VBYTE_TYPE to list of specially handled types.
**      13-Mar-1999 (hanal04)
**          Initialise dv elements to ensure bad addresses are not passed
**          to adi_0calclen(). b96344.
*/

static DB_STATUS
ad0_fill_vltws(
ADF_CB             *adf_scb,
PTR                cx,
i4                 nbases,
PTR                bases[],
i4                 *nvlts,
ADE_VLT_WS_STRUCT  **pvlt_ws)
{
    ADE_CXHEAD          *cxhead = (ADE_CXHEAD *) cx;
    i4             nexti;
    ADE_INSTRUCTION     *iptr;
    ADE_I_OPERAND       *optr;
    ADE_OPERAND         oprs[ADI_MAX_OPERANDS+3];
    ADE_OPERAND         *oprP[ADI_MAX_OPERANDS+3];
    DB_DATA_VALUE	dv[ADI_MAX_OPERANDS+2];
    DB_DATA_VALUE	*opptrs[ADI_MAX_OPERANDS];
    ADI_LENSPEC         lenspec;
    i4			i;
    i4			j;
    i4			k;
    i4                 base;
    i4             len;
    i4             status;
    DB_STATUS           db_stat;
    DB_DT_ID		bdt;
    DB_DT_ID		out_bdt;
    ADE_VLT_WS_STRUCT   *vlt_ws = *pvlt_ws; /* We start with ADE_MXVLTS_SOFT */
    bool		vlt_grown = FALSE;

    *nvlts = 0;

    /* If no VLTs in CX, just return */
    /* ----------------------------- */
    if (!cxhead->cx_n_vlts)
	return(E_DB_OK);

    /* b96344 - Initialise all 5 elements of dv */
    MEfill(sizeof(dv), (u_char) '\0', (char *)&dv[0]);

    /* Loop through VIRGIN segment searching for ADE_CALCLEN instructions */
    /* ------------------------------------------------------------------ */
    nexti = cxhead->cx_seg[ADE_SVIRGIN].seg_1st_offset;
    while (nexti != ADE_END_OFFSET_LIST)
    {
	iptr = (ADE_INSTRUCTION *) ((char *) cx + nexti);
	if (iptr->ins_icode == ADE_CALCLEN)
	{
	    if (*nvlts >= ADE_MXVLTS)
		return(adu_error(adf_scb, E_AD550E_TOO_MANY_VLTS, 0));
	    if (vlt_grown == FALSE &&
		*nvlts >= ADE_MXVLTS_SOFT)
	    {
		/* Allocate temporary memory - caller will free */
		PTR tmp = MEreqmem(0, ADE_MXVLTS*sizeof(ADE_VLT_WS_STRUCT), FALSE, &status);
		if (!tmp)
		    return(adu_error(adf_scb, E_AD550E_TOO_MANY_VLTS, 0));
		/* Copy existing */
		MEcopy((PTR)vlt_ws, ADE_MXVLTS_SOFT*sizeof(ADE_VLT_WS_STRUCT), tmp);
		/* Adopt the block as current */
		*pvlt_ws = vlt_ws = (ADE_VLT_WS_STRUCT*)tmp;
		vlt_grown = TRUE;
	    }
	    /* Now let's process the VLT for this ADE_CALCLEN instruction */
	    /* ---------------------------------------------------------- */

	    optr = (ADE_I_OPERAND *) ((char *) iptr + sizeof(ADE_INSTRUCTION));
	    for (i=0, k = 0; i < iptr->ins_nops; i++)
	    {
		/* Loop on the operands for this ADE_CALCLEN instruction */
		/* ----------------------------------------------------- */
		ADE_GET_OPERAND_MACRO(base_array, optr, &oprs[i], &oprP[i]);

		if (i == 0)	/* If operand #0 ... */
		{
		    lenspec.adi_lncompute = oprP[0]->opr_dt;
		    lenspec.adi_fixedsize = oprP[0]->opr_len;
		    lenspec.adi_psfixed   = oprP[0]->opr_prec;
		}
		else		/* ... else; operand #1, #2, etc. */
		{
		    i4	    im1 = i - 1;

		    /* set datatype and prec for operand #i */
		    dv[im1].db_datatype = oprP[i]->opr_dt;
		    bdt = abs(dv[im1].db_datatype);
		    dv[im1].db_prec = oprP[i]->opr_prec;

		    if (im1 != 0)   /* If this is not the output operand */
		    {
	    	        opptrs[k++] = &dv[im1];
			/* set length for operand #i, if not the output */
			base = oprP[i]->opr_base;
			len = oprP[i]->opr_len;	/* if not a VLUP or VLT, then
						** length can just be taken
						** from the operand.
						*/
			if (len == ADE_LEN_UNKNOWN)
			{
			    /* see if this opr is a previously processed VLT */
			    for (j=0; j < *nvlts; j++)
			    {
				if (vlt_ws[j].vlt_base == base)
				{
				    /* if so, get len from the vlt work space */
				    len = vlt_ws[j].vlt_len;
				    if (    bdt != DB_VCH_TYPE
					&&  bdt != DB_VBYTE_TYPE
					&&  bdt != DB_TXT_TYPE
					&&  bdt != DB_LTXT_TYPE
					&&  bdt != DB_NVCHR_TYPE
				       )
				    {
					len -= DB_CNTSIZE;
				    }
				    break;
				}
			    }
			}
			
			/*
			** If length is still ADE_LEN_UNKNOWN then this must be
                        ** a VLUP, so get length from the actual data that the
                        ** base/offset point to ... do not forget to add
                        ** DB_CNTSIZE for the DB_CNTSIZE byte count field at
                        ** the front of the data if variable length type. 
			*/
			if (len != ADE_LEN_UNKNOWN)
			{
			    dv[im1].db_length = len;
			}
			else
			{
			    u_i2	    c;
			    
			    switch (abs(dv[im1].db_datatype))
			    {
			      case DB_VCH_TYPE:
			      case DB_VBYTE_TYPE:
			      case DB_TXT_TYPE:
			      case DB_LTXT_TYPE:
			      case DB_NVCHR_TYPE:
				/* The variable length types */
				dv[im1].db_length =
				I2ASSIGN_MACRO(((DB_TEXT_STRING *)
				  ((char *) bases[base] + oprP[i]->opr_offset))
				  ->db_t_count, c);

				if (abs(dv[im1].db_datatype) == DB_NVCHR_TYPE)
				{
				    c *= sizeof(UCS2);
				}
				dv[im1].db_length = c + DB_CNTSIZE;
				break;

			      default:
				/* all other types */
				I2ASSIGN_MACRO(*(u_i2 *)((char *) bases[base] 
					+ oprP[i]->opr_offset - DB_CNTSIZE), c);
                                if (abs(dv[im1].db_datatype) == DB_NCHR_TYPE)
                                {
                                    c *= sizeof(UCS2);
                                }
				dv[im1].db_length = c;
				break;
			    }
			    if (dv[im1].db_datatype < 0)
				dv[im1].db_length++;
			}
		    }
		}
		optr++;
	    }	/* End of for loop for the ADE_CALCLEN operands */

	    /* Done with the ADE_CALCLEN operands */
	    /* ---------------------------------- */

	    /* Now place the base for the VLT we are processing in the vlt */
	    /* work space, and then call adi_calclen() to find the correct */
	    /* result length for it, and place that in the vlt work space. */
	    /* ----------------------------------------------------------- */
	    vlt_ws[*nvlts].vlt_base = oprP[0]->opr_base;
	    status = E_AD0000_OK;
	    db_stat = adi_0calclen(adf_scb, &lenspec, iptr->ins_nops - 2,
	    		 opptrs, &dv[0]);
	    vlt_ws[*nvlts].vlt_len = dv[0].db_length;

	    if (db_stat)
	        status = adf_scb->adf_errcb.ad_errcode;

	    if (status)
		return(adu_error(adf_scb, status, 0));

	    out_bdt = abs(dv[0].db_datatype);
	    if (    out_bdt != DB_VCH_TYPE
		&&  out_bdt != DB_VBYTE_TYPE
		&&  out_bdt != DB_TXT_TYPE
		&&  out_bdt != DB_LTXT_TYPE
		&&  out_bdt != DB_NVCHR_TYPE
	       )
		vlt_ws[*nvlts].vlt_len += DB_CNTSIZE;

	    /* Increment the # VLTs in the vlt work space */
	    /* ------------------------------------------ */
	    (*nvlts)++;

	}   /* End of if statement for the ADE_CALCLEN instruction */

	/* Done processing the VLT for this ADE_CALCLEN instruction; */
	/* now we can go to the next instruction in the INIT segment */
	/* --------------------------------------------------------- */
	nexti = ADE_GET_OFFSET_MACRO(iptr, nexti, cxhead->cx_ii_last_offset);

    }	/* End of while loop for all instructions in the INIT segment */

    /* Done with all instructions in the INIT segment */
    /* ---------------------------------------------- */

    return(E_DB_OK);
}
#endif /* ADE_BACKEND */


/*
** Name: ade_countstar_loc() - Pick the location of the out buf for count 
**
** Description:
**      This is a simple aggregate optimization which gets the output
**      buffer in which to copy the countstar or count col value as
**      returned by DMR_AGGREGATE. 
**
** Inputs:
**      adf_scb                         Pointer to an ADF session control block.
**          .adf_errcb                  ADF_ERROR struct.
**              .ad_ebuflen             The length, in bytes, of the buffer
**                                      pointed to by ad_errmsgp.
**              .ad_errmsgp             Pointer to a buffer to put formatted
**                                      error message in, if necessary.
**      ade_excb                        Pointer to CX execution control block.
**          .excb_cx                    Pointer to the CX.
**          .excb_seg                   Which segment of the CX to execute.
**                                      This will be either ADE_SVIRGIN,
**                                      ADE_SINIT, ADE_SMAIN, or ADE_SFINIT.
**          .excb_nbases                The # of base addresses in the
**                                      .excb_bases array.
**          .excb_limited_base          Number of the base which may be size
**                                      limited.  May be ADE_NOBASE if no
**                                      base is size limited (i.e. all bases
**                                      are known to be big enough).
**          .excb_size                  Size of the base identified as
**                                      limited.
**          .excb_continuation          Indicates that this is a continuation of
**                                      previous execution.  If this is
**                                      non-zero, then this field contains an
**                                      indicator of which instruction is to be
**                                      continued.  That being the case, this
**                                      field is expected to be unmodified.
**                                      This field should be zero for the first
**                                      execution of any CX.
**              (.excb_value)           This field may have been altered, and
**                                      be preserved if a call is made with a
**                                      nonzero excb_continuation field.
**          .excb_bases                 Array of base addresses.
**
**
** Outputs:
**     data                             address of the out buffer for the count 
**     instr_cnt                        number of middle instructions
**
**      Returns:
**          none
**
**
** Side Effects:
** History:
**      21-aug-95 (ramra01)
**          for simple aggregates that have a countstar or count(<NOT NULL>)
**          column on a base table.
**	18-nov-03 (inkdo01)
**	    If adf_base_array is non-null, use it as base array for 
**	    operand addr resolution, not excb_bases[].
**	27-aug-04 (inkdo01)
**	    Change to pick up global base array addr from local base array.
**	13-Jan-2005 (jenjo02)
**	    Removed deprecated ADFI_? defines from ins_icode check,
**	    always return *instr_cnt;
*/


DB_STATUS
ade_countstar_loc(
ADF_CB             *adf_scb,
ADE_EXCB           *ade_excb,
PTR                *data,
i4		   *instr_cnt)
{
    ADE_CXHEAD      *cxhead = (ADE_CXHEAD *) ade_excb->excb_cx;
    PTR             cxbase = ade_excb->excb_bases[ADE_CXBASE];
    PTR		    *base_array;
    i4         next_instr_offset;
    i4         last_instr_offset;
    ADE_INSTRUCTION *f;
    ADE_I_OPERAND   *opr_internal;
    ADE_OPERAND     oprs;
    ADE_OPERAND     *oprP;
    i4		    mi_count; 
 
    /* Copy a couple of local CX addresses to the global array, if it is
    ** being used. Those entries in the global array have been reserved
    ** for the purpose. 
    */
    if ((base_array = (PTR *)ade_excb->excb_bases[ADE_GLOBALBASE]) != NULL)
    {
	base_array[ADE_CXBASE] = ade_excb->excb_bases[ADE_CXBASE];
	base_array[ADE_VLTBASE] = ade_excb->excb_bases[ADE_VLTBASE];
	base_array[ADE_NULLBASE] = NULL;
	base_array[ADE_LABBASE] = ade_excb->excb_bases[ADE_LABBASE];
    }
    else base_array = ade_excb->excb_bases;

    next_instr_offset = cxhead->cx_seg[ADE_SMAIN].seg_1st_offset;
    last_instr_offset = cxhead->cx_seg[ADE_SMAIN].seg_last_offset;
    f = (ADE_INSTRUCTION *) ((char *)cxbase + next_instr_offset);
    mi_count = cxhead->cx_seg[ADE_SMAIN].seg_n;
    while (mi_count--)
    {
        f = (ADE_INSTRUCTION *)((char *)cxbase + next_instr_offset);
        next_instr_offset = 
		ADE_GET_OFFSET_MACRO(f, next_instr_offset, last_instr_offset);

        if ( f->ins_icode == ADFI_396_COUNTSTAR ||
             f->ins_icode == ADFI_090_COUNT_C_ALL )
        {
            opr_internal = (ADE_I_OPERAND *) ((char *)f
                                        + sizeof(ADE_INSTRUCTION));
            ADE_GET_OPERAND_MACRO(base_array, opr_internal,
                                        &oprs, &oprP);
            *data = (PTR) ((char *) base_array[oprP->opr_base]
                                        + oprP->opr_offset);
	    *instr_cnt = cxhead->cx_seg[ADE_SMAIN].seg_n;
	    break;
        }
    }	
    return(E_DB_OK);
}
 

#ifdef COUNTS
#ifdef ADE_BACKEND
/*
** Name: ade_barf_counts - displays accumulated instruction counts.
**
** Description:
**      This routine displays the accumulated instruction execution 
**	count values from the static count arrays.
**
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-oct-95 (inkdo01)
**          written.
*/
void ade_barf_counts()

{
	i4   i, i1;
	i4   j[3], k[3];
	double total = 0.0;

	i = 0; adf_printf("\n\n ADFI Count Values:\n\n");
	while (i < 1025)
	{
	   for (i1 = 0; i1 < 3 && i < 1025; i1++, i++)
	   {
		while (i < 1000 && adfi_counts[i] == 0) i++;
		if (i < 1000)
		{
		    j[i1] = i;
		    k[i1] = adfi_counts[i];
		    total += k[i1];
		    continue;
		}
		 else while (i <= 1022 && adfi_counts[i] == 0) i++;
		if (i <= 1022)
		{
		    j[i1] = -i;
		    k[i1] = adfi_counts[i];
		    total += k[i1];
		    continue;
		}
		 else while (i < 1027 && adfi_counts[i] == 0) i++;
		if (i < 1027)
		{
		    j[i1] = -i-977;
		    k[i1] = adfi_counts[i];
		    total += k[i1];
		}
	    }

	    if (i >= 1027) i1--;

	    switch (i1) {
		case 1:
		 adf_printf("  <%6d, %7d>\n",j[0],k[0]);
		 break;

		case 2:
		 adf_printf("  <%6d, %7d>,   <%6d, %7d>\n",j[0],k[0],
			j[1],k[1]);
		 break;

		case 3:
		 adf_printf("  <%6d, %7d>,   <%6d, %7d>,  <%6d, %7d>\n",
			j[0],k[0],j[1],k[1],j[2],k[2]);
		 break;
	    }
	}
	adf_printf("\n  Total F.I. Count: %15.0f\n", total);
}

/*
** Name: adf_printf() - Perform a printf through SCC_TRACE
**
** Description:
**      This routine performs the printf function by returning the info to 
**      be displayed through SCC_TRACE. 
**
** Inputs:
**	str -
**	    The control string
**	p1 - p8
**	    parameters 1 to 8 for the control string
**
** Outputs:
**  none
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-oct-95 (inkdo01)
**          copied from adedebug.c
*/
static VOID
adf_printf( str, p1, p2, p3, p4, p5, p6, p7, p8)
char	    *str;
char	    *p1;
char	    *p2;
char	    *p3;
char	    *p4;
char	    *p5;
char	    *p6;
char	    *p7;
char	    *p8;
{
    char	cbuf[500];
    DB_STATUS   db_stat=E_DB_ERROR;
    DB_STATUS   (*func)();


    STprintf(cbuf, str, p1, p2, p3, p4, p5, p6, p7, p8);

    if ((func = Adf_globs->Adi_fexi[ADI_02ADF_PRINTF_FEXI].adi_fcn_fexi) 
	!= NULL)
	db_stat = (*func)(cbuf);

    if(db_stat != E_DB_OK)
    {
	TRdisplay("SCF error displaying an ADF message to user\n");
	TRdisplay("The ADF message is :%s",cbuf);
    }
}
#endif
#endif

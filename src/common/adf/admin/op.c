/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/
#include    <iiadd.h>
#include    <math.h>
#include    <stdio.h>
#include    "udt.h"

/*
**  The following macros are used to move various types of data where the
**  machine environment is not known.  On machines on which data must be
**  properly aligned for the system to operate, the #define ALIGNMENT_REQUIRED
**  should be defined.  (Its value is not important.)  In these cases,
**  the values will be moved byte by byte into a presumably aligned space.
**
**  On machines where such operation is not required, the #define should remain
**  undefined.  This will cause the data to be moved using a standard assignment
**  statement.
*/

#ifdef	ALIGNMENT_REQUIRED

#define		    I2ASSIGN_MACRO(a,b)		byte_copy((char *) &(a), sizeof(short), (char *) &(b))
#define		    I4ASSIGN_MACRO(a,b)		byte_copy((char *) &(a),sizeof(i4), (char *) &(b))
#define		    F4ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(float), (char *) &(b))
#define		    F8ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(double), (char *) &(b))

#else

#define		    I2ASSIGN_MACRO(a,b)		((*(short *)&(b)) = (*(short *)&(a)))
#define		    I4ASSIGN_MACRO(a,b)		((*(i4 *)&(b)) = (*(i4 *)&(a)))
#define		    F4ASSIGN_MACRO(a,b)		((*(float *)&(b)) = (*(float *)&(a)))
#define		    F8ASSIGN_MACRO(a,b)		((*(double *)&(b)) = (*(double *)&(a)))

#endif

#define			FMAX		1E37
#define			FMIN		-(FMAX)

#define			min(a, b)   ((a) <= (b) ? (a) : (b))

/**
**
**  Name: OP.C - Implements the OP datatype and related functions
**
**  Description:
**      This file contains the routines necessary to implement the ordered-pair
**	datatype.  This datatype consists of elements of data which are
**	internally represented as two double's.  These two values represent the
**	(x, y) mapping learned in [junior] high school algebra.
**
**	A variety of functions are provided.  Midpoint takes two OP's,
**	returning an OP which represents the midpoint on a line segment between
**	the two arguments.  Slope takes two OP's, and returns the slope of the
**	line segment drawn from the first to the second.  Distance returns an
**	double representing the distance from the first arg to the second.  The
**	function ord_pair(double, double) returns the ordered pair with x & y
**	coordinates given by the two arguments.
**
**
**
##  History:
##      8-Feb-1989 (fred)
##          Created.
##	30-may-1990 (fred)
##	    Fixed byte alignment problems.  Added callback functions for the
##	    trace message routines.
##	03-jul-1990 (boba)
##	    Include iiadd.h with quotes since it's in this directory
##	    (reintegration of 13-mar-1990 change).
##	08-jul-1991 (seng)
##	    The format for reading in doubles with sscanf() should be %lF,
##	    not %F.  Change made in routine usop_convert().
##	19-aug-1991 (lan)
##	    Added additional externs for new user-defined datatypes, complex
##	    and zchar.  Also removed the following definitions:
##	    - structure definition of ORD_PAIR (udt.h)
##	    - definition of datatype id for ORD_PAIR (udt.h)
##	    - byte_copy (common.c)
##	    - usop_error (common.c)
##	    Also put #ifdef ris_us5 around the call to sscanf() as suggested
##	    by seng.
##      21-apr-1993 (stevet)
##	    Changed <udt.h> back to "udt.h" since this file lives in 
##	    DEMO.
##      09-jun-1993 (stevet)
##          Added check for input db_length for usop_trace(), this 
##          routine failed to return trace message when input length is
##          not 4 (B41123).
##       8-Jul-1993 (fred)
##          Added support for the long zchar datatype.  While not
##          significantly different than long varchar, it demonstrates
##          how the call back routines are used.
##      12-aug-1993 (stevet)
##          Merged PNUM to 65.  Also added LONGTEXT to/from coercions for
##          ZCHAR and CPX.  LONGTEXT coercions are required for all UDTs.
##       1-Nov-1993 (fred)
##          Upgrade datatype definitions to new structure.  See iiadd.h.
##	15-nov-93 (vijay)
##	    Correct AIX scanf format string.
##	05-nov-1993 (swm)
##	    Bug #58879
##	    The ASSIGN macros assume that the respective C types in each
##	    macro are the correct size. On axp_osf a long is 8 bytes, not
##	    4, so in the I4ASSIGN_MACRO the sizeof(long) is replaced with
##	    sizeof(int) for axp_osf. (Is there some obscure reason why the
##	    literal value 4 is not used?)
##      16-feb-1994 (stevet)
##          Added non prototype function declarations.  On HP/UX and
##          SunOS, not everyone have ANSI C compiler.
##	22-mar-1994 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
##	    Add axp_osf to ris_us5 entry to read doubles in sscanf().
##      17-may-94 (mhuishi)
##          Add __ALPHA to ris_us5 entry for sscanf format.
##      09-feb-95 (chech02)
##          Add rs4_us5 to ris_us5 entry for sscanf format. (AIX 4.1 port)
##	2-mar-1995 (shero03)
##	    Bug # 67208
##	    Add seglen and xform routines for LONG ZCHAR.
## 	11-apr-1995 (wonst02)
##	    Bug #68008
## 	    Handle internal DECIMAL type, which looks just like
## 	    the PNUM user-defined type.
##	21-nov-95 (albany)
##	    Updated add_l_ustring (length) of register_block's add_ustring.
##	24-jan-1995 (wadag01)
##          Add sos_us5 to ris_us5 entry for sscanf format (SCO OpenServer 5).
##	27-feb-1996 (smiba01)
##          Add usl_us5 to ris_us5 entry for sscanf format (Unixware 2.0.2)
##	06-mar-1996 (canor01)
##          Add int_wnt entry for sscanf format (Intel Windows NT).
## 	05-feb-1996 (morayf)
##	    Added SNI RMx00 (rmx_us5) to those needing %lf in sscanf.
##	    However, it is the original %F which is non-portable and should
##	    be only used explicitly by the platform which needs it, should
##	    this even exist.
##	01-mar-1996 (morayf)
##	    Made Pyramid (pym_us5) like rmx_us5.
##	08-apr-1996 (tutto01)
##	    Changed int_wnt to NT_GENERIC to include Alpha NT.
##	02-feb-1997 (kamal03)
##	    Splitted #if defined ... statement with continuation into multiple
##	    #if  statements to make VAX C precompiler happy.
##	12-dec-1997 (hweho01)
##          Add dgi_us5 entry for sscanf format (DG/UX Intel). 
##	5-May-1998 (bonro01)
##          Add dg8_us5 entry for sscanf format (DG/UX Motorola). 
##	14-jul-1998 (toumi01)
##	    Add lnx_us5 entry for sscanf format (Linux).
##      23-Sep-1998 (kinte01)
##          Add VMS entry for sscanf format.
##	5-dec-1998 (nanpr01)
##	    Add su4_us5 entry for sscanf format (Sun/Solaris)
##      03-jul-1999 (podni01)
##          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
##	06-oct-1999 (toumi01)
##	    Change Linux config string from lnx_us5 to int_lnx.
##	22-Nov-1999 (hweho01)
##	    Add ris_u64 entry for sscanf format (AIX 64-bit).
##	14-Jun-2000 (hanje04)
##	    Add ibm_lnx entry for sscanf format. (OS/390 Linux)
##	08-Sep-2000 (hanje04)
##	    Add axp_lnx entry for sscanf format (Alpha Linux).
##	11-Jan-2000 (gupsh01)
##	    Move the iiregister to the nvarchar.c.
##      13-Sep-2002 (hanch04)
##          Add su9_us5 entry for sscanf format (Solaris 64-bit).
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##	26-Sep-2002 (hweho01)
##	    Add r64_us5 entry for sscanf format (AIX hybrid build).
##      08-Oct-2002 (hanje04)
##          As part of AMD x86-64 Linux port, replace individual Linux
##          defines with generic LNX define where apropriate
##      07-Jan-2003 (hanch04)  
##          Back out change for bug 108637
##          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
##	07-Dec-2004 (hanje04)
##	    BUG 113057
##	    Use generic __linux__ define for linux.
##	20-Jan-2005 (hanje04)
##	    Re-add fix for BUG 113057 after it was removed by X-integ
##	03-Feb-2005 (bonro01)
##	    Add a64_sol entry for sscanf format Solaris AMD64
##	2-May-2007 (bonro01)
##	    All supported platforms now use %lf in sscanf()
##      24-nov-2009 (joea)
##          Change sizeof(int) to sizeof(char) in function_instances array.
**/

/*}
** Name: ORD_PAIR - An ordered pair
**
** Description:
**      This structure defines an ordered pair in our world.  The two elements,
**	both double's, represent the x & y coordinates of the ordered pair. 
**
## History:
##      8-Feb-1989 (fred)
##          Created.
[@history_template@]...
*/
/*
** 	This typedef is actually defined in "UDT.h".  Included here for
**	anyone's information
**
typedef struct _ORD_PAIR
{
    double              op_x;		/ The x coordinate /
    double		op_y;		/ The y coordinate /
} ORD_PAIR;
**
*/

/*
**  Definition of static variables and forward static functions.
*/

static  II_DT_ID    op_datatype_id = ORP_TYPE;
	II_STATUS   (*Ingres_trace_function)() = 0;


/*{
** Name:    usop_trace -- Send a random trace message
**
** Description:
**	This routine merely sends the supplied text as a trace message
**	based on the first parameter.  This is merely to demonstrate how trace
**	messages are sent.
**
**  Inputs:
**	scb				The scb to fill in
**	dispose_mask			The dispose mask (a db_data_value)
**	trace_string			The  string to send
**
**  Outputs:
**	(none)
## History:
##	13-jun-89 (fred)
##	    Created.
##      09-jun-1993 (stevet)
##          Added check for input db_length (B41123).
*/
II_STATUS
#ifdef __STDC__
usop_trace(II_SCB	   *scb ,
            II_DATA_VALUE	  *dispose_mask ,
            II_DATA_VALUE	  *trace_string ,
            II_DATA_VALUE	  *result )
#else
usop_trace( scb, dispose_mask, trace_string, result)
II_SCB	          *scb;
II_DATA_VALUE	  *dispose_mask;
II_DATA_VALUE	  *trace_string;
II_DATA_VALUE	  *result;
#endif
{
    i4  res = II_ERROR;
    int	     dm;
    short    sdm;

    if (Ingres_trace_function)
    {
        switch (dispose_mask->db_length)
        {
	  case 1:
	    dm = *dispose_mask->db_data;
            break;
 
          case 2:
	    I2ASSIGN_MACRO(*dispose_mask->db_data, sdm);
	    dm = sdm;
            break;
	  case 4:
	    I4ASSIGN_MACRO(*dispose_mask->db_data, dm);
	    break;
        }
	res = (*Ingres_trace_function)(dm,
			trace_string->db_length,
			trace_string->db_data);
    }
    I4ASSIGN_MACRO(res, *result->db_data);
    return(II_OK);
}

/*{
** Name: op_compare	- Compare two OP's
**
** Description:
**      This routine compares two OP's.  The following rules apply.
**
**	    if (op1.op_x > op2.op_x)
**		op1 > op2
**	    else if (op1.op_x < op2.op_x)
**		op1 < op2
**	    else if (op1.op_x == op2.op_x)
**	    {
**		if (op1.op_y > op2.op_y)
**		    op1 > op2
**		else if (op1.op_y < op2.op_y)
**		    op1 < op2
**		else
**		    op1 == op2
**	    } 
**
** Inputs:
**      scb                             Scb structure
**      op1                             First operand
**      op2                             Second operand
**					These are pointers to II_DATA_VALUE
**					structures which contain the values to
**					be compared.
**      result                          Pointer to i4  to contain the result of
**					the operation
**
** Outputs:
**      *result                         Filled with the result of the operation.
**					This routine will make *result
**					    < 0 if op1 < op2
**					    > 0 if op1 > op2
**					  or  0 if op1 == op2
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      01-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_compare(II_SCB	  *scb ,
              II_DATA_VALUE   *op1 ,
              II_DATA_VALUE   *op2 ,
              int        *result )
#else
usop_compare(scb, op1, op2, result)
II_SCB	        *scb;
II_DATA_VALUE   *op1;
II_DATA_VALUE   *op2;
int             *result;
#endif
{
    ORD_PAIR            *opv_1 = (ORD_PAIR *) op1->db_data;
    ORD_PAIR		*opv_2 = (ORD_PAIR *) op2->db_data;
    double		op1_x;
    double		op1_y;
    double		op2_x;
    double		op2_y;

    if (    (op1->db_datatype != op_datatype_id)
	 || (op2->db_datatype != op_datatype_id)
	 || (op1->db_length != sizeof(ORD_PAIR))
	 || (op2->db_length != sizeof(ORD_PAIR))
       )
    {
	/* Then this routine has been improperly called */
	us_error(scb, 0x200000, "Usop_compare: Type/length mismatch");
	return(II_ERROR);
    }

    /*
    ** Now perform the comparison based on the rules described above.
    */

    F8ASSIGN_MACRO(opv_1->op_x, op1_x);
    F8ASSIGN_MACRO(opv_1->op_y, op1_y);
    F8ASSIGN_MACRO(opv_2->op_x, op2_x);
    F8ASSIGN_MACRO(opv_2->op_y, op2_y);

    if (op1_x > op2_x)
    {
	*result = 1;
    }
    else if (op1_x < op2_x)
    {
	*result = -1;
    }
    else if (op1_y > op2_y)
    {
	*result = 1;
    }
    else if (op1_y < op2_y)
    {
	*result = -1;
    }
    else
    {
	*result = 0;
    }
    return(II_OK);
}

/*{
** Name: usop_lenchk	- Check the length of the datatype for validity
**
** Description:
**      This routine checks that the specified length for the datatype is valid.
**	Since ordered pairs are fixed length (i.e. more like dates than text),
**	this routine has little work to do.  If the length is user specified,
**	than the only valid length is zero (i.e. unspecified);  if the length is
**	not user specified, then the only valid length is the size of the
**	ORD_PAIR structure.  Not really much work to do.
**
** Inputs:
**      scb                             Pointer to Scb structure.
**      user_specified                  0 if not user specified, nonzero otherwise.
**      dv                              Datavalue to be checked.
**					if user_specified, than the length is
**					the external length; otherwise it is the
**					internal length.
**      result_dv                       Pointer to an II_DATA_VALUE into which
**					to place the correct length.
**					This parameter can be NULL;  when this
**					is the case, simply return success or
**					error status.
**
** Outputs:
**      result_dv->db_length		Contains the valid length.
**	result_dv->db_prec		Contains a valid precision.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      01-Mar-1989 (fred)
##          Created.
##      23-mar-1994 (johnst)
##          Make length variable type match function result type.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_lenchk(II_SCB	    *scb ,
	    int	    	    user_specified ,
	    II_DATA_VALUE   *dv ,
	    II_DATA_VALUE   *result_dv )
#else
usop_lenchk(scb, user_specified, dv, result_dv)
II_SCB	        *scb;
int	    	user_specified;
II_DATA_VALUE   *dv;
II_DATA_VALUE   *result_dv;
#endif
{
    II_STATUS		    result = II_OK;
    int 		    length;

    if (user_specified)
    {
	if ((dv->db_length != 0) )
	    result = II_ERROR;
	length = sizeof(ORD_PAIR);
    }
    else
    {
	if ((dv->db_length != sizeof(ORD_PAIR)) )
	    result = II_ERROR;
	length = 0;
    }

    if (result_dv)
    {
	result_dv->db_prec = 0;
	result_dv->db_length = length;
    }
    if (result)
    {
	us_error(scb, 0x200001,
			"Usop_lenchk: Invalid length for ORD_PAIR datatype");
    }
    return(result);
}

/*{
** Name: usop_keybld	- Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is built based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	our simple ordered pair data, these are typically simply the values.
**
**	Note that it is not the case that the input to this routine will always
**	be of type ordered pair.  Therefore, we must call the conversion routine
**	to build the actual key.  We will know that this routine will only be
**	called if we have built a coercion from the input datatype to our type.
**
**	    For correct interaction with the optimizer, if keybld() is
**	    given a data value for which it cannot create a key, it should
**	    not return an error.  Instead, it should simply change the type
**	    of the key to II_KNOMATCH and return OK.  For example, if a date
**	    datatype was given the character string "9999-nomonth-abcd",
**	    the correct response is II_OK with  key_block->adc_tykey set to
**	    II_KNOMATCH.  This will cause higher levels of the system to
**	    decide how to perform the query so that the correct error message
**	    will be returned to the user.  Failure to manage things this
**	    way (i.e. how things worked before I made this bug fix) resulted
**	    in an optimizer consistency check when a query was made to a table
**	    keyed on the user defined type.  The query was not run, which was
**	    correct, but the error was, shall we say, ugly.
**
**	    Also, a patch has been included to help pre-6.3/02 releases run
**	    repeat queries using user defined datatypes.  Turns out that when
**	    running repeat queries which use user defined datatypes in the where
**	    clause for keyed tables, if the UDT column is referenced by a
**	    parameter to the query, the optimizer does not pass the complete
**	    bit of data to this routine.  Instead, it simply calls this routine
**	    with a kdv.db_length set to negative.  This is supposed to indicate
**	    that the optimizer is only interested in the type of key involved,
**	    not the real key.  For these cases, this routine should simply stop
**	    after determining the key type in these cases.  For release 6.3/02,
**	    this has been fixed at a higher level, so that in these cass, the
**	    datatype specific routine (e.g. usop_keybld()) will not even be
**	    called.  However, to make things workable for 6.3/01 and previous
**	    releases, this change is included here as well.
**
**	However, the type of operation to be used is also included amongst the
**	parameters.  The possible operators, and their results, are listed
**	below.
**
**	    II_EQ_OP ('=')
**	    II_NE_OP ('!=')
**	    II_LT_OP ('<')
**	    II_LE_OP ('<=')
**	    II_GT_OP ('>')
**	    II_GE_OP ('>=')
**
**	In addition to returning the key pair, the type of key produced is
**	returned.  The choices are
**
**	    II_KNOMATCH    --> No values will match this key.
**				    No key is formed.
**	    II_KEXACTKEY   --> The key will match exactly one value.
**				    The key that is formed is placed as the low
**				    key.
**	    II_KRANGEKEY   --> Two keys were formed in the pair.  The caller
**				    should seek to the low-key, then scan until
**				    reaching a point greater than or equal to
**				    the high key.
**	    II_KHIGHKEY    --> High key found -- only high key formed.
**	    II_KLOWKEY	    --> Low key found -- only low key formed.
**	    II_KALLMATCH   --> The given value will match all values --
**				    No key was formed.
**
**	    II_KRANGEKEY and II_KALLMATCH are used only with pattern match
**	    searches which are not supported at this time.
**	    
**	    Given these values, the combinations possible are shown below.
**	 
**	    II_EQ_OP ('=')
**		    returns II_KEXACTKEY, low_key == value provided
**	    II_NE_OP ('!=')
**		    returns II_KALLMATCH, no key provided
**	    II_LT_OP ('<')
**	    II_LE_OP ('<=')
**		    These return II_KHIGHKEY, with the high_key == value provided.
**	    II_GT_OP ('>')
**	    II_GE_OP ('>=')
**		    These return II_KLOWKEY, with the low_key == value provided.
**
**
** Inputs:
**      scb                             Scb
**      key_block                       Pointer to key block data structure...
**	    .adc_kdv			    Datavalue for which to build a key.
**	    .adc_opkey			    Operator type for which key is being
**					    built.
**	    .adc_lokey, .adc_hikey	    Pointer to area for key.  If 0, do
**					    not build key.
**
** Outputs:
**      *key_block                      Key block filled with following
**	    .adc_tykey                       Type key provided
**	    .adc_lokey                       if .adc_tykey is II_KEXACTKEY or
**					     II_KLOWKEY, this is key built.
**	    .adc_hikey			     if .adc_tykey is II_KEXACTKEY or
**					     ADC_KHIGHKEY, this is the key built.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      6-Mar-1989 (fred)
##          Created.
##	10-aug-1990 (fred)
##	    Made a few bug fixes.
##
##	    For correct interaction with the optimizer, if keybld() is
##	    given a data value for which it cannot create a key, it should
##	    not return an error.  Instead, it should simply change the type
##	    of the key to II_KNOMATCH and return OK.  For example, if a date
##	    datatype was given the character string "9999-nomonth-abcd",
##	    the correct response is II_OK with  key_block->adc_tykey set to
##	    II_KNOMATCH.  This will cause higher levels of the system to
##	    decide how to perform the query so that the correct error message
##	    will be returned to the user.  Failure to manage things this
##	    way (i.e. how things worked before I made this bug fix) resulted
##	    in an optimizer consistency check when a query was made to a table
##	    keyed on the user defined type.  The query was not run, which was
##	    correct, but the error was, shall we say, ugly.
##
##	    Also, a patch has been included to help pre-6.3/02 releases run
##	    repeat queries using user defined datatypes.  Turns out that when
##	    running repeat queries which use user defined datatypes in the where
##	    clause for keyed tables, if the UDT column is referenced by a
##	    parameter to the query, the optimizer does not pass the complete
##	    bit of data to this routine.  Instead, it simply calls this routine
##	    with a kdv.db_length set to negative.  This is supposed to indicate
##	    that the optimizer is only interested in the type of key involved,
##	    not the real key.  For these cases, this routine should simply stop
##	    after determining the key type in these cases.  For release 6.3/02,
##	    this has been fixed at a higher level, so that in these cass, the
##	    datatype specific routine (e.g. usop_keybld()) will not even be
##	    called.  However, to make things workable for 6.3/01 and previous
##	    releases, this change is included here as well.
##
##	    Finally, when converting keys, the test was incorrect.  Once (if)
##	    the low key is converted, we should convert the high key if the low
##	    key was converted successfully, not if it failed :-).
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_keybld(II_SCB	  *scb ,
            II_KEY_BLK   *key_block )
#else
usop_keybld(scb, key_block)
II_SCB	     *scb;
II_KEY_BLK   *key_block;
#endif
{

    II_STATUS           result = II_OK;

    switch (key_block->adc_opkey)
    {
	case II_NE_OP:
	    key_block->adc_tykey = II_KALLMATCH;
	    break;

	case II_EQ_OP:
	    key_block->adc_tykey = II_KEXACTKEY;
	    break;

	case II_LT_OP:
	case II_LE_OP:
	    key_block->adc_tykey = II_KHIGHKEY;
	    break;

	case II_GT_OP:
	case II_GE_OP:
	    key_block->adc_tykey = II_KLOWKEY;
	    break;

	default:	/* To catch LIKE operators which are not legal */
	    result = II_ERROR;
	    key_block->adc_tykey = II_KNOMATCH;
	    break;
    }
    if (result)
    {
	us_error(scb, 0x200002, "Usop_keybld: Invalid key type");
    }
    else if (key_block->adc_kdv.db_length >= 0)
    {
	/*
	**  If the length was negative, then we only care about the key type.
	**  This change is necessary for repeat queries keying off of user
	**  defined datatypes.  This check could be removed in post 6.3/02
	**  as it is made at a higher level, but is included for clarity and
	**  to help systems not yet running (at least) 6.3/02.
	*/

	if ((key_block->adc_tykey == II_KLOWKEY) ||
		(key_block->adc_tykey == II_KEXACTKEY))
	{
	    if (key_block->adc_lokey.db_data)
	    {
		result = usop_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_lokey);
	    }
	}
	if ( (result == II_OK) &&
		((key_block->adc_tykey == II_KHIGHKEY) ||
		    (key_block->adc_tykey == II_KEXACTKEY)))
	{
	    if (key_block->adc_hikey.db_data)
	    {
		result = usop_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_hikey);
	    }
	}

	/*
	**  If we cannot correctly convert a key, then simply return a key
	**  type of II_KNOMATCH, and a successful status.  This will allow
	**  the optimizer to correctly determine and report the syntax error
	**  in the datatype specification at a later stage.
	*/

	if (result != II_OK)
	{
	    key_block->adc_tykey = II_KNOMATCH;
	    result = II_OK;
	}
    }    
    return(result);
}

/*{
** Name: usop_getempty	- Get an empty value
**
** Description:
**      This routine constructs the given empty value for this datatype.  By
**	definition, the empty value will be the origin (0,0).  This routine
**	merely constructs one of those in the space provided. 
**
** Inputs:
**      scb				Pointer to an session control block.
**      empty_dv			Pointer to II_DATA_VALUE to
**                                      place the empty data value in.
**              .db_datatype            The datatype for the empty data
**                                      value.
**              .db_length              The length for the empty data
**                                      value.
**              .db_data                Pointer to location to place
**                                      the .db_data field for the empty
**                                      data value.  Note that this will
**                                      often be a pointer into a tuple.
**
** Outputs:
**             *empty_dv.db_data	The data for the empty data
**                                      value will be placed here.
**
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      01-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_getempty(II_SCB       *scb ,
               II_DATA_VALUE   *empty_dv )
#else
usop_getempty(scb, empty_dv)
II_SCB          *scb;
II_DATA_VALUE   *empty_dv;
#endif
{
    II_STATUS		result = II_OK;
    ORD_PAIR		*op;
    double		zero = 0.0;
    
    if ((empty_dv->db_datatype != op_datatype_id)
	||  (empty_dv->db_length != sizeof(ORD_PAIR)))
    {
	result = II_ERROR;
	us_error(scb, 0x200003, "Usop_getempty: type/length mismatch");
    }
    else
    {
	op = (ORD_PAIR *) empty_dv->db_data;
	F8ASSIGN_MACRO(zero, op->op_x);
	F8ASSIGN_MACRO(zero, op->op_y);
    }
    return(result);
}

/*{
** Name: usop_valchk	- Check for valid values
**
** Description:
**      This routine checks for valid values.  In fact, all values are valid, so
**	this routine just returns OK. 
**
** Inputs:
**      scb                             II_SCB
**      dv                              Data value in question.
**
** Outputs:
**      None.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      01-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_valchk(II_SCB       *scb ,
             II_DATA_VALUE   *dv )
#else
usop_valchk(scb, dv)
II_SCB         *scb;
II_DATA_VALUE   *dv;
#endif
{

    return(II_OK);
}

/*{
** Name: usop_hashprep	- Prepare a datavalue for becoming a hash key.
**
** Description:
**      This routine prepares a value for becoming a hash key.  For our
**	datatype, no changes are necessary, so the value is simply copied.
**	No other work is performed. 
**
** Inputs:
**      scb                             Scb.
**      dv_from                         Pointer to value to be keyed upon.
**	dv_key				Pointer to datavalue are to become a
**					key. 
**
** Outputs:
**      dv_key                          Subfields below are filled with
**	    .db_length                       length of the key
**	    .db_data                         key value.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      01-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_hashprep(II_SCB       *scb ,
               II_DATA_VALUE   *dv_from ,
               II_DATA_VALUE  *dv_key )
#else
usop_hashprep(scb, dv_from, dv_key)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_key;
#endif
{
    II_STATUS           result = II_OK;

    if (dv_from->db_datatype == op_datatype_id)
    {
	byte_copy((char *)dv_from->db_data,
		dv_from->db_length,
		dv_key->db_data);
	dv_key->db_length = dv_from->db_length;
    }
    else
    {
	result = II_ERROR;
	us_error(scb, 0x200004, "Usop_hashprep: type/length mismatch");
    }
    return(result);
}

/*{
** Name: usop_helem	- Create histogram element for data value
**
** Description:
**      This routine creates a histogram value for a data element.  Histogram
**	values are used by the INGRES optimizer in the evaluations of query
**	plans.
**
**	A histogram function h is a function such that if a < b, then h(a) <=
**	h(b).  Since there may be many histograms in use, the optimizer
**	restricts the length of the histogram value to 8.
**
**	Given these facts and the algorithm used in ordered pair comparison (see
**	usop_compare() above), the histogram for any value will be the x
**	coordinate portion.  The datatype will be II_FLOAT (float), with a
**	length of 8. 
**
** Inputs:
**      scb                             Scb.
**      dv_from                         Value for which a histogram is desired.
**      dv_histogram                    Pointer to datavalue into which to place
**					the histogram.
**	    .db_datatype		    Should contain II_FLOAT
**	    .db_length			    Should contain 8
**	    .db_data			    Assumed to be a pointer to 8 bytes
**					    into which to place the value.
**
** Outputs:
**      *dv_histogram->db_data          Will contain the histogram value.
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_helem(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE  *dv_histogram )
#else
usop_helem(scb, dv_from, dv_histogram)
II_SCB         *scb;
II_DATA_VALUE  *dv_from;
II_DATA_VALUE  *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    double		value;
    long		error_code;
    char		*msg;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200005;
	msg = "Usop_helem: Type for histogram incorrect";
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200006;
	msg = "Usop_helem: Length for histogram incorrect";
    }
    else if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200007;
	msg = "Usop_helem: Base type for histogram incorrect";
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x200008;
	msg = "Usop_helem: Base length for histogram incorrect";
    }
    else
    {
	
	F8ASSIGN_MACRO(((ORD_PAIR *)dv_from->db_data)->op_x, value);
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usop_hmin	- Create histogram for minimum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	smallest value of a type.  The same histogram rules apply as above.
**
**	In this case, the minimum value is (<smallest double>, <smallest double>),
**	so the minimum histogram is simply the smallest double.
**
** Inputs:
**      scb                             Scb.
**      dv_from                         Datavalue describing the type & length
**					for the desired histogram.
**      dv_histogram                    Datavalue for the histogram.
**
** Outputs:
**      *dv_histogram->db_data          Contains the histogram.
**	
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_hmin(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usop_hmin(scb, dv_from, dv_histogram)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = FMIN;
    long			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200008;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200009;
    }
    else if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x20000a;
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x20000b;
    }
    else
    {
	
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "Usop_hmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usop_dhmin	- Create `default' minimum histogram.
**
** Description:
**      This routine creates the minimum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' ordered pairs are found
**	between (0,0) and (100, 100).  Therefore, the default minimum histogram
**	will have a value of 0; the default maximum histogram (see below) will
**	have a value of 100. 
**
** Inputs:
**      scb                             Scb
**      dv_from                         Datavalue containing the type for the value
**      dv_histogram                    Datavalue for the histogram
**
** Outputs:
**      *dv_histogram->db_data          Filled with the histogram.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_dhmin(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE   *dv_histogram )
#else
usop_dhmin(scb, dv_from, dv_histogram)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = 0.0;
    long			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x20000c;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x20000d;
    }
    else if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x20000e;
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x20000f;
    }
    else
    {
	
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "Usop_dhmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usop_hmax	- Create histogram for maximum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	largest value of a type.  The same histogram rules apply as above.
**
**	In this case, the maximum value is (<largest double>, <largest double>),
**	so the maximum histogram is simply the largest double.
**
** Inputs:
**      scb                             Scb.
**      dv_from                         Datavalue describing the type & length
**					for the desired histogram.
**      dv_histogram                    Datavalue for the histogram.
**
** Outputs:
**      *dv_histogram->db_data          Contains the histogram.
**	
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_hmax(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usop_hmax(scb, dv_from, dv_histogram)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = FMAX;
    long			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200010;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200011;
    }
    else if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200012;
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x200013;
    }
    else
    {
	
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "Usop_hmax:  Invalid input parameters");
	
    return(result);
}

/*{
** Name: usop_dhmax	- Create `default' maximum histogram.
**
** Description:
**      This routine creates the maximum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' ordered pairs are found
**	between (0,0) and (100, 100).
**
** Inputs:
**      scb                             Scb
**      dv_from                         Datavalue containing the type for the value
**      dv_histogram                    Datavalue for the histogram
**
** Outputs:
**      *dv_histogram->db_data          Filled with the histogram.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_dhmax(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE   *dv_histogram )
#else
usop_dhmax(scb, dv_from, dv_histogram)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_histogram;
#endif

{
    II_STATUS			result = II_OK;
    double			value = 100.0;
    long			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200014;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200015;
    }
    else if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200016;
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x200017;
    }
    else
    {
	
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "Usop_dhmax: Invalid parameters");
    return(result);
}

/*{
** Name: usop_hg_dtln	- Provide datatype & length for a histogram.
**
** Description:
**      This routine provides the datatype & length for a histogram for a given
**	datatype.  For our datatype, the histogram's datatype an length are
**	II_FLOAT (float) and 8, respectively. 
**
** Inputs:
**      scb                             Scb.
**      dv_from                         Datavalue describing the type about
**					whose histogram's information is needed.
**	    .db_datatype		    Datatype in question
**	    .db_length			    Length of said type
**	dv_histogram			Datavalue provided to describe the
**					histogram.
**
** Outputs:
**      dv_histogram
**	    .db_datatype                     Filled with the required type ...
**	    .db_length			     and length
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_hg_dtln(II_SCB       *scb ,
              II_DATA_VALUE   *dv_from ,
              II_DATA_VALUE   *dv_histogram )
#else
usop_hg_dtln(scb, dv_from, dv_histogram)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
II_DATA_VALUE   *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    long		error_code;

    if (dv_from->db_datatype != op_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200018;
    }
    else if (dv_from->db_length != sizeof(ORD_PAIR))
    {
	result = II_ERROR;
	error_code = 0x200019;
    }
    else
    {
	dv_histogram->db_datatype = II_FLOAT;
	dv_histogram->db_length = 8;
    }
    if (result)
	us_error(scb, error_code, "Usop_hg_dtln: Invalid parameters");
	
    return(result);
}

/*{
** Name: usop_minmaxdv	- Provide the minimum/maximum values/lengths for a
**			    datatype 
**
** Description:
**      This routine provides the caller with information about the limits of a
**	datatype.  Depending upon the parameters (subject to the `rules' listed
**	below), the minimum and/or maximum value  and/or the minimum and/or
**	maximum length for a datatype will be provided.
**
**	Here are the rules:
**	-------------------
**	Let MIN_DV and MAX_DV represent the input parameters; both are pointers
**	to II_DATA_VALUEs.  The lengths for MIN_DV and MAX_DV may be different,
**	but their datatypes MUST be the same.
**
**	Rule 1:
**          If MIN_DV (or MAX_DV) is NULL, then the processing for MIN_DV (or
**          MAX_DV) will be completely skipped.  This allows the caller who is
**          only interested in the `maximum' (or `minimum') to more effeciently
**          use this routine.
**	Rule 2:
**          If the .db_length field of *MIN_DV (or *MAX_DV) is supplied as
**          II_LEN_UNKNOWN, then no minimum (or maximum) value will be built
**          and placed at MIN_DV->db_data (or MAX_DV->db_data).  Instead, the
**          minimum (or maximum) valid internal length will be returned in
**          MIN_DV->db_length (or MAX_DV->db_length).
**	Rule 3:
**          If MIN_DV->db_data (or MAX_DV->db_data) is NULL, then then no
**          minimum (or maximum) value will be built and placed at
**          MIN_DV->db_data (or MAX_DV->db_data).
**	Rule 4:
**          If none of rules 1-3 applies to MIN_DV (or MAX_DV), then the minimum
**          (or maximum) non-null value for the datatype/length will be built
**          and placed at MIN_DV->db_data (or MAX_DV->db_data). 
**
** Inputs:
**	scb				Pointer to an session control block.
**      min_dv	 			Pointer to II_DATA_VALUE for the `min'.
**					If this is NULL, `min' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `max'.
**		.db_length		The length to build the `min' value for,
**					or II_LEN_UNKNOWN, if the `min' length
**					is requested.
**		.db_data		Pointer to location to place the `min'
**					non-null value, if requested.  If this
**					is NULL no `min' value will be created.
**      max_dv	 			Pointer to II_DATA_VALUE for the `max'.
**					If this is NULL, `max' processing will
**					be skipped.
**		.db_datatype		Its datatype.  Must be the same as
**					datatype for `min'.
**		.db_length		The length to build the `max' value for,
**					or II_LEN_UNKNOWN, if the `max' length
**					is requested.
**		.db_data		Pointer to location to place the `max'
**					non-null value, if requested.  If this
**					is NULL no `max' value will be created.
**
** Outputs:
**      min_dv	 			If this was supplied as NULL, `min'
**					processing will be skipped.
**		.db_length		If this was supplied as II_LEN_UNKNOWN,
**					then the `min' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					II_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `min'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
**      max_dv    			If this was supplied as NULL, `max'
**					processing will be skipped.
**		.db_length		If this was supplied as II_LEN_UNKNOWN,
**					then the `max' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					II_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `max'
**					non-null value for this datatype/length
**					will be built and placed at the location
**					pointed to by .db_data.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_minmaxdv(II_SCB	  *scb ,
               II_DATA_VALUE   *min_dv ,
               II_DATA_VALUE  *max_dv )
#else
usop_minmaxdv(scb, min_dv, max_dv )
II_SCB	        *scb;
II_DATA_VALUE   *min_dv;
II_DATA_VALUE   *max_dv;
#endif
{
    II_STATUS           result = II_OK;
    ORD_PAIR		*op;
    long		error_code;
    double		value;

    if (min_dv)
    {
	if (min_dv->db_datatype != op_datatype_id)
	{
	    result = II_ERROR;
	    error_code = 0x20001A;
	}
	else if (min_dv->db_length == sizeof(ORD_PAIR))
	{
	    if (min_dv->db_data)
	    {
		op = (ORD_PAIR *) min_dv->db_data;
		value = FMIN;
		F8ASSIGN_MACRO(value, op->op_x);
		F8ASSIGN_MACRO(value, op->op_y);
	    }
	}
	else if (min_dv->db_length == II_LEN_UNKNOWN)
	{
	    min_dv->db_length = sizeof(ORD_PAIR);
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20001B;
	}
    }
    if (max_dv)
    {
	if (max_dv->db_datatype != op_datatype_id)
	{
	    result = II_ERROR;
	    error_code = 0x20001C;
	}
	else if (max_dv->db_length == sizeof(ORD_PAIR))
	{
	    if (max_dv->db_data)
	    {
		op = (ORD_PAIR *) max_dv->db_data;
		value = FMAX;
		F8ASSIGN_MACRO(value, op->op_x);
		F8ASSIGN_MACRO(value, op->op_y);
	    }
	}
	else if (max_dv->db_length == II_LEN_UNKNOWN)
	{
	    max_dv->db_length = sizeof(ORD_PAIR);
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20001D;
	}
    }
    if (result)
	us_error(scb, error_code, "Usop_minmaxdv: Invalid parameters");
    return(result);
}

/*{
** Name: usop_convert	- Convert to/from ordered pairs
**
** Description:
**      This routine converts values to and/or from ordered pairs.
**	The following conversions are supported.
**	    OP -> OP
**	    OP <-> longtext (these two are required)
**	    OP -> c (the ASCII function)
**	    c  -> OP
**	    char -> OP
**	    varchar -> OP
**	    text -> OP
**
** Inputs:
**      scb                             Scb
**      dv_in                           Data value to be converted
**      dv_out				Location to place result
**
** Outputs:
**      *dv_out                         Result is filled in here
**	    .db_data                         Contains result
**	    .db_length			     Contains length of result
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_convert(II_SCB       *scb ,
              II_DATA_VALUE   *dv_in ,
              II_DATA_VALUE   *dv_out )
#else
usop_convert(scb, dv_in, dv_out)
II_SCB          *scb;
II_DATA_VALUE   *dv_in;
II_DATA_VALUE   *dv_out;
#endif

{
    char		*b;
    char		*b1;
    ORD_PAIR            *op;
    double			value;
    int			length;
    II_STATUS		result = II_OK;
    double		x, y;
    int			match_requirement;
    long		error_code;
    char		*msg;
    char                buffer[64];
    char		end_paren[2];
    char		junk[2];

    for (length = 0; length < sizeof(buffer); length++)
	buffer[length] = 0;
    
    dv_out->db_prec = 0;
    if ((dv_in->db_datatype == op_datatype_id)
	&& (dv_out->db_datatype == op_datatype_id))
    {
	byte_copy((char *)dv_in->db_data,
		dv_in->db_length,
		dv_out->db_data);
    }
    else if (dv_in->db_datatype == op_datatype_id)
    {
	double	    x_part;
	double	    y_part;
	
	op = (ORD_PAIR *) dv_in->db_data;
	F8ASSIGN_MACRO(op->op_x, x_part);
	F8ASSIGN_MACRO(op->op_y, y_part);
	
	sprintf(buffer, "(%11.3f, %11.3f)", x_part, y_part);
	length = strlen(buffer);
	if ((dv_out->db_datatype == II_CHAR) ||
	    (dv_out->db_datatype == II_C))
	{
	    if (length > dv_out->db_length)
	    {
		error_code = 0x20001e;
		msg = "Usop_convert: Insufficient space for ord_pair output";
		result = II_ERROR;
	    }
	    else
	    {
		dv_out->db_length = length,
		byte_copy((char *)buffer,
			    length,
			    dv_out->db_data);
	    }
	}
	else
	{
	    if (length+2 > dv_out->db_length)
	    {
		error_code = 0x20001e;
		msg = "Usop_convert: Insufficient space for ord_pair output";
		result = II_ERROR;
	    }
	    else
	    {
		dv_out->db_length = length+2;
		byte_copy((char *)buffer, length,
		    (char *)dv_out->db_data+2);
		I2ASSIGN_MACRO(length, * (short *) dv_out->db_data);
	    }
	}
    }
    else
    {
	for (;;)
	{
	    op = (ORD_PAIR *) dv_out->db_data;
	    
	    switch (dv_in->db_datatype)
	    {
		case II_C:
		case II_CHAR:
		    length = dv_in->db_length;
		    if (length < sizeof(buffer))
			byte_copy((char *)dv_in->db_data, length, buffer);
		    break;

		case II_TEXT:
		case II_VARCHAR:
		case II_LONGTEXT:
		    length = dv_in->db_length - 2;
		    if (length < sizeof(buffer))
			byte_copy((char *)((char *) dv_in->db_data + 2),
				length,
				buffer);
		    break;

		default:
		    error_code = 0x20001F;
		    msg = "Usop_convert:  Unknown input type";
		    result = II_ERROR;
	    }
	    if (result != II_OK)
		break;

	    result = II_ERROR;
	    if (length < sizeof(buffer))
	    {
		length = sscanf(buffer, " ( %lf , %lf%1s %1s",
				    &x, &y, end_paren, 	junk);
	    }
	    
	    if ((length != 3) || strcmp(")", end_paren))
	    {
		error_code = 0x200020;
		msg = "Unable to convert to ordered pair";
		break;
	    }

	    F8ASSIGN_MACRO(x, op->op_x);
	    F8ASSIGN_MACRO(y, op->op_y);
	    
	    result = II_OK;
	    break;
	}
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usop_tmlen	- Determine 'terminal monitor' lengths
**
** Description:
**      This routine returns the default and worst case lengths for a datatype
**	if it were to be printed as text (which is the way things are
**	displayed in the terminal monitor).  Although in this release,
**	user-defined datatypes are not returned to the terminal monitor, this
**	routine (and its partner, usi4_tmcvt()) are needed for various trace
**	flag and error formatting usages within the DBMS.  Therefore, it is
**	required at this time.
**
**
** Inputs:
**      scb                             Session Control Block
**					    (as above)
**      dv_from                         Pointer to II_DATA_VALUE for which the
**					    call is being made.
**
** Outputs:
**      def_width                       Pointer to 2-byte integer into which the
**					    default width should be placed.
**      largest_width                   Pointer to 2-byte integer into which the
**					    worst case width (widest) should be
**					    placed.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      10-July-89 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_tmlen(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            short       *def_width ,
            short       *largest_width )
#else
usop_tmlen(scb, dv_from, def_width, largest_width)
II_SCB          *scb;
II_DATA_VALUE   *dv_from;
short           *def_width;
short           *largest_width;
#endif
{
    II_DATA_VALUE	    local_dv;

    if (dv_from->db_datatype != ORP_TYPE)
    {
	us_error(scb, 0x200021, "usop_tmlen: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	*def_width = *largest_width = 26;
    }
    return(II_OK);
}

/*{
** Name: usop_tmcvt	- Convert to displayable format
**
** Description:
**      This routine converts the data from an internal format to a displayable
**	format which (could) be used by the INGRES terminal monitor.  This
**	routine is also used internally by the database manager to format
**	various trace statements and errors.  It therefore must exist even
**	though these datatypes will not currently be directly visible by the
**	terminal monitor. 
**
** Inputs:
**      scb                             Session control block.
**      from_dv                         Ptr to datavalue containing the data to
**					    be displayed.
**      to_dv                           Ptr to a data value which provides the
**					    output space.  The db_data field
**					    points to an area of db_length
**					    bytes, into which a character
**					    representation of the data should be
**					    placed.  The db_prec and
**					    db_datatypes fields should be
**					    ignored.
**
** Outputs:
**      to_dv->db_data                  Filled with the output.
**      *output_length                   Filled with the number of characters
**					placed in to_dv->db_data.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      10-jul-89 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_tmcvt(II_SCB       *scb ,
            II_DATA_VALUE  *from_dv ,
            II_DATA_VALUE   *to_dv ,
            int        *output_length )
#else
usop_tmcvt(scb, from_dv, to_dv, output_length )
II_SCB         *scb;
II_DATA_VALUE  *from_dv;
II_DATA_VALUE  *to_dv;
int            *output_length;
#endif
{
    II_DATA_VALUE	    local_dv;
    II_STATUS		    status;

    local_dv.db_data = to_dv->db_data;
    local_dv.db_length = to_dv->db_length;
    local_dv.db_prec = 0;
    local_dv.db_datatype = II_CHAR;
    
    status = usop_convert(scb, from_dv, &local_dv);
    *output_length = local_dv.db_length;
    return(status);
}

/*{
** Name: usop_dbtoev	- Determine which external datatype this will convert to
**
** Description:
**      This routine returns the external type to which ordered pairs will be converted. 
**      The correct type is CHAR, with a length of 26.
**
** Inputs:
**      scb                             Sdb.
**      db_value                        PTR to II_DATA_VALUE for database type
**	ev_value			ptr to II_DATA_VALUE for export type.
**
** Outputs:
**      *ev_value                       Filled in as follows
**          .db_datatype                Type of export value
**	    .db_length			Length of export value
**	    .db_prec			Precision of export value
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      07-apr-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_dbtoev(II_SCB	  *scb ,
             II_DATA_VALUE  *db_value ,
             II_DATA_VALUE  *ev_value )
#else
usop_dbtoev(scb, db_value, ev_value)
II_SCB	       *scb;
II_DATA_VALUE  *db_value;
II_DATA_VALUE  *ev_value;
#endif
{

    if (db_value->db_datatype != op_datatype_id)
    {
	us_error(scb, 0x200021, "Usop_dbtoev: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	ev_value->db_datatype = II_CHAR;
	ev_value->db_length = 26;
	ev_value->db_prec = 0;
    }
    return(II_OK);
}

/*{
** Name: usop_add	- Compute the sum of two points
**
** Description:
**      This routine adds two ordered pairs.  It performs the function
**	    (res_x,res_y) = ((x1 + x2),(y1 + y2))
**
**	The result is an ordered pair.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
**	dv_2				Datavalue representing point 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              double result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      Aug-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_add(II_SCB       *scb ,
          II_DATA_VALUE   *dv_1 ,
          II_DATA_VALUE   *dv_2 ,
          II_DATA_VALUE   *dv_result )
#else
usop_add(scb, dv_1, dv_2, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_2;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			op1_x;
    double			op1_y;
    double			op2_x;
    double			op2_y;
    ORD_PAIR			*op_1;
    ORD_PAIR			*op_2;
    ORD_PAIR			sum;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_2->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == op_datatype_id)
	&&  (dv_result->db_length == sizeof(ORD_PAIR))
	&&  (dv_result->db_data)
       )  
    {
	op_1 = (ORD_PAIR *) dv_1->db_data;
	op_2 = (ORD_PAIR *) dv_2->db_data;
	F8ASSIGN_MACRO(op_1->op_x, op1_x);
	F8ASSIGN_MACRO(op_1->op_y, op1_y);
	F8ASSIGN_MACRO(op_2->op_x, op2_x);
	F8ASSIGN_MACRO(op_2->op_y, op2_y);
	sum.op_x = op1_x + op2_x;
	sum.op_y = op1_y + op2_y;
	byte_copy((char *)&sum,sizeof(sum),dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200022, "Usop_add: Invalid input");
    return(result);
}

/*{
** Name: usop_distance	- Compute distance between two points
**
** Description:
**      This routine computes the distance between two ordered pairs.  It
**	performs this function using the standard formula of
**	    distance = sqrt( ((x2 - x1) ** 2) + ( (y2 - y1) ** 2) )
**
**	The result is a floating point number (double).
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
**	dv_2				Datavalue representing point 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              double result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_distance(II_SCB       *scb ,
               II_DATA_VALUE   *dv_1 ,
               II_DATA_VALUE   *dv_2 ,
               II_DATA_VALUE   *dv_result )
#else
usop_distance(scb, dv_1, dv_2, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_2;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			distance;
    double			op1_x;
    double			op1_y;
    double			op2_x;
    double			op2_y;
    ORD_PAIR			*op_1;
    ORD_PAIR			*op_2;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_2->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	op_1 = (ORD_PAIR *) dv_1->db_data;
	op_2 = (ORD_PAIR *) dv_2->db_data;
	F8ASSIGN_MACRO(op_1->op_x, op1_x);
	F8ASSIGN_MACRO(op_1->op_y, op1_y);
	F8ASSIGN_MACRO(op_2->op_x, op2_x);
	F8ASSIGN_MACRO(op_2->op_y, op2_y);
	distance = sqrt(
			pow((double) (op2_x - op1_x), (double) 2.0) +
			pow((double) (op2_y - op1_y), (double) 2.0)
			);
	F8ASSIGN_MACRO(distance, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200022, "Usop_distance: Invalid input");
    return(result);
}

/*{
** Name: usop_slope	- Compute slope of line between two points
**
** Description:
**      This routine computes the slope of the line running between two points.
**	The slope is calculated using the standard method of rise/run, or
**
**	    slope = (y2 - y1) / (x2 - x1).
**
**	If (x2 - x1) == 0, a slope of zero (0) is returned.
**
**	The result is returned as an double. 
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
**	dv_2				Datavalue representing point 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              double result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_slope(II_SCB       *scb ,
            II_DATA_VALUE   *dv_1 ,
            II_DATA_VALUE   *dv_2 ,
            II_DATA_VALUE   *dv_result )
#else
usop_slope(scb, dv_1, dv_2, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_2;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			slope;
    double			op1_x;
    double			op1_y;
    double			op2_x;
    double			op2_y;
    ORD_PAIR			*op_1;
    ORD_PAIR			*op_2;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_2->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	op_1 = (ORD_PAIR *) dv_1->db_data;
	op_2 = (ORD_PAIR *) dv_2->db_data;
	F8ASSIGN_MACRO(op_1->op_x, op1_x);
	F8ASSIGN_MACRO(op_1->op_y, op1_y);
	F8ASSIGN_MACRO(op_2->op_x, op2_x);
	F8ASSIGN_MACRO(op_2->op_y, op2_y);
	if (op2_x - op1_x)
	    slope = (op2_y - op1_y) / (op2_x - op1_x);
	else
	    slope = 0.0;
	F8ASSIGN_MACRO(slope, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200023, "Usop_slope: Invalid input");
    return(result);
}

/*{
** Name: usop_midpoint	- Compute the midpoint of line between two points
**
** Description:
**      This routine computes the midpoint of the line running between two
**	points.  The midpoint is calculated as follows.
**
**	    midpoint.op_x = (x1 + x2) / 2,
**	    midpoint.op_y = (y1 + y2) / 2,
**
**	The result is returned as an ORD_PAIR. 
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
**	dv_2				Datavalue representing point 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Ordered pair result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_midpoint(II_SCB       *scb ,
               II_DATA_VALUE   *dv_1 ,
               II_DATA_VALUE   *dv_2 ,
               II_DATA_VALUE   *dv_result )
#else
usop_midpoint(scb, dv_1, dv_2, dv_result )
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_2;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    ORD_PAIR			midpoint;
    double			op1_x;
    double			op1_y;
    double			op2_x;
    double			op2_y;
    ORD_PAIR			*op_1;
    ORD_PAIR			*op_2;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_2->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == op_datatype_id)
	&&  (dv_result->db_length == sizeof(ORD_PAIR))
	&&  (dv_result->db_data)
       )  
    {
	op_1 = (ORD_PAIR *) dv_1->db_data;
	op_2 = (ORD_PAIR *) dv_2->db_data;
	F8ASSIGN_MACRO(op_1->op_x, op1_x);
	F8ASSIGN_MACRO(op_1->op_y, op1_y);
	F8ASSIGN_MACRO(op_2->op_x, op2_x);
	F8ASSIGN_MACRO(op_2->op_y, op2_y);
	midpoint.op_x = (op1_x + op2_x) / 2.0;
	midpoint.op_y = (op1_y + op2_y) / 2.0;
	
	byte_copy((char *)&midpoint, sizeof(midpoint), dv_result->db_data);
	
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200023, "Usop_midpoint: Invalid input");
    return(result);
}

/*{
** Name: usop_ordpair	- Create an ordered pair from two floats
**
** Description:
**
**	This routine creates an ordered pair from two floats.  This is performed
**	by simple assignment.  The first argument is the x coordinate; the
**	second, the y.
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing x coord
**	dv_2				Datavalue representing y coord
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Ordered pair result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_ordpair(II_SCB       *scb ,
              II_DATA_VALUE   *dv_1 ,
              II_DATA_VALUE   *dv_2 ,
              II_DATA_VALUE   *dv_result )
#else
usop_ordpair(scb, dv_1, dv_2, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_2;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    ORD_PAIR			op;
    double			x_coord;
    double			y_coord;
    float			an_f4;

    if (    (dv_1->db_datatype == II_FLOAT)
	&&  (dv_2->db_datatype == II_FLOAT)
	&&  (dv_result->db_datatype == op_datatype_id)
	&&  (dv_result->db_length == sizeof(ORD_PAIR))
	&&  (dv_result->db_data)
       )  
    {
	if (dv_1->db_length == 4)
	{
	    F4ASSIGN_MACRO(*dv_1->db_data, an_f4);
	    x_coord = an_f4;
	}
	else
	{
	    F8ASSIGN_MACRO(*dv_1->db_data, x_coord);
	}
	if (dv_2->db_length == 4)
	{
	    F4ASSIGN_MACRO(*dv_2->db_data, an_f4);
	    y_coord = an_f4;
	}
	else
	{
	    F8ASSIGN_MACRO(*dv_2->db_data, y_coord);
	}
	op.op_x = x_coord;
	op.op_y = y_coord;
	byte_copy((char *) &op, sizeof(op), dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200025, "Usop_ordpair: Invalid input");
    return(result);
}

/*{
** Name: usop_xcoord	- Find x coordinate of an ordered pair
**
** Description:
**
**	This routine extracts the x coordinate of an ordered pair
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing the ordered pair
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              double result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_xcoord(II_SCB       *scb ,
             II_DATA_VALUE   *dv_1 ,
             II_DATA_VALUE   *dv_result )
#else
usop_xcoord(scb, dv_1, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_result;
#endif

{
    II_STATUS			result = II_ERROR;
    ORD_PAIR			*op;
    double			x_coord;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	op = (ORD_PAIR *) dv_1->db_data;
	F8ASSIGN_MACRO(op->op_x, x_coord);
	F8ASSIGN_MACRO(x_coord, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200026, "Usop_xcoord: Invalid input");
    return(result);
}

/*{
** Name: usop_ycoord	- Find y coordinate of an ordered pair
**
** Description:
**
**	This routine extracts the y coordinate of an ordered pair
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing the ordered pair
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              double result is placed here.
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      03-Mar-1989 (fred)
##          Created
##      15-Feb-2008 (hanal04) Bug 119922
##          IIADD_DT_DFN entries were all missing an initialisation for
##          the dtd_compr_addr() fcn ptr.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop_ycoord(II_SCB           *scb ,
             II_DATA_VALUE   *dv_1 ,
             II_DATA_VALUE   *dv_result )
#else
usop_ycoord(scb, dv_1, dv_result)
II_SCB          *scb;
II_DATA_VALUE   *dv_1;
II_DATA_VALUE   *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    ORD_PAIR		*op;
    double		y_coord;

    if (    (dv_1->db_datatype == op_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	op = (ORD_PAIR *) dv_1->db_data;
	F8ASSIGN_MACRO(op->op_y, y_coord);
	F8ASSIGN_MACRO(y_coord, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200027, "Usop_ycoord: Invalid input");
    return(result);
}

/*
**  This array contains the list of datatypes to be added.  Each element
**  contains the name, the type identifier, and the addresss of the functions
**  which perform the various base operations.  See the include file <iiadd.h>
**  for more detailed information
*/

static IIADD_DT_DFN   datatypes[] =
    {
	{ II_O_DATATYPE,
	    {"ord_pair"},
	    ORP_TYPE, II_NODT,
	    II_DT_NOBITS,
	    usop_lenchk, usop_compare, usop_keybld, usop_getempty,
	    0, 0, usop_valchk, usop_hashprep,
	    usop_helem, usop_hmin, usop_hmax, usop_dhmin,
	    usop_dhmax, usop_hg_dtln, usop_minmaxdv, 0,
	    0, usop_tmlen, usop_tmcvt, usop_dbtoev, 0, 0, 0, 0
	},
	{ II_O_DATATYPE,
	    {"intlist"},
	    INTLIST_TYPE, II_NODT,
	    II_DT_NOBITS,
	    usi4_lenchk, usi4_compare, usi4_keybld, usi4_getempty,
	    0, 0, usi4_valchk, usi4_hashprep,
	    usi4_helem, usi4_hmin, usi4_hmax, usi4_dhmin,
	    usi4_dhmax, usi4_hg_dtln, usi4_minmaxdv, 0,
	    0, usi4_tmlen, usi4_tmcvt, usi4_dbtoev, 0, 0, 0, 0
	},
	{ II_O_DATATYPE,
	    {"complex"},
	    COMPLEX_TYPE, II_NODT,
	    II_DT_NOBITS,
	    uscpx_lenchk, uscpx_compare, uscpx_keybld, uscpx_getempty,
	    0, 0, uscpx_valchk, uscpx_hashprep,
	    uscpx_helem, uscpx_hmin, uscpx_hmax, uscpx_dhmin,
	    uscpx_dhmax, uscpx_hg_dtln, uscpx_minmaxdv, 0,
	    0, uscpx_tmlen, uscpx_tmcvt, uscpx_dbtoev, 0, 0, 0, 0
	},
	{ II_O_DATATYPE,
	    {"zchar"},
	    ZCHAR_TYPE, II_NODT,
	    II_DT_NOBITS,
	    usz_lenchk, usz_compare, usz_keybld, usz_getempty,
	    0, 0, usz_valchk, usz_hashprep,
	    usz_helem, usz_hmin, usz_hmax, usz_dhmin,
	    usz_dhmax, usz_hg_dtln, usz_minmaxdv, 0,
	    0, usz_tmlen, usz_tmcvt, usz_dbtoev, 0, 0, 0, 0
	},
	{ II_O_DATATYPE,
	    {"pnum"},
	    PNUM_TYPE, II_NODT,
	    II_DT_NOBITS,
	    usnum_lenchk, usnum_compare, usnum_keybld, usnum_getempty,
	    0, 0, usnum_valchk, usnum_hashprep,
	    usnum_helem, usnum_hmin, usnum_hmax, usnum_dhmin,
	    usnum_dhmax, usnum_hg_dtln, usnum_minmaxdv, 0,
	    0, usnum_tmlen, usnum_tmcvt, usnum_dbtoev, 0, 0, 0, 0
	},
	{ II_O_DATATYPE,
	    {"long zchar"},
	    LZCHAR_TYPE, ZCHAR_TYPE,
	    (II_DT_NOKEY | II_DT_NOSORT
	                 | II_DT_NOHISTOGRAM | II_DT_PERIPHERAL),
	    usz_lenchk, 0, 0, usz_getempty,
	    0, 0, usz_valchk, 0,
	    0, 0, 0, 0,
	    0, 0, 0, 0, 
	    0, usz_tmlen, usz_tmcvt, usz_dbtoev, 0,
#ifdef __STDC__
	      (II_XFORM_FUNC *) uslz_xform, uslz_seglen, 0
#else
	      uslz_xform, uslz_seglen, 0
#endif
	},
	{ II_O_DATATYPE,
	    {"nvarchar"},
	    VARUCS2_TYPE, II_NODT,
	    II_DT_NOBITS,
	    vucs2_lenchk, vucs2_compare, vucs2_keybld, vucs2_getempty,
	    0, 0, vucs2_valchk, vucs2_hashprep,
	    vucs2_helem, vucs2_hmin, vucs2_hmax, vucs2_dhmin,
	    vucs2_dhmax, vucs2_hg_dtln, vucs2_minmaxdv, 0,
	    0, vucs2_tmlen, vucs2_tmcvt, vucs2_dbtoev, 0, 0, 0, 0
	},
    };

/*
**  The following define the operator identifiers used to specify the operators
**  used for the ordered pair and integer list datatypes.
*/
#define             OP_DISTANCE         II_OPSTART
#define		    OP_SLOPE		(OP_DISTANCE+1)
#define		    OP_MIDPOINT		(OP_SLOPE+1)
#define		    OP_XCOORD		(OP_MIDPOINT+1)
#define		    OP_YCOORD		(OP_XCOORD+1)
#define		    OP_ORDPAIR		(OP_YCOORD+1)
#define		    OP2CPX		(OP_ORDPAIR+1)

#define             I4_ELEMENT		(OP2CPX+1)
#define		    I4_CONVERT		(I4_ELEMENT+1)
#define		    I4_TOTAL		(I4_CONVERT+1)
#define		    OP_TRACE		(I4_TOTAL+1)

#define		    CPX_CONJ		(OP_TRACE+1)
#define		    CPX_POLAR		(CPX_CONJ+1)
#define		    CPX2OP		(CPX_POLAR+1)
#define		    CPX_REAL		(CPX2OP+1)
#define		    CPX_IM		(CPX_REAL+1)
#define		    CPX_CPX		(CPX_IM+1)

#define		    Z_ZCHAR		(CPX_CPX+1)

#define		    NUM_NUMERIC		(Z_ZCHAR+1)
#define		    NUM_CURRENCY	(Z_ZCHAR+2)
#define		    NUM_FRACTION	(Z_ZCHAR+3)

#define             VUCS2_VARUCS2       (NUM_FRACTION+1)
/*
**  The following array specifies the operators to be added to support the
**  ordered pair and integer list datatypes.
**
**  Each element contains the encoding that this is an operator, the name of the
**  operator, the operator identifier, and the operation type.  See the include
**  file <iiadd.h> for more details.
*/
static IIADD_FO_DFN   operators[] =
    {
	{ II_O_OPERATION, { "distance"},
	    OP_DISTANCE, II_NORMAL },
	{ II_O_OPERATION, { "slope"},
	    OP_SLOPE, II_NORMAL },	    
	{ II_O_OPERATION, { "midpoint"},
	    OP_MIDPOINT, II_NORMAL },	    
	{ II_O_OPERATION, { "x_coord"},
	    OP_XCOORD, II_NORMAL },	    
	{ II_O_OPERATION, { "y_coord"},
	    OP_YCOORD, II_NORMAL },	    
	{ II_O_OPERATION, { "ord_pair"},
	    OP_ORDPAIR, II_NORMAL },
	{ II_O_OPERATION, { "re_im" },
	    OP2CPX, II_NORMAL },

	{ II_O_OPERATION, { "element"},
	    I4_ELEMENT, II_NORMAL },
	{ II_O_OPERATION, { "intlist"},
	    I4_CONVERT, II_NORMAL },
	{ II_O_OPERATION, { "total" },
	    I4_TOTAL, II_NORMAL },
	{ II_O_OPERATION, { "trace_message" },
	    OP_TRACE, II_NORMAL },

	{ II_O_OPERATION, {"conj"}, CPX_CONJ, II_NORMAL },
	{ II_O_OPERATION, {"polar"}, CPX_POLAR, II_NORMAL },
	{ II_O_OPERATION, {"xy"}, CPX2OP, II_NORMAL },
	{ II_O_OPERATION, {"re"}, CPX_REAL, II_NORMAL },
	{ II_O_OPERATION, {"im"}, CPX_IM, II_NORMAL },
	{ II_O_OPERATION, {"complex"}, CPX_CPX, II_NORMAL },

	{ II_O_OPERATION, {"zchar"}, Z_ZCHAR, II_NORMAL },

	{ II_O_OPERATION, {"pnum"}, NUM_NUMERIC, II_NORMAL },
	{ II_O_OPERATION, {"currency"}, NUM_CURRENCY, II_NORMAL },
	{ II_O_OPERATION, {"fraction"}, NUM_FRACTION, II_NORMAL },
	{ II_O_OPERATION, {"nvarchar"}, VUCS2_VARUCS2, II_NORMAL }
    };

/*
**  The following static arrays identify parameter lists for various function
**  instances.  Each is an array of 1 or 2 elements, specifying the datatype for
**  the first and [optionally] second argument for the function instance.
*/

static	II_DT_ID comp_parms[] = { ORP_TYPE, ORP_TYPE };
static	II_DT_ID xy_parms[] = { ORP_TYPE, ORP_TYPE };
static  II_DT_ID op_parms[] = { II_FLOAT, II_FLOAT};
static  II_DT_ID op2char_parms[] = { ORP_TYPE };
static  II_DT_ID op2ltxt_parms[] = { ORP_TYPE };
static  II_DT_ID op2op_parms[] = { ORP_TYPE };
static  II_DT_ID char2op_parms[] = { II_CHAR};
static  II_DT_ID varchar2op_parms[] = { II_VARCHAR };
static	II_DT_ID ascii_parms[] = { ORP_TYPE };
static	II_DT_ID add_parms[] = { ORP_TYPE, ORP_TYPE };

static	II_DT_ID i4_comp_parms[] = { INTLIST_TYPE, INTLIST_TYPE };
static  II_DT_ID i4_add_parms[] = { INTLIST_TYPE, INTLIST_TYPE };
static  II_DT_ID i4_length_parms[] = { INTLIST_TYPE };
static	II_DT_ID i4_element_parms[] = { INTLIST_TYPE, II_INTEGER };
static	II_DT_ID i4_convert_parms[] = { II_INTEGER};
static  II_DT_ID i4_list2char_parms[] = { INTLIST_TYPE };
static  II_DT_ID i42ltxt_parms[] = { INTLIST_TYPE };
static	II_DT_ID i4_list2ltxt_parms[] = { INTLIST_TYPE };
static  II_DT_ID i4_list2list_parms[] = { INTLIST_TYPE };
static  II_DT_ID i4_char2list_parms[] = { II_CHAR };
static  II_DT_ID i4_varchar2list_parms[] = { II_VARCHAR };
static	II_DT_ID i4_locate_parms[] = { INTLIST_TYPE, II_INTEGER };
static	II_DT_ID i4_total_parms[] = { INTLIST_TYPE };
static	II_DT_ID op_trace_parms[] = { II_INTEGER, II_CHAR };

static	II_DT_ID cpx_complex_parms[] = { II_FLOAT, II_FLOAT };
static	II_DT_ID cpx_re_parms[] = { COMPLEX_TYPE };
static	II_DT_ID cpx_im_parms[] = {  COMPLEX_TYPE };
static	II_DT_ID cpx_parms[] = {  COMPLEX_TYPE,  COMPLEX_TYPE };
static  II_DT_ID cpx_vc_parms[] = { II_VARCHAR };
static  II_DT_ID cpx_char_parms[] = { II_CHAR };

static  II_DT_ID zchar_comp_parms[] = { ZCHAR_TYPE, ZCHAR_TYPE };
static  II_DT_ID zchar_parms[] = { ZCHAR_TYPE };
static  II_DT_ID char_parms[] = { II_CHAR };
static  II_DT_ID c_parms[] = { II_C };
static  II_DT_ID text_parms[] = { II_TEXT };
static  II_DT_ID varchar_parms[] = { II_VARCHAR };
static	II_DT_ID longtext_parms[] = { II_LONGTEXT };
static  II_DT_ID lz_lvch2_parms[] = { II_LVCH };
static  II_DT_ID lz_2lvch_parms[] = { LZCHAR_TYPE };
static  II_DT_ID lz_vch2_parms[]  = { II_VARCHAR };
static  II_DT_ID lz_concat_parms[] = { LZCHAR_TYPE, LZCHAR_TYPE };

static	II_DT_ID num_comp_parms[] = { PNUM_TYPE, PNUM_TYPE};
static	II_DT_ID num_func_parms[] = { PNUM_TYPE, II_CHAR};
static	II_DT_ID num_numeric_parms[] = { II_CHAR, II_CHAR};
static	II_DT_ID num_math_parms[] = { PNUM_TYPE, PNUM_TYPE};
static  II_DT_ID int_parms[] = { II_INTEGER };
static	II_DT_ID float_parms[] = { II_FLOAT };
static	II_DT_ID dec_parms[] = { II_DECIMAL };
static  II_DT_ID numeric_parms[] = { PNUM_TYPE };

static  II_DT_ID vucs2_comp_parms[] = { VARUCS2_TYPE, VARUCS2_TYPE };
static  II_DT_ID vucs2_parms[] = { VARUCS2_TYPE };
static  II_DT_ID vucs2_concat_parms[] = { VARUCS2_TYPE , VARUCS2_TYPE };

/*
**  The following #define's specify the identifiers for all the function
**  instances used by the ordered pair and integer list datatypes.
*/

#define             FI_NOTEQUAL          II_FISTART
#define		    FI_EQUAL		 (FI_NOTEQUAL+1)
#define		    FI_LESS		 (FI_EQUAL+1)
#define		    FI_GREATEREQUAL	 (FI_LESS+1)
#define		    FI_LESSEQUAL	 (FI_GREATEREQUAL+1)
#define		    FI_GREATER		 (FI_LESSEQUAL+1)
#define		    FI_DISTANCE		 (FI_GREATER+1)
#define		    FI_SLOPE		 (FI_DISTANCE+1)
#define		    FI_MIDPOINT		 (FI_SLOPE+1)
#define		    FI_XCOORD		 (FI_MIDPOINT+1)
#define		    FI_YCOORD		 (FI_XCOORD+1)
#define		    FI_ORDPAIR		 (FI_YCOORD+1)
#define		    FI_OP2CHAR		 (FI_ORDPAIR+1)
#define		    FI_OP2OP		 (FI_OP2CHAR+1)
#define		    FI_OP2CPX		 (FI_OP2OP+1)
#define		    FI_CHAR2OP		 (FI_OP2CPX+1)
#define		    FI_VARCHAR2OP	 (FI_CHAR2OP+1)
#define		    FI_ASCII		 (FI_VARCHAR2OP+1)
#define		    FI_ADD		 (FI_ASCII+1)

#define             FI_I4NOTEQUAL        (FI_ADD+1)
#define		    FI_I4EQUAL		 (FI_I4NOTEQUAL+1)
#define		    FI_I4LESS		 (FI_I4EQUAL+1)
#define		    FI_I4GREATEREQUAL	 (FI_I4LESS+1)
#define		    FI_I4LESSEQUAL	 (FI_I4GREATEREQUAL+1)
#define		    FI_I4GREATER	 (FI_I4LESSEQUAL+1)
#define		    FI_I4ADD		 (FI_I4GREATER+1)
#define		    FI_I4ELEMENT	 (FI_I4ADD+1)
#define		    FI_I4LENGTH		 (FI_I4ELEMENT+1)
#define		    FI_I4INT2I4		 (FI_I4LENGTH+1)
#define		    FI_I42CHAR		 (FI_I4INT2I4+1)
#define		    FI_I42I4		 (FI_I42CHAR+1)
#define		    FI_I4CHAR2I4	 (FI_I42I4+1)
#define		    FI_I4VARCHAR2I4	 (FI_I4CHAR2I4+1)
#define		    FI_I4VARCHAR2LONGI4	 (FI_I4VARCHAR2I4+1)
#define		    FI_I4CHAR2LONGI4	 (FI_I4VARCHAR2LONGI4+1)
#define		    FI_I4FUNCINT2I4	 (FI_I4CHAR2LONGI4+1)
#define		    FI_I4LOCATE		 (FI_I4FUNCINT2I4+1)
#define		    FI_I4TOTAL		 (FI_I4LOCATE+1)
#define		    FI_OP2LTXT		 (FI_I4TOTAL+1)
#define		    FI_I42LTXT		 (FI_OP2LTXT+1)
#define		    FI_LTXT2OP		 (FI_I42LTXT+1)
#define		    FI_LTXT2I4		 (FI_LTXT2OP+1)
#define		    FI_TRACE		 (FI_LTXT2I4+1)

#define		    FI_CPXEQUAL		 (FI_LTXT2I4+2)
#define		    FI_CPXNOTEQUAL	 (FI_LTXT2I4+3)
#define		    FI_CPXLESS		 (FI_LTXT2I4+4)
#define		    FI_CPXGREATEREQUAL	 (FI_LTXT2I4+5)
#define		    FI_CPXLESSEQUAL	 (FI_LTXT2I4+6)
#define		    FI_CPXGREATER	 (FI_LTXT2I4+7)
#define		    FI_CPX2CHAR		 (FI_LTXT2I4+8)
#define		    FI_CPX2CPX		 (FI_LTXT2I4+9)
#define		    FI_CPX2VARCHAR       (FI_LTXT2I4+10)
#define		    FI_CPX2LTXT	         (FI_LTXT2I4+11)
#define		    FI_CHAR2CPX		 (FI_LTXT2I4+12)
#define		    FI_VARCHAR2CPX	 (FI_LTXT2I4+13)
#define		    FI_LTXT2CPX	         (FI_LTXT2I4+14)
#define		    FI_CPXASCII		 (FI_LTXT2I4+15)
#define		    FI_CPXADD		 (FI_LTXT2I4+16)
#define		    FI_CPXSUB		 (FI_LTXT2I4+17)
#define		    FI_CPXMUL		 (FI_LTXT2I4+18)
#define		    FI_CPXDIV		 (FI_LTXT2I4+19)
#define		    FI_CPXMINUS		 (FI_LTXT2I4+20)
#define		    FI_ABS		 (FI_LTXT2I4+21)
#define		    FI_CONJ		 (FI_LTXT2I4+22)
#define		    FI_CPX2OP		 (FI_LTXT2I4+23)
#define		    FI_POLAR		 (FI_LTXT2I4+24)
#define		    FI_RE		 (FI_LTXT2I4+25)
#define		    FI_IM		 (FI_LTXT2I4+26)
#define		    FI_COMPLEX		 (FI_LTXT2I4+27)

#define             FI_ZNOTEQUAL         (FI_COMPLEX+1)
#define		    FI_ZEQUAL		 (FI_COMPLEX+2)
#define		    FI_ZLESS		 (FI_COMPLEX+3)
#define		    FI_ZGREATEREQUAL	 (FI_COMPLEX+4)
#define		    FI_ZLESSEQUAL	 (FI_COMPLEX+5)
#define		    FI_ZGREATER		 (FI_COMPLEX+6)
#define		    FI_ZCH2CHAR	 	 (FI_COMPLEX+7)
#define		    FI_ZCH2C	 	 (FI_COMPLEX+8)
#define		    FI_ZCH2TEXT	 	 (FI_COMPLEX+9)
#define		    FI_ZCH2VARCHAR	 (FI_COMPLEX+10)
#define		    FI_ZCH2LTXT	         (FI_COMPLEX+11)
#define		    FI_ZCH2ZCH		 (FI_COMPLEX+12)
#define		    FI_CHAR2ZCH	 	 (FI_COMPLEX+13)
#define		    FI_C2ZCH	 	 (FI_COMPLEX+14)
#define		    FI_TEXT2ZCH	 	 (FI_COMPLEX+15)
#define		    FI_VARCHAR2ZCH	 (FI_COMPLEX+16)
#define		    FI_LTXT2ZCH	         (FI_COMPLEX+17)
#define		    FI_SIZE		 (FI_COMPLEX+18)
#define		    FI_ZCHAR		 (FI_COMPLEX+19)
#define		    FI_LIKE		 (FI_COMPLEX+20)
#define		    FI_UNLIKE		 (FI_COMPLEX+21)
#define             FI_LVCH2LZCH         (FI_COMPLEX+22)
#define             FI_LZCH2LVCH         (FI_COMPLEX+23)
#define             FI_VCH2LZCH          (FI_COMPLEX+24)
#define             FI_LZCHCONCAT        (FI_COMPLEX+25)
#define             FI_LZCHADD           (FI_COMPLEX+26)

#define		    FI_NUMEQUAL		 (FI_LZCHADD+1)
#define		    FI_NUMNOTEQUAL	 (FI_LZCHADD+2)
#define		    FI_NUMLESS		 (FI_LZCHADD+3)
#define		    FI_NUMGREATEREQUAL	 (FI_LZCHADD+4)
#define		    FI_NUMLESSEQUAL	 (FI_LZCHADD+5)
#define		    FI_NUMGREATER	 (FI_LZCHADD+6)
#define		    FI_NUM2CHR		 (FI_LZCHADD+7)
#define		    FI_CHR2NUM		 (FI_LZCHADD+8)
#define		    FI_NUM2CHA		 (FI_LZCHADD+9)
#define		    FI_CHA2NUM		 (FI_LZCHADD+10)
#define		    FI_NUM2TXT		 (FI_LZCHADD+11)
#define		    FI_TXT2NUM		 (FI_LZCHADD+12)
#define		    FI_NUM2LTXT		 (FI_LZCHADD+13)
#define		    FI_LTXT2NUM		 (FI_LZCHADD+14)
#define		    FI_VCH2NUM		 (FI_LZCHADD+15)
#define             FI_NUM2VCH           (FI_LZCHADD+16)
#define             FI_NUM2NUM           (FI_LZCHADD+17)
#define             FI_INT2NUM           (FI_LZCHADD+18)
#define             FI_NUM2INT           (FI_LZCHADD+19)
#define             FI_FLT2NUM           (FI_LZCHADD+20)
#define             FI_NUM2FLT           (FI_LZCHADD+21)
#define	    	    FI_NUMMUL	    	 (FI_LZCHADD+22)
#define		    FI_NUMADD		 (FI_LZCHADD+23)
#define	    	    FI_NUMSUB	    	 (FI_LZCHADD+24)
#define	    	    FI_NUMDIV            (FI_LZCHADD+25)
#define		    FI_NUMERIC		 (FI_LZCHADD+26)
#define		    FI_NUMERIC1ARG	 (FI_LZCHADD+27)
#define		    FI_CURRENCY		 (FI_LZCHADD+28)
#define		    FI_CURRENCY1ARG	 (FI_LZCHADD+29)
#define		    FI_FRACTION		 (FI_LZCHADD+30)
#define		    FI_FRACTION1ARG 	 (FI_LZCHADD+31)
#define             FI_DEC2NUM           (FI_LZCHADD+32)
#define             FI_NUM2DEC           (FI_LZCHADD+33)

#define             FI_VARUCS2           (FI_NUM2DEC+1)
#define             FI_VUCS2EQUAL        (FI_NUM2DEC+2)
#define             FI_VUCS2NOTEQUAL     (FI_NUM2DEC+3)
#define             FI_VUCS2LESS         (FI_NUM2DEC+4)
#define             FI_VUCS2GREATEREQUAL (FI_NUM2DEC+5)
#define             FI_VUCS2LESSEQUAL    (FI_NUM2DEC+6)
#define             FI_VUCS2GREATER      (FI_NUM2DEC+7)
#define             FI_VUCS2LIKE         (FI_NUM2DEC+8)
#define             FI_VUCS2UNLIKE       (FI_NUM2DEC+9)
#define             FI_VUCS22CHAR        (FI_NUM2DEC+10)
#define             FI_VUCS22C           (FI_NUM2DEC+11)
#define             FI_VUCS22TEXT        (FI_NUM2DEC+12)
#define             FI_VUCS22VARCHAR     (FI_NUM2DEC+13)
#define             FI_VUCS22VUCS2       (FI_NUM2DEC+14)
#define             FI_CHAR2VUCS2        (FI_NUM2DEC+15)
#define             FI_C2VUCS2           (FI_NUM2DEC+16)
#define             FI_TEXT2VUCS2        (FI_NUM2DEC+17)
#define             FI_VARCHAR2VUCS2     (FI_NUM2DEC+18)
#define             FI_VUCS2SIZE         (FI_NUM2DEC+19)
#define             FI_VUCS2ADD          (FI_NUM2DEC+20)
#define             FI_VUCS2CONCAT       (FI_NUM2DEC+21)


/*
**  The following array lists the function instances to be added for use in
**  ordered pair and integer list datatypes.  Each element contains the function
**  instance element encoding, the function instance identifier, the complement
**  function instance identifier (if applicable), the operator which invokes the
**  function instance, the type of instance, the parameter count, a pointer to
**  the parameter types, the result type, the result length specifier, the
**  precision specifier, and the function address to call to perform the
**  function instance.  For more details, see the include file <iiadd.h>.
*/

static IIADD_FI_DFN   function_instances[] =
    {
	{ II_O_FUNCTION_INSTANCE, FI_NOTEQUAL, FI_EQUAL,
		II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I4NOTEQUAL, FI_I4EQUAL,
		II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXNOTEQUAL, FI_CPXEQUAL,
		II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_compare , 0 },
 	    
	{ II_O_FUNCTION_INSTANCE, FI_ZNOTEQUAL, FI_ZEQUAL,
		II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMNOTEQUAL, FI_NUMEQUAL,
		II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_VUCS2NOTEQUAL, FI_VUCS2EQUAL,
	        II_NE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, vucs2_compare, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LESS, FI_GREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I4LESS, FI_I4GREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXLESS, FI_CPXGREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_bad_op , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_ZLESS, FI_ZGREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMLESS, FI_NUMGREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare , 0 },
	
	{ II_O_FUNCTION_INSTANCE, FI_VUCS2LESS, FI_VUCS2GREATEREQUAL,
		II_LT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, vucs2_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_LESSEQUAL, FI_GREATER,
		II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I4LESSEQUAL, FI_I4GREATER,
		II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_CPXLESSEQUAL, FI_CPXGREATER,
		II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_bad_op , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_ZLESSEQUAL, FI_ZGREATER,
		II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_NUMLESSEQUAL, FI_NUMGREATER,
		II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2LESSEQUAL, FI_VUCS2GREATER,
                II_LE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_EQUAL, FI_NOTEQUAL,
		II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I4EQUAL, FI_I4NOTEQUAL,
		II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_CPXEQUAL, FI_CPXNOTEQUAL,
		II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_ZEQUAL, FI_ZNOTEQUAL,
		II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_NUMEQUAL, FI_NUMNOTEQUAL,
		II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare, 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2EQUAL, FI_VUCS2NOTEQUAL,
                II_EQ_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_compare, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_GREATER, FI_LESSEQUAL,
		II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I4GREATER, FI_I4LESSEQUAL,
		II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_CPXGREATER, FI_CPXLESSEQUAL,
		II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_bad_op , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_ZGREATER, FI_ZLESSEQUAL,
		II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMGREATER, FI_NUMLESSEQUAL,
		II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2GREATER, FI_VUCS2LESSEQUAL,
                II_GT_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_GREATEREQUAL, FI_LESS,
		II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usop_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4GREATEREQUAL, FI_I4LESS,
		II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, i4_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usi4_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXGREATEREQUAL, FI_CPXLESS,
		II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, cpx_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, uscpx_bad_op , 0 },
	    
        { II_O_FUNCTION_INSTANCE, FI_ZGREATEREQUAL, FI_ZLESS,
                II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, usz_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMGREATEREQUAL, FI_NUMLESS,
		II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, num_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usnum_compare , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2GREATEREQUAL, FI_VUCS2LESS,
                II_GE_OP, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_compare , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LIKE, FI_UNLIKE,
		80, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_like , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2LIKE, FI_VUCS2UNLIKE,
                80, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_like , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_UNLIKE, FI_LIKE,
		81, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
	    2, zchar_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
	    II_PSVALID, usz_like , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2UNLIKE, FI_VUCS2LIKE,
                81, II_COMPARISON, II_FID_F0_NOFLAGS, 0,
            2, vucs2_comp_parms, II_BOOLEAN, II_RES_FIXED, sizeof(char),
            II_PSVALID, vucs2_like , 0 },


    /* Begin Operator instances */
    
	{ II_O_FUNCTION_INSTANCE, FI_CPXMUL, II_NO_FI,
		II_MUL_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, cpx_parms, COMPLEX_TYPE,
		    II_RES_FIXED, sizeof(COMPLEX),
		    II_PSVALID, uscpx_mul, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMMUL, II_NO_FI,
		II_MUL_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, num_math_parms, PNUM_TYPE,
		    II_RES_LONGER, II_LEN_UNKNOWN,
		    II_PSVALID, usnum_mul, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4ADD, II_NO_FI,
		II_ADD_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, i4_add_parms, INTLIST_TYPE,
		    II_RES_LONGER, II_LEN_UNKNOWN,
		    II_PSVALID, usi4_concatenate, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ADD, II_NO_FI,
		II_ADD_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, add_parms, ORP_TYPE,
		    II_RES_FIXED, sizeof(ORD_PAIR),
		    II_PSVALID, usop_add, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXADD, II_NO_FI,
		II_ADD_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, cpx_parms, COMPLEX_TYPE,
		    II_RES_FIXED, sizeof(COMPLEX),
		    II_PSVALID, uscpx_add, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMADD, II_NO_FI,
		II_ADD_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, num_math_parms, PNUM_TYPE,
		    II_RES_LONGER, II_LEN_UNKNOWN,
		    II_PSVALID, usnum_add, 0 },

    	{ II_O_FUNCTION_INSTANCE, FI_LZCHADD, II_NO_FI,
	        II_ADD_OP, II_OPERATOR,
	                (II_FID_F4_WORKSPACE | II_FID_F8_INDIRECT),
	                      (2 * 2048),
	        2, lz_concat_parms, LZCHAR_TYPE,
	            II_RES_FIXED, sizeof(II_PERIPHERAL),
	            II_PSVALID, uslz_concat, 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2ADD, II_NO_FI,
                II_ADD_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
                2, vucs2_concat_parms, VARUCS2_TYPE,
                    II_RES_LONGER, II_LEN_UNKNOWN,
                    II_PSVALID, vucs2_concatenate, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXSUB, II_NO_FI,
		II_SUB_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, cpx_parms, COMPLEX_TYPE,
		    II_RES_FIXED, sizeof(COMPLEX),
		    II_PSVALID, uscpx_sub, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMSUB, II_NO_FI,
		II_SUB_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, num_math_parms, PNUM_TYPE,
		    II_RES_LONGER, II_LEN_UNKNOWN,
		    II_PSVALID, usnum_sub, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXDIV, II_NO_FI,
		II_DIV_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, cpx_parms, COMPLEX_TYPE,
		    II_RES_FIXED, sizeof(COMPLEX),
		    II_PSVALID, uscpx_div, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMDIV, II_NO_FI,
		II_DIV_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		2, num_math_parms, PNUM_TYPE,
		    II_RES_FIRST, II_LEN_UNKNOWN,
		    II_PSVALID, usnum_div, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXMINUS, II_NO_FI,
		II_MINUS_OP, II_OPERATOR, II_FID_F0_NOFLAGS, 0,
		1, cpx_re_parms, COMPLEX_TYPE,
		II_RES_FIXED, sizeof(COMPLEX), II_PSVALID, uscpx_negate , 0 },

    /* Begin `Normal' instances */

	{ II_O_FUNCTION_INSTANCE, FI_ABS, II_NO_FI,
		II_ABS_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_FLOAT, II_RES_FIXED, 8,
	    II_PSVALID, uscpx_abs , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ASCII, II_NO_FI,
		II_ASCII_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, ascii_parms, II_C, II_RES_FIXED,
		    26, II_PSVALID, usop_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPXASCII, II_NO_FI,
		II_ASCII_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_C, II_RES_FIXED,
		    28, II_PSVALID, uscpx_convert, 0 },

    	{ II_O_FUNCTION_INSTANCE, FI_LZCHCONCAT, II_NO_FI,
	        II_CNCAT_OP, II_NORMAL,
                    (II_FID_F4_WORKSPACE | II_FID_F8_INDIRECT),
	                   (2 * 2048),
	        2, lz_concat_parms, LZCHAR_TYPE,
	            II_RES_FIXED, sizeof(II_PERIPHERAL),
	            II_PSVALID, uslz_concat, 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2CONCAT, II_NO_FI,
                II_CNCAT_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
                2, vucs2_concat_parms, VARUCS2_TYPE,
                    II_RES_LONGER, II_LEN_UNKNOWN,
                    II_PSVALID, vucs2_concatenate, 0 },

    { II_O_FUNCTION_INSTANCE, FI_I4LENGTH, II_NO_FI,
		II_LEN_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		1, i4_length_parms, II_INTEGER,
		II_RES_FIXED, 2, II_PSVALID, usi4_size , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4LOCATE, II_NO_FI,
		II_LOC_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		2, i4_locate_parms, II_INTEGER,
		II_RES_FIXED, 2, II_PSVALID, usi4_locate , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_SIZE, II_NO_FI,
		II_SIZE_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_INTEGER, II_RES_FIXED, 2,
	    II_PSVALID, usz_size , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS2SIZE, II_NO_FI,
                II_SIZE_OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, II_INTEGER, II_RES_FIXED, 2,
            II_PSVALID, vucs2_size , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_DISTANCE, II_NO_FI,
		OP_DISTANCE, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_FLOAT,
		    II_RES_FIXED, 8, II_PSVALID, usop_distance, 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_SLOPE, II_NO_FI,
		OP_SLOPE, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, II_FLOAT, II_RES_FIXED, 8, II_PSVALID, usop_slope, 0 },
	    
	{ II_O_FUNCTION_INSTANCE , FI_MIDPOINT, II_NO_FI,
		OP_MIDPOINT, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, comp_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR), II_PSVALID,
	    usop_midpoint, 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_XCOORD, II_NO_FI,
		OP_XCOORD, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, xy_parms, II_FLOAT, II_RES_FIXED, 8, II_PSVALID, usop_xcoord, 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_YCOORD, II_NO_FI,
		OP_YCOORD, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, xy_parms, II_FLOAT, II_RES_FIXED, 8, II_PSVALID, usop_ycoord, 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_ORDPAIR, II_NO_FI,
		OP_ORDPAIR, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, op_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_ordpair, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_OP2CPX, II_NO_FI,
		OP2CPX, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, op2op_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, usop2cpx, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4ELEMENT, II_NO_FI,
		I4_ELEMENT, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		2, i4_element_parms, II_INTEGER,
		II_RES_FIXED, 4, II_PSVALID, usi4_element , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4CHAR2LONGI4, II_NO_FI,
		I4_CONVERT, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		1, i4_char2list_parms, INTLIST_TYPE,
		II_RES_FIXED, 44, II_PSVALID, usi4_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4VARCHAR2LONGI4, II_NO_FI,
		I4_CONVERT, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		1, i4_varchar2list_parms, INTLIST_TYPE,
		II_RES_FIXED, 44, II_PSVALID, usi4_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4FUNCINT2I4, II_NO_FI,
		I4_CONVERT, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		1, i4_convert_parms, INTLIST_TYPE,
		II_RES_FIXED, 44, II_PSVALID, usi4_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4TOTAL, II_NO_FI,
		I4_TOTAL, II_NORMAL, II_FID_F0_NOFLAGS, 0,
		1, i4_total_parms, II_INTEGER,
		II_RES_FIXED, 4, II_PSVALID, usi4_total , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_TRACE, II_NO_FI,
		OP_TRACE, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, op_trace_parms, II_INTEGER, II_RES_FIXED, 4,
	    II_PSVALID, usop_trace , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CONJ, II_NO_FI,
		CPX_CONJ, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_conj , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_POLAR, II_NO_FI,
		CPX_POLAR, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, uscpx_rtheta , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPX2OP, II_NO_FI,
		CPX2OP, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, uscpx2op , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_RE, II_NO_FI,
		CPX_REAL, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_FLOAT, II_RES_FIXED, 8,
	    II_PSVALID, uscpx_real, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_IM, II_NO_FI,
		CPX_IM, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, cpx_im_parms, II_FLOAT, II_RES_FIXED, 8,
	    II_PSVALID, uscpx_imag, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_COMPLEX, II_NO_FI,
		CPX_CPX, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, cpx_complex_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_cmpnumber, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ZCHAR, II_NO_FI,
		Z_ZCHAR, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, varchar_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMERIC, II_NO_FI,
		NUM_NUMERIC, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, num_numeric_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_numeric , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUMERIC1ARG, II_NO_FI,
		NUM_NUMERIC, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, char_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_numeric_1arg , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CURRENCY, II_NO_FI,
		NUM_CURRENCY, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, num_func_parms, II_VARCHAR, II_RES_FIXED, II_MAX_NUMCURR,
	    II_PSVALID, usnum_currency , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CURRENCY1ARG, II_NO_FI,
		NUM_CURRENCY, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_VARCHAR, II_RES_FIXED, II_MAX_NUMCURR,
	    II_PSVALID, usnum_currency_1arg , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_FRACTION, II_NO_FI,
		NUM_FRACTION, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    2, num_func_parms, II_VARCHAR, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_fraction , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_FRACTION1ARG, II_NO_FI,
		NUM_FRACTION, II_NORMAL, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_VARCHAR, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_fraction_1arg , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VARUCS2, II_NO_FI,
                VUCS2_VARUCS2, II_NORMAL, II_FID_F0_NOFLAGS, 0,
            1, varchar_parms, VARUCS2_TYPE, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

    /* Begin Coercion Instances */

	{ II_O_FUNCTION_INSTANCE, FI_OP2CHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, op2char_parms, II_CHAR, II_RES_FIXED, 26,
	    II_PSVALID, usop_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_OP2LTXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
		1, op2ltxt_parms, II_LONGTEXT, II_RES_FIXED, 28,
		II_PSVALID, usop_convert, 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_OP2OP, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, op2op_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_CHAR2OP, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, char2op_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_VARCHAR2OP, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, varchar2op_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_LTXT2OP, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, longtext_parms, ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_I42CHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, i4_list2char_parms, II_CHAR, II_RES_FIXED, 50,
	    II_PSVALID, usi4_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I42LTXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
		1, i42ltxt_parms, II_LONGTEXT, II_RES_FIXED, 52,
		II_PSVALID, usi4_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I42I4, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, i4_list2list_parms, INTLIST_TYPE, II_RES_KNOWN, II_LEN_UNKNOWN,
	    II_PSVALID, usi4_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4CHAR2I4, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, i4_char2list_parms, INTLIST_TYPE, II_RES_FIXED, 44,
	    II_PSVALID, usi4_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_I4VARCHAR2I4, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, i4_varchar2list_parms, INTLIST_TYPE, II_RES_FIXED, 44,
	    II_PSVALID, usi4_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LTXT2I4, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, longtext_parms, INTLIST_TYPE, II_RES_FIXED, 44,
	    II_PSVALID, usi4_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPX2CHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_CHAR, II_RES_FIXED, 28,
	    II_PSVALID, uscpx_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPX2VARCHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_VARCHAR, II_RES_FIXED, 28,
	    II_PSVALID, uscpx_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CPX2LTXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, II_LONGTEXT, II_RES_FIXED, 28,
	    II_PSVALID, uscpx_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_CPX2CPX, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_re_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CHAR2CPX, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_char_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_VARCHAR2CPX, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, cpx_vc_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LTXT2CPX, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, longtext_parms, COMPLEX_TYPE, II_RES_FIXED, sizeof(COMPLEX),
	    II_PSVALID, uscpx_convert , 0 },

/*	{ II_O_FUNCTION_INSTANCE, FI_ZCH2CHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_CHAR, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ZCH2C, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_C, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },*/

	{ II_O_FUNCTION_INSTANCE, FI_ZCH2TEXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_TEXT, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ZCH2VARCHAR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_VARCHAR, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ZCH2LTXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, II_LONGTEXT, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_ZCH2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, zchar_parms, ZCHAR_TYPE, II_RES_KNOWN, II_LEN_UNKNOWN,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CHAR2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, char_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_C2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, c_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_TEXT2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, text_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_VARCHAR2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, varchar_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LTXT2ZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, longtext_parms, ZCHAR_TYPE, II_RES_FIXED, 30,
	    II_PSVALID, usz_convert , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_LVCH2LZCH, II_NO_FI,
		II_NOOP, II_COERCION,
	                (II_FID_F4_WORKSPACE | II_FID_F8_INDIRECT),
	                         (2 * 2048),
	      1, lz_lvch2_parms, LZCHAR_TYPE, II_RES_FIXED,
	      sizeof(II_PERIPHERAL), II_PSVALID, uslz_lmove , 0 },
	    
	{ II_O_FUNCTION_INSTANCE, FI_LZCH2LVCH, II_NO_FI,
		II_NOOP, II_COERCION, 
	                (II_FID_F4_WORKSPACE | II_FID_F8_INDIRECT),
	                         (2 * 2048),
	      1, lz_2lvch_parms, II_LVCH, II_RES_FIXED,
	      sizeof(II_PERIPHERAL), II_PSVALID, uslz_lmove , 0 },

    	{ II_O_FUNCTION_INSTANCE, FI_VCH2LZCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	      1, lz_vch2_parms, LZCHAR_TYPE, II_RES_FIXED,
	      sizeof(II_PERIPHERAL), II_PSVALID, uslz_move , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2CHR, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_C, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CHR2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, c_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2CHA, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_CHAR, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_CHA2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, char_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2TXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_TEXT, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_TXT2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, text_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2LTXT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_CHAR, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_LTXT2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, longtext_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2VCH, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_VARCHAR, II_RES_FIXED, II_MAX_NUMLEN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_VCH2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, varchar_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, PNUM_TYPE, II_RES_FIRST, II_LEN_UNKNOWN,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_INT2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, int_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_NUM2INT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_INTEGER, II_RES_FIXED, 4,
	    II_PSVALID, usnum_convert , 0 },

	{ II_O_FUNCTION_INSTANCE, FI_FLT2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, float_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },  

	{ II_O_FUNCTION_INSTANCE, FI_NUM2FLT, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_FLOAT, II_RES_FIXED, 8,
	    II_PSVALID, usnum_convert, 0 },

	{ II_O_FUNCTION_INSTANCE, FI_DEC2NUM, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, dec_parms, PNUM_TYPE, II_RES_FIXED, II_MAX_NUMBYTE,
	    II_PSVALID, usnum_convert , 0 },  

	{ II_O_FUNCTION_INSTANCE, FI_NUM2DEC, II_NO_FI,
		II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
	    1, numeric_parms, II_DECIMAL, II_RES_FIXED, 8,
	    II_PSVALID, usnum_convert, 0 },

  /* ****************************************************************** */
        { II_O_FUNCTION_INSTANCE, FI_VUCS22CHAR, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, II_CHAR, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS22C, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, II_C, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS22TEXT, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, II_TEXT, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS22VARCHAR, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, II_VARCHAR, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VUCS22VUCS2, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, vucs2_parms, VARUCS2_TYPE, II_RES_KNOWN, II_LEN_UNKNOWN,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_CHAR2VUCS2, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, char_parms, VARUCS2_TYPE, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_C2VUCS2, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, c_parms, VARUCS2_TYPE, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_TEXT2VUCS2, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, text_parms, VARUCS2_TYPE, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 },

        { II_O_FUNCTION_INSTANCE, FI_VARCHAR2VUCS2, II_NO_FI,
                II_NOOP, II_COERCION, II_FID_F0_NOFLAGS, 0,
            1, varchar_parms, VARUCS2_TYPE, II_RES_FIXED, 200,
            II_PSVALID, vucs2_convert , 0 }

  /* ********************************************************************* */
    };

static  IIADD_DEFINITION	register_block =
{
    (IIADD_DEFINITION *) 0,
    (IIADD_DEFINITION *) 0,
    sizeof(IIADD_DEFINITION),
    IIADD_DFN2_TYPE,
    0,
    0,
    0,
    0,
    IIADD_INCONSISTENT,	/* Ignore the current datatypes known by the recovery
				    system */
    2,	/* Major id */
    0,	/* minor id */
    50,
    "UDT demo (ord_pair, intlist, complex, zchar, pnum)",
    IIADD_T_FAIL_MASK | IIADD_T_LOG_MASK,
    0,
    (sizeof(datatypes)/sizeof(IIADD_DT_DFN)),
    datatypes,
    (sizeof(operators)/sizeof(IIADD_FO_DFN)),
    operators,
    (sizeof(function_instances)/sizeof(IIADD_FI_DFN)),
    function_instances
};


/*{
** Name: IIUDADT_REGISTER	- Add the datatype to the server
**
** Description:
**      This routine is called by the DBMS server to add obtain information to
**	add the datatype to the server.  It simply fills in the provided
**	structure and returns. 
**
** Inputs:
**      ui_block                        Pointer to user information block.
**	callback_block			Pointer to an II_CALLBACKS structure
**					which contains information about INGRES
**					callbacks which are available.
**
**					Note that after this routine returns
**					the address of this block is no longer
**					valid.  Therefore, this routine must
**					copy the contents in which it is
**					interested before returning.
**
** Outputs:
**      *ui_block                       User information block
**
**	Returns:
**	    II_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      02-Mar-1989 (fred)
##          Created.
##	20-Feb-2001 ( fanra01, gupsh01)
##	    Added new ifdef #NEED_THIS, the udadt_register
##	    is now moved to nvarchar.c
##
[@history_template@]...
*/
#ifdef NEED_THIS
II_STATUS
#ifdef __STDC__
IIudadt_register(IIADD_DEFINITION  **ui_block_ptr ,
                  II_CALLBACKS  *callback_block )
#else
IIudadt_register(ui_block_ptr, callback_block)
IIADD_DEFINITION  **ui_block_ptr;
II_CALLBACKS      *callback_block;
#endif
{

    register_block.add_count = register_block.add_dt_cnt +
				register_block.add_fo_cnt +
				register_block.add_fi_cnt;
    *ui_block_ptr = &register_block;

    /*
    ** Note that after this call, the call back block address will
    ** no longer be valid (the function addresses will, but the block
    ** address itself will not).  Therefore, it is necessary to
    ** copy the values in which we are interested before returning.
    */    
    
    if (callback_block && callback_block->ii_cb_version >= II_CB_V1)
    {
	Ingres_trace_function = callback_block->ii_cb_trace;
	if (callback_block->ii_cb_version >= II_CB_V2)
	{
	    usc_lo_handler = callback_block->ii_lo_handler_fcn;
	    usc_lo_filter = callback_block->ii_filter_fcn;
	    usc_setup_workspace = callback_block->ii_init_filter_fcn;
	    usc_error = callback_block->ii_error_fcn;
	}
    }
    else
    {
	Ingres_trace_function = 0;
    }
    return(II_OK);
} 
#endif

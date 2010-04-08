/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/
#include    <iiadd.h>
#include    <ctype.h>
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

#else

#define		    I2ASSIGN_MACRO(a,b)		((*(short *)&(b)) = (*(short *)&(a)))
#define		    I4ASSIGN_MACRO(a,b)		((*(i4 *)&(b)) = (*(i4 *)&(a)))

#endif

#define			min(a, b)   ((a) <= (b) ? (a) : (b))

/**
**
**  Name: INTLIST.C - Implements the intlist datatype and related functions
**
**  Description:
**	This file provides the implementation of a datatype "intlist" which
**	provides a list of 4 byte integers.  The list can be accessed either as
**	a whole or individual elements can be addressed.  Such a datatype would
**	be useful for storing the last x observations of a value for a
**	particular key, such as the last 10 days of yen/dollar equivalents.
**
**	This datatype is implemented as a variable length type.  When creating a
**	field of this type, one must specify the number of possible elements in
**	the list.  Thus, as above, to store the last 10 yen/dollar equivalents,
**	declare
**	    create table yen_dollar(latest_observation date,
**				    last_10_values  i4_array(10));
**
**	In fact, this will create a field which is 44 bytes long:  10 * 4 bytes
**	per element plus a 4 byte count field specifying the current number of
**	valid elements.
**
**	Textually, integer lists are represented as a list of integers
**	surrounded by the curly bracket characters, making the appear like
**	mathematical sets.  Thus, the list containing 1, 2, and 3 will be
**	represented as "{ 1, 2, 3}" with spaces being ignored.
**
**
**
##  History:
##      14-Jun-1989 (fred)
##          Created.
##	30-may-1990 (fred)
##	    Altered to be simpler on machines requiring byte alignment.
##	03-jul-1990 (boba)
##	    Include iiadd.h with quotes since it's in this directory
##	    (reintegration of 13-mar-1990 change).
##	10-apr-1991 (rog)
##	    Improved error message for error 0x200133 ("Terminating '}' ...").
##	19-aug-1991 (lan)
##	    Moved the following definitions to other files:
##	    - structure definition of INTLIST (udt.h)
##	    - definition of datatype id for INTLIST (udt.h)
##	    - byte_copy (common.c)
##	    - usi4_error (now called us_error in common.c)
##      21-apr-1993 (stevet)
##          Changed <udt.h> back to "udt.h" since this file lives in DEMO.
##       6-Jul-1993 (fred)
##          Added prototyped.
##	05-nov-1993 (swm)
##	    Bug #58879
##	    The ASSIGN macros assume that the respective C types in each
##	    macro are the correct size. On axp_osf a long is 8 bytes, not
##	    4, so in the I4ASSIGN_MACRO the sizeof(long) is replaced with
##	    sizeof(int) for axp_osf. (Is there some obscure reason why the
##	    literal value 4 is not used?)
##      16-feb-1994 (stevet)
##          Added non prototype function declarations.  OS like HP/UX and 
##          SunOS does not come with ANSI C compiler.
##	22-mar-1994 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
##	16-apr-1999 (hanch04)
##	    replace MAX_I4 and MIN_I4 with I4_MAX and I4_MIN
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##      07-Jan-2003 (hanch04)  
##          Back out change for bug 108637
##          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
[@history_template@]...
**/

/*}
** Name: intlist - A list of i4's
**
** Description:
**      This structure defines an INTLIST in our world.  The first member of the
**	struct contains the number of valid elements in the list, the remaining
**	N contain the actual elements.
**
## History:
##      14-Jun-1989 (fred)
##          Created.
[@history_template@]...
*/
/*
**  This typedef is actually defined in "UDT.h".
**  Included here just for anyone's information
**
typedef struct _INTLIST
{
    i4		element_count;	/ Number of valid elements /
    i4		element_array[1];   / The elements themselves /
} INTLIST;
**
*/

/*{
** Name: usi4_compare	- Compare two intlist's
**
** Description:
**      This routine compares two lists.
**
**	The lists are compared by comparing each member of the list.  If two
**	lists are of different lengths, but equal upto the length of the shorter
**	list, then the longer list is 'greater'.
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
##      14-jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_compare(II_SCB	  *scb ,
              II_DATA_VALUE   *op1 ,
              II_DATA_VALUE   *op2 ,
              int        *result )
#else
usi4_compare(scb, op1, op2, result)
II_SCB		   *scb;
II_DATA_VALUE      *op1;
II_DATA_VALUE      *op2;
int                *result;
#endif
{
    INTLIST             *opv_1 = (INTLIST *) op1->db_data;
    INTLIST 		*opv_2 = (INTLIST *) op2->db_data;
    int			index;
    i4		end_result;
    i4		op1_count;
    i4		op2_count;
    i4		value1;
    i4		value2;
    

    if (    (op1->db_datatype != INTLIST_TYPE)
	 || (op2->db_datatype != INTLIST_TYPE)
	 || (usi4_lenchk(scb, 0, op1, 0))
	 || (usi4_lenchk(scb, 0, op2, 0))
       )
    {
	/* Then this routine has been improperly called */
	us_error(scb, 0x200100, "usi4_compare: Type/length mismatch");
	return(II_ERROR);
    }

    /*
    ** Now perform the comparison based on the rules described above.
    */

    end_result = 0;

    /*
    **	Result of search is negative when the first operand is the smaller,
    **	positive when the first operand is larger, and zero when they are equal.
    */

    I4ASSIGN_MACRO(opv_1->element_count, op1_count);
    I4ASSIGN_MACRO(opv_2->element_count, op2_count);
    for (index = 0;
	    !end_result &&
		(index < op1_count) &&
		(index < op2_count);
	    index++)
    {
	I4ASSIGN_MACRO(opv_1->element_array[index], value1);
	I4ASSIGN_MACRO(opv_2->element_array[index], value2);
	end_result = value1 - value2;
    }

    /*
    **	If the two strings are equal [so far], then their difference is
    **	determined by their length.
    */
    
    if (!end_result)
    {
	end_result = op1_count - op2_count;
    }
    *result = end_result;
    return(II_OK);
}

/*{
** Name: usi4_lenchk	- Check the length of the datatype for validity
**
** Description:
**      This routine checks that the specified length for the datatype is valid.
**	Since the maximum tuple size of 2000, the maximum length of the list is
**	(2000 - 4) / 4 or 499 (the four is the list length).  Thus, the maximum
**	user specified length is 499.  Given that this complete implementation
**	will only print lists up to length 5, this is somewhat inconsistent.
**	However, the printout routines can easily be enhanced to allow the full
**	functionality, and for demonstration purposes, the rationale behind the
**	499 limit is important to understand.
**
**	To calculate the internal size based on user size, multiply by 4 and add
**	4.
**
**	In reverse, if provided with an internl length, to be valid, the
**	difference between it and 4 must be divisible by 4.  The corresponding
**	user length is that difference divided by 4.  Note that it is true that
**	a valid length is simply divisible by 4 (i.e. if x - 4 is divisible by
**	4, then so is x...);  the difference is used for essentially teaching
**	and clarity purposes.
**
** Inputs:
**      scb                             Pointer to scb Structure.
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
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_lenchk(II_SCB	  *scb ,
            int		  user_specified ,
	    II_DATA_VALUE   *dv ,
	    II_DATA_VALUE   *result_dv )
#else
usi4_lenchk(scb, user_specified, dv, result_dv)
II_SCB		    *scb;
int		    user_specified;
II_DATA_VALUE	    *dv;
II_DATA_VALUE	    *result_dv;
#endif
{
    II_STATUS		    result = II_OK;
    i4		    length;

    if (user_specified)
    {
	if ((dv->db_length <= 0) || (dv->db_length > 499))
	    result = II_ERROR;

	length = sizeof(i4) + (sizeof(i4) * dv->db_length);
    }
    else
    {
	if (dv->db_length % 4 )
	    result = II_ERROR;

	length = (dv->db_length - 4) / 4;
    }
    if (result_dv)
    {
	result_dv->db_prec = 0;
	result_dv->db_length = length;
    }
    if (result)
    {
	us_error(scb, 0x200101,
		 "usi4_lenchk: Invalid length for INTLIST datatype");
    }
    return(result);
}

/*{
** Name: usi4_keybld	- Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is built based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	our INTLIST data, these are typically simply the values.
**
**	Note that it is not the case that the input to this routine will always
**	be of type INTLIST.  Therefore, we must call the conversion routine
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
##      02-Mar-1989 (fred)
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
usi4_keybld(II_SCB	  *scb ,
             II_KEY_BLK   *key_block )
#else
usi4_keybld(scb, key_block)
II_SCB		    *scb;
II_KEY_BLK	    *key_block;
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
	us_error(scb, 0x200102, "Usi4_keybld: Invalid key type");
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
		result = usi4_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_lokey);
	    }
	}
	if ( (result == II_OK) &&
		((key_block->adc_tykey == II_KHIGHKEY) ||
		    (key_block->adc_tykey == II_KEXACTKEY)))
	{
	    if (key_block->adc_hikey.db_data)
	    {
		result = usi4_convert(scb, &key_block->adc_kdv,
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
** Name: usi4_getempty	- Get an empty value
**
** Description:
**      This routine constructs the given empty value for this datatype.  By
**	definition, the empty value will be the an empty list.  This routine
**	merely constructs one of those in the space provided. 
**
** Inputs:
**      scb				Pointer to a session control block.
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
usi4_getempty(II_SCB       *scb ,
               II_DATA_VALUE   *empty_dv )
#else
usi4_getempty(scb, empty_dv)
II_SCB             *scb;
II_DATA_VALUE      *empty_dv;
#endif
{
    II_STATUS		result = II_OK;
    INTLIST		*list;
    i4		zero = 0;
    
    if (usi4_lenchk(scb, 0, empty_dv, 0))
    {
	result = II_ERROR;
	us_error(scb, 0x200103, "usi4_getempty: type/length mismatch");
    }
    else
    {
	list = (INTLIST *) empty_dv->db_data;
	I4ASSIGN_MACRO(zero, list->element_count);
    }
    return(result);
}

/*{
** Name: usi4_valchk	- Check for valid values
**
** Description:
**      This routine checks for valid values.  In fact, all values are valid, so
**	this routine just returns OK. 
**
** Inputs:
**      scb                             scb
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
usi4_valchk(II_SCB       *scb ,
             II_DATA_VALUE   *dv )
#else
usi4_valchk(scb, dv)
II_SCB             *scb;
II_DATA_VALUE      *dv;
#endif
{

    return(II_OK);
}

/*{
** Name: usi4_hashprep	- Prepare a datavalue for becoming a hash key.
**
** Description:
**      This routine prepares a value for becoming a hash key.  The hash key
**	data element is viewed by the access methods as simply a byte string.
**	Therefore, to create a hash key for a variable length data element, we
**	must `normalize' the value, setting all unused fields to some known and
**	consistent value.  In the case of the integer list datatype, all unused
**	fields are set to zero (by filling the byte stream with zeros).
**
**	Once the stream is initialized, the values in question are copied over.
**
** Inputs:
**      scb                             scb
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
usi4_hashprep(II_SCB       *scb ,
               II_DATA_VALUE   *dv_from ,
               II_DATA_VALUE  *dv_key )
#else
usi4_hashprep(scb, dv_from, dv_key)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_key;
#endif
{
    II_STATUS           result = II_OK;
    int			i;
    INTLIST		*list;
    INTLIST		*key_list;
    i4		list_length;
    i4		value;

    if (dv_from->db_datatype == INTLIST_TYPE)
    {
	/*
	** Guarantee normalized data for use by the system HASH algorithm.
	*/
	
	for (i = 0; i < dv_from->db_length; i++)
	    dv_key->db_data[i] = 0;

	list = (INTLIST *) dv_from->db_data;
	key_list = (INTLIST *) dv_key->db_data;

	I4ASSIGN_MACRO(list->element_count, list_length);
	I4ASSIGN_MACRO(list_length, key_list->element_count);
	for (i = 0; i < list_length; i++)
	{
	    I4ASSIGN_MACRO(list->element_array[i], key_list->element_array[i]);
	}
	
	dv_key->db_length = dv_from->db_length;
    }
    else
    {
	result = II_ERROR;
	us_error(scb, 0x200104, "usi4_hashprep: type/length mismatch");
    }
    return(result);
}

/*{
** Name: usi4_helem	- Create histogram element for data value
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
**	Given these facts and the algorithm used in intlist comparison (see
**	usi4_compare() above), the histogram for any value will be the first
**	element in the list.  If there is no first element, than the smallest
**	integer is used.  The datatype is II_INTEGER, with a length of 4.
**
** Inputs:
**      scb                              scb.
**      dv_from                         Value for which a histogram is desired.
**      dv_histogram                    Pointer to datavalue into which to place
**					the histogram.
**	    .db_datatype		    Should contain II_INTEGER
**	    .db_length			    Should contain 4
**	    .db_data			    Assumed to be a pointer to 4 bytes
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
##      14-jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_helem(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE  *dv_histogram )
#else
usi4_helem(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    char		*msg;
    INTLIST		*list;
    i4		error_code;
    i4		length;

    if (dv_histogram->db_datatype != II_INTEGER)
    {
	result = II_ERROR;
	error_code = 0x200105;
	msg = "usi4_helem: Type for histogram incorrect";
    }
    else if (dv_histogram->db_length != 4)
    {
	result = II_ERROR;
	error_code = 0x200106;
	msg = "usi4_helem: Length for histogram incorrect";
    }
    else if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x200107;
	msg = "usi4_helem: Base type for histogram incorrect";
    }
    else if (usi4_lenchk(scb, 0, dv_from, (II_DATA_VALUE *) 0) != II_OK)
    {
	result = II_ERROR;
	error_code = 0x200108;
	msg = "usi4_helem: Base length for histogram incorrect";
    }
    else
    {
	list = (INTLIST *) dv_from->db_data;
	I4ASSIGN_MACRO(list->element_count, length);
	if (length)
	{
	    I4ASSIGN_MACRO(list->element_array[0], *dv_histogram->db_data);
	}
	else
	{
	    i4	small_number = I4_MIN;

	    I4ASSIGN_MACRO(small_number, *dv_histogram->db_data);
	}	
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usi4_hmin	- Create histogram for minimum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	smallest value of a type.  The same histogram rules apply as above.
**
**	In this case, the minimum value is <smallest i4>,
**	so the minimum histogram is simply the smallest i4.
**
** Inputs:
**      scb                             scb.
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
usi4_hmin(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usi4_hmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    i4			error_code;
    i4			value = I4_MIN;

    if (dv_histogram->db_datatype != II_INTEGER)
    {
	result = II_ERROR;
	error_code = 0x200108;
    }
    else if (dv_histogram->db_length != 4)
    {
	result = II_ERROR;
	error_code = 0x200109;
    }
    else if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x20010a;
    }
    else if (usi4_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x20010b;
    }
    else
    {
	
	I4ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usi4_hmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usi4_dhmin	- Create `default' minimum histogram.
**
** Description:
**      This routine creates the minimum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' list elements are found
**	between 0 and 10,000.  Therefore, the default minimum histogram
**	will have a value of 0; the default maximum histogram (see below) will
**	have a value of 10,000. 
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
##      14-jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_dhmin(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE   *dv_histogram )
#else
usi4_dhmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    i4			error_code;
    i4			value = 0;

    if (dv_histogram->db_datatype != II_INTEGER)
    {
	result = II_ERROR;
	error_code = 0x20010c;
    }
    else if (dv_histogram->db_length != 4)
    {
	result = II_ERROR;
	error_code = 0x20010d;
    }
    else if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x20010e;
    }
    else if (usi4_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x20010f;
    }
    else
    {
	
	I4ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usi4_hmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usi4_hmax	- Create histogram for maximum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	largest value of a type.  The same histogram rules apply as above.
**
**	In this case, the maximum value is <largest i4>,
**	so the maximum histogram is simply the largest i4.
**
** Inputs:
**      scb                              scb.
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
##      14-Jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_hmax(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usi4_hmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    i4			error_code;
    i4			value = I4_MAX;

    if (dv_histogram->db_datatype != II_INTEGER)
    {
	result = II_ERROR;
	error_code = 0x200110;
    }
    else if (dv_histogram->db_length != 4)
    {
	result = II_ERROR;
	error_code = 0x200111;
    }
    else if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x200112;
    }
    else if (usi4_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200113;
    }
    else
    {
	I4ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usi4_dhmin:  Invalid input parameters");
	
    return(result);
}

/*{
** Name: usi4_dhmax	- Create `default' maximum histogram.
**
** Description:
**      This routine creates the maximum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' integers are found
**	between 0 and 10,000.
**
** Inputs:
**      scb                              scb
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
usi4_dhmax(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            II_DATA_VALUE   *dv_histogram )
#else
usi4_dhmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    i4			error_code;
    i4			value = 10000;

    if (dv_histogram->db_datatype != II_INTEGER)
    {
	result = II_ERROR;
	error_code = 0x200114;
    }
    else if (dv_histogram->db_length != 4)
    {
	result = II_ERROR;
	error_code = 0x200115;
    }
    else if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x200116;
    }
    else if (usi4_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200117;
    }
    else
    {
	I4ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usi4_dhmax: Invalid parameters");
    return(result);
}

/*{
** Name: usi4_hg_dtln	- Provide datatype & length for a histogram.
**
** Description:
**      This routine provides the datatype & length for a histogram for a given
**	datatype.  For our datatype, the histogram's datatype an length are
**	II_INTEGER (integer) and 4, respectively. 
**
** Inputs:
**      scb                              scb.
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
usi4_hg_dtln(II_SCB       *scb ,
              II_DATA_VALUE   *dv_from ,
              II_DATA_VALUE   *dv_histogram )
#else
usi4_hg_dtln(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    i4		error_code;

    if (dv_from->db_datatype != INTLIST_TYPE)
    {
	result = II_ERROR;
	error_code = 0x200118;
    }
    else if (usi4_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200119;
    }
    else
    {
	dv_histogram->db_datatype = II_INTEGER;
	dv_histogram->db_length = 4;
    }
    if (result)
	us_error(scb, error_code, "usi4_hg_dtln: Invalid parameters");
	
    return(result);
}

/*{
** Name: usi4_minmaxdv	- Provide the minimum/maximum values/lengths for a
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
**          ADE_LEN_UNKNOWN, then no minimum (or maximum) value will be built
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
**	scb				Pointer to a session control block.
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
usi4_minmaxdv(II_SCB	  *scb ,
               II_DATA_VALUE   *min_dv ,
               II_DATA_VALUE  *max_dv )
#else
usi4_minmaxdv(scb, min_dv, max_dv)
II_SCB		   *scb;
II_DATA_VALUE      *min_dv;
II_DATA_VALUE	   *max_dv;
#endif
{
    II_STATUS           result = II_OK;
    INTLIST		*list;
    i4		error_code;
    II_DATA_VALUE	local_list;

    if (min_dv)
    {
	if (min_dv->db_datatype != INTLIST_TYPE)
	{
	    result = II_ERROR;
	    error_code = 0x20011A;
	}
	else if (min_dv->db_length == II_LEN_UNKNOWN)
	{
	    min_dv->db_length = sizeof(list->element_count);
	}
	else if (usi4_lenchk(scb, 0, min_dv, 0) == II_OK)
	{
	    if (min_dv->db_data)
	    {
		i4		zero = 0;
		list = (INTLIST *) min_dv->db_data;
		I4ASSIGN_MACRO(zero, list->element_count);
	    }
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20011B;
	}
    }
    if (max_dv)
    {
	if (max_dv->db_datatype != INTLIST_TYPE)
	{
	    result = II_ERROR;
	    error_code = 0x20011C;
	}
	else if (max_dv->db_length == II_LEN_UNKNOWN)
	{
	    max_dv->db_length = (499 * 4) + 4;
	}
	else if (usi4_lenchk(scb, 0, max_dv, &local_list) == II_OK)
	{
	    if (max_dv->db_data)
	    {
		int		i;
		i4		max = I4_MAX;
		i4		s;
		
		list = (INTLIST *) max_dv->db_data;
		for (i = 0; i < local_list.db_length; i++)
		{
		    I4ASSIGN_MACRO(max, list->element_array[i]);
		}
		s = local_list.db_length;
		I4ASSIGN_MACRO(s, list->element_count);
	    }
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20011D;
	}
    }
    if (result)
	us_error(scb, error_code, "usi4_minmaxdv: Invalid parameters");
    return(result);
}

/*{
** Name: usi4_convert	- Convert to/from INTLISTs.
**
** Description:
**      This routine converts values to and/or from intlistS.
**	The following conversions are supported.
**	    integer <-> intlist of length 1.
**	    Varchar -> intlist
**		from format '{e1, e2, e3, ...}'
**		where e1 is element 1, e2...
**	    Char <-> intlist -- same format
**	and, of course,
**	    intlist <-> intlist (length independent)
**	    longtext <-> intlist
**
** Inputs:
**      scb                              scb
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
##      14-jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_convert(II_SCB         *scb ,
	     II_DATA_VALUE  *dv_in ,
	     II_DATA_VALUE  *dv_out )
#else
usi4_convert(scb, dv_in, dv_out)
II_SCB             *scb;
II_DATA_VALUE      *dv_in;
II_DATA_VALUE      *dv_out;
#endif
{
    char		*b;
    char		*b1;
    INTLIST		*list;
    int			length;
    int			i;
    II_STATUS		result = II_OK;
    char		*msg;
    char                buffer[64];
    char		*end;
    II_VLEN		*vlen;
    II_DATA_VALUE	local_dv;
    i4		error_code;
    i4		value;
    i4		llength;
    short		s;

    dv_out->db_prec = 0;
    if ((dv_in->db_datatype == INTLIST_TYPE)
	&& (dv_out->db_datatype == INTLIST_TYPE))
    {
	byte_copy((char *)dv_in->db_data,
		min(dv_in->db_length, dv_out->db_length),
		dv_out->db_data);
	if (dv_in->db_length > dv_out->db_length)
	{
	    /*
	    ** If we are potentially truncating, then determine the largest
	    ** possible internal length, and set the internal length of the
	    ** output to the min(input length, max. possible output length).
	    */
	    
	    (void) usi4_lenchk(scb, 1, dv_out, &local_dv);
	    list = (INTLIST *) dv_in->db_data;
	    I4ASSIGN_MACRO(list->element_count, llength);
	    llength = min(llength, local_dv.db_length);
	    I4ASSIGN_MACRO(llength,
			((INTLIST *) dv_out->db_data)->element_count);
	}
    }
    else if (dv_in->db_datatype == INTLIST_TYPE)
    {
	int		check_length;
	
	list = (INTLIST *) dv_in->db_data;
	
	b = dv_out->db_data;
	check_length = dv_out->db_length;
	if (dv_out->db_datatype != II_CHAR)
	{
	    check_length -= 2;
	    b += sizeof(short);
	}
	    
	if (check_length < 2)
	{
	    error_code = 2001130;
	    msg = "usi4_convert: Insufficient space for empty INTLIST output";
	    result = II_ERROR;
	}
	else
	{
	    I4ASSIGN_MACRO(list->element_count, llength);
	    for (length = 0; length < sizeof(buffer); length++)
		buffer[length] = 0;

	    *b++ = '{';
	    length = 1;
	    for (i = 0; i < llength; i++)
	    {
		I4ASSIGN_MACRO(list->element_array[i], value);
		sprintf(buffer, "%d", value);
		if ((length + 2 + strlen(buffer)) < check_length)
		{
		    if (i)
			*b++ = ',';
		    *b++ = ' ';
		    strcpy(b, buffer);
		    b += strlen(buffer);
		    length += strlen(buffer) + (i ? 2 : 1);
		}
	    }
	    if ((i == llength)
		&& (length < check_length))
	    {
		/* then we made it to the end and it fits.  End the string. */
		*b++ = '}';
		if (dv_out->db_datatype == II_CHAR)
		{
		    for (length++; length < check_length; length++)
			*b++ = ' ';
		}
		else
		{
		    short	final_length;

		    final_length = length + 1;	 /* + 1 for the '}' */
		    I2ASSIGN_MACRO(final_length, *dv_out->db_data);
		}
		
	    }
	    else
	    {
		error_code = 0x20011e;
		msg = "usi4_convert: Insufficient space for INTLIST output";
		result = II_ERROR;
	    }
	}
    }
    else if (dv_in->db_datatype == II_INTEGER)
    {
	list = (INTLIST *) dv_out->db_data;
	(void) usi4_lenchk(scb, 0, dv_out, &local_dv);

	if (local_dv.db_length < 1)
	{
	    error_code = 0x20011e;
	    msg = "usi4_convert: Insufficient space for INTLIST output";
	    result = II_ERROR;
	}
	else
	{
	    i4		value;

	    llength = 1;
	    I4ASSIGN_MACRO(llength, list->element_count);
	    switch (dv_in->db_length)
	    {
		case 1:
		    value = *dv_in->db_data;
		    break;

		case 2:
		{
		    short	svalue;
		    
		    I2ASSIGN_MACRO(*dv_in->db_data, svalue);
		    value = svalue;
		    break;
		}
		case 4:
		    I4ASSIGN_MACRO(*dv_in->db_data, value);
		    break;
	    }
	    I4ASSIGN_MACRO(value, list->element_array[0]);
	    result = II_OK;
	}
	    
    }
    else
    {
	for (;;)    /* Not a loop, just need something to 'break' out of */
	{
	    list = (INTLIST *) dv_out->db_data;
	    (void) usi4_lenchk(scb, 0, dv_out, &local_dv);
	    	    
	    switch (dv_in->db_datatype)
	    {
		case II_C:
		case II_CHAR:
		    length = dv_in->db_length;
		    b = dv_in->db_data;
		    break;

		case II_TEXT:
		case II_VARCHAR:
		case II_LONGTEXT:
		    vlen = (II_VLEN *) dv_in->db_data;
		    I2ASSIGN_MACRO(vlen->vlen_length, s);
		    length = s;
		    b = ((char *) dv_in->db_data + 2);
		    break;

		default:
		    error_code = 0x20011F;
		    msg = "usi4_convert:  Unknown input type";
		    result = II_ERROR;
	    }
	    if (result != II_OK)
		break;

	    result = II_ERROR;

	    /*
	    **	First find the end of the list.  We look forward from the
	    **	beginning until we find a '}'.  Then, if there are more
	    **	characters at the end, make sure they are all white space.
	    **	Anything else constitutes an error.
	    */

	    b1 = b;
	    while (((b1 - b) < length) && (*b1 != '}'))
		b1++;
	    if (*b1 == '}')
	    {
		end = b1++;
		while (((b1 - b) < length) && (isspace(*b1)))
		    b1++;
		if ((b1 != end) && ((b1 - b) != length) && (!isspace(*b1)))
		{
		    error_code = 0x200134;
		    msg = "Extraneous text found after list end";
		    break;
		}
	    }
	    else
	    {
		error_code = 0x200133;
		msg = "Terminating '}' not found.  Integer list format is {e1, ...}.";
		break;
	    }

	    if (*b != '{')
	    {
		error_code = 0x200120;
		msg = "Unable to convert into integer list.  Format is {e1, ...}";
		break;
	    }
	    b++;

	    /*
	    **	Would be nice to put a NULL in at the end, but this buffer may
	    **	be used again later in the query, and that would render it
	    **	invalid.  Therefore, we must always leave the input alone.
	    */

	    I4ASSIGN_MACRO(list->element_count, llength);
	    for (llength = -1;
		    local_dv.db_length > ++llength && b != end;
		/* No endloop action */)
	    {
		/* This converter ignores overflow and underflow */
		
		 value = strtol(b, &b1, 10);
		I4ASSIGN_MACRO(value, list->element_array[llength]);

		/* Now skip to next element */
		while ((*b1 != '}') && (*b1 != ',') && isspace(*b1))
		    b1++;
		if ((*b1 != '}') && (*b1 != ',') && (b1 != end))
		{
		    if (b1 != end)
		    {
			error_code = 200132;
			msg =
			 "Unable to convert to integer list -- list too long";
		    }
		    else
		    {
			error_code = 0x200120;
			msg =
"Invalid character found int integer list.  Format is {int, ...}";
		    }
		    break;
		}
		if ((*b1 != '}') && (b1 != end))
		    b1++;
		b = b1;
	    }
	    I4ASSIGN_MACRO(llength, list->element_count);

	    if ((b != end) || (llength == -1))
	    {
		error_code = 0x200120;
		msg = "Unable to convert into integer list.  Format is {e1, ...}";
		break;
	    }
	    result = II_OK;
	    break;
	}
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usi4_tmlen	- Determine 'terminal monitor' lengths
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
usi4_tmlen(II_SCB       *scb ,
            II_DATA_VALUE   *dv_from ,
            short       *def_width ,
            short       *largest_width )
#else
usi4_tmlen(scb, dv_from, def_width, largest_width)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
short              *def_width;
short              *largest_width;
#endif
{
    II_DATA_VALUE	    local_dv;

    if (dv_from->db_datatype != INTLIST_TYPE)
    {
	us_error(scb, 0x200121, "usi4_tmlen: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	(void) usi4_lenchk(scb, 0, dv_from, &local_dv);
	*largest_width = 2 /* {} */
			    + (12 /* 10 decimal digits, */
				    * local_dv.db_length);
	*def_width = 2 /* {} */
			    + (7 /* 5 decimal digits, */
				    * local_dv.db_length);
    }
    return(II_OK);
}

/*{
** Name: usi4_tmcvt	- Convert to displayable format
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
usi4_tmcvt(II_SCB       *scb ,
            II_DATA_VALUE  *from_dv ,
            II_DATA_VALUE   *to_dv ,
            int        *output_length )
#else
usi4_tmcvt(scb, from_dv, to_dv, output_length)
II_SCB             *scb;
II_DATA_VALUE	   *from_dv;
II_DATA_VALUE      *to_dv;
int                *output_length;
#endif
{
    II_DATA_VALUE	    local_dv;
    II_STATUS		    status;

    local_dv.db_data = to_dv->db_data;
    local_dv.db_length = to_dv->db_length;
    local_dv.db_prec = 0;
    local_dv.db_datatype = II_CHAR;
    
    status = usi4_convert(scb, from_dv, &local_dv);
    *output_length = local_dv.db_length;
    return(status);
}

/*{
** Name: usi4_dbtoev	- Determine which external datatype this will convert to
**
** Description:
**      This routine returns the external type to which lists will be converted. 
**      The correct type is CHAR, with a length determined by this function.
**
**	The coercion function instance implementing this coercion must be have a
**	result length calculation specified as II_RES_FIXED with a length of
**	50.  Any length could have been chosen.  For demonstration purposes,
**	this fits nicely on a screen.
**
** Inputs:
**      scb				Scb pointer.
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
usi4_dbtoev(II_SCB	  *scb ,
             II_DATA_VALUE  *db_value ,
             II_DATA_VALUE  *ev_value )
#else
usi4_dbtoev(scb, db_value, ev_value)
II_SCB		   *scb;
II_DATA_VALUE	   *db_value;
II_DATA_VALUE	   *ev_value;
#endif
{
    II_DATA_VALUE	local_dv;
    II_STATUS	    	status;

    status = II_OK;
    switch (db_value->db_datatype)
    {
    case INTLIST_TYPE:
	ev_value->db_datatype = II_CHAR;
	(void) usi4_lenchk(scb, 0, db_value, &local_dv);
	ev_value->db_length = 50;
	ev_value->db_prec = 0;
	break;

    default:
	us_error(scb, 0x200121, "usi4_dbtoev: Invalid input data");
	status = II_ERROR;
	break;
    }
    return(status);
}

/*{
** Name: usi4_concatenate	- Concatenate two lists
**
** Description:
**	This function concatenates two lists.  The result is a list which is the
**	concatenation of the two lists.  The first element must be large enough
**	to hold both lists, and the result of this calculation is based on the
**	size of the first list.
**
** Inputs:
**      scb                              scb
**      dv_1                            Datavalue representing list 1
**	dv_2				Datavalue representing list 2
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Result is placed here.
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
##      14-Jun-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_concatenate(II_SCB       *scb ,
                  II_DATA_VALUE   *dv_1 ,
                  II_DATA_VALUE   *dv_2 ,
                  II_DATA_VALUE   *dv_result )
#else
usi4_concatenate(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    INTLIST			*list_1;
    INTLIST			*list_2;
    INTLIST			*res_list;
    II_DATA_VALUE		local_dv;
    int				i;
    i4			len_1;
    i4			len_2;
    i4			len_res;

    if (    (dv_1->db_datatype != INTLIST_TYPE)
	||  (dv_2->db_datatype != INTLIST_TYPE)
	||  (dv_result->db_datatype != INTLIST_TYPE)
	||  ((dv_result->db_length != dv_1->db_length)
	    && (dv_result->db_length != dv_2->db_length))
	||  (!dv_result->db_data)
       )  
    {
	us_error(scb, 0x200122, "Usi4_concatenate: Invalid input");
	return(II_ERROR);
    }
    
    list_1 = (INTLIST *) dv_1->db_data;
    list_2 = (INTLIST *) dv_2->db_data;
    res_list = (INTLIST *) dv_result->db_data;
    
    (void) usi4_lenchk(scb, 0, dv_result, &local_dv);
    if (local_dv.db_length < (list_1->element_count + list_2->element_count))
    {
	us_error(scb, 0x200122, "Usi4_concatenate: Result too long.");
	return(II_ERROR);
    }

    I4ASSIGN_MACRO(list_1->element_count, len_1);

    for (i = 0, len_res = 0;
	i < len_1;
	i++)
    {
	I4ASSIGN_MACRO(list_1->element_array[i],
		res_list->element_array[len_res++]);
    }
    I4ASSIGN_MACRO(list_2->element_count, len_2);
    for (i = 0;
	    i < len_2;
	    i++)
    {
	I4ASSIGN_MACRO(list_2->element_array[i],
		res_list->element_array[len_res++]);
    }
    I4ASSIGN_MACRO(len_res, res_list->element_count);
    
    return(II_OK);
}

/*{
** Name: usi4_element	- Pick element out of list
**
** Description:
**      This routine picks element specified by argument 2 out of the list
**	specified as argument 1.
**	
**	The result is returned as an i4.
**
** Inputs:
**      scb                              scb
**      dv_1                            Datavalue representing the list
**	dv_2				Datavalue representing the index
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              integer result is placed here.
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
##      14-jun-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_element(II_SCB       *scb ,
              II_DATA_VALUE   *dv_1 ,
              II_DATA_VALUE   *dv_2 ,
              II_DATA_VALUE   *dv_result )
#else
usi4_element(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    INTLIST			*list;
    int				index;
    i4			answer = 0;
    i4			count;
    
    if (    (dv_1->db_datatype == INTLIST_TYPE)
	&&  (dv_2->db_datatype == II_INTEGER)
	&&  (dv_result->db_datatype == II_INTEGER)
	&&  (dv_result->db_length == 4)
	&&  (dv_result->db_data)
       )  
    {
	list = (INTLIST *) dv_1->db_data;
	switch (dv_2->db_length)
	{
	    case 1:
		index = *dv_2->db_data;
		break;
	    case 2:
	    {
		short	i2;

		I2ASSIGN_MACRO(*dv_2->db_data, i2);
		index = i2;
		break;
	    }
	    case 4:
		I4ASSIGN_MACRO(*dv_2->db_data, index);
		break;
	}

	index -= 1;
	I4ASSIGN_MACRO(list->element_count, count);
	if ((index >= 0) && (index < count))
	{
	    I4ASSIGN_MACRO(list->element_array[index], answer);
	    I4ASSIGN_MACRO(answer, *dv_result->db_data);
	    result = II_OK;
	}
	else
	{
	    us_error(scb, 0x200131, "Element specified does not exist");
	}
	

    }
    else
    {
	us_error(scb, 0x200123, "usi4_element: Invalid input");
    }
    return(result);
}

/*{
** Name: usi4_size	- Find number of elements in a list
**
** Description:
**
**	This routine extracts the element count from a list
**  
** Inputs:
**      scb                             scb
**      dv_1                            Datavalue representing the list
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              integer result is placed here.
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
##      14-jun-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_size(II_SCB       *scb ,
           II_DATA_VALUE   *dv_1 ,
           II_DATA_VALUE   *dv_result )
#else
usi4_size(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    INTLIST		*list;
    short		answer;
    i4		lanswer;

    if (    (dv_1->db_datatype == INTLIST_TYPE)
	&&  (dv_result->db_datatype == II_INTEGER)
	&&  (dv_result->db_length == 2)
	&&  (dv_result->db_data)
       )  
    {
	list = (INTLIST *) dv_1->db_data;
	I4ASSIGN_MACRO(list->element_count, lanswer);
	answer = lanswer;
	I2ASSIGN_MACRO(answer, *dv_result->db_data);
	result = II_OK;
    }
    else
    {
	us_error(scb, 0x200127, "usi4_size: Invalid input");
    }
    return(result);
}

/*{
** Name: usi4_total 	- Find the additive total of the elements in a list
**
** Description:
**
**	This routine adds the values of the elements in a list, producing a
**	total value, represented as a 4 byte integer.  Empty lists are assigned
**	the value 0.
**  
** Inputs:
**      scb                             scb
**      dv_1                            Datavalue representing the list
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              integer result is placed here.
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
##      14-jun-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_total(II_SCB       *scb ,
            II_DATA_VALUE   *dv_1 ,
            II_DATA_VALUE   *dv_result )
#else
usi4_total(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    INTLIST		*list;
    i4		count;
    i4		total = 0;
    i4		element;
    int			i;

    if (    (dv_1->db_datatype == INTLIST_TYPE)
	&&  (dv_result->db_datatype == II_INTEGER)
	&&  (dv_result->db_length == 4)
	&&  (dv_result->db_data)
       )  
    {
	list = (INTLIST *) dv_1->db_data;
	I4ASSIGN_MACRO(list->element_count, count);
	for (i = 0; i < count; i++)
	{
	    I4ASSIGN_MACRO(list->element_array[i], element);
	    total += element;
	}
	I4ASSIGN_MACRO(total, *dv_result->db_data);
	result = II_OK;
    }
    else
    {
	us_error(scb, 0x200135, "usi4_total: Invalid input");
    }
    return(result);
}

/*{
** Name: usi4_locate	- Determine if (and where) a list contains a value
**
** Description:
**
**	This routine searches thru a list for a specific value.  If the value is
**	found, then this routine returns the index at which it was found,
**	otherwise it returns 0.
**
**	The return value is a 2 byte integer.
**  
** Inputs:
**      scb                             scb
**      dv_1                            Datavalue representing the list
**	dv_2				Datavalue representing the value
**					    to be found.
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Search result is placed here.
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
##      14-jun-1989 (fred)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usi4_locate(II_SCB       *scb ,
             II_DATA_VALUE   *dv_1 ,
             II_DATA_VALUE  *dv_2 ,
             II_DATA_VALUE   *dv_result )
#else
usi4_locate(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE	   *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    INTLIST		*list;
    i4		count;
    i4		target;
    i4		element;
    short		search_result = -1;
    int			i;

    if (    (dv_1->db_datatype == INTLIST_TYPE)
	&&  (dv_2->db_datatype == II_INTEGER)
	&&  (dv_result->db_datatype == II_INTEGER)
	&&  (dv_result->db_length == 2)
	&&  (dv_result->db_data)
       )  
    {
	list = (INTLIST *) dv_1->db_data;
	I4ASSIGN_MACRO(list->element_count, count);

	switch (dv_2->db_length)
	{
	    case 1:
		target = *dv_2->db_data;
		break;
	    case 2:
	    {
		short	s;

		I2ASSIGN_MACRO(*dv_2->db_data, s);
		target = s;
		break;
	    }

	    case 4:
		I4ASSIGN_MACRO(*dv_2->db_data, target);
		break;
	}
	    
	for (i = 0; i < count; i++)
	{
	    I4ASSIGN_MACRO(list->element_array[i], element);
	    if (element == target)
	    {
		search_result = i;
		break;
	    }
	}
	search_result += 1; /* change to 1-based addressing */
	
	I2ASSIGN_MACRO(search_result, *dv_result->db_data);
	result = II_OK;
    }
    else
    {
	us_error(scb, 0x200135, "usi4_locate: Invalid input");
    }
    return(result);
}

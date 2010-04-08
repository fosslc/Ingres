
/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
**
##  History:
##
##	29-aug-1991 (jonb)
##		Don't redefine tolower if it's already defined by ctype.
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
##      14-apr-1994 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct
##	    alignment on platforms such as axp_osf where long's are 64-bit.
##	2-mar-1995 (shero03)
##	    Bug #67208
##	    Add seglen and xform routines for LONG ZCHAR.
##	22-mar-1995 (shero03)
##	    Bug #67529
##	    Verify that a ZCHAR has a valid length before converting it.
##     29-mar-1995 (wolf)
##	    Correct typo in the parameter list of uslz_xform().
##	21-jan-1999 (hanch04)
##	    replace nat and longnat with i4
##	31-aug-2000 (hanch04)
##	    cross change to main
##	    replace nat and longnat with i4
##	11-jan-2001 (somsa01)
##	    For S/390 Linux, in usz_compare() end_result must be a
##	    signed char so that zchar compares will correctly yield a
##	    negative number.
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##      07-Jan-2003 (hanch04)  
##          Back out change for bug 108637
##          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
*/
#include    <iiadd.h>
#include    <ctype.h>
#include    <stdio.h>
#include    "udt.h"

#ifdef	ALIGNMENT_REQUIRED

#define		    I2ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(short), (char *) &(b))
#define		    I4ASSIGN_MACRO(a,b)		byte_copy((char *) &(a),sizeof(i4), (char *) &(b))
#define		    BYTEASSIGN_MACRO(a,b)	byte_copy((char *) &(a), 1,(char *) &(b))

#else

#define		    I2ASSIGN_MACRO(a,b)		((*(short *)&(b)) = (*(short *)&(a)))
#define		    I4ASSIGN_MACRO(a,b)		((*(i4 *)&(b)) = (*(i4 *)&(a)))
#define		    BYTEASSIGN_MACRO(a,b)	((*(char *)&(b)) = (*(char *)&(a)))

#endif

#ifndef tolower
#define		    tolower(c)	((c) >= 'A' && (c) <= 'Z' ? (c)|0x20:(c))
#endif

#define			min(a, b)   ((a) <= (b) ? (a) : (b))
#define			MAX_ASCII	0x7f

/**
**
**  Name: ZCHAR.C - Implements the ZCHAR datatype and related functions
**
**  Description:
**	This file provides the implementation of the CH datatype, which is
**	similar to the INGRES 'varchar' datatype except that case is ignored
**	when comparing 2 strings.  For example, for the SQL query
**		select name from t where name="Bob"
**	the strings 'Bob', 'BOB', 'bob' would all be found.  So Bob can spell
**	his name backwards and get away with it.
**
**	This datatype is implemented as a variable length type.  When creating a
**	field of this type, one must specify the number of possible elements in
**	the list.  Thus to store strings of maximum length 20, declare
**	    create table t (name zchar(20), salary i4);
**
**	In fact, this will create a field which is 24 bytes long:  20 * 1 byte
**	per element plus a 4 byte count field specifying the current number of
**	valid elements.
**
{@func_list@}...
**
**
##  History:
##      Aug-1989 (T.)
##          Created.
##      21-apr-1993 (stevet)
##          Changed <udt.h> back to "udt.h" since this file lives in DEMO.
##      06-oct-1993 (stevet)
##          Fixed problem with VARCHAR to LONG ZCHAR coercions.
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
*/

/*}
** Name: zchar - A list of characters
**
** Description:
**      This structure defines a ZCHAR in our world.  The first member of the
**	struct contains the number of valid elements in the list, the remaining
**	N contain the actual elements.
**
## History:
##      Aug-1989 (T.)
##          Created.
[@history_template@]...
*/
/*
**	This structure definition actually appears in "UDT.h".  Included here
**	just FYI
**

typedef struct _ZCHAR
{
    i4	count;		    **  Number of valid elements  **
    char	element_array[1];   **  The elements themselves   **
} ZCHAR;
*/
/*
**
**	Forward static variable and function declarations
**
*/

static		II_DT_ID ch_datatype_id = ZCHAR_TYPE;


/*{
** Name: usz_compare	- Compare two ZCHARs, for use by INGRES internally
**
** Description:
**      This routine compares two ZCHAR strings.
**
**	The strings are compared by comparing each character of the string.  If
**	two strings are of different lengths, but equal up to the length of the
**	shorter	string, then the shorter string padded with blanks.
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
##      Aug-1989 (T.)
##          Created.
##	28-jul-1997 (walro03)
##	    For Tandem NonStop (ts2_us5), end_result must be a signed char so
##	    that zchar compares will correctly yield a negative number.
##	11-jan-2001 (somsa01)
##	    For S/390 Linux, end_result must be a signed char so
##	    that zchar compares will correctly yield a negative number.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_compare(II_SCB	  *scb ,
             II_DATA_VALUE   *op1 ,
             II_DATA_VALUE   *op2 ,
             int        *result )
#else
usz_compare(scb, op1, op2, result)
II_SCB		   *scb;
II_DATA_VALUE      *op1;
II_DATA_VALUE      *op2;
int                *result;
#endif
{
    ZCHAR		*opv_1 = (ZCHAR *) op1->db_data;
    ZCHAR		*opv_2 = (ZCHAR *) op2->db_data;
    int			index;
#if defined(ibm_lnx) || \
    defined(ts2_us5) || defined(__nonstopux) /* these are the same */
    signed char		end_result;
#else
    char		end_result;
#endif /* ibm_lnx ts2_us5 */
    short		blank = 32;
    i4		z1_count;
    i4		z2_count;
    char		char1;
    char		char2;

    if (    (op1->db_datatype != ZCHAR_TYPE)
	 || (op2->db_datatype != ZCHAR_TYPE)
	 || (usz_lenchk(scb, 0, op1, 0))
	 || (usz_lenchk(scb, 0, op2, 0))
       )
    {
	/* Then this routine has been improperly called */
	us_error(scb, 0x200000, "usz_compare: Type/length mismatch");
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

    I4ASSIGN_MACRO(opv_1->count, z1_count);
    I4ASSIGN_MACRO(opv_2->count, z2_count);
    for (index = 0;
	    !end_result &&
		(index < z1_count) &&
		(index < z2_count);
	    index++)
    {
	strncpy(&char1,(opv_1->element_array+index), 1);
	strncpy(&char2,(opv_2->element_array+index), 1);
/*	BYTEASSIGN_MACRO(opv_1->element_array[index], char1);
	BYTEASSIGN_MACRO(opv_2->element_array[index], char2);*/
	end_result = tolower(char1) - tolower(char2);
    }

    /*
    **	If the two strings are equal [so far], then their difference is
    **	determined by appending blanks.
    */

    if (!end_result) {
      if (z1_count > z2_count)
	for (index = z2_count;
	     !end_result && (index < z1_count);
	     index++) {
	strncpy(&char1,(opv_1->element_array+index), 1);
/*	  BYTEASSIGN_MACRO(opv_1->element_array[index], char1);*/
	  end_result = tolower(char1) - blank;
        }
      else if (z2_count > z1_count)
	for (index = z1_count;
	     !end_result && (index < z2_count);
	     index++) {
	strncpy(&char2,(opv_2->element_array+index), 1);
/*	  BYTEASSIGN_MACRO(opv_2->element_array[index], char2);*/
	  end_result = blank - tolower(char2);
        }
    }
    *result = (i4) end_result;
    return(II_OK);
}

/*{
** Name: usz_like	- Compare two ZCHARs, used by INGRES internally to
**			  resolve sql "like" predicates
**
** Description:
**      This routine compares two ZCHAR strings.
**
**	The strings are compared by comparing each character of the string and
**	taking into account the special character '_' and '%'.  If two strings
**	are of different lengths, but equal up to the length of the shorter
**	string, then the shorter string padded with blanks.
** Inputs:
**      scb                             Scb structure
**      op1                             First operand
**      op2                             Second operand, following "like"
**					These are pointers to II_DATA_VALUE
**					structures which contain the values to
**					be compared.
**      result                          Pointer to int, to contain the result of
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
##      Oct-1989 (T.)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_like(II_SCB	  *scb ,
          II_DATA_VALUE   *op1 ,
          II_DATA_VALUE   *op2 ,
          int        *result )
#else
usz_like(scb, op1, op2, result)
II_SCB		   *scb;
II_DATA_VALUE      *op1;
II_DATA_VALUE      *op2;
int                *result;
#endif
{
    ZCHAR		*opv_1 = (ZCHAR *) op1->db_data;
    ZCHAR		*opv_2 = (ZCHAR *) op2->db_data;
    int			i1, i2;
    i4		end_result;
    short		blank = 32;
    i4		op1_count;
    i4		op2_count;

    if (    (op1->db_datatype != ZCHAR_TYPE)
	 || (op2->db_datatype != ZCHAR_TYPE)
	 || (usz_lenchk(scb, 0, op1, 0))
	 || (usz_lenchk(scb, 0, op2, 0))
       )
    {
	/* Then this routine has been improperly called */
	us_error(scb, 0x200001, "usz_like: Type/length mismatch");
	return(II_ERROR);
    }

    /*
    ** Now perform the comparison based on the rules described above.
    */

    I4ASSIGN_MACRO(opv_1->count, op1_count);
    I4ASSIGN_MACRO(opv_2->count, op2_count);
    end_result = 0;

    /*
    **	Result of search is negative when the first operand is the smaller,
    **	positive when the first operand is larger, and zero when they are equal.
    */
    i1 = 0; i2 = 0;
    while ( !end_result && (i1 < op1_count) && (i2 < op2_count) ) {

	if (opv_2->element_array[i2] == '%') {
	  while (opv_2->element_array[i2+1] == '%') i2++;
	  while (opv_2->element_array[i2+1] == '_') { i1++; i2++; }
	  if ( (opv_2->element_array[i2+1] != '_') && ((i2+1) < op2_count) )
	  {
	    int i,j,k,patt_end;
	    for (i=i2+1; i<op2_count &&
			 opv_2->element_array[i] != '%'; i++) ;
	    patt_end = i - 1;
	    for (i=i1; i<op1_count; i++) {
	      for (j=i, k=i2+1;
		   (k<=patt_end) &&
		   ( !(end_result = (tolower(opv_1->element_array[j]) -
				     tolower(opv_2->element_array[k]))) ||
	  	     (opv_2->element_array[k] == '_') );
		   j++,k++) ;
	      if (k > patt_end) {
		i1 = j;
		i2 = k;
		i = op1_count;
	      }
	    }
	  }
	  else i2++;
	}
	else {
	  if ( opv_2->element_array[i2] != '_' ) end_result =
	    tolower(opv_1->element_array[i1]) -
	    tolower(opv_2->element_array[i2]);
	  i1++;
	  i2++;
	}
    }

    /*
    **	If the two strings are equal [so far], then their difference is
    **	determined by appending blanks.
    */
    if ( !end_result ) {
      if ( (i1 < op1_count) &&
	   (opv_2->element_array[op2_count-1] != '%') )
	while ( !end_result && (i1 < op1_count) ) {
	  end_result = opv_1->element_array[i1] - blank;
	  i1++;
	}
      if ( i2 < op2_count )
	while ( !end_result && (i2 < op2_count) ) {
	  if ( (opv_2->element_array[i2] != '%') &&
	       (opv_2->element_array[i2] != blank) ) end_result = -1;
	  else i2++;
	}
    }
    *result = end_result;
    return(II_OK);
}

/*{
** Name: usz_lenchk	- Check the length of the datatype for validity
**
** Description:
**      This routine checks that the specified length for the datatype is valid.
**	Since the maximum tuple size of 2000, the maximum length of the string
**	is (2000 - 4) = 1996 (the four is the string length).  Thus, the maximum
**	user specified length is 1996.
**
**	To calculate the internal size based on user size, just add 4.
**	In reverse, if provided with an internal length, just subtract 4.
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
##      Aug-1989 (T.)
##          Created.
##       7-Jul-1993 (fred)
##          Added long zchar support.
**	16-Feb-1999 (kosma01)
**	    IRIX64 6.4 sgi_us5. compiler promotes term to
**	    unsigned int, even tho it could be negative.
**	    (dv->db_length - sizeof(i4))
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_lenchk(II_SCB	  *scb ,
	   int 	          user_specified ,
	   II_DATA_VALUE  *dv ,
	   II_DATA_VALUE  *result_dv )
#else
usz_lenchk(scb, user_specified, dv, result_dv)
II_SCB		    *scb;
int		    user_specified;
II_DATA_VALUE	    *dv;
II_DATA_VALUE	    *result_dv;

#endif
{
    II_STATUS		    result = II_OK;
    i4		    length;
    short		    zero  = 0;

    if (user_specified)
    {
	if (dv->db_datatype == LZCHAR_TYPE)
	{
	    if (dv->db_length != 0)
		result = II_ERROR;

	    length = sizeof(II_PERIPHERAL);
	}
	else
	{
	    if ((dv->db_length <= 0) || (dv->db_length > 1996))
		result = II_ERROR;
	    length = sizeof(i4) + dv->db_length;
	}
    }
    else
    {
	if (dv->db_datatype == LZCHAR_TYPE)
	{
	    length = 0;
	    if (dv->db_length != sizeof(II_PERIPHERAL))
	    {
		result = II_ERROR;
	    }
	}
	else
	{
	    if ((dv->db_length - (int)sizeof(i4)) < 0 )
		result = II_ERROR;
	    length = dv->db_length - sizeof(i4);
	}
    }

    if (result_dv)
    {
	I2ASSIGN_MACRO( zero, result_dv->db_prec);
	I4ASSIGN_MACRO( length, result_dv->db_length);
    }
    if (result)
    {
	us_error(scb, 0x200002,
			"usz_lenchk: Invalid length for ZCHAR datatype");
    }
    return(result);
}

/*{
** Name: usz_keybld	- Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	the ZCHAR data, the key is simply the value itself.
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
**      scb                             scb
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
##      14-Jun-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_keybld(II_SCB	  *scb ,
            II_KEY_BLK   *key_block )
#else
usz_keybld(scb, key_block)
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

	default:	/* To catch LIKE operators */
	    key_block->adc_tykey = II_KRANGEKEY;
	    break;
    }
    if (result)	us_error(scb, 0x200003, "usz_keybld: Invalid key type");
    else
    {
	if ( (key_block->adc_tykey == II_KLOWKEY) ||
	     (key_block->adc_tykey == II_KEXACTKEY) )
	{
	    if (key_block->adc_lokey.db_data)
	    {
		result = usz_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_lokey);
	    }
	}
	if ( !result && (key_block->adc_tykey == II_KRANGEKEY) ) {
	  ZCHAR *z;
	  int 	i;
	  char	*ptr;

	  if (key_block->adc_lokey.db_data) {
	    result = usz_convert(scb, &key_block->adc_kdv,
					&key_block->adc_lokey);
	    z = (ZCHAR *) key_block->adc_lokey.db_data;
	    for (i=0, ptr=z->element_array; i<z->count; i++,ptr++)
	      if ( *ptr == '_' || *ptr == '%' ) *ptr = 0;
	  }
	  if (key_block->adc_hikey.db_data) {
	    result = usz_convert(scb, &key_block->adc_kdv,
					&key_block->adc_hikey);
	    z = (ZCHAR *) key_block->adc_hikey.db_data;
	    for (i=0, ptr=z->element_array; i<z->count; i++,ptr++)
	      if ( *ptr == '_' || *ptr == '%' ) *ptr = MAX_ASCII;
	  }
	}

	if ( !result &&
		( (key_block->adc_tykey == II_KHIGHKEY) ||
		  (key_block->adc_tykey == II_KEXACTKEY) ))
	{
	    if (key_block->adc_hikey.db_data)
	    {
		result = usz_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_hikey);
	    }
	}
    }
    return(result);
}

/*{
** Name: usz_getempty	- Get an empty value
**
** Description:
**      This routine constructs the given empty value for this datatype.  By
**	definition, the empty value will be a string of length 0.  This
**	routine merely constructs one of those in the space provided.
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
##      Aug-1989 (T.)
##          Created.
##       7-Jul-1993 (fred)
##          Added support for long zchar datatype.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_getempty(II_SCB       *scb ,
              II_DATA_VALUE   *empty_dv )
#else
usz_getempty(scb, empty_dv)
II_SCB             *scb;
II_DATA_VALUE      *empty_dv;
#endif
{
    II_STATUS		result = II_OK;
    ZCHAR		*chr;
    i4		zero = 0;
    II_PERIPHERAL       *peripheral;

    if (((empty_dv->db_datatype != ch_datatype_id)
	   &&
	 (empty_dv->db_datatype != LZCHAR_TYPE))
	||  (usz_lenchk(scb, 0, empty_dv, 0)))
    {
	result = II_ERROR;
	us_error(scb, 0x200004, "usz_getempty: type/length mismatch");
    }
    else
    {
	if (empty_dv->db_datatype == ZCHAR_TYPE)
	{
	    chr = (ZCHAR *) empty_dv->db_data;
	    I4ASSIGN_MACRO(zero, chr->count);
	}
	else
	{
	    peripheral = (II_PERIPHERAL *) empty_dv->db_data;
	    peripheral->per_length0 =
		peripheral->per_length1 = 0;
	    peripheral->per_tag = II_P_DATA;
	}

    }
    return(result);
}

/*{
** Name: usz_valchk	- Check for valid values
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
usz_valchk(II_SCB       *scb ,
            II_DATA_VALUE   *dv )
#else
usz_valchk(scb, dv)
II_SCB             *scb;
II_DATA_VALUE      *dv;
#endif
{
    return(II_OK);
}

/*{
** Name: usz_hashprep	- Prepare a datavalue for becoming a hash key.
**
** Description:
**      This routine prepares a value for becoming a hash key.  For our
**	datatype, no changes are necessary, so the value is simply copied.
**	No other work is performed.
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
##      Aug-1989 (T.)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_hashprep(II_SCB       *scb ,
              II_DATA_VALUE   *dv_from ,
              II_DATA_VALUE  *dv_key )
#else
usz_hashprep(scb, dv_from, dv_key)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_key;
#endif
{
    II_STATUS           result = II_OK;
    int			i;
    ZCHAR		*chr;
    ZCHAR		*key_chr;
    i4		chr_length;
    i4		value;

    if (dv_from->db_datatype == ch_datatype_id)
    {

	for (i = 0; i < dv_from->db_length; i++)
	    dv_key->db_data[i] = 0;

	chr = (ZCHAR *) dv_from->db_data;
	key_chr = (ZCHAR *) dv_key->db_data;

	I4ASSIGN_MACRO(chr->count, chr_length);
	I4ASSIGN_MACRO(chr_length, key_chr->count);
	for (i = 0; i < chr_length; i++)
	{
	    BYTEASSIGN_MACRO(chr->element_array[i], key_chr->element_array[i]);
	}
	dv_key->db_length = dv_from->db_length;
    }
    else
    {
	result = II_ERROR;
	us_error(scb, 0x200005, "usz_hashprep: type/length mismatch");
    }
    return(result);
}

/*{
** Name: usz_helem	- Create histogram element for data value
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
**	Given these facts and the algorithm used in ZCHAR comparison (see
**	usz_compare() above), the histogram for any value will be the first
**	character in the string.  If there is no first character, then 0
**	(NULL) is used.  The datatype is II_INTEGER, with a length of 1.
**
** Inputs:
**      scb                              scb.
**      dv_from                         Value for which a histogram is desired.
**      dv_histogram                    Pointer to datavalue into which to place
**					the histogram.
**	    .db_datatype		    Should contain II_CHAR
**	    .db_length			    Should contain 1
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
usz_helem(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE  *dv_histogram )
#else
usz_helem(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    i4		error_code;
    char		*msg;
    ZCHAR		*chr;
    i4		length;

    if (dv_histogram->db_datatype != II_CHAR)
    {
	result = II_ERROR;
	error_code = 0x200006;
	msg = "usz_helem: Type for histogram incorrect";
    }
    else if (dv_histogram->db_length != 1)
    {
	result = II_ERROR;
	error_code = 0x200007;
	msg = "usz_helem: Length for histogram incorrect";
    }
    else if (dv_from->db_datatype != ch_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200008;
	msg = "usz_helem: Base type for histogram incorrect";
    }
    else if (usz_lenchk(scb, 0, dv_from, (II_DATA_VALUE *) 0) != II_OK)
    {
	result = II_ERROR;
	error_code = 0x200009;
	msg = "usz_helem: Base length for histogram incorrect";
    }
    else
    {
	char z;
	chr = (ZCHAR *) dv_from->db_data;
	I4ASSIGN_MACRO(chr->count, length);
	if (length)
	{
	    z = tolower(chr->element_array[0]);
	    BYTEASSIGN_MACRO(z, *dv_histogram->db_data);
	}
	else
	{
	    z = 0;
	    BYTEASSIGN_MACRO(z, *dv_histogram->db_data);
	}
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usz_hmin	- Create histogram for minimum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	smallest value of a type.  The same histogram rules apply as above.
**
**	In this case, the minimum value is 0 (NULL), so the minimum
**	histogram is simply NULL.
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
##      Aug-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_hmin(II_SCB       *scb ,
          II_DATA_VALUE   *dv_from ,
          II_DATA_VALUE   *dv_histogram )
#else
usz_hmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    char			value = 0;
    i4			error_code;

    if (dv_histogram->db_datatype != II_CHAR)
    {
	result = II_ERROR;
	error_code = 0x20000a;
    }
    else if (dv_histogram->db_length != 1)
    {
	result = II_ERROR;
	error_code = 0x20000b;
    }
    else if (dv_from->db_datatype != ch_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x20000c;
    }
    else if (usz_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x20000d;
    }
    else
    {
	BYTEASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usz_hmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usz_dhmin	- Create `default' minimum histogram.
**
** Description:
**      This routine creates the minimum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' character strings
**	are printable characters, ie. ascii values between 32 and 126.
**	Therefore, the default minimum histogram will have a value of ' ', or
**	32 ascii; the default maximum histogram (see below) will have a value
**	value of '~' or 126 ascii.
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
##      Aug-1989 (T.)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_dhmin(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usz_dhmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    char			value = 0;
    i4			error_code;

    if (dv_histogram->db_datatype != II_CHAR)
    {
	result = II_ERROR;
	error_code = 0x20000e;
    }
    else if (dv_histogram->db_length != 1)
    {
	result = II_ERROR;
	error_code = 0x20000f;
    }
    else if (dv_from->db_datatype != ch_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200010;
    }
    else if (usz_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200011;
    }
    else
    {
	BYTEASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usz_dhmin: Invalid input parameters");
    return(result);
}

/*{
** Name: usz_hmax	- Create histogram for maximum value.
**
** Description:
**      This routine is used by the optimizer to obtain the histogram for the
**	largest value of a type.  The same histogram rules apply as above.
**
**	In this case, the maximum value is ascii 127, so the maximum
**	histogram is simply the largest ascii value
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
usz_hmax(II_SCB       *scb ,
          II_DATA_VALUE   *dv_from ,
          II_DATA_VALUE   *dv_histogram )
#else
usz_hmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    char			value = 127;
    i4			error_code;

    if (dv_histogram->db_datatype != II_CHAR)
    {
	result = II_ERROR;
	error_code = 0x200012;
    }
    else if (dv_histogram->db_length != 1)
    {
	result = II_ERROR;
	error_code = 0x200013;
    }
    else if (dv_from->db_datatype != ZCHAR_TYPE)
    {
	result = II_ERROR;
	error_code = 0x200014;
    }
    else if (usz_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200015;
    }
    else
    {
	BYTEASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usz_hmax:  Invalid input parameters");

    return(result);
}

/*{
** Name: usz_dhmax	- Create `default' maximum histogram.
**
** Description:
**      This routine creates the maximum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' character strings
**	are made up of printable characters between 32 and 126 ascii.
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
##      Aug-1989 (T.)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_dhmax(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           II_DATA_VALUE   *dv_histogram )
#else
usz_dhmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    char			value = 126;
    i4			error_code;

    if (dv_histogram->db_datatype != II_CHAR)
    {
	result = II_ERROR;
	error_code = 0x200016;
    }
    else if (dv_histogram->db_length != 1)
    {
	result = II_ERROR;
	error_code = 0x200017;
    }
    else if (dv_from->db_datatype != ch_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200018;
    }
    else if (usz_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x200019;
    }
    else
    {
	BYTEASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "usz_dhmax: Invalid parameters");
    return(result);
}

/*{
** Name: usz_hg_dtln	- Provide datatype & length for a histogram.
**
** Description:
**      This routine provides the datatype & length for a histogram for a given
**	datatype.  For our datatype, the histogram's datatype and length are
**	II_CHAR (character) and 1, respectively.
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
##      Aug-1989 (T.)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_hg_dtln(II_SCB       *scb ,
             II_DATA_VALUE   *dv_from ,
             II_DATA_VALUE   *dv_histogram )
#else
usz_hg_dtln(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    i4		error_code;

    if (dv_from->db_datatype != ZCHAR_TYPE)
    {
	result = II_ERROR;
	error_code = 0x20001a;
    }
    else if (usz_lenchk(scb, 0, dv_from, 0))
    {
	result = II_ERROR;
	error_code = 0x20001b;
    }
    else
    {
	dv_histogram->db_datatype = II_CHAR;
	dv_histogram->db_length = 1;
    }
    if (result)
	us_error(scb, error_code, "usz_hg_dtln: Invalid parameters");

    return(result);
}

/*{
** Name: usz_minmaxdv	- Provide the minimum/maximum values/lengths for a
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
usz_minmaxdv(II_SCB	  *scb ,
              II_DATA_VALUE   *min_dv ,
              II_DATA_VALUE  *max_dv )
#else
usz_minmaxdv(scb, min_dv, max_dv)
II_SCB		   *scb;
II_DATA_VALUE      *min_dv;
II_DATA_VALUE	   *max_dv;
#endif
{
    II_STATUS           result = II_OK;
    ZCHAR		*chr;
    i4		error_code;
    II_DATA_VALUE	local_chr;

    if (min_dv)
    {
	if (min_dv->db_datatype != ch_datatype_id)
	{
	    result = II_ERROR;
	    error_code = 0x20001c;
	}
	else if (min_dv->db_length == II_LEN_UNKNOWN)
	{
	    min_dv->db_length = sizeof(chr->count);
	}
	else if (usz_lenchk(scb, 0, min_dv, 0) == II_OK)
	{
	    if (min_dv->db_data)
	    {
		i4 zero = 0;
		chr = (ZCHAR *) min_dv->db_data;
		I4ASSIGN_MACRO(zero, chr->count);
	    }
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20001d;
	}
    }
    if (max_dv)
    {
	if (max_dv->db_datatype != ZCHAR_TYPE)
	{
	    result = II_ERROR;
	    error_code = 0x20001e;
	}
	else if (max_dv->db_length == II_LEN_UNKNOWN)
	{
	    max_dv->db_length = 2000;
	}
	else if (usz_lenchk(scb, 0, max_dv, &local_chr) == II_OK)
	{
	    if (max_dv->db_data)
	    {
		int		i;
		char		max = MAX_ASCII;
		i4		s;

		chr = (ZCHAR *) max_dv->db_data;
		for (i = 0; i < local_chr.db_length; i++)
		{
		    BYTEASSIGN_MACRO(max, chr->element_array[i]);
		}
		s = local_chr.db_length;
		I4ASSIGN_MACRO(s, chr->count);
	    }
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20001f;
	}
    }
    if (result)
	us_error(scb, error_code, "usz_minmaxdv: Invalid parameters");
    return(result);
}

/*{
** Name: usz_convert	- Convert to/from ZCHARs.
**
** Description:
**      This routine converts values to and/or from ZCHARs.
**	The following conversions are supported.
**	    Varchar -> ZCHAR
**		from format '. . .'
**	    Char <-> ZCHAR -- same format
**	and, of course,
**	    intlist <-> intlist (length independent)
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
##      Aug-1989 (fred)
##          Created.
##	22-mar-1995 (shero03)
##	    Bug #67529
##	    Verify that a ZCHAR has a valid length before converting it.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_convert(II_SCB       *scb ,
             II_DATA_VALUE   *dv_in ,
             II_DATA_VALUE   *dv_out )
#else
usz_convert(scb, dv_in, dv_out)
II_SCB             *scb;
II_DATA_VALUE      *dv_in;
II_DATA_VALUE      *dv_out;
#endif
{
    char		*b;
    ZCHAR		*chr;
    i4		value;
    int			i;
    II_STATUS		result = II_OK;
    i4		error_code;
    char		*msg;
    II_VLEN		*vlen;
    II_DATA_VALUE	local_dv;
    i4		llength;
    short		s;

    dv_out->db_prec = 0;
    if ( dv_in->db_datatype == ZCHAR_TYPE )
    {

	/*
	**  Bug 67529 - verify that the internal length
	**  doesn't exceed the external length.
	**  For instance, if improper data is copied into the UDT
	**  it can have an invalid length e.g. 20202020.
	*/
	chr = (ZCHAR *) dv_in->db_data;
	I4ASSIGN_MACRO(chr->count, llength);
        if (llength > dv_in->db_length - 4)
        {
	    us_error(scb, 0x200002,
			"usz_convert: Invalid length for ZCHAR datatype");
	    return (II_ERROR);
        }

      switch (dv_out->db_datatype) {

	case ZCHAR_TYPE:
	  byte_copy((char *) dv_in->db_data,
		min(dv_in->db_length, dv_out->db_length),
		dv_out->db_data);
	  if (dv_in->db_length > dv_out->db_length) {
	    /*
	    ** If we are potentially truncating, then determine the largest
	    ** possible internal length, and set the internal length of the
	    ** output to the min(input length, max. possible output length).
	    */

	    (void) usz_lenchk(scb, 1, dv_out, &local_dv);
	    llength = min(llength, local_dv.db_length);
	    I4ASSIGN_MACRO(llength,
	    	((ZCHAR *) dv_out->db_data)->count);
	  }
	  break;

	case II_TEXT:
	case II_VARCHAR:
	  (void) usz_lenchk(scb, 1, dv_out, &local_dv);
	  b = dv_out->db_data;
	  b += sizeof(short);
/*	  byte_copy (chr->element_array, min(llength, local_dv.db_length), b);*/
	  byte_copy (chr->element_array, llength, b);
	  s = llength;
	  I2ASSIGN_MACRO(s, *dv_out->db_data);
/*	  dv_out->db_length = min(llength, local_dv.db_length);*/
	  break;

	case II_CHAR:
        case II_C:
	  (void) usz_lenchk(scb, 1, dv_out, &local_dv);
/*	  byte_copy (chr->element_array,min(llength, local_dv.db_length),
		dv_out->db_data);*/
	  byte_copy (chr->element_array,llength,
		dv_out->db_data);
/*	  dv_out->db_length = min(llength, local_dv.db_length);*/
	  break;

	default:
	  error_code = 0x200020;
	  msg = "usz_convert: Unkown output type";
	  result = II_ERROR;
      }
    }

    else {
	chr = (ZCHAR *) dv_out->db_data;
	(void) usz_lenchk(scb, 0, dv_out, &local_dv);

	switch (dv_in->db_datatype)
	{
	  case II_C:
	  case II_CHAR:
	    llength = min(dv_in->db_length,local_dv.db_length);
/*	    llength = dv_in->db_length;*/
	    I4ASSIGN_MACRO(llength, chr->count);
	    strncpy(chr->element_array, dv_in->db_data, chr->count);
	    result = II_OK;
	    break;

	   case II_TEXT:
	   case II_VARCHAR:
	   case II_LONGTEXT:
	     vlen = (II_VLEN *) dv_in->db_data;
	     I2ASSIGN_MACRO(vlen->vlen_length, s);
	     llength = min(s, local_dv.db_length);
/*	     llength = s;*/
	     I4ASSIGN_MACRO(llength, chr->count);
	     strncpy(chr->element_array, vlen->vlen_array, chr->count);
	     result = II_OK;
	     break;

	    default:
	      error_code = 0x200021;
	      msg = "usz_convert:  Unknown input type";
	      result = II_ERROR;
	}
    }
    if (result)	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: usz_tmlen	- Determine 'terminal monitor' lengths
**
** Description:
**      This routine returns the default and worst case lengths for a datatype
**	if it were to be printed as text (which is the way things are
**	displayed in the terminal monitor).  Although in this release,
**	user-defined datatypes are not returned to the terminal monitor, this
**	routine (and its partner, usz_tmcvt()) are needed for various trace
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
usz_tmlen(II_SCB       *scb ,
           II_DATA_VALUE   *dv_from ,
           short       *def_width ,
           short       *largest_width )
#else
usz_tmlen(scb, dv_from, def_width, largest_width)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
short              *def_width;
short              *largest_width;
#endif
{
    II_DATA_VALUE	    local_dv;

    if (dv_from->db_datatype != ZCHAR_TYPE)
    {
	us_error(scb, 0x200022, "usz_tmlen: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	*def_width = *largest_width = 30;
    }
    return(II_OK);
}

/*{
** Name: usz_tmcvt	- Convert to displayable format
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
usz_tmcvt(II_SCB       *scb ,
           II_DATA_VALUE  *from_dv ,
           II_DATA_VALUE   *to_dv ,
           int        *output_length )
#else
usz_tmcvt(scb, from_dv, to_dv, output_length)
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
    local_dv.db_datatype = II_VARCHAR;

    status = usz_convert(scb, from_dv, &local_dv);
    *output_length = local_dv.db_length;
    return(status);
}

/*{
** Name: usz_dbtoev	- Determine which external datatype this will convert to
**
** Description:
**      This routine returns the external type to which lists will be converted.
**      The correct type is II_VARCHAR, with a length determined by this function.
**
**	The coercion function instance implementing this coercion must be have a
**	result length calculation specified as II_RES_KNOWN.
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
##      Aug-1989 (T.)
##          Created.
##       7-Jul-1993 (fred)
##          Added long zchar support.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usz_dbtoev(II_SCB	  *scb ,
            II_DATA_VALUE  *db_value ,
            II_DATA_VALUE  *ev_value )
#else
usz_dbtoev(scb, db_value, ev_value)
II_SCB		   *scb;
II_DATA_VALUE	   *db_value;
II_DATA_VALUE	   *ev_value;
#endif
{

    if (db_value->db_datatype == ch_datatype_id)
    {
	ev_value->db_datatype = II_VARCHAR;
	ev_value->db_length = db_value->db_length;
	ev_value->db_prec = 0;
    }
    else if (db_value->db_datatype == LZCHAR_TYPE)
    {
	ev_value->db_datatype = II_LVCH;
	ev_value->db_length = 0;
	ev_value->db_prec = 0;
    }
    else
    {
	us_error(scb, 0x200023, "usz_dbtoev: Invalid input data");
	return(II_ERROR);
    }

    return(II_OK);
}

/*{
** Name: usz_concatenate	- Concatenate two lists
**
** Description:
**	This function concatenates two ZCHARs.  The result is a stringe which is the
**	concatenation of the two strings.
**
** Inputs:
**      scb                             scb
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
##      Sept-1989 (T.)
##          Created
[@history_template@]...
*/
/*II_STATUS
usz_concatenate(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
{
    II_STATUS			result = II_ERROR;
    ZCHAR			*str_1;
    ZCHAR			*str_2;
    ZCHAR			*res_str;
    II_DATA_VALUE		local_dv;
    int				i;
    i4			len_1;
    i4			len_2;
    i4			len_res;

    if (    ((dv_1->db_datatype != ZCHAR_TYPE)
	    &&  (dv_2->db_datatype != ZCHAR_TYPE))
	||  (dv_result->db_datatype != ZCHAR_TYPE)
	||  ((dv_result->db_length != dv_1->db_length)
	    && (dv_result->db_length != dv_2->db_length))
	||  (!dv_result->db_data)
       )
    {
	us_error(scb, 0x200024, "usz_concatenate: Invalid input");
	return(II_ERROR);
    }

    str_1 = (ZCHAR *) dv_1->db_data;
    list_2 = (ZCHAR *) dv_2->db_data;
    res_list = (ZCHAR *) dv_result->db_data;

    (void) usz_lenchk(scb, 0, dv_result, &local_dv);
    if (local_dv.db_length < (list_1->count + list_2->count))
    {
	us_error(scb, 0x200025, "Usz_concatenate: Result too long.");
	return(II_ERROR);
    }

    I4ASSIGN_MACRO(list_1->count, len_1);
    for (i = 0, len_res = 0;
	i < len_1;
	i++)
    {
	I4ASSIGN_MACRO(list_1->element_array[i],
		res_list->element_array[len_res++]);
    }
    I4ASSIGN_MACRO(list_2->count, len_2);
    for (i = 0;
	    i < len_2;
	    i++)
    {
	I4ASSIGN_MACRO(list_2->element_array[i],
		res_list->element_array[len_res++]);
    }
    I4ASSIGN_MACRO(len_res, res_list->count);

    return(II_OK);
}*/

/*{
** Name: usz_size	- Find number of characters in a string
**
** Description:
**
**	This routine extracts the count from a ZCHAR
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
usz_size(II_SCB       *scb ,
          II_DATA_VALUE   *dv_1 ,
          II_DATA_VALUE   *dv_result )
#else
usz_size(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    II_DATA_VALUE	local_dv;

    if (    (dv_1->db_datatype == ZCHAR_TYPE)
	&&  (dv_result->db_datatype == II_INTEGER)
	&&  (dv_result->db_length == 2)
	&&  (dv_result->db_data)
	&&  (usz_lenchk(scb, 0, dv_1, &local_dv) != II_ERROR)
       )
    {
	I4ASSIGN_MACRO(local_dv.db_length, *dv_result->db_data);
	result = II_OK;
    }
    else us_error(scb, 0x200026, "usz_size: Invalid input");
    return (result);
}

/*{
** Name:	uslz_concat_slave - Perform segment by segment concat
**
** Description:
**	This routine simply provides the slave filter function for the
**      concatenatio of long zchar objects.  For each segment with
**      which it is called, it copies the segment into the new segment.
**      It then sets the next expected action (shd_exp_action) field to
**      ADW_CONTINUE, since the concat function will always traverse
**      the entire long zchar.
**
**      This routine exists separately from the normal concat()
**      function because its calling convention is slightly different.
**
** Inputs:
**      scb                              Session control block.
**      dv_in                            Ptr to II_DATA_VALUE describing
**                                       input
**      dv_out                           Ptr to II_DATA_VALUE describing
**                                       output area.
**      work                             Ptr to II_LO_WKSP workspace.
**
** Outputs:
**      scb errors                       Filled as appropriate.
**      dv_out->db_length                Set correctly.
**      work->adw_shared.shd_exp_action  Always set to ADW_CONTINUE.
**
** Returns:
**	DB_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      None.
**
## History:
##       7-Jul-1993 (fred)
##          Created.
*/
static II_STATUS
#ifdef __STDC__
uslz_concat_slave(II_PTR	scb,
		  II_DATA_VALUE *dv_in,
		  II_DATA_VALUE *dv_out,
		  II_LO_WKSP    *work)
#else
uslz_concat_slave(scb, dv_in, dv_out, work)
II_PTR	      scb;
II_DATA_VALUE *dv_in;
II_DATA_VALUE *dv_out;
II_LO_WKSP    *work;
#endif
{
    i4                count;

    count = ((ZCHAR *) dv_in->db_data)->count;

    byte_copy(dv_in->db_data,
	      dv_in->db_length,
	      dv_out->db_data);
    work->adw_shared.shd_exp_action = ADW_CONTINUE;
    work->adw_shared.shd_l1_check += count;
    return(II_OK);
}

/*{
** Name:	uslz_concat - Perform Concat(Long zchar, long zchar)
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long zchar field to perform the "concat"
**      function on said objects.  It will set up the workspace with
**      initial parameter information,  then call the general filter
**      function which will arrange to get the the segments for the
**      first, then second objects.
**
**      This routine makes use of the static uslz_concat_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**      This routine uses the workspace to pass information to the slave
**      routine.  This routine will initialize the
**      adw_shared.shd_l{0,1}_check areas to zero;  it expects that the
**      slave routine will provide the correct values here for the
**      resultant large object.
**
**      If either of the input objects has zero length, the resultant
**      object is simply copied from the other (possibly non-zero) object.
**
**
** Inputs:
**      scb                              Session control block.
**      dv_in1                           Ptr to II_DATA_VALUE describing
**                                       the first input coupon
**      dv_in2                           Ptr to II_DATA_VALUE describing
**                                       the second input coupon
**      dv_work                          Ptr to II_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an II_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to II_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb errors                       Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	II_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      None.
**
## History:
##       7-Jul-1993 (fred)
##          Created.
*/
II_STATUS
#ifdef __STDC__
uslz_concat(II_SCB         *scb,
	    II_DATA_VALUE *dv_in,
	    II_DATA_VALUE *dv_in2,
	    II_DATA_VALUE *dv_work,
	    II_DATA_VALUE *dv_out)
#else
uslz_concat(scb, dv_in, dv_in2, dv_work, dv_out)
II_SCB        *scb;
II_DATA_VALUE *dv_in;
II_DATA_VALUE *dv_in2;
II_DATA_VALUE *dv_work;
II_DATA_VALUE *dv_out;
#endif
{
    II_STATUS              status = II_OK;
    II_PERIPHERAL          *inp1_cpn = (II_PERIPHERAL *) dv_in->db_data;
    II_PERIPHERAL          *inp2_cpn = (II_PERIPHERAL *) dv_in2->db_data;
    II_PERIPHERAL          *out_cpn = (II_PERIPHERAL *) dv_out->db_data;
    II_LO_WKSP             *work = (II_LO_WKSP *) dv_work->db_data;
    int                    one_worth_doing = 0;
    int                    two_worth_doing = 0;

    for (;;)
    {
	if (	(inp1_cpn->per_length0 != 0)
	    ||  (inp1_cpn->per_length1 != 0))
	{
	    one_worth_doing = 1;
	}
	if (    (inp2_cpn->per_length0 != 0)
	    ||  (inp2_cpn->per_length1 != 0))
	{
	    two_worth_doing = 1;
	}

	if (one_worth_doing && two_worth_doing)
	{
	    status = (*usc_setup_workspace)(scb, dv_in, dv_work);
	    if (status)
		break;

	    work->adw_shared.shd_l1_check = 0;
	    work->adw_shared.shd_l0_check = 0;

	    status = (*usc_lo_filter)(scb,
				      dv_in,
				      dv_out,
				      uslz_concat_slave,
				      work,
				      II_C_BEGIN_MASK);
	    if (status)
		break;

	    status = (*usc_lo_filter)(scb,
				      dv_in2,
				      dv_out,
				      uslz_concat_slave,
				      work,
				      II_C_END_MASK);

	    out_cpn->per_length0 = work->adw_shared.shd_l0_check;
	    out_cpn->per_length1 = work->adw_shared.shd_l1_check;
	}
	else if (one_worth_doing)
	{
	    byte_copy((char *) inp1_cpn,
		      sizeof(*inp1_cpn),
		      (char *) out_cpn);
	}
	else
	{
	    byte_copy((char *) inp2_cpn,
		      sizeof(*inp2_cpn),
		      (char *) out_cpn);
	}
	break;
    }
    return(status);
}

/*{
** Name: uslz_move	- Move to/from long zchar elements
**
** Description:
**      This routine moves data to or from long zchar elements.  These
**	elements are different from `normal' string handling, because long
**	zchar is a peripheral datatype.  This requires different handling.
**
**	For most string datatypes, there can be only a single operation, since
**	any other string datatype will fit into memory, and long zchar will
**	not.  However, if this is a long varchar to long varchar move, then this
**	is a more complicated operation.
**
** Inputs:
**      scb                             Session control block
**      dv_in                           Ptr to II_DATA_VALUE for original string
**      dv_out                          Ptr to II_DATA_VALUE for output
**
** Outputs:
**      dv_out                          Filled appropriately
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##       7-Jul-1993 (fred)
##          Created.
##       6-Oct-1993 (stevet)
##          Added missing workseg.db_length value.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uslz_move(II_SCB             *scb,
	  II_DATA_VALUE	     *dv_in,
	  II_DATA_VALUE	     *dv_out)
#else
uslz_move(scb, dv_in, dv_out)
II_SCB             *scb;
II_DATA_VALUE	   *dv_in;
II_DATA_VALUE      *dv_out;
#endif
{
    II_STATUS           status;
    II_POP_CB		pop_cb;
    II_DATA_VALUE	segment_dv;
    II_DATA_VALUE	under_dv;
    II_DATA_VALUE	cpn_dv;
    II_PERIPHERAL	*p;
    int			length;
    II_DATA_VALUE       workseg;
    char		segspace[2048];

    for (;;)
    {
	if (dv_in->db_datatype == LZCHAR_TYPE)
	{
	    cpn_dv.db_datatype = dv_in->db_datatype;
	}
	else
	{
	    cpn_dv.db_datatype = dv_out->db_datatype;
	}

	pop_cb.pop_length = sizeof(pop_cb);
	pop_cb.pop_type = II_POP_TYPE;
	pop_cb.pop_ascii_id = 0;
	pop_cb.pop_temporary = II_POP_SHORT_TEMP;
	pop_cb.pop_underdv = &under_dv;
	    under_dv.db_prec = 0;
	    under_dv.db_data = 0;
	    under_dv.db_datatype = ZCHAR_TYPE;
	pop_cb.pop_segment = &segment_dv;
	byte_copy((char *) &under_dv,
		  sizeof(under_dv),
		  (char *) &segment_dv);
	pop_cb.pop_coupon = &cpn_dv;
	    cpn_dv.db_length = sizeof(II_PERIPHERAL);
	    cpn_dv.db_prec = 0;
	    cpn_dv.db_data = (char *) 0;

	workseg.db_data = segspace;

	/* Determine the size we can use */
	status = (*usc_lo_handler)(II_INFORMATION, &pop_cb);
	if (status)
	{
	    us_error(scb, pop_cb.pop_error.err_code, (char *) 0);
	    break;
	}

	if (dv_in->db_datatype != LZCHAR_TYPE)
	{
	    p = (II_PERIPHERAL *) dv_out->db_data;
	    p->per_tag = II_P_COUPON;
	    cpn_dv.db_data = dv_out->db_data;
	    if (under_dv.db_datatype != dv_in->db_datatype)
	    {
		/* Then we have another coercion to perform.  do that first */

		workseg.db_datatype = under_dv.db_datatype;
		workseg.db_length = under_dv.db_length;
		workseg.db_prec = 0;

		status = usz_convert(scb, dv_in, &workseg);
		if (status)
		    break;
	    }
	    else
	    {
	        byte_copy((char *) dv_in->db_data,
			  dv_in->db_length,
			  (char *) workseg.db_data);
		workseg.db_length = dv_in->db_length;
		workseg.db_prec = 0;
		workseg.db_datatype = dv_in->db_datatype;
	    }
	    dv_in = &workseg;

	    pop_cb.pop_continuation = II_C_BEGIN_MASK;
	    segment_dv.db_data = dv_in->db_data;
	    p->per_length0 = 0;
	    p->per_length1 =
		    ((ZCHAR *) segment_dv.db_data)->count;

	    do
	    {
		length = ((ZCHAR *) segment_dv.db_data)->count;

		if ((length + 4) > under_dv.db_length)
		{
		    segment_dv.db_length = under_dv.db_length;
		    ((ZCHAR *) segment_dv.db_data)->count =
							under_dv.db_length - 4;
		}
		else
		{
		    segment_dv.db_length = length + 4;
		    pop_cb.pop_continuation |= II_C_END_MASK;
		}
		status = (*usc_lo_handler)(II_PUT, &pop_cb);
		pop_cb.pop_continuation &= ~II_C_BEGIN_MASK;
		if (status)
		{
		    break;
		}
		if (pop_cb.pop_continuation & II_C_END_MASK)
		    break;

		segment_dv.db_data = ((char *) segment_dv.db_data +
						    segment_dv.db_length - 4);
		((ZCHAR *) segment_dv.db_data)->count =
			    length - segment_dv.db_length + 4;

	    } while ((status < II_ERROR) &&
			((pop_cb.pop_continuation & II_C_END_MASK) == 0));
	}
	else if (dv_out->db_datatype != LZCHAR_TYPE)
	{
	    II_DATA_VALUE       *res_dv;
	    II_DATA_VALUE       result;
	    char                buffer[2048];


	    p = (II_PERIPHERAL *) dv_in->db_data;
	    cpn_dv.db_data = (char *) p;

	    switch (dv_out->db_datatype)
	    {
	    case II_CHAR:
	    case II_C:
	    case II_VARCHAR:
	    case II_TEXT:
		res_dv = &result;
		res_dv->db_datatype = ZCHAR_TYPE;
		res_dv->db_prec = 0;
		res_dv->db_data = buffer;
		res_dv->db_length = dv_out->db_length + 4;
		break;

	    case ZCHAR_TYPE:
		res_dv = dv_out;
		break;

	    default:
		return(II_ERROR);
	    }
	    ((ZCHAR *) res_dv->db_data)->count = 0;
	    pop_cb.pop_segment = &workseg;
	    pop_cb.pop_continuation = II_C_BEGIN_MASK;
	    pop_cb.pop_temporary = II_POP_SHORT_TEMP;
	    pop_cb.pop_segment->db_datatype = under_dv.db_datatype;
	    pop_cb.pop_segment->db_length = under_dv.db_length;
	    pop_cb.pop_segment->db_prec = under_dv.db_prec;

	    do
	    {
		status = (*usc_lo_handler)(II_GET, &pop_cb);
		if (status)
		{
		    if ((status >= II_ERROR) ||
			    (pop_cb.pop_error.err_code != II_E_NOMORE))
		    {
			us_error(scb, pop_cb.pop_error.err_code, (char *)0);
			break;
		    }
		}
		pop_cb.pop_continuation &= ~II_C_BEGIN_MASK;
/*		status = adu_4strconcat(adf_scb,
						res_dv,
						pop_cb.pop_segment,
						res_dv);
*/
	    } while ((status <= II_ERROR) &&
			(pop_cb.pop_error.err_code != II_E_NOMORE));
	    if (pop_cb.pop_error.err_code == II_E_NOMORE)
		status = II_OK;
	    if ((status <= II_ERROR) && (res_dv != dv_out))
	    {
		status = usz_convert(scb, res_dv, dv_out);
	    }
	}
	else
	{
	    pop_cb.pop_segment = dv_in;
	    pop_cb.pop_coupon = dv_out;
	    p = (II_PERIPHERAL *) dv_out->db_data;
	    p->per_tag = II_P_COUPON;
	    p->per_length0 = ((II_PERIPHERAL *) dv_in->db_data)->per_length0;
	    p->per_length1 = ((II_PERIPHERAL *) dv_in->db_data)->per_length1;

	    status = (*usc_lo_handler)(II_COPY, &pop_cb);
	    if (status)
	    {
		us_error(scb, pop_cb.pop_error.err_code, (char *) 0);
		break;
	    }
	}
	break;
    }
    return(status);
}

/*{
** Name:	uslz_lmove - Move long type to/from long zchar
**
** Description:
**      This routine sets up the general purpose large object filter to
**      have it filter a long zchar field to convert between long
**      types and the long zchar type.  It will set up the workspace with
**      initial parameter information,  then call the general filter
**      function which will arrange to get the the segments for the
**      objects.
**
**      This routine makes use of the static uslz_0lmove_slave()
**      routine also defined in this file.  It is thru this slave
**      routine that the work actually gets done.
**
**      This routine uses the workspace to pass information to the slave
**      routine.  This routine will initialize the
**      adw_shared.shd_l{0,1}_check areas to zero;  it expects that the
**      slave routine will provide the correct values here for the
**      resultant large object.
**
**      If either of the input objects has zero length, the resultant
**      object is simply copied from the other (possibly non-zero) object.
**
**
** Inputs:
**      scb                              Session control block.
**      dv_in1                           Ptr to II_DATA_VALUE describing
**                                       the first input coupon
**      dv_work                          Ptr to II_DATA_VALUE which
**                                       points to an area large enough
**                                       to hold an II_LO_WKSP plus 2
**                                       segments (for input & output).
**      dv_out                           Ptr to II_DATA_VALUE describing
**                                       output area in which the output
**                                       coupon will be created.
**
** Outputs:
**      scb errors                       Filled as appropriate.
**      *dv_out->db_data                 Defines the output coupon
**
** Returns:
**	II_STATUS
**
** Exceptions:
**      None.
**
** Side Effects:
**      None.
**
## History:
##       8-Jul-1993 (fred)
##          Created.
*/
II_STATUS
#ifdef __STDC__
uslz_lmove(II_SCB         *scb,
	   II_DATA_VALUE *dv_in,
	   II_DATA_VALUE *dv_work,
	   II_DATA_VALUE *dv_out)
#else
uslz_lmove(scb, dv_in, dv_work, dv_out)
II_SCB        *scb;
II_DATA_VALUE *dv_in;
II_DATA_VALUE *dv_work;
II_DATA_VALUE *dv_out;
#endif
{
    II_STATUS           status;
    II_POP_CB		in_pop_cb;
    II_POP_CB		out_pop_cb;
    II_DATA_VALUE	in_segment_dv;
    II_DATA_VALUE       out_segment_dv;
    II_DATA_VALUE	in_under_dv;
    II_DATA_VALUE       out_under_dv;
    II_DATA_VALUE	cpn_dv;
    II_PERIPHERAL	*p;
    int			length;
    char		*inspace;
    char                *outspace;
    int                 done;

    inspace = (char *) dv_work->db_data;
    outspace = inspace + 2048;

    for (;;)
    {
	if (	(((II_PERIPHERAL *) dv_in)->per_length0 != 0)
	    ||  (((II_PERIPHERAL *) dv_in)->per_length1 != 0))
	{
	    in_pop_cb.pop_length = sizeof(in_pop_cb);
	    in_pop_cb.pop_type = II_POP_TYPE;
	    in_pop_cb.pop_ascii_id = 0;
	    in_pop_cb.pop_temporary = II_POP_SHORT_TEMP;
	    in_pop_cb.pop_underdv = &in_under_dv;
	    in_pop_cb.pop_continuation = II_C_BEGIN_MASK;
	    in_under_dv.db_prec = 0;
	    in_under_dv.db_data = 0;
	    in_pop_cb.pop_coupon = dv_in;
	    in_pop_cb.pop_segment = &in_segment_dv;

	    /* Determine the size we can use */
	    status = (*usc_lo_handler)(II_INFORMATION, &in_pop_cb);
	    if (status)
	    {
		us_error(scb, in_pop_cb.pop_error.err_code,
			 "Internal Error");
		break;
	    }

	    out_pop_cb.pop_length = sizeof(out_pop_cb);
	    out_pop_cb.pop_type = II_POP_TYPE;
	    out_pop_cb.pop_ascii_id = 0;
	    out_pop_cb.pop_temporary = II_POP_SHORT_TEMP;
	    out_pop_cb.pop_underdv = &out_under_dv;
	    out_pop_cb.pop_continuation = II_C_BEGIN_MASK;
	    out_under_dv.db_prec = 0;
	    out_under_dv.db_data = 0;
	    out_pop_cb.pop_coupon = dv_out;
	    out_pop_cb.pop_segment = &out_segment_dv;
	    ((II_PERIPHERAL *) dv_out->db_data)->per_tag = II_P_COUPON;

	    /* Determine the size we can use */
	    status = (*usc_lo_handler)(II_INFORMATION, &out_pop_cb);
	    if (status)
	    {
		us_error(scb, out_pop_cb.pop_error.err_code,
			 "Internal Error");
		break;
	    }

	    if (dv_in->db_datatype == LZCHAR_TYPE)
	    {
		in_under_dv.db_datatype = ZCHAR_TYPE;
		out_under_dv.db_datatype = II_VARCHAR;
	    }
	    else
	    {
		out_under_dv.db_datatype = ZCHAR_TYPE;
		in_under_dv.db_datatype = II_VARCHAR;
	    }

	    byte_copy((char *) &in_under_dv,
		      sizeof(in_under_dv),
		      (char *) &in_segment_dv);
	    in_segment_dv.db_data = inspace;

	    byte_copy((char *) &out_under_dv,
		      sizeof(out_under_dv),
		      (char *) &out_segment_dv);
	    out_segment_dv.db_data = outspace;

	    done = 0;
	    do
	    {
		status = (*usc_lo_handler)(II_GET, &in_pop_cb);
		if (status)
		{
		    if (in_pop_cb.pop_error.err_code == II_E_NOMORE)
		    {
			done = 1;
		    }
		    else
		    {
			us_error(scb, in_pop_cb.pop_error.err_code,
				 "Internal Error");
			break;
		    }
		}
		if (done)
		{
		    out_pop_cb.pop_continuation |= II_C_END_MASK;
		}

		status = usz_convert((II_SCB *) scb,
				     &in_segment_dv,
				     &out_segment_dv);
		if (status)
		    break;

		status = (*usc_lo_handler)(II_PUT, &out_pop_cb);

		in_pop_cb.pop_continuation &= ~II_C_BEGIN_MASK;
		out_pop_cb.pop_continuation &= ~II_C_BEGIN_MASK;

	    } while (!done && status == II_OK);

	    ((II_PERIPHERAL *) dv_out->db_data)->per_length0
		= ((II_PERIPHERAL *) dv_in->db_data)->per_length0;
	    ((II_PERIPHERAL *) dv_out->db_data)->per_length1
		= ((II_PERIPHERAL *) dv_in->db_data)->per_length1;
	}
	else
	{
	    byte_copy((char *) dv_in->db_data,
		      dv_in->db_length,
		      (char *) dv_out->db_data);
	}
	break;
    }
    return(status);
}


/*
**  Name:    uslz_seglen - determine the length of each segment
**                         for LONG ZCHAR.
**
**  Description:
**       The length of each segment for LONG ZCHAR is the size of
**       the ZCHAR segment.
**
**  Inputs:
**       scb                scb
**       l_dt               datatype ID
**       dv_out             ptr to II_DATA_VALUE for result type and length
**
**  Outputs:
**       *dv_out            filled in as follows
**          .db_datatype    type of underlying data type
**          .db_length      length of underlying data type
**          .db_prec        precision of underlying data type
**  Returns:
**     II_STATUS
**
##  History:
##     2-mar-1995 (shero03)
##        Created.
*/
II_STATUS
#ifdef __STDC__
uslz_seglen(II_SCB        *scb,
	    II_DT_ID      l_dt,
	    II_DATA_VALUE *dv_out)
#else
uslz_seglen(scb, l_dt, dv_out)
II_SCB        *scb;
II_DT_ID      l_dt;
II_DATA_VALUE *dv_out;
#endif

{
    II_STATUS result = II_OK;

    if (l_dt != LZCHAR_TYPE)
    {
	us_error(scb, 0x200027, "uslz_seglen: Invalid input type");
	return(II_ERROR);
    }
    else
    {
	dv_out->db_datatype = ZCHAR_TYPE;
	/* Input length is OK */
	dv_out->db_prec = 0;
    }

    return (result);
}


/*
** Name:  uslz_xform() - Transform long data types.
**
** Description:
**      This routine transforms a LONG ZCHAR into its component segements
**      for copy into/from.
**
** Inputs:
**      scb                             Session control block.
**      in_space                        Ptr to workspace for peripheral
**                                      operations.
**                                      This is to be preserved intact
**                                      (subject to its usage) across
**                                      calls to this routine.
**          adw_adc                     Private area -- untouched but
**                                      preserved by caller
**          adw_shared                  Area used to communicate
**                                      between this routine and its
**                                      callers
**              shd_type                Datatype being xformed.
**              shd_exp_action          Instructions for caller and
**                                      callee.
**                                          ADW_START -- first call.
**                                          ADW_NEXT_SEGMENT &
**                                          ADW_GET_DATA -- last call
**                                              got more data from
**                                              segment.
**                                          ADW_FLUSH_SEGMENT -- Output
**                                              area full, flush it.
**                                      This value is set on output to
**                                      tell the caller what to do
**                                      (why returned).
**              shd_i_{area,length,used} Pointer to, length of, and
**                                      amount used of the input area
**                                      to be consumed by the routine.
**              shd_o_{area,length,used} Pointer to, length of, and
**                                      amount used of the output area
**                                      to be built ed by the routine.
**              shd_{inp,out}_segmented  Whether the input or output
**                                      objects are in a style that
**                                      is constructed of segments of
**                                      the underlying type.  A value
**                                      of 0 means a byte stream;
**                                      non-zero indicates segmented
**                                      types.  In general, this is
**                                      most of the information this
**                                      routine needs to glean from...
**              shd_{in,out}_tag        The actual type of large
**                                      object.  Irrelevant (outside
**                                      of above) in most cases, but
**                                      provided nonetheless.
** Outputs:
**      scb->adf_error....              Usual error stuff.
**      workspace                       Filled as appropriate.
**          adw_adc                     Private area -- caller must
**                                      preserve.
**          adw_shared
**             shd_exp_action           Filled as above...
**             shd_i_used               Modified as used by routine
**             shd_o_used               Modified as used by routine
**             shd_l{0,1}_chk           Filled with the length of the
**                                      object being xform'd.  This is
**                                      checked (where possible) with
**                                      the input type for validation
**                                      of the object.
**
**
** Returns:
**	II_STATUS                       Error will be reported by calling
**                                      routine.
**
** Exceptions:
**      None.
**
** Side Effects:
**      None.
**
## History:
##     2-mar-1995 (shero03)
##        Created.
##     29-mar-1995 (wolf)
##        Correct typo in parameter list (in the #else clause).
*/
II_STATUS
#ifdef __STDC__
uslz_xform(II_SCB        *scb,
	   char          *ws)
#else
uslz_xform(scb, ws)
II_SCB        *scb;
char          *ws;
#endif


{
    II_LO_WKSP	 *wrk = (II_LO_WKSP *)ws;
    II_STATUS    status = II_OK;

# define		START		0
# define		GET_SEG_LENGTH	1
# define		GET_SEGMENT	2
# define	        END_SEGMENT	3
# define		COMPLETE	4

    int		    old_state = 1;
    short	    count;
    short           *countloc;
    int		    done;
    int		    data_available;
    int		    moved;
    int		    really_moved;
    int		    offset;
    int		    space_left;
    char	    *p;
    char            *chp;

    if (wrk->adw_shared.shd_exp_action == ADW_START)
    {
	wrk->adw_adc.adc_state = START;
    }
    else if (wrk->adw_adc.adc_state < 0)
    {
	/*
	**  Recover from a negative state -- this indicates that the previous
	**  call returned with an incomplete status.  Therefore, the caller
	**  will have provided a new input buffer -- of which we have used no
	**  information
	*/

	wrk->adw_adc.adc_state = -wrk->adw_adc.adc_state;
    }
    p = ((char *) wrk->adw_shared.shd_i_area) +
	    	    	    	wrk->adw_shared.shd_i_used;

    if (wrk->adw_shared.shd_exp_action == ADW_FLUSH_SEGMENT)
    {
	/* If the last thing we did was flush a segment, then we need */
	/* to store the new location in which to store the segment */
	/* count.  We keep the the beginning of this area's offset in */
	/* the adc_longs[ADW_L_COUNTLOC] entry.  Its position is the */
	/* number of bytes used at the moment. */

	wrk->adw_adc.adc_longs[ADW_L_COUNTLOC] =
					 wrk->adw_shared.shd_o_used;
    }
    countloc = (short *) &((char *)wrk->adw_shared.shd_o_area)[
			     wrk->adw_adc.adc_longs[ADW_L_COUNTLOC]];

    wrk->adw_shared.shd_exp_action = ADW_CONTINUE;

    for (done = 0; !done; )
    {
	switch (wrk->adw_adc.adc_state)
	{
	case START:

	    wrk->adw_adc.adc_need = 0;
	    wrk->adw_shared.shd_l0_check = 0;
	    wrk->adw_shared.shd_l1_check = 0;
	    count = 0;
	    wrk->adw_adc.adc_longs[ADW_L_COUNTLOC] =
					 wrk->adw_shared.shd_o_used;
	    countloc = (short *) &((char *)wrk->adw_shared.shd_o_area)[
				  wrk->adw_adc.adc_longs[ADW_L_COUNTLOC]];
	    wrk->adw_adc.adc_chars[ADW_C_HALF_DBL] = '\0';
	    wrk->adw_adc.adc_state = GET_SEG_LENGTH;
	    old_state = 0;
	    break;

	case GET_SEG_LENGTH:
	    if (wrk->adw_shared.shd_inp_segmented)
	    {
		/* Then this data comes with a length up front. */
		/* If coupon then this is generating data */
		if (!old_state++)
		{
		    wrk->adw_adc.adc_need = sizeof(short);
		}
		data_available =
		    wrk->adw_shared.shd_i_length -
			wrk->adw_shared.shd_i_used;

		if (data_available > 0)
		{
		    moved = min(wrk->adw_adc.adc_need, data_available);
		    offset = sizeof(short) - wrk->adw_adc.adc_need;
	            byte_copy(p, moved,
		              &wrk->adw_adc.adc_memory[offset]);
		    data_available -= moved;
		    wrk->adw_shared.shd_i_used += moved;
		    p += moved;
		    wrk->adw_adc.adc_need -= moved;
		    if (wrk->adw_adc.adc_need == 0)
		    {
			I2ASSIGN_MACRO(*wrk->adw_adc.adc_memory,
			                wrk->adw_adc.adc_shorts[ADW_S_COUNT]);
			wrk->adw_adc.adc_state = GET_SEGMENT;
			old_state = 0;
		    }
		    else
		    {
			wrk->adw_shared.shd_exp_action = ADW_GET_DATA;
			done = 1;
		    }
		}
		else
		{
		    wrk->adw_shared.shd_exp_action = ADW_GET_DATA;
		    done = 1;
		}

	    }
	    else
	    {
		/*
		**	If this is not a coupon which has segments embedded as
		**	varchar style strings (i.e. with embedded lengths),
		**	then we treat each call as a segment unto itself.
		*/

		wrk->adw_adc.adc_shorts[ADW_S_COUNT] =
		    wrk->adw_shared.shd_i_length;
		wrk->adw_adc.adc_state = GET_SEGMENT;
		old_state = 0;
	    }
	    break;

	case GET_SEGMENT:

	    if (!old_state++)
	    {
		wrk->adw_adc.adc_need =
		    wrk->adw_adc.adc_shorts[ADW_S_COUNT];
	    }

	    /*
	    ** If this is the first time thru here and we are
	    ** beginning a new segment, then we need to initialize the
	    ** segment, and account for the space used by it.
	    */

	    if (wrk->adw_shared.shd_o_used ==
		        wrk->adw_adc.adc_longs[ADW_L_COUNTLOC])
	    {
		count = 0;
		if (wrk->adw_shared.shd_out_segmented)
		{
		    wrk->adw_shared.shd_o_used += sizeof(short);
		    I2ASSIGN_MACRO(count, *countloc);
		}
	    }

	    data_available =
		wrk->adw_shared.shd_i_length -
		    wrk->adw_shared.shd_i_used;

	    if (wrk->adw_shared.shd_out_segmented)
	    {
	        I2ASSIGN_MACRO(*countloc, count);
	    }
	    chp = ((char *) &((char *)wrk->adw_shared.shd_o_area)[
					  wrk->adw_shared.shd_o_used]);

	    space_left = wrk->adw_shared.shd_o_length
		    - wrk->adw_shared.shd_o_used;
	    if (data_available <= 0)
	    {
		wrk->adw_shared.shd_exp_action = ADW_GET_DATA;
		done = 1;
		break;
	    }
	    else if (wrk->adw_adc.adc_chars[ADW_C_HALF_DBL])
	    {
		if (space_left < 2)
		{
		    /* Then dbl byte won't fit -- get out now */
		    wrk->adw_shared.shd_exp_action = ADW_FLUSH_SEGMENT;
		    done = 1;
		    break;
		}
	        chp[count] = wrk->adw_adc.adc_chars[ADW_C_HALF_DBL];
	        wrk->adw_adc.adc_chars[ADW_C_HALF_DBL] = '\0';

	        count += 1;
	        chp[count] = *p++;
	        data_available -= 1;
	        wrk->adw_shared.shd_i_used += 1;
	        wrk->adw_shared.shd_o_used += 2;
	        space_left -= 2;
	        if (data_available <= 0)
	        {
	            wrk->adw_shared.shd_exp_action = ADW_GET_DATA;
		    done = 1;
		    break;
	        }
	    }


	    moved = min(wrk->adw_adc.adc_need, data_available);

	    if (space_left > 0)
	    {
		moved = min(moved, space_left);
	        byte_copy(p, moved, chp);
		really_moved = moved;
		wrk->adw_adc.adc_need -= really_moved;
		wrk->adw_shared.shd_i_used += really_moved;
		wrk->adw_shared.shd_o_used += really_moved;

		if (wrk->adw_shared.shd_out_segmented)
		{
		    I2ASSIGN_MACRO(count, *countloc);
		}
		p += really_moved;
		if (moved != really_moved)
		{
		    wrk->adw_adc.adc_chars[ADW_C_HALF_DBL] = *p++;
		    wrk->adw_adc.adc_need -= 1;
		}

		wrk->adw_shared.shd_l1_check += really_moved;

		if (wrk->adw_adc.adc_need == 0)
		{
		    wrk->adw_adc.adc_state = END_SEGMENT;
		    old_state = 0;
		}
		/*
		** Else, go around loop and figure out what we're out
		** of.
		*/
	    }
	    else
	    {
		wrk->adw_shared.shd_exp_action =
                                         ADW_FLUSH_SEGMENT;
		done = 1;
	    }
	    break;

	case END_SEGMENT:
	    if (!old_state++)
	    {
		/*
		** If we are at the end of a segment, then we must
		** return to our caller so that it can prepare the
		** next segment for our consumption.  This
		** preparation may involve some manipulation of the
		** input buffer.
		**
		** The first time thru here (signified by oldstate ==
		** 0), we will tell our caller that we need a new
		** segment (exp_action = ADW_NEXT_SEGMENT).  When we
		** return here, (oldstate != 0), we move on to the
		** beginning of segment processing.
		*/

		wrk->adw_shared.shd_exp_action =
		    ADW_NEXT_SEGMENT;
		done = 1;
	    }
	    else
	    {
		wrk->adw_adc.adc_state = GET_SEG_LENGTH;
		old_state = 0;
	    }
	    break;


	case COMPLETE:

	    if (wrk->adw_adc.adc_chars[ADW_C_HALF_DBL])
	    {
	        us_error(scb, 0x200028, "uslz_xform: Bad Blob");
	        status = II_ERROR;
	        break;
	    }

	    if (wrk->adw_shared.shd_o_used)
	    {
	        wrk->adw_shared.shd_exp_action = ADW_FLUSH_SEGMENT;
	        done = 1;
	    }
	    break;


	default:
	    us_error(scb, 0x200029, "uslz_xform: Internal_Error");
	    status = II_ERROR;
	    break;
	}

	if ((wrk->adw_shared.shd_exp_action != ADW_CONTINUE)
	    || (status != II_OK))
	    break;
    }
    if (    (status == II_OK)
	&&  (wrk->adw_shared.shd_exp_action != ADW_CONTINUE))
    {
	/*
	**  Here, we have exhausted the users input buffer, and must
	**  request more.  Since the next call will be made with a new buffer,
	**  we will mark that nothing has been used here, and correct the used
	**  count on the way in.  We mark this state by making the state
	**  negative.  We do not reset the used field here, since that would
	**  not provide the appropriate information to the caller.
	*/

	wrk->adw_adc.adc_state = -wrk->adw_adc.adc_state;
	status = II_INFO;
    }
    return(status);
}


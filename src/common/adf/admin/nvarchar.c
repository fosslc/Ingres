
/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
**
##  History:
##     08-mar-1996 (tsale01)
##	    Create varucs2 data type from various existing udt's.
##     20-feb-2001 (gupsh01)
##	    Modified varucs2 to nvarchar.	
*/
#include    <ctype.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <sys/types.h>
#include    <string.h>
#include    "iiadd.h"
#include    "udt.h"
#include    "depend.h"

#ifdef	ALIGNMENT_REQUIRED

#define	BYTEASSIGN_MACRO(a, b)	byte_copy((char *)&(a), sizeof(char), (char *)&(b))
#define	I2ASSIGN_MACRO(a, b)	byte_copy((char *)&(a), sizeof(short), (char *)&(b))
#define	I4ASSIGN_MACRO(a, b)	byte_copy((char *)&(a), sizeof(unsigned int), (char *)&(b))

#else

#define	BYTEASSIGN_MACRO(a, b)	((*(char *)&(b)) = (*(char *)&(a)))
#define	I2ASSIGN_MACRO(a, b)	((*(short *)&(b)) = (*(short *)&(a)))
#define	I4ASSIGN_MACRO(a, b)	((*(unsigned int *)&(b)) = (*(unsigned int *)&(a)))

#endif

#define	min(a, b)	((a) <= (b) ? (a) : (b))
#define	MIN_UCS2	0
#define	MAX_UCS2	0xffff

/**
**
**	Name: NVARCHAR.C - Implements the VARUCS2 datatype and related functions
**
**	Description:
**	This file provides the implementation of the VARUCS2 datatype, which is
**	similar to the INGRES 'varchar' datatype except that the basic datatype
**	is 16 bit Unicode code point.
**
**	This datatype is implemented as a variable length type.  When creating a
**	field of this type, one must specify the number of possible elements in
**	the list.  Thus to store strings of maximum 20 Unicode characters, declare
**	    create table t (name nvarchar(20), salary i4);
**
**	In fact, this will create a field which is 42 bytes long:  20 * 2 byte
**	per element plus a 2 byte count field specifying the current number of
**	valid elements.
**
{@func_list@}...
**
**
##  History:
##      08-mar-1996 (tsale01)
##		Created.
##	24-jan-2001 (fanra01, gupsh01)
##		Modified function vucs2_convert in order to correctly interpret
##		the output length.
##      24-nov-2009 (joea)
##          Change sizeof(int) to sizeof(char) in function_instances array.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
*/

/*}
** Name: nvarchar.c - A list of Unicode code points
**
** Description:
**      This structure defines a VARUCS2 in our world.  The first member of the
**	struct contains the number of valid elements in the list, the remaining
**	N contain the actual elements.
**
## History:
##      08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
/*
**	This structure definition actually appears in "UDT.h".  Included here
**	just FYI
**
*/

/*
**typedef struct _VARUCS2
**{
**	short	count;		    **  Number of valid elements  **
**	UCS2	element_array[1];   **  The elements themselves   **
**} VARUCS2;
*/

/* define a dummy VARUCS2 so we can use various sizeof operations properly */
static VARUCS2 dummy;
#define	CNTSZ	(sizeof(dummy.count))
#define	ELMSZ	(sizeof(dummy.element_array[0]))
#define	MAXVUCS2LEN		((2000 - CNTSZ) / ELMSZ)

/*
**
**	Forward static variable and function declarations
**
*/

/*{
** Name: vucs2_compare	- Compare two VARUCS2s, for use by INGRES internally
**
** Description:
**	This routine compares two VARUCS2 strings.
**
**	The strings are compared by comparing each unicode code point of the
**	string. If two strings are of different lengths, but equal up to the
**	length of the shorter string, then the shorter string padded with
**	blanks.
**
** Inputs:
**	scb		Scb structure
**	op1		First operand
**	op2		Second operand
**			These are pointers to II_DATA_VALUE
**			structures which contain the values to
**			be compared.
**	result		Pointer to nat to contain the result of
**			the operation
**
** Outputs:
**	*result		Filled with the result of the operation.
**			This routine will make *result
**				< 0 if op1 <  op2
**				> 0 if op1 >  op2
**				= 0 if op1 == op2
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_compare(
	II_SCB		*scb,
	II_DATA_VALUE	*op1,
	II_DATA_VALUE	*op2,
	int		*result)
#else
vucs2_compare(scb, op1, op2, result)
II_SCB		   *scb;
II_DATA_VALUE      *op1;
II_DATA_VALUE      *op2;
int                *result;
#endif
{
	VARUCS2		*opv_1 = (VARUCS2 *) op1->db_data;
	VARUCS2		*opv_2 = (VARUCS2 *) op2->db_data;
	int		index;
	int		end_result;
	short		blank = 32;
	short		u1_count;
	short		u2_count;
	UCS2		uchar1;
	UCS2		uchar2;

	if ((op1->db_datatype != VARUCS2_TYPE)
	 || (op2->db_datatype != VARUCS2_TYPE)
	 || (vucs2_lenchk(scb, 0, op1, 0) != II_OK)
	 || (vucs2_lenchk(scb, 0, op2, 0) != II_OK)
	   )
	{
		/* Then this routine has been improperly called */
		us_error(scb, 0x200000, "vucs2_compare: Type/length mismatch");
		return(II_ERROR);
	}

	/*
	** Now perform the comparison based on the rules described above.
	*/

	end_result = 0;

	/*
	** Result of search is negative when the first operand is the smaller,
	** positive when the first operand is larger, and zero when they are
	** equal.
	*/

	I2ASSIGN_MACRO(opv_1->count, u1_count);
	I2ASSIGN_MACRO(opv_2->count, u2_count);

	for (index = 0;
	     end_result == 0 && index < u1_count && index < u2_count;
	     index++)
	{
		I2ASSIGN_MACRO(opv_1->element_array[index], uchar1);
		I2ASSIGN_MACRO(opv_2->element_array[index], uchar2);
		if (uchar1 == uchar2)
			;	/* EMPTY */
		else if (uchar1 < uchar2)
			end_result = -1;
		else if (uchar1 > uchar2)
			end_result = 1;
	}

	/*
	** If the two strings are equal (so far), then their difference is
	** determined by their length.
	*/

	if (end_result == 0)
	{
		end_result = u1_count - u2_count;
	}

	*result = end_result;
	return(II_OK);
}

/*{
** Name: vucs2_like	- Compare two VARUCS2s, used by INGRES internally to
**			  resolve sql "like" predicates
**
** Description:
**	This routine compares two VARUCS2 strings.
**
**	The strings are compared by comparing each character of the string and
**	taking into account the special character '_' and '%'.  If two strings
**	are of different lengths, but equal up to the length of the shorter
**	string, then the shorter string padded with blanks.
** Inputs:
**	scb		Scb structure
**	op1		First operand
**	op2		Second operand, following "like"
**			These are pointers to II_DATA_VALUE
**			structures which contain the values to
**			be compared.
**	result		Pointer to int, to contain the result of
**			the operation
**
** Outputs:
**      *result		Filled with the result of the operation.
**			This routine will make *result
**			    < 0 if op1 < op2
**			    > 0 if op1 > op2
**			  or  0 if op1 == op2
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_like(II_SCB	*scb ,
          II_DATA_VALUE	*op1 ,
          II_DATA_VALUE	*op2 ,
          int		*result )
#else
vucs2_like(scb, op1, op2, result)
II_SCB		   *scb;
II_DATA_VALUE      *op1;
II_DATA_VALUE      *op2;
int                *result;
#endif
{
	VARUCS2		*opv_1 = (VARUCS2 *)op1->db_data;
	VARUCS2		*opv_2 = (VARUCS2 *)op2->db_data;
	short		i1, i2;
	int		end_result;
	UCS2		blank = 32;
	short		op1_count;
	short		op2_count;

	if (op1->db_datatype != VARUCS2_TYPE ||
	    op2->db_datatype != VARUCS2_TYPE ||
	    vucs2_lenchk(scb, 0, op1, 0) != II_OK ||
	    vucs2_lenchk(scb, 0, op2, 0) != II_OK
	   )
	{
		/* Then this routine has been improperly called */
		us_error(scb, 0x200001, "vucs2_like: Type/length mismatch");
		return(II_ERROR);
	}

	/*
	** Now perform the comparison based on the rules described above.
	*/

	I2ASSIGN_MACRO(opv_1->count, op1_count);
	I2ASSIGN_MACRO(opv_2->count, op2_count);
	end_result = 0;

	/*
	** Result of search is negative when the first operand is the smaller,
	** positive when the first operand is larger, and zero when they
	** are equal.
	*/
	i1 = 0; i2 = 0;
	while (end_result == 0 && i1 < op1_count && i2 < op2_count) {
		if (opv_2->element_array[i2] == '%')
		{
			while (opv_2->element_array[i2+1] == '%')	
				i2++;
			while (opv_2->element_array[i2+1] == '_')
				{ i1++; i2++; }

			if (opv_2->element_array[i2+1] != '_' &&
			    (i2+1) < op2_count)
			{
				int i, j, k, patt_end;
				for (i = i2 + 1;
				     i < op2_count &&
					opv_2->element_array[i] != '%';
				     i++)
					;	/* EMPTY */
				patt_end = i - 1;
				for (i=i1; i<op1_count; i++) {
					for (j = i, k = i2+1;
					     k <= patt_end &&
						(end_result = opv_1->element_array[j] - opv_2->element_array[k]) == 0 ||
						opv_2->element_array[k] == '_';
					     j++,k++)
						;	/* EMPTY */
			  		if (k > patt_end) {
						i1 = j;
						i2 = k;
						i = op1_count;
					}
				}
			} else
				i2++;
		}
		else {
			if ( opv_2->element_array[i2] != '_' ) {
				if (opv_1->element_array[i1] == opv_2->element_array[i2])
					;	/* EMPTY */
				else if (opv_1->element_array[i1] < opv_2->element_array[i2])
					end_result = -1;
				else if (opv_1->element_array[i1] > opv_2->element_array[i2])
					end_result = 1;
			}
			i1++;
			i2++;
		}
	}

	/*
	**	If the two strings are equal [so far], then their difference is
	**	determined by appending blanks.
	*/
	if (end_result == 0) {
		if (i1 < op1_count &&
		    opv_2->element_array[op2_count-1] != '%')
			while ( !end_result && (i1 < op1_count) ) {
	  			end_result = opv_1->element_array[i1] - blank;
	  			i1++;
			}
		if (i2 < op2_count)
			while (end_result == 0 && i2 < op2_count) {
				if (opv_2->element_array[i2] != '%' &&
				    opv_2->element_array[i2] != blank)
					end_result = -1;
		  		else
					i2++;
			}
	}
	*result = end_result;
	return(II_OK);
}

/*{
** Name: vucs2_lenchk	- Check the length of the datatype for validity
**
** Description:
**	This routine checks that the specified length for the datatype is valid.
**	Since the maximum tuple size of 2000, the maximum length of the string
**	is (2000 - 2) = 1998 (the two is the string length).  Thus, the maximum
**	user specified length is 1998/2 == 999.
**
**	To calculate the internal size based on user size, multiply by 2
**	then add 2. To calculate user size given internal length, subtract 2
**	then divide by 2;
**	note that the internal length must be divisible by 2 in this case.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_lenchk(II_SCB	  *scb ,
	   int 	          user_specified ,
	   II_DATA_VALUE  *dv ,
	   II_DATA_VALUE  *result_dv )
#else
vucs2_lenchk(scb, user_specified, dv, result_dv)
II_SCB		*scb;
int		user_specified;
II_DATA_VALUE	*dv;
II_DATA_VALUE	*result_dv;

#endif
{
	II_STATUS	result = II_OK;
	unsigned int		length;
	short		zero  = 0;

	if (user_specified)
	{
		length = dv->db_length;
		if (length <= 0) {
			length = 0;
			result = II_ERROR;
		} else if (length > MAXVUCS2LEN) {
			length = MAXVUCS2LEN;
			result = II_ERROR;
		}
		length = CNTSZ + length * ELMSZ;
	}
	else
	{
		if ((dv->db_length - CNTSZ) % ELMSZ != 0)
			result = II_ERROR;
		if ((dv->db_length - CNTSZ) < 0 )
			result = II_ERROR;

		length = (dv->db_length - CNTSZ) / ELMSZ;
	}

	if (result_dv != (II_DATA_VALUE *)NULL)
	{
		I2ASSIGN_MACRO(zero, result_dv->db_prec);
		I4ASSIGN_MACRO(length, result_dv->db_length);
	}

	if (result != II_OK)
	{
		us_error(scb, 0x200002,
			"vucs2_lenchk: Invalid length for VARUCS2 datatype");
	}

	return(result);
}

/*{
** Name: vucs2_keybld	- Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	the VARUCS2 data, the key is simply the value itself.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_keybld(II_SCB	  *scb ,
            II_KEY_BLK   *key_block )
#else
vucs2_keybld(scb, key_block)
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

	if (result != II_OK)
		us_error(scb, 0x200003, "vucs2_keybld: Invalid key type");
	else
	{
		if (key_block->adc_tykey == II_KLOWKEY ||
		    key_block->adc_tykey == II_KEXACTKEY)
		{
			if (key_block->adc_lokey.db_data)
			{
				result = vucs2_convert(scb, &key_block->adc_kdv,
						&key_block->adc_lokey);
			}
		}
		if (result == II_OK && (key_block->adc_tykey == II_KRANGEKEY) )
		{
			VARUCS2 *z;
			int 	i;
			UCS2	*ptr;

			if (key_block->adc_lokey.db_data) {
				result = vucs2_convert(scb, &key_block->adc_kdv,
					&key_block->adc_lokey);
				z = (VARUCS2 *) key_block->adc_lokey.db_data;
				for (i=0, ptr=z->element_array; i<z->count; i++,ptr++)
				if ( *ptr == '_' || *ptr == '%' )
					*ptr = 0;
			}
			if (key_block->adc_hikey.db_data) {
				result = vucs2_convert(scb, &key_block->adc_kdv,
					&key_block->adc_hikey);
				z = (VARUCS2 *) key_block->adc_hikey.db_data;
				for (i=0, ptr=z->element_array; i<z->count; i++,ptr++)
				if ( *ptr == '_' || *ptr == '%' ) *ptr = MAX_UCS2;
			}
		}

		if (result != II_OK &&
		    key_block->adc_tykey == II_KHIGHKEY ||
		    key_block->adc_tykey == II_KEXACTKEY)
		{
			if (key_block->adc_hikey.db_data)
			{
				result = vucs2_convert(scb, &key_block->adc_kdv,
							&key_block->adc_hikey);
			}
		}
	}
	return(result);
}

/*{
** Name: vucs2_getempty	- Get an empty value
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_getempty(II_SCB		*scb ,
              II_DATA_VALUE	*empty_dv )
#else
vucs2_getempty(scb, empty_dv)
II_SCB             *scb;
II_DATA_VALUE      *empty_dv;
#endif
{
	II_STATUS	result = II_OK;
	VARUCS2		*chr;
	short		zero = 0;

	if (empty_dv->db_datatype != VARUCS2_TYPE ||
	    vucs2_lenchk(scb, 0, empty_dv, 0) != II_OK)
	{
		result = II_ERROR;
		us_error(scb, 0x200004, "vucs2_getempty: type/length mismatch");
	}
	else
	{
		chr = (VARUCS2 *)empty_dv->db_data;
		I2ASSIGN_MACRO(zero, chr->count);
	}
	return(result);
}

/*{
** Name: vucs2_valchk	- Check for valid values
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_valchk(II_SCB		*scb,
            II_DATA_VALUE	*dv )
#else
vucs2_valchk(scb, dv)
II_SCB             *scb;
II_DATA_VALUE      *dv;
#endif
{
	return(II_OK);
}

/*{
** Name: vucs2_hashprep	- Prepare a datavalue for becoming a hash key.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_hashprep(II_SCB       *scb ,
              II_DATA_VALUE   *dv_from ,
              II_DATA_VALUE  *dv_key )
#else
vucs2_hashprep(scb, dv_from, dv_key)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_key;
#endif
{
	II_STATUS	result = II_OK;
	int			i;
	VARUCS2		*chr;
	VARUCS2		*key_chr;
	short			chr_length;
	unsigned int		value;

	if (dv_from->db_datatype == VARUCS2_TYPE)
	{
		for (i = 0; i < dv_from->db_length; i++)
			dv_key->db_data[i] = 0;

		chr = (VARUCS2 *) dv_from->db_data;
		key_chr = (VARUCS2 *) dv_key->db_data;

		I2ASSIGN_MACRO(chr->count, chr_length);
		I2ASSIGN_MACRO(chr_length, key_chr->count);
		for (i = 0; i < chr_length; i++)
		{
			I2ASSIGN_MACRO(chr->element_array[i], key_chr->element_array[i]);
		}
		dv_key->db_length = dv_from->db_length;
	}
	else
	{
		result = II_ERROR;
		us_error(scb, 0x200005, "vucs2_hashprep: type/length mismatch");
	}
	return(result);
}

/*{
** Name: vucs2_helem	- Create histogram element for data value
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
**	Given these facts and the algorithm used in VARUCS2 comparison (see
**	vucs2_compare() above), the histogram for any value will be the first
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_helem(II_SCB		*scb,
            II_DATA_VALUE	*dv_from,
            II_DATA_VALUE	*dv_histogram)
#else
vucs2_helem(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	unsigned int		error_code;
	char		*msg;
	VARUCS2		*chr;
	short		length;

	if (dv_histogram->db_datatype != II_INTEGER)
	{
		result = II_ERROR;
		error_code = 0x200006;
		msg = "vucs2_helem: Type for histogram incorrect";
	}
	else if (dv_histogram->db_length != 2)
	{
		result = II_ERROR;
		error_code = 0x200007;
		msg = "vucs2_helem: Length for histogram incorrect";
	}
	else if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x200008;
		msg = "vucs2_helem: Base type for histogram incorrect";
	}
	else if (vucs2_lenchk(scb, 0, dv_from, (II_DATA_VALUE *) 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x200009;
		msg = "vucs2_helem: Base length for histogram incorrect";
	}
	else
	{
		UCS2 z;

		chr = (VARUCS2 *)dv_from->db_data;
		I2ASSIGN_MACRO(chr->count, length);

		if (length != 0)
		{
			I2ASSIGN_MACRO(chr->element_array[0], z);
			I2ASSIGN_MACRO(z, *dv_histogram->db_data);
		}
		else
		{
			z = 0;
			I2ASSIGN_MACRO(z, *dv_histogram->db_data);
		}
	}

	if (result != II_OK)
		us_error(scb, error_code, msg);
	return(result);
}

/*{
** Name: vucs2_hmin	- Create histogram for minimum value.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_hmin(II_SCB		*scb,
	   II_DATA_VALUE	*dv_from,
	   II_DATA_VALUE	*dv_histogram)
#else
vucs2_hmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	UCS2		zero = 0;
	unsigned int		error_code;

	if (dv_histogram->db_datatype != II_INTEGER)
	{
		result = II_ERROR;
		error_code = 0x20000a;
	}
	else if (dv_histogram->db_length != 2)
	{
		result = II_ERROR;
		error_code = 0x20000b;
	}
	else if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x20000c;
	}
	else if (vucs2_lenchk(scb, 0, dv_from, 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x20000d;
	}
	else
	{
		I2ASSIGN_MACRO(zero, *dv_histogram->db_data);
	}
	if (result != II_OK)
		us_error(scb, error_code, "vucs2_hmin: Invalid input parameters");
	return(result);
}

/*{
** Name: vucs2_dhmin	- Create `default' minimum histogram.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_dhmin(II_SCB		*scb,
	    II_DATA_VALUE	*dv_from,
	    II_DATA_VALUE	*dv_histogram)
#else
vucs2_dhmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	UCS2		zero = 0;
	unsigned int		error_code;

	if (dv_histogram->db_datatype != II_INTEGER)
	{
		result = II_ERROR;
		error_code = 0x20000e;
	}
	else if (dv_histogram->db_length != 2)
	{
		result = II_ERROR;
		error_code = 0x20000f;
	}
	else if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x200010;
	}
	else if (vucs2_lenchk(scb, 0, dv_from, 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x200011;
	}
	else
	{
		I2ASSIGN_MACRO(zero, *dv_histogram->db_data);
	}
	if (result != II_OK)
		us_error(scb, error_code, "vucs2_dhmin: Invalid input parameters");
	return(result);
}

/*{
** Name: vucs2_hmax	- Create histogram for maximum value.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_hmax(II_SCB		*scb,
	   II_DATA_VALUE	*dv_from,
	   II_DATA_VALUE	*dv_histogram)
#else
vucs2_hmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	UCS2		value = MAX_UCS2;
	unsigned int		error_code;

	if (dv_histogram->db_datatype != II_INTEGER)
	{
		result = II_ERROR;
		error_code = 0x200012;
	}
	else if (dv_histogram->db_length != 2)
	{
		result = II_ERROR;
		error_code = 0x200013;
	}
	else if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x200014;
	}
	else if (vucs2_lenchk(scb, 0, dv_from, 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x200015;
	}
	else
	{
		I2ASSIGN_MACRO(value, *dv_histogram->db_data);
	}
	if (result != II_OK)
		us_error(scb, error_code, "vucs2_hmax:  Invalid input parameters");

	return(result);
}

/*{
** Name: vucs2_dhmax	- Create `default' maximum histogram.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_dhmax(II_SCB		*scb,
	    II_DATA_VALUE	*dv_from,
	    II_DATA_VALUE	*dv_histogram)
#else
vucs2_dhmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	UCS2		value = MAX_UCS2;
	unsigned int		error_code;

	if (dv_histogram->db_datatype != II_INTEGER)
	{
		result = II_ERROR;
		error_code = 0x200016;
	}
	else if (dv_histogram->db_length != 2)
	{
		result = II_ERROR;
		error_code = 0x200017;
	}
	else if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x200018;
	}
	else if (vucs2_lenchk(scb, 0, dv_from, 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x200019;
	}
	else
	{
		I2ASSIGN_MACRO(value, *dv_histogram->db_data);
	}
	if (result != II_OK)
		us_error(scb, error_code, "vucs2_dhmax: Invalid parameters");
	return(result);
}

/*{
** Name: vucs2_hg_dtln	- Provide datatype & length for a histogram.
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_hg_dtln(II_SCB		*scb,
	      II_DATA_VALUE	*dv_from,
	      II_DATA_VALUE	*dv_histogram)
#else
vucs2_hg_dtln(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
	II_STATUS	result = II_OK;
	unsigned int		error_code;

	if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		result = II_ERROR;
		error_code = 0x20001a;
	}
	else if (vucs2_lenchk(scb, 0, dv_from, 0) != II_OK)
	{
		result = II_ERROR;
		error_code = 0x20001b;
	}
	else
	{
		dv_histogram->db_datatype = II_INTEGER;
		dv_histogram->db_length = 2;
	}
	if (result != II_OK)
		us_error(scb, error_code, "vucs2_hg_dtln: Invalid parameters");

	return(result);
}

/*{
** Name: vucs2_minmaxdv	- Provide the minimum/maximum values/lengths for a
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_minmaxdv(II_SCB		*scb,
	       II_DATA_VALUE	*min_dv,
	       II_DATA_VALUE	*max_dv)
#else
vucs2_minmaxdv(scb, min_dv, max_dv)
II_SCB		   *scb;
II_DATA_VALUE      *min_dv;
II_DATA_VALUE	   *max_dv;
#endif
{
	II_STATUS	result = II_OK;
	VARUCS2		*chr;
	unsigned int		error_code;
	II_DATA_VALUE	local_chr;

	if (min_dv != (II_DATA_VALUE *)NULL)
	{
		if (min_dv->db_datatype != VARUCS2_TYPE)
		{
			result = II_ERROR;
			error_code = 0x20001c;
		}
		else if (min_dv->db_length == II_LEN_UNKNOWN)
		{
			min_dv->db_length = sizeof(chr->count);
		}
		else if (vucs2_lenchk(scb, 0, min_dv, 0) == II_OK)
		{
			if (min_dv->db_data)
			{
				short zero = 0;
				chr = (VARUCS2 *) min_dv->db_data;
				I2ASSIGN_MACRO(zero, chr->count);
			}
		}
		else
		{
			result = II_ERROR;
			error_code = 0x20001d;
		}
	}

	if (max_dv != (II_DATA_VALUE *)NULL)
	{
		if (max_dv->db_datatype != VARUCS2_TYPE)
		{
			result = II_ERROR;
			error_code = 0x20001e;
		}
		else if (max_dv->db_length == II_LEN_UNKNOWN)
		{
			max_dv->db_length = 2000;
		}
		else if (vucs2_lenchk(scb, 0, max_dv, &local_chr) == II_OK)
		{
			if (max_dv->db_data)
			{
				int		i;
				UCS2		max = MAX_UCS2;
				short		s;

				chr = (VARUCS2 *) max_dv->db_data;
				for (i = 0; i < local_chr.db_length; i++)
				{
					I2ASSIGN_MACRO(max, chr->element_array[i]);
				}
				s = local_chr.db_length;
				I2ASSIGN_MACRO(s, chr->count);
			}
		}
		else
		{
			result = II_ERROR;
			error_code = 0x20001f;
		}
	}

	if (result != II_OK)
		us_error(scb, error_code, "vucs2_minmaxdv: Invalid parameters");

	return(result);
}

/*{
** Name: vucs2_convert	- Convert to/from VARUCS2s.
**
** Description:
**      This routine converts values to and/or from VARUCS2s.
**	The following conversions are supported.
**	    Varchar <-> VARUCS2
**	    Char <-> VARUCS2 -- same format
**	and, of course,
**	    VARUCS2 <-> VARUCS2 (length independent)
**
**	The Varchar and Char values are UTF8 (an 8-bit UCS transformation
**	format) data fields. The UTF8 fields are UCS2 values that have
**	been transformed into multibyte characters from 1 to 6 bytes in
**	length. This transformation is done by the Unicode Conversion
**	routines, vucs2_UTF8toUCS2 and vucs2_UCS2toUTF8. This
**	transformation has the following characteristics:
**
**	1)  The first byte indicates the number of bytes to follow in a
**	    multibyte sequence. Specifically, the number of binary 1's
**	    at the beginning of the data indicates the number of bytes
**	    in the UTF8 character. The next most significant bit is
**	    always 0. For example, a 2 byte UTF8 character starts with
**	    110 and a 6 byte character starts with 11111110.
**
**	2)  UTF8 data that starts with a binary 0 is a one byte character.
**	    Its format is 0xxxxxxx which means that 7 bits are free for data.
**
**	3)  A byte that starts with binary 10 is one of the trailing bytes of
**	    a multibyte sequence. This makes it efficient to find the start
**	    of a character starting from an arbitrary location in the data
**	    stream.
**
**	In summary:
**
**		0xxxxxxx	1st of 1 byte character
**		110xxxxx	1st of 2 byte charater
**		1110xxxx	1st of 3 byte character
**		11110xxx	1st of 4 byte character
**		111110xx	1st of 5 byte character
**		1111110x	1st of 6 byte character
**
**		10xxxxxx	2nd, 3rd, 4th, 5th, or 6th byte
**
**	where x, xx, ... represents the actual UCS2 data portion.
**
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
##	08-mar-1996 (tsale01)
##		Created.
##	02-apr-1998 (matbe01)
##		Revised to conform to Unicode Standard, version 2.0.
##		24-jan-2001 (fanra01, gupsh01)
##		Modified function vucs2_convert in order to correctly interpret the 
##		output length.

[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_convert(II_SCB		*scb,
	      II_DATA_VALUE	*dv_in,
	      II_DATA_VALUE	*dv_out)
#else
vucs2_convert(scb, dv_in, dv_out)
II_SCB             *scb;
II_DATA_VALUE      *dv_in;
II_DATA_VALUE      *dv_out;
#endif
{
	UTF8			*pf, *pfEnd;
	UCS2			*pu, *puEnd;
	VARUCS2			*chr_in;
	VARUCS2			*chr_out;
	int			i;
	II_STATUS		result = II_OK;
	unsigned int			error_code;
	char			*msg;
	II_VLEN			*vlen;
	II_DATA_VALUE		local_dv;
	short			length;
	short			count;

	dv_out->db_prec = 0;
	if (dv_in->db_datatype == VARUCS2_TYPE)
	{
		chr_in = (VARUCS2 *)dv_in->db_data;

		switch (dv_out->db_datatype) {
		case VARUCS2_TYPE:
			chr_out = (VARUCS2 *)dv_out->db_data;
			(void)vucs2_lenchk(scb, 1, dv_out, &local_dv);
			count = min(chr_in->count, local_dv.db_length);
			/*
			** Count is 1,. .,n but i is 0,. .,n-1
			** therefore:
			*/
			for (i = 0; i < count; i++)
				I2ASSIGN_MACRO(chr_in->element_array[i], 
						chr_out->element_array[i]);
			I2ASSIGN_MACRO(count, chr_out->count);
	  		break;

		case II_TEXT:
		case II_VARCHAR:
			vlen = (II_VLEN *)dv_out->db_data;
			length = dv_out->db_length - sizeof(vlen->vlen_length);
			pf = (UTF8 *)vlen->vlen_array;
			pfEnd = pf + length;
			pu = chr_in->element_array;
			puEnd = pu + chr_in->count;
			/*
			** On return from convert_to_UTF8, the source and
			** target pointers will point to the end of source
			** and end of target areas.
			*/
			if (convert_to_UTF8(&pu, puEnd, &pf, pfEnd) != II_OK)
			{
			    error_code = 0x200320;
			    msg = "vucs2_convert: Invalid data or parameters";
			    result = II_ERROR;
			}
			vlen->vlen_length = pf - (UTF8 *)vlen->vlen_array;
			break;

		case II_CHAR:
		case II_C:
			length = dv_out->db_length;
			pf = (UTF8 *)dv_out->db_data;
			pfEnd = pf + length;
			pu = chr_in->element_array;
			puEnd = pu + chr_in->count;
			/*
			** On return from convert_to_UTF8, the source and
			** target pointers will point to the end of source
			** and end of target areas.
			*/
			if (convert_to_UTF8(&pu, puEnd, &pf, pfEnd) != II_OK)
			{
			    error_code = 0x200320;
			    msg = "vucs2_convert: Invalid data or parameters";
			    result = II_ERROR;
			}
			break;

		default:
			error_code = 0x200020;
			msg = "vucs2_convert: Unkown output type";
			result = II_ERROR;
		}
	} else {
		chr_out = (VARUCS2 *)dv_out->db_data;
		(void) vucs2_lenchk(scb, 0, dv_out, &local_dv);

		switch (dv_in->db_datatype) {
		case II_C:
		case II_CHAR:
			pf = (UTF8 *)dv_in->db_data;
			pfEnd = pf + dv_in->db_length;
	/* copying a 20 byte char into a 20 size unicode will not work
	** because length will be (20 - 2) thus puEnd will exhaust first.
	** removed length = local_dv.db_length - sizeof(chr_in->count); 
	*/
			length = local_dv.db_length;
			pu = chr_out->element_array;
			puEnd = pu + length;
			/*
			** On return from convert_from_UTF8, the source and
			** target pointers will point to the end of source
			** and end of target areas.
			*/
			if (convert_from_UTF8(&pf, pfEnd, &pu, puEnd) != II_OK)
			{
			    error_code = 0x200320;
			    msg = "vucs2_convert: Invalid data or parameters";
			    result = II_ERROR;
			}
			/* chr_out->count = chr_out->element_array - pu; */
			chr_out->count = pu - (UCS2 *)chr_out->element_array;
			break;

		case II_TEXT:
		case II_VARCHAR:
		case II_LONGTEXT:
			vlen = (II_VLEN *) dv_in->db_data;
			length = dv_in->db_length - sizeof(vlen->vlen_length);
			pf = (UTF8 *)vlen->vlen_array;
			pfEnd = pf + length;
			pu = chr_out->element_array;
			
			/*			
			** Incorrect end point calculation removed
			** puEnd = pu + chr_out->count; 
			*/
            
			puEnd = pu + local_dv.db_length;

			/*
			** On return from convert_from_UTF8, the source and
			** target pointers will point to the end of source
			** and end of target areas.
			*/
			if (convert_from_UTF8(&pf, pfEnd, &pu, puEnd) != II_OK)
			{
			    error_code = 0x200320;
			    msg = "vucs2_convert: Invalid data or parameters";
			    result = II_ERROR;
			}

			/*			
			** removed: 
			** vlen->vlen_length = pu - (UCS2 *)vlen->vlen_array; 
			*/           
            chr_out->count = pu - (UCS2 *)chr_out->element_array;

			break;

		default:
			error_code = 0x200021;
			msg = "vucs2_convert:  Unknown input type";
			result = II_ERROR;
		}
	}
	if (result != II_OK)	
		us_error(scb, error_code, msg);
	return(result);
}
II_STATUS
#ifdef __STDC__
convert_to_UTF8 (       UCS2**	sourceStart,
                  const UCS2*	sourceEnd,
                        UTF8**	targetStart,
                  const UTF8*	targetEnd)
#else
convert_to_UTF8 (sourceStart, sourceEnd, targetStart, targetEnd)
      UCS2**      sourceStart;
const UCS2*       sourceEnd;
      UTF8**      targetStart;
const UTF8*       targetEnd;
#endif
{
II_STATUS	    result = II_OK;
register UCS2*              source = *sourceStart;
register UTF8*              target = *targetStart;
register UCS4               ch, ch2;
register unsigned short     bytesToWrite;
register const UCS4         byteMask = 0xBF;
register const UCS4         byteMark = 0x80;
const 	 int   		    halfShift = 10;
         UTF8               firstByteMark[7] =
                            {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

const    UCS4	halfBase = 0x0010000UL, halfMask = 0x3FFUL, 
		kReplacementCharacter =	0x0000FFFDUL, 
		kMaximumUCS2 = 0x0000FFFFUL, kMaximumUCS4 = 0x7FFFFFFFUL,
		kSurrogateHighStart = 0xD800UL, kSurrogateHighEnd = 0xDBFFUL,
		kSurrogateLowStart = 0xDC00UL, kSurrogateLowEnd = 0xDFFFUL;

      while (source < sourceEnd) {
              bytesToWrite = 0;
              ch = *source++;
              if (ch >= kSurrogateHighStart && ch <= kSurrogateHighEnd
                              && source < sourceEnd) 
              {
                     ch2 = *source;
                     if (ch2 >= kSurrogateLowStart && ch2 <= kSurrogateLowEnd) 
                     {
                              ch = ((ch - kSurrogateHighStart) << halfShift)
                                      + (ch2 - kSurrogateLowStart) + halfBase;
                              ++source;
                     }
               }
               if (ch < 0x80)
                     bytesToWrite = 1; 
               else if (ch < 0x800)
                     bytesToWrite = 2;
               else if (ch < 0x10000)
                     bytesToWrite = 3;
               else if (ch < 0x200000)
                     bytesToWrite = 4;
               else if (ch < 0x4000000)
                     bytesToWrite = 5;
               else if (ch <= kMaximumUCS4)
                     bytesToWrite = 6;
               else {
                     bytesToWrite = 2;
                     ch = kReplacementCharacter;
		    }
                
               target += bytesToWrite;
               if (target > targetEnd)
	       {
                     target -= bytesToWrite;
                     result = II_TARGET_EXHAUSTED;
                     break;
               }
               switch (bytesToWrite)   /* note: code falls through cases! */
		{
                case 6:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 5:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 4:
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 3: 
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 2: 
                        *--target = (ch | byteMark) & byteMask; ch >>= 6;
                case 1: 
                        *--target =  ch | firstByteMark[bytesToWrite];
                }
                target += bytesToWrite;
        }
        *sourceStart = source;
        *targetStart = target;
        return result;
}

II_STATUS
#ifdef __STDC__
convert_from_UTF8(      UTF8**      sourceStart,
                  const UTF8*       sourceEnd,
                        UCS2**      targetStart,
                  const UCS2*       targetEnd)
#else
convert_from_UTF8(sourceStart, sourceEnd, targetStart, targetEnd)
      UTF8**      sourceStart;
const UTF8*       sourceEnd;
      UCS2**      targetStart;
const UCS2*       targetEnd;
#endif
{
II_STATUS	    result = II_OK;
register UTF8*              source = *sourceStart;
register UCS2*              target = *targetStart;
register UCS4               ch;
register unsigned short     extraBytesToWrite;
const 	 int   		    halfShift = 10;

const    UCS4	halfBase = 0x0010000UL, halfMask = 0x3FFUL, 
		kReplacementCharacter =	0x0000FFFDUL, 
		kMaximumUCS2 = 0x0000FFFFUL, kMaximumUCS4 = 0x7FFFFFFFUL,
		kSurrogateHighStart = 0xD800UL, kSurrogateHighEnd = 0xDBFFUL,
		kSurrogateLowStart = 0xDC00UL, kSurrogateLowEnd = 0xDFFFUL;

         UCS4   offsetsFromUTF8[6] =  
                {0x00000000UL, 0x00003080UL, 0x000E2080UL,
                 0x03C82080UL, 0xFA082080UL, 0x82082080UL};

         char   bytesFromUTF8[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

        while (source < sourceEnd) {
                ch = 0;
                extraBytesToWrite = bytesFromUTF8[*source];
                if (source + extraBytesToWrite > sourceEnd) 
		{
                        result = II_SOURCE_EXHAUSTED;
			break;
                }
                switch(extraBytesToWrite) /* note: code falls through cases! */
		{
                case 5:
			ch += *source++; 
			ch <<= 6;
                case 4: 
			ch += *source++; 
			ch <<= 6;
                case 3: 
			ch += *source++; 
			ch <<= 6;
                case 2: 
			ch += *source++; 
			ch <<= 6;
                case 1: 
			ch += *source++; 
			ch <<= 6;
                case 0: 
			ch += *source++;
                }
                ch -= offsetsFromUTF8[extraBytesToWrite];

                if (target >= targetEnd) 
		{
                        result = II_TARGET_EXHAUSTED; 
			break;
                }
                if (ch <= kMaximumUCS2) 
                        *target++ = ch;
                else if (ch > kMaximumUCS4)
                        *target++ = kReplacementCharacter;
                else {
			if (target + 1 >= targetEnd) 
			{
                       		result = II_TARGET_EXHAUSTED; 
				break;
                	}
                	ch -= halfBase;
                	*target++ = (ch >> halfShift) + kSurrogateHighStart;
                	*target++ = (ch & halfMask) + kSurrogateLowStart;
                }
        }
        *sourceStart = source;
        *targetStart = target;
        return result;
}

/*{
** Name: vucs2_tmlen	- Determine 'terminal monitor' lengths
**
** Description:
**      This routine returns the default and worst case lengths for a datatype
**	if it were to be printed as text (which is the way things are
**	displayed in the terminal monitor).  Although in this release,
**	user-defined datatypes are not returned to the terminal monitor, this
**	routine (and its partner, vucs2_tmcvt()) are needed for various trace
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_tmlen(II_SCB		*scb,
	    II_DATA_VALUE	*dv_from,
	    short		*def_width,
	    short		*largest_width)
#else
vucs2_tmlen(scb, dv_from, def_width, largest_width)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
short              *def_width;
short              *largest_width;
#endif
{
	II_DATA_VALUE	    local_dv;

	if (dv_from->db_datatype != VARUCS2_TYPE)
	{
		us_error(scb, 0x200022, "vucs2_tmlen: Invalid input data");
		return(II_ERROR);
	}
	else
	{
		*def_width = *largest_width = 30;
	}
	return(II_OK);
}

/*{
** Name: vucs2_tmcvt	- Convert to displayable format
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_tmcvt(II_SCB		*scb,
	    II_DATA_VALUE	*from_dv,
	    II_DATA_VALUE	*to_dv,
	    int			*output_length)
#else
vucs2_tmcvt(scb, from_dv, to_dv, output_length)
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

    status = vucs2_convert(scb, from_dv, &local_dv);
    *output_length = local_dv.db_length;
    return(status);
}

/*{
** Name: vucs2_dbtoev	- Determine which external datatype this will convert to
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
##	08-mar-1996 (tsale01)
##		Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_dbtoev(II_SCB		*scb,
	     II_DATA_VALUE	*db_value,
	     II_DATA_VALUE	*ev_value)
#else
vucs2_dbtoev(scb, db_value, ev_value)
II_SCB		   *scb;
II_DATA_VALUE	   *db_value;
II_DATA_VALUE	   *ev_value;
#endif
{

	if (db_value->db_datatype == VARUCS2_TYPE)
	{
		ev_value->db_datatype = II_VARCHAR;
		ev_value->db_length = db_value->db_length;
		ev_value->db_prec = 0;
	}
	else
	{
		us_error(scb, 0x200023, "vucs2_dbtoev: Invalid input data");
		return(II_ERROR);
	}

	return(II_OK);
}

/*{
** Name: vucs2_concatenate	- Concatenate two lists
**
** Description:
**	This function concatenates two VARUCS2s.  The result is a string which is
**	the concatenation of the two strings.
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
##	08-mar-1996 (tsale01)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_concatenate(II_SCB		*scb,
		  II_DATA_VALUE		*dv_1,
		  II_DATA_VALUE		*dv_2,
		  II_DATA_VALUE		*dv_result)
#else
vucs2_concatenate(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
	II_STATUS	result = II_ERROR;
	VARUCS2		*str_1;
	VARUCS2		*str_2;
	VARUCS2		*res_str;
	II_DATA_VALUE	local_dv;
	int		i;
	short		len_1;
	short		len_2;
	short		len_res;

	if (dv_1->db_datatype != VARUCS2_TYPE ||
	    dv_2->db_datatype != VARUCS2_TYPE ||
	    dv_result->db_datatype != VARUCS2_TYPE ||
	    dv_result->db_data == NULL
	   )
	{
		us_error(scb, 0x200024, "vucs2_concatenate: Invalid input");
		return(II_ERROR);
	}

	str_1 = (VARUCS2 *)dv_1->db_data;
	str_2 = (VARUCS2 *)dv_2->db_data;
	res_str = (VARUCS2 *)dv_result->db_data;

	(void)vucs2_lenchk(scb, 0, dv_result, &local_dv);
	I2ASSIGN_MACRO(str_1->count, len_1);
	I2ASSIGN_MACRO(str_2->count, len_2);

	if (local_dv.db_length < len_1 + len_2)
	{
		us_error(scb, 0x200025, "Usz_concatenate: Result too long.");
		return(II_ERROR);
	}

	len_res = 0;

	for (i = 0; i < len_1; i++)
	{
		I2ASSIGN_MACRO(str_1->element_array[i],
			res_str->element_array[len_res++]);
	}

	for (i = 0; i < len_2; i++)
	{
		I2ASSIGN_MACRO(str_2->element_array[i],
			res_str->element_array[len_res++]);
	}

	I2ASSIGN_MACRO(len_res, res_str->count);

	return(II_OK);
}

/*{
** Name: vucs2_size	- Find number of Unicode code points in a string
**
** Description:
**
**	This routine extracts the count from a VARUCS2
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
##	08-mar-1996 (tsale01)
##          Created
##      15-Feb-2008 (hanal04) Bug 119922
##          IIADD_DT_DFN entries were all missing an initialisation for
##          the dtd_compr_addr() fcn ptr.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
vucs2_size(II_SCB		*scb,
	   II_DATA_VALUE	*dv_1,
	   II_DATA_VALUE	*dv_result)
#else
vucs2_size(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
	II_STATUS	result = II_ERROR;
	II_DATA_VALUE	local_dv;
	unsigned int		lanswer;
	short		answer;

	if (dv_1->db_datatype == VARUCS2_TYPE
	&&  dv_result->db_datatype == II_INTEGER
	&&  dv_result->db_length == 2
	&&  dv_result->db_data
	&&  vucs2_lenchk(scb, 0, dv_1, &local_dv) != II_ERROR
	   )
	{
		I4ASSIGN_MACRO(local_dv.db_length, lanswer);
		answer = lanswer;
		I2ASSIGN_MACRO(answer, *dv_result->db_data);
		result = II_OK;
	}
	else
		us_error(scb, 0x200026, "vucs2_size: Invalid input");

	return (result);
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
#define		    OP_SUMOP		(OP_ORDPAIR+1)
#define		    OP2CPX		(OP_SUMOP+1)

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
	{ II_O_OPERATION, { "sumop"}, OP_SUMOP, II_AGGREGATE },
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
#define		    FI_SUM		 (FI_LTXT2I4+27)
#define		    FI_SUMOP		 (FI_LTXT2I4+28)
#define		    FI_COMPLEX		 (FI_LTXT2I4+29)

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

	{ II_O_FUNCTION_INSTANCE, FI_SUM, II_NO_FI,
		II_SUM_OP, II_AGGREGATE, II_FID_F0_NOFLAGS, 0,
	    1, op2op_parms , ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_sum,  0 },

	{ II_O_FUNCTION_INSTANCE, FI_SUMOP, II_NO_FI,
		OP_SUMOP, II_AGGREGATE, II_FID_F0_NOFLAGS, 0,
	    1, op2op_parms , ORP_TYPE, II_RES_FIXED, sizeof(ORD_PAIR),
	    II_PSVALID, usop_sum,  0 },

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

  /********************************************************************/
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

  /************************************************************************/
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
[@history_template@]...
*/
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

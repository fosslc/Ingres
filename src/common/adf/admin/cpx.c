
/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/
#include    <iiadd.h>
#include    <math.h>
#include    <stdio.h>
#include    "udt.h"

#ifdef	ALIGNMENT_REQUIRED

#define		    I2ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(short), (char *) &(b))
#define		    F4ASSIGN_MACRO(a,b)		byte_copy((char *) &(a), sizeof(float),(char *) &(b))
#define		    F8ASSIGN_MACRO(a,b)		byte_copy((char *) &(a), sizeof(double), (char *)&(b))

#else

#define		    I2ASSIGN_MACRO(a,b)		((*(short *)&(b)) = (*(short *)&(a)))
#define		    F4ASSIGN_MACRO(a,b)		((*(float *)&(b)) = (*(float *)&(a)))
#define		    F8ASSIGN_MACRO(a,b)		((*(double *)&(b)) = (*(double *)&(a)))

#endif

#define			FMAX		1E37
#define			FMIN		-(FMAX)

#define			min(a, b)   ((a) <= (b) ? (a) : (b))

/**
**
**  Name: CPX.C - Implements the CPX datatype and related functions
**
**  Description:
**      This file contains the routines necessary to implement the complex 
**	number datatype.  This datatype consists of elements of data which are
**	internally represented as two double's.  These two values represent the
**	complex number z = x + yi learned in high school algebra.
**
**	A variety of functions are provided.  The operators +,-,* and / are
**	implemented.  
**
{@func_list@}...
**
**
##  History:
##	19-aug-1991 (lan)
##          Created by Tim H.
##      05-aug-1992 (stevet)
##          Modified uscpx_convert to force number > -0.0005 and < 0.005 to 
##          display as 0.000 instead of -0.000, which can happen in SunOS.
##      21-apr-1993 (stevet)
##          Changed <udt.h> back to "udt.h" since this file lives in DEMO.
##       5-May-1993 (fred)
##          Prototyped code.
##      17-aug-1993 (stevet)
##          Added support for CPX->VARCHAR,CPX->LONGTEXT and LONTEXT->CPX.
##	28-oct-93 (vijay)
##	    Add AIX scanf format string.
##	14-apr-94 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
##	    Add axp_osf to ris_us5 entry to read doubles in sscanf().
##      17-may-94 (mhuishi)
##          Add __ALPHA to ris_us5 entry for sscanf format.
##      09-feb-95 (chech02)
##          Add rs4_us5 to ris_us5 entry for sscanf format.
##	24-jan-1995 (wadag01)
##          Add sos_us5 to ris_us5 entry for sscanf format (SCO OpenServer 5).
##	27-feb-1996 (smiba01)
##          Add usl_us5 to ris_us5 entry for sscanf format (Unixware 2.0.2)
##      06-mar-1996 (canor01)
##          Add int_wnt entry for sscanf format (Intel Windows NT).
##	05-feb-96 (morayf)
##          Add rmx_us5 to ris_us5 entry for sscanf format for SNI RMx00 port.
##          However, it is the original %F which is non-portable and should
##          be only used explicitly by the platform which needs it, should
##          this even exist.
##	01-mar-1996 (morayf)
##          Add pym_us5 to ris_us5 entry for sscanf format for Pyramid port.
##	08-apr-1996 (tutto01)
##	    Changed int_wnt to NT_GENERIC to include Alpha NT.
##	02-feb-1997 (kamal03)
##	    Splitted #if defined ... statement with continuation into multiple
##	    #if  statements to make VAX C precompiler happy.
##	12-dec-1997 (hweho01)
##          Add dgi_us5 entry for sscanf format (DG/UX Intel)
##	5-May-1998 (bonro01)
##          Add dg8_us5 entry for sscanf format (DG/UX Motorola)
##	5-dec-1998 (nanpr01)
##          Add su4_us5 entry for sscanf format (Sun/Solaris)
##	01-Apr-1999 (kinte01)
##	    Add VMS entry for sscanf format.
##	22-jun-1999 (toumi01)
##	    Add lnx_us5 entry for sscanf format (Linux).
##      03-jul-99 (podni01)
##          Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
##	06-oct-1999 (toumi01)
##	    Change Linux config string from lnx_us5 to int_lnx.
##	22-Nov-1999 (hweho01)
##	    Add ris_u64 entry for sscanf format (AIX 64-bit).
##	05-may-2000 (hanch04)
##	    Now spaces are allow after a line continuation.
##	14-Jun-2000 (hanje04)
##	    Added ibm_lnx entry for sscanf format
##	07-Sep-2000 (hanje04)
##	    Add axp_lnx (Alpha Linux) entry for sscanf format.
##	13-Sep-2002 (hanch04)
##	    Add su9_us5 entry for sscanf format (Solaris 64-bit).
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##	22-Sep-2002 (hweho01)
##	    Add r64_us5 entry for sscanf format (AIX hybrid build).
##      08-Oct-2002 (hanje04)
##         As part of AMD x86-64 Linux port, replace individual Linux
##         defines with generic LNX define where apropriate
##      07-Jan-2003 (hanch04)  
##          Back out change for bug 108637
##          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
##	07-Dec-2004 (hanje04)
##	    BUG 113057
##	    Use generic __linux__ define for linux.
##	20-Jan-2005 (hanje04)
##	    Re-add fix for BUG 113057 after it was removed by X-integ
##	15-Mar-2005 (bonro01)
##	    Add a64_sol entry for sscanf format on Solaris AMD65
##	2-May-2007 (bonro01)
##	    All supported platforms now use %lf in sscanf()
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
*/

/*}
** Name: COMPLEX - A complex number
**
** Description:
**      This structure defines a complex number in our world.  The two elements,
**	both double's, represent the real and imaginary parts a complex number. 
**
**
**
**	This typedef is actually defined in "UDT.h".  Included here for
**	anyone's information.
**
typedef struct _COMPLEX
{
    double              real;		( The real part        )
    double		imag;		( The imaginary part  )
} COMPLEX;
**
*/

/*
**  Definition of static variables and forward static functions.
*/

static  II_DT_ID    cpx_datatype_id = COMPLEX_TYPE;


/*{
** Name: cpx_compare	- Compare two CPX's for the purpose of INGRES in-
**			  ternals (eg sorting)
**
** Description:
**      This routine compares two CPX's.  The following rules apply.
**
**	    if (cpx1.real > cpx2.real)
**		cpx1 > cpx2
**	    else if (cpx1.real < cpx2.real)
**		cpx1 < cpx2
**	    else if (cpx1.real == cpx2.real)
**	    {
**		if (cpx1.imag > cpx2.imag)
**		    cpx1 > cpx2
**		else if (cpx1.imag < cpx2.imag)
**		    cpx1 < cpx2
**		else
**		    cpx1 == cpx2
**	    } 
**
** Inputs:
**      scb                             Scb structure
**      cpx1                            First operand
**      cpx2                            Second operand
**					These are pointers to II_DATA_VALUE
**					structures which contain the values to
**					be compared.
**      result                          Pointer to i4  to contain the result of
**					the operation
**
** Outputs:
**      *result                         Filled with the result of the operation.
**					This routine will make *result
**					    < 0 if cpx1 < cpx2
**					    > 0 if cpx1 > cpx2
**					  or  0 if cpx1 == cpx2
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
uscpx_compare(II_SCB		 *scb,
	      II_DATA_VALUE      *cpx1,
	      II_DATA_VALUE      *cpx2,
	      int                *result)
#else
uscpx_compare(scb, cpx1, cpx2, result)
II_SCB		   *scb;
II_DATA_VALUE      *cpx1;
II_DATA_VALUE      *cpx2;
int                *result;
#endif
{
    COMPLEX		*cpxv_1 = (COMPLEX *) cpx1->db_data;
    COMPLEX		*cpxv_2 = (COMPLEX *) cpx2->db_data;
    double		c1_real;
    double		c1_imag;
    double		c2_real;
    double		c2_imag;

    if (    (cpx1->db_datatype != cpx_datatype_id)
	 || (cpx2->db_datatype != cpx_datatype_id)
	 || (cpx1->db_length != sizeof(COMPLEX))
	 || (cpx2->db_length != sizeof(COMPLEX))
       )
    {
	/* Then this routine has been improperly called */
	us_error(scb, 0x200200, "uscpx_compare: Type/length mismatch");
	return(II_ERROR);
    }

    /*
    ** Now perform the comparison based on the rules described above.
    */

    F8ASSIGN_MACRO(cpxv_1->real, c1_real);
    F8ASSIGN_MACRO(cpxv_1->imag, c1_imag);
    F8ASSIGN_MACRO(cpxv_2->real, c2_real);
    F8ASSIGN_MACRO(cpxv_2->imag, c2_imag);

    if (c1_real > c2_real)
    {
	*result = 1;
    }
    else if (c1_real < c2_real)
    {
	*result = -1;
    }
    else if (c1_imag > c2_imag)
    {
	*result = 1;
    }
    else if (c1_imag < c2_imag)
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
** Name: uscpx_lenchk	- Check the length of the datatype for validity
**
** Description:
**      This routine checks that the specified length for the datatype is valid.
**	Since complex numbers are fixed length (i.e. more like dates than text),
**	this routine has little work to do.  If the length is user specified,
**	than the only valid length is zero (i.e. unspecified);  if the length is
**	not user specified, then the only valid length is the size of the
**	COMPLEX structure.  Not really much work to do.
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
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_lenchk(II_SCB		*scb,
	     int		user_specified,
	     II_DATA_VALUE	*dv,
	     II_DATA_VALUE	*result_dv)
#else
uscpx_lenchk(scb, user_specified, dv, result_dv)
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
	if ((dv->db_length != 0) )
	    result = II_ERROR;
	length = sizeof(COMPLEX);
    }
    else
    {
	if ((dv->db_length != sizeof(COMPLEX)) )
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
	us_error(scb, 0x200201,
			"uscpx_lenchk: Invalid length for COMPLEX datatype");
    }
    return(result);
}

/*{
** Name: uscpx_keybld	- Build an ISAM, BTREE, or HASH key from the value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is build based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	our simple complex number data, these are typically simply the values.
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
##      6-Mar-1989 (fred)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_keybld(II_SCB	*scb,
	     II_KEY_BLK	*key_block)
#else
uscpx_keybld(scb, key_block)
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
	us_error(scb, 0x200202, "uscpx_keybld: Invalid key type");
    }
    else
    {
	if ((key_block->adc_tykey == II_KLOWKEY) ||
		(key_block->adc_tykey == II_KEXACTKEY))
	{
	    if (key_block->adc_lokey.db_data)
	    {
		result = uscpx_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_lokey);
	    }
	}
	if ( result &&
		((key_block->adc_tykey == II_KHIGHKEY) ||
		    (key_block->adc_tykey == II_KEXACTKEY)))
	{
	    if (key_block->adc_hikey.db_data)
	    {
		result = uscpx_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_hikey);
	    }
	}
    }    
    return(result);
}

/*{
** Name: uscpx_getempty	- Get an empty value
**
** Description:
**      This routine constructs the given empty value for this datatype.  By
**	definition, the empty value will be z = 0.  This routine merely con-
**	structs one of those in the space provided. 
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
uscpx_getempty(II_SCB         *scb,
	       II_DATA_VALUE  *empty_dv)
#else
uscpx_getempty(scb, empty_dv)
II_SCB             *scb;
II_DATA_VALUE      *empty_dv;
#endif
{
    II_STATUS		result = II_OK;
    COMPLEX		*cpx;
    double		zero = 0.0;
    
    if ((empty_dv->db_datatype != cpx_datatype_id)
	||  (empty_dv->db_length != sizeof(COMPLEX)))
    {
	result = II_ERROR;
	us_error(scb, 0x200203, "uscpx_getempty: type/length mismatch");
    }
    else
    {
	cpx = (COMPLEX *) empty_dv->db_data;
	F8ASSIGN_MACRO(zero, cpx->real);
	F8ASSIGN_MACRO(zero, cpx->imag);
    }
    return(result);
}

/*{
** Name: uscpx_valchk	- Check for valid values
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
uscpx_valchk(II_SCB             *scb,
	     II_DATA_VALUE      *dv)
#else
uscpx_valchk(scb, dv)
II_SCB             *scb;
II_DATA_VALUE      *dv;
#endif
{

    return(II_OK);
}

/*{
** Name: uscpx_hashprep	- Prepare a datavalue for becoming a hash key.
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
uscpx_hashprep(II_SCB             *scb,
	       II_DATA_VALUE      *dv_from,
	       II_DATA_VALUE	   *dv_key)
#else
uscpx_hashprep(scb, dv_from, dv_key)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_key;
#endif
{
    II_STATUS           result = II_OK;

    if (dv_from->db_datatype == cpx_datatype_id)
    {
	byte_copy((char *) dv_from->db_data,
		dv_from->db_length,
		dv_key->db_data);
	dv_key->db_length = dv_from->db_length;
    }
    else
    {
	result = II_ERROR;
	us_error(scb, 0x200204, "uscpx_hashprep: type/length mismatch");
    }
    return(result);
}

/*{
** Name: uscpx_helem	- Create histogram element for data value
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
**	Given these facts and the algorithm used in complex number comparison 
**	(see uscpx_compare() above), the histogram for any value will be the 
**	real part.  The datatype will be II_FLOAT (float), with a length of 8.
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
uscpx_helem(II_SCB             *scb,
	    II_DATA_VALUE      *dv_from,
	    II_DATA_VALUE      *dv_histogram)
#else
uscpx_helem(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE	   *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    double		value;
    i4		error_code;
    char		*msg;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200205;
	msg = "uscpx_helem: Type for histogram incorrect";
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200206;
	msg = "uscpx_helem: Length for histogram incorrect";
    }
    else if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200207;
	msg = "uscpx_helem: Base type for histogram incorrect";
   }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x200208;
	msg = "uscpx_helem: Base length for histogram incorrect";
    }
    else
    {
	F8ASSIGN_MACRO(((COMPLEX *)dv_from->db_data)->real, value);
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: uscpx_hmin	- Create histogram for minimum value.
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
uscpx_hmin(II_SCB             *scb,
	   II_DATA_VALUE      *dv_from,
	   II_DATA_VALUE      *dv_histogram)
#else
uscpx_hmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = FMIN;
    i4			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200209;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x20020a;
    }
    else if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x20020b;
    }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x20020c;
    }
    else
    {
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "uscpx_hmin: Invalid input parameters");
    return(result);
}

/*{
** Name: uscpx_dhmin	- Create `default' minimum histogram.
**
** Description:
**      This routine creates the minimum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' complex numbers are 
**	found between z=0 and z=100+100i.  Therefore, the default minimum 
**	histogram will have a value of 0; the default maximum histogram (see 
**	below) will have a value of 100. 
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
uscpx_dhmin(II_SCB             *scb,
	    II_DATA_VALUE      *dv_from,
	    II_DATA_VALUE      *dv_histogram)
#else
uscpx_dhmin(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = 0.0;
    i4			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x20020d;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x20020e;
    }
    else if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x20020f;
    }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x200210;
    }
    else
    {
	
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "uscpx_dhmin: Invalid input parameters");
    return(result);
}

/*{
** Name: uscpx_hmax	- Create histogram for maximum value.
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
uscpx_hmax(II_SCB             *scb,
	   II_DATA_VALUE      *dv_from,
	   II_DATA_VALUE      *dv_histogram)
#else
uscpx_hmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = FMAX;
    i4			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200211;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200212;
    }
    else if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200213;
    }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x200214;
    }
    else
    {
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "uscpx_hmax:  Invalid input parameters");
	
    return(result);
}

/*{
** Name: uscpx_dhmax	- Create `default' maximum histogram.
**
** Description:
**      This routine creates the maximum default histogram.  The default
**	histograms are used by the optimizer when no histogram data is present
**	in the system catalogs (i.e. OPTIMIZEDB has not been run).
**
**	For our datatype, we will assume that ``usual'' complex numbers are 
**	found between z=0 and z=100 + 100i.
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
uscpx_dhmax(II_SCB             *scb,
	    II_DATA_VALUE      *dv_from,
	    II_DATA_VALUE      *dv_histogram)
#else
uscpx_dhmax(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS			result = II_OK;
    double			value = 100.0;
    i4			error_code;

    if (dv_histogram->db_datatype != II_FLOAT)
    {
	result = II_ERROR;
	error_code = 0x200215;
    }
    else if (dv_histogram->db_length != 8)
    {
	result = II_ERROR;
	error_code = 0x200216;
    }
    else if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200217;
    }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x200218;
    }
    else
    {
	F8ASSIGN_MACRO(value, *dv_histogram->db_data);
    }
    if (result)
	us_error(scb, error_code, "uscpx_dhmax: Invalid parameters");
    return(result);
}

/*{
** Name: uscpx_hg_dtln	- Provide datatype & length for a histogram.
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
uscpx_hg_dtln(II_SCB             *scb,
	      II_DATA_VALUE      *dv_from,
	      II_DATA_VALUE      *dv_histogram)
#else
uscpx_hg_dtln(scb, dv_from, dv_histogram)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
II_DATA_VALUE      *dv_histogram;
#endif
{
    II_STATUS           result = II_OK;
    i4		error_code;

    if (dv_from->db_datatype != cpx_datatype_id)
    {
	result = II_ERROR;
	error_code = 0x200219;
    }
    else if (dv_from->db_length != sizeof(COMPLEX))
    {
	result = II_ERROR;
	error_code = 0x20021a;
    }
    else
    {
	dv_histogram->db_datatype = II_FLOAT;
	dv_histogram->db_length = 8;
    }
    if (result)
	us_error(scb, error_code, "uscpx_hg_dtln: Invalid parameters");
	
    return(result);
}

/*{
** Name: uscpx_minmaxdv	- Provide the minimum/maximum values/lengths for a
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
uscpx_minmaxdv(II_SCB		   *scb,
	       II_DATA_VALUE       *min_dv,
	       II_DATA_VALUE	   *max_dv)
#else
uscpx_minmaxdv(scb, min_dv, max_dv)
II_SCB		   *scb;
II_DATA_VALUE      *min_dv;
II_DATA_VALUE	   *max_dv;
#endif
{
    II_STATUS           result = II_OK;
    COMPLEX		*cpx;
    i4		error_code;
    double		value;

    if (min_dv)
    {
	if (min_dv->db_datatype != cpx_datatype_id)
	{
	    result = II_ERROR;
	    error_code = 0x20021b;
	}
	else if (min_dv->db_length == sizeof(COMPLEX))
	{
	    if (min_dv->db_data)
	    {
		cpx = (COMPLEX *) min_dv->db_data;
		value = FMIN;
		F8ASSIGN_MACRO(value, cpx->real);
		F8ASSIGN_MACRO(value, cpx->imag);
	    }
	}
	else if (min_dv->db_length == II_LEN_UNKNOWN)
	{
	    min_dv->db_length = sizeof(COMPLEX);
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20021c;
	}
    }
    if (max_dv)
    {
	if (max_dv->db_datatype != cpx_datatype_id)
	{
	    result = II_ERROR;
	    error_code = 0x20021d;
	}
	else if (max_dv->db_length == sizeof(COMPLEX))
	{
	    if (max_dv->db_data)
	    {
		cpx = (COMPLEX *) max_dv->db_data;
		value = FMAX;
		F8ASSIGN_MACRO(value, cpx->real);
		F8ASSIGN_MACRO(value, cpx->imag);
	    }
	}
	else if (max_dv->db_length == II_LEN_UNKNOWN)
	{
	    max_dv->db_length = sizeof(COMPLEX);
	}
	else
	{
	    result = II_ERROR;
	    error_code = 0x20021e;
	}
    }
    if (result)
	us_error(scb, error_code, "uscpx_minmaxdv: Invalid parameters");
    return(result);
}

/*{
** Name: uscpx_convert	- Convert to/from complex numbers
**
** Description:
**      This routine converts values to and/or from ordered pairs.
**	The following conversions are supported.
**	    CPX -> c
**	    c  -> CPX
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
##      July-1989 (Tim)
##          Created.
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_convert(II_SCB             *scb,
	      II_DATA_VALUE      *dv_in,
	      II_DATA_VALUE      *dv_out)
#else
uscpx_convert(scb, dv_in, dv_out)
II_SCB             *scb;
II_DATA_VALUE      *dv_in;
II_DATA_VALUE      *dv_out;
#endif
{
    char		*b;
    char		*b1;
    COMPLEX             *cpx;
    short		length;
    II_STATUS		result = II_OK;
    double		x, y;
    int			match_requirement;
    i4		error_code;
    char		*msg;
    char                buffer[64];
    char		end_paren[2];
    char		junk[2];

    for (length = 0; length < sizeof(buffer); length++)
	buffer[length] = 0;
    
    dv_out->db_prec = 0;
    if ((dv_in->db_datatype == cpx_datatype_id)
	&& (dv_out->db_datatype == cpx_datatype_id))
    {
	byte_copy((char *) dv_in->db_data,
		dv_in->db_length,
		dv_out->db_data);
    }
    else if (dv_in->db_datatype == cpx_datatype_id)
    {
	double	    x_part;
	double	    y_part;
	
	cpx = (COMPLEX *) dv_in->db_data;
	F8ASSIGN_MACRO(cpx->real, x_part);
	F8ASSIGN_MACRO(cpx->imag, y_part);


	/* 
	** Change x_part and y_part so that -0.000 will appear as 0.000 
	** on SunOS.
	*/
	if( y_part < 0.0005 && y_part > -0.0005)
	    y_part = 0.0;

	if( x_part < 0.0005 && x_part > -0.0005)
	    x_part = 0.0;

	sprintf(buffer, " %11.3f + %11.3fi ", x_part, y_part);
	length = strlen(buffer);
	
	switch (dv_out->db_datatype) 
	{
	  case II_CHAR:
	  case II_C:
	    if (length > dv_out->db_length)
	    {
		error_code = 0x20021f;
		msg = "uscpx_convert: Insufficient space for COMPLEX output";
		result = II_ERROR;
	    }
	    byte_copy((char *) buffer, length, dv_out->db_data);
	    break;
	  case II_VARCHAR:
	  case II_TEXT:
	  case II_LONGTEXT:
	    if (length > dv_out->db_length - 2)
	    {
		error_code = 0x20021f;
		msg = "uscpx_convert: Insufficient space for COMPLEX output";
		result = II_ERROR;
	    }
	    byte_copy((char *) buffer, length,
		      (char *)(dv_out->db_data + 2));
	    I2ASSIGN_MACRO(dv_out->db_data, length);
	    break;
	  default:
	    error_code = 0x200220;
	    msg = "uscpx_convert:  Unknown input type";
	    result = II_ERROR;
	}
    }
    else
    {
	for (;;)
	{
	    cpx = (COMPLEX *) dv_out->db_data;
	    
	    switch (dv_in->db_datatype)
	    {
		case II_C:
		case II_CHAR:
		    length = dv_in->db_length;
		    if (length < sizeof(buffer))
			byte_copy((char *) dv_in->db_data, length, buffer);
		    break;

		case II_TEXT:
		case II_VARCHAR:
		case II_LONGTEXT:
		    length = dv_in->db_length - 2;
		    if (length < sizeof(buffer))
			byte_copy((char *) (dv_in->db_data + 2),
				length,
				buffer);
		    break;

		default:
		    error_code = 0x200220;
		    msg = "uscpx_convert:  Unknown input type";
		    result = II_ERROR;
	    }
	    if (result != II_OK)
		break;

	    result = II_ERROR;
	    if (length < sizeof(buffer))
	    {
		length = sscanf(buffer, " [ %lf , %lf%1s %1s",
				&x, &y, end_paren, junk);
	    }
	    
	    if ((length != 3) || strcmp("]", end_paren))
	    {
		error_code = 0x200221;
		msg = "Unable to convert to complex number";
		break;
	    }

	    F8ASSIGN_MACRO(x, cpx->real);
	    F8ASSIGN_MACRO(y, cpx->imag);
	    
	    result = II_OK;
	    break;
	}
    }
    if (result)
	us_error(scb, error_code, msg);
    return(result);
}

/*{
** Name: uscpx_tmlen	- Determine 'terminal monitor' lengths
**
** Description:
**      This routine returns the default and worst case lengths for a datatype
**	if it were to be printed as text (which is the way things are
**	displayed in the terminal monitor).  Although in this release,
**	user-defined datatypes are not returned to the terminal monitor, this
**	routine (and its partner, uscpx_tmcvt()) are needed for various trace
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
uscpx_tmlen(II_SCB             *scb,
	    II_DATA_VALUE      *dv_from,
	    short              *def_width,
	    short              *largest_width)
#else
uscpx_tmlen(scb, dv_from, def_width, largest_width)
II_SCB             *scb;
II_DATA_VALUE      *dv_from;
short              *def_width;
short              *largest_width;
#endif
{
    II_DATA_VALUE	    local_dv;

    if (dv_from->db_datatype != ORP_TYPE)
    {
	us_error(scb, 0x200222, "uscpx_tmlen: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	*def_width = *largest_width = 28;
    }
    return(II_OK);
}

/*{
** Name: uscpx_tmcvt	- Convert to displayable format
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
uscpx_tmcvt(II_SCB             *scb,
	    II_DATA_VALUE      *from_dv,
	    II_DATA_VALUE      *to_dv,
	    int                *output_length)
#else
uscpx_tmcvt(scb, from_dv, to_dv, output_length)
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
    
    status = uscpx_convert(scb, from_dv, &local_dv);
    *output_length = local_dv.db_length;
    return(status);
}

/*{
** Name: uscpx_dbtoev	- Determine which external datatype this will convert to
**
** Description:
**      This routine returns the external type to which ordered pairs will be converted. 
**      The correct type is CHAR, with a length of 28.
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
uscpx_dbtoev(II_SCB		   *scb,
	     II_DATA_VALUE	   *db_value,
	     II_DATA_VALUE	   *ev_value)
#else
uscpx_dbtoev(scb, db_value, ev_value)
II_SCB		   *scb;
II_DATA_VALUE	   *db_value;
II_DATA_VALUE	   *ev_value;
#endif
{

    if (db_value->db_datatype != cpx_datatype_id)
    {
	us_error(scb, 0x200223, "uscpx_dbtoev: Invalid input data");
	return(II_ERROR);
    }
    else
    {
	ev_value->db_datatype = II_CHAR;
	ev_value->db_length = 28;
	ev_value->db_prec = 0;
    }
    return(II_OK);
}

/*{
** Name: uscpx_add	- Compute the sum of two complex numbers
**
** Description:
**      This routine adds two complex numbers.
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing z1
**	dv_2				Datavalue representing z2
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_add(II_SCB             *scb,
	  II_DATA_VALUE      *dv_1,
	  II_DATA_VALUE      *dv_2,
	  II_DATA_VALUE      *dv_result)
#else
uscpx_add(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			real1;
    double			imag1;
    double			real2;
    double			imag2;
    COMPLEX			*cpx_1;
    COMPLEX			*cpx_2;
    COMPLEX			sum;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_2->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_1 = (COMPLEX *) dv_1->db_data;
	cpx_2 = (COMPLEX *) dv_2->db_data;
	F8ASSIGN_MACRO(cpx_1->real, real1);
	F8ASSIGN_MACRO(cpx_1->imag, imag1);
	F8ASSIGN_MACRO(cpx_2->real, real2);
	F8ASSIGN_MACRO(cpx_2->imag, imag2);
	sum.real = real1 + real2;
	sum.imag = imag1 + imag2;
	byte_copy((char *) &sum,
		  sizeof(sum),
		  dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200224, "uscpx_add: Invalid input");
    return(result);
}

/*{
** Name: uscpx_sub	- Compute the difference of two complex numbers
**
** Description:
**      This routine subtracts two complex numbers, arg1 - arg2
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing z1
**	dv_2				Datavalue representing z2
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_sub(II_SCB             *scb,
	  II_DATA_VALUE      *dv_1,
	  II_DATA_VALUE      *dv_2,
	  II_DATA_VALUE      *dv_result)
#else
uscpx_sub(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			real1;
    double			imag1;
    double			real2;
    double			imag2;
    COMPLEX			*cpx_1;
    COMPLEX			*cpx_2;
    COMPLEX			sum;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_2->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_1 = (COMPLEX *) dv_1->db_data;
	cpx_2 = (COMPLEX *) dv_2->db_data;
	F8ASSIGN_MACRO(cpx_1->real, real1);
	F8ASSIGN_MACRO(cpx_1->imag, imag1);
	F8ASSIGN_MACRO(cpx_2->real, real2);
	F8ASSIGN_MACRO(cpx_2->imag, imag2);
	sum.real = real1 - real2;
	sum.imag = imag1 - imag2;
	byte_copy((char *)&sum,sizeof(sum),dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200225, "uscpx_sub: Invalid input");
    return(result);
}

/*{
** Name: uscpx_mul	- Compute the product of two complex numbers
**
** Description:
**      This routine multiplies two complex numbers.
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing z1
**	dv_2				Datavalue representing z2
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_mul(II_SCB             *scb,
	  II_DATA_VALUE      *dv_1,
	  II_DATA_VALUE      *dv_2,
	  II_DATA_VALUE      *dv_result)
#else
uscpx_mul(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			real1;
    double			imag1;
    double			real2;
    double			imag2;
    COMPLEX			*cpx_1;
    COMPLEX			*cpx_2;
    COMPLEX			mul;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_2->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_1 = (COMPLEX *) dv_1->db_data;
	cpx_2 = (COMPLEX *) dv_2->db_data;
	F8ASSIGN_MACRO(cpx_1->real, real1);
	F8ASSIGN_MACRO(cpx_1->imag, imag1);
	F8ASSIGN_MACRO(cpx_2->real, real2);
	F8ASSIGN_MACRO(cpx_2->imag, imag2);
	mul.real = (real1*real2) - (imag1*imag2);
	mul.imag = (real1*imag2) + (real2*imag1);
	byte_copy((char *)&mul,sizeof(mul),dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200226, "uscpx_mul: Invalid input");
    return(result);
}

/*{
** Name: uscpx_div	- Compute the quotient of two complex numbers
**
** Description:
**      This routine divides two complex numbers, arg1 / arg2
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing z1
**	dv_2				Datavalue representing z2
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_div(II_SCB             *scb,
	  II_DATA_VALUE      *dv_1,
	  II_DATA_VALUE      *dv_2,
	  II_DATA_VALUE      *dv_result)
#else
uscpx_div(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			real1;
    double			imag1;
    double			real2;
    double			imag2;
    double			conj;
    COMPLEX			*cpx_1;
    COMPLEX			*cpx_2;
    COMPLEX			quo;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_2->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_1 = (COMPLEX *) dv_1->db_data;
	cpx_2 = (COMPLEX *) dv_2->db_data;
	F8ASSIGN_MACRO(cpx_1->real, real1);
	F8ASSIGN_MACRO(cpx_1->imag, imag1);
	F8ASSIGN_MACRO(cpx_2->real, real2);
	F8ASSIGN_MACRO(cpx_2->imag, imag2);
	conj = (real2*real2) + (imag2*imag2);
	quo.real = ((real1*real2) + (imag1*imag2)) / conj;
	quo.imag = ((real2*imag1) - (real1*imag2)) / conj;
	byte_copy((char *)&quo,sizeof(quo),dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200227, "uscpx_div: Invalid input");
    return(result);
}

/*{
** Name: uscpx_cmpnumber - Create a complex number from two floats
**
** Description:
**
**	This routine creates a complex number from two floats.  This is per-
**	formed by simple assignment.  The first argument is the real part; the
**	second, the imaginary.
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing real part
**	dv_2				Datavalue representing imaginary part
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Complex number result is placed here.
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_cmpnumber(II_SCB             *scb,
		II_DATA_VALUE      *dv_1,
		II_DATA_VALUE      *dv_2,
		II_DATA_VALUE      *dv_result)
#else
uscpx_cmpnumber(scb, dv_1, dv_2, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_2;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			cpx;
    double			rl;
    double			im;
    float			an_f4;

    if (    (dv_1->db_datatype == II_FLOAT)
	&&  (dv_2->db_datatype == II_FLOAT)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	if (dv_1->db_length == 4)
	{
	    F4ASSIGN_MACRO(*dv_1->db_data, an_f4);
	    rl = an_f4;
	}
	else
	{
	    F8ASSIGN_MACRO(*dv_1->db_data, rl);
	}
	if (dv_2->db_length == 4)
	{
	    F4ASSIGN_MACRO(*dv_2->db_data, an_f4);
	    im = an_f4;
	}
	else
	{
	    F8ASSIGN_MACRO(*dv_2->db_data, im);
	}
	cpx.real = rl;
	cpx.imag = im;
	byte_copy((char *)&cpx, sizeof(cpx), dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200228, "uscpx_cmpnumber: Invalid input");
    return(result);
}

/*{
** Name: uscpx_real - Find real part a complex number
**
** Description:
**	This routine extracts the real part a complex number
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_real(II_SCB             *scb,
	   II_DATA_VALUE      *dv_1,
	   II_DATA_VALUE      *dv_result)
#else
uscpx_real(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			*cpx;
    double			rl;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	cpx = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx->real, rl);
	F8ASSIGN_MACRO(rl, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200229, "uscpx_real: Invalid input");
    return(result);
}

/*{
** Name: uscpx_imag - Find imaginary part of a complex number
**
** Description:
**	This routine extracts the imaginary of a complex number
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_imag(II_SCB             *scb,
	   II_DATA_VALUE      *dv_1,
	   II_DATA_VALUE      *dv_result)
#else
uscpx_imag(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS           result = II_ERROR;
    COMPLEX		*cpx;
    double		im;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	cpx = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx->imag, im);
	F8ASSIGN_MACRO(im, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x20022a, "uscpx_imag: Invalid input");
    return(result);
}

/*{
** Name: uscpx_conj	- Compute the conjugate of a complex number
**
** Description:
**      This routine computes the conjugate value of a complex number.  If
**	z = x + yi, then conj(z) = x - yi
**
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing input
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              complex result is placed here.
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_conj(II_SCB             *scb,
	   II_DATA_VALUE      *dv_1,
	   II_DATA_VALUE      *dv_result)
#else
uscpx_conj(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			*cpx_in;
    COMPLEX			cpx_out;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_in = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx_in->real, cpx_out.real);
	F8ASSIGN_MACRO(cpx_in->imag, cpx_out.imag);
	cpx_out.imag = (-cpx_out.imag);
	byte_copy((char *) &cpx_out, dv_result->db_length, dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x20022b, "uscpx_conj: Invalid input");
    return(result);
}

/*{
** Name: uscpx_negate	- Compute the negative of a complex number
**
** Description:
**      This routine computes the negative value of a complex number.  If
**	z = x + yi, then -(z) = -x - yi
**
**	The result is a complex number.
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing input
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              complex result is placed here.
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_negate(II_SCB             *scb,
	     II_DATA_VALUE      *dv_1,
	     II_DATA_VALUE      *dv_result)
#else
uscpx_negate(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			*cpx_in;
    COMPLEX			cpx_out;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	cpx_in = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx_in->real, cpx_out.real);
	F8ASSIGN_MACRO(cpx_in->imag, cpx_out.imag);
	cpx_out.real = (-cpx_out.real);
	cpx_out.imag = (-cpx_out.imag);
	byte_copy((char *) &cpx_out, dv_result->db_length, dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x20022c, "uscpx_negate: Invalid input");
    return(result);
}

/*{
** Name: uscpx_abs	- Compute the absolute value of a complex number
**
** Description:
**      This routine computes the absolute value of a complex number.  It
**	performs this function using the standard formula of
**	    abs(z) = sqrt( re(z) ** 2) + ( im(z) ** 2) )
**
**	The result is a floating point number (double).
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_abs(II_SCB             *scb,
	  II_DATA_VALUE      *dv_1,
	  II_DATA_VALUE      *dv_result)
#else
uscpx_abs(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			distance;
    double			cpx_r;
    double			cpx_i;
    COMPLEX			*cpx;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == II_FLOAT)
	&&  (dv_result->db_length == 8)
	&&  (dv_result->db_data)
       )  
    {
	cpx = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx->real, cpx_r);
	F8ASSIGN_MACRO(cpx->imag, cpx_i);
	distance = sqrt(
			pow(cpx_r, (double) 2.0) +
			pow(cpx_i, (double) 2.0)
			);
	F8ASSIGN_MACRO(distance, *dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x20022d, "uscpx_abs: Invalid input");
    return(result);
}

/*{
** Name: uscpx_rtheta	- Compute the ordered pair (r,theta) of a complex 
**			  number, ie polar coordinates in the x-y plane
**
** Description:
**      This routine computes an ordered pair (r,theta) for a complex number,
**	such that if z = x + yi, then 
**		     z = exp(ln(r)) * (cos(theta + isin(theta)).
**	It performs this function using the standard formulae
**		r = abs(z)
**		theta = arctan(x/y)
**
**	The results is a ordered pair (2 doubles).
**
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing point 1
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_rtheta(II_SCB             *scb,
	     II_DATA_VALUE      *dv_1,
	     II_DATA_VALUE      *dv_result)
#else
uscpx_rtheta(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    double			distance;
    double			cpx_r;
    double			cpx_i;
    II_DATA_VALUE		r;
    ORD_PAIR			rtheta;
    COMPLEX			*z;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == ORP_TYPE)
	&&  (dv_result->db_length == sizeof(ORD_PAIR))
	&&  (dv_result->db_data)
       )  
    {
	r.db_datatype = II_FLOAT; r.db_length = 8; 
	r.db_data = (char *) &distance;
	result = uscpx_abs(scb, dv_1, &r);
	if (!result) {
	  F8ASSIGN_MACRO(distance, rtheta.op_x);
	  z = (COMPLEX *) dv_1->db_data;
	  F8ASSIGN_MACRO(z->real, cpx_r);
	  F8ASSIGN_MACRO(z->imag, cpx_i);
	  if ( (cpx_i == 0.0) && (cpx_r == 0.0) ) rtheta.op_y = 0.0;
	  else rtheta.op_y = atan2( cpx_i, cpx_r );
	  byte_copy((char *) &rtheta, dv_result->db_length, dv_result->db_data );
	}
    }
    if (result)
	us_error(scb, 0x20022e, "uscpx_rtheta: Invalid input");
    return(result);
}

/*{
** Name: usop2cpx - Create a complex number from an ordered pair
**
** Description:
**
**	This routine creates a complex number from an ordered pair.  This is 
**	performed by simple assignment, the x value becomes the real part; the
**	the y value, the imaginary.
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing ordered pair
**	dv_result			Datavalue in which to place the result
**
** Outputs:
**      *dv_result.db_data              Complex number result is placed here.
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
usop2cpx(II_SCB             *scb,
	 II_DATA_VALUE      *dv_1,
	 II_DATA_VALUE      *dv_result)
#else
usop2cpx(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			cpx;
    ORD_PAIR			*op;
    double			rl;
    double			im;

    if (    (dv_1->db_datatype == ORP_TYPE)
	&&  (dv_result->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_length == sizeof(COMPLEX))
	&&  (dv_result->db_data)
       )  
    {
	op = (ORD_PAIR *) dv_1->db_data;
	F8ASSIGN_MACRO(op->op_x,cpx.real);
	F8ASSIGN_MACRO(op->op_y,cpx.imag);
	byte_copy((char *)&cpx, sizeof(cpx), dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x20022f, "uscpx_op2cpx: Invalid input");
    return(result);
}

/*{
** Name: uscpx2op - Create an ordered pair from a complex number 
**
** Description:
**
**	This routine creates an ordered pair from a complex number.  This is 
**	performed by simple assignment, the real part becomes x value; the
**	the imaginary part becomes the y value.
**  
** Inputs:
**      scb                             Scb
**      dv_1                            Datavalue representing complex number
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx2op(II_SCB             *scb,
	 II_DATA_VALUE      *dv_1,
	 II_DATA_VALUE      *dv_result)
#else
uscpx2op(scb, dv_1, dv_result)
II_SCB             *scb;
II_DATA_VALUE      *dv_1;
II_DATA_VALUE      *dv_result;
#endif
{
    II_STATUS			result = II_ERROR;
    COMPLEX			*cpx;
    ORD_PAIR			op;

    if (    (dv_1->db_datatype == cpx_datatype_id)
	&&  (dv_result->db_datatype == ORP_TYPE)
	&&  (dv_result->db_length == sizeof(ORD_PAIR))
	&&  (dv_result->db_data)
       )  
    {
	cpx = (COMPLEX *) dv_1->db_data;
	F8ASSIGN_MACRO(cpx->real,op.op_x);
	F8ASSIGN_MACRO(cpx->imag,op.op_y);
	byte_copy((char *)&op, sizeof(op), dv_result->db_data);
	result = II_OK;
    }
    if (result)
	us_error(scb, 0x200230, "uscpx_cpx2op: Invalid input");
    return(result);
}

/*{
** Name: uscpx_bad_op - Tell user operator cannot be used with complex numbers
**
** Description:
**
**	This routine returns an error, informing the user that the operators 
**	'<', '<=', '>', and '>=' cannot be used with complex numbers.
**  
** Inputs:
**      scb                             Scb
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
##      July-1989 (T.)
##          Created
[@history_template@]...
*/
II_STATUS
#ifdef __STDC__
uscpx_bad_op(II_SCB             *scb)
#else
uscpx_bad_op(scb)
II_SCB             *scb;
#endif
{
    II_STATUS			result = II_ERROR;
    us_error(scb, 0x200231, 
	"uscpx_bad_op: Bad operator used with complex numbers");
    return(result);
}

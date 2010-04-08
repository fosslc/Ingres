/*
** Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/
#include    <iiadd.h>
#include    <ctype.h>
#include    <stdio.h>
#include    "iipk.h"
#include    "udt.h"

/*
** NUMERIC.C -- Routines implementing PNUM datatype as well as currency,
**              fraction and pnum functions.
**
**  The routines included in this file are:
**       byte_copy() - Copy bytes from one place to another.
**       usnum_compare() - Compare two numeric datatype values.
**       usnum_dbtoev() - Determine which type to export
**       usnum_getempty() - Build the "empty" data value.
**       usnum_hashprep() - Convert PNUM data value into
**       usnum_helem() - Build a histogram element for PNUM data value.
**       usnum_hg_dtln() - Get datatype and length histogram mapping
**       usnum_dhmax() - Create the "default" maximum histogram element
**       usnum_hmax() - Create the maximum histogram element for PNUM.
**       usnum_keybld - Build an ISAM, BTREE, or HASH key from a PNUM data.
**       usnum_lenchk() - Check validity of length and precision.
**       usnum_minmaxdv() - Actually builds the min and max value or length.
**       usnum_tmlen() - Provide default and worst-case lengths for display.
**       usnum_tmcvt() - Convert data value to terminal monitor display.
**       usnum_dhmin() - Create the "default" minimum histogram element.
**       usnum_hmin() -  Create the minimum histogram element for PNUM.
**       usnum_valchk - Check for valid values.
**       usnum_convert() - Convert various datatypes to/from PNUM datatype.
**       usnum_add() - Adding two PNUM numbers.
**       usnum_sub() - Substracting PNUM numbers.
**       usnum_mul() - Multiply two PNUM numbers.
**       usnum_div() - Dividing two PNUM numbers.
**       usnum_numeric() - Convert formatted character string to PNUM.
**       usnum_numeric_1arg() - Convert character string to PNUM using the
**                              default format.
**       usnum_currency() - Convert PNUM data to currency.
**       usnum_currency_1arg() - Convert PNUM data to currency using the
**                               default format.
**       usnum_fraction() - Convert PNUM data to fraction.
**       usnum_fraction_1arg() - Convert PNUM data to fraction using the
**                               defualt format.
**       usnum_format() - Internal routine that parses the format string.
**	 usop_sum() - Sum the 2 elements of a ord_pair.
**
##  History:
##       16-mar-92 (stevet)
##         Initial Creation.
##       09-apr-92 (stevet)
##         Fixed compiler warnings on ULTRIX.  
##         Add validation to usnum_lenchk() to make sure db_length > II_SCALE.
##         Added I1 support for in usnum_convert().
##         Fixed problem with handling negative fraction values.
##         Cleanup error trapping for usnum_add, usnum_sub, usnum_mul, 
##           and usnum_div routines.
##       12-dec-1993 (stevet)
##         64->65 merge.  Remove usnum_error() and replace it with
##         us_error().  Also prototype functions.
##	20-Sep-1993 (fredv)
##	   Moved iipk.h after ctype.h so it can be compiled.
##	05-nov-1993 (swm)
##	    Bug #58879
##	    The ASSIGN macros assume that the respective C types in each
##	    macro are the correct size. On axp_osf a long is 8 bytes, not
##	    4, so in the I4ASSIGN_MACRO the sizeof(long) is replaced with
##	    sizeof(int) for axp_osf. (Is there some obscure reason why the
##	    literal value 4 is not used?)
##      16-feb-1994 (stevet)
##          Added non prototype function declarations.  OS like HP/UX and 
##          SunOS does not come with ANSI C compiler.  Removed byte_copy()
##          routine, which is already defined in common.c file.
##      14-apr-94 (johnst)
##	    Bug #60764
##	    Change explicit long declarations to use i4 type defined
##	    in iiadd.h, which is conditionally defined to be either long
##	    or int as necessary to ensure type correctness and correct 
##	    alignment on platforms such as axp_osf where long's are 64-bit.
## 	11-apr-1995 (wonst02)
##	    Bug #68008
## 	    Handle internal DECIMAL type, which looks just like
## 	    the PNUM user-defined type.
##      28-aug-1997 (musro02)
##          Exchanged the order of includes for iipk.h and stdio.h to resolve
##          a problem in sqs_ptx:  the define of u_char in iipk.h collides
##          with a subsequent typedef in <sys/types.h>;  config string is not
##          defined here so that ifdef is not possible.
##	01-apr-1999 (kinte01)
##	    Add a return(status) to satisfy DEC C 5.6 compiler warnings. 
##	20-apr-1999 (somsa01)
##	    Where we pass in II_SCALE as the source value to IICVpkpk(),
##	    we now pass in the result of II_S_DECODE_MACRO(). This is the
##	    PROPER scale of the source value.
##	08-July-1999 (andbr08)
##	    Bug #94792: In usnum_convert() in the case of a Decimal value, need
##	    to use scale of decimal value to be coerced, not II_SCALE.
##	21-jan-1999 (hanch04)
##	    replace nat and longnat with i4
##	31-aug-2000 (hanch04)
##	    cross change to main
##	    replace nat and longnat with i4
##      13-Sep-2002 (hanal04) Bug 108637
##          Mark 2232 build problems found on DR6. Include iiadd.h
##          from local directory, not include path.
##      07-Jan-2003 (hanch04)  
##          Back out change for bug 108637
##          Previous change breaks platforms that need ALIGNMENT_REQUIRED.
##          iiadd.h should be taken from $II_SYSTEM/ingres/files/iiadd.h
##  	29-Aug-2004 (kodse01)
##          BUG 112917
##          Handled i8 to PNUM conversion in usnum_convert.
##      26-Oct-2005 (hanje04)
##          Add prototype for usnum_format, to prevent compiler errors
##          under GCC 4.0 due to conflicts with implicit prototypes.
##	28-Oct-2005 (drivi01)
##	    Added missing colon after usnum_format ptototype.
*/

  

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
#define		    I4ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(i4), (char *) &(b))
#define		    I8ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(i8), (char *) &(b))
#define		    F4ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(float), (char *) &(b))
#define		    F8ASSIGN_MACRO(a,b)		byte_copy((char *)&(a), sizeof(double), (char *) &(b))

#else

#define		    I2ASSIGN_MACRO(a,b)		((*(short *)&(b)) = (*(short *)&(a)))
#define		    I4ASSIGN_MACRO(a,b)		((*(i4 *)&(b)) = (*(i4 *)&(a)))
#define		    I8ASSIGN_MACRO(a,b)		((*(i8 *)&(b)) = (*(i8 *)&(a)))
#define             F4ASSIGN_MACRO(a,b)         ((*(float *)&(b)) = (*(float *)&(a)))
#define             F8ASSIGN_MACRO(a,b)         ((*(double *)&(b)) = (*(double *)&(a)))

#endif

/* 
** Scale, which is the number of digits after the decimal point, for this 
** numeric data is defined by II_SCALE.  Change this value if the scale of 
** this data type needs to be a value other than 2.
*/
#define 		II_SCALE  	2
#define			FMAX		1E37
#define			FMIN		-(FMAX)

/* 
** These prime numbers are used to convert decimal value to fraction, only 
** prime numbers from 1-20 are listed.  User can add to this list but this
** implementation can only provide complete support for fractions with 
** denominator divisible by 2 or 5.
*/

static int prime[]={2, 3, 5, 7, 11, 13, 17, 19, -1};

/* format structure */

typedef struct
{
    char  decimal;                 /* decimal point character */
    int     sc;                      /* Scale, number of digits after decimal 
				        point */
    char  thousand;                /* Thousand spearator */
    char  fraction;                /* The sparator for denominator and 
				        numerator */
    char  curr[II_MAX_CURR+1];                /* Currency sign */
} NUM_FORMAT;
  
/* local prototypes */
static II_STATUS
#ifdef __STDC__
usnum_format( 
NUM_FORMAT           *format,
int                  len,
char        	     *string);
#else
usnum_format();
#endif

  
/*
** Name: usnum_compare()	- compare two numeric datatype values.
**
** Description:
**      This routine compares two numeric values.
**
** Inputs:
**	scb				scb structure.
**      dv1                             Pointer to the 1st data value
**                                      to compare.
**      dv2                             Pointer to the 2nd data value
**                                      to compare.
**      cmp_result                      Pointer to the int to put the
**				 	comparison result.
**
** Outputs:
**	scb				scb structure.
**      *cmp_result                     Result of comparison.  This is
**                                      guaranteed to be:
**                                          < 0   if 1st < 2nd
**                                          = 0   if 1st = 2nd
**                                          > 0   if 1st > 2nd
**
**      Returns:
**          II_OK                 Operation succeeded.
**        
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##      12-mar-92 (stevet)
##          Created.
*/

II_STATUS
#ifdef __STDC__
usnum_compare(
II_SCB		    *scb,
II_DATA_VALUE	    *adc_dv1,
II_DATA_VALUE	    *adc_dv2,
int		    *cmp_result)
#else
usnum_compare(scb, adc_dv1, adc_dv2, cmp_result)
II_SCB		    *scb;
II_DATA_VALUE	    *adc_dv1;
II_DATA_VALUE	    *adc_dv2;
int		    *cmp_result;
#endif
{
    *cmp_result = IIMHpkcmp( adc_dv1->db_data,
			   II_LEN_TO_PR_MACRO(adc_dv1->db_length),
			   II_SCALE,
			   adc_dv2->db_data,
			   II_LEN_TO_PR_MACRO(adc_dv2->db_length),
			   II_SCALE);

    return(II_OK);
}




/*{
** Name: usnum_dbtoev	- Determine which type to export
**
** Description:
**      This routine determines which type to send to output for 
**	PNUM.  It is assumed that a coercion exists for this datatype
**	transformation.
**
**
** Inputs:
**	scb				Pointer to a session control block.
**      db_value			Ptr to db_data_value for database
**					type/prec/length
**      ev_value                        Pointer to II_DATA_VALUE for exported 
**                                      type
**
** Outputs:
**      *ev_value                       Filled appropriately.
**
**	Returns:
**          II_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      12-mar-92 (stevet)
##          Created.
*/
II_STATUS
#ifdef __STDC__
usnum_dbtoev(
II_SCB             *scb,
II_DATA_VALUE      *db_value,
II_DATA_VALUE	   *ev_value)
#else
usnum_dbtoev(scb, db_value, ev_value)
II_SCB             *scb;
II_DATA_VALUE      *db_value;
II_DATA_VALUE	   *ev_value;
#endif
{

	ev_value->db_datatype = II_CHAR;
	ev_value->db_length   = II_PS_TO_PRN_MACRO(
				  II_LEN_TO_PR_MACRO(db_value->db_length), 
				  II_SCALE);
	ev_value->db_prec     = db_value->db_prec;
	return( II_OK);
}



/*
** Name: usnum_getempty() - Build the "empty" data value.
**
** Description:
**      This function will build the "empty" (or "default-default") value 
**      PNUM data value.  
**
** Inputs:
**	scb				Pointer to a session control block.
**      emptydv 			Pointer to II_DATA_VALUE to
**                                      place the empty data value in.
**
** Outputs:
**      emptydv                         The empty data value.
**              .db_data		The data for the empty data
**                                      value will be placed here.
**
**      Returns:
**              II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
**
## History:
*/

II_STATUS
#ifdef __STDC__
usnum_getempty(
II_SCB              *scb,
II_DATA_VALUE       *emptydv)	/* Ptr to empty data value */
#else
usnum_getempty(scb, emptydv)
II_SCB              *scb;
II_DATA_VALUE       *emptydv;	/* Ptr to empty data value */
#endif
{
	int		i;
	char		*p=(char *)emptydv->db_data;

	for( i=0; i < emptydv->db_length; i++)
	     *(p+i) = 0;

	((char *)emptydv->db_data)[emptydv->db_length-1]
		= 0xc;

	return( II_OK);
}

/*
** Name: usnum_hashprep()	-   Convert PNUM data value into 
**                                  "hash-prep" form.
**
** Description:
**      This function actually does the hash preparation for PNUM
**      datatype.  Besides +0 and -0 are hash to the same bucket, -0 is 
**      converted to +0. 
**
** Inputs:
**	scb				Pointer to a session control block.
**      adc_dvfrom                      Pointer to the input data value.
**      adc_dvhp                        Pointer to the data value to be
**					created. 
**
** Outputs:
**      dvhp                            The resulting data value.
**		.db_length		The resulting length of the hashprep
**					value.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##      16-mar-92 (stevet)
##          Initial creation.
*/

II_STATUS
#ifdef __STDC__
usnum_hashprep(
II_SCB              *scb,
II_DATA_VALUE	    *dvfrom,
II_DATA_VALUE	    *dvhp)
#else
usnum_hashprep(scb, dvfrom, dvhp)
II_SCB              *scb;
II_DATA_VALUE	    *dvfrom;
II_DATA_VALUE	    *dvhp;
#endif
{
    char	*fromp;
    char	*fromendp;
    char	*hpp;
    char	*c;
    int		pr;
    int		cnt;
    int		len;



	byte_copy( dvfrom->db_data, dvfrom->db_length,
		   dvhp->db_data);
	dvhp->db_length = dvfrom->db_length;

	/* must make sure +0 and -0 hash to the same bucket; convert -0 to +0 */
	c  = (char *)dvhp->db_data;
	pr = II_LEN_TO_PR_MACRO(dvfrom->db_length);
	while (pr > 0)
	{
	    if (((pr-- & 1  ?  (*c >> 4)  :  *c++) & 0xf) != 0)
		break;
	}

	/* if all digits were zero, make sure number is positive zero */
	if (pr == 0)
	    *c =0xc;
	    
	return( II_OK);
}


/*
** Name: usnum_helem() - Build a histogram element for PNUM data value.
**
** Description:
**      This function will convert a data value of PNUM datatype into
**	its FLOAT8 histogram element form.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dvfrom                          Pointer to the data value to be
**                                      converted.
**      dvhg                            Pointer to the data value to be
**					created.
** Outputs:
**      dvhg                            The resulting data value.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##      16-mar-92 (stevet)
##          Initial creation.
*/

II_STATUS
#ifdef __STDC__
usnum_helem(
II_SCB		   *scb,
II_DATA_VALUE      *dvfrom,
II_DATA_VALUE      *dvhg)
#else
usnum_helem(scb, dvfrom, dvhg)
II_SCB		   *scb;
II_DATA_VALUE      *dvfrom;
II_DATA_VALUE      *dvhg;
#endif
{
    char	    *fromp;
    char	    *fromendp;
    char	    *top;
    char	    *toendp;


	IICVpkf(	dvfrom->db_data,
		(int)II_LEN_TO_PR_MACRO(dvfrom->db_length),
		(int)II_SCALE,
		(double *)dvhg->db_data);

	return( II_OK);

}

/*
** Name: usnum_hg_dtln()-	Get datatype and length histogram mapping
**				information for PNUM datatype.
**
** Description:
**      This function returns a structure containing datatypes and lengths for
**	the histogram mapping function on a PNUM datatype.
**
** Inputs:
**	scb				Pointer to a session control block.
**      adc_fromdv                      Ptr to II_DATA_VALUE describing the
**					data value from which the histogram
**					value will be made.
**	adg_hgdv			Ptr to II_DATA_VALUE that will be
**					updated to describe the result
**					histogram value.
** Outputs:
**	adg_hgdv
**	    .db_datatype		The datatype this datatype will
**					be mapped into to create a
**					histogram element. 
**	    .db_length			The length, in bytes, of the
**					data value resulting from the
**					"hg" mapping.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##      16-mar-92 (stevet)
##          Initial creation.
*/

II_STATUS
#ifdef __STDC__
usnum_hg_dtln(
II_SCB              *scb,
II_DATA_VALUE	    *fromdv,
II_DATA_VALUE	    *hgdv)
#else
usnum_hg_dtln(scb, fromdv, hgdv)
II_SCB              *scb;
II_DATA_VALUE	    *fromdv;
II_DATA_VALUE	    *hgdv;
#endif
{


    hgdv->db_datatype = II_FLOAT;
    hgdv->db_prec     = 0;
    hgdv->db_length   = 8;

    return(II_OK);
}

/*
** Name: usnum_dhmax() - Create the "default" maximum histogram element
**			 for the PNUM datatype.
**
** Description:
**	This routine creates a "default" maximum histogram element
**	for the PNUM datatype.  
**
** Inputs:
**	scb				Pointer to a session control block.
**	fromdv   			II_DATA_VALUE describing the data value
**					for which a maximum "default" histogram
**					value is to be made.
**      max_dvdhg                       Pointer to the II_DATA_VALUE
**					which describes the maximum default
**					histogram element to make.
**
** Outputs:
**      max_dvhg                        The maximum histogram element.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##          16-mar-92 (stevet)
##              Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_dhmax(
II_SCB              *scb,
II_DATA_VALUE	    *fromdv,
II_DATA_VALUE	    *max_dvdhg)
#else
usnum_dhmax(scb, fromdv, max_dvdhg)
II_SCB              *scb;
II_DATA_VALUE	    *fromdv;
II_DATA_VALUE	    *max_dvdhg;
#endif
{
    /* 
    ** 1000.0 is the default value for FLOAT8, which is the closest to PNUM
    */

     *(double *) max_dvdhg->db_data = 1000.0;

     return(II_OK);
}



/*
** Name: usnum_hmax() -  Create the maximum histogram element for PNUM.
**
** Description:
**	This routine creates a maximum histogram element for PNUM
**	datatype
**
** Inputs:
**	scb				Pointer to a session control block.
**	fromdv  			II_DATA_VALUE describing the data value
**					for which a maximum histogram
**					value is to be made.
**      max_dvhg                        Pointer to the II_DATA_VALUE
**					which describes the maximum
**					histogram element to make.
** Outputs:
**      max_dvhg                        The maximum histogram element.
**	    .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##          16-mar-92 (stevet)
##              Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_hmax(
II_SCB         	    *scb,
II_DATA_VALUE	    *fromdv,
II_DATA_VALUE	    *max_dvhg)
#else
usnum_hmax(scb, fromdv, max_dvhg)
II_SCB         	    *scb;
II_DATA_VALUE	    *fromdv;
II_DATA_VALUE	    *max_dvhg;
#endif
{

	*(double *) max_dvhg->db_data = FMAX;

	return( II_OK);
}


/*{
** Name: usnum_keybld	- Build an ISAM, BTREE, or HASH key from a PNUM
**                        data value.
**
** Description:
**      This routine constructs a key pair for use by the system.  The key pair
**	is built based upon the type of key desired.  A key pair consists of a
**	high-key/low-key combination.  These values represent the largest &
**	smallest values which will match the key, respectively.  In the case of
**	our PNUM data, these are typically simply the values.
**
**	Note that it is not the case that the input to this routine will always
**	be of type PNUM.  Therefore, we must call the conversion routine
**	to build the actual key.  We will know that this routine will only be
**	called if we have built a coercion from the input datatype to our type.
**
** Inputs:
**      scb                             Scb
**      key_block                       Pointer to key block data structure...
**	    .adc_kdv			Datavalue for which to build a key.
**	    .adc_opkey			Operator type for which key is being
**					built.
**	    .adc_lokey, .adc_hikey	Pointer to area for key.  If 0, do
**					not build key.
**
** Outputs:
**      *key_block                      Key block filled with following
**	    .adc_tykey                  Type key provided
**	    .adc_lokey                  if .adc_tykey is II_KEXACTKEY or
**					II_KLOWKEY, this is key built.
**	    .adc_hikey			if .adc_tykey is II_KEXACTKEY or
**					II_KHIGHKEY, this is the key built.
**
**	Returns:
**	    II_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##      16-mar-92 (stevet)
##          Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_keybld(
II_SCB		    *scb,
II_KEY_BLK	    *key_block)
#else
usnum_keybld(scb, key_block)
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
	us_error(scb, 0x201000, "Usnum_keybld: Invalid key type");
    }
    else 
    {
	if ((key_block->adc_tykey == II_KLOWKEY) ||
		(key_block->adc_tykey == II_KEXACTKEY))
	{
	    if (key_block->adc_lokey.db_data)
	    {
		result = usnum_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_lokey);
		/* If input value cannot be fitted just set it to max value */
		if( result == IICV_OVERFLOW)
		    result = usnum_minmaxdv(scb, NULL, &key_block->adc_lokey);
	    }
	}
	if ( (result == II_OK) &&
		((key_block->adc_tykey == II_KHIGHKEY) ||
		    (key_block->adc_tykey == II_KEXACTKEY)))
	{
	    if (key_block->adc_hikey.db_data)
	    {
		result = usnum_convert(scb, &key_block->adc_kdv,
					    &key_block->adc_hikey);
		/* If input value cannot be fitted just set it to max value */
		if( result == IICV_OVERFLOW)
		    result = usnum_minmaxdv(scb, NULL, &key_block->adc_hikey);
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

/*
** Name: usnum_lenchk() - Check validity of length and precision.
**
** Description:
**     Check to see if the length given in the CREATE TABLE statement
**     or internal defined length is valid.  
**
** Inputs:
**     scb		        Pointer to scb structure.
**     is_usr               	TRUE if the length, precision, and scale
**				provided in dv are 'user specified',
**				FALSE if 'internal'.
**     dv	   	        The data value to check.
**	   .db_length           Either the user specified length
**				(if is_usr is TRUE), or the
**				internal length (if is_usr
**				is FALSE) of the data value.
**     rdv		        Pointer to the II_DATA_VALUE used to
**				return info in.	 If this is a NULL
**				pointer, then only the check will be
**				done, and no info returned.
**
** Outputs:
**     scb		        Pointer to scb structure.
**     rdv		        If not supplied as NULL, and the check
**				succeeded, then...
**	   .db_length	        Either the user specified length (if
**				is_usr is FALSE), or the internal
**				length (if is_usr is TRUE) of the
**				data value.	 
**
**     Returns:
**	   II_STATUS
**
**     Exceptions:
**         none
**
** Side Effects:
**         none
**
## History:
##     23-jan-92 (stevet)
##	   Initial Creation.
##     24-apr-92 (stevet)
##         Added validation to make sure db_length is greater than II_SCALE.
*/

II_STATUS
#ifdef __STDC__
usnum_lenchk(
II_SCB          *scb,
int	    	is_usr,
II_DATA_VALUE	*dv,
II_DATA_VALUE	*rdv)	
#else
usnum_lenchk(scb, is_usr, dv, rdv)
II_SCB          *scb;
int	    	is_usr;
II_DATA_VALUE	*dv;
II_DATA_VALUE	*rdv;	
#endif
{		
    II_STATUS	result = II_OK;
    int       dt;

    if (rdv != NULL)
    {
	rdv->db_datatype = PNUM_TYPE;
	if( is_usr)
	    rdv->db_length = II_PREC_TO_LEN_MACRO(dv->db_length);      
	else
	    rdv->db_length = dv->db_length;
	
	if(rdv->db_length > II_PREC_TO_LEN_MACRO( II_MAX_NUMPREC) || 
	   rdv->db_length < 1 || rdv->db_length < II_SCALE)
	{
	    us_error( scb, 0x201010, 
	        "usnum_lenchk: Invalid length for PNUM datatype");
	    result = II_ERROR; 
	}
    }

    return( result);

}

/*
** Name: usnum_minmaxdv() -    Actually builds the min and max value or length
**			       for the PNUM datatype.
**
** Description:
**	This function builds the min and max values and lenghts for the
**      PNUM datatype.
**
** Inputs:
**	scb				Pointer to a session control block.
**      mindv 	           		Pointer to II_DATA_VALUE for the `min'.
**					If this is NULL, `min' processing will
**					be skipped.
**      maxdv   			Pointer to II_DATA_VALUE for the `max'.
**					If this is NULL, `max' processing will
**					be skipped.
**
** Outputs:
**      mindv    			If this was supplied as NULL, `min'
**					processing will be skipped.
**		.db_length		If this was supplied as II_LEN_UNKNOWN,
**					then the `min' valid internal length for
**					this datatype will be returned here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					II_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `min'
**					non-null value for PNUM datatype
**					will be built and placed at the 
**					location pointed to by .db_data.
**    maxdv 			        If this was supplied as NULL, `max'
**					processing will be skipped.
**		.db_length		If this was supplied as II_LEN_UNKNOWN,
**					then the `max' valid internal length 
**					for this datatype will be returned 
**                                      here.
**		.db_data		If this was supplied as NULL, or if the
**					.db_length field was supplied as
**					II_LEN_UNKNOWN, nothing will be
**					returned here.  Otherwise, the `max'
**					non-null value for PNUM will be
**					built and placed at the location
**					pointed to by .db_data.
**
**      Returns:
**
**         II_STATUS
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##  	03-20-92 (stevet)
##  	    Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_minmaxdv(
II_SCB		    *scb,
II_DATA_VALUE	    *mindv,
II_DATA_VALUE	    *maxdv)
#else
usnum_minmaxdv(scb, mindv, maxdv)
II_SCB		    *scb;
II_DATA_VALUE	    *mindv;
II_DATA_VALUE	    *maxdv;
#endif
{
    II_STATUS		db_stat = II_OK;
    II_DT_ID		dt;
    int			len;
    int			i;
    char  		*pk;
    char		*data;


    /* Process the `min' */
    /* ----------------- */
    if (mindv != NULL)
    {
	dt   = mindv->db_datatype;
	len  = mindv->db_length;
	data = (char *)mindv->db_data; 

	if (len == II_LEN_UNKNOWN)
	{
	    /* Set the `min' length */
	    /* -------------------- */
		mindv->db_length = 1;

	}
	else if (data != NULL)
	{
		/* Build the `min' value */
	    	/* --------------------- */

	    	/* First, make sure supplied length is valid for this datatype */
	    	/* ----------------------------------------------------------- */
	    	if (db_stat = usnum_lenchk(scb, 0, mindv, NULL))
			return(db_stat);

	    	/* Now, we can build the `min' value */
	    	/* --------------------------------- */
	        pk = (char *)data;

		/* fill first byte */
    	    	*pk = 0x99;

		/* fill intermediate bytes if there are any */
		for(i=0; i+2 < len; i++)
		{
		    *(pk+1+i) = 0x99;
		}

		/* place minus sign */
		pk[len-1] = 0x90 | 0xd;
	}
    }


    /* Process the `max' */
    /* ----------------- */
    if (maxdv != NULL)
    {		
	dt   = maxdv->db_datatype;
	len  = maxdv->db_length;
	data = (char *)maxdv->db_data; 

	if (len == II_LEN_UNKNOWN)
	{
		maxdv->db_length = II_PREC_TO_LEN_MACRO( II_MAX_NUMPREC);

	}
	else if (data != NULL)
	{
	     	/* Build the `max' value */
	     	/* --------------------- */

	    	/* First, make sure supplied length is valid for this datatype */
	    	/* ----------------------------------------------------------- */
	    	if (db_stat = usnum_lenchk(scb, 0, maxdv, NULL))
		     return(db_stat);

		/* Now, we can build the `max' value */
	    	/* --------------------------------- */
		pk = (char *)data;

		/* fill first byte */
	    	*pk = 0x99;

		/* fill intermediate bytes if there are any */
		for(i=0; i+2 < len; i++)
		{
		    *(pk+1+i) = 0x99;	
		}


		/* place plus sign */
		pk[len-1] = 0x90 | 0xc;
	}
    }

    return(II_OK);
}

/*
** Name: usnum_tmlen() - Provide default and worst-case lengths for display
**		         PNUM datatype.
**
** Description:
**      This routine is used to retrieve both the default width (smallest
**	display field width) and the worst-case width for the PNUM datatype.
**
** Inputs:
**      scb                             Ptr to a session control block struct.
**	adc_dv				Ptr to a II_DATA_VALUE struct holding
**					datatype and length in question.
** Outputs:
**	adc_defwid			The length, in bytes, of the default
**					print field size for the given datatype
**					and length.
**	adc_worstwid			The length, in bytes, of the worst-case
**					print field size for the given datatype
**					and length.  That is, what is the
**					largest possible print field needed for
**					any data value with the given datatype
**					and length.
**
**	Returns:
**	    II_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##          16-mar-92 (stevet)
##              Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_tmlen(
II_SCB              *scb,
II_DATA_VALUE	    *dv,
short               *defwid,
short               *worstwid)
#else
usnum_tmlen(scb, dv, defwid, worstwid)
II_SCB              *scb;
II_DATA_VALUE	    *dv;
short               *defwid;
short               *worstwid;
#endif
{
	int	pr = II_LEN_TO_PR_MACRO(dv->db_length);
	int	sc = II_SCALE;
	    
	*defwid   = 
	*worstwid = II_PS_TO_PRN_MACRO(pr, sc);
    
	return( II_OK);
}


/*
** Name: usnum_tmcvt() - Convert data value to terminal monitor display
**		         format.
**
** Description:
**      This routine is used to convert a data value into its terminal monitor
**	display format.
**
** Inputs:
**      scb                             Ptr to a session control struct.
**	dv				Ptr to a II_DATA_VALUE struct holding
**					data value to be converted.
**	tmdv    			Ptr to a II_DATA_VALUE struct for
**					the result.
**
** Outputs:
**	dv			        Ptr to a II_DATA_VALUE struct holding
**					the result.
**	    .db_data			The result will be placed in the buffer
**					pointed to by this. 
**	outlen		        	Number of characters used for the
**					result.
**
**	Returns:
**          II_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
## History:
##          16-mar-92 (stevet)
##              Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_tmcvt(
II_SCB              *scb,
II_DATA_VALUE	    *dv,
II_DATA_VALUE	    *tmdv,
int		    *outlen)
#else
usnum_tmcvt(scb, dv, tmdv, outlen)
II_SCB              *scb;
II_DATA_VALUE	    *dv;
II_DATA_VALUE	    *tmdv;
int		    *outlen;
#endif
{
    II_STATUS	status;
    char	*f = (char *)dv->db_data;
    char	*t = (char *)tmdv->db_data;
    int		pr = II_LEN_TO_PR_MACRO(dv->db_length);
    int		sc = II_SCALE;
    char	decimal = '.';

    /* now convert to ascii and put resulting length in outlen */

    status = IICVpka(f, pr, sc, decimal, II_PS_TO_PRN_MACRO(pr, sc),
	    sc, IICV_PKLEFTJUST, t, outlen);
    if( status != II_OK && status != IICV_TRUNCATE)
    {
	status = II_ERROR;
	us_error(scb, 0x201020, "numeric_tmcvt error");
    }	

    return (status);
}

/*
** Name: usnum_dhmin() - Create the "default" minimum histogram element
**			 for PNUM datatype.
**
** Description:
**	This routine creates a "default" minimum histogram element
**	for a PNUM datatype.
**
** Inputs:
**	scb				Pointer to a session control block.
**	fromdv    			II_DATA_VALUE describing the data value
**					for which a minimum "default" histogram
**					value is to be made.
**      min_dvdhg                       Pointer to the II_DATA_VALUE
**					which describes the minimum default
**					histogram element to make.
**
** Outputs:
**      min_dvhg                       The minimum histogram element.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##          16-mar-92 (stevet)
##               Initial Creation.
*/

II_STATUS
#ifdef __STDC__
usnum_dhmin(
II_SCB              *scb,
II_DATA_VALUE	    *fromdv,
II_DATA_VALUE	    *min_dvdhg)
#else
usnum_dhmin(scb, fromdv, min_dvdhg)
II_SCB              *scb;
II_DATA_VALUE	    *fromdv;
II_DATA_VALUE	    *min_dvdhg;
#endif
{
	
	*(double *) min_dvdhg->db_data = 0.0;

    	return(II_OK);
}



/*
** Name: usnum_hmin() -  Create the minimum histogram element for PNUM
**			 datatype.
**
** Description:
**      This routine creates a minimum histogram element for PNUM
**	datatype.
**
** Inputs:
**	scb				Pointer to a session control block.
**	fromdv   			II_DATA_VALUE describing the data value
**					for which a minimum histogram
**					value is to be made.
**      min_dvhg                        Pointer to the II_DATA_VALUE
**					which describes the minimum
**					histogram element to make.
**
** Outputs:
**      min_dvhg                        The minimum histogram element.
**	    .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
*/

II_STATUS
#ifdef __STDC__
usnum_hmin(
II_SCB              *scb,
II_DATA_VALUE	    *adc_fromdv,
II_DATA_VALUE	    *adc_min_dvhg)
#else
usnum_hmin(scb, adc_fromdv, adc_min_dvhg)
II_SCB              *scb;
II_DATA_VALUE	    *adc_fromdv;
II_DATA_VALUE	    *adc_min_dvhg;
#endif
{
	*(double *) adc_min_dvhg->db_data = FMIN;

	return( II_OK);
}


/*{
** Name: usnum_valchk    - Check for valid values
**
** Description:
**      This routine checks for valid values.  In fact, all values are valid, 
**      so this routine just returns OK.
**
** Inputs:
**      scb                             II_SCB
**      dv                              Data value in question.
**
** Outputs:
**      None.
**
**      Returns:
**          II_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
## History:
##          16-mar-92 (stevet)
##		Initial Creation
*/
II_STATUS
#ifdef __STDC__
usnum_valchk(
II_SCB             *scb,
II_DATA_VALUE      *dv)
#else
usnum_valchk(scb, dv)
II_SCB             *scb;
II_DATA_VALUE      *dv;
#endif
{

    return(II_OK);
}

/*
** Name: usnum_convert() - Convert various datatypes to/from PNUM datatype.
**
**
** Descriptions:
**  	This routine supports the convertion of data values in various
**  	types to/from PNUM datatypes.  The datatypes that are support
**  	in this routines are II_CHAR, II_VARCHAR, II_C, II_TEXT,
**      II_FLOAT and II_INTEGER.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv                              Ptr to II_DATA_VALUE to be coerced.
**	    .db_datatype		Type of data to be coerced.
**	    .db_length			Length of data to be coerced.
**	    .db_data			Ptr to actual data to be coerced.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      24-apr-92 (stevet)
##          Added I1 support for integers.
##      16-aug-1993 (stevet)
##          Fixed blank padding for PNUM->CHAR which should depend on
##          rdv->db_length value.
##      11-oct-1993 (markm)
##          Fixed assigment typo in usnum_convert() that resulted in
##          the variable "status" not being updated if there was a
##          float passed with an incorrect length.
## 	11-apr-1995 (wonst02)
##	    Bug #68008
## 	    Handle internal DECIMAL type, which looks just like
## 	    the PNUM user-defined type.
##	20-apr-1999 (somsa01)
##	    Pass in the result of II_S_DECODE_MACRO() as the source scale
##	    into IICVpkpk() instead of II_SCALE.
##  29-Aug-2004 (kodse01)
##      BUG 112917
##      Handled i8 to PNUM conversion.
#	18-oct-2006 (dougi)
##	    Add i8 support for PNUM coercions.
*/

II_STATUS
#ifdef __STDC__
usnum_convert(
II_SCB		    *scb,
II_DATA_VALUE	    *dv,
II_DATA_VALUE	    *rdv)
#else
usnum_convert(scb, dv, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS	    status=II_OK, ret_status=II_OK;
    char   	    *cp;
    int		    val;
    i8		    val8;
    short	    sval;
    float	    f4val;
    double	    fval;
    short	    count;
    II_VLEN	    *vlen;
    char	    temp[II_MAX_NUMLEN];
    char	    decimal = '.';


    switch (dv->db_datatype)
    {
      case II_INTEGER:
        switch (dv->db_length)
        {
          case 1:
    	    val = *dv->db_data;
            break;

          case 2:
            I2ASSIGN_MACRO(*dv->db_data, sval);
	    val = sval;
            break;

          case 4:
            I4ASSIGN_MACRO(*dv->db_data, val);
            break;

          case 8:
            I8ASSIGN_MACRO(*dv->db_data, val8);
            break;

	  default:
    	    status = II_ERROR;
        }

    if (dv->db_length == 8)
    {
        ret_status = IICVl8pk(val8, II_LEN_TO_PR_MACRO(rdv->db_length),
                   II_SCALE, rdv->db_data);
    }
    else
    {
	    ret_status = IICVlpk(val, II_LEN_TO_PR_MACRO(rdv->db_length),
	    		   II_SCALE, rdv->db_data);
    }
	if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    status = ret_status;
        break;

      case II_DECIMAL:
	ret_status = IICVpkpk( dv->db_data,
				II_LEN_TO_PR_MACRO(dv->db_length),
				II_PREC_TO_SC_MACRO(dv->db_prec),
				II_LEN_TO_PR_MACRO(rdv->db_length),
				II_SCALE,
				rdv->db_data);
	if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
		status = ret_status;
	break;

      case PNUM_TYPE:
	switch( abs(rdv->db_datatype))
	{
	     case II_FLOAT:
		ret_status = IICVpkf( dv->db_data,
				    II_LEN_TO_PR_MACRO(dv->db_length),
				    II_SCALE,
				    rdv->db_data);
		if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
		     status = ret_status;
		break;
	     case II_DECIMAL:
	     case PNUM_TYPE:
		ret_status = IICVpkpk( dv->db_data,
				    II_LEN_TO_PR_MACRO(dv->db_length),
				    II_S_DECODE_MACRO(dv->db_prec),
				    II_LEN_TO_PR_MACRO(rdv->db_length),
				    II_SCALE,
				    rdv->db_data);
		 if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
		     status = ret_status;
		break;
      	     case II_CHAR:
      	     case II_C:
		{
    		   char *f = (char *)dv->db_data;
    		   char *t = (char *)rdv->db_data;
	           int	pr = II_LEN_TO_PR_MACRO( dv->db_length);
		   int  sc = II_SCALE;
		   int  outlen;

		   ret_status = IICVpka(f, pr, sc, decimal, 
				      II_PS_TO_PRN_MACRO(pr, sc),
				      sc, IICV_PKLEFTJUST, t, &outlen);
		   /* padding blanks */
		   for(t=t+outlen; outlen < rdv->db_length;
		       outlen++, t++)
		       *t=' ';
		   if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    	        status = ret_status;
 		   break;
		}
      	     case II_LONGTEXT:
	     case II_TEXT:
      	     case II_VARCHAR:
		{
    		   char *f = (char *)dv->db_data;
    		   char *t = (char *)rdv->db_data;
	           int	pr = II_LEN_TO_PR_MACRO( dv->db_length);
		   int  sc = II_SCALE;
		   int  outlen;
		   short slen; 
		   ret_status = IICVpka(f, pr, sc, decimal, 
				      II_PS_TO_PRN_MACRO(pr, sc),
				      sc, IICV_PKLEFTJUST, t+2, &outlen);
		   slen = outlen;
		   I2ASSIGN_MACRO( slen, *rdv->db_data);
		   if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    	        status = ret_status;
 		   break;
		}

	     default:
		status = II_ERROR;
	}
	break;

      case II_FLOAT:
        if (dv->db_length == 8)
	    F8ASSIGN_MACRO( *dv->db_data, fval);
    	else
    	{
    	    if( dv->db_length == 4 )
    	    {
    	    	F4ASSIGN_MACRO( *dv->db_data, f4val);
    	    	fval = f4val;
    	    }
    	    else
    	    {
    	    	status = II_ERROR;
    	    	break;
    	    }
    	}
	ret_status = IICVfpk(fval,
			   II_LEN_TO_PR_MACRO(rdv->db_length),
			   II_SCALE,
			   rdv->db_data);
	if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    status = ret_status;
        break;
	
      case II_CHAR:
      case II_C:

        cp = temp;
        byte_copy(dv->db_data, dv->db_length, cp);
        temp[dv->db_length] = '\0';

	if( rdv->db_length == II_LEN_UNKNOWN)
	    rdv->db_length = II_MAX_NUMLEN;	
        ret_status = IICVapk(temp, decimal, II_LEN_TO_PR_MACRO(rdv->db_length),
		    II_SCALE, rdv->db_data);
	if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    status = ret_status;
        break;

      case II_VARCHAR:
      case II_TEXT:
      case II_LONGTEXT:

        cp = temp;
        I2ASSIGN_MACRO( *dv->db_data, count);
        vlen = ((II_VLEN *) dv->db_data);
	byte_copy( vlen->vlen_array, count, cp);
        cp[count] = '\0';

        ret_status = IICVapk(temp, decimal, II_LEN_TO_PR_MACRO(rdv->db_length),
                           II_SCALE, rdv->db_data);
	if( ret_status != II_OK && ret_status != IICV_TRUNCATE)
	    status = ret_status;	    
        break;

      default:
	   status = II_ERROR;
    }

    if( status != II_OK)
	us_error( scb, 0x201030, "Error in usnum_convert");
    return(status);
}

/*
** Name: usnum_add() - Adding two PNUM numbers.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand.
**      dv2                             Ptr to II_DATA_VALUE of the second 
**                                      operand.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      28-apr-92 (stevet)
##          Clean up error trapping.
*/
II_STATUS
#ifdef __STDC__
usnum_add(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_add(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS  status=II_OK;
    int   prec, scale;
    char  temp[II_MAX_NUMLEN+1];

    II_DATA_VALUE  tmp;


    status = IIMHpkadd(	dv1->db_data,
		II_LEN_TO_PR_MACRO(dv1->db_length),
		II_SCALE,
		dv2->db_data,
		II_LEN_TO_PR_MACRO(dv2->db_length),
		II_SCALE,
		temp,
		(int *)&prec,
		(int *)&scale);


    if( status == II_OK ||  status == IICV_TRUNCATE)
    {
	/* The result may not be in the right precision and scale */
	tmp.db_datatype = dv1->db_datatype;
	tmp.db_length = II_PREC_TO_LEN_MACRO(prec);
	tmp.db_data = (char *)temp;

	status = usnum_convert( scb, &tmp, rdv);
    }

    if( status != II_OK && status != IICV_TRUNCATE)
	us_error( scb, 0x201051, "Error in usnum_add");
    else
	status = II_OK;

    return(status);
}

/*
** Name: usnum_sub() - Substracting PNUM numbers.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand.
**      dv2                             Ptr to II_DATA_VALUE of the second 
**                                      operand.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      28-apr-92 (stevet)
##          Clean up error trapping.
*/
II_STATUS
#ifdef __STDC__
usnum_sub(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_sub(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS status=II_OK;
    int       prec, scale;
    char    temp[II_MAX_NUMLEN+1];

    II_DATA_VALUE  tmp;


    status = IIMHpksub(	dv1->db_data,
		II_LEN_TO_PR_MACRO(dv1->db_length),
		II_SCALE,
		dv2->db_data,
		II_LEN_TO_PR_MACRO(dv2->db_length),
		II_SCALE,
		temp,
		(int *)&prec,
		(int *)&scale);

    if( status == II_OK ||  status == IICV_TRUNCATE)
    {
	/* The result may not be in the right precision and scale */
	tmp.db_datatype = dv1->db_datatype;
	tmp.db_length = II_PREC_TO_LEN_MACRO(prec);
	tmp.db_data = (char *)temp;

	status = usnum_convert( scb, &tmp, rdv);
    }

    if( status != II_OK && status != IICV_TRUNCATE)
	us_error( scb, 0x201052, "Error in usnum_sub");
    else
	status = II_OK;

    return(status);
}

/*
** Name: usnum_mul() - Multiply two PNUM numbers.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand.
**      dv2                             Ptr to II_DATA_VALUE of the second 
**                                      operand.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      28-apr-92 (stevet)
##          Clean up error trapping.
*/
II_STATUS
#ifdef __STDC__
usnum_mul(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_mul(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS   status=II_OK;
    int     	prec, scale;
    char  	temp[II_MAX_NUMLEN+1];

    status = IIMHpkmul(	dv1->db_data,
		II_LEN_TO_PR_MACRO(dv1->db_length),
		II_SCALE,
		dv2->db_data,
		II_LEN_TO_PR_MACRO(dv2->db_length),
		II_SCALE,
		temp,
		(int *)&prec,
		(int *)&scale);

    if( status == II_OK ||  status == IICV_TRUNCATE)
    {
	/* The result may not be in the right precision and scale */
	status = IICVpkpk( temp,
			 prec,
			 scale,
			 II_LEN_TO_PR_MACRO(rdv->db_length),
		         II_SCALE,
		         rdv->db_data);
    }

    if( status != II_OK && status != IICV_TRUNCATE)
	us_error( scb, 0x201053, "Error in usnum_mul");
    else
	status = II_OK;

    return(status);
}

/*
** Name: usnum_div() - Dividing two PNUM numbers.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand.
**      dv2                             Ptr to II_DATA_VALUE of the second 
**                                      operand.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      28-apr-92 (stevet)
##          Clean up error trapping.
*/
II_STATUS
#ifdef __STDC__
usnum_div(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_div(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS   status=II_OK;
    int     	prec, scale;
    char  	temp[II_MAX_NUMLEN+1];

    status = IIMHpkdiv(	dv1->db_data,
		II_LEN_TO_PR_MACRO(dv1->db_length),
		II_SCALE,
		dv2->db_data,
		II_LEN_TO_PR_MACRO(dv2->db_length),
		II_SCALE,
		temp,
		(int *)&prec,
		(int *)&scale);

    if( status == II_OK ||  status == IICV_TRUNCATE)
    {
	status = IICVpkpk( temp,
			 prec,
			 scale,
			 II_LEN_TO_PR_MACRO(rdv->db_length),
		         II_SCALE,
		         rdv->db_data);	
    }

    if( status != II_OK && status != IICV_TRUNCATE)
	us_error( scb, 0x201054, "Error in usnum_div");
    else
	status = II_OK;

    return(status);
}

/*
** Name: usnum_numeric() - Convert formatted character string to PNUM
**                         data value.
**
** Description:
**  	Convert character string to PNUM data values using the
**      format string.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is in II_CHAR type.
**      dv2                             Ptr to II_DATA_VALUE in II_CHAR type
**                                      which holds the format string.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##      28-apr-92 (stevet)
##          Fixed problem with handling negative fractional values.
*/
II_STATUS
#ifdef __STDC__
usnum_numeric(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_numeric(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS  status=II_OK;
    char  temp[II_MAX_NUMLEN+1];
    char  temp2[II_MAX_NUMLEN+1];
    char  int_num[II_MAX_NUMLEN+1];
    char  dec_num[II_MAX_NUMLEN+1];
    NUM_FORMAT format;
    char  *fp;
    char  *f = (char *)dv1->db_data;
    char  *s = (char *)dv2->db_data;
    char  *r = (char *)rdv->db_data;
    char  *t, *it;
    int	    pr = II_LEN_TO_PR_MACRO( rdv->db_length);
    int     sc = II_SCALE;
    int     outlen=0;
    int     i=0,len, neg;
    long    int_part=0, numer=0, denom=0;
    double  dec_part=0;
    II_DATA_VALUE  tmp;

    /* Set default values */
    strcpy( format.curr, "$");
    format.decimal= '.';
    format.thousand=',';
    format.fraction='\0';
    format.sc=II_SCALE;

    /* Parse format string */

    usnum_format( &format, dv2->db_length, s);

    /* See if the input data is a fraction */

    if( format.fraction != '\0')
    {
	len = dv1->db_length;
	/* find integer first */
    	neg = FALSE;
	while( *f != format.decimal && len > 0)
	{
    	    if( *f == '-')
    	    	neg = TRUE;
	    if( *f >= '0' && *f <= '9')
		int_part = int_part*10 + (*f - '0');
	    f++; len--;
	}
	/* get numerator */
	while( *f != format.fraction && len > 0)
	{
	    if( *f >= '0' && *f <= '9')
		numer = numer*10 + (*f - '0');
	    f++; len--;
	}
	/* get denominator */
	while( len > 0)
	{
	    if( *f >= '0' && *f <= '9')
		denom = denom*10 + (*f - '0');
	    f++; len--;
	}
	/* Convert number to numeric */

	dec_part = numer;
	dec_part /= denom;
	dec_part += int_part;

	if( neg == TRUE)
	    dec_part = -dec_part;

	status = IICVfpk(dec_part, pr, sc, rdv->db_data);
    }
    else
    {
	for( i=dv1->db_length; i>0; i--, f++)
	{
	    if( (*f >= '0' && *f <= '9') || *f == format.decimal || 
    	    	 *f == '-')
		temp[outlen++] = *f;
	}
	temp[outlen] = '\0';
    	if( format.sc != II_SCALE)
    	{
	    status = IICVapk(temp,
		   format.decimal,
		   II_LEN_TO_PR_MACRO(rdv->db_length),
		   format.sc,
		   temp2);
    	    if( status == II_OK || status == IICV_TRUNCATE)
    	    	status = IICVpkpk( temp2, pr, format.sc, pr, II_SCALE, 
    	    	    	     rdv->db_data);
    	}
    	else
    	{
	    status = IICVapk(temp,
		   format.decimal,
		   II_LEN_TO_PR_MACRO(rdv->db_length),
		   II_SCALE,
		   rdv->db_data);
    	}

    }

    if( status != II_OK && status != IICV_TRUNCATE)
	us_error( scb, 0x201100, "Error in usnum_numeric");
    else
    	status = II_OK;
    return(status);
}

/*
** Name: usnum_numeric_1arg() - Convert character string to PNUM
**                              data value using the default format.
**
** Description:
**  	Convert character string to PNUM data values using the
**      default format string. This is a wrapper routine that set up
**      the format string argument (dv2) and call usnum_numeric() to 
**      do that actual work.  This function will be called when user
**  	calls the numeric() function with only one argument.
**
**  	Users can customize this routine by replacing the empty format
**      string with the format string that is most appropriated  for their 
**      applications.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is in II_CHAR type.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					decimal value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting decimal value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_numeric_1arg(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *rdv)
#else
usnum_numeric_1arg(scb, dv1, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *rdv;
#endif
{
    II_DATA_VALUE   dv2;
    char    	    def[20];

    strcpy( def, " ");
    dv2.db_length = strlen(def);
    dv2.db_data = (char *)def;
    dv2.db_datatype = II_CHAR;

    return( usnum_numeric(scb, dv1, &dv2, rdv));
}

/*
** Name: usnum_currency() - Convert PNUM data to currency.
**
** Description:
**  	Convert PNUM data values to currency using given 
**      format string.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is a PNUM datatype.
**      dv2                             Ptr to II_DATA_VALUE which holds 
**                                      the format string.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					currency value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting currency value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_currency(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_currency(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{    
    II_STATUS  status=II_OK;
    char  temp[II_MAX_NUMLEN+1];
    char  temp2[II_MAX_NUMLEN+1];
    char  int_part[II_MAX_NUMLEN+1];
    char  currency[II_MAX_NUMLEN+1];
    NUM_FORMAT format;
    char  *fp;
    char  *f = (char *)dv1->db_data;
    char  *s = (char *)dv2->db_data;
    char  *r = (char *)rdv->db_data;
    char  *t, *it;
    int	    pr = II_LEN_TO_PR_MACRO( dv1->db_length);
    int     outlen, intlen;
    int     i=0;
    short   count;
    II_VLEN    *vlen;
    /* Parse format string */

    /* Default values */
    strcpy( format.curr, "$");
    format.decimal= '.';
    format.thousand=',';
    format.sc=II_SCALE;

    usnum_format( &format, dv2->db_length, s);

    if( format.sc != II_SCALE)
    {
	status = IICVpkpk( dv1->db_data, pr, II_SCALE, 
			 pr, format.sc, temp2);
	f = temp2;
    }
	

    /* Convert decimal to string */

    status = IICVpka(f, pr, format.sc, format.decimal, 
		   II_PS_TO_PRN_MACRO(pr, format.sc),
		   format.sc, 0, temp, &outlen);
    if (status && status != IICV_TRUNCATE)
	status = II_ERROR;
    else
    {

	t = temp;
	it = int_part;
	/* Get rid of leading blanks */
	for(t = temp; *t == ' ';t++, outlen--)
	    ;

	/* Calcalate the size of integer part */
	intlen = (format.sc > 0) ? outlen - format.sc - 1 : outlen - format.sc;
	/* Take care of the minus sign first */
	if( *t == '-')
	{
	    *it = *t;
	    it++;
	    t++;
	    outlen--;
	    intlen--;
	}

	
	/* Get integer part */
	for( i=intlen; *t != format.decimal && intlen > 0; 
	    t++, outlen--, intlen--, it++)
	{
	    /* Add thouand separator */
	    if( intlen == 3 * (intlen/3) && format.thousand != '\0' && 
	        intlen != i)
	    {
		*it=format.thousand;
		it++;
	    }
	    *it = *t;
	}
	*it='\0';


	fp = (char *)format.curr;
	
	/* Currency sign at the end of number */
	if( *fp == '-')
	{
	    fp++;
	    sprintf( currency, "%s%s%s", int_part, t, fp);
	}
	else
	    sprintf( currency, "%s%s%s", fp, int_part, t);
	
	count = min( strlen(currency), rdv->db_length);
        I2ASSIGN_MACRO( count, *rdv->db_data);
        vlen = ((II_VLEN *) rdv->db_data);
	
	byte_copy( currency, count, vlen->vlen_array);

    }

    if( status != II_OK)
	us_error( scb, 0x201110, "Error in usnum_currecny");
    return(status);
}

/*
** Name: usnum_currency_1arg() - Convert PNUM data to currency using the 
**  	    	    	         default formats.
**
** Description:
**  	Convert PNUM data values to currency using the
**      default format string. This is a wrapper routine that set up
**      the format string argument (dv2) and call usnum_currency() to 
**      do that actual work.  This function will be called when user
**  	calls the currency() function with only one argument.
**
**  	Users can customize this routine by replacing the empty format
**      string with the format string that is most appropriated  for their 
**      applications.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is a PNUM datatype.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					currency value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting currency value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_currency_1arg(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *rdv)
#else
usnum_currency_1arg(scb, dv1, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *rdv;
#endif
{ 
    II_DATA_VALUE   dv2;
    char    	    def[20];

    strcpy( def, " ");
    dv2.db_length=strlen(def);
    dv2.db_data=(char *)def;
    dv2.db_datatype=II_CHAR;
    return( usnum_currency( scb, dv1, &dv2 , rdv));
   
}


/*
** Name: usnum_fraction() - Convert PNUM data to fraction.
**
** Description:
**  	Convert PNUM data values to fraction using the given 
**      format string. 
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is a PNUM datatype.
**      dv2                             Ptr to II_DATA_VALUE which holds the 
**                                      formatted string.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					fraction value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting fraction value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_fraction(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *dv2,
II_DATA_VALUE	    *rdv)
#else
usnum_fraction(scb, dv1, dv2, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *dv2;
II_DATA_VALUE	    *rdv;
#endif
{
    II_STATUS  status=II_OK;
    NUM_FORMAT format;
    char  temp[II_MAX_NUMLEN+1];
    char  int_part[II_MAX_NUMLEN+1];
    char  fraction[II_MAX_NUMLEN+1];
    char  *fp;
    char  decimal = '.';
    char  *f = (char *)dv1->db_data;
    char  *s = (char *)dv2->db_data;
    char  *r = (char *)rdv->db_data;
    char  *t, *it;
    int	    pr = II_LEN_TO_PR_MACRO( dv1->db_length);
    int     sc = II_SCALE;
    int     outlen, neg;
    int     i=0, numer=0, denum=1;
    short   count;
    II_VLEN    *vlen;
    
    /* Set default values */
    strcpy( format.curr, "$");
    format.decimal= ' ';
    format.thousand=',';
    format.fraction='/';


    /* Parse format string */

    usnum_format( &format, dv2->db_length, s);

    /* Convert decimal to string */

    status = IICVpka(f, pr, sc, decimal, II_PS_TO_PRN_MACRO(pr, sc),
		   sc, 0, temp, &outlen);
    if (status)
	status = II_ERROR;
    else
    {
	t = temp;
	it = int_part;
	/* Get rid of leading blanks */
	for(t = temp; *t == ' ';t++, outlen--)
	    ;

    	if( *t == '-')
    	{
    	    neg = TRUE;
    	    t++; outlen--;
    	}
    	else
    	    neg = FALSE;

    	/* Get rid of leading 0 */
    	while( *t == '0')
    	{
    	    t++;
    	    outlen--;
    	}

	/* Get integer part */
	for( it = int_part, *it= *t; *t != '.'; t++, outlen--, it++)
	    *it = *t;
	*it='\0';

	/* Get decimal part */
	while( --outlen)
	{
	    t++;
	    numer = numer*10 + (*t - '0');
	    denum = denum*10;
	}  

	/* normalize */
	for(i=0; prime[i] != -1; i++)
	{
	    if( numer == prime[i] * (numer/prime[i]) &&
	       denum == prime[i] * (denum/prime[i]))
	    {
		numer = numer/prime[i];
		denum = denum/prime[i];
		/* 
		** Process the same prime number again so that we can take 
		** care of the square of prime[i].
		*/
		i--;
	    }
	}
		
	/* Compose the output */
    	
    	if( numer == 0)
    	{
    	    if( neg == TRUE)
		sprintf(fraction, "-%s", int_part);
	    else
	    	sprintf(fraction, "%s", int_part);
    	}
    	else
    	{
    	    if( neg == TRUE)
	    {
		if( strlen(int_part) != 0)
		    sprintf(fraction, "-%s%c%d%c%d", int_part, format.decimal, 
			    numer, format.fraction, denum);
	        else
		    sprintf(fraction, "-%d%c%d", 
			    numer, format.fraction, denum);
	    }
	    else
	    {
		if( strlen(int_part) != 0)
		    sprintf(fraction, "%s%c%d%c%d", int_part, format.decimal, 
			    numer, format.fraction, denum);
		else
		    sprintf(fraction, "%d%c%d", 
			    numer, format.fraction, denum);	
	    }
    	}

	count = min( strlen(fraction), rdv->db_length);
        I2ASSIGN_MACRO( count, *rdv->db_data);
        vlen = ((II_VLEN *) rdv->db_data);
	
	byte_copy( fraction, count, vlen->vlen_array);

    }
	
    if( status != II_OK)
	us_error( scb, 0x201110, "Error in usnum_fraction");
    return(status);
}

/*
** Name: usnum_fraction_1arg() - Convert PNUM data to fraction using the 
**  	    	    	         default formats.
**
** Description:
**  	Convert PNUM data values to fraction using the
**      default format string. This is a wrapper routine that set up
**      the format string argument (dv2) and call usnum_fraction() to 
**      do that actual work.  This function will be called when user
**  	calls the fraction() function with only one argument.
**
**  	Users can customize this routine by replacing the empty format
**      string with the format string that is most appropriated  for their 
**      applications.
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is a PNUM datatype.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					currency value.
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting currency value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
*/
II_STATUS
#ifdef __STDC__
usnum_fraction_1arg(
II_SCB		    *scb,
II_DATA_VALUE	    *dv1,
II_DATA_VALUE	    *rdv)
#else
usnum_fraction_1arg(scb, dv1, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *dv1;
II_DATA_VALUE	    *rdv;
#endif
{
    II_DATA_VALUE   dv2;
    char    	    def[20];

    strcpy( def, " ");
    dv2.db_length=strlen(def);
    dv2.db_data=(char *)def;
    dv2.db_datatype=II_CHAR;

    return( usnum_fraction( scb, dv1, &dv2, rdv));
}


/*
** Name: usnum_format() - Internal routine that parses the format string
**
** Description:
**  	Internal routine that parses the format string and assigns the
**      values to the num_format structure.
**
** Inputs:
**	format				Pointer to the structure that holds 
**                                      all format options.
**      len                             Length of the formatted string.
**      string                          The format string with the following
**                                      options:
**                                        '-f' to indicate fraction sparator
**                                        '-d' to indicate decimal separator
**                                        '-c' to indicate currency sign
**                                        '-t' to indicate thousand separator
**                                        '-s' to indicate scale
** Outputs:
**	format
**	    .decimal                    Decimal point separator
**          .thousand                   Thousand separator
**          .fraction                   Fraction separator
**          .curr                       Currency sign
**  	    .scale                      Scale, number of digits after decimal
**  	    	    	    	    	point.
**	Returns:
**	    II_STATUS
**
##  History:
##	16-mar-92 (stevet)
##	    Initial Creation.
##	01-apr-1999 (kinte01)
##	    Add a return(status) to satisfy DEC C 5.6 compiler warnings. 
*/
static II_STATUS
#ifdef __STDC__
usnum_format( 
NUM_FORMAT           *format,
int                  len,
char        	     *string)
#else
usnum_format( format, len, string)
NUM_FORMAT           *format;
int                  len;
u_char        	     *string;
#endif
{
    II_STATUS  status=II_OK;
    char       separator='-';
    int        i;

    /* Parse format string */
    
    while(len > 0)
    {
	if( *string != separator)
	{
	    string++; len--;
	    continue;
	}
	string++;
	len--;
	switch( *string)
	{
	  /* Get decimal character */
	  case 'd':  
	    string++; len--;
	    format->decimal = *string;
	    break;

	  /* Number in fraction format */
	  case 'f':
	    string++; len--;
	    format->fraction = *string;
	    break;

	  /* Get Thousand separator */
	  case 't':  
	    string++; len--;
	    if( *string == ' ' || len <= 0)
		format->thousand = '\0';
            else
		format->thousand = *string;
	    break;

	  /* Currency sign can be in multiple characters */
	  case 'c':  
	    for(i=0, string++, len--; 
    	    	  *string != ' ' && len > 0 && i < II_MAX_CURR;
		  string++, len--)
	        format->curr[i++] = *string;
	    format->curr[i] = '\0';
	    break;

	  /* Get Scale */
	  case 's':
	    for(i=0, format->sc=0, string++, len--; 
		  *string != ' ' && len > 0; 
		  string++, len--)
		format->sc = format->sc * 10 + (*string - '0');
	    break;

	  default:
	    status = II_ERROR;
	}
	if( status == II_ERROR)
	    break;
	len--; string++;
    }

     return (status);
}

/*
** Name: usop_sum() - sum a set of ord_pair's (just sums each element).  
**
** Description:
**
** Inputs:
**	scb				Pointer to a session control block.
**      dv1                             Ptr to II_DATA_VALUE of the first 
**                                      operand, which is a ORD_PAIR datatype.
**	rdv				Ptr to II_DATA_VALUE to hold resulting
**					summed result. 
**	    .db_length			The length of the result type.
**
** Outputs:
**	rdv
**	    .db_data			Ptr to resulting currency value.
**
**	Returns:
**	    II_STATUS
**
##  History:
##	19-oct-05 (inkdo01)
##	    Written as proof of concept for UDT aggregation.
*/
II_STATUS
#ifdef __STDC__
usop_sum(
II_SCB		    *scb,
II_DATA_VALUE	    *rdv,
II_DATA_VALUE	    *dv1)
#else
usop_sum(scb, dv1, rdv)
II_SCB		    *scb;
II_DATA_VALUE	    *rdv;
II_DATA_VALUE	    *dv1;
#endif
{
    ORD_PAIR	*ival, *rval;

    ival = (ORD_PAIR *)dv1->db_data;
    rval = (ORD_PAIR *)rdv->db_data;

    rval->op_x += ival->op_x;
    rval->op_y += ival->op_y;

    return( II_OK );
}


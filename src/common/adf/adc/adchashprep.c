/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <mh.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adfhash.h>

/**
**
**  Name: ADCHASHPREP.C - Convert data value into "hash-prep" form.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the  external ADF call "adc_hashprep()" which
**      will convert a data value into its "hash-prep" form.
**
**      This file defines:
**
**          adc_hashprep() -	    Convert data value into "hash-prep" form.
**	    adc_1hashprep_rti() -   Convert a builtin data value into
**				    its corresponding "hash-prep" data
**				    value.
**	    adc_inplace_hashprep()  Stub when no conversion needed
**	    adc_vch_hashprep()	    Hashprep for VARCHAR type
**
**	Important general note!
**	You cannot change how a data value is hashprep'ed unless you are
**	willing to rewrite data in existing databases.  The prepared
**	value goes into the hash function, and if the preparation method
**	changes, the hash result will change relative to the original
**	and we wouldn't be able to find rows hashed the old way.
**
**	If there is ever some compelling reason to change a hashprep
**	technique, you will need a versioning scheme, or rewrite data.
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	24-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	09-jun-88 (jrb)
**	    Modified to use CM macros for Kanji support.
**	09-aug-88 (jrb)
**	    Changed hashprep for CHAR from strip-trailing-blanks to copy-as-is.
**	    This is because DMF currently avoids hashprep for non-nullable CHAR
**	    and we want to be consistent.  Also, the removal of trailing blanks
**	    was done incorrectly so this is now fixed, but it invalidates
**	    existent 6.0 databases with hashed tables containing nullable CHAR
**	    columns in their keys.  A program is being written to correct this.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	27-mar-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1hashprep_rti().
**	28-apr-89 (fred)
**	    Altered to use ADI_DT_MAP_MACRO().
**	05-jul-89 (jrb)
**	    Added decimal support.
**	16-apr-90 (nancy)
**	    Fix bug 21004 in adc_1hashprep_rti().  See comments below.
**      22-mar-91 (jrb)
**          Put comment markers around "xDEBUG" after #endif's.  This was
**          causing problems on some compilers.
**	22-Oct-1992 (fred)
**	    Fixed up DB_VBIT_TYPE to reflect ANSI data representation.
**      22-dec-1992 (stevet)
**          Added function prototype.
**      05-Apr-1993 (fred)
**          Added byte datatypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	14-may-1993 (robf)
**	    Added seclabel id  datatype
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Feb-2001 (gupsh01)
**	    Added support for nvarchar, unicode data type.
**	12-dec-2001 (devjo01)
**	    Corrected NVCHR code in adc_1hashprep() to address b106597.
**	13-Jul-2006 (kschendel)
**	    Eliminate data movement when possible, more comments.
**	04-Oct-2007 (kibro01) b119246
**	    Add return codes to adc_inplace_hashprep and adc_vch_hashprep
**/


/*{
** Name: adc_hashprep()	- Convert data value into "hash-prep" form.
**
** Description:
**      This function is the external ADF call "adc_hashprep()" which
**      will convert a data value into its "hash-prep" form.
**
**      It should be noted that this operation does not actually form a
**      hash key in the usual sense.  That is, it does not actually
**      perform any hashing.  Rather, it translates data values into
**      values on which INGRES can perform its standard hashing
**      function.  With most datatypes, this is simply a matter of
**      copying the data value.  But, with the c datatype, for
**      instance, blanks are eliminated because they are ignored in
**      comparisons.  (One wants all values wich compare as equal to
**      hash to the same bucket.)
**
**	Note, that if a NULL value is passed to this routine, the "hash-prep"
**	form returned will be 3 bytes of 0xff, regardless of datatype.
**	This value was chosen because of the low probability of collisions
**	with "hash-prep" forms of other non-null values.
**
**	This function seems redundant, because one should be able to 
**	form hash keys using the ISAM key builder, by giving it the data
**	value and telling it that the operator is "equals".  However,
**	this won't work for historical reasons:  The way keys have been 
**	formed with some datatypes are different for the HASH access
**      method than for the ISAM or B-TREE access method.  To change
**      one or the other would invalidate any existing HASH, ISAM, or
**	B-TREE indexes.
**
**	The hashprep function is allowed to modify the result DB_DATA_VALUE
**	so that its pointer points at the input, eliminating data copying.
**	The caller can't count on this and must allocate result space for
**	a copy, just in case.  (If the caller is sure that the input value
**	can be used as-is, it shouldn't call hashprep at all!)
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dvfrom                      Pointer to the data value to be
**                                      converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value to be
**					converted.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvhp                        Pointer to the data value to be
**					created.  Note, the datatype of
**					this DB_DATA_VALUE is irrelevant.
**					The resulting "hash-prep" value is
**					always treated as a stream of bytes.
**		.db_length  		Length, in bytes, of buffer pointed
**					to by the .db_data field.  If this
**					length is too small for the resulting
**					hashprep value, the error
**					E_AD3010_BAD_HP_DTLEN will be gen-
**					erated.
**		.db_data    		Pointer to location to put the
**					actual data for the data value
**					being created.
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
**      adc_dvhp                        The resulting data value.
**		.db_length		The actual length of the resulting
**					hashprep value.
**		.db_data    		Points to the prepared value.  Note
**					that this may or may not be the same
**					place as originally passed in.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**          E_AD3010_BAD_HP_DTLEN       Length for the resulting data
**					value is not correct for the
**					"hp" mapping on the from datatype
**					and length.
**
**	    NOTE:  The following error codes are internal consistency
**	    checks which will be returned only when the system is
**	    compiled with the xDEBUG flag.
**	    E_AD2005_BAD_DTLEN		Internal length for the adc_fromdv
**					datatype is invalid.
**	    E_AD200B_BAD_PREC		Invalid precision specified for DECIMAL;
**					must be: 1<= precision <=DB_MAX_DECPREC
**	    E_AD200C_BAD_SCALE		Invalid scale specified for DECIMAL;
**					must be: 0<= scale <=precision
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	24-feb-87 (thurston)
**	    Added support for nullable datatypes.
**	13-Jul-2006 (kschendel)
**	    Do a little code bumming.
**	09-nov-2010 (gupsh01) SIR 124685
**	    Protype cleanup.
*/

/* It's not clear that this is the best replacement for NULL, but
** it can't be changed now or NULL will start hashing differently.
*/
static char null_hash[] = { '\377', '\377', '\377'};


DB_STATUS
adc_hashprep(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *adc_dvfrom,
DB_DATA_VALUE       *adc_dvhp)
{
    ADI_DATATYPE	*dtptr;
    DB_STATUS   	db_stat;
    i4			from_bdt = abs((i4) adc_dvfrom->db_datatype);
    i4			bdtv;


    /* Check whether the "from" datatype is known to ADF */
    bdtv = ADI_DT_MAP_MACRO(from_bdt);
    if (bdtv <= 0 || bdtv > ADI_MXDTS
	||  (dtptr = Adf_globs->Adi_dtptrs[bdtv]) == NULL
       )
    {
	db_stat = adu_error(adf_scb, E_AD2004_BAD_DTID, 0);
	return (db_stat);
    }

    if (adc_dvhp->db_length < adc_dvfrom->db_length)
    {
	db_stat = adu_error(adf_scb, E_AD3010_BAD_HP_DTLEN, 0);
	return (db_stat);
    }

#ifdef xDEBUG
    /* Check the consistency of adc_dvfrom's length */
    if (db_stat = adc_lenchk(adf_scb, FALSE, adc_dvfrom, NULL))
	return (db_stat);
#endif /* xDEBUG */

    /*
    ** If non-nullable, call the appropriate routine with input
    ** DB_DATA_VALUE to create a "hashprep" value.  If nullable, but
    ** without a NULL value, call the appropriate routine with a temp
    ** DB_DATA_VALUE (adjusted to look non-nullable).  If a NULL value is
    ** supplied, just return 3 bytes of \377.  This value will have a low
    ** probability of coliding with other hash values.
    **
    */

    if (adc_dvfrom->db_datatype > 0)	/* non-nullable */
    {
	db_stat = (*dtptr->adi_dt_com_vect.adp_hashprep_addr)
			    (adf_scb, adc_dvfrom, adc_dvhp);
	return (db_stat);
    }

    if ( ((*((u_char *)adc_dvfrom->db_data + adc_dvfrom->db_length - 1)) & ADF_NVL_BIT) == 0)
    {
	DB_DATA_VALUE	tmp_dvfrom;

	/* Just ignore the null indicator byte and prep the rest */
	tmp_dvfrom.db_data = adc_dvfrom->db_data;
	tmp_dvfrom.db_prec = adc_dvfrom->db_prec;
	tmp_dvfrom.db_collID = adc_dvfrom->db_collID;
	tmp_dvfrom.db_datatype = from_bdt;
	tmp_dvfrom.db_length = adc_dvfrom->db_length - 1;

	db_stat = (*dtptr->adi_dt_com_vect.adp_hashprep_addr)
			(adf_scb, &tmp_dvfrom, adc_dvhp);
    }
    else					/* nullable, with NULL value */
    {
	adc_dvhp->db_data = (PTR) &null_hash[0];
	adc_dvhp->db_length = 3;
	db_stat = E_DB_OK;
    }

    return(db_stat);
}


/*
** Name: adc_xxx_hashprep()	-   Convert builtin datatypes' data value
**				    into "hash-prep" form.
**
** Description:
**      This function actually does the hash preparation for data values
**	with built-in, non-nullable data types.
**
**	There can be separate adc_xxx_hashprep's tuned for common
**	datatypes, and a catch-all with a type switch to handle
**	uncommon ones.  The exact implementation doesn't really
**	matter;  the important thing is that the call sequence is
**	identical for all variants.  This header comment applies to
**	all variants equally.
**
**	The output DB_DATA_VALUE is adjusted to reflect the prepared
**	value.  The length may be updated, and the db_data pointer may
**	be changed to point elsewhere (e.g. at the input directly).
**	The caller is not supposed to know whether any given value is
**	hash-prepped in-place or not, so a suitable result area should
**	always be passed in with the output DB_DATA_VALUE.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adc_dvfrom                      Pointer to the data value to be
**                                      converted.
**		.db_datatype		Datatype id of data value to be
**					converted.
**		.db_prec		Precision/Scale of data value to be
**					converted.
**		.db_length  		Length of data value to be
**					converted.
**		.db_data    		Pointer to the actual data for
**		         		data value to be converted.
**      adc_dvhp                        Pointer to the data value to be
**					created.  Note, the datatype of
**					this DB_DATA_VALUE is irrelevant.
**					The resulting "hash-prep" value is
**					always treated as a stream of bytes.
**		.db_length  		Length, in bytes, of buffer pointed
**					to by the .db_data field.  
**		.db_data    		Pointer to location to put the
**					actual data for the data value
**					being created.
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
**      adc_dvhp                        The resulting data value.
**		.db_length		The resulting length of the hashprep
**					value.
**	        .db_data    		The actual data for the data
**					value created will be placed
**					here.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-jun-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	28-oct-86 (thurston)
**	    Added char and varchar support.
**	09-jun-88 (jrb)
**	    Modified to use CM macros for Kanji support.
**	09-aug-88 (jrb)
**	    Changed hashprep for CHAR from strip-trailing-blanks to copy-as-is.
**	    This is because DMF currently avoids hashprep for non-nullable CHAR
**	    and we want to be consistent.  Also, the removal of trailing blanks
**	    was done incorrectly so this is now fixed, but it invalidates
**	    existent 6.0 databases with hashed tables containing nullable CHAR
**	    columns in their keys.  A program is being written to correct this.
**	27-mar-89 (mikem)
**	    Logical key development.  Added support for logical_key datatype
**	    to adc_1hashprep_rti().
**	19-may-89 (Derek)
**	    Added support for SECURITY_LABEL datatype.
**	05-jul-89 (jrb)
**	    Added decimal support.
**	16-apr-90 (nancy)
**	    Use I2ASSIGN_MACRO to fix bug 21004.
**	22-Oct-1992 (fred)
**	    Fixed up DB_VBIT_TYPE to reflect ANSI data representation.
**      05-Apr-1993 (fred)
**          Added byte datatypes.
**	05-Feb-2001 (gupsh01)
**	    Added support for nvarchar, unicode data type.
**	12-dec-2001 (devjo01)
**	    Fix up NVARCHAR processing.
**	13-Jul-2006 (kschendel)
**	    Break out a couple hashpreps based on type, as it's
**	    easy enough to do.  Don't copy data value when not needed.
**	04-Oct-2007 (kibro01) b119246
**	    Add return codes to adc_inplace_hashprep and adc_vch_hashprep
*/

/* This hashprep used for any data type that is ok as-is.  Simply point
** the result at the input, and adjust the result length.
*/

DB_STATUS
adc_inplace_hashprep(
	ADF_CB *adf_scb,
	DB_DATA_VALUE *adc_dvfrom,
	DB_DATA_VALUE *adc_dvhp)
{
    adc_dvhp->db_data = adc_dvfrom->db_data;
    adc_dvhp->db_length = adc_dvfrom->db_length;
    return (E_DB_OK);
}


/* This hashprep is for VARCHAR, which would seem to be pretty common.
** If there are trailing blanks, the value has to be copied and the
** leading length count (in the data) adjusted to ignore the trailing
** blanks.  If there aren't any trailing blanks, leave the value in place.
*/

DB_STATUS
adc_vch_hashprep(
	ADF_CB *adf_scb,
	DB_DATA_VALUE *adc_dvfrom,
	DB_DATA_VALUE *adc_dvhp)
{
    u_char *c;
    u_i2 len;
    u_i2 oldlen;

    I2ASSIGN_MACRO(((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_count,
		       len);
    oldlen = len;

    c = ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_text;

    /* set len to length of string disregarding trailing blanks */
    ADF_TRIMBLANKS_MACRO(c, len);
    adc_dvhp->db_length = len + DB_CNTSIZE;
    if (len == oldlen)
	adc_dvhp->db_data = adc_dvfrom->db_data;
    else
    {
	((DB_TEXT_STRING *) adc_dvhp->db_data)->db_t_count = len;
	MEcopy( (PTR) c, len,
		(PTR) ((DB_TEXT_STRING *)adc_dvhp->db_data)->db_t_text);
    }
    return (E_DB_OK);
} /* adc_vch_hashprep */


/* Use this for all the others, for now */
DB_STATUS
adc_1hashprep_rti(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *adc_dvfrom,
DB_DATA_VALUE	    *adc_dvhp)
{
    char	*fromp;
    char	*fromendp;
    char	*hpp;
    u_char	*c;
    i4		pr;
    i4		cnt;
    u_i2	len, oldlen;
    DB_NVCHR_STRING	*chr;
    i2		nvchr_bytes;
    i4		i;


    switch(adc_dvfrom->db_datatype)
    {
      case DB_CHR_TYPE:	    /* Remove all blanks. */
	fromp	 = (char *) adc_dvfrom->db_data;
	hpp	 = (char *) adc_dvhp->db_data;
	fromendp = fromp + adc_dvfrom->db_length;

	while (fromp < fromendp)
	    if (CMspace(fromp))
		CMnext(fromp);
	    else
		CMcpyinc(fromp, hpp);
			
	adc_dvhp->db_length = hpp - (char *)adc_dvhp->db_data;
	break;

      case DB_DTE_TYPE:	    /* Remove first two bytes. */
	/* (kschendel) This one is a surprise.  I would have expected that
	** dates were normalized here.  Does DMF do that by hand?  are
	** we at risk with whichpart for not pre-normalizing?
	** FIXME!
	*/
	adc_dvhp->db_data = (char *)adc_dvfrom->db_data + 2;
	adc_dvhp->db_length = AD_DTE_HPLEN;
	break;

      case DB_NVCHR_TYPE:
	chr = (DB_NVCHR_STRING *) adc_dvfrom->db_data;
	I2ASSIGN_MACRO(chr->count, nvchr_bytes);
	nvchr_bytes = nvchr_bytes * sizeof(UCS2);

	MEcopy( (PTR) chr->element_array, nvchr_bytes,
		(PTR) adc_dvhp->db_data);
	if ( nvchr_bytes < adc_dvfrom->db_length )
	    MEfill( adc_dvfrom->db_length - nvchr_bytes,
		    '\0', adc_dvhp->db_data + nvchr_bytes );
	adc_dvhp->db_length = adc_dvfrom->db_length;
	break;

      case DB_DEC_TYPE:
	MEcopy( (PTR) adc_dvfrom->db_data, adc_dvfrom->db_length,
		(PTR) adc_dvhp->db_data);
	adc_dvhp->db_length = adc_dvfrom->db_length;

	/* must make sure +0 and -0 hash to the same bucket; convert -0 to +0 */
	c  = (u_char *)adc_dvhp->db_data;
	pr = DB_P_DECODE_MACRO(adc_dvfrom->db_prec);
	while (pr > 0)
	{
	    if (((pr-- & 1  ?  (*c >> 4)  :  *c++) & 0xf) != 0)
		break;
	}

	/* if all digits were zero, make sure number is positive zero */
	if (pr == 0)
	    *c = MH_PK_PLUS;
	    
	break;	

      case DB_TXT_TYPE:	    /* Only need to take count field and the number
			    ** of characters reflected in the count field.
			    */
	I2ASSIGN_MACRO(((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_count,
		       len);
	len += DB_CNTSIZE;
	adc_dvhp->db_data = adc_dvfrom->db_data;
	adc_dvhp->db_length = len;
	break;

      case DB_VBYTE_TYPE:   /* Only need to take count field and the number
			    ** of characters reflected in the count field,
			    ** minus any trailing zeros.
			    */
	I2ASSIGN_MACRO(((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_count,
		       len);
	oldlen = len;

	c = ((DB_TEXT_STRING *) adc_dvfrom->db_data)->db_t_text;

	while (len--  &&  *(c + len) == '\0')
	    ;
	len++;

	adc_dvhp->db_length = len + DB_CNTSIZE;
	if (len == oldlen)
	    adc_dvhp->db_data = adc_dvfrom->db_data;
	else
	{
	    ((DB_TEXT_STRING *) adc_dvhp->db_data)->db_t_count = len;
	    MEcopy( (PTR) c, len,
		(PTR) ((DB_TEXT_STRING *)adc_dvhp->db_data)->db_t_text);
	}
	break;

      case DB_VBIT_TYPE:
      {
	DB_BIT_STRING 	*obs, *nbs;
	i4		length;
	i4		byte_length;
	i4		bits_remaining;
	u_char		mask;

	/*
	**  For bit varying type, need only
	**  take the number of bytes over which have valid bits in them.
	**  For these, we must make sure that unused bits are all zeros.
	*/

	obs = (DB_BIT_STRING *) adc_dvfrom->db_data;
	nbs = (DB_BIT_STRING *) adc_dvhp->db_data;

	I4ASSIGN_MACRO(obs->db_b_count, length);
	I4ASSIGN_MACRO(length, nbs->db_b_count);

	byte_length = length / BITSPERBYTE;
	bits_remaining = length % BITSPERBYTE;

	if (byte_length)
	{
	    MEcopy((PTR) obs->db_b_bits,
		   byte_length,
		   (PTR) nbs->db_b_bits);
	}

	if (bits_remaining)
	{
	    nbs->db_b_bits[byte_length] =
		obs->db_b_bits[byte_length] & ~(~0 >> bits_remaining);
	}
	adc_dvhp->db_length = byte_length + (bits_remaining ? 1 : 0);
	break;
      }

      default:
	return(adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    
    }	/* end of switch stmt */

    return(E_DB_OK);
}

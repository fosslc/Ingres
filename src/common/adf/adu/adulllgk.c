/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>


/**
** Name:  ADULLLGK.C -    Low level logical_key routines which primarily 
**				support ADF interface.
**
** Description:
**	  This file defines low level logical_key routines which primarily 
**	  support the ADF interface.
**
**	This file defines:
**
**	    adu_1logkey_comp()	    - compare 2 logical keys
**	    adu_2logkeytologkey()   - convert a logical key to a logical key
**	    adu_3logkeytostr()	    - convert a logical key to a string
**	    adu_4strtologkey()	    - convert a string to a logical key
**	    adu_5logkey_exp()	    - export a logical key to fe as a CHAR.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	16-mar-89 (mikem)
**	    created.
**	26-may-89 (mikem)
**	    Added adu_5logkey_exp().
**	26-jun-89 (mikem)
**	    Added support of DB_LTXT_TYPE to the adu_3logkeytostr().
**	14-aug-89 (mikem)
**	    Add better error message for incompatible lengths in the data
**	    string <-> logical key conversions.
**	02-oct-89 (mikem)
**	    Switch parameters to E_AD5080_LOGKEY_BAD_CVT_LEN error around.
**	08-aug-91 (seg)
**	    Can't do arithmetic on PTR types.
**	16-oct-91 (stevet)
**	    Fixed problem with converting logical key to CHAR when
**	    resulting length is less than 8 byte (B40360). 
**      03-aug-92 (stevet)
**          Added VARCHAR and TEXT supports for adu_3logkeytostr().
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2008 (stial01)
**          adu_3logkeytostr() added support for rdv BYTE
**          adu_5logkey_exp() export tab_key,log_key as BYTE
**      13-oct-2008 (stial01)
**          adu_5logkey_exp() export tablkey,logkey as BYTE if UTF8
**/


/*{
** Name: adu_1logkey_comp() -	Internal routine for comparing logical_keys.
**
** Description:
**      This routine compares two logical_keys.  Logical key's come in two
**	flavors 6 byte and 16 byte.  If the lengths are different then we
**	will return an error for now.  If length's are the same we do a
**	byte by byte compare of the two datatype's, returning MEcmp()
**	semantics.  If performance ever becomes an issue, the MEcmp() call
**	could be expanded in-line.
**
**	In the future we may want to support comparison of different length
**	logical_key's but we must be careful about defining the semantics of 
**	this comparison.  Should we just truncate the larger key?  Should we
**	alway return not equal?  > and < already do not have any semantic
**	meaning for this data type (except that they must be supported so 
**	that a user can use btree, isam indexing on these keys).
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	logkey_dv1			DB_DATA_VALUE describing the 1st
**					logical_key data value.
**	    .db_data			Ptr to a internal logkey struct.
**      logkey_dv2                      DB_DATA_VALUE describing the second
**					logical_key data value.
**	    .db_data			Ptr to internal logkey struct to be 
**					copied into.
** Outputs:
**	cmp    			        Result of comparison, as follows:
**						<0  if  logkey1 <  logkey2
**						>0  if  logkey1 >  logkey2
**						=0  if  logkey1 == logkey2
**
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
**	    E_AD0000_OK			Completed successfully.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-mar-89 (mikem)
**	    created.
*/
/*ARGSUSED*/
DB_STATUS
adu_1logkey_cmp(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*logkey1,
DB_DATA_VALUE	*logkey2,
i4		*cmp)
{
    PTR    lk1 = (PTR) logkey1->db_data;
    PTR    lk2 = (PTR) logkey2->db_data;
    i4	   tmp_val;
	    
    if (logkey1->db_length != logkey2->db_length)
    {
	
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cmp-logkey length"));
    }
    else
    {
	tmp_val = MEcmp(lk1, lk2, logkey1->db_length);

	if (tmp_val < 0)
	    *cmp = -1;
	else if (tmp_val > 0)
	    *cmp = 1;
	else
	    *cmp = 0;
    }

    return(E_DB_OK);
}

/*{
** Name: adu_1logkeytologkey() -	convert a logical key data value into 
**					a logkical key data value.
**
** Description:
**        This routine converts a logical key data value into a logical key
**	data value.
**	This is basically a copy of the data.  It will be used primarily
**	to copy a logical key data value into a tuple buffer.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	date_dv1			DB_DATA_VALUE describing the source
**					date data value.
**	    .db_data			Ptr to a logical key internal struct.
**      date_dv2                        DB_DATA_VALUE describing the destination
**					date data value.
**	    .db_data			Ptr to logical key internal struct to 
**					be copied into.
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
**	date_dv2
**	    *.db_data			Ptr to AD_DATENTRNL struct to be copied
**					into.
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			Completed successfully.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	04-may-89 (mikem)
**	    created.
*/
DB_STATUS
adu_2logkeytologkey(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *logkey_dv1,
DB_DATA_VALUE	    *logkey_dv2)
{
    if (logkey_dv2->db_length != logkey_dv1->db_length)
    {
	/* only support assigning logical_key's of the same length */
	return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2,
				0, "logkey-logkey length"));
    }
    else
    {
	MEcopy( (PTR) logkey_dv1->db_data, logkey_dv2->db_length,
		(PTR) logkey_dv2->db_data);
    }

    return (E_DB_OK);
}


/*{
** Name: adu_3logkeytostr() - logical_key to string conversion.
**
**  Description:
**	Used for converting logical_key to char.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be converted.
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
**	rdv				Ptr to destination DB_DATA_VALUE for
**					converted string.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**	E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE operands was
**					of the wrong type for this operation.
**	E_AD0100_NOPRINT_CHAR		One or more of the non-printing chars
**					were transformed to blanks on conver-
**					sion (for text, longtext, char, or
**					varchar to c conversions).
**
**	Side Effects:
**	      If the input string is text, varchar, or char, and the output
**	    string is c, non-printing characters will be converted to blanks.
**          Also, if the the source string is longer than the destination
**          string, it will be truncated to the length of the destination
**          string. 
**
**  History:
**	20-mar-89 (mikem)
**	08-aug-91 (seg)
**	    Can't do arithmetic on PTR types.
**	    created.
**	26-jun-89 (mikem)
**	    Added support of DB_LTXT_TYPE.
**	16-oct-91 (stevet)
**	    Fixed problem with converting logical key to CHAR when
**	    resulting length is less than 8 byte.  Also delete the
**	    codes that change the length of the resulting data value.
**	    (B40360).
**      03-aug-92 (stevet)
**          Added support for VARCHAR and TEXT.
**	23-apr-2003 (gupsh01)
**	    Added case for DB_CHR_TYPE.
**	20-apr-04 (drivi01)
**		Added routine to handle logical_key/table_key coercions 
**		to unicode.
**	01-Oct-2009 (kiria01) b122673
**	    Initialise unicode properly.
*/

DB_STATUS
adu_3logkeytostr(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *logkey_dv,
DB_DATA_VALUE	    *str_dv)
{
    DB_STATUS	db_stat = E_AD0000_OK;
    char	*copy_ptr;
    u_i2	length_to_copy;
	DB_DATA_VALUE	str2_dv;

    length_to_copy = min(logkey_dv->db_length, str_dv->db_length); 

    switch (str_dv->db_datatype)
    {
	case DB_NCHR_TYPE:
		str2_dv.db_length = length_to_copy;
		str2_dv.db_data = (PTR)MEreqmem(0, str2_dv.db_length, TRUE, NULL);
		copy_ptr = str2_dv.db_data;
		break;

	case DB_CHA_TYPE:
	case DB_CHR_TYPE:
	case DB_BYTE_TYPE:
	    copy_ptr = str_dv->db_data;
	    break;

    case DB_NVCHR_TYPE:
		str2_dv.db_length = length_to_copy;
		str2_dv.db_data = (PTR)MEreqmem(0, str2_dv.db_length, TRUE, NULL);
		copy_ptr = (char *)str2_dv.db_data + sizeof(u_i2);
	    I2ASSIGN_MACRO(length_to_copy, 
			((DB_TEXT_STRING *)str2_dv.db_data)->db_t_count);
		break;
	case DB_LTXT_TYPE:
	case DB_VCH_TYPE:
	case DB_TXT_TYPE:
	    copy_ptr = (char *)str_dv->db_data + sizeof(u_i2);
	    I2ASSIGN_MACRO(length_to_copy, 
			((DB_TEXT_STRING *)str_dv->db_data)->db_t_count);
	    break;

	default:
	    db_stat = E_AD5001_BAD_STRING_TYPE;
	    break;
    }

    if (db_stat == E_AD0000_OK)
    {
		if (str_dv->db_datatype == DB_NCHR_TYPE || 
			str_dv->db_datatype == DB_NVCHR_TYPE)
		{
			if (str2_dv.db_datatype = DB_NVCHR_TYPE)
				str2_dv.db_length = (str_dv->db_length - DB_CNTSIZE)/sizeof(UCS2) + DB_CNTSIZE;
			else
				str2_dv.db_length = str_dv->db_length/sizeof(UCS2);
			if (str_dv->db_datatype == DB_NCHR_TYPE)
				str2_dv.db_datatype = DB_CHA_TYPE;	
			else if (str_dv->db_datatype == DB_NVCHR_TYPE)
				str2_dv.db_datatype = DB_VCH_TYPE;
			MEcopy((PTR) logkey_dv->db_data, length_to_copy, (PTR) copy_ptr);
			db_stat = adu_nvchr_coerce(adf_scb,  &str2_dv, str_dv);
			MEfree((PTR)str2_dv.db_data);
		}
		else
			MEcopy((PTR) logkey_dv->db_data, length_to_copy, (PTR) copy_ptr);
    }

    return(db_stat);
}

/*{
** Name: adu_4strtologkey() - String to logical_key conversion.
**
**  Description:
**	Used for converting c, text, char, varchar to logical_key.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             Ptr to DB_DATA_VALUE containing string
**					to be converted.
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
**	rdv				Ptr to destination DB_DATA_VALUE for
**					converted string.
**  Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	E_AD0000_OK			Completed successfully.
**	E_AD5001_BAD_STRING_TYPE	One of the DB_DATA_VALUE operands was
**					of the wrong type for this operation.
**	E_AD0100_NOPRINT_CHAR		One or more of the non-printing chars
**					were transformed to blanks on conver-
**					sion (for text, longtext, char, or
**					varchar to c conversions).
**
**	Side Effects:
**	      If the input string is text, varchar, or char, and the output
**	    string is c, non-printing characters will be converted to blanks.
**          Also, if the the source string is longer than the destination
**          string, it will be truncated to the length of the destination
**          string. 
**
**  History:
**	20-mar-89 (mikem)
**	    created.
**	14-aug-89 (mikem)
**	    Add better error message for incompatible lengths in the data
**	    string <-> logical key conversions.
**	02-oct-89 (mikem)
**	    Switch parameters to E_AD5080_LOGKEY_BAD_CVT_LEN error around.
**	14-dec-90 (jrb)
**	    Bug 33799.  This routine called adu_size when it really wanted
**	    adu_5strcount; this resulted in stripping blanks from the string
**	    before converting to logical key.
**	20-apr-04 (drivi01)
**		Added routine to handle table_key/logical_key coercions 
**		from unicode.
*/

DB_STATUS
adu_4strtologkey(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    char	*dv1_str_addr;
    i4		dv1_len;
    DB_STATUS   db_stat;
	DB_DATA_VALUE	dv2;

	if (dv1->db_datatype == DB_NCHR_TYPE
		|| dv1->db_datatype == DB_NVCHR_TYPE)
	{
		if (dv2.db_datatype = DB_NVCHR_TYPE)
			dv2.db_length = (dv1->db_length - DB_CNTSIZE)/sizeof(UCS2) + DB_CNTSIZE;
		else
			dv2.db_length = dv1->db_length/sizeof(UCS2);
		dv2.db_data = (PTR)MEreqmem(0, dv2.db_length, TRUE, NULL);
		if (dv1->db_datatype == DB_NCHR_TYPE)
			dv2.db_datatype = DB_CHA_TYPE;	
		else if (dv1->db_datatype == DB_NVCHR_TYPE)
			dv2.db_datatype = DB_VCH_TYPE;	
		dv2.db_prec = dv1->db_prec;
		adu_nvchr_coerce(adf_scb,  dv1, &dv2);
		MEcopy(dv2.db_data, dv2.db_length, dv1->db_data);
		dv1->db_length = dv2.db_length;
		dv1->db_datatype = dv2.db_datatype;
		dv1->db_prec = dv2.db_prec;
		MEfree((PTR)dv2.db_data);
	}

    if ((db_stat = adu_3straddr(adf_scb, dv1, &dv1_str_addr)) != E_DB_OK)
	return (db_stat);
    if ((db_stat = adu_5strcount(adf_scb, dv1, &dv1_len)) != E_DB_OK)
	return (db_stat);
    
    /* Just copy first 16 bytes */

    if (dv1_len >= rdv->db_length)
    {
	MEcopy((PTR) dv1_str_addr, rdv->db_length, (PTR) rdv->db_data);
	db_stat = E_DB_OK;
    }
    else
    {
	db_stat = adu_error(adf_scb, E_AD5080_LOGKEY_BAD_CVT_LEN, 4,
			    sizeof(i4), &rdv->db_length,
			    sizeof(i4), &dv1_len);
    }

    return(db_stat);
}

/*{
** Name: adu_5logkey_exp() - export a logical key data value to the world
**
**  Description:
**
**      This routine, given an logical key datatype, determines which type to send
**	to output.
**
**	Used for exporting logical_key data type to the external world.  Currently
**	we will always export the logical as a "DB_CHA_TYPE".  "table_key" will
**	be exported as a "CHAR(8)" while an "object_key" will be exported as a
**	"CHAR(16)."
**
**	Logical key types are actually "conditionally exportable" (ie. they have the
**	AD_CONDEXPORT bit set).  Sometime in the future the FE's may decide they 
**	rather have the true datatype returned, rather than the CHAR datatype.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      db_value			Ptr to db_datavalue for database
**					type/length/prec
**      ev_value                        Pointer to DB_DATA_VALUE for exported type
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
**      *ev_value                       Filled appropriately.
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
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2004_BAD_DTID           Datatype id unknown to ADF.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
**  History:
**	26-may-89 (mikem)
**	    created.
*/
DB_STATUS
adu_5logkey_exp(
ADF_CB		    *adf_scb,
DB_DATA_VALUE	    *db_value,
DB_DATA_VALUE	    *ev_value)
{
    DB_STATUS	status = E_DB_OK;

    if (db_value->db_datatype != DB_LOGKEY_TYPE && 
        db_value->db_datatype != DB_TABKEY_TYPE)
    {
	status = adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "export nonlogkey");
    }
    else
    {
	/*
	** logical key's are BYTE data
	** Until all of our front ends support BYTE data..export as CHAR
	** On UTF8 installations, export as BYTE
	*/

	ev_value->db_length   = db_value->db_length;
	if (adf_scb->adf_utf8_flag & AD_UTF8_ENABLED)
	    ev_value->db_datatype = DB_BYTE_TYPE;
	else
	    ev_value->db_datatype = DB_CHA_TYPE;

	ev_value->db_prec     = 0;
    }

    return(status);
}

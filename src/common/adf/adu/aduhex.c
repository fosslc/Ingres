/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/* [@#include@]... */

/**
**
**  Name: ADUHEX.C - Routines related to hex strings.
**
**  Description:
**      This file contains routines related to strings of characters that
**	represent hexidecimal numbers.
**
**	This file defines the following externally visible routines:
**
**          adu_hex() - ADF function instance to return hex value of a string,
**		        as a varchar.
**	    adu_hxnum() - ADF function instance to return integer value of a hex
**			  string.
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**      30-dec-86 (thurston)
**          Initial creation.
**	7-jan-86 (daved)
**	    added comments, compiled, debugged, etc
**	07-jan-87 (thurston)
**	    Added line in adu_hex() that sets the count field in the
**	    resulting varchar.
**	29-jan-87 (thurston)
**	    Changed GLOBALDEF to static for the hex chars array.
**	10-feb-87 (thurston)
**	    Had to remove the use of the I1_CHECK_MACRO() in adu_hex() ... we
**	    WANT unsigned characters!
**	31-aug-87 (thurston)
**	    Added the adu_hxnum() routine.
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	29-jun-88 (jrb)
**	    Rewrote hxnum() for Kanji support.
**	05-apr-91 (jrb)
**	    Bug #36901.  We were blindly putting (in_len * 2) into the
**	    resulting varchar's db_t_count field even if this exceeded the
**	    db_length field.  Added a check to stop this from happening.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      21-jan-1999 (hanch04)
**          replace i4  and i4 with i4
**	08-Jun-1999 (shero03)
**	    Add unhex function.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-mar-2001 (somsa01)
**	    Modified adu_hex() so that it will properly send back the
**	    hex string on non byte-swapped machines.
**	29-jun-2001 (somsa01)
**	    Modified the previous change to ONLY be used on native data
**	    types for non byte-swapped machines (i4, f4, f8).
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/


/*
**  Definition of static variables and forward static functions.
*/

static  u_char	   adu_hexchars[16] = { '0','1','2','3','4','5','6','7',
					'8','9','A','B','C','D','E','F' };
/*
[@static_variable_or_function_definition@]...
*/


/*
** Name: adu_hex() - ADF function instance to return hex value of a string,
**		     as a varchar.
**
** Description:
**      This function converts a text datatype into a varchar stringf where
**	each character pair represents the hex representation of the 
**	corresponding character of the input string.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
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
**
**	Exceptions:
**	    none
**
** History:
**      30-dec-86 (thurston)
**	    Initial creation.
**	07-jan-87 (thurston)
**	    Added line that sets the count field in the resulting varchar.
**	10-feb-87 (thurston)
**	    Had to remove the use of the I1_CHECK_MACRO() ... we WANT unsigned
**	    characters!
**	05-apr-91 (jrb)
**	    Bug #36901.  We were blindly putting (in_len * 2) into the
**	    resulting varchar's db_t_count field even if this exceeded the
**	    db_length field.  Added a check to stop this from happening.
**	17-jul-96 (inkdo01)
**	    Change to compute length/addr directly from dv1 for non-string
**	    types (for overloaded hex implementation).
**	09-mar-2001 (somsa01)
**	    Modified so that it will properly send back the hex string on
**	    non byte-swapped machines.
**	29-jun-2001 (somsa01)
**	    Modified the previous change to ONLY be used on native data
**	    types for non byte-swapped machines (i4, f4, f8).
*/

DB_STATUS
adu_hex(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    i4		    in_len;
    i4		    res_len;
    char	    *tmp;
    u_char	    *in_p;
    u_char	    *out_p = ((DB_TEXT_STRING *)rdv->db_data)->db_t_text;
    i4		    chr;
    i4		    c1;
    i4		    c2;
    DB_STATUS	    db_stat;


    switch (dv1->db_datatype)
    {
	/* call lenaddr for the stringish types - it gets varying length
	** lengths, among other things. */
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
	 if (db_stat = adu_lenaddr(adf_scb, dv1, &in_len, &tmp))
	 return(db_stat);
	 break;
	default:	/* all the others */
	 in_len = dv1->db_length;	/* fixed length */
	 tmp = dv1->db_data;
	 break;
    }

    /* If the result size (in_len * 2) is larger than we can accomodate in
    ** the result buffer, then we truncate the output to the smallest multiple
    ** of two that will fit.
    */
    if ((res_len = in_len * 2) > rdv->db_length - DB_CNTSIZE)
	res_len = (rdv->db_length - DB_CNTSIZE) & ~1;

    ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = res_len;

#ifdef LITTLE_ENDIAN_INT
    /*
    ** For non byte-swapped machines, we'll have to reverse
    ** the order of natural machine types, as this is how
    ** they are stored.
    */
    switch (dv1->db_datatype)
    {
	case DB_INT_TYPE:
	case DB_FLT_TYPE:
	case DB_MNY_TYPE:
	    /* Reverse the order of printout. */
	    in_p = (u_char *)(tmp + in_len - 1);
	    for (; res_len != 0; res_len -= 2)
	    {
		chr = *in_p--;	    /* Do NOT use I1_CHECK_MACRO !!! */
		c1 = chr / 16;
		c2 = chr - (c1*16);
		*out_p++ = adu_hexchars[c1];
		*out_p++ = adu_hexchars[c2];
	    }
	    return(E_DB_OK);

	default:
	    /* Fall through to the byte-swap part. */
	    break;
    }
#endif  /* LITTLE_ENDIAN_INT */

    in_p = (u_char *)tmp;
    for (; res_len != 0; res_len -= 2)
    {
	chr = *in_p++;	    /* Do NOT use I1_CHECK_MACRO !!! */
	c1 = chr / 16;
	c2 = chr - (c1*16);
	*out_p++ = adu_hexchars[c1];
	*out_p++ = adu_hexchars[c2];
    }

    return(E_DB_OK);
}


/*
** Name: adu_hxnum() - ADF function instance to return integer value of a hex
**		       string.
**
** Description:
**      This function converts a string of characters (a varchar) which
**	represent a hexidecimal number, into an integer which is that number.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to create the integer from.
**	    .db_datatype		Must be `varchar',`text', or `longtext'.
**	    .db_length			Its length.
**	    .db_data			Ptr to data.
**	rdv				DB_DATA_VALUE describing the integer
**					which will hold the number created.
**	    .db_datatype		Must be `integer'.
**	    .db_length			The integer's length.
**	    .db_data			Ptr to place to put the integer.
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
**	rdv				DB_DATA_VALUE describing the integer
**					which holds the number created.
**	    .db_data			Ptr to the integer.
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
**
**	Exceptions:
**	    none
**
** History:
**      30-dec-86 (thurston)
**	    Initial creation.
**	07-jan-87 (thurston)
**	    Added line that sets the count field in the resulting varchar.
**	10-feb-87 (thurston)
**	    Had to remove the use of the I1_CHECK_MACRO() ... we WANT unsigned
**	    characters!
**	24-nov-87 (thurston)
**	    Converted CH calls into CM calls.
**	28-jun-88 (jrb)
**	    Made changes for Kanji support.
*/

DB_STATUS
adu_hxnum(
ADF_CB              *adf_scb,
DB_DATA_VALUE	    *dv1,
DB_DATA_VALUE	    *rdv)
{
    register u_char	*c = ((DB_TEXT_STRING *)dv1->db_data)->db_t_text;
    register u_char	*endc;
    register i4	n = 0;

    endc = c + ((DB_TEXT_STRING *)dv1->db_data)->db_t_count;
    
    while (c < endc  &&  CMspace(c))
	CMnext(c);

    while (c < endc)
    {
	if (CMhex(c))
	{
	    n = n * 16 +
	        (CMdigit(c) ? *c-'0': (CMupper(c) ? *c-'A'+10 : *c-'a'+10));

	    CMnext(c);
	}
	else
	{
	    break;
	}
    }

    while (c < endc  &&  CMspace(c))
	CMnext(c);

    if (c < endc)
    {
	return (adu_error(adf_scb, E_AD5003_BAD_CVTONUM, 0));
	/*                        \____________________/
	**               		        |
	**	{@fix_me@}		This ---+
	**				should be E_AD????_BAD_HEXCHAR.
	*/
    }
    
    switch (rdv->db_length)
    {
      case 1:
	*(i1 *)rdv->db_data = n;
	break;

      case 2:
	*(i2 *)rdv->db_data = n;
	break;

      case 4:
	*(i4 *)rdv->db_data = n;
	break;

      default:
	return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
    }

    return (E_DB_OK);
}

/*
[@function_definition@]...
*/

/*
** Name: adu_unhex() - ADF function instance to return a string give a hex value
**		     as a char, text, varchar, or c.
**
** Description:
**      This function converts a text datatype into a varchar string where
**	each character pair of the input string represents the hex
**	representation of the corresponding character of the output string.
**	For instance, hex('a123') -> '61313233'; unhex('61313233') -> 'a123'
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1                             DB_DATA_VALUE describing the string
**					operand to be shown as hex.
**	    .db_datatype		
**	    .db_length			
**	    .db_data			Ptr to data.
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
**	rdv				DB_DATA_VALUE describing the varchar
**					which holds the hex representation of
**					the input string.
**	    .db_data			Ptr to the varchar.
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
**
**	Exceptions:
**	    none
**
** History:
**      08-Jun-1999 (shero03)
**	    Initial creation.
*/

DB_STATUS
adu_unhex(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
    i4		    in_len;
    register u_char *c;
    register u_char *endc;
    char	    *tmp;
    u_char	    *out_p = ((DB_TEXT_STRING *)rdv->db_data)->db_t_text;
    register u_char n;
    DB_STATUS	    db_stat;
    char	    charerror = 'N';

    ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = 0; /* set null output str */

    switch (dv1->db_datatype)
    {
	/* call lenaddr for the stringish types - it gets varying length
	** lengths, among other things. */
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
        case DB_LTXT_TYPE:
	case DB_VBYTE_TYPE:
	 if (db_stat = adu_lenaddr(adf_scb, dv1, &in_len, &tmp))
	 return(db_stat);
	 break;
	default:	/* all the others */
	 in_len = dv1->db_length;	/* fixed length */
	 tmp = dv1->db_data;
	 break;
    }

    if (in_len % 2)	/* must be an even number of characters */
    {
	return (adu_error(adf_scb, E_AD5007_BAD_HEX_CHAR, 0));
    }
    		       
    c = (u_char*)tmp;
    endc = c + in_len;
    
    while (c < endc  &&  CMspace(c))
	CMnext(c);

    while (c < endc)
    {
	if (CMhex(c))
	{
	    n = (CMdigit(c) ? *c-'0': (CMupper(c) ? *c-'A'+10 : *c-'a'+10))
	    	<< 4;
	    CMnext(c);
	}
	else
	{
	    charerror = 'Y';
	    break;
	}

	if (CMhex(c))
	{
	    n += (CMdigit(c) ? *c-'0': (CMupper(c) ? *c-'A'+10 : *c-'a'+10));
	    CMnext(c);
	}
	else
	{
	    charerror = 'Y';
	    break;
	}

	*out_p++ = n;
    }

    while (c < endc  &&  CMspace(c))
	CMnext(c);

    if (charerror == 'Y')
    {
	return (adu_error(adf_scb, E_AD5007_BAD_HEX_CHAR, 0));
    }
    else
    {
      ((DB_TEXT_STRING *)rdv->db_data)->db_t_count = in_len / 2;
    }
    	
    return(E_DB_OK);
}

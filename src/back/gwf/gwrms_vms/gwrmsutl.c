/*
** Copyright (c) 1990, 2001 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <er.h>
#include    <cm.h>
#include    <dbms.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <adf.h>
#include    <add.h>
#include    "gwrmsdt.h"
#include    "gwrmsdtmap.h"
#include    "gwrmserr.h"

/**
**
**  Name: GWRMSUTL.C - RMS Gateway Datatype Conversion Utility Functions
**
**  Description:
**	This file contains a bunch of utility functions to handle various
**	common tasks (like error formatting), and some specific commonly-
**	used routines for handle certain datatypes.
**
**          rms_dterr	    - Fill in error information and format error message
**	    rms_intcmpl	    - Negate an i8 or i16
**	    rms_movestring  - Move string into DB_DATA_VALUE
**	    rms_straddr	    - Get address of string base
**	    rms_strcount    - Get length of string
[@func_list@]...
**
**
**  History:
**	01-apr-90 (jrb)
**	    Created.
**      17-dec-92 (schang)
**          prototype
**      30-dec-97 (chash01)
**          add support for byte_type
**      03-mar-1999 (chash01)
**          integrate changes since ingres 6.4
**	28-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from RMS GW code as the use is no longer allowed
**	27-aug-2001 (kinte01)
**	   rename max_char to iimax_char_value since it conflicts with
**	   limits.h max_char on sequent and the compat.h value is overwritten
**	   Required as changes affect generic code per change 452604
[@history_template@]...
**/

/*
[@forward_type_references@]
*/

/*
**  Forward and/or External function references.
*/

DB_STATUS   rms_straddr();          /* get the address of an INGRES string */
DB_STATUS   rms_strcount();	    /* get the char count of an INGRES string */

/*
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/

/*{
** Name: rms_dterr	- Datatype-related error handler
**
** Description:
**	This routine accepts an error code and up to 3 parameter/length pairs
**	and inserts this and other information into the ADF control block
**	passed to it.  This routine, therefore, returns errors as if it were
**	ADF; its callers should treat errors from this module the same way it
**	treats ADF errors.
**
** Inputs:
**      scb		    The ADF_CB which contains the error block
**	  adf_errcb
**	    ad_ebuflen	    Length of following buffer (if 0, no msg desired)
**	    ad_errmsgp	    Pointer to buffer for msg (if NULL, no msg desired)
**	errnum		    The GWF error number of the error
**	num_params	    The number of parameter pairs needed to format this
**			    error's error message
**	p1l		    Length of parameter 1
**	p1p		    Parameter 1
**	....		    More of the same (up to 3 pairs)
**
** Outputs:
**	scb		    Contains adf_errcb to hold error info
**	  adf_errcb
**	    ad_errcode		Error code of error being returned
**	    ad_class		User error or internal error
**	    ad_usererr		Same as ad_errcode
**	    ad_sqlstate 	The sqlstate status code for this error
**	    ad_emsglen		Length of msg put into following buffer
**	    ad_errmsgp		Error msg if this pointer was not NULL
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR  (depending on the error)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	01-apr-90 (jrb)
**	    Created.
**	29-oct-92 (andre)
**	    in ADF_ERROR, ad_generic_error has been replaced with ad_sqlstate +
**	    ERlookup() was renamed to ERslookup() and its interface was changed
**	    to return sqlstate status code instead of generic error number
**      06-nov-96 (lawst01) changed define name from SS50000_MISC_ING_ERRORS
**          to SS50000_MISC_ERRORS to match changed name in header file
**          sqlstate.h.
[@history_template@]...
*/
DB_STATUS
rms_dterr
(
    ADF_CB  *scb,
    i4 	    errnum,
    i4 	    num_params,
    i4 	    p1l,
    PTR	    p1p,
    i4 	    p2l,
    PTR	    p2p,
    i4 	    p3l,
    PTR	    p3p
)
{
    DB_STATUS		status = E_DB_ERROR;
    ADF_ERROR		*errcb;
    STATUS		tmp_status;
    CL_SYS_ERR		sys_err;
    ER_ARGUMENT		parms[3];

    if (scb == NULL)
	return(status);

    errcb = &scb->adf_errcb;

    switch (errnum)
    {
      case E_GW7026_BAD_FLT_TO_MNY:
      case E_GW7050_BAD_ERLOOKUP:
      case E_GW7100_NOCVT_AT_STARTUP:
      case E_GW7999_INTERNAL_ERROR:
	errcb->ad_errclass = ADF_INTERNAL_ERROR;

      default:
	errcb->ad_errclass = ADF_USER_ERROR;
    }

    errcb->ad_errcode = errcb->ad_usererr = errnum;

    /* see if user really wants an error msg */
    if (errcb->ad_ebuflen == 0  ||  errcb->ad_errmsgp == NULL)
    {
	char	    *sqlstate_str = SS50000_MISC_ERRORS;
	i4	    i;

	for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	    errcb->ad_sqlstate.db_sqlstate[i] = sqlstate_str[i];
	errcb->ad_emsglen = 0;
	return(status);
    }

    switch (num_params)
    {
      case 3:
	parms[2].er_size = p3l;
	parms[2].er_value = p3p;
	/* fall through */
      case 2:
	parms[1].er_size = p2l;
	parms[1].er_value = p2p;
	/* fall through */
      case 1:
	parms[0].er_size = p1l;
	parms[0].er_value = p1p;
	/* fall through */
      case 0:
	tmp_status  = ERslookup(errnum, 0, ER_TIMESTAMP,
				errcb->ad_sqlstate.db_sqlstate,
				errcb->ad_errmsgp, errcb->ad_ebuflen, -1,
				&errcb->ad_emsglen, &sys_err, num_params,
				parms);
	break;
    }

    /* If ERlookup failed, handle it */
    if (tmp_status != OK)
    {
	char	    *sqlstate_str = SS50000_MISC_ERRORS;
	i4	    i;

	for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	    errcb->ad_sqlstate.db_sqlstate[i] = sqlstate_str[i];

	errcb->ad_errcode = E_GW7050_BAD_ERLOOKUP;
	errcb->ad_errclass = ADF_INTERNAL_ERROR;

	parms[0].er_size = 4;
	parms[0].er_value = (PTR)&errnum;
	ERslookup(errcb->ad_errcode, 0, ER_TIMESTAMP,
				errcb->ad_sqlstate.db_sqlstate,
				errcb->ad_errmsgp,
				errcb->ad_ebuflen, -1, &errcb->ad_emsglen,
				&sys_err, 1, parms);
	status = E_DB_ERROR;
    }

    return (status);
}


/*{
** Name: rms_intcmpl	- Complement an integer
**
** Description:
**	This routine computes the negation of a 2's complement integer either
**	8 bytes or 16 bytes long.
**
**	The method is to take the 1's complement (i.e. do a binary NOT on
**	each i4 component) and then add 1.
**
** Inputs:
**	iptr		Pointer to array of i4's
**	isize		Length of array (in # of i4's)
**	ires		Pointer to resultant array of i4's
**
** Outputs:
**	*ires		Negation of input is put here
**	
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-apr-90 (jrb)
**	    Created.
[@history_template@]...
*/
VOID
rms_intcmpl
(
    u_i4 *iptr,
    i4	 isize,
    u_i4 *ires
)
{
    static u_i4	    ow1[] = { 1, 0, 0, 0 };
    i4		    tmp[3];


    if (isize == 2)
    {
	tmp[0] = ~iptr[0];
	tmp[1] = ~iptr[1];

	/* ow1 can be used for the quadword 1 too because of byte-swapping */
	lib$addx(tmp, ow1, ires, &isize);
    }
    else
    {
	tmp[0] = ~iptr[0];
	tmp[1] = ~iptr[1];
	tmp[2] = ~iptr[2];
	tmp[3] = ~iptr[3];

	lib$addx(tmp, ow1, ires, &isize);
    }
}


/*{
** Name: rms_octashift	- Shift and add digit to octaword
**
** Description:
**	Multiply octaword by 10 and add digit.
**
** Inputs:
**	op		Pointer to octaword
**	digit		Digit to add
**
** Outputs:
**	*op		New value of shifted octaword with digit added
**	
**	Returns:
**	    E_DB_OK		No problem
**	    E_DB_ERROR		Overflow occurred
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-90 (jrb)
**	    Created.
[@history_template@]...
*/
DB_STATUS
rms_octashift
(
    RMS_OCTAWORD *op,
    i4           digit
)
{
    u_i2	*o2p = op;	/* look at the octaword as an array of i2's */
    i4		i;
    i4		temp;
    i4		carry;
    
    carry = digit;
    for	(i=0; i < 8; i++)
    {
	temp = o2p[i] * 10 + carry;
	o2p[i] = temp % 65536;
	carry = temp / 65536;
    }

    if (carry)
	return(E_DB_ERROR);
    else
	return(E_DB_OK);
}


/*{
** Name: rms_movestring - moves a string into a DB_DATA_VALUE.
**
** Description:
**        Moves a "C" string into a DB_DATA_VALUE padding the DB_DATA_VALUE
**	string appropriately.  If the DB_DATA_VALUE string has INGRES type c,
**	any non-printing characters will be mapped to blanks.  If the string
**	has INGRES type text, then null characters are converted to blanks.
**
** Inputs:
**      source                          The source "C" string.
**      sourcelen                       The length of the source string.
**      dest	                        Ptr to the destination DB_DATA_VALUE.
**	    .db_datatype		Datatype of the DB_DATA_VALUE.
**	    .db_length			Length of the result DB_DATA_VALUE.
**
** Outputs:
**      dest                            Ptr to the destination DB_DATA_VALUE.
**	    .db_data			Ptr to memory where string is moved to.
**	Returns:
**	    E_DB_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-apr-90 (jrb)
**	    Created.
*/
DB_STATUS
rms_movestring
(
    register u_char	   *source,
    register i4		   sourcelen,
    register DB_DATA_VALUE *dest
)
{
    DB_STATUS           db_stat;
    char		*cptr;
    u_char		*endsource;
    register u_char     *outstring;
    register i4		outlength;
    u_char		*endout;

    if (db_stat = rms_straddr(dest, &cptr) != E_DB_OK)
	return (db_stat);
    outstring = (u_char *)cptr;

    switch (dest->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_BYTE_TYPE:
      case DB_LBYTE_TYPE:
        outlength = dest->db_length;
        break;

      case DB_TXT_TYPE:
      case DB_VCH_TYPE:
      case DB_VBYTE_TYPE:
        outlength = dest->db_length - DB_CNTSIZE;
        break;
       
    }

    endsource = source + sourcelen;
    endout    = outstring + outlength;
    
    if (dest->db_datatype == DB_CHR_TYPE)
    {
        while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
        {
	    /*
	    ** The characters that are allowed as part of a string of type
	    ** c are the printable characters(alpha, numeric, 
	    ** punctuation), blanks, IIMAX_CHAR_VALUE, or the pattern matching
	    ** characters.  NOTE: other whitespace (tab, etc.) is not
	    ** allowed.
	    */
            if (!CMprint(source) && (*source) != ' ' 
		&& (*source) != IIMAX_CHAR_VALUE
		&& (*source) != DB_PAT_ANY && (*source) != DB_PAT_ONE
		&& (*source) != DB_PAT_LBRAC && (*source) != DB_PAT_RBRAC)
            {
                *outstring++ = ' ';
		CMnext(source);
            }
	    else
	    {
		CMcpyinc(source, outstring);
	    }
        }
    }
    else if (dest->db_datatype == DB_TXT_TYPE)
    {
	while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
        {
	    /* NULLCHARs get cvt'ed to blank for text */
	    if (*source == NULLCHAR)
	    {
                *outstring++ = ' ';
		CMnext(source);
	    }
	    else
	    {
		CMcpyinc(source, outstring);
	    }
	}
    }
    else
    {
	while (source < endsource  &&  outstring + CMbytecnt(source) <= endout)
	    CMcpyinc(source, outstring);
    }

    switch (dest->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
	while (outstring < endout)
	    *outstring++ = ' ';
        break;

      case DB_VCH_TYPE:
      case DB_TXT_TYPE:
	((DB_TEXT_STRING *)dest->db_data)->db_t_count =
						    outstring - (u_char *)cptr;
	break;
    }
    
    return (E_DB_OK);
}


/*
** Name: rms_straddr - Returns the address of the string in a DB_DATA_VALUE
**
** Description:
**	  This routine returns the address of a string in a DB_DATA_VALUE.
**
** Inputs:
**      dv				Ptr to the DB_DATA_VALUE.
**
** Outputs:
**	str_ptr				Ptr to character ptr to be updated
**					with address of DB_DATA_VALUE string.
**  Returns:
**	E_DB_OK
**
**  History:
**	12-apr-90 (jrb)
**	    Created.
*/
DB_STATUS
rms_straddr
(
    DB_DATA_VALUE  *dv,
    char	   **str_ptr
)
{
    switch (dv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
        case DB_BYTE_TYPE:
        case DB_LBYTE_TYPE:
            *str_ptr = (char *) dv->db_data;
            break;

        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
            *str_ptr = (char *) ((DB_TEXT_STRING *) dv->db_data)->db_t_text;
            break;
    }

    return (E_DB_OK);
}


/*
** Name: rms_strcount - Return the length of a string.
**
** Description:
**	  For the the c and char types the length is the declared length.  For
**	the text and varchar types the length is the actual length
**	stored in the string.
**
** Inputs:
**      dv				Pointer to the DB_DATA_VALUE
**
** Outputs:
**	len_ptr				Address of an i2 to hold length of
**					DB_DATA_VALUE's string length.
**  Returns:
**	    E_DB_OK
**
**  History:
**	12-apr-90 (jrb)
**	    Created.
*/
DB_STATUS
rms_strcount
(
    DB_DATA_VALUE  *dv,
    i4		   *str_len

)
{

    switch (dv->db_datatype)
    {
        case DB_CHA_TYPE:
        case DB_CHR_TYPE:
            *str_len = dv->db_length;
            break;

        case DB_VCH_TYPE:
        case DB_TXT_TYPE:
            *str_len = ((DB_TEXT_STRING *) dv->db_data)->db_t_count;
            break;
    }

    return (E_DB_OK);
}

/*
[@function_definition@]...
*/

/*
** Copyright (c) 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <aduint.h>

/**
**
**	File: adumisc.c -- Miscellaneous adu functions.
**
**  Description:
**	This file contains miscellaneous functions that don't fit
**	anywhere else, and don't seem worthy of their own file.
**
**	adu_cmptversion -- Generate version string from db/relcmptversion.
**
**  History:
**
**	12-Nov-2009 (kschendel) SIR 122882
**	    Create.
*/


/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
*/


/*{
** Name: adu_cmptversion -- Generate major-version string.
**
** Description:
**	This is the implementation of the iicmptversion() function.
**	varchar result = iicmptversion(i4 lvl)
**
**	This function exists because the major version values in
**	dbcmptlvl (iidatabase) and relcmptlvl (iirelation)
**	were originally 4-character strings.  When the version went
**	from 9.x to 10.0, we ran into two problems:  4 characters
**	is too small (what if we want a 10.0.1?), and when
**	compared as a string, 10.0 is LESS than 9.x.
**
**	The solution chosen was to convert the cmptlvl values from
**	char(4) to u_i4, and reset the version to a decimal integer
**	xxxyyzz (xxx = major version, yy = minor, zz = patch).
**	Since the old character-string-ish versions looked like
**	very large integers, there is no chance of clash;
**	and the integerized versions are easier to handle internally.
**
**	The one drawback is that the standard catalog interface is
**	used to returning a string-ized version, which is used
**	e.g. by the terminal monitor HELP command.  So, the
**	iitables and iidatabase_info views use the iicmptversion()
**	function to generate an old-fashioned string out of the
**	input value.
**
**	If the (unsigned) input is larger than DU_OLD_DBCMPTLVL,
**	we assume it's already a character string and just return it.
**	Otherwise, if it's xxxyy00, we return 'x.y', and if it's
**	xxxyyzz, we return 'x.y.z'.  In all cases the result is
**	defined to be VARCHAR(9) (enough for 'xxx.yy.zz').
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      dv1				Ptr to DB_DATA_VALUE for first param.
**	    .db_datatype		Datatype; assumed to be DB_INT_TYPE.
**	    .db_length			Length.
**	    .db_data			Ptr to the data of first param.
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_datatype		Datatype, assumed to be VARCHAR.
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
**      rdv				Pointer DB_DATA_VALUE for the result.
**	    .db_data			Result will be placed here.
**
**	Returns:
**	    E_DB_OK or error status.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**	12-Nov-2009 (kschendel) SIR 122882
**	    Written.
*/

DB_STATUS
adu_cmptversion(
ADF_CB             *adf_scb,
DB_DATA_VALUE      *dv1,
DB_DATA_VALUE      *rdv)
{
    char	str[10+1];
    DB_TEXT_STRING *result;
    u_i4	cmptlvl;
    u_i8        i8temp;

    switch (dv1->db_length)
    {
	case 1:
	    cmptlvl = I1_CHECK_MACRO( *(i1 *) dv1->db_data);
	    break;
	case 2: 
	    cmptlvl = *(i2 *) dv1->db_data;
	    break;
	case 4:
	    cmptlvl = *(i4 *) dv1->db_data;
	    break;
	case 8:
	    i8temp  = *(u_i8 *)dv1->db_data;

	    /* limit to u_i4 values,  there's no MAXUI4 */
	    if ( i8temp > (u_i8) 0xffffffffL )
		return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cmptversion input overflow"));

	    cmptlvl = (u_i4) i8temp;
	    break;
	default:
	    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cmptversion input length"));
    }

    if (rdv->db_datatype != DB_VCH_TYPE)
	return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "cmptversion result"));
    result = (DB_TEXT_STRING *) rdv->db_data;

    if (cmptlvl > DU_OLD_DBCMPTLVL)
    {
	/* Old style, just stuff into result */
	MEcopy((char *)&cmptlvl, 4, str);
	str[4] = EOS;
	/* There might be ONE trailing blank, if so trim it */
	if (str[3] == ' ')
	    str[3] = EOS;
    }
    else if (cmptlvl <= 9999999)
    {
	/* New style, interpret number */
	if ( (cmptlvl % 100) == 0)
	    STprintf(str,"%d.%d",cmptlvl/10000,(cmptlvl/100)%100);
	else
	    STprintf(str,"%d.%d.%d",cmptlvl/10000,(cmptlvl/100)%100,cmptlvl%100);
    }
    else
    {
	/* New style version too large for fixed result size in fi_defn.txt,
	** someone is goofing off or we're reached version 1000.0 ...
	*/
	STcopy("Err", str);
    }
    result->db_t_count = STlength(str);
    MEcopy(str, result->db_t_count, &result->db_t_text[0]);

    return (E_DB_OK);
} /* adu_cmptversion */

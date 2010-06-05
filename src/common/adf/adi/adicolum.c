/*
** Copyright (c) 1986, Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cm.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adudate.h>
#include    <cv.h>

/**
**
**  Name: ADICOLUM.C - Routines to deal with column specifications.
**
**  Description:
**      This file contains all of the routines necessary to
**      deal with encoding/decoding users' column specifications
**      into/from an internal form, usually a DB_DATA_VALUE.
**
**      This file defines:
**
**          adi_encode_colspec() - Generate DB_DATA_VALUE from column spec.
**
**
**  History:
**      02-may-88 (thurston)
**          Initial creation.
**	03-aug-89 (jrb)
**	    Moved from ADC to ADI and coded.
**      02-feb-1991 (jennifer)
**          Fix code to support numbers in type names and to ignore
**          embed and paren type col info for UDTs.
**      02-nov-1992 (stevet)
**          Added check for DB_FLT_TYPE.  If length is embedded in column
**          spec the value has to be 4 or 8 (i.e. f4, f8, float4 or float8).
**          This is added to support float precision float(n) so that
**          it does not affect the behavior for f or float column spec.
**          Also added check for protocol level to see if decimal
**          column can be created.
**      12-dec-1992 (stevet)
**          Added function prototype.
**      20-apr-1993 (stevet)
**          Added missing include file <st.h>.
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
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	06-May-2010 (kiria01) b123689
**	    Correct header comment.
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definition@]
*/


/*{
** Name: adi_encode_colspec() - Generate DB_DATA_VALUE from column spec.
**
** Description:
**      This function is the external ADF call "adi_encode_colspec()" which
**      generates a description of the column in the form of a DB_DATA_VALUE.
**	The datatype ID, precision/scale, and internal length fields are set
**	up from the inputs.  This routine assumes that the column spec will have
**	no trailing blanks; case is not significant.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	name				Null terminated string that should be
**					what the user typed in as the datatype
**					name.  This will include strings such
**					as:  "date", "i2", "double precision",
**					"float4", "smallint", et. al.
**	numnums				The number of numbers in the nums array.
**	nums				An array of nats which is the
**					sequence of zero or more, parenthesized,
**                                      comma separated integers which followed
**                                      the datatype name in the user's
**                                      specification.  An array of zero numbers
**                                      will be passed for a string like "f8",
**                                      "money", or "c200".  An array of one
**                                      number will be passed for a string like
**                                      "text(50)" or "char(12)".  An array of
**                                      two numbers will be passed for a string
**                                      like "decimal(10,2)".  Etc. 
**	flags				
**	    ADI_F1_WITH_NULLS		If set, the column will be nullable. If
**					not set, the column will be
**					nonnullable.  The parser, or other
**					caller, determines this from either
**					what the user typed in, or by the
**					default for the query language in use.
**	    ADI_F2_COPY_SPEC		Allow slightly different spec for column
**					for COPY command (e.g. varchar(0) is a
**					valid COPY column spec, but invalid for
**					table creation.)
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
**	dv				DB_DATA_VALUE representing the column
**					specified.
**	    .db_datatype		The datatype ID for the column.  This
**					will be negative if the column is
**					nullable, positive if the column is
**					non-nullable.  This will be set to
**					DB_NODT if the datatype name is unknown.
**	    .db_prec			The precision and scale for this column,
**					if the datatype is DECIMAL, FLOAT or one
**					of the precision carrying ANSI datatype.
**					Set to zero otherwise.
**	    .db_length			The internal length for the column.
**					That is, the number of bytes necessary
**					to store a value in this column.
**
**      Returns:
**	    E_DB_OK			Succeeded.
**	    E_DB_ERROR			( see below )
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD2003_BAD_DTNAME         Datatype name unknown to ADF.
**	    E_AD2006_BAD_DTUSRLEN	Illegal user specified length for this
**					datatype.
**	    E_AD2070_NO_EMBED_ALLOWED	Embedded length not allow for this
**					datatype.
**	    E_AD2071_NO_PAREN_ALLOWED	Parenthesized numbers not allowed for
**					this datatype.
**	    E_AD2072_NO_PAREN_AND_EMBED	User specified both embedded length and
**					had parenthesized numbers following his
**					datatype name!
**	    E_AD2073_BAD_USER_PREC	User specified illegal value for
**					precision.
**	    E_AD2074_BAD_USER_SCALE	User specified illegal value for scale.
**
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      03-aug-89 (jrb)
**          written
**      02-feb-1991 (jennifer)
**          Fix code to support numbers in type names and to ignore
**          embed and paren type col info for UDTs.
**      02-nov-1992 (stevet)
**          Added check for DB_FLT_TYPE.  If length is embedded in column
**          spec the value has to be 4 or 8 (i.e. f4, f8, float4 or float8).
**          This is added to support float precision float(n) so that
**          it will not affect f and float column spec.
**          Also added check for protocol level to see if decimal 
**          column can be created.
**	13-nov-01 (inkdo01)
**	    When precision is specified with float data type, send it to
**	    adc_1lenchk_rti in db_prec, NOT db_length.
**	9-jan-02 (inkdo01)
**	    Convert float precision to length here.
**	25-nov-2003 (gupsh01)
**	    For formatted copy case, if nchar(0) or nvarchar(0) is specified
**	    convert data to UTF8 for formatted copy of nchar/nvarchar data.
**	1-feb-06 (dougi)
**	    Add support for collation IDs.
**	25-apr-06 (dougi)
**	    Add precision for new date/time types.
**	14-sep-06 (gupsh01)
**	    Added support for date_type_alias config paramter.
**	11-dec-2006(dougi)
**	    Validate date/time precision values.
**	3-apr-2007 (dougi)
**	    Support ADI_F4_CAST flag to zero the length for cast's to 
**	    length qualified types with no explicit length.
**	31-Aug-2009 (kschendel) b122510
**	    Long datatypes allow parens, unfortunately, but the only legal
**	    length value is zero.  Enforce same.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adi_encode_colspec(
ADF_CB		    *adf_scb,
char		    *name,
i4		    numnums,
i4		    nums[],
i4		    flags,
DB_DATA_VALUE	    *dv)
# else
DB_STATUS
adi_encode_colspec( adf_scb, name, numnums, nums, flags, dv)
ADF_CB		    *adf_scb;
char		    *name;
i4		    numnums;
i4		    nums[];
i4		    flags;
DB_DATA_VALUE	    *dv;
# endif
{
    ADI_DT_NAME		dt_name;
    ADI_DT_NAME		dt_name_num;
    ADI_DATATYPE	*dtp;
    ADI_COLUMN_INFO	*col_info;
    DB_DATA_VALUE	idv;
    DB_STATUS		status;
    bool		has_embed = FALSE;
    i4			embed_len = 0;
    char		*p;
    char                *num = 0;


    /*
    ** Search for length embedded in column spec  and make
    ** sure it is at end
    */
    for (p = name; *p  ; CMnext(p))
    {
        if (CMdigit(p))
        {
            for (num=p; *p; CMnext(p))
            {
                if (!CMdigit(p))
                {
                    num =0;
                    break;
                }
            }
            if (!*p)
	        break;
        }

    }

    /* if length was found, convert to i4  and put in embed_len */
    if (num)
    {
        has_embed = TRUE;
        if (CVal(num, &embed_len) != OK)
            return(adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0));
    }
 
    /*
    ** Move datatype name into local storage varialbes,
    ** both with and without embedded numbers.
    ** UDTs could have numbers in their names.
    */
    if (num)
        MEmove(num - name, (PTR)name, '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name);
    MEmove(STlength(name), (PTR)name, '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name_num);

    /* search datatypes table for this datatype name */
    if (!num && name && ((*name == 'd') || (*name == 'D')))
    {
	if (STbcompare (name, 0, "date", 0, TRUE) == 0)
	{
	    if (adf_scb->adf_date_type_alias & AD_DATE_TYPE_ALIAS_INGRES)
	    {
		/* replace the input string with ingresdate */
        	MEmove(10, (PTR)"ingresdate", '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name_num.adi_dtname);
	    }
	    else if (adf_scb->adf_date_type_alias & AD_DATE_TYPE_ALIAS_ANSI)
	    {
		/* replace the input string with ansidate */
        	MEmove(8, (PTR)"ansidate", '\0',
                                sizeof(ADI_DT_NAME), (PTR)&dt_name_num.adi_dtname);
	    }
	    else 
	    {
		return (adu_error(adf_scb, E_AD5065_DATE_ALIAS_NOTSET, 0));
	    }
	}
    }
	
    for (dtp = Adf_globs->Adi_datatypes; dtp->adi_dtid != DB_NODT; dtp++)
    {

        if (num && STbcompare(dtp->adi_dtname.adi_dtname, 0,
                       dt_name.adi_dtname, 0, TRUE) == 0)
        {
            break;
        }
        if (STbcompare(dtp->adi_dtname.adi_dtname, 0,
                       dt_name_num.adi_dtname, 0, TRUE) == 0)
        {
            embed_len = 0;
            break;
        }

    }
  

    if (dtp->adi_dtid == DB_NODT)
	return(adu_error(adf_scb, E_AD2003_BAD_DTNAME, 0));

    col_info = &dtp->adi_column_info;

    /* variable length when parens needed must not have embedded length; when
    ** parens are not needed must not have parenthesized length
    */
    if ((dtp->adi_dtstat_bits & AD_USER_DEFINED) == 0)
    {

        if (!col_info->adi_embed_allowed  &&  has_embed)
    	    return(adu_error(adf_scb, E_AD2070_NO_EMBED_ALLOWED, 0));

        if (!col_info->adi_paren_allowed  &&  numnums > 0)
	    return(adu_error(adf_scb, E_AD2071_NO_PAREN_ALLOWED, 0));

        if (has_embed  &&  numnums > 0)
	    return(adu_error(adf_scb, E_AD2072_NO_PAREN_AND_EMBED, 0));

	/* For builtin "long" datatypes, parens are allowed for some
	** unfortunate historical reason, but the length must be zero.
	*/
	if (numnums > 0 && (dtp->adi_dtstat_bits & AD_PERIPHERAL)
	  && nums[0] != 0)
            return(adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0));	    
    }

    /* set the type */
    idv.db_datatype =
	(flags & ADI_F1_WITH_NULL)  ?  -dtp->adi_dtid  :  dtp->adi_dtid;

    /*
    ** Check embedded length for DB_FLT_TYPE.  If there is embedded length,
    ** the value should be 4 or 8. 
    */
    if (dtp->adi_dtid == DB_FLT_TYPE && has_embed 
	&& embed_len != 4 && embed_len !=8)
            return(adu_error(adf_scb, E_AD2006_BAD_DTUSRLEN, 0));	    

    /* Fail if protocol level cannot support decimal */
    if (dtp->adi_dtid == DB_DEC_TYPE 
	&& !(adf_scb->adf_proto_level & AD_DECIMAL_PROTO))
	return(adu_error(adf_scb, E_AD2003_BAD_DTNAME, 0));

    /* set length, precision, and scale */
    if (numnums > 0)
    {
	/* for decimal the nums are precision and scale; for other types it's
	** the length
	*/
	if (dtp->adi_dtid == DB_DEC_TYPE)
	{
	    if (nums[0] < 1  ||  nums[0] > DB_MAX_DECPREC)
	    {
		return(adu_error(adf_scb, E_AD2073_BAD_USER_PREC, 2,
				    (i4)sizeof(i4), (PTR)&nums[0]));
	    }

	    if (numnums == 1)
	    {
		idv.db_prec =
		    DB_PS_ENCODE_MACRO(nums[0], col_info->adi_default_scale);
	    }
	    else
	    {
		if (nums[1] < 0  ||  nums[1] > nums[0])
		{
		    return(adu_error(adf_scb, E_AD2074_BAD_USER_SCALE, 4,
					(i4)sizeof(i4), (PTR)&nums[1],
					(i4)sizeof(i4), (PTR)&nums[0]));
		}

		idv.db_prec = DB_PS_ENCODE_MACRO(nums[0], nums[1]);
	    }
	    idv.db_length = DB_PREC_TO_LEN_MACRO(nums[0]);
	}
	else if (dtp->adi_dtid == DB_FLT_TYPE)
	{
	    idv.db_length = 4;
	    idv.db_prec   = nums[0];
	    if (idv.db_prec >= 0 && idv.db_prec <= DB_MAX_F8PREC)
	    {
		if (idv.db_prec > DB_MAX_F4PREC)
		idv.db_length = 8;
	    }
	    else
		return(adu_error(adf_scb, E_AD2073_BAD_USER_PREC, 2,
				    (i4)sizeof(i4), (PTR)&nums[0]));
	}
	else if (dtp->adi_dtfamily == DB_DTE_TYPE)
	{
	    /* Must be new date/time types - value is precision. */
	    idv.db_length = 0;
	    idv.db_prec = nums[0];

	    /* Precision must be 0 to 9 and can only be specified for
	    ** TIME, TIMSETAMP and INTERVAL DAY TO SECOND. */
	    if ((idv.db_prec != 0 &&
		(dtp->adi_dtid == DB_ADTE_TYPE ||
		dtp->adi_dtid == DB_INYM_TYPE || 
		dtp->adi_dtid == DB_DTE_TYPE)) ||
		idv.db_prec < 0 || idv.db_prec > AD_45DTE_MAXSPREC)
		return(adu_error(adf_scb, E_AD2073_BAD_USER_PREC, 2,
				    (i4)sizeof(i4), (PTR)&nums[0]));
	}
	else
	{
	    idv.db_length = nums[0];
	    idv.db_prec   = 0;
	}
    }
    else
    {
	idv.db_length = embed_len;
	idv.db_prec   = 0;
    }

    /* set default values if no length was specified */
    if (!has_embed  &&  numnums == 0)
    {
	idv.db_length = col_info->adi_default_len;
	idv.db_prec = DB_PS_ENCODE_MACRO(col_info->adi_default_prec,
					 col_info->adi_default_scale);
    }

    /* if we're doing a COPY and a specified length was zero, just return now
    ** (this is done to allow c0, char(0), etc. in COPY)
    */
    if (    (flags & ADI_F2_COPY_SPEC)
	&&  (has_embed  ||  numnums != 0)
	&&  idv.db_length == 0
       )
    {
	STRUCT_ASSIGN_MACRO(idv, *dv);
	/* if format is specified in copy as nchar(0) or
	** varchar(0), modify datatype to UTF8.
	*/
	if ((idv.db_datatype == DB_NVCHR_TYPE) || 
	    (idv.db_datatype == DB_NCHR_TYPE))
	{
	  dv->db_datatype = DB_UTF8_TYPE;
	}
	return(E_DB_OK);
    }

    /* Set the result datatype to be same as input. */

    dv->db_datatype = idv.db_datatype;
    dv->db_collID = -1;


    /* now get adc_lenchk to convert from the external length, precision,
    ** and scale to the internal length, precision, and scale
    */
    if ((status = adc_lenchk(adf_scb, TRUE, &idv, dv)) != E_DB_OK)
	return(status);

    /* Special check for CAST to length qualified type with no specified
    ** length. Set length to 0 to be computed later. */
    if (!has_embed  &&  numnums == 0 &&
	(flags & ADI_F4_CAST) && col_info->adi_paren_allowed == TRUE &&
	(col_info->adi_embed_allowed == FALSE || 
			dv->db_datatype == DB_CHR_TYPE))
	    dv->db_length = dv->db_prec = 0;

    return(E_DB_OK);
}

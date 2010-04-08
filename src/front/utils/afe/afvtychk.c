/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:	afvtychk.c -	Check Datatype Specification from VIFRED.
**
** Description:
**	This file contains the routine used by VIFRED to check the
**	validity of a datatype specification.  The normal 'afe_tychk()'
**	routine is not used because the character datatypes are not
**	allowed to be specified with a length in VIFRED.  The length
**	is taken from the format specification.
**
**	Only routine is this file is:
**
**	afe_vtychk() - Check datatype for VIFRED and set the DB_DATA_VALUE.
**
** History:
**	Revision 6.2  89/06/01  wong
**	Modified since 'iiafParseType()' now accepts string types with
**	no length specified.
**
**	Revision 6.0  87/08/05  dave
**	Initial revision.
**
**	06/15/92 (dkh) - Moved called to adc_lenchk() so that datatypes
**			 that must not have have a specific length will
**			 not cause an error.  Currently, most things
**			 work because of a side effect on checking for
**			 synonyms.  But decimal will fail.
**	10/22/92 (dkh) - Put in special check for the decimal datatype
**			 to make sure no length has been specified.
**			 Can't rely on iiParseType() since that has been
**			 changed to provide a default size of (5,0) if
**			 the user did not specify a size.
**      01/03/93 (smc) - Added missing third parameter to STindex.
**	04/15/93 (dkh) - Fixed bug 50604.  Screen out datatypes that
**			 are not supported by the FEs by calling
**			 IIAFfedatatype().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS	iiafParseType();
DB_STATUS	adi_tyid();
DB_STATUS	adc_lenchk();

/*{
** Name:	afe_vtychk() - Check Datatype for VIFRED and set the DB_DATA_VALUE.
**
** Description:
**	This is a special routine that is used by VIFRED to check that
**	a proper datatype was entered in the attribute form.  The difference
**	between this routine and afe_tychk() is that the character
**	datatypes ('c', 'text', 'char' and 'varchar') must NOT be
**	specified with a length.  It is an error to specify a length
**	for these particular datatypes.  The length will be taken from
**	the format specification for the field or column.  If the
**	specified datatype is not one of the character datatypes listed
**	above, then this routine will simply call afe_tychk() to do
**	checking for the other datatypes.  Once a datatype has been
**	validated, the passed in DB_DATA_VALUE will be updated with
**	the correct ADF type information.  A length of zero will be
**	placed in the DB_DATA_VALUE if one of the character datatypes
**	is specified.
**
** Input:
**	cb		The address of an ADF control block.
**	user_type	Pointer to a DB_USER_TYPE structure.
**
** Output:
**	dbv
**	    .db_datatype	The ADF typeid corresponding to
**				the type string.
**	    .db_length		The internal length of the type
**				corresponding to the type string or
**				zero if a character datatype found.
**
** Returns:
**	{STATUS}	OK
**			E_AF601F_HAS_STRLEN	Length found for a string type.
**
**		returns from:
**			iiafParseType
**			adi_tyid
**			adc_lenchk
**
** History:
**	08/05/87 (dkh) - Initial version.
**	06/14/88 (dkh) - Fixed bug so that dbv.db_length is still zero
**			 even if the datatype is nullable.
**	06/01/88 (jhw) - Modified since 'iiafParseType()' now accepts
**				string types with no length.
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
**	11/89 (jhw) -- Added decimal support.
*/
DB_STATUS
afe_vtychk ( cb, user_type, dbv )
ADF_CB			*cb;
register DB_USER_TYPE	*user_type;
register DB_DATA_VALUE  *dbv;
{
	DB_STATUS	rval;
	DB_DATA_VALUE	dblength;		/* internal length */
	ADI_DT_NAME	typename;

	/*
	** 'iiafParseType()' will parse the type and length from the type
	** specification string.  For example, "text(10)" becomes "text" and
	** "10".  Fixed-length types like "integer", "long varchar", or "date"
	** will have lengths of 0 to comply with the 'adc_lenchk()' interface.
	**
	** The type name will be lowercased for 'adi_tyid()'.
	**
	** Note 'adc_lenchk()' will pass back the internal length,
	** but uses it for intermediate results, hence the indirection.
	*/

	if ( (rval = iiafParseType(cb, user_type->dbut_name, &typename, dbv))
			!= OK  ||
		(rval = adi_tyid(cb, &typename, &(dbv->db_datatype))) != OK)
	{
		return rval;
	}

	/*
	**  If user entered a datatype that is not supported by Vifred,
	**  return FAIL.
	*/
	if (!IIAFfedatatype(dbv))
	{
		return(FAIL);
	}

	/*
	**  Check if the datatype is one of the string types
	**  with which we need to be concerned.
	*/
	if ( dbv->db_datatype == DB_TXT_TYPE
			|| dbv->db_datatype == DB_VCH_TYPE
			|| dbv->db_datatype == DB_CHR_TYPE
			|| dbv->db_datatype == DB_CHA_TYPE
			|| dbv->db_datatype == DB_DEC_TYPE )
	{ /* special case for string and decimal types */
		register char	*name = user_type->dbut_name;
		register i4	cnt = sizeof(user_type->dbut_name);

		if ( dbv->db_length > 1 || dbv->db_prec > 0 )
		{
			/*
			**  Only return if we don't have a decimal datatype
			**  or if it is a decimal datatype, make sure that
			**  the user did not specify a length by looking
			**  for the opening parenthesis.  If we find it
			**  and the above routines don't complain about
			**  the specification, then the user must have
			**  specified a size for the decimal.  In this
			**  case, we must return an error as well.
			**
			**  This check is needed since iiParseType generates
			**  a default size of (5,0) which gets us to this
			**  point in the code.
			*/
			if (dbv->db_datatype != DB_DEC_TYPE ||
				STindex(name, ERx("("), 0) != NULL)
			{
				return E_AF601F_HAS_STRLEN;
			}
		}
		else while ( *name != EOS && cnt > 0 )
		{
			if ( CMdigit(name) )
				return E_AF601F_HAS_STRLEN;
			cnt -= CMbytecnt(name);
			CMnext(name);
		}

		/* Set the Nullability if so specified */
		if ( user_type->dbut_kind == DB_UT_NULL )
		{
			AFE_MKNULL_MACRO(dbv);
		}

		dbv->db_length = 0;
		dbv->db_prec = 0;
	}
	else
	{ /* all other types */

		if ((rval = adc_lenchk(cb, TRUE, dbv, &dblength)) != OK)
		{
			return(rval);
		}

		dbv->db_length = dblength.db_length;

		/* Set the Nullability if so specified */
		if ( user_type->dbut_kind == DB_UT_NULL )
		{
			AFE_MKNULL_MACRO(dbv);
		}
	}

	return OK;
}

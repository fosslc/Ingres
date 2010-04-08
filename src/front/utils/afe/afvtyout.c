/*
**	afvtyout.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	afvtyout.c - Textual representation of ADF datatype for VIFRED.
**
** Description:
**	File contains routine to generate a textual representation of
**	an ADF datatype appropriate for display in the attribute form
**	of VIFRED.
**
**	Routines:
** 		afe_vtyoutput - Generate textual representation of
**				ADF datatype for display in VIFRED.
**
** History:
**	Revision 6.4  89/11  wong
**	Added decimal support.
**
**	Revision 6.0  87/08/05  dave
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

i4		adi_tyname();
DB_STATUS	adc_lenchk();

/*{
** Name:	afe_vtyoutput() -	Get display of ADF datatype for VIFRED.
**
** Description:
**	VIFRED calls this routine to get fill in a DB_USER_TYPE for a field data
**	type.  Difference between this and 'afe_tyoutput()' is that VIFRED does
**	not want to display a length for any of the character datatypes (C,
**	TEXT, CHAR and VARCHAR) or for the decimal type.
**
** Inputs:
**	cb	Pointer to a ADF_CB control block.
**
**	dbv
**	    .db_datatype	The ADF typeid of the ADF type.
**	    .db_length		The internal length of the ADF type.
**
**	user_type
**	    .adut_name		Points to a buffer to hold the type name.
**
** Outputs
**	user_type
**	    .adut_kind		Set to the nullability and defaultability
**				of the type.
**	    .adut_name		Filled in with the type name for the type.
**
** Returns:
**	OK
**
**	returns from:
**    	    adi_tyname
**    	    adc_lenchk
**
** History:
**	08/05/87 (dkh) - Initial version.
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
**	11/89 (jhw) -- Handle DECIMAL similarly to character types, CHAR, etc.
*/
DB_STATUS
afe_vtyoutput(cb, dbv, user_type)
ADF_CB			*cb;
register DB_DATA_VALUE	*dbv;
register DB_USER_TYPE	*user_type;
{
	DB_STATUS	rval;
	DB_DT_ID	dbtype;
	DB_DATA_VALUE	dblength;
	ADI_DT_NAME	name;

	/*
	**  Call to 'adi_tyname()' will return the
	**  textual representation of the datatype.
	*/
	if ( (rval = adi_tyname(cb, dbv->db_datatype, &name)) != OK
	   || (rval = adc_lenchk(cb, FALSE, dbv, &dblength)) != OK )
		return rval;

	/*
	**  Set nullability information and assume datatype is
	**  defaultable if datatype is non-NULL.
	*/
	user_type->dbut_kind =
		AFE_NULLABLE_MACRO(dbv->db_datatype) ? DB_UT_NULL : DB_UT_DEF;

	/*
	**  Don't print out the length for the datatypes:
	**	C		DB_CHR_TYPE
	**	TEXT		DB_TXT_TYPE
	**	CHAR		DB_CHA_TYPE
	**	VARCHAR		DB_VCH_TYPE
	**	DECIMAL		DB_DEC_TYPE
	*/

	switch ( dbtype = AFE_DATATYPE(dbv) )
	{
	  default:
		/* Note:  At present, ignore the length for other types. */
	  case DB_CHR_TYPE:
	  case DB_TXT_TYPE:
	  case DB_CHA_TYPE:
	  case DB_VCH_TYPE:
	  case DB_DEC_TYPE:
		STcopy(name.adi_dtname, user_type->dbut_name);
		break;
	  case DB_INT_TYPE:
	  case DB_FLT_TYPE:
		/* Note:  integer and float types are displayed using the
		** old QUEL synonyms, '[fi]%d', not the ANSI SQL names.
		*/
		_VOID_ STprintf( user_type->dbut_name, ERx("%c%ld"),
					dbtype == DB_INT_TYPE ? 'i' : 'f',
					dblength.db_length
		);
		break;
	} /* end switch */

	return OK;
}

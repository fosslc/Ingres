/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
#include	<cm.h>
#include	<adf.h>
#include	<afe.h>

/**
** Name:    afctychk.c -	Check Type Components and Set DB_DATA_VALUE.
**
** Description:
**	Contains a routine used to check type components and set a
**	DB_DATA_VALUE to the type.  Defines:
**
**     afe_ctychk()	check type components and set DB_DATA_VALUE.
**
** History:
**	Revision 6.0  87/01  wong
**	Initial revision.
**
**	07/06/93 (dkh) - Code cleanup: use ADF_MAXNAME instead of DB_MAXNAME.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:    afe_ctychk() -	Check Type Components and Set DB_DATA_VALUE.
**
** Description:
**	Checks the components of a user-entered type specification for type
**	correctness.  The components are the type name (e.g., "text" or "c10",)
**	the type length (which must be NULL or empty if the type name is of the
**	form "[icf][0-9]+" or is one of the other recognized type synonyms,)
**	and the type scale (which should be not NULL and not empty only if the
**	type length was specified.)
**
**	As part of the verification process, the referenced DB_DATA_VALUE
**	has its datatype ID, length and precision/scale set with internal values
**	corresponding to the input type specification.
**
** Input:
**	cb	{ADF_CB *}  The address of the current ADF control block.
**	len	{char *}  The length component of the type specification.
**	scale	{char *}  The scale component of the type specification.
**
** Output:
**	dbv	{DB_DATA_VALUE *}
**	    .db_datatype    The ADF type ID corresponding to the type string.
**	    .db_length	    The internal length of the type corresponding to]`
**			    the type string.
**
** History:
**	01/87 (jhw) -- Written.
**	4-may-1987	(daver)	Replace "i-f-c" special case with iiafTypeHack.
**	03-aug-1989	(mgw)
**		adc_lenchk() interface changed to take DB_DATA_VALUE last
**		argument instead of i4.
**	11/89 (jhw)  Added decimal support.
**	03-apr-90 (sandyd)
**		Changed DB_LN_FRM_PR_MACRO reference to DB_PREC_TO_LEN_MACRO
**		to match the name change that was made by JRB in dbms.h.
**	10/90 (jhw) - Distinguish 'scale' input string from 'scale' integer.
**		Also, check for empty length and scale strings.  Bug #34262.
**	20-aug-92 (davel)
**		Changed length argument in call to iiafTypeHack() to a
**		DB_DATA_VALUE pointer, so precision and scale can be set too.
**	23-mar-1993 (rdrane)
**		Decimal and numeric datatypes do not always have a scale
**		component.  Regardless, they must still set the db_length
**		and db_prec via the DB_* macros or they will be declared
**		invalid (b49206).
**       9-Jan-2008 (hanal04) Bug 121484
**              If we have a time type and a scale was supplied, store the
**              scale value in db_prec for later use. Non-decimal with scale
**              is assumed to be a fall through to time types.
*/

DB_STATUS
afe_ctychk ( cb, typename, len, scale, dbv )
ADF_CB			*cb;
DB_USER_TYPE		*typename;
char			*len;
char			*scale;
register DB_DATA_VALUE	*dbv;
{
	ADI_DT_NAME	tname;
	i4		length;
	i4		scalev;
	STATUS		rval;
	DB_DATA_VALUE	dbdv;


	STlcopy( typename->dbut_name, tname.adi_dtname, ADF_MAXNAME-1 );
	CVlower( tname.adi_dtname );
	dbv->db_datatype = DB_NODT;
	dbv->db_length = 0;
	dbv->db_prec = 0;

	/*
	** If a length and/or scale exists, validate the value(s).
	** If no scale exists and the type is not decimal or numeric,
	** then set the db_length  to the specified length.
	** If the type is decimal or numeric, then set db_length and
	** db_prec using the appropriate macros.  Note that the scale
	** may be non-existent in this case.
	*/
	if  ((len != NULL) && (*len != EOS))
	{
		if  (iiafScanNum(&len, &length) != OK)
		{
			return afe_error(cb, E_AF6013_BAD_NUMBER, 0);
		}
		scalev = 0;
		if  ((scale != NULL) && (*scale != EOS) &&
		     (iiafScanNum(&scale, &scalev) != OK))
		{
			return afe_error(cb, E_AF6013_BAD_NUMBER, 0);
		}
		if  (((scale == NULL) || (scalev == 0)) &&
		     (STequal(tname.adi_dtname,ERx("dec")) == 0) &&
		     (STequal(tname.adi_dtname,ERx("decimal")) == 0) &&
		     (STequal(tname.adi_dtname,ERx("numeric")) == 0))
		{
			/*
			** No scale was specified, and the type is not
			** DECIMAL/NUMERIC
			*/
			dbv->db_length = length;
		}
		else
		{
			/*
			** We had an explicit scale specification, an explicit
			** type specification of DECIMAL/NUMERIC, or both.
                        ** OR
                        ** We have a TIME with precision.
			*/
                        if((STequal(tname.adi_dtname,ERx("dec")) == TRUE) ||
                           (STequal(tname.adi_dtname,ERx("decimal")) == TRUE) ||
                           (STequal(tname.adi_dtname,ERx("numeric")) == TRUE))
                        {
			    dbv->db_length = DB_PREC_TO_LEN_MACRO(length);
			    dbv->db_prec = DB_PS_ENCODE_MACRO(length, scalev);
                        }
                        else
                        {
                            dbv->db_length = length;
                            dbv->db_prec = scalev;
                        }
		}
	}
	else if ((rval = iiafTypeHack(cb, tname.adi_dtname, dbv)) != OK)
	{
		return rval;
	}

	if  (((rval = adi_tyid(cb, &tname, &(dbv->db_datatype))) != OK) ||
    	     ((rval = adc_lenchk(cb, TRUE, dbv, &dbdv)) != OK))
	{
		return rval;
	}

	dbv->db_length = dbdv.db_length;
	dbv->db_prec = dbdv.db_prec;

	if  (typename->dbut_kind == DB_UT_NULL)
	{
		AFE_MKNULL_MACRO(dbv);
	}

	return(OK);
}


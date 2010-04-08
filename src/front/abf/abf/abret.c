/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	"erab.h"

/**
** Name:	abret.c -	Return Type Utilities.
**
** Description:
**	Contains routines used to check return types.  Defines:
**
**	iiabCkTypeSpec()	check type specification.
**	iiabTypeError()		report error in type specification.
**
** History:
**	Revision 6.2  89/02  wong
**	Removed unused routines.
**
**	Revision 6.0  87/04  bobm
**	Initial revision.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

/* externs */

static ADF_CB	*Cb = NULL;
static char	*_none = NULL;
static char	*_string = NULL;

/*{
** Name:	iiabCkTypeSpec() -	Check Type Specification.
**
** Description:
**	Verifies a type specification and then sets the values in a data value
**	descriptor to match the specification.  As a special case, both "none"
**	and "string" are recognized.
**
**	The type specification should be lower-case.
**
** Inputs:
**	comp		{APPL_COMP *} we may need some info out of this.
**	typespec	{char *}  The type specification.
**	nullable	{bool}  Whether the type is Nullable.
**	dbd		{DB_DATA_VALUE *}  Address of the data value descriptor.
**
** Outputs:
**	dbd		{DB_DATA_VALUE *}  Address of the data value descriptor.
**				.db_datatype	{DB_DT_ID}  The data type.
**				.db_length	{longnat}  The data length.
**				.db_prec	{i2}  The data scale for
**							DB_DEC_TYPEs.
** Returns:
**	{STATUS}	OK or 'afe_tychk()' status.
**
** History:
**	6/87 (bobm)	written
**	08/89 (jhw) -- Use 'STbcompare()' to be case insensitive.  JupBug #7449.
**	22-apr-93 (blaise)
**	    Add an additional check: IIAFfedatatype() checks whether we have
**	    have a valid front end datatype. This is necessary because long
**	    varchar is a valid ingres datatype, but is not supported in the 
**	    front ends. (bug #51167)
**	16-oct-2006 (gupsh01)
**	     Fix date key word to refer to ingresdate or ansidate.
**	10-Jul-2007 (kibro01) b118702
**	    date_type_alias translation should not be done here, since it's
**	    already done in adi_tyid
*/

STATUS
iiabCkTypeSpec ( typespec, nullable, dbd )
char		*typespec;
bool		nullable;
DB_DATA_VALUE	*dbd;
{
	DB_USER_TYPE	ut;
        char            dtemp[11];
	char	    	*dp = dtemp;

	if ( Cb == NULL )
	{
		Cb = FEadfcb();
		_none = ERget(F_AB007E_none);
		_string = ERget(F_AB007F_string);
	}

	if ( STbcompare(typespec, 0, _none, 0, TRUE)  == 0)
	{
		MEfill(sizeof(DB_DATA_VALUE), '\0', dbd);
		dbd->db_datatype = DB_NODT;
		return OK;
	}
	else if ( STbcompare(typespec, 0, _string, 0, TRUE) == 0 )
	{
		MEfill(sizeof(DB_DATA_VALUE), '\0', dbd);
		dbd->db_datatype = DB_DYC_TYPE;
		dbd->db_length = ADE_LEN_UNKNOWN;
		return OK;
	}

	_VOID_ STlcopy( typespec, ut.dbut_name, sizeof(ut.dbut_name)-1 );
	ut.dbut_kind = ( nullable) ? DB_UT_NULL : DB_UT_NOTNULL;

	if (afe_tychk(Cb, &ut, dbd) == OK && IIAFfedatatype(dbd) == TRUE)
		return OK;
	else
		return FAIL;
}

/*{
** Name:	iiabTypeError() -	Report Error in Type Specification.
**
** Description:
**	Reports an error that was found in the specification of a return type.
**
** Input:
**	spec	{char *} 	The type specification.
**	e_default {DB_STATUS}	The default error.
**
** History:
**	02/89 (jhw)  Written.
**	12/1991 (jhw) - Removed check for E_AF6011, which hasn't been returned
**		by AFE for a long time.
*/
VOID
iiabTypeError ( spec , e_default )
char	*spec;
DB_STATUS	e_default;
{
	DB_STATUS	err = Cb->adf_errcb.ad_errcode;

	if ( err == E_AD2003_BAD_DTNAME )
	{
		/* Bad specification */
		IIUGerr(e_default, UG_ERR_ERROR, 1, spec);
	}
	else if (err == E_AD2006_BAD_DTUSRLEN || err == E_AD2007_DT_IS_FIXLEN ||
		err == E_AD2008_DT_IS_VARLEN || err == E_AF6013_BAD_NUMBER)
	{
		/* Bad length */
		IIUGerr(e_default, UG_ERR_ERROR, 1, spec);
	}
	else if (err == E_DB_OK)
	{
		/* Error detected in ABF, not in ADF. */
		IIUGerr(e_default, UG_ERR_ERROR, 1, spec);
	}
	else
	{
		/* mystery error. */
		FEafeerr(Cb);
	}
}

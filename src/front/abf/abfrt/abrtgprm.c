/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
#include	<erar.h>
#include	"keyparam.h"

/**
** Name:	abrtgprm.c -	ABF Run-time System Get Keyword Parameter Value.
**
** Description:
**	Contains the routine that gets a keyword parameter value, i.e., moves it
**	from the 4GL parameter structure into a specified internal C variable.
**	Defines:
**
**	iiarGetValue()	get value from parameter into internal variable.
**
** History:
**	Revision 8.0  89/07  wong
**	Initial revision.
**
**	02-sep-93 (connie) Bug #45520
**		Corrected the incompatible argument in iiarUsrErr:
**		changed prm->class to ERget(prm->class)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
iiarGetValue ( prm, value, type, size )
KEYWORD_PRM	*prm;
PTR		value;	/* data value */
DB_DT_ID	type;	/* data type */
i4		size;	/* data size */
{
	ADF_CB	*cb;
	STATUS	status;

	ADF_CB	*FEadfcb();

	DB_EMBEDDED_DATA	edv;

	cb = FEadfcb();
	edv.ed_data = value;
	edv.ed_type = type;
	edv.ed_length = size;
	edv.ed_null = (i2 *)NULL;
	if ( (status = adh_dbcvtev(cb, prm->value, &edv)) != E_DB_OK )
	{
		if ( cb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL )
			FEafeerr(cb);
		else
		{
			DB_USER_TYPE	stype;
			DB_USER_TYPE	dtype;
			DB_DATA_VALUE	xdbv;

			IIARtyoTypeOutput(prm->value, &stype);
			xdbv.db_datatype = edv.ed_type;
			xdbv.db_length = edv.ed_length;
			xdbv.db_prec = 0;
			IIARtyoTypeOutput(&xdbv, &dtype);
			iiarUsrErr( E_AR0003_PRMTYPE,
					5, prm->parent, ERget(prm->pclass), 
					prm->name,
					dtype.dbut_name, stype.dbut_name
			);
		}
	}
	return status;
}

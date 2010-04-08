/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#define _MEalloc(size)	MEreqmem(0, (size), FALSE, (STATUS *)NULL)
#include	<ol.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#define _FEalloc(size)	FEreqmem(0, (size), FALSE, (STATUS *)NULL)
#include	<adf.h>
#include	<ade.h>
#include	<afe.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	"rts.h"

/**
** Name:	abrtret.c -	ABF Run-Time System Return Value Module.
**
** Description:
**	Contains the routine used to return values from an ABF RTS object, that
**	is a frame or procedure.  Defines:
**
**	IIARrvlReturnVal()	return value from ABF frame/procedure object.
**	iiarDefaultRet()	default return value, v4.0.
**
** History:
**	Revision 6.1  88/05  wong
**	Moved from "abrtcall.qc".
**
**	Revision 6.0  87/07  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need rts.h to satisfy gcc 4.3.
*/

# ifndef abs
# define	abs(x)	((x) >= 0 ? (x) : (-(x)))
# endif

VOID	iiarDefaultRet();

/*{
** Name:	IIARrvlReturnVal() -	Return Value to Calling Object.
**
** Description:
**	Assigns a return value to a calling frame/procedure in v6.0, given the
**	DB_DATA_VALUE into which to return and the parameters of the caller.
**	The equivalent routine for earlier versions is 'abrtsaret()'.
**
** Inputs:
**	dbdv	{DB_DATA_VALUE *}  The data value being returned by an 4GL
**			frame/procedure.
**	name	{char *}  Name of the called procedure or frame.
**	kind	{char *}  String describing the kind of object:  either "frame"
**			or "procedure".
**
** Outputs:
**	prm		{ABRTSPRM *}  The param struct of the caller.
**
** History:
**	11-jun-1987 (agh) -- Written.
**	07/87 (jhw) -- Added support for `dynamic' strings.
**	03/90 (jhw) -- Only print RETTYPE errors if ADH could not convert the
**			return type.  JupBug #8425.
*/
static DB_STATUS	dyn_string();

VOID
IIARrvlReturnVal (dbdv, prm, name, kind)
register DB_DATA_VALUE	*dbdv;
register ABRTSPRM	*prm;
char			*name;
char			*kind;
{
	DB_USER_TYPE	caller_type;
	DB_USER_TYPE	callee_type;
	register ADF_CB	*cb;

	char	*abstrsave();
	char	*iiarOLstrType();

	if ( prm == (ABRTSPRM *)NULL || prm->pr_retvalue == (i4 *)NULL )
		return;		/* no return value expected */

	cb = FEadfcb();
	if ( !NEWPARAM_MACRO(prm) || prm->pr_version < 2 )
	{ /* returning to ... 3.0 4.0 5.0 */
		/*
		** Returning DBDV to old-style frame or procedure.
		*/
		if ( dbdv == (DB_DATA_VALUE *)NULL )
		{
			iiarDefaultRet( prm );
		}
		else
		{
			DB_EMBEDDED_DATA	edv;

			edv.ed_type = iiarDbvType(prm->pr_rettype);
			edv.ed_null = (i2 *)NULL;
			if (edv.ed_type == DB_CHR_TYPE)
			{
				edv.ed_length = dbdv->db_length + 1;
				if ( (edv.ed_data = _FEalloc(edv.ed_length))
						== NULL )
				{
					abproerr( ERx("IIARrvlReturnVal"),
						OUTOFMEM, (char *)NULL
					);
					/*NOTREACHED*/	/* exit! */
				}
			}
			else
			{ /* integer or float */
				edv.ed_data = (PTR)prm->pr_retvalue;
				edv.ed_length = (edv.ed_type == DB_INT_TYPE)
						? sizeof(i4) : sizeof(f8);
			}
			if ( adh_dbcvtev(cb, dbdv, &edv) != E_DB_OK )
			{
    				if ( cb->adf_errcb.ad_errcode !=
							E_AD2009_NOCOERCION )
				{
					FEafeerr(cb);
				}
				else
				{
					IIARtyoTypeOutput(dbdv, &callee_type);
					abusrerr( RETTYPE, name, kind,
						callee_type.dbut_name,
						iiarOLstrType(prm->pr_rettype),
						(char *)NULL
					);
				}
				iiarDefaultRet(prm);
			}
			else if ( edv.ed_type == DB_CHR_TYPE )
			{
				*((char **) prm->pr_retvalue) =
						abstrsave((char *)edv.ed_data);
			}
		}
	} /* end 3.0 4.0 5.0 */
	else
	{ /* returning to ... >= 6.0 */
		/*
		** If the called frame was defined as NOT returning a value,
		** and an attempt was made to return a value, then the 4GL
		** translator issued a compile-time error message.  If the
		** called frame was defined as returning a value, but did not
		** do so, then we return the default value for the datatype
		** expected by the caller.
		*/
		if ( dbdv == (DB_DATA_VALUE *)NULL )
			adc_getempty(cb, (DB_DATA_VALUE *)prm->pr_retvalue);
		else
		{
			register DB_DATA_VALUE  *rdbv =
					(DB_DATA_VALUE *)prm->pr_retvalue;

			if ( ( rdbv->db_datatype == DB_VCH_TYPE &&
					( rdbv->db_length == ADE_LEN_UNKNOWN ||
						rdbv->db_length == 0 ) &&
					/* dynamic string -- allocate space */
				    dyn_string(cb, dbdv, rdbv) != E_DB_OK )  ||
				afe_cvinto(cb, dbdv, rdbv) != E_DB_OK )
			{
    				if ( cb->adf_errcb.ad_errcode !=
							E_AD2009_NOCOERCION )
				{
					FEafeerr(cb);
				}
				else
				{
					/*
					** If the value being returned by the
					** callee and the value expected by the
					** caller do not have compatible data-
					** types, issue a runtime error message.
					*/
					IIARtyoTypeOutput(rdbv, &caller_type);
					IIARtyoTypeOutput(dbdv, &callee_type);
					abusrerr( RETTYPE, name, kind,
						callee_type.dbut_name,
						caller_type.dbut_name,
						(char *)NULL
					);
				}
				adc_getempty(cb, rdbv);
			}
		}
	} /* 6.0 */
}

/*
** Name:	dyn_string() -	Verify and Allocate Space for Return of a
**					Dynamic String.
** Description:
**	Verifies that the data value being returned is coercible to the
**	type expected (which must be a dynamic string of type DB_VCH_TYPE
**	with length equal to zero.)  Then, space is allocated for the
**	return value (with its members set accordingly.)
**
** Input:
**	cb	{ADF_CB *}  The ADF control block.
**	idbv	{DB_DATA_VALUE *}  The data value being returned.
**	rdbv	{DB_DATA_VALUE *}  The expected return value.
**			.db_datatype	{DB_DT_ID}  == DB_VCH_TYPE.
**
** Output:
**	rdbv	{DB_DATA_VALUE *}  The dynamic string return value with
**					sufficient space allocated.
**			.db_data	{PTR}  The allocated space.
**			.db_length	{nat}  The size of the allocated space.
**
** Returns:
**	E_DB_OK		On success.
**	E_DB_ERROR	Incompatible (or invalid) types.
**
** History:
**	07/87 (jhw) -- Written.
**	26-Aug-2009 (kschendel) b121804
**	    Fix cancoerce call.
*/
static DB_STATUS
dyn_string ( cb, idbv, rdbv )
ADF_CB				*cb;
DB_DATA_VALUE		*idbv;
register DB_DATA_VALUE	*rdbv;
{
	DB_EMBEDDED_DATA	edv;
	bool			coerce;

	if ( afe_cancoerce(cb, idbv, rdbv, &coerce) != E_DB_OK ||
			adh_dbtoev(cb, idbv, &edv) != E_DB_OK )
	{
		FEafeerr(cb);
		return E_DB_ERROR;
	}

	if ( !coerce )
		return E_DB_ERROR;

	rdbv->db_length = edv.ed_length + DB_CNTSIZE;
	if ( (rdbv->db_data = _MEalloc(rdbv->db_length)) == NULL )
	{
		abproerr(ERx("dyn_string"), OUTOFMEM, (char *)NULL); /* exit! */
		/*NOTREACHED*/
	}

	return E_DB_OK;
}

/*{
** Name:	iiarDefaultRet() -  Default Return Value, V4.0.
**
** Description:
**	Returns a default value to objects of v4.0 or less when a return value
**	is expected, but the callee did not supply one (e.g. the callee is
**	undefined or they return the wrong type).
**	A return is expected when the param structure is a new param structure
**	and its retvalue is not NULL and its rettype is not OL_NOTYPE.
**
** Inputs:
**	prm		-	The parameter structure. Might not be new.
**
** Side Effects:
**	It will assign to the return if it has to.
**
** History:
**	9-jan-1986 (joe)
**		First Written
*/
VOID
iiarDefaultRet (prm)
ABRTSPRM	*prm;
{
	if (prm != NULL && NEWPARAM_MACRO(prm) && RETEXPECT_MACRO(prm))
	{
		switch (prm->pr_rettype)
		{
		  case OL_STR:
			*((char **) prm->pr_retvalue) = ERx("");
			break;
		  case OL_I4:
			*((i4 *) prm->pr_retvalue) = 0;
			break;
		  case OL_F8:
			*((f8 *) prm->pr_retvalue) = 0.0;
			break;
		}
	}
}

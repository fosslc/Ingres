/*
**	iiretfld.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h> 
# include	<frserrno.h>
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iiretfld.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIgetfldio()	- get a value from a field
**		IIretfield()	- pre 6.0 compatability routine
**	Private (static) routines defined:
**
** History:
**	19-jun-87 (bruceb)	Code cleanup.
**	06/20/87 (dkh) - Fixed problem with trying to convert data
**			 even though field validation failed.
**	07/25/87 (dkh) - Fixed jup bug 515.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/11/87 (dkh) - Code cleanup.
**	16-aug-89 (bruceb)
**		Indicate evaluating aggregates is OK when 'getting' data.
**	07-feb-90 (bruceb)
**		Added IIseterr-called routine for supressing getmessages.
**		Added call on IIseterr for that routine when control block
**		so indicates.
**	13-feb-90 (bruceb)
**		Changed default of getmsgs-supression as per LRC/FRC.
**	13-apr-92 (seg)		"errno" is reserved by ANSI C.  Change name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/* Error message numbers to supress on 'gets'. */
# define	DCONV1_ERR	8651
# define	DCONV2_ERR	8652
# define	DCONV3_ERR	8653
# define	DCONV4_ERR	8130
# define	DCONV5_ERR	8276
# define	MAND_ERR	8650
# define	VAL1_ERR	8600
# define	VAL2_ERR	8601


FUNC_EXTERN	i4	FDqryfld();
FUNC_EXTERN	i4	FDgetfld();
FUNC_EXTERN	STATUS	adh_dbcvtev();
FUNC_EXTERN	i4	(*IIseterr())();

i4	IIFRgmoGetmsgsOff();


/*{
** Name:	IIretfield	-	return field value to caller
**
** Description:
**	Return a field's value to the caller.  This is a
**	cover routine, for pre 6.0 compatability, that calls
**	IIgetfldio to actually get the field's value and convert
**	it into the caller's variable.
**	
**	This routine is part of RUNTIME's external interface for
**	compatability only.
**	
**
** Inputs:
**	variable	Flag - is variable
**	type		Type of embedded data
**	len		Length of embedded data
**	data		Area to receive the data value
**	name		Name of field to get value from
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## getform f1 (i4var = f2)
**
**	if (IIfsetio("f1") != 0) {
**	    IIretfield(ISREF, DB_INT_TYPE, 4, &i4var, "f2");
**	}
**
** Side Effects:
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	23-feb-1984  -	Added new query mode option. (ncg)
**	01/09/87 (dkh) - ADT stubbing changes.
**	24-feb-1987  -  Changed to a compatability cover (drh)
*/

i4
IIretfield(i4 variable, i4 type, i4 len, PTR data, char *name)
{
	return( IIgetfldio( (i2 *) NULL, variable, type, len, data, name ) );
}


/*{
** Name:	IIgetfldio	-	return field value to caller
**
** Description:
**	Return a field's value to the caller.  A number of EQUEL
**	statements will generate calls to this routine.  It is
**	called to return a field's oper or value.  
**
**	IIgetfldio replaced IIretfld at release 6.0, since it
**	allows the caller to pass a NULL indicator.  
**
**	
**	This routine is part of RUNTIME's external interface.
**	
**
** Inputs:
**	ind		Ptr to null indicator
**	variable	Flag - is variable
**	type		Type of embedded data
**	len		Length of embedded data
**	data		Area to receive the data value
**	name		Name of field to get value from
**
** Outputs:
**	Data will be updated to contain the field's value, and if a null
**	indicator was provided, that will be udpated too.
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## getform f1 (i4var = f2)
**
**	if (IIfsetio("f1") != 0) {
**	    IIgetfldio( 0,ISREF, DB_INT_TYPE, 4, &i4var, "f2");
**	}
**
** Side Effects:
**
** History:
**	24-feb-1987	Created, using code extracted from IIretfld (drh).
**	01-apr-1987	Modified to trim the length of character-type
**			fields before adh call (drh).
**	22-apr-1987	Changed parameters in IIftrim call (drh).
**	06/20/87 (dkh) - Fixed problem with trying to convert data
**			 even though field validation failed.
*/

i4
IIgetfldio(i2 *ind, i4 variable, i4 type, i4 len, PTR data, char *name)
{
	char			*namestr;
	char			fbuf[MAXFRSNAME+1];
	DB_EMBEDDED_DATA	edv;
	DB_DATA_VALUE		*dbvptr;
	DB_DATA_VALUE		oper_dbv;
	DB_DATA_VALUE		dbvcop;
	i4			oper;
	ADF_CB			*cb;
	bool			errflag = FALSE;
	i4			getop = 0;
	i4			(*oldproc)();
	bool			disp_msgs = TRUE;

	/*	Check field name for validity  */

	namestr = IIstrconv(II_CONV, name, fbuf, (i4)MAXFRSNAME);
	if (namestr == NULL)
	{
		IIFDerror(RTRFFL, 2, IIfrmio->fdfrmnm, ERx(""));
		return(FALSE);
	}

	/*  Set up an EDV to describe the caller's parameters.  */

	edv.ed_type = type;
	edv.ed_length = len;
	edv.ed_data = (PTR) data;
	edv.ed_null = ind;
	
	cb = FEadfcb();

	if (IIfrscb->frs_globs->enabled & GETMSGS_OFF)
	{
	    disp_msgs = FALSE;
	    oldproc = IIseterr(IIFRgmoGetmsgsOff);
	}

	/*
	**  If getting an 'oper', not a value, special processing required.
	**  Operators are not returned from FRAME as DBV's, so we must build
	**  one so we can follow the normal path and convert to the final EDV.
	*/

	getop = IIgetoper( (i4) 0 );

	if ( getop )
	{
		oper_dbv.db_datatype = DB_INT_TYPE;
		oper_dbv.db_length = sizeof(i4);
		oper_dbv.db_prec = 0;
		oper_dbv.db_data = (PTR) &oper;

		/*
		** Make sure the frame is in Query mode, else return NOOP.
		*/
		if (IIfrmio->fdrunmd != fdrtQRY)
			oper = fdNOP;
		else
			FDqryop( IIfrmio->fdrunfrm, namestr, (i4 *)
				oper_dbv.db_data );

		dbvptr = &oper_dbv;
	}
	else if (IIfrmio->fdrunmd == fdrtQRY)
	{
		/*
		**	If in query mode, we must use a different routine
		**	to retrieve the data portion of the field.  This
		**	routine also strips off the operator.
		*/

		if (!FDqryfld(IIfrmio->fdrunfrm, namestr, &dbvptr))
			errflag = TRUE;
	}
	else
	{
		IIfrscb->frs_event->eval_aggs = TRUE;

		/*
		**	Get data portion of the field
		*/

		if (!FDgetfld(IIfrmio->fdrunfrm, namestr, &dbvptr))
			errflag = TRUE;

		IIfrscb->frs_event->eval_aggs = FALSE;
	}

	/*
	**  If "errflag" is set at this point, then it was not
	**  possible to get the DB_DATA_VALUE pointer for a field.
	**
	**  Trim trailing blanks on character-type fields
	*/
	if ( errflag == FALSE && ( IIftrim( dbvptr, &dbvcop ) != OK ) )
		errflag = TRUE;

	/*
	**  Convert the DBV containing the field's value to an EDV
	*/

	if ( errflag == FALSE && ( adh_dbcvtev( cb, &dbvcop, &edv ) != OK ) )
	{
		if (cb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
		{
			IIFDerror(RTRNLNNL, 2, IIfrmio->fdfrmnm, namestr);
		}
		else
		{
			IIFDerror(RTGFERR, 2, IIfrmio->fdfrmnm, namestr);
		}
		errflag = TRUE;
	}

	if (!disp_msgs)
	    _VOID_ IIseterr(oldproc);

	if ( errflag == TRUE )
		return (FALSE);
	else
		return (TRUE);
}


/*{
** Name:	IIFRgmoGetmsgsOff	- Supress 'get' error messages.
**
** Description:
**	Supress data conversion, mandatory and validation error messages
**	from getform, getrow and unloadtable.  Referenced in IIgetfldio
**	and IItcogetio.
**
** Inputs:
**	errnum		IIFDerror number.
**
** Outputs:
**
**	Returns:
**		0		Don't print the error message.
**		error number	Print the error message.
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	07-feb-90 (bruceb)	Written.
**	13-apr-92 (seg)		"errnum" is reserved by ANSI C.  Change name.
*/
i4
IIFRgmoGetmsgsOff(errnum)
i4	*errnum;
{
    register i4	err = *errnum;

    if ((err == MAND_ERR)
	|| (err == VAL1_ERR)
	|| (err == VAL2_ERR)
        || (err == DCONV1_ERR)
	|| (err == DCONV2_ERR)
	|| (err == DCONV3_ERR)
	|| (err == DCONV4_ERR)
	|| (err == DCONV5_ERR))
    {
	return(0);
    }
    else
    {
	return(err);
    }
}

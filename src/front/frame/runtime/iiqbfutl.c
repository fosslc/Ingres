/**
** Name:	iiqbf.c - Special routines for use for QBF.
**
** Copyright (c) 2004 Ingres Corporation
**
** Description:
**	This file contains special routines for use by QBF to changed
**	the mode of a form without causing a redisplay and a high
**	level interface for range query support.  The use of the latter
**	feature may be in the following way.
**
**	Set up a form for using range query with
**		IIrnginit()
**	Take away range query for a form with
**		IIrngend()
**	Suspend range query capability for a form with
**		IIunrung()
**	Reenable range query capability for a form with
**		IIrngon()
**	Prepare to unload range queries on form with
**		IIrgetinit()
**	Unload range query and build qualification clause by doing
**		{ Check syntax, etc. for fields}
**		IIrvalid()
**		{ Unload a regular field }
**		IIrngget()
**		{ Unload a table field }
**		while (IIrnxtrow())
**		{
**			...
**			IIrngget()
**			...
**		}
**	Get the last query entered by calling
**		IIrnglast()
**
**  History:
**
**	Created - 08/24/85 (dkh)
**	02/12/87 (dkh) - Added headers.
**	5-jan-1987 (peter)	Changed RTfndcol to FDfndcol.
**	27-mar-1987	Modified IIrngget for new QG interface, ADTs, 
**			NULLs (drh)
**	5-may-1987 (Joe)
**		Modified IIrngget to use the newer QG interface.  Now
**		it calls a function to send down the QRYSPECs built.
**	11-sep-87 (bab)
**		Changed cast in call to FDfind from char** to nat**.
**	09-dec-87 (kenl)
**		Changed IIrvalid so that it is passed (and passes on) two more
**		parameters, a function pointer and a structure pointer.  This
**		is needed for QBF to properly build the FROM clause so QG can
**		read a Master row for a Master/Detail JoinDef.
**	02/24/88 (dkh) - Fixed jup bug 2083.
**	05/27/88 (dkh) - Added user support for scrollable fields.
**	23-aug-88 (kenl)
**		Changed IIrvalid so that it is passed (and passes on) one
**		more parameter, the DML being spoken.  This is required
**		by the validity routines farther down the line in order
**		to know if we are talking to a gateway.
**	12/31/88 (dkh) - Perf changes.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
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
# include	<qg.h>

FUNC_EXTERN	bool	FTrvalid();
FUNC_EXTERN	bool	FTrnxtrow();
FUNC_EXTERN	FLDCOL	*FDfndcol();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	VOID	FTgetdisp();
FUNC_EXTERN	FLDHDR	*IIFDgfhGetFldHdr();


GLOBALREF	FRAME	*IIqbffrm ;


/*{
** Name:	IIinitsp - Special forms initialization for QBF.
**
** Description:
**	Special routine for QBF to clear out fields in a form and set
**	mode of form without having the form be cleared and redisplayed
**	on the terminal.  Most of the work relies on FDinitsp() to set
**	the proper mode and to clear the field based on the mode setting.
**	Routine has to transform the name of a form to a pointer to the
**	data structure representing the form.
**
** Inputs:
**	formname	Name of form.
**	clear		Field clear indicator, non zero to clear fields.
**	mode		Encoded mode to set on form.
**
** Outputs:
**	Returns:
**		OK	If everything worked.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/12/87 (dkh) - Added procedure header.
*/
STATUS
IIinitsp(char *formname, i4 clear, i4 mode)
{
	RUNFRM	*runf;

	if ((runf = RTfindfrm(formname)) == NULL)
	{
		return(FAIL);
	}
	FDinitsp(runf->fdrunfrm, clear, mode);

	/*
	**  Reset change bit stuff.
	*/
	FDschgbit(runf->fdrunfrm, 0, FALSE);
	FDschgbit(runf->fdrunfrm, 1, TRUE);

	return(OK);
}


/*{
** Name:	IIqbfutil - Utility routine for other routines in file.
**
** Description:
**	Given the name of a form, return a pointer to the FRAME
**	structure that represents the form.
**
** Inputs:
**	formname	Name of form to find.
**
** Outputs:
**	frmp		Pointer to FRAME structure for form.
**
**	Returns:
**		OK	If form was found.
**		FAIL	If form was not found.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
static STATUS
IIqbfutil(char *formname, FRAME **frmp)
{
	RUNFRM	*runf;
	FRAME	*frm;

	if ((runf = RTfindfrm(formname)) == NULL)
	{
		return(FAIL);
	}
	else
	{
		frm = runf->fdrunfrm;
	/*
	**  Allow things to work as long as the frame has been
	**  initialized.
	**
		if (IIstkfrm->fdrunfrm != frm)
		{
			return(FAIL);
		}
	*/
	}
	*frmp = frm;
	return(OK);
}

# ifndef	PCINGRES

/*{
** Name:	IIrnginit - Initialize a form for range query processing.
**
** Description:
**	Given the name of a form, initialize the form for range query
**	processing.  Most of the work will be done by FTrnginit() to
**	allocate various data structures to support range querying.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If range query initialization was done.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	Various data structures are allocated (and occupied unused
**	structure members in the FRAME structure) to support range
**	querying.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
STATUS
IIrnginit(formname)
char	*formname;
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FAIL);
	}
	FTrnginit(frm);

	/*
	**  Going into range query mode, need to clear out
	**  scrolling bits.
	*/
	IIFDsfsSetFieldScrolling(frm, FALSE);

	return(OK);
}


/*
**  Deallocate storage, etc. for a form that had
**  been set up to handle range specifications.
*/

/*{
** Name:	IIrngend - Clean up range querying for a form.
**
** Description:
**	Routine will cause any memory allocated for range querying
**	to be freed.  Most of the work will be done by FTrngend().
**	Note that not error detection is currently done by FTrngend()
**	since range querying is available only to QBF.  Exposing
**	range querying as a general feature will necessitate error
**	detection to be put into some FT routines.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If clean up was done.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	Memory allocated by a previous call to FTrnginit will be freed
**	via the FE*alloc routines.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
STATUS
IIrngend(formname)
char	*formname;
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FAIL);
	}
	FTrngend(frm);

	/*
	**  Going out of range query mode, need to set
	**  scrolling bits.
	*/
	IIFDsfsSetFieldScrolling(frm, TRUE);

	return(OK);
}


/*{
** Name:	IIunrng - Suspend range querying capability.
**
** Description:
**	Suspend range querying capability for a form.  Relies on FTunrng
**	to do the real work.  This routine is necessary to allow a form
**	to run normally (for update and browse mode of QBF) after a query
**	was entered by the user.
**	Same caveat as noted in IIrngend() applies here.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If suspension was accomplised.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
STATUS
IIunrng(formname)
char	*formname;
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FAIL);
	}

	FTunrng(frm);
	/*
	**  Going out of range query mode, need to set
	**  scrolling bits.
	*/
	IIFDsfsSetFieldScrolling(frm, TRUE);

	return(OK);
}




/*{
** Name:	IIrngon - Enable range querying.
**
** Description:
**	Reinstate range querying capability for a form to allow QBF
**	users to specify another query.  Same caveat as noted in
**	IIrngend() applies here.  Real work done by FTrngon.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If range querying was reenabled.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/85 (dkh) - Added procedure header.
*/
STATUS
IIrngon(formname)
char	*formname;
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FAIL);
	}
	FTrngon(frm);

	/*
	**  Going into range query mode, need to clear out
	**  scrolling bits.
	*/
	IIFDsfsSetFieldScrolling(frm, FALSE);

	return(OK);
}


/*{
** Name:	IIrnglast - Put up last set of range queries.
**
** Description:
**	This routine will cause the last query (entered via QBF) to
**	reappear in the fields again.  All the work is done by a
**	call to FTrnglast().  This routine must only be called
**	after a "## clear field all" statement has been executed.
**
**	Same caveat as noted in IIrngend() applies here.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If last query was put into the fields.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	Field values will be updated with previous range queries.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
STATUS
IIrnglast(formname)
char	*formname;
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FAIL);
	}
	FTrnglast(frm);
	return(OK);
}


/*{
** Name:	IIrvalid - Validate range queries.
**
** Description:
**	This routine is called by QBF to validate all the values
**	entered into fields.  Accepted values are the various
**	range query syntax that is docuemnted in the QBF manual.
**	All the work is done by FTrvalid().
**	Same caveat as noted in IIrngend() applies here.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		TRUE	If no syntax/field value error was found.
**		FALSE	If syntax or bad field values was found or
**			form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/12/87 (dkh) - Added procedure header.
**	09-dec-87 (kenl)
**		Changed IIrvalid so that it is passed (and passes on) two more
**		parameters, a function pointer and a structure pointer.  This
**		is needed for QBF to properly build the FROM clause so QG can
**		read a Master row for a Master/Detail JoinDef.
**	23-aug-88 (kenl)
**		Add paramter dml_level (see history comment at top).
*/
bool
IIrvalid(char *formname, STATUS (*bffunc)(), PTR bfvalue, i4 dml_level)
{
	FRAME	*frm;

	if (IIqbfutil(formname, &frm) != OK)
	{
		return(FALSE);
	}
	return(FTrvalid(frm, bffunc, bfvalue, dml_level));
}



/*{
** Name:	IIrgetinit - Get ready to unload range queries.
**
** Description:
**	Set up a form for unloading range queries.  All the work
**	done by FTrgetinit().  It is assumed that a call to
**	IIrvalid() has already been done and it returned TRUE. 
**	Var "IIqbffrm" is also set to point to the FRAME
**	structure for the specified form.  This will be used
**	when actual unloading occurs.
**	Same caveat as noted in IIrngend() applies here.
**
** Inputs:
**	formname	Name of form.
**
** Outputs:
**	Returns:
**		OK	If form is ready to be unloaded.
**		FAIL	If form does not exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	A copy of the range query for a field is also saved at this time.
**	IIqbffrm is set to the FRAME structure for the specified form.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/12/87 (dkh) - Added procedure header.
*/
STATUS
IIrgetinit(formname)
char	*formname;
{
	if (IIqbfutil(formname, &IIqbffrm) != OK)
	{
		return(FAIL);
	}
	FTrgetinit(IIqbffrm);
	return(OK);
}


/*{
** Name:	IIrnxtrow - Is there another range query row to unload.
**
** Description:
**	Checks to see if there is another range query row for the
**	specified table field to unload.  The form to use is
**	contained in "IIqbffrm".  Most of the work is done by
**	FTrnxtrow().  The "current" row is set to the new row
**	for unloading by a call to "IIrngget()".
**	Same caveat as noted in IIrngend() applies here.
**
** Inputs:
**	tablename	Name of table field.
**
** Outputs:
**	Returns:
**		TRUE	If there is another row for unloading
**		FALSE	If there are no more rows to unload.
**	Exceptions:
**		None.
**
** Side Effects:
**	Set up the current row for unloading via IIrngget().
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/11/87 (dkh) - Added procedure header.
*/
bool
IIrnxtrow(tablename)
char	*tablename;
{
	FRAME	*frm;
	FIELD	*fld;
	TBLFLD	*tbl;
	FLDHDR	*hdr;

	frm = IIqbffrm;
	if ((fld = FDfind((i4 **) frm->frfld, tablename, frm->frfldno,
		IIFDgfhGetFldHdr)) == NULL)
	{
		return(FALSE);
	}
	if (fld->fltag != FTABLE)
	{
		return(FALSE);
	}
	tbl = fld->fld_var.fltblfld;
	hdr = &(tbl->tfhdr);
	return(FTrnxtrow(frm, hdr->fhseq));
}

/*{
** Name:	IIrngget - Get range query for a field.
**
** Description:
**	Called exclusively by QBF, this routine builds a WHERE clause
**	qualification from the contents of the specified field.
**
**	Routine gets the range query for the specified field and
**	builds part of a where qualification clause for the
**	field.  No row number is necessary since we are always
**	unloading from the "current" row which has already been
**	set up by a call to IIrnxtrow().
**
**	The routine gets a pointer to the DB_LTXT_TYPE displayed field value, 
**	and parses that value, building several query specifications
**	based on the field content.  Multiple specifications may be
**	specified using 'and', 'or', and parentheses.  The default
**	query operator for each specification is '=', although others
**	are allowed when entered explicitly.  
**
**	The actual data values in the qualfication are converted to their
**	internal representation, using the field's format and datatype.
**	The DB_DATA_VALUE for the internal field value and become part of
**	the list of QRY_SPECs.  The textual parts of the qualfication
**	(i.e. 'and', 'or', query operators, table and column names)
**	are stored in the QRY_SPEC array as QS_TEXT type specifications.
**
**	Each QRY_SPEC in the list is passed to argument func for processing
**	as it is built.
**
**	WARNING:  This routine relies on other FT routines that manipulate
**	'scrollable' fields.  It should be called only by QBF, in the
**	context of those other FT routines.  See bldwhere.qc in QBF,
**	and iiqbf.c in RUNTIME for the details.
**
** Inputs:
**	fieldname	Name of field.
**	colname		Name of column if field is a table field.
**	dbtblname	Database table name for use in building qualification
**			clause.
**	dbcolname	Database column name for use in building qualifcation
**			clause.
**	dml_level		{nat}  DML compatiblity, UI_DML_QUEL, etc.	
**				(Passed through to 'FDrngchk()'.)
**
**	prefix		A text string to send before any other
**			QRY_SPEC.  If NULL don't send.
**
**	func		A function to call foreach QRY_SPEC.
**
**	func_param	A parameter to pass to the function given by func.
** Outputs:
**	Returns:
**		OK	If qualification clause was successfully built.
**		QG_NOSPEC
**			If no QRY_SPEC where generated for the field.
**		!= OK	If clause could not be built due to syntax error,
**			bad value conversion or specified field does not
**			exist.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/12/87 (dkh) - Added procedure header.
**	03/27/87 (drh) - Updated for ADTs, NULLs, new QG interface
**	5-may-1987 (Joe)
**		Changed to use new FDrngchk interface which no longer builds
**		an array of QRY_SPECs, but rather passes each QRY_SPEC as it
**		is built to the given function.
*/
static i2	IIqtag ZERO_FILL;

STATUS
IIrngget(char *fieldname, char *colname, char *dbtblname, char *dbcolname,
	i4 dml_level, char *prefix, STATUS (*func)(), PTR func_param)
{
	FRAME			*frm;
	FIELD			*fld;
	FLDCOL			*col;
	FLDHDR			*hdr;
	TBLFLD			*tbl;
	DB_DATA_VALUE		*display;
	i4			fldno;
	i4			colnum = 0;
	STATUS			rval;
	FUNC_EXTERN STATUS	FDrngchk();

	frm = IIqbffrm;
	if ((fld = FDfind((i4 **) frm->frfld, fieldname, frm->frfldno,
		IIFDgfhGetFldHdr)) == NULL)
	{
		return(FAIL);
	}
	if (fld->fltag == FTABLE)
	{
		tbl = fld->fld_var.fltblfld;
		hdr = &(tbl->tfhdr);
		fldno = hdr->fhseq;
		if ((col = FDfndcol(tbl, colname)) == NULL)
		{
			return(FAIL);
		}
		hdr = &(col->flhdr);
		colnum = hdr->fhseq;
	}
	else
	{
		hdr = FDgethdr(fld);
		fldno = hdr->fhseq;
	}

	/*
	**  Get ptr to the DB_DATA_VALUE for the longtext display buffer.
	*/

	if (IIqtag == 0)
	{
		IIqtag = FEgettag();
	}
	FTgetdisp( frm, fldno, colnum, IIqtag, 0, &display);
	rval =  FDrngchk( frm, fieldname, colname, display, (bool) TRUE, 
		dbtblname, dbcolname, (bool) FALSE, dml_level,
		prefix, func, func_param);
	FEfree(IIqtag);
	return rval;
}
# endif		/* PCINGRES */


/*{
** Name:	IIqbfval - Turn off data type checking for QBF.
**
** Description:
**	This routine is called by QBF to turn off data type checking
**	so the sort/order specification part of QBF can work.  Only
**	QBF is allowed to call this routine which essentially turns
**	all fields into a character data type temporarily.  A call
**	to FDqbfval is done to set/reset data type checking in the
**	FD layer and a call to FTqbfval does the same for the FT
**	layer.
**
** Inputs:
**	value	Validation value to set.  TRUE turns off any data type
**		checking.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	09/27/85 (dkh) - Initial version.
**	02/12/87 (dkh) - Added procedure header.
*/
IIqbfval(value)
bool	value;
{
	FDqbfval(value);
	FTqbfval(value);
}

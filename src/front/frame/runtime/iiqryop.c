/*
**	iiqryop.c
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
** Name:	iiqryop.c
**
** Description:
**
**	Public (extern) routines defined:
**		IIgetqry()	- pre 6.0 compatability cover routine
**		IIgtqryio()	- external interface to get query op
**	Private (static) routines defined:
**
** History:
**	19-jun-87 (bab)	Code cleanup.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	STATUS	adh_dbcvtev();


/*{
** Name:	IIgetqry	-	getoper for query mode field
**
** Description:
**	Get the query operator from a field and return it to the caller.
**
**	This routine is part of RUNTIME's external interface as a
**	compatability cover for pre-6.0 EQUEL programs.
**
** Inputs:
**	isvar		Whether data is variable or not
**	type		Type of user variable
**	len		Length of user variable
**	variable	User variable to receive the oper value
**	name		Name of field to get the oper from
**
** Outputs:
**	variable	Will be udpated with the code for the oper
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	03-feb-1986  -  Added support for ADTs and NULLs (drh)
**	25-feb-1987  -  Converted to a compatability cover for 6.0 (drh)
*/

i4
IIgetqry(i4 isvar, i4 type, i4 len, i4 *variable, char *name )
{
	return( IIgtqryio( (i2 *) 0, isvar, type, len, variable, name));

}


/*{
** Name:	IIgtqryio	-	getoper for query mode field
**
** Description:
**	Return the query operator for a query mode field to the caller.
**
**	Confirms that the named field exists, then builds an embedded-
**	data-type from the caller's parameters. 
**	If the form is in query mode, get the query operator for the
**	field, otherwise, return NOP.  Call adh_dbcvtev to convert the
**	query operator value into the embedded value.
**
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	ind		Null indicator
**	isvar		Whether data is variable or not
**	type		Type of user variable
**	len		Length of user variable
**	data		User variable to receive the oper value
**	name		Name of field to get the oper from
**
** Outputs:
**	data		Will be udpated with the code for the oper
**
**
** Returns:
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	25-feb-1987	Created with code extracted from IIgetqry, modified
**			for ADTs and NULLs (drh)
*/

i4
IIgtqryio(ind, isvar, type, len, variable, name )
i2	*ind;
i4	isvar;
i4	type;
i4	len;
i4	*variable;			/* pointer to variable to fill */
char	*name;				/* name of field to retrieve from */
{
	char			*namestr;
	char			fbuf[MAXFRSNAME+1];
	DB_EMBEDDED_DATA	edv;
	i4			retval;
	DB_DATA_VALUE		operdbv;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized and that there is a current frame.
	*/
	if (!RTchkfrs(IIfrmio))
		return (FALSE);

	if ( !isvar )	/* Error by preprocessor */
		return FALSE;

	if ((namestr = IIstrconv(II_CONV, name, fbuf, (i4)MAXFRSNAME)) == NULL)
	{
		IIFDerror(RTRFFL, 2, IIfrmio->fdfrmnm, ERx(""));
		return(FALSE);
	}

	/*
	**	Build an EDV from the user's parameters
	*/

	edv.ed_type = DB_INT_TYPE;
	edv.ed_length = len;
	edv.ed_data = (PTR) variable;
	edv.ed_null = ind;


	/*
	**	Build a DBV to hold the oper
	*/

	operdbv.db_datatype = DB_INT_TYPE;
	operdbv.db_length = sizeof( i4  );
	operdbv.db_prec = 0;
	operdbv.db_data = (PTR) &retval;

	/*
	**	Check to make sure the frame is in QUERY mode.
	**	If not, return no operation.
	*/

	if (IIfrmio->fdrunmd == fdrtQRY)
	{
		if (!FDqryop(IIfrmio->fdrunfrm, namestr, (i4 *) 
			operdbv.db_data))
			retval = fdNOP;
	}
	else
	{
		retval = fdNOP;
	}

	/*
	**  Convert the query operator to an embedded-data-value
	*/

	if ( adh_dbcvtev( FEadfcb(), &operdbv, &edv ) != OK )
	{
		IIFDerror(RTGOERR, 2, IIfrmio->fdrunfrm, namestr);
		return( FALSE );
	}
	else
		return( TRUE );
}

/*
**	iivalcol.c
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
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iivalcol.c	-	validate a row/column
**
** Description:
**
**	Public (extern) routines defined:
**		IItcolval()
**		IItvalval()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/11/87 (dkh) - Fixed validrow to work correctly.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

GLOBALREF	TBSTRUCT	*IIcurtbl;

static 	i4	validated = TRUE;

/*{
** Name:	IItcolval	-	Validate a column/row value
**
** Description:
**	Validate the value in tablefield cell (row/column) by
**	calling FDckval.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	colname		Name of column to validate
**
** Outputs:
**
** Returns:
**	i4	TRUE	if column value is valid
**		FALSE	if not valid
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	31-mar-1983 	- Written (ncg)
**	24-jan-1984	- Static validation flag for rewrite (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**
*/


i4
IItcolval(char *colname)
{
	char			*colptr;	/* ptr to column name */
	char			c_buf[MAXFRSNAME+1];

	if (IIfrmio == NULL)
	{
		/* no form is currently active to run Equel/Forms */
		IIFDerror(RTFRACT, 0, NULL);
		return (FALSE);
	}

	/* skip following validations if previous column was not validated */
	if (!validated)
		return (FALSE);

	if ((colptr=IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME)) == NULL)
	{
		/* no column name given for table I/O statement */
		IIFDerror(TBNOCOL, 1, IIcurtbl->tb_name);
		return (FALSE);
	}

	/* validate displayed column */
	if (!FDckcol(IIcurtbl->tb_fld, IIcurtbl->tb_rnum - 1, colptr))
	{
		validated = FALSE;
		return (FALSE);
	}
	return (TRUE);
}



/*{
** Name:	IItvalval	-	Get/set validated flag
**
** Description:
**	Updates or returns the value of the static 'validated' flag.
**
** 	Set to TRUE from IItsetup at the startup of a validrow.
**	Returns validation value when called from user program.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	set		Flag - non-zero means set flag to validated
**
** Outputs:
**
** Returns:
**	i4	value of the static validated flag
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

i4
IItvalval(i4 set)
{
	if (set)
	{
		validated = TRUE;
		return (TRUE);
	}
	else
	{
		return (validated);
	}
}

/*{
** Name:	IItvalrow	-	validate table field row
**
** Description:
** 	Validate row in given table in the displayed frame.  
**	Return TRUE if okay.
** 
**	Check that the frame and the table field are set up, then 
**	validate.
** 
**	Calling this routine with argument rowALL is equivalent to 
**	## VALIDATE FIELD <table> (Does not apply anymore.)
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
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
** Side Effects:
**
** History:
**	15-mar-1983 	- written (ncg)
**	30-apr-1984	- Improved interface to FD routines (ncg)
**	11/11/87 (dkh) - Fixed validrow to work correctly.
*/

i4
IItvalrow(void)
{
	if (IIfrmio == NULL)
	{
		/* no form is currently active to run Equel/Forms */
		IIFDerror(RTFRACT, 0, NULL);
		return (FALSE);
	}
	/* validate displayed row */
	if (!FDckrow(IIcurtbl->tb_fld, IIcurtbl->tb_rnum -1))
	{
		validated = FALSE;
		return(FALSE);
	}
	return(TRUE);
}

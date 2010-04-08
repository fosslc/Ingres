/*
**	iiscrlbrk.c
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
** Name:	iiscrlbrk.c
**
** Description:
**	Support for activate on scroll up or down
**
**	Public (extern) routines defined:
**		IIactscrl()
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	25-apr-96 (chech02)
**		Added function type i4  to IIactscrl() for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIactscrl	-	Define an activate on scroll
**
** Description:
**	An EQUEL '## ACTIVATE SCROLL' statement will generate a call
**	to this routine.  Sets the table's scroll activation code
**	to the value passed.
**
**	Uses IIstkfrm as the current form, and looks for the named
**	table in that form.
**	The TBSTRUCT has an array scrintrp[] where cell 0 stores
**	the SCROLLUP interrupt value and cell 1 stores the SCROLLDOWN
**	interrupt value.  Both cells are initialized to 0, as Equel
**	interrupt values start at 1.
**
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	tabname		Name of tablefield
**	mode		Flag - up or down
**	value		Activation code to set
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
**	## activate scrollup t1
**
**	IIactscrl("t1", scrlUP, activate_val);
**
** Side Effects:
**
** History:
**	04-mar-1983 	- written (ncg)
**     	14-feb-1984	- don't activate scroll if table
**			  field name is null. (nml)
**	
*/

i4
IIactscrl(char *tabname, i4 mode, i4 val)
{
	register TBSTRUCT	*tb;
	char			*bufptr;	/* ptr to table name */
	char			t_buf[MAXFRSNAME+1];

	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	bufptr = IIstrconv(II_CONV, tabname, t_buf, (i4)MAXFRSNAME);

	/* search for table on current displayed frame's table list */
	if (bufptr == NULL || *bufptr == '\0')
	{
		/* allow for no name of table field - don't activate */
		return(TRUE);
	}

	if ( (tb = IItfind(IIstkfrm, bufptr) ) == NULL )
	{
		/* table field isn't within given frame */
		IIFDerror(TBBAD, 2, (char *) bufptr, IIstkfrm->fdfrmnm);
		return (FALSE);
	}

	/* set scroll interrupt value */
	tb->scrintrp[mode] = val;
	return (TRUE);
}

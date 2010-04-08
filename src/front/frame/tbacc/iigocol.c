/*
**	iigotocol.c
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
** Name:	iigotocol.c
**
** Description:
**	Routines for resuming to a column
**
**	Public (extern) routines defined:
**		IIrescol()
**	Private (static) routines defined:
**
** History:
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/


/*{
** Name:	IIrescol	-	Resume on a column
**
** Description:
**	Provided with a tablefield name and column name, resume to
**	the column.  Calls FDgotocol to do the work.
**
**	An EQUEL '## RESUME COLUMN' statement will generate a call
**	to this routine.
**	This routine is part of TBACC's external interface.
**	
** Inputs:
**	tablename	Name of tablefield
**	colname		Name of column to resume to
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
** Examples and Code Generation:
**	## resume column t1 col2
**
**      IIrescol("t1", "col2");
**	goto IIfdBegin1;
**
** Side Effects:
**
** History:
**		18-nov-1983	- Written (ncg)
**		29-mar-1984	- Fix code to check for TRUE/FALSE
**				  on FDgotocol return instead of OK.
**		30-apr-1984	- Improved interface to FD routines (ncg)
*/

IIrescol(tabname, colname)
char	*tabname;
char	*colname;
{
	TBSTRUCT	*tb;
	char		*tabbuf;
	char		*colbuf;
	char		t_buf[MAXFRSNAME+1];
	char		c_buf[MAXFRSNAME+1];

	/*
	** Validate form state and convert string arguments.
	*/
	if (!RTchkfrs(IIstkfrm))
		return (FALSE);

	tabbuf = IIstrconv(II_CONV, tabname, t_buf, (i4)MAXFRSNAME);
	colbuf = IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME);

	/*
	** Search for table on current displayed frame's table list.
	*/
	if ((tb = IItfind(IIstkfrm, tabbuf)) == NULL )
	{
		IIFDerror(TBBAD, 2, (char *) tabbuf, IIstkfrm->fdfrmnm);
		return (FALSE);
	}
	if (!FDgotocol(IIstkfrm->fdrunfrm, tb->tb_fld, colbuf))
	{
		/* no column to resume on */
		IIFDerror(RTRESCOL, 2, (char *) tabbuf, colbuf);
		return(FALSE);	
	}
	return (TRUE);
}

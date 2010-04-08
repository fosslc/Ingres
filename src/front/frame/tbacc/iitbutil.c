/*
**	iitbutil.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
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
# include	<cm.h>
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iitbutil.c	-	Table field utilities
**
** Description:
**
**	Public (extern) routines defined:
**		IItstart()	Start up table field i/o
**		IItfind()	Return a table on a frame
**		IIcdget()	Get a column descriptor
**	Private (static) routines defined:
**
** History:
**	4-mar-1983 	- written (ncg)
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	12/23/87 (dkh) - Performance changes.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Update prototypes for gcc 4.3.
*/

/*{
** Name:	IItstart	-	Start tablefield i/o
**
** Description:
**	Update i/o frame, and finds a table corresponding to
**	the gien table name, and returns it.
**
**	Call IIsetio() for the frame i/o.
**
** Inputs:
**	tabname		Name of the tablefield
**	formname	Name of the form
**
** Outputs:
**
** Returns:
**	Ptr to the TBSTRUCT for the tablefield.
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

TBSTRUCT *
IItstart(const char *tabname, const char *formname)
{
	TBSTRUCT	*tb;			/* table field to return */
	char		*bufptr;		/* pointer to table buffer */
	char		buf[MAXFRSNAME+1];

	/* check that general I/O frame is set, and update IIfrmio */
	if (IIfsetio(formname) == 0)
	{
		return (NULL);
	}
	/* valid I/O frame */
	if (IIfrmio == NULL)
	{
		IIFDerror(RTFRACT, 0, NULL);
		return (NULL);
	}
	/* table field name must be given */
	if ((bufptr=IIstrconv(II_CONV, tabname, buf, (i4)MAXFRSNAME)) == NULL)
	{
		IIFDerror(TBNONE, 0);
		return (NULL);
	}

	/* is table field within the given frame */
	if ((tb = (TBSTRUCT *) IItfind(IIfrmio, bufptr)) == NULL)
	{
		IIFDerror(TBBAD, 2, (char *) bufptr, IIfrmio->fdfrmnm);
		return (NULL);
	}
	return (tb);
}

/*{
** Name:	IItfind	-	Find tblfld in frame's tbl list
**
** Description:
**	Given the runtime frame, find a table on the frame's table
**	list using the name provided.  Retrun a pointer to the
**	table field if found, else NULL.
**
** Inputs:
**	runf		Ptr to the frame
**	tname		Name of the tablefield to find
**
** Outputs:
**
** Returns:
**	Ptr to the TBSTRUCT for the tablefield, if found, NULL otherwise.
**	i4	TRUE
**		FALSE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

TBSTRUCT *
IItfind(RUNFRM *runf, const char *tname)
{
	register TBSTRUCT	*tb;

	if (tname == NULL)
	{
		return (NULL);
	}
	
	for (tb = runf->fdruntb; tb != NULL; tb = tb->tb_nxttb)
	{
		if (STcompare(tname, tb->tb_name) == 0)
			break;
	}
	return ((TBSTRUCT *) tb);
}		


/*{
** Name:	IIcdget		-	Get a column descriptor
**
** Description:
**	Given the name of a column, find the column's descriptor
**	in the tablefield and return it to the caller.
**
** Inputs:
**	tb		Tablefield descriptor
**	cname		Name of the column to find
**
** Outputs:
**
** Returns:
**	Ptr to the COLDESC for the column, or NULL if not found
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

COLDESC *
IIcdget(tb, cname)
TBSTRUCT			*tb;
char				*cname;
{
	register ROWDESC	*rd;		/* row descriptor for table */
	register COLDESC	*cd;		/* column desc to return */

	rd = tb->dataset->ds_rowdesc;

	/* search list of column descriptors for displayed columns */
	for (cd = rd->r_coldesc; cd != NULL; cd = cd->c_nxt)
	{
		if (CMcmpcase(cd->c_name, cname) != 0)
		{
			continue;
		}
		if (STcompare(cd->c_name, cname) == 0)
		{
			return (cd);
		}
	}
	/* search list of column descriptors for hidden columns */
	for (cd = rd->r_hidecd; cd != NULL; cd = cd->c_nxt)
	{
		if (CMcmpcase(cd->c_name, cname) != 0)
		{
			continue;
		}
		if (STcompare(cd->c_name, cname) == 0)
		{
			return (cd);
		}
	}
	return (NULL);
}

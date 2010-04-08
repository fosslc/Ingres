/*
**	iiinvis.c
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
# include	<rtvars.h>

/**
** Name:	iiinvis.c	-	'Invisible' table fields
**
** Description:
**	Contains (undocumented, unsupported) routines to make table
**	fields appear and disappear.
**
**	Public (extern) routines defined:
**		IIutinvis()
**		IItfinvis()
**		IItfvis()
**	Private (static) routines defined:
**
** History:
**	Created - 08/21/85 (dkh)
**	19-sep-89 (bruceb)
**		Use fdINVISIBLE instead of fdTFINVIS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

FUNC_EXTERN	FIELD	*FDfndfld();


/*{
** Name:	IIutinvis	-	Utility routine for invisible tfs
**
** Description:
**	Seems to be a utility routine for IItfinvis and IItfvis.
**	Given a formname and tablefield name, returns a ptr to
**	the frame and the tablefield, as well as an indicator as
**	to whether the field is 'current'.
**
** Inputs:
**	formname	Name of the form
**	tblname		Name of the table field
**
** Outputs:
**	frmp		Updated with frame ptr for the form
**	fldp		Updated with ptr to the field
**	cur		TRUE if the field is current
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

STATUS
IIutinvis(formname, tblname, frmp, fldp, cur)
char	*formname;
char	*tblname;
FRAME	**frmp;
FIELD	**fldp;
bool	*cur;
{
	RUNFRM	*rfrm;
	FRAME	*frm;
	FIELD	*fld;
	bool	nonseq;
	bool	current = TRUE;

	if ((rfrm = RTfindfrm(formname)) == NULL)
	{
		return(FAIL);
	}
	if (rfrm->fdrunfrm != IIstkfrm->fdrunfrm)
	{
		current = FALSE;
	}

	frm = rfrm->fdrunfrm;

	if ((fld = FDfndfld(frm, tblname, &nonseq)) == NULL)
	{
		return(FAIL);
	}

	if (fld->fltag != FTABLE)
	{
		return(FAIL);
	}

	*frmp = frm;
	*fldp = fld;
	*cur = current;
	return(OK);
}

/*{
** Name:	IItfinvis	-	Make tablefield invisible
**
** Description:
**
** Inputs:
**	formname	Name of form
**	tblname		Name of table field to make invisible
**
** Outputs:
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/


STATUS
IItfinvis(formname, tblname)
char	*formname;
char	*tblname;
{
	FRAME	*frm;
	FIELD	*fld;
	FLDHDR	*hdr;
	TBLFLD	*tbl;
	STATUS	stat;
	i4	seqno;
	bool	current = FALSE;

	if ((stat = IIutinvis(formname, tblname, &frm, &fld, &current)) == FAIL)
	{
		return(stat);
	}

	tbl = fld->fld_var.fltblfld;
	hdr = &(tbl->tfhdr);

	/*
	**  If table field is already invisible, then just return.
	*/
	if (hdr->fhdflags & fdINVISIBLE)
	{
		return(stat);
	}

	hdr->fhdflags |= fdINVISIBLE;

	if (current)
	{
		/*
		**  For now, assume table fields are on the array of
		**  updatable fields.
		*/
		seqno = hdr->fhseq;
		FTtfinvis(frm, seqno, FT_UPDATE);
	}

	return(stat);
}

/*{
** Name:	IIfivis		-	Make invisible tblfld visible
**
** Description:
**
** Inputs:
**	formname		Name of the form
**	tblname			Name of the tblfld to make visible
**
** Outputs:
**
** Returns:
**	STATUS
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

STATUS
IItfvis(formname, tblname)
char	*formname;
char	*tblname;
{
	FRAME	*frm;
	FIELD	*fld;
	FLDHDR	*hdr;
	TBLFLD	*tbl;
	STATUS	stat;
	i4	seqno;
	bool	current = FALSE;

	if ((stat = IIutinvis(formname, tblname, &frm, &fld, &current)) == FAIL)
	{
		return(stat);
	}

	tbl = fld->fld_var.fltblfld;
	hdr = &(tbl->tfhdr);

	/*
	**  If table field is already visible, then just return.
	*/
	if (!(hdr->fhdflags & fdINVISIBLE))
	{
		return(stat);
	}

	hdr->fhdflags &= ~fdINVISIBLE;

	if (current)
	{
		/*
		**  For now, assume table fields are on the array of
		**  updatable fields.
		*/
		seqno = hdr->fhseq;
		FTtfvis(frm, seqno, FT_UPDATE);
	}

	return(stat);
}

/*
**	iifbrk.c
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
# include	<er.h>
# include	<rtvars.h>

/**
** Name:	iifbrk.c	-	Define field activation
**
** Description:
**
**	Public (extern) routines defined:
**		IIactfld()	- Define field activation (backward compatible
**				  routine.)
**		IIFRafActFld()	- Define 'before' and 'after' field activations.
**	Private (static) routines defined:
**
** History:
**	16-feb-1983  -  Extracted from original runtime.c (jen)
**	11-apr-1983  -	Checks for table fields, does nothing (ncg)
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	11/11/87 (dkh) - Code cleanup.
**	22-feb-89 (bruceb)
**		Made IIactfld() a cover for the new IIFRafActFld() routine.
**		New routine takes extra parameter indicating whether
**		an entry or exit activation.
**	26-sep-89 (bruceb)
**		Defining an activation on a derived field generates a warning.
**	08/08/91 (dkh) - Fixed bug 37385.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	FLDHDR	*FDgethdr();
i4			IIFRafActFld();


/*{
** Name:	IIactfld	-	Define a field activation
**
** Description:
**	Cover routine for IIFRafActFld() -- for backward compatability.
**
**	This routine is part of the external interface to RUNTIME.  
**	
** Inputs:
**	strvar		Name of the field having activation defined
**	val		Activation code for the field
**
** Outputs:
**
** Returns:
**	Value of IIFRafActFld().
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	23-feb-89 (bruceb)
**		Made cover routine for IIFRafActFld().
*/
i4
IIactfld(char *strvar, i4 val)
{
    return (IIFRafActFld(strvar, FALSE, val));
}

/*{
** Name:	IIFRafActFld	- Define entry and exit field activations
**
** Description:
**	Sets the 'before' or 'after' activation interrupt code for the field
**	passed.  This value is passed up to user code on entering (or leaving)
**	the field.  Note that only the frfld[] array can be activated as
**	frnsfld[] is display only.
**
**	This routine is part of the external interface to RUNTIME.  
**	
** Inputs:
**	strvar		Name of the field having activation defined
**	entry_act	Is this an entry or exit activation?
**	val		Activation code for the field
**
** Outputs:
**
** Returns:
**	i4	TRUE	Activation defined successfully
**		FALSE	Error in activation definition
**
** Exceptions:
**	none
**
** Example and Code Generation:
**	## activate field "field1"
**
**	IIFRafActFld("field1", 0, activate_val);
**
**	## activate before field "field1"
**
**	IIFRafActFld("field1", 1, activate_val);
**
** Side Effects:
**
** History:
**	23-feb-89 (bruceb)
**		Created, mostly from old IIactfld() source.
*/
i4
IIFRafActFld(char *strvar, i4 entry_act, i4 val)
{
    i4		i;
    FRAME	*frm;
    FIELD	*fld;
    FLDHDR	*hdr;
    char	*name;
    char	fbuf[MAXFRSNAME+1];
    bool	all = FALSE;

    /*
    **	Check all variables make sure they are valid.
    **	Also check to see that the forms system has been
    **	initialized and that there is a current frame.
    */
    name = IIstrconv(II_CONV, strvar, fbuf, (i4)MAXFRSNAME);
    if (name == NULL || *name == EOS)
	return (TRUE);

    frm = IIstkfrm->fdrunfrm;

    /*
    **	If the name "ALL" has been passed, set the interrupt
    **	value on all simple fields, else only on the specified field.
    **
    **	WARNING: This does nothing with table fields.
    */
    if (STcompare(name, ERx("all")) == 0)
	all = TRUE;

    for (i = 0; i < frm->frfldno; i++)
    {
	fld = frm->frfld[i];

	/*
	**  Can't use call to FDgethdr() here since that will
	**  look into the column headers if the field is a table
	**  field.  Looking into column headers will ruin the
	**  name space separation between columns and simple/table fields.
	*/
	if (fld->fltag == FTABLE)
	{
		hdr = &(fld->fld_var.fltblfld->tfhdr);
	}
	else
	{
		hdr = &(fld->fld_var.flregfld->flhdr);
	}

	if (all || (STcompare(hdr->fhdname, name) == 0))
	{
	    if (fld->fltag != FTABLE)
	    {
		if (entry_act)
		    hdr->fhenint = val;
		else
		    hdr->fhintrp = val;
	    }

	    if (!all)
	    {
		if (hdr->fhd2flags & fdDERIVED)
		    IIFDerror(E_FI2269_8809_ActDerivedFld, 1, name);
		return (TRUE);
	    }
	}
    }
    if (all)
	return (TRUE);

    /*
    **	Not found, so return error.
    */
    IIFDerror(RTFBFL, 2, IIstkfrm->fdfrmnm, name);
    return (FALSE);
}

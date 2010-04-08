/*
**	iicolbrk.c
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
** Name:	iicolbrk.c
**
** Description:
**	Define an activation for a tablefield column
**
**	Public (extern) routines defined:
**		IIactclm()	- Backward compat routine for column activation.
**		IITBcaClmAct()	- 'Before' and 'after' column activation.
**	Private (static) routines defined:
**
** History:
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	23-feb-89 (bruceb)
**		Made IIactclm() a cover for the new IITBcaClmAct() routine.
**		New routine takes extra parameter indicating whether
**		an entry or exit activation.
**	26-sep-89 (bruceb)
**		Defining an activation on a derived column generates a warning.
**	04-oct-89 (bruceb)
**		Allow NULL or "" column name--also a no-op.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	FLDCOL		*RTgetcol();


/*{
** Name:	IIactclm	-	Define tblfld column activation
**
** Description:
**	Cover routine for IITBcaClmAct() -- for backward compatability.
**
**	This routine is part of TBACC's external interface.
**
** Inputs:
**	tabname		Name of the tablefield
**	colname		Name of the column, or "all"
**	val		Activion code value for the column
**
** Outputs:
**
** Returns:
**	Value of IITBcaClmAct().
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	23-feb-89 (bruceb)
**		Made cover routine for IITBcaClmAct().
*/
i4
IIactclm(char *tabname, char *colname, i4 val)
{
    return (IITBcaClmAct(tabname, colname, FALSE, val));
}

/*{
** Name:	IITBcaClmAct	- Define entry and exit tblfld column activation
**
** Description:
**	Sets the 'before' or 'after' activation interrupt code for the
**	tablefield column specified.  This value is passed up to user code
**	on entering (or leaving) the column.
**
** Inputs:
**	tabname		Name of the tablefield
**	colname		Name of the column, or "all"
**	entry_act	Is is an entry or exit activation?
**	val		Activion code value
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
**	## activate column t1 col2
**
**	IITBcaClmAct("t1", "col2", 0, activate_val);
**
**	## activate before column t1 col2
**
**	IITBcaClmAct("t1", "col2", 1, activate_val);
**
** Side Effects:
**
** History:
**	11-apr-1983 	- written (ncg)
**	18-jul-1985	- added "all" option (ncg)
**	23-feb-89  (bruceb)
**		Created, mostly from old IIactclm() source.
*/
i4
IITBcaClmAct(char *tabname, char *colname, i4 entry_act, i4 val)
{
    i4  	i;
    char	*colbuf;	/* pointer to column name    */
    char	*tbbuf;		/* pointer to table name   */
    FLDCOL	*col;		/* specific column descriptor */
    TBSTRUCT	*tb;
    TBLFLD	*tfld;
    char	c_buf[MAXFRSNAME+1];
    char	t_buf[MAXFRSNAME+1];


    /*
    ** Validate forms state, and convert string arguments.
    */
    if (!RTchkfrs(IIstkfrm))
	return (FALSE);

    tbbuf = IIstrconv(II_CONV, tabname, t_buf, (i4)MAXFRSNAME);
    if (tbbuf == NULL || *tbbuf == EOS)
    {
	/* allow for null table name as a NOP */
	return (TRUE);
    }
    colbuf = IIstrconv(II_CONV, colname, c_buf, (i4)MAXFRSNAME);
    if (colbuf == NULL || *colbuf == EOS)
    {
	/* allow for null column name as a NOP */
	return (TRUE);
    }

    /*
    ** Search for table on current displayed frame's table list.
    */
    if ((tb = IItfind(IIstkfrm, tbbuf)) == NULL )
    {
	IIFDerror(TBBAD, 2, (char *) tbbuf, IIstkfrm->fdfrmnm);
	return (FALSE);
    }

    /* Check if user wants to activate all columns */
    if (STcompare(colbuf, ERx("all")) == 0)
    {
	tfld = tb->tb_fld;

	if (entry_act)
	{
	    for (i = 0; i < tfld->tfcols; i++)
	    {
		tfld->tfflds[i]->flhdr.fhenint = val;
	    }
	}
	else
	{
	    for (i = 0; i < tfld->tfcols; i++)
	    {
		tfld->tfflds[i]->flhdr.fhintrp = val;
	    }
	}
	return (TRUE);
    }
    /* 
    ** Fetch column in table and set the interrupt value.
    */
    if ((col = RTgetcol(tb->tb_fld, colbuf)) == NULL)
    {
	IIFDerror(TBBADCOL, 2, (char *) tbbuf, colbuf);
	return (FALSE);
    }

    if (entry_act)
	col->flhdr.fhenint = val;
    else
	col->flhdr.fhintrp = val;
    
    if (col->flhdr.fhd2flags & fdDERIVED)
	IIFDerror(E_FI226A_8810_ActDerivedCol, 1, colbuf);

    return (TRUE);
}

/*
**	iifrminit.c
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
# include	<me.h>
# include	<st.h>
# include	<eqlang.h>
# include	<lqgdata.h>
# include	<er.h>

/**
** Name:	iifrminit.c	-	Get form from database
**
** Description:
**
**	Public (extern) routines defined:
**		IIforminit()
**
**	Private (static) routines defined:
**
** History:
**	16-feb-1983  -	Extracted from original runtime.c (jen)
**	24-feb-1983  -	Added table field implementation  (ncg)
**	12-20-84 (rlk) - Added calls to FEbegintag and FEendtag.
**	11-feb-1985  -	moved code common to IIaddfrm to new
**			routine IIaddrun().  (bab)
**	06/16/87 (dkh) - Integrated FElocktag()/FEtaglocked() usage
**			 from PC for bobm.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	10/28/88 (dkh) - Performance changes.
**	16-mar-89 (bruceb)
**		Added call on IIFRaff() after a successful IIaddrun().
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	VOID	IIFRaffAddFrmFrres2();


/*{
** Name:	IIforminit	-	Retrieve form from database
**
** Description:
**	This routine retrieves a frame from the database and initializes
**	it along with its storage areas and windows.  It then adds the
**	runtime information necessary.
**
**	This is primarily a cover routine to establish a tag region
**	around the form retrieval and set up.  The actual work is 
**	done by calling FDfrcreate and IIaddrun.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	nm		name of the form
**
** Outputs:
**
** Returns:
**	i4	TRUE	If form was added successfully
**		FALSE	If form could not be added
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

IIforminit(nm)
char	*nm;
{
	FRAME		*frm;
	char		fbuf[FE_MAXNAME+1];
	reg	char	*name;
	FRAME		*FDfrcreate();
	i2		frmtag = -1;		/* Memory tag-id for frame */
	i4		savelang;
	i4		ret_stat;
	i4		IIlang();

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized.
	*/
	if (nm == NULL)
		return (FALSE);
	name = IIstrconv(II_CONV, nm, fbuf, (i4)MAXFRSNAME);

	IIsetferr((ER_MSGID) 0);
	if(!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return (FALSE);
	}

	/*
	**	Call FDfrcreate to retrieve the frame.	This routine
	**	does all the dirty work. Start a new (unique) tagged
	**	memory region for the allocated frame.
	*/

	if (!FEtaglocked())
	{
		frmtag = FEbegintag();
	}
	savelang = IIlang(hostC);

	if((frm = FDfrcreate(name)) == NULL)
	{
		if (frmtag > 0)
		{
			FEendtag();
		}
		IIlang(savelang);
		return (FALSE);
	}
	IIlang(savelang);

	if (frmtag)
	{
		frm->frtag = frmtag;
	}

	if (ret_stat = IIaddrun(frm, name))
	{
		IIFRaffAddFrmFrres2(frm);
	}

	if (frmtag > 0)
	{
		FEendtag();
	}

	return(ret_stat);
}

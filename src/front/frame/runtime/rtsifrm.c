/*
**	rtsifrm.c
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
# include	"setinq.h"
# include	<er.h>

/**
** Name:	rtsifrm.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTinqfrm()
**		RTsetfrm()
**	Private (static) routines defined:
**
** History:
**	05/02/87 (dkh) - Integrated change bit code.
**	08/14/87 (dkh) - ER changes.
**	09/01/87 (dkh) - Changed <eqforms.h> to "setinq.h"
**	10/21/90 (dkh) - Fixed bug 33996.
**	07/20/92 (dkh) - Added support for inquiring on existence of
**			 a form.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

FUNC_EXTERN	char	*FDfldnm();

/*{
** Name:	RTinqfrm	-	Inquire about frame attributes
**
** Description:
**	Entry point for supporting the inquire_frs on a form
**	statement.  Supported options are:
**	 - Was a change made to the form by the user. (integer)
**	 - Display mode of the form. (string)
**	 - Current field of the form (i.e., field cursor is on). (string)
**	 - Name of the form. (string)
**
** Inputs:
**	runf	Pointer to a RUNFRM structure.
**	frm	Pointer to a FRAME structure.
**	frsflg	Desired operation to perform.
**
** Outputs:
**	data	Data space to place output of desired operation.  Output
**		may be a string (null terminated) or an integer.
**
**	Returns:
**		TRUE	If desired operation was performed successfully.
**		FALSE	If unknown operation was requested.
**
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (jen) - Initial version.
**	02/13/87 (dkh & drh) - Added procedure header.
**	
*/
i4
RTinqfrm(RUNFRM *runf, FRAME *frm, i4 frsflg, i4 *data)
{
	switch (frsflg)
	{
	  case frsFRMCHG:
		*data = frm->frchange;
		break;

	  case frsFLDMODE:
		RTgetmode(runf, (char *) data);
		break;

	  case frsFLD:
		STcopy(FDfldnm(frm), (char *) data);
		break;

	  case frsFLDNM:
		STcopy(runf->fdfrmnm, (char *) data);
		break;

	  case frsSROW:
		*data = frm->frposy;
		break;

	  case frsSCOL:
		*data = frm->frposx;
		break;

	  case frsHEIGHT:
		*data = frm->frmaxy;
		break;

	  case frsWIDTH:
		*data = frm->frmaxx;
		break;

	  case frsEXISTS:
		if (runf != NULL)
		{
			*data = 1;
		}
		else
		{
			*data = 0;
		}
		break;

	  default:
		return (FALSE);			/* preprocessor bug */
	}
	return (TRUE);
}


/*{
** Name:	RTsetfrm	-	Set frame attributes
**
** Description:
**	Entry point for supporting the set_frs on a form statement.
**	Supported options are:
**	 - Set/reset the change bit for a form. (integer)
**	 - Set the mode of a form.  Valid values are "read", "fill",
**	   "query", and "update". (string)
**
** Inputs:
**	runf	Pointer to a RUNFRM structure.
**	frm	Pointer to a FRAME structure.
**	frsflg	Desired operation to perform.
**	dataptr	Pointer to data for use in desired operation.  May be
**		a null terminated string or an integer.
**
** Outputs:
**	Returns:
**		TRUE	If requested operation was successful.
**		FALSE	If an unknown operation or unknown mode
**			was requested.
**
**	Exceptions:
**		none
**
** Side Effects:
**	None.
**
** History:
**	xx/xx/xx (jen) - Initial version.
**	02/13/87 (dkh & drh) - Added procedure header.
*/
i4
RTsetfrm(RUNFRM *runf, FRAME *frm, i4 frsflg, char *dataptr)
{
	i4	*natptr;

	natptr = (i4 *) dataptr;

	switch (frsflg)
	{
	  case frsFRMCHG:
		frm->frchange = *natptr;
		/*
		**  Don't cascade form change bit value
		**  since this may be quite expensive when
		**  dealing with table fields with huge
		**  datasets.
		*/
		break;

	  case frsFLDMODE:
		/*
		**  Just mark fields invalid instead of changed
		**  and invalid.  This will solve some problems
		**  with abf that performs mode changes liberally.
		*/
		IIFDcvbClearValidBit(frm);
		return (RTsetmode(FALSE, runf, dataptr));

	  default:
		return (FALSE);			/* preprocessor bug */
	}
	return (TRUE);
}

GLOBALREF RTATTLIST   IIattform[];

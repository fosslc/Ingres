
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<abclass.h>
# include       "ervq.h"
# include       <metafrm.h>


/**
** Name:	vqescmen.c - Get menuitem to edit escape code for
**
** Description:
**	This file defines:
**		IIVQegfEditGenForm  - implement user edit of gen'ed form.
**
** History:
**	10/01/89 (tom)	- created
**	6/90     (Mike S) - Allow edit if iiabGenCheck fails
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

FUNC_EXTERN bool IIVQgcGenCheck(); 

FUNC_EXTERN STATUS iiabGenCheck(); 
FUNC_EXTERN VOID iiabFormEdit(); 

/*{
** Name:	IIVQegfEditGenForm	- Edit generated form
**
** Description:
**	This function invokes the edit of forms associated with
**	various types of objects. For the metaframe types.. we
**	will generate the form if it doesn't exist yet.. the
**	other types will just invoke vifred and do the default 
**	thing.
**
** Inputs:
**	OO_OBJECT *ao;	- application object that has the form attached
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	10/04/89 (tom) - created
*/
VOID
IIVQegfEditGenForm(ao)
OO_OBJECT *ao;
{
	FORM_REF *form_ref;

	switch (ao->class)
	{
	case OC_MUFRAME:
	case OC_APPFRAME:
	case OC_UPDFRAME:
	case OC_BRWFRAME:
		_VOID_ IIVQgcGenCheck( (USER_FRAME*) ao);
		form_ref = ((USER_FRAME*)ao)->form;
		break;

	case OC_QBFFRAME:
		form_ref = ((QBF_FRAME*)ao)->form;
		break;

	case OC_RWFRAME:
		form_ref = ((REPORT_FRAME*)ao)->form;
		break;

	case OC_OSLFRAME:
		form_ref = ((USER_FRAME*)ao)->form;
		break;

	default:		/* shouldn't happen */
		return;
	}

	iiabFormEdit( ((APPL_COMP*)ao)->appl, form_ref);
}



/*{
** Name:	IIVQgcGenCheck	- check insures that code & form are present 
**
** Description:
**	Insure that both code and form are present..
**
** Inputs:
**	USER_FRAME *uf;	- user frame pointer
**
** Outputs:
**	
**	Returns:
**		TRUE - if there were no problems
**		FALSE - if there were problems
**
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**
** History:
**	10/14/89  (tom) - created
*/
bool
IIVQgcGenCheck(uf)
USER_FRAME *uf;
{
	i4 ostate;
	char *ogdate;
	char *ofdate;
	METAFRAME *m;
	bool	dummy;
	STATUS genstatus;

	/* remember the state information */
	m = uf->mf;
	ostate = m->state;
	ogdate = m->gendate;
	ofdate = m->formgendate;

	/* ensure that the frame is properly regenerated */
	genstatus = iiabGenCheck(uf, FALSE, &dummy);
	if (genstatus == FAIL)
	{
		return (FALSE);
	}

	/* if the state info changed.. then update the flags on disk */
	if (	ostate != m->state 
	   ||	ogdate != m->gendate
	   ||	ofdate != m->formgendate
	   )
	{
		IIAMufqUserFrameQuick(uf);
	}
	return (TRUE);
}


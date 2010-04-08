/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<st.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<ossym.h>

#include	<ereq.h>
#include	<equel.h>
#include	<eqfw.h>
#include	<fsicnsts.h>

#include	<iltypes.h>
#include	<ilops.h>

/**
** Name:	osfrsopt.c -	OSL Translator Generate FRS Options Module.
**
** Description:
**	Contains the routine used to generate the IL code for setting any
**	FRS options that may have been specified.  Defines:
**
**	frs_optgen()	generate IL code for FRS options.
**
** History:
**	Revision 6.1  88/05/11  wong
**	Initial revision.
*/

/*{
** Name:	frs_optgen()	Generate IL Code for FRS Options.
**
** Description:
**	Generate the IL code for an FRS display option based on values stored
**	in the option value list.  These values are set from EQFW routines
**	called from the grammar recognition actions.
**
** Inputs:
**	None
**
** History:
**	05/88 (jhw) -- Written.
**
**      03/31/93 (DonC) Bug 47323
**	Modified the check for 'default' value to not check for default
**	VIFRED forms screenwidth.  For some strange reason in 1989 the FRS
**	default screenwidth for a CALLFRAME was set to be the screenwidth
**	of the calling frame (value = 1), NOT the VIFRED default screenwidth
**	(value=0). With this being the case, when a 4GL frame called another
**	frame with (SCREENWIDTH = 0), meaning display the form using its 
**	VIFRED defined screenwidth, NO IL code would be generated.  This 
**	lack of IL code to set the screen width caused the called frame to be
**	displayed using the screenwidth of the calling frame.
**
**      07/16/93 (DonC)
**      Re-substitued gl.h, sl.h, and iicommon.h. for dbms.h
*/

VOID
frs_optgen ()
{
	register IIFWGI_GLOBAL_INFO		*gip;
	register IIFWOV_OPTION_VALUE	*ovp;

	if ( (gip = EQFWgiget()) == NULL || gip->gi_err )
		return;

	IGgenStmt( IL_FRSOPT, (IGSID *)NULL, 0 );

	for (ovp = gip->gi_ovp ; ovp->ov_pval != NULL ; ++ovp)
	{
		/* Bug 47323 (DonC) exclude SCREENWIDTH from default check */
		if ( !EQFW_ISDEFAULT(ovp) 
		   || ovp->ov_odptr->od_rtsubopid == FSP_SCRWID )
		{ /* not default */
			DB_DT_ID	type = ovp->ov_odptr->od_type;
			 
			IGgenStmt( IL_TL3ELM, (IGSID *)NULL, 3,
					ovp->ov_odptr->od_rtsubopid, type,
					ovp->ov_varinfo != NULL
						? (ILALLOC)((OSSYM *)ovp->ov_varinfo)->s_ilref
						: IGsetConst(type, ovp->ov_pval)
			);
		}
	} /* end for */
	IGgenStmt( IL_ENDLIST, (IGSID *)NULL, 0 );
}

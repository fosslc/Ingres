/*
**	iifrskact.c
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
# include	<frscnsts.h>
# include	<st.h>
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iifrskact.c
**
** Description:
**
**	Routines called as a result of an EQUEL "## ACTIVATE FRSKEY"
**	statement.
**
**	Public (extern) routines defined:
**		IIfrskact()	-	Pre 6.0 frskey activation
**		IInfrskact()	-	6.0 and later frskey activation
**		IIresfrsk()
**	Private (static) routines defined:
**
** History:
**	Created - 07/29/85 (dkh)
**	12/23/86 (dkh) - Added support for new activations.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	09/08/87 (dkh) - Added special hook for object manager so that
**			 a frskey setting can be undone in the init block.
**	07/12/89 (dkh) - Added support for emerald internal hooks (frskey0).
**	11/09/89 (dkh) - Added new entry point (IIFRInternal()) for
**			 enabling internal use only features (e.g.,
**			 parameterization of frskey activations and
**			 activate on frskey0).
**	07/05/90 (dkh) - Enabled special check for frskey parameterization.
**      05/27/93 (rudyw)
**              Fix bug 51153. Memory allocated for a certain forms key string
**              for nested menus in IInfrskact needs to be allocated as tagged
**              memory so that it gets freed properly in routine IIendnest.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

GLOBALREF	i4	form_mu;

GLOBALREF	i4	IIFRIOIntOpt;

/*{
** Name:	IIfrskact	-	FRS key activation
**
** Description:
**
**	This seems to be a cover routine to call the new 6.0
**	FRSkey activation handler, IInfrskact, to provide compatability
**	without requiring pre 6.0 EQUEL programs to re-preprocess.
**
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	frsknum		FRSkey number
**	exp		Explanation
**	val		Flag - validate or not
**	intrval		Activation code
**
** Outputs:
**
** Returns:
**	i4	TRUE	FRSkey activation added successfully
**		FALSE	Error in adding FRSkey activation
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

void
IIfrskact(i4 frsknum, char *exp, i4 val, i4 intrval)
{
	IInfrskact(frsknum, exp, val, FRS_UF, intrval);
}

/*{
** Name:	IInfrskact	-	Define an FRSkey activation
**
** Description:
**	Define an FRSkey activation
**	
**	This routine is part of RUNTIME's external interface.
**	
** Inputs:
**	
**	frsknum		FRSkey number
**	exp		Explanation (optional)
**	val		Flag - validate or not
**	act		Flag - activate or not
**	intrval		Activation code
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
** Side Effects:
**
** History:
**	
*/

IInfrskact(frsknum, exp, val, act, intrval)
i4	frsknum;
char	*exp;
i4	val;
i4	act;
i4	intrval;
{
	i2	savetag;
	i4	valid;
	i4	activ;
	char	*cp;
	FRSKY	*frskey;
	i4	keynum;

	if (frsknum < 1 || frsknum > MAX_FRS_KEYS)
	{
		/*
		**  If the internal use option is not set
		**  or if the number is not zero, then
		**  we just return.  Numbers less than
		**  zero or greater than the max are
		**  simply ignored.
		*/
		if (!IIFRIOIntOpt || frsknum != 0)
		{
			IIFRIOIntOpt = FALSE;
			return(TRUE);
		}
	}

	/*
	**  This is here so we always unset the internal option.
	*/
	IIFRIOIntOpt = FALSE;

	/*
        ** Check if the current running menu has a previous menu pointer.
        ** If so, we have a nested menu for which we must grab the tag id in
        ** order to allocate tagged memory that can be properly freed. 51153
        */
	savetag = 0;
	if ( IIstkfrm->fdrunmu != NULL &&
	     IIstkfrm->fdrunmu->mu_prev != NULL &&
	     IIstkfrm->fdrunmu->mu_prev->mu_act != NULL )
	{
		savetag = *((i2 *)IIstkfrm->fdrunmu->mu_prev->mu_act);
	}
	if (exp != NULL)
	{
		cp = ( (savetag>0) ? STtalloc(savetag,exp) : STalloc(exp) );
	}
	else
	{
		cp = exp;
	}

	if (val != FRS_NO && val != FRS_YES)
	{
		valid = FRS_UF;
	}
	else
	{
		valid = val;
	}

	if (act != FRS_NO && act != FRS_YES)
	{
		activ = FRS_UF;
	}
	else
	{
		activ = act;
	}

	if (frsknum == 0)
	{
		IIfrscb->frs_globs->intr_frskey0 = intrval;
		if (form_mu)
		{
			IIstkfrm->fdrunfrm->frintrp = intrval;
		}
		return(TRUE);
	}

	/*
	**  If activate is not for a submenu
	*/
	if (form_mu)
	{
		frskey = &(IIstkfrm->fdfrskey[frsknum - 1]);
		/*
		**  Special hack for Object Manager so that
		**  it can clear a frskey setting after it
		**  has been set via the normal path.  Trick
		**  is to clear any settings for passed in frskey
		**  and then to call IIresfrsk() to clear all
		**  frskey settings and to set things again.
		**  This round about way is needed to side
		**  step error checking in FTaddfrs().
		*/
		if (intrval == 0)
		{
			frskey->frs_exp = NULL;
			frskey->frs_int = fdopERR;
			frskey->frs_val = FRS_UF;
			frskey->frs_act = FRS_UF;
			IIresfrsk();
			return(TRUE);
		}
		else
		{
			if (frskey->frs_int > 0)
			{
				keynum = frsknum;
				IIFDerror(RTFRSKUSED, 1, (PTR) &keynum);
			}
			frskey->frs_exp = cp;
			frskey->frs_int = intrval;
			frskey->frs_val = valid;
			frskey->frs_act = activ;
		}
	}
	if (FTaddfrs(frsknum, intrval, cp, valid, activ) != OK)
	{
		keynum = frsknum;
		IIFDerror(RTFRSKUSED, 1, (PTR) &keynum);
	}
	return(TRUE);
}



/*{
** Name:	IIresfrsk	-	Restore FRSkey mappings
**
** Description:
**	
**	Clear old FRSkey definitions in FT, then using the
**	FRSkey definition currently in the frame on top
**	of the stack, call FTaddfrs to inform FT about the current
**	FRSkey settings.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

VOID
IIresfrsk()
{
	FRSKY	*frskey;
	i4	intrval;
	i4	i;

	FTclrfrsk();

	if (IIstkfrm == NULL || IIstkfrm->fdfrskey == NULL)
	{
		return;
	}
	frskey = IIstkfrm->fdfrskey;

	for (i = 0; i < MAX_FRS_KEYS; i++, frskey++)
	{
		if ((intrval = frskey->frs_int) > 0)
		{
			FTaddfrs(i + 1, (i4) intrval, frskey->frs_exp,
				frskey->frs_val, frskey->frs_act);
		}
	}
}


/*{
** Name:	IIFRInternal - Entry point to enable internal use options.
**
** Description:
**	This routine is used to indicate that it is OK to accept
**	internal use only options for the next statement.  The
**	only two at this point are:
**
**	- ability to parameterize frskey activations.  The real
**	  restriction is to allow "frskey int_var" where int_var is minus one.
**	- ability to allow frskey0 activation.
**
**	Currently just set variable IIFRIOIntOpt to TRUE to indicate
**	that the next 3GL statement that is executed can accept interal
**	use options.  It is expected that the 3GL statement that checks
**	the flag will reset the flag before exiting.
**
** Inputs:
**	dummy	This is a dummy argument for the future.  Not used at this time.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/09/89 (dkh) - Initial version.
*/
VOID
IIFRInternal(dummy)
i4	dummy;
{
	IIFRIOIntOpt = TRUE;
}

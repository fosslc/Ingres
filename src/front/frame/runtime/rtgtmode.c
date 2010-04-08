/*
**	rtgetmode.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
**
**	History:
**		mm/dd/yy (RTI) -- created for 5.0.
**		10/20/86 (KY)  -- Changed CH.h to CM.h.
**	05/02/87 (dkh) - Integrated change bit code.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	24-aug-89 (bruceb)
**		Invalidate aggregate derivations when changing
**		mode to/from query mode.
**	09-nov-89 (bruceb)
**		Clear simple field derivations when changing to query mode,
**		and display simple field constant-value derivations when
**		changing from query mode to any other mode.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
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
# include	<er.h> 
# include	<rtvars.h>

/**
** Name:	rtgetmode.c
**
** Description:
**
**	Public (extern) routines defined:
**		RTgetmode()
**		RTsetmode()
**	Private (static) routines defined:
**
** History:
**/

FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();
FUNC_EXTERN	VOID	IIFDpadProcessAggDeps();
FUNC_EXTERN	STATUS	IIFVedEvalDerive();
FUNC_EXTERN	VOID	IIFDidfInvalidDeriveFld();
FUNC_EXTERN	VOID	IIFDudfUpdateDeriveFld();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	i4	FDersfld();
FUNC_EXTERN	VOID	IIFDfccb();

static VOID	IIFRiaInvalidAggs(FRAME *);


/*
** the ordering of the contents of mode_strings is determined
** by the values of the 'fdrt' modes as specified in runtime.h
*/

static char	*mode_strings[] =
	{
		ERx("NONE"),		/* fdrtNOP */
		ERx("UPDATE"),		/* fdrtUPD */
		ERx("FILL"),		/* fdrtINS */
		ERx("READ"),		/* fdrtRD */
		ERx("FORMDATA"),	/* fdrtNAMES */
		ERx("QUERY")		/* fdrtQRY */
	};

/*{
** Name:	RTgetmode	-	Return string value of mode
**
** Description:
**	Given a mode (fdrt mode from runtime.h), lookup the
**	string version of the mode, and copy that string into the
**	buffer provided by the caller.
**
** Inputs:
**	runf		Frame pointer 
**	md		Ptr to buffer to update with mode string
**
** Outputs:
**	md		Will be updated with the mode as a null terminated
**			string.
**
** Returns:
**	i4	TRUE
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

RTgetmode(runf, md)
reg	RUNFRM	*runf;
reg	char	*md;
{
	i4	mode = runf->fdrunmd;

	if ((mode < fdrtNOP) || (mode > fdrtQRY))
		mode = fdrtNOP;
	STcopy(mode_strings[mode], md);

	return (TRUE);
}


/*{
** Name:	RTsetmode	-	Set the mode of a frame	
**
** Description:
**	Set the mode of a frame, and if the init flag is on,
**	initialize the frame too by calling FDinitsp.
**	If the frame having it's mode set is the currently displayed
**	one, call FDrstfrm to set up the mode correctly so
**	subsequent FDputfrm calls will work.
**
** Inputs:
**	init		Flag - if on, initialize after mode is set
**	runf		Ptr to the frame
**	md		Character string for the mode to set
**
** Outputs:
**
** Returns:
**	i4	TRUE	Mode was set successfully
**		FALSE	Error occurred.  Mode could not be set
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	
*/

i4
RTsetmode(i4 init, RUNFRM *runf, char *md)
{
	reg	FRAME	*frm;
	reg	i4	mode;
	u_char		tmp_char[3];	/* temp buf to lowercase a character. */
	i4		omode = fdrtNOP;
	i4		fldno;
	FIELD		*fld;
	FLDHDR		*hdr;
	FLDTYPE		*type;
	FLDVAL		*val;

	if (runf == NULL)
		return (FALSE);

	frm = runf->fdrunfrm;

	/*
	**	Initialize and display frame according to mode.
	*/

	CMtolower(md, tmp_char);
	switch (*tmp_char)
	{
	  case	'u':	/* UPDATE MODE */
		runf->fdrunmd = fdrtUPD;
		frm->frmode = fdcmINSRT;
		mode = fdmdUPD;
		if (init)
		{
			FDinitsp(frm, (i4) 0, mode);
		}
		break;

	  case	'e':	/* FILL MODE */
	  case	'f':
		runf->fdrunmd = fdrtINS;
		frm->frmode = fdcmINSRT;
		mode = fdmdADD;
		if (init)
		{
			FDinitsp(frm, (i4) 0, mode);
		}
		break;

	  case	'q':	/* QUERY MODE */
		runf->fdrunmd = fdrtQRY;
		frm->frmode = fdcmINSRT;
		mode = fdmdFILL;
		if (init)
		{
			FDinitsp(frm, (i4) 0, mode);	/* fdmdFILL = fdmdQRY */
		}
		break;

	  case	'r':	/* READ MODE */
		runf->fdrunmd = fdrtRD;
		frm->frmode = fdcmBRWS;
		mode = fdmdUPD;
		if (init)
		{
			FDinitsp(frm, (i4) 0, mode);
		}
		break;

	  case	'n':	/* FORMDATA MODE */
		runf->fdrunmd = fdrtNAMES;
		frm->frres2->formmode = fdrtNAMES;
		return(TRUE);

	  default:	/* BAD MODE */
		IIFDerror(RTDSMD, 2, md, runf->fdfrmnm);
		return (FALSE);
	}

	omode = frm->frres2->formmode;

	/*
	**  If changing a form mode rather than setting mode on initial
	**  display, and if changing to/from query mode, then invalidate
	**  aggregate values.  Also, if going to query mode, clear derived
	**  simple fields; if leaving query mode, display constant derived
	**  simple fields.
	*/
	if (!init
	    && ((omode == fdrtQRY) || (runf->fdrunmd == fdrtQRY))
	    && (omode != runf->fdrunmd))
	{
	    IIFRiaInvalidAggs(frm);
	    if (runf->fdrunmd == fdrtQRY)
	    {
		for (fldno = 0; fldno < frm->frfldno; fldno++)
		{
		    fld = frm->frfld[fldno];
		    if (fld->fltag != FTABLE)
		    {
			hdr = FDgethdr(fld);
			/* Clear out derived fields on going to query mode. */
			if (hdr->fhd2flags & fdDERIVED)
			{
			    _VOID_ FDersfld(frm, fldno, FT_UPDATE, FT_REGFLD,
				(i4) 0, (i4) 0);
			}
		    }
		}

		/*
		** Call IIFDpad() here, because won't get called in
		** IIrunform() since by then the form is in query mode.
		** This is called for the side effect of blanking out
		** any aggregate dependent simple fields on the form.
		*/
		IIFDpadProcessAggDeps(frm);
	    }
	    else
	    {
		for (fldno = 0; fldno < frm->frfldno; fldno++)
		{
		    fld = frm->frfld[fldno];
		    if (fld->fltag != FTABLE)
		    {
			hdr = FDgethdr(fld);
			/* Display constant-value derivations. */
			if ((hdr->fhd2flags & fdDERIVED)
			    && (hdr->fhdrv->drvflags & fhDRVCNST))
			{
			    IIFDfccb(frm, hdr->fhdname);
			    type = FDgettype(fld);
			    val = FDgetval(fld);
			    if ((IIFVedEvalDerive(frm, hdr, type, val->fvdbv))
				== FAIL)
			    {
				IIFDidfInvalidDeriveFld(frm, fld, BADCOLNO);
			    }
			    else
			    {
				IIFDudfUpdateDeriveFld(frm, fld, BADCOLNO);
			    }
			}
		    }
		}
	    }
	}

	frm->frres2->formmode = runf->fdrunmd;

	/*
	**  If we are changing the mode of the form that is currently
	**  displayed, then call FDrstfrm to set up mode correctly
	**  so everything will work when we call FDputfrm later. (dkh)
	*/
	if (IIstkfrm == runf)
	{
		FDrstfrm(frm, mode);
	}
	return (TRUE);
}


/*{
** Name:	IIFRiaInvalidAggs	- Invalidate aggs for all tblflds.
**
** Description:
**	Invalidate the aggregates for all table fields on the form.
**	Used when the form mode is either changing to query mode or
**	changing from query mode.
**
** Inputs:
**	frm		Ptr to the frame
**
** Outputs:
**
** Returns:
**	VOID
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	24-aug-89 (bruceb)	Written.
*/
VOID
IIFRiaInvalidAggs(FRAME *frm)
{
    i4		i;
    FIELD	*fld;

    for (i = 0; i < frm->frfldno; i++)
    {
	fld = frm->frfld[i];
	if (fld->fltag == FTABLE)
	{
	    IIFDiaaInvalidAllAggs(frm, fld->fld_var.fltblfld);
	}
    }
}

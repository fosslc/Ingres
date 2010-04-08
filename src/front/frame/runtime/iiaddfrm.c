/*
**	iiaddfrm.c
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
# include	<er.h>
# include	<cv.h>
# include	<rtvars.h>
# include	<lqgdata.h>

GLOBALREF	FRAME	*FDFTiofrm;
/**
** Name:	iiaddfrm.c
**
** Description:
**	This routine adds the frame to the frame active list and any
**	runtime information necessary.
**
**	Public (extern) routines defined:
**	Private (static) routines defined:
**
** History:
**	16-feb-1983  -	Extracted from original runtime.c (jen)
**	24-feb-1983  -	Added table field implementation  (ncg)
**	12-20-84 (rlk) - Added FEbegintag and FEendtag calls.
**	11-feb-1985  -	removed code common to IIfrminit into
**			routine called IIaddrun().  (bab)
**	06/16/87 (dkh) - Integrated FElocktag()/FEtaglocked() usage
**			 and IIdelfrm() from PC for bobm.
**	12-aug-87 (bab)
**		Removed _VOID_ from in from of CVlower call.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	10/28/88 (dkh) - Performance changes.
**	16-mar-89 (bruceb)
**		Added call on IIFRaff() after a successful IIaddrun().
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	3/91 (Mike S) Use MEfree only where appropriate in IIdelfrm
**	08/11/91 (dkh) - Added changes so that all memory allocated
**			 for a dynamically created form is freed.
**	08/15/91 (dkh) - Added changes to make above changes work
**			 correctly on the PC as well as using IIUGtagFree()
**			 to allow tags to be reused.
**      04/23/96 (chech02)
**          added i4  to the IIaddform() function type for windows 3.1 port.
**  23-jul-1998 (rodjo04) Cross-integrated change 271728 from 6.4/06
**          07-jan-94 (rudyw) Add comments to cross-reference with fix for 47408 for future.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Aug-2007 (hanal04) Bug 118248
**          In IIdelfrm() clear down FDFTiofrm if it referenced the memory
**          we just freed.
**	2-Aug-2007 (kibro01) b118719
**	    Frame lookup routine leaks tags if the delfrm doesn't drop the
**	    table field tag before freeing the form itself.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

FUNC_EXTERN	VOID	IIFRaffAddFrmFrres2();

/*{
** Name:	IIaddform	-	Add frame to list of active frames
**
** Description:
**	Called from a '## ADDFORM' statement, adds a compiled form to the
**	list of active forms.
**
**	Starts a tag region so that form structures that are not specifically
**	allocated with a tag will fall into a common tag region for the form.
**	Calls IIaddrun to do the work.
**
**	This routine is part of the external interface to RUNTIME.  
**	
** Inputs:
**	frame		Ptr to the compiled form to add
**
** Outputs:
**
** Returns:
**	i4	TRUE
**		FALSE	If form could not be added
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	?????
*/

i4
IIaddform(frame)
FRAME	*frame;
{
	FRAME	*frm;
	char	*name;
	char	namestr[FE_MAXNAME + 1];
	i2	frmtag = -1;			/* frame's memory tag-id */
	i4	savelang;
	i4	IIlang();
	i4	ret_stat;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized.
	*/
	if (frame == NULL)
	{
		return (FALSE);
	}

	frm = frame;

	IIsetferr((ER_MSGID) 0);

	if(!IILQgdata()->form_on)
	{
		IIFDerror(RTNOFRM, 0, NULL);
		return (FALSE);
	}

	/*
	**	Do memory allocation for a pre-defined frame
	**	Insure the new frame is in a unique memory tag region.
	*/

	if (!FEtaglocked())
	{
		frmtag = FEbegintag();
	}
	savelang = IIlang(hostC);

	if(!FDcrfrm(&frm))
	{
		if (frmtag > 0)
		{
			FEendtag();
		}
		IIlang(savelang);
		return (FALSE);
	}

	IIlang(savelang);

	STcopy(frm->frname, namestr);
	CVlower(namestr);
	name = namestr;
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


VOID
IIdelfrm(name)
char	*name;
{
	RUNFRM	*runf;
	FRAME   *frm;
	RUNFRM	*stkf;
	TBSTRUCT *tb;
	TBSTRUCT *ttb;
	char	buf[FE_MAXNAME + 1];
	TAGID	tag1;
	TAGID	tag2;

	STcopy(name, buf);
	CVlower(buf);
	if ((runf = RTfindfrm(buf)) == NULL)
	{
		return;
	}

	for (stkf = IIstkfrm; stkf != NULL; stkf = stkf->fdrunnxt)
	{
		if (STcompare(buf, stkf->fdfrmnm) == 0)
		{
			return;
		}
	}

	if (runf == IIrootfrm)
	{
		IIrootfrm = IIrootfrm->fdlisnxt;
	}
	for (stkf = IIrootfrm; stkf != NULL; stkf = stkf->fdlisnxt)
	{
		if (stkf->fdlisnxt == runf)
		{
			stkf->fdlisnxt = runf->fdlisnxt;
			break;
		}
	}

	/* Before we free the main frame, free the tag from the 
	** table field list - otherwise its reference is lost.
	** (kibro01) b118719
	*/

	if (runf->fdruntb)
	{
		tb = runf->fdruntb;
		while (tb)
		{
			if (tb->dataset && tb->dataset->ds_mem)
			{
				u_i4 tag = tb->dataset->ds_mem->dm_memtag;
				if (tag)
					IIUGtagFree(tag);
			}
			tb = tb->tb_nxttb;
		}
	}

	/*
	**  Tag1 is for memory allocated by the dynamic form creation
	**  routines while tag2 is for memory allocated by the
	**  runtime/fd routines.  Tag1 and tag2 are never the
	**  same.
	*/
	frm = runf->fdrunfrm;
	tag1 = runf->fdrunfrm->frrngtag;
	tag2 = runf->fdrunfrm->frtag;
	if (runf->fdrunfrm->frmflags & fdISDYN)
	{
		IIUGtagFree(tag1);
	}
	IIUGtagFree(tag2);

	/*
	**  Remove possible references to just deleted memory.
	*/
	if (IIfrmio == runf)
	{
       /*
       ** Setting IIfrmio to NULL here had a side effect resulting
       ** in bug 47408 where IIfrmio gets used in IIdsinsert without
       ** having been set. A fix has been introduced within IIdsinsert
       ** but a full analysis may reveal a need for a change here.
       */

		IIfrmio = NULL;
	}

	if (FDFTiofrm == frm)
	{
	   FDFTiofrm = (FRAME *)NULL;
	}
}

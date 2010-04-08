/*
** sequence.c
**
** Copyright (c) 2004 Ingres Corporation
**
** Contains routines which handle resquencing of fields.
**
** History:
**	08/14/87 (dkh) - ER changes.
**	10/02/87 (dkh) - Help file changes.
**	10/25/87 (dkh) - Error message cleanup.
**	02/02/88 (dkh) - Fixed jup bug 1874.
**	21-jul-88 (bruceb)	Fix for bug 2969.
**		Changes to vfEdtStr to specify maximum length.
**	11/09/88 (dkh) - Changed encoding of box/trim sizes to
**			 make each row in ii_trim unique.
**	01/31/89 (dkh) - Added code to create checksums before saving
**			 form in db.
**	04/18/89 (dkh) - Integrated IBM changes.
**	24-apr-89 (bruceb)
**		Removed mention of nonseq fields; no longer exist.
**	31-apr-89 (bruceb)
**		Free seqfrm at end of vfsequence.
**	21-jun-89 (bruceb)
**		Changed so that derived fields aren't considered as
**		editable on the Order form.  Also, no longer calling
**		vfposarr(), nor passing the resulting args down to
**		VTcursor()--those parameters to VTcursor aren't used.
**	07-sep-89 (bruceb)
**		Call VTputstring with trim attributes now, instead of 0.
**	09/23/89 (dkh) - Porting changes integration.
**	12/05/89 (dkh) - Changed interface to VTcursor() so VTlib could
**			 be moved into ii_framelib shared lib.
**	01/12/90 (dkh) - Only change noChanges if user changed the ordering.
**	07-mar-90 (bruceb)
**		Lint:  Add 5th param to FTaddfrs() calls.
**	30-mar-90 (bruceb)
**		Force Vifred menu display back to first menuitem.
**      04/10/90 (esd) - added extra parm FALSE to calls to VTdispbox
**                       to tell VT3270 *not* to clear the positions
**                       under the box before drawing the box
**	09-may-90 (bruceb)
**		Call VTcursor() with new arg indicating whether there is
**		'not' a menu associated with this call.
**      06/12/90 (esd) - Tighten up the meaning of the global variable
**                       endxFrm.  In the past, it was usually (but
**                       not always) assumed that in a 3270 environment,
**                       the form included a blank column on the right,
**                       so that endxFrm was the 0-relative column
**                       number of that blank column at the right edge.
**                       I am removing that assumption:  endxFrm will
**                       now typically be the 0-relative column number
**                       of the rightmost column containing a feature.
**                       Also, I'm assuming there was a bug in the code
**                       in that when VTcursor was called, the cursor
**                       was allowed in the end marker at the bottom
**                       but not in the end marker at the right.
**                       (I changed it so that it's allowed in both).
**      06/12/90 (esd) - Add extra parm to VTputstring to indicate
**                       whether form reserves space for attribute bytes
**	06/28/92 (dkh) - Added support for rulers in vifred/rbf.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      10/26/99 (hweho01)
**          Added allocArr() function prototype. Without the 
**          explicit declaration, the default int return value   
**          for a function will truncate a 64-bit address
**          on ris_u64 platform.
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-May-2003 (bonro01)
**	    Add i64_hpu to NO_OPTIM list because of SEGV in vifred tests
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/
/*
NO_OPTIM = i64_hpu
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<ug.h>
# include	"seq.h"
# include	<si.h>
# include	<vt.h>
# include	<frsctblk.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>
# include	"ervf.h"
# include	"vfhlp.h"



# ifndef FORRBF
static	POS	*curps = NULL;
static	char	*vfseqstr = NULL;
static	FRAME	*seqfrm = NULL;
# endif

static	i4	*osequence = NULL;
static	POS	**opslist = NULL;
static	FLDHDR	**ohdr = NULL;
static	i4	*available = NULL;
static	i4	unique = 0;
/* Number of fields used; does not include derived fields for Order form. */
static	i4	numfds = 0;

GLOBALREF	FILE	*vfdfile;
GLOBALREF	FIELD	**fldlist;
GLOBALREF	TRIM	**trmlist;
GLOBALREF	i4	tnumfield;
GLOBALREF	i4	tnumfld;

FUNC_EXTERN	STATUS	IImumatch();
FUNC_EXTERN	i4	IIFDcncCreateNameChecksum();
FUNC_EXTERN	i4	VFfeature();
FUNC_EXTERN     PTR     allocArr();

/* REMEMBER to set flag to indicate that user has set sequence numbers.
** Also note that Vfformname has to be handled differently now.
*/


/*
** vfseqdisp:
**	Routine to display the sequence number of a field.
**	Only displays as many digits as there is room in
**	the data portion of the field.
*/

# ifndef FORRBF
vfseqdisp(str, y, x, start, end)
char	*str;
i4	y;
i4	x;
i4	start;
i4	end;
{
	reg	i4	len;
	reg	i4	room;
	reg	i4	copylength;
		char	buf[256];

	len = STlength(str);
	room = end - start + 1;
	if (room < len)
	{
		copylength = room;
	}
	else
	{
		copylength = len;
	}
	STlcopy(str, buf, copylength);

	VTputstring(seqfrm, y, x, buf, 0, FALSE, IIVFsfaSpaceForAttr);
}


/*
** Routine to change sequence numbers into a default
** ordering of left to right, top to bottom.
*/

i4
vfseqdflt()
{
	reg	POS	**pslist;
	reg	POS	*ps;
	reg	FLDHDR	*hdr;
	reg	FLDHDR	**hdrlist;
	i4	i;
	i4	j;
	i4	y;
	i4	x;
	i4	start;
	i4	end;
	SMLFLD	sfd;
	char	dfltstr[40];
	char	*blanks = ERx("                                       ");

	pslist = opslist;
	hdrlist = ohdr;
	for (i = 0, j = 0; i < numfds; i++, j++, pslist++, hdrlist++)
	{
		ps = *pslist;
		hdr = *hdrlist;
		if (hdr->fhseq != j)
		{
			if (ps->ps_name == PFIELD)
			{
				FDToSmlfd(&sfd, ps);
				y = sfd.dy;
				x = sfd.dx;
				start = sfd.dx;
				end = sfd.dex;
			}
			else
			{
				y = hdr->fhposy + 1;
				x = hdr->fhposx;
				start = hdr->fhposx;
				end = ps->ps_endx;
			}
			hdr->fhseq = j;
			CVna(j + 1, dfltstr);
			vfseqdisp(blanks, y, x, start, end);
			vfseqdisp(dfltstr, y, x, start, end);
		}
	}

	noChanges = FALSE;
}


/*
** Routine to do the actual changing.
** Message to say type MENU KEY when finished.
*/

i4
vfseqchg()
{
		FIELD	*fd;
		FLDHDR	*hdr;
		SMLFLD	sfd;
		i4	fldnum;
		char	buf[40];
		char	strbuf[150];
		i4	stry;
		i4	strx;
		i4	newseq;
		char	*str;
		u_char	*newstr;
	reg	char	*numstr;
		i4	numfld;
		i4	i;
		i4	newlength;
		i4	start;
		i4	end;
		char	*blanks = ERx("                                   ");

	/* need to somehow find out where I am on the form */

	if ((curps = onPos(globy, globx)) == NULL)
	{
		IIUGerr(E_VF00C5_Cursor_not_on_a_compo, UG_ERR_ERROR, 0);
		return;
	}
	switch (curps->ps_name)
	{
	case PFIELD:
		fd = (FIELD *) curps->ps_feat;
		hdr = FDgethdr(fd);
		if (hdr->fhd2flags & fdDERIVED)
		{
		    IIUGerr(E_VF0139_Change_option_not_val,
			    UG_ERR_ERROR, 0);
		    return;
		}
		fldnum = hdr->fhseq + 1;
		CVna(fldnum, buf);
		FDToSmlfd(&sfd, curps);
		str = &buf[0];
		stry = sfd.dy;
		strx = sfd.dx;
		start = sfd.dx;
		end = sfd.dex;
		if (hdr->fhdflags & fdBOXFLD)
		{
			VTxydraw(seqfrm, stry, strx);
		}
		break;

	case PTABLE:
		fd = (FIELD *) curps->ps_feat;
		hdr = &(fd->fld_var.fltblfld->tfhdr);
		fldnum = hdr->fhseq + 1;
		CVna(fldnum, buf);
		str = &buf[0];
		stry = hdr->fhposy + 1;
		strx = hdr->fhposx;
		start = hdr->fhposx;
		end = curps->ps_endx;
		VTxydraw(seqfrm, stry, strx);
		break;

	case PBOX:
	case PTRIM:
		IIUGerr(E_VF00C8_Change_option_not_val,
			UG_ERR_ERROR, 0);
		return;
	}

	noChanges = FALSE;

	scrmsg(vfseqstr);

	numfld = (i4)numfds;

	for (;;)
	{
		do
		{
			/*
			** Arbitrarily, allow max of 10 spaces for user
			** to enter the sequence number for this field.
			*/
			newstr = vfEdtStr(seqfrm, str, stry, strx, 10);
		} 
		while (*newstr == '\0');

		STtrmwhite((char *) newstr);
		newlength = STlength((char *) newstr);
		STcopy(newstr, strbuf);
		if (strbuf[0] == '\0')
		{
			IIUGerr(E_VF00C9_No_sequence_number_en,
				UG_ERR_ERROR, 1, &numfld);
			continue;
		}
		numstr = &strbuf[0];
		if (CVan(numstr, &newseq) != 0)
		{
			IIUGerr(E_VF00CA_not_a_valid_num,
				UG_ERR_ERROR, 2, numstr, &numfld);
			goto err_fixup;
		}
		if (newseq <= 0)
		{
			IIUGerr(E_VF00CB_A_sequence_number_mus, 
				UG_ERR_ERROR, 1, &numfld);
			goto err_fixup;
		}
		if ((newseq - 1) >= numfld)
		{
			i4	seqnum;

			seqnum = newseq;
			IIUGerr(E_VF00CC_is_too_large,
				UG_ERR_ERROR, 2, &seqnum, &numfld);
		    err_fixup:
			scrmsg(vfseqstr);
			VTdelstring(seqfrm, stry, strx, newlength);
			VTxydraw(seqfrm, stry, strx);
			continue;
		}
		i = start;
		vfseqdisp(blanks, stry, strx, i, end);
		hdr->fhseq = newseq - 1;
		vfseqdisp(numstr, stry, strx, start, end);
		vfseqnext(VF_NEXT, &globy, &globx);
		VTxydraw(seqfrm, globy, globx);
		break;
	}
}

# endif

/*
** vfseqerrmsg
**	Routine to print out error message about not being
**	able to allocate any more space for sequencing.
*/

vfseqerrmsg()
{
	IIUGbmaBadMemoryAllocation(ERx("vfseqerrmsg"));
}


/*
** vfseqbuild:
**	Routine to create a window that will allow the user to change
**	a sequence number.
**
**	11/2/83 (nml) -Put in check to see if there are any fields on
**		       the form to avoid calling MEcalloc with 0 as
**		       first arg (which then causes return of !OK)
*/

VOID
vfseqbuild(seqchg)
i4	seqchg;
{
	reg	i4	i;
	reg	VFNODE	**lp;
	reg	VFNODE	*lt;
		FLDHDR	*hdr;
		FIELD	*fd;
# ifndef FORRBF
		i4	start;
		i4	end;
		TRIM	*tr;
		i4	*bseq;
		SMLFLD	sfd;
		char	buf[40];
		i4	numcols;
		i4	j;
		FLDCOL	**colarr;
		FLDHDR	*chdr;
# endif
		POS	*ps;
		POS	**pslist;
		FLDHDR	**hdrptr;
		i4	boxtrimcount;

	tnumfield = 0;
	tnumfld = 0;
	boxtrimcount = 0;
	frame->frfld = fldlist = (FIELD **)allocArr(frame->frfldno);
	frame->frnsfld = NULL;
	/*
	** Note on trim/box features:
	**	This function is called for several reasons amoung
	**	which is the need to write out a form... since we
	**	are (for present) writing out the box features as
	**	trim.. it is necessary to add the boxes to 
	**	the trim count.. room for the trim table is 
	**	allocated here.. the frtrimno struct member is only 
	**	modified here if it is called for the purposes of writting
	**	the form, in this case the caller has saved the real 
	**	frtrimno and will restore after the write takes place.
	*/
	if (!seqchg)
	{
		frame->frtrimno += vfBoxCount(); 
	}
	frame->frtrim = trmlist = (TRIM **)allocArr(frame->frtrimno);

	if (frame->frfldno > 0)
	{
		/* fields are on this form, so continue	 */

		if ((osequence = (i4 *)FEreqmem((u_i4)0,
		    (u_i4)(frame->frfldno * sizeof(i4)),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			vfseqerrmsg();
		}
		if ((pslist = (POS **)FEreqmem((u_i4)0,
		    (u_i4)(frame->frfldno * sizeof(POS *)),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			vfseqerrmsg();
		}
		opslist = pslist;
		if ((available = (i4 *)FEreqmem((u_i4)0,
		    (u_i4)(frame->frfldno * sizeof(i4)),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			vfseqerrmsg();
		}
		if ((ohdr = (FLDHDR **)FEreqmem((u_i4)0,
		    (u_i4)(frame->frfldno * sizeof(FLDHDR *)),
		    TRUE, (STATUS *)NULL)) == NULL)
		{
			vfseqerrmsg();
		}
	}

	hdrptr = ohdr;
	tnumfield = frame->frfldno;

# ifndef FORRBF
	if (seqchg)
		vfDispBoxes(seqfrm, endFrame - 1); /* display boxes first */
# endif /* FORRBF */

	if (seqchg)	/* Order form. */
	{
	    numfds = 0;
	    lp = line;
	    for (i = 0; i < endFrame; i++, lp++)
	    {
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, i))
		{
		    if (lt->nd_pos->ps_begy != i)
			    continue;

		    ps = lt->nd_pos;
		    switch(ps->ps_name)
		    {
		    case PFIELD:
			fd = (FIELD *) ps->ps_feat;
			hdr = FDgethdr(fd);
			if (hdr->fhd2flags & fdDERIVED)
			{
			    hdr->fhseq = NEWFLDSEQ;
			}
			else
			{
			    numfds++;
			}
			break;
		    
		    case PTABLE:
			numfds++;
			break;
		    }
		}
	    }
	}
	else	/* Saving form. */
	{
	    numfds = frame->frfldno;
	}

	lp = line;
	for (i = 0; i < endFrame; i++, lp++)
	{
		for (lt = *lp; lt != NNIL; lt = vflnNext(lt, i))
		{
			if (lt->nd_pos->ps_begy != i)
				continue;

			ps = lt->nd_pos;
			switch(ps->ps_name)
			{
			case PFIELD:
				fd = (FIELD *) ps->ps_feat;
				hdr = FDgethdr(fd);
				if (seqchg && (hdr->fhd2flags & fdDERIVED))
				{
# ifndef	FORRBF
				    /*
				    ** This field won't be on the list to
				    ** process later, so add to the display
				    ** now.
				    */
				    FDToSmlfd(&sfd, ps);
				    vfClearFld(seqfrm, hdr);
				    VTputstring(seqfrm, sfd.ty, sfd.tx,
					hdr->fhtitle, 0, FALSE,
					IIVFsfaSpaceForAttr);
				    if (hdr->fhdflags & fdBOXFLD)
				    {
					VTdispbox(seqfrm,
					    hdr->fhposy, hdr->fhposx,
					    hdr->fhmaxy, hdr->fhmaxx,
					    (i4)0, FALSE, TRUE,
					    FALSE);
				    }
# endif	/* FORRBF */
				    break;
				}
				if (!seqchg)
				{
				    hdr->fhdfont =
					IIFDcncCreateNameChecksum(hdr->fhdname);
				}
				*fldlist++ = fd;
				*pslist++ = ps;
				*hdrptr++ = hdr;
				if (hdr->fhseq == NEWFLDSEQ)
				{
					;
				}
				else if (hdr->fhseq >= numfds)
				{
					hdr->fhseq = NEWFLDSEQ;
				}
				else
				{
					if (++(available[hdr->fhseq]) > 1)
					{
						hdr->fhseq = NEWFLDSEQ;
					}
				}
				tnumfld++;
				break;

# ifndef FORRBF
			case PTABLE:
				fd = (FIELD *) ps->ps_feat;
				hdr = &(fd->fld_var.fltblfld->tfhdr);
				if (!seqchg)
				{
				    hdr->fhdfont =
					IIFDcncCreateNameChecksum(hdr->fhdname);
				    numcols = fd->fld_var.fltblfld->tfcols;
				    colarr = fd->fld_var.fltblfld->tfflds;
				    for (j = 0; j < numcols; j++)
				    {
					chdr = &(colarr[j]->flhdr);
					chdr->fhdfont =
					    IIFDcncCreateNameChecksum(chdr->fhdname);
				    }
				}
				*pslist++ = ps;
				*fldlist++ = fd;
				*hdrptr++ = hdr;
				if (hdr->fhseq == NEWFLDSEQ)
				{
					;
				}
				else if (hdr->fhseq >= numfds)
				{
					hdr->fhseq = NEWFLDSEQ;
				}
				else
				{
					if (++(available[hdr->fhseq]) > 1)
					{
						hdr->fhseq = NEWFLDSEQ;
					}
				}
				tnumfld++;
				break;

			case PBOX: 
				/* if  called due to form write, encode box */
				/* only encode if not a ruler */
				if (!seqchg && ps->ps_attr != IS_STRAIGHT_EDGE)
				{
					/*
					**  Encode a unique identifier to
					**  end of box/trim size string
					**  to make each row in ii_trim
					**  unique even if several box/trim
					**  compoenents start at the same
					**  location and are of the same size.
					*/
					_VOID_ STprintf(buf, ERx("%d:%d:%d"), 
						ps->ps_endy - ps->ps_begy + 1, 
						ps->ps_endx - ps->ps_begx + 1,
						boxtrimcount++);
					tr = trAlloc(ps->ps_begy, ps->ps_begx, 
							saveStr(buf));
					/* Note: The flags of the trim struct
					   hold the attribute flags for the box.
					   Trim with fdBOXFLD set are really 
					   boxes and not really trim. */
					tr->trmflags = *(i4*)ps->ps_feat 
							| fdBOXFLD;
					*trmlist++ = tr;
				}
				break;
# endif

			case PTRIM:
				*trmlist++ = (TRIM *) ps->ps_feat;
# ifndef FORRBF
				if (seqchg)
				{
					tr = (TRIM *) ps->ps_feat;
					VTputstring(seqfrm, ps->ps_begy, 
						ps->ps_begx, tr->trmstr, 
						tr->trmflags, FALSE,
						IIVFsfaSpaceForAttr);
				}
# endif
				break;
			}
		}
	}
	pslist = opslist;
# ifndef FORRBF
	bseq = osequence;
# endif
	hdrptr = ohdr;
	unique = 0;
	for (i = 0; i < numfds; i++, pslist++, hdrptr++)
	{
		ps = *pslist;
		if (ps->ps_name == PFIELD)
		{
			hdr = *hdrptr;
			vfunique(hdr);
# ifndef FORRBF
			if (seqchg)
			{
				*bseq++ = hdr->fhseq;
				FDToSmlfd(&sfd, ps);
				vfClearFld(seqfrm, hdr);
				VTputstring(seqfrm, sfd.ty, sfd.tx,
					hdr->fhtitle, 0, FALSE,
					IIVFsfaSpaceForAttr);
				CVna(hdr->fhseq + 1, buf);
				start = sfd.dx;
				end = sfd.dex;
				vfseqdisp(buf, sfd.dy, sfd.dx, start, end);
				if (hdr->fhdflags & fdBOXFLD)
				{
					VTdispbox(seqfrm, 
						hdr->fhposy, hdr->fhposx, 
						hdr->fhmaxy, hdr->fhmaxx,
						(i4) 0, FALSE, TRUE,
						FALSE);
				}
			}
# endif
		}
# ifndef FORRBF
		else
		{
			hdr = *hdrptr;
			vfunique(hdr);
			if (seqchg)
			{
				start = ps->ps_begx;
				end = ps->ps_endx;
				vfseqdisp(hdr->fhdname, ps->ps_begy,
					ps->ps_begx, start, end);
				*bseq++ = hdr->fhseq;
				CVna(hdr->fhseq + 1, buf);
				start = hdr->fhposx;
				end = ps->ps_endx;
				vfseqdisp(buf, hdr->fhposy + 1,
					hdr->fhposx, start, end);
			}

		}
# endif
	}
}

VOID
vfunique(hdr)
FLDHDR	*hdr;
{
	if (hdr->fhseq == NEWFLDSEQ)
	{
		while(available[unique] >= 1)
		{
			unique++;
		}
		hdr->fhseq = unique;
		available[unique]++;
	}
}

/*
** vfseqck:
**	Routine to check for unique sequence numbers.
**
*/

# ifndef FORRBF
vfseqck()
{
	reg	FLDHDR	*hdr;
	reg	POS	**pslist;
	reg	FLDHDR	**hdrlist;
	reg	i4	i;
		POS	*ps;
		i4	*seqarray;

	if ((seqarray = (i4 *)MEreqmem((u_i4)0, (u_i4)(numfds * sizeof(i4)),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		vfseqerrmsg();
	}
	pslist = opslist;
	hdrlist = ohdr;
	for (i = 0; i < numfds; i++)
	{
		ps = *pslist++;
		hdr = *hdrlist++;
		switch(ps->ps_name)
		{
		case PFIELD:
			if (++(seqarray[hdr->fhseq]) > 1)
			{
				curps = ps;
				MEfree(seqarray);
				return (0);
			}
			break;
		case PTABLE:
			if (++(seqarray[hdr->fhseq]) > 1)
			{
				curps = ps;
				MEfree(seqarray);
				return (0);
			}
			break;
		default:
			break;
		}
	}
	MEfree(seqarray);
	return (1);
}


/*
** vfseqmu_put:
**	Routine to put up the resequencing menu.
**	Cannot use vfmu_put since that refreshes frame->frscr.
*/

vfseqmu_put(mu)
MENU	mu;
{
	FTputmenu(mu);
	VTxydraw(seqfrm, globy, globx);
	vfTestDump();
}



/*
** Entry point for changing sequence of fields.
*/

i4
vfsequence()
{
	MENU		mu;
	i4		menu_val;
	FRS_EVCB 	evcb;
	FRAME		*dummy;

	if (vfseqstr == NULL)
	{
		vfseqstr = ERget(F_VF002B_Change_sequence_num);
	}

	if ((seqfrm = (FRAME *)MEreqmem((u_i4)0, (u_i4)sizeof(FRAME), TRUE,
	    (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vfsequence"));
	}
	seqfrm->frmaxx = frame->frmaxx;
	seqfrm->frmaxy = frame->frmaxy;
	if (!VTnewfrm(seqfrm))
	{
		IIUGbmaBadMemoryAllocation(ERx("vfsequence"));
	}

	seqfrm->frfldno = frame->frfldno;
	/* Set before frame->frtrimno changed by vfseqbuild(). */
	seqfrm->frtrimno = frame->frtrimno;
	globy = 0;
	globx = 0;
	vfseqbuild(TRUE);

	/*
	** numfds is set up by vfseqbuild().  It equals the number of
	** non-derived fields.
	*/
	if (numfds < 2)
	{
		IIUGerr(E_VF00CD_There_are_no_fields, UG_ERR_ERROR, 0);
		dummy = seqfrm;
		IIVTdfDelFrame(&dummy);
		MEfree(seqfrm);
		return;
	}

	MEfill(sizeof(FRS_EVCB), '\0', &evcb);
	mu = muSequence;

	/* Force the next FTputmenu() call to re-construct the menu. */
	mu->mu_flags |= MU_NEW;

	FTclear();
	VTxydraw(seqfrm, globy, globx);
	VFfeatmode(VF_SEQ);
	for(;;)
	{
		evcb.event = fdNOCODE;
		FTclrfrsk();
		FTaddfrs(9, UNDO, NULL, 0, 0);
		FTaddfrs(1, HELP, NULL, 0, 0);
		FTaddfrs(3, QUIT, NULL, 0, 0);
		vfseqmu_put(mu);
		if (VTcursor(seqfrm, &globy, &globx, endFrame,
			endxFrm + 1 + IIVFsfaSpaceForAttr,
			0, (FLD_POS *)NULL, &evcb, VFfeature,
			RBF, FALSE, FALSE, FALSE, FALSE) 
			== fdopSCRLT)
		{
			continue;
		}
		VTdumpset(vfdfile);

		if (evcb.event == fdopFRSK)
		{
			menu_val = evcb.intr_val;
		}
		else if (evcb.event == fdopMUITEM)
		{
			if (IImumatch(mu, &evcb, &menu_val) != OK)
			{
				continue;
			}
		}
		else
		{
			/*
			**  Fix for BUG 7123. (dkh)
			*/
			menu_val = FTgetmenu(mu, &evcb);
			if (evcb.event == fdopFRSK)
			{
				menu_val = evcb.intr_val;
			}
		}

		VTdumpset(NULL);
		switch (menu_val)
		{
		case QUIT:
			/* check for sequece uniqueness */
			if (!vfseqck())
			{
				IIUGerr(E_VF00CE_Non_unique_sequence,
					UG_ERR_ERROR, 0);
				globy = curps->ps_begy;
				globx = curps->ps_begx;
				VTxydraw(seqfrm, globy, globx);
				continue;
			}
			break;

		case UNDO:
			vfseqrestore();
			break;

		case HELP:
			vfhelpsub(VFH_RESEQ,
				ERget(S_VF00CF_Field_Order_Menu), mu);
			continue;

		default:
			continue;
		}
		break;
	}
	FTclear();
	VFfeatmode(VF_NORM);
	dummy = seqfrm;
	IIVTdfDelFrame(&dummy);
	MEfree(seqfrm);
} 

# endif


/*
** vfseqrestore:
**	Routine to restore the original sequence numbers
**	for a form.
*/

# ifndef FORRBF
vfseqrestore()
{
	reg	FLDHDR	*hdr;
	reg	FLDHDR	**hdrlist;
		i4	i;
		i4	*bseq;

	bseq = osequence;
	hdrlist = ohdr;
	for (i = 0; i < numfds; i++, hdrlist++)
	{
		hdr = *hdrlist;
		hdr->fhseq = *bseq++;
	}
}
# endif

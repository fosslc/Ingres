/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>
# include	<pc.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<si.h>
# include	<lo.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	<er.h>
# include	<erft.h>

/*
** RECOVER.c
**
** Writes a "recovery" file for a frame driver session.
**
** Contains
**     	FTsvinput(name)
**		Allocate a recovery file with the name "name".
**
**	FTsvclose()
**		Closes the current recovery file.
**
**
** HISTORY
**	Stolen 9/13/82 (jrc) from vifred.
**
**	9/20/83 (nml) - Now use PG_SIZE for size of buffers to
**			allocate for recovery file (instead of BUFSIZ).
**			Get this value by including compat.h  (the
**			definition of PG_SIZE is actually in bzarch.h
**	03/04/87 (dkh) - Added support for ADTs.
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changes for 8000 series error numbers.
**	11/13/88 (dkh) - Rearranged code so that non-forms programs can link
**			 in IT* routines.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	08/02/91 (dkh) - Fixed bug 37514.  Don't set fdrecover flag unless
**			 the save file was opened successfully.
**	26-may-92 (leighb) DeskTop Porting Change: Added IIFTdsrDSRecords
**			 pointer to FTprocinit.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
**	19-May-2009 (kschendel) b122041
**	    DSRecords proc is really void, fix here.
*/


GLOBALREF char		*fdrcvname;
GLOBALREF FILE		*fdrcvfile;
GLOBALREF bool		fdrecover;
GLOBALREF bool		Rcv_disabled;	/* recovery file output temporarily disabled */
GLOBALREF LOCATION	fdrcvloc;

GLOBALREF	FLDHDR	*(*FTgethdr)();
GLOBALREF	FLDTYPE	*(*FTgettype)();
GLOBALREF	FLDVAL	*(*FTgetval)();
GLOBALREF	VOID	(*FTgeterr)();
GLOBALREF	STATUS	(*FTrngchk)();
GLOBALREF	i4	(*FTvalfld)();
GLOBALREF	VOID	(*FTdmpmsg)();
GLOBALREF	VOID	(*FTdmpcur)();
GLOBALREF	i4	(*FTfieldval)();
GLOBALREF	VOID	(*FTsqlexec)();
GLOBALREF	VOID	(*IIFTiaInvalidateAgg)();
GLOBALREF	VOID	(*IIFTpadProcessAggDeps)();
GLOBALREF	VOID	(*IIFTiaaInvalidAllAggs)();
GLOBALREF	i4	(*IIFTgtcGoToChk)();
GLOBALREF	i4	(*IIFTldrLastDsRow)();
GLOBALREF	void	(*IIFTdsrDSRecords)();
GLOBALREF	i4	IIFTltLabelTag;


/*
**  Initialize routine pointers to routines above FT layer.
**  It is here so non-forms programs can link it IT* routines.
*/

FTprocinit(proccb)
FRS_UTPR	*proccb;
{
	FTgethdr = proccb->gethdr_proc;
	FTgettype = proccb->gettype_proc;
	FTgetval = proccb->getval_proc;
	FTgeterr = proccb->error_proc;
	FTrngchk = proccb->rngchk_proc;
	FTvalfld = proccb->valfld_proc;
	FTdmpmsg = proccb->dmpmsg_proc;
	FTdmpcur = proccb->dmpcur_proc;
	FTfieldval = proccb->fldval_proc;
	FTsqlexec = proccb->sqlexec_proc;
	IIFTiaInvalidateAgg = proccb->inval_agg_proc;
	IIFTpadProcessAggDeps = proccb->eval_agg_proc;
	IIFTiaaInvalidAllAggs = proccb->inval_allaggs_proc;
	IIFTgtcGoToChk = proccb->gotochk_proc;
	IIFTldrLastDsRow = proccb->lastdsrow_proc;
	IIFTdsrDSRecords = proccb->dsrecords_proc;

	IIFTltLabelTag = FEgettag();
}



/*
** allocate the recovery file
*/

FTsvinput(name)
char	*name;
{
    	static char	buf[MAX_LOC + 1];

    	if (fdrcvname != NULL)
    		return;
	STcopy(name, buf);
    	fdrcvname = buf;
	LOfroms(PATH & FILENAME, fdrcvname, &fdrcvloc);
     	if (SIopen(&fdrcvloc, ERx("w"), &fdrcvfile) != OK)
    	{
		(*FTgeterr)(E_FT0039_TSTBADOPN, (i4) 1, fdrcvname);
		FTclose(NULL);
		PCexit(FAIL);
    	}
	fdrecover = TRUE;
}

FTsvclose(rem)
bool	rem;
{
    if (fdrcvname == NULL)
    	return;

    /*
    **  Added this to always put a carriage
    **  return at the end of the keystroke
    **  file. (dkh)
    */
    TDrcvwrite('\n');

    SIclose(fdrcvfile);
    if (rem)
    {
	    LOdelete(&fdrcvloc);
    }
    fdrcvname = NULL;
}

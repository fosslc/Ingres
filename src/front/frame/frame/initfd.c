/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**  INITFD.c  -  Initialize the Frame Driver
**
**  This routine initializes the frame driver and the commands
**  associated with it.  The routine first calls TDinitscr() to
**  initialize curses.  It then initializes the character
**  commands for the frame driver and what modes the commands
**  are accepted in.
**
**  Arguments:  none
**
**  History:  JEN  -  1 Nov 1981
**	12/22/86 (dkh) - Added support for new activations.
**	08/14/87 (dkh) - ER changes.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	07/23/88 (dkh) - Added exec sql immediate hook.
**	28-nov-88 (bruceb)
**		Add argument to initfd()--an event control block--to
**		be passed down to FTinit().  (For timeout.)
**	16-mar-89 (bruceb)
**		Arg to initfd() and FTinit() changed from evcb to
**		FRS_CB.  (For entry activation.)
**	07-jul-89 (bruceb)
**		New arg to initfd().  Func_cb is a control block containing
**		back pointers to runtime/tbacc routines.  Used for entry
**		activation and derivation processing.  Also, added new
**		function pointer to fdutprocs.
**	08/03/89 (dkh) - Added definition of IIFDevcb to get access to
**			 the event control block at the "frame" layer.
**	12/27/89 (dkh) - Put in handling for call back pointer gotochk_proc.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Added IIFDdsrDSRecords as a new utility procedure pointer.
**	01/27/92 (dkh) - Changed the way IIFDdsrDSRecords is set up so that
**			 it is not a link problem for various forms utilities.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      24-sep-96 (hanch04)
**          Global data moved to framdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-May-2009 (kschendel) b122041
**	    The DSRecords proc is actually void, change here.
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
# include	<frsctblk.h>
# include	<me.h>
# include	<nm.h>
# include	<si.h>
# include       <er.h>

GLOBALREF	FRS_RFCB	*IIFDrfcb ;
GLOBALREF	FRS_GLCB	*IIFDglcb ;

GLOBALREF	FRS_EVCB	*IIFDevcb ;

GLOBALREF	i4		(*IIFDldrLastDsRow)() ;
GLOBALREF	void		(*IIFDdsrDSRecords)() ;

GLOBALREF	FRS_UTPR	fdutprocs;
GLOBALREF	char		dmpbuf[];
GLOBALREF	FT_TERMINFO	frterminfo;

FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	FLDTYPE	*FDgettype();
FUNC_EXTERN	FLDVAL	*FDgetval();
FUNC_EXTERN	VOID	IIFDerror();
FUNC_EXTERN	STATUS	FDrngchk();
FUNC_EXTERN	i4	FDvalidate();
FUNC_EXTERN	VOID	FDdmpmsg();
FUNC_EXTERN	VOID	FDdmpcur();
FUNC_EXTERN	i4	FDfieldval();
FUNC_EXTERN	VOID	IIFDesiExecSqlImm();
FUNC_EXTERN	VOID	IIFViaInvalidateAgg();
FUNC_EXTERN	VOID	IIFDiaaInvalidAllAggs();
FUNC_EXTERN	VOID	IIFDpadProcessAggDeps();
FUNC_EXTERN	i4	IIFDgcGotoChk();

initfd(frscb, func_cb)				/* INITFD: */
FRS_CB		*frscb;
FRS_RFCB	*func_cb;
{

	/*
	**  Set up utility procedure pointers.
	*/
	fdutprocs.gethdr_proc = FDgethdr;
	fdutprocs.gettype_proc = FDgettype;
	fdutprocs.getval_proc = FDgetval;
	fdutprocs.error_proc = IIFDerror;
	fdutprocs.rngchk_proc = FDrngchk;
	fdutprocs.valfld_proc = FDvalidate;
	fdutprocs.dmpmsg_proc = FDdmpmsg;
	fdutprocs.dmpcur_proc = FDdmpcur;
	fdutprocs.fldval_proc = FDfieldval;
	fdutprocs.sqlexec_proc = IIFDesiExecSqlImm;
	fdutprocs.inval_agg_proc = IIFViaInvalidateAgg;
	fdutprocs.eval_agg_proc = IIFDpadProcessAggDeps;
	fdutprocs.inval_allaggs_proc = IIFDiaaInvalidAllAggs;
	fdutprocs.gotochk_proc = IIFDgcGotoChk;
	fdutprocs.lastdsrow_proc = IIFDldrLastDsRow;
	fdutprocs.dsrecords_proc = IIFDdsrDSRecords;

	IIFDrfcb = func_cb;
	IIFDglcb = frscb->frs_globs;

	IIFDevcb = frscb->frs_event;

	/*
	**  Start up FT.
	*/
	if (FTinit(&fdutprocs, frscb, dmpbuf) == FAIL)
	{
		return(FALSE);
	}

	/*
	**  Get user's terminal information.
	*/

	FTterminfo(&frterminfo);

	return (TRUE);
}

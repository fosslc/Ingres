/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** 10-nov-90 (sandyd)
**      Workaround VAXC optimizer bug, encountered in both VAXC V3.0-031 
**	and V2.3-024.
NO_OPTIM = vax_vms ris_us5 rs4_us5 m88_us5 sgi_us5 ris_u64 i64_aix i64_hpu
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<ex.h>
# include	<er.h>
# include	<lo.h>
# include	<frame.h>

/*
** interface to vifred from ABF.
**
**  History:
**	08/14/87 (dkh) - ER changes.
**	11-apr-89 (bruceb)
**		Renamed IIvifred to IIVFvqvVQVifred and modified to be
**		the interface from VQ.
**	31-may-89 (bruceb)
**		Re-init form and line table memory on return from IIVFvqv.
**	29-jun-89 (bruceb)
**		Renamed to IIVFabvABVifred and modified the interface.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	11/02/90 (dkh) - Added code to change dml in ADF_CB to DB_SQL so that
**			 default datatype nullability in created fields is the
**			 same as standalone Vifred.  On exit, dml is set
**			 back to whatever it was before this was called.
**  25-nov-91 (gautam) - Added ris_us5 to NO_OPTIM list.
**  23-jun-92 (thomasm) Added hp8_us5 to NO_OPTIM list.
**  30-Jul-1992 (fredv)
**	Added NO_OPTIM for m88_us5. The symptom was vision sep tests failed.
**	It looked the the optimizer generated bad code so that old_dml wasn't
**	saved right. Therefore, when we restore qlang, it becomes junk.
**	09/01/92 (dkh) - Added support for group moves.
**	29-jan-97 (mosjo01)
**		Added no optim for rs4_us5 because old_dml was junk in vision
**		sep testing. Seeing ris_us5 further confirmed solution.
**	19-jan-1999 (muhpa01)
**		Removed NO_OPTIM for hp8_us5.
**      21-Jul-99 (linke01)
**              added NO_OPTIM on sgi_us5 IRIX64 6.5 on II2.5 port, sympotom
**              same as fredv's comment. it didn't happen on IRIX64 6.4 II2.0
**              port. 
**      02-Aug-1999 (hweho01)
**              Added NO_OPTIM for ris_u64. The symptom was failure of vision 
**              sep test. The version of compiler is AIX C 3.6.0.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      29-May-2005 (hweho01)   
**          Added NO_OPTIM for i64_hpu platform, avoid parse error (E_FI206C)
**          during FE/vis54.sep test. Star 14063593.
**	21-Aug-2009 (kschendel) 121804
**	    Need frame.h to satisfy gcc 4.3.
*/

/*
**	iivifred.c
*/


/*
** Var to indicate we took a long jump using exception mechanism.
*/

# ifndef FORRBF
FUNC_EXTERN	VOID	IIVTdfDelFrame();
FUNC_EXTERN	DB_LANG	IIAFdsDmlSet();

GLOBALREF	bool	vftkljmp;


vfljmp(exargs)
EX_ARGS *exargs;
{
	if (exargs->exarg_num != EXVFLJMP)
	{
		return(EXRESIGNAL);
	}
	else
	{
		return(EXDECLARE);
	}
}

/*{
** Name:	IIVFabvABVifred	- Interface to VIFRED from ABF
**
** Description:
**	Interface to VIFRED from ABF.  Edit the specified form, returning
**	to ABF on saving the form or ending from form edit phase.  This
**	routine will be ABF's sole communication with Vifred.
**
**	This call is equivalent to:
**		vifred dbname -F frmname
**	or
**		vifred dbname -F frmname -Csource -Ssymbol
**
**	Note that this routine doesn't set up the FDhandler exception
**	code.  This is expected to be already established in ABF.
**
** Inputs:
**	dbasename	Name of the already opened database.
**	frmname	Name of the frame to edit.  If it doesn't exist, allow the
**		user to create it.
**	compile	TRUE if the form should be compiled after being saved.  The
**		following two arguments are ignored if this is FALSE.
**	source	Name of compiled form file.
**	symbol	Name of the symbol of the compile form.
**
** Outputs:
**
**	Returns:
**		retstatus	At present, only OK.
**
**	Exceptions:
**		None
**
** Side Effects:
**
** History:
**	11-apr-89 (bruceb)	Written.
*/
STATUS
IIVFabvABVifred(dbasename, frmname, compile, source, symbol)
char	*dbasename;
char	*frmname;
bool	compile;
char	*source;
char	*symbol;
{
    i4		argc;
    char	*argv[6];
    EX_CONTEXT	context;
    char	srcname[MAX_LOC + 1];
    char	symname[FE_MAXNAME + 1];
    STATUS	retstatus = OK;
    DB_LANG	old_dml;

# ifdef DATAVIEW
    if (IIMWsmwSuspendMW() == FAIL)
	return(FAIL);
# endif	/* DATAVIEW */
    vftkljmp = FALSE;
    if (EXdeclare(vfljmp, &context) == OK)
    {
	Vfeqlcall = TRUE;	/* Not a standalone VIFRED. */

	/*
	**  Set dml to SQL to be consistent with standalone
	**  vifred when it comes to creating fields with
	**  datatype nullability set to y.
	*/
	old_dml = IIAFdsDmlSet(DB_SQL);

	argv[0] = ERx("vifred");
	argv[1] = dbasename;
	argv[2] = ERx("-F");
	argv[3] = frmname;

	if (compile)
	{
	    STprintf(srcname, ERx("-C%s"), source);
	    argv[4] = srcname;		/* -Csource */

	    STprintf(symname, ERx("-S%s"), symbol);
	    argv[5] = symname;		/* -Ssymbol */

	    argc = 6;
	}
	else
	{
	    argc = 4;
	}
	vifred(argc, argv);
    }

    /* vftkljmp will be TRUE if vfexit() was called.  (No use for this now.) */

    /*
    **  Clean up cross hair stuff and rulers when we exit back to vision.
    */
    IIVFru_disp = FALSE;
    IIVFxh_disp = FALSE;

    /*
    **  We would like to free up the small amount of memory associated
    **  with the features for the cross hair POS structures.  But since
    **  these were allocated with FEreqmem(), it is not possible to just
    **  free these individually.  This just gets lost in the shuffle as
    **  user enters/exits Vifred from Vision.  Since this is only 4 bytes
    **  each, we ignore it for now.
    MEfree(IIVFvcross->ps_feat);
    MEfree(IIVFhcross->ps_feat);
    */

    /*
    **  The call to FEfree(vflinetag) below will free up the
    **  memory associated with the cross hair POS structures.
    **  So we just need to reset the pointers here.
    */
    IIVFvcross = NULL;
    IIVFhcross = NULL;

    /* Free up the frame and line table memory. */
    IIVTdfDelFrame(&frame);
    if (vffrmtag)
    {
	if (vfendfrmtag)
	{
	    vffrmtag = FEendtag();
	    vfendfrmtag = FALSE;
	}
	FEfree(vffrmtag);
	vffrmtag = 0;
    }
    if (vflinetag)
    {
	FEfree(vflinetag);
	vflinetag = 0;
    }
    if (IIVFmtMiscTag)
    {
	IIUGtagFree(IIVFmtMiscTag);
	IIVFmtMiscTag = 0;
    }
    linit();
    ndinit();
    Vfformname = NULL;	/* Satisfy future calls on getargs. */
    (void) FDsetparse(FALSE);

    /*
    **  Set dml back to whatever it was before we were called.
    */
    _VOID_ IIAFdsDmlSet(old_dml);

    EXdelete();
# ifdef DATAVIEW
    if (IIMWrmwResumeMW() == FAIL)
	return(FAIL);
# endif	/* DATAVIEW */
    return(retstatus);
}
# endif

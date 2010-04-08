/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ol.h>
#include	<si.h>
#include	<lo.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
exec sql include	<ui.sh>;
#include	<adf.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<feconfig.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"itcstr.h"

/**
** Name:	ilcall.c -	Execute IL Call Statements
**				    (Frame/Procedure/System/Subsystem/Return.)
** Description:
**	Executes IL statements for calls and returns.  Includes a different
**	'IIITstmtExec()' routine for each type of statement.
**
** History:
**	Revision 5.1  87/04/23  arthur
**	86/12/08  09:32:29  arthur
**	When a called frame has returned, clear the (ABRTSPRM *) element of the
**	calling frame via a call to IIITFsmSetPrm().  Fix for bug 10987.
**	86/10/01  arthur
**	Initial revision.
**
**	Revision 6.2  88/12/04  joe
**	Updated for 6.2.
**	89/05  wong  Removed clear/redisplay after subsystem/system call.
**
**	Revision 6.3  89/11  wong
**	Modified to use interpreter session while fetching IL.  Bug #8364.
**
**	Revision 6.3/03/00  89/11  wong
**	Moved data initialization ('IIARhfiHidFldInit()') here from
**	'IIITinitializeEx()' in "ildisp.c".
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**	01/14/91 (emerson)
**		Fix for bug 34840 in IIITeilExecuteIL.
**	02/17/91 (emerson)
**		Changed FD_VERSION to PR_VERSION for bug 35946 (see fdesc.h).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added IIITmpMainProc and IIITlpLocalProc;
**		modified IIIT4gCallEx, _returnEx, IIITeilExecuteIL.
**	08/17/91 (emerson)
**		Small refinement in IIIT4gCallEx.
**
**	Revision 6.5
**	26-jun-92 (davel)
**		added argument to the IIOgvGetVal() calls.
**	06-nov-92 (davel)
**		When switching to/from the Interpreter session (e.g. to fetch
**		forms or frames), use IIARrsiRestoreSessID(), which will
**		also handle the case where no session was active.
**		Modified both IIIT4gCallEx() and IIITeilExecuteIL().
**	16-nov-93 (donc)
**		Modified IIOrpRunProc. Added support for EXEC 4GL CALLPROC.
**		Need to look at external global for call arguments in this
**		case and force an entry onto the control stack and then
**		set the parameters.
**	17-nov-93 (donc)
**		Use wrapper routine IIARgarGetArRtsprm to access global
**		address for 3GL-passed input parameters.
**	18-nov-93 (donc)
**		Modified IIOrpRunProc. Added support for EXEC 4GL CALLPROC.
**		Need to look at external global for call arguments in this
**		case and force an entry onto the control stack and then
**		set the parameters.
**	22-nov-93 (donc)
**		Modified IIOrfRunFrame. Added support for EXEC 4GL CALLFRAME.
**		Need to look at external global for call arguments in this
**		case and force an entry onto the control stack and then
**		set the parameters.
**	22-dec-1993 (donc) Bug 56911
**		Allocate a starting frame pointer in case one doesn't already
**		exist. This situation can occur if a 3GL procedure is the
**		the first thing run from the intrepreter and it does a
**		EXEC 4GL CALLFRAME or CALLPROC
**	29-dec-1993 (donc)
**	 	Modified to do a IIOFufPushFrame for RunProc and RunFrame.
**		These pushes onto the frame stack are then Pop'ped
**		by IIOFpfPopFrame. This method is cleaner for fixing bug
**	        56911, as IIOframeptr manipulation is localized within if.c
**	1-feb-94 (donc)
**		Cast arguments to IIOFufPushFrame explicitly. VMS wasn't
**		detecting that an EXEC 4GL call was happening.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for _returnEx() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    30-May-2006 (hweho014)
**        Changed the old style parameter list in  _returnEx(), prevent 
**        compiler error on AIX platform.
*/

VOID	IIOilInterpLoop();
VOID	IIOFsesSetExecutedStatic();
STATUS	IIOFufPushFrame();
STATUS	IIOFpfPopFrame();
STATUS	IIOFucPushControl();
STATUS	IIOFpcPopControl();
VOID	IIARrsiRestoreSessID();

static _returnEx(
		IL	retstmt,
		DB_DATA_VALUE	*ret);
/*{
** Name:	IIIT4gCallEx() -	Execute call to OSL frame or procedure
**
** Description:
**	Executes the OSL 'callframe' and 'callproc' statements,
**	when they involve a call to an OSL frame or procedure.
**
** IL statements:
**	CALLF VALUE
**	CALLP VALUE
**	CALLPL VALUE SID
**
**	The VALUE is the name of the frame or procedure being called.
**	For a CALLPL (call local procedure), SID specifies where
**	the local procedure starts (in the current code block).
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	10/01/86 (agh) - Written.
**	12/14/88 (joe) - Updated for 6.2 interpreter.
**	06/90 (jhw) - Added code to initialize form in interpreter session not
**		the user's to avoid transaction collisions.  Bug #30774.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Added logic to handle them.
**		Revised logic in keeping with changes to if.c.
**		Also got rid of obsolete code bracketed by #ifdef PC.
**	08/17/91 (emerson)
**		Cleanup: Use FEtsalloc instead of doing awkward gyrations.
**	08/21/91 (emerson)
**		Fix bug uncovered by previous change.  When making a copy
**		(in tagged memory) of the string containing the name of
**		the called local procedure, a tag of 0 was being specified
**		in the case where no parameters were passed on the first call
**		to a local procedure (because IIITprBuildPrmEx was never called)
**		As long we were using FEreqmem, this caused a slow memory leak.
**		FEtsalloc, however, does not accept a zero tag.
**		(Dave thinks this is just an oversight).  This caused
**		the interpreter to issue error E_IT0018.  I am fixing the
**		problem by calling FEgettag if IIITprBuildPrmEx wasn't called.
**		(IIOFfpFreePrm (in if.c) has been modified to free memory
**		whenever a tag has been acquired, even if there were no params).
**		I've also added logic to clean up better if the FEtsalloc
**		still does fail.
**	26-aug-92 (davel)
**		Add check for IL modifier ILM_EXEPROC (for EXECUTE PROCEDURE).
*/
static	char	*routine_name;	/* passed from IIIT4gCallEx to IIITlpLocalProc*/

VOID
IIIT4gCallEx ( stmt )
register IL	*stmt;
{
	u_i4	rval;
	TAGID		tag;

	/*
	** Save return addres.
	*/
	IIOFsaSetAddr(IIOstmt - IIOFgbGetCodeblock());

	if ( (*stmt&ILM_MASK) == IL_CALLF )
	{
		/* Switch sessions and call 'IIARformGet()' to initialize form
		** for frame in case it is fetched from the DB.  Then, it will
		** be fetched from the interpreter session and not the user's.
		*/

		char	*frmname;
		EXEC SQL BEGIN DECLARE SECTION;
		i4	saved_id;
		EXEC SQL END DECLARE SECTION;

		frmname = IIITtcsToCStr(ILgetOperand(stmt, 1));

		/* Get current session ID */
		EXEC SQL INQUIRE_INGRES ( :saved_id = SESSION );

		/* Switch to Interpreter/Front-End Session */
		EXEC SQL SET_INGRES ( SESSION = :UI_SESSION );

		IIARformGet(frmname);	/* ... may fetch form from DB */

		/* Switch to (saved) User/Application Session */
		IIARrsiRestoreSessID(saved_id);

		routine_name = frmname;
		abrtscall( frmname, IIOFtpTopPrm() );
		IIITrcsResetCStr();
	}
	else if ( (*stmt&(ILM_MASK|ILM_EXEPROC)) == (IL_CALLP|ILM_EXEPROC) )
	{
		/* Database procedure */
		routine_name = IIITtcsToCStr(ILgetOperand(stmt, 1));
		IIARdbProcCall( routine_name, IIOFtpTopPrm() );
		IIITrcsResetCStr();
	}
	else if ( (*stmt&ILM_MASK) == IL_CALLP )
	{
		routine_name = IIITtcsToCStr(ILgetOperand(stmt, 1));
		abrtsretcall( routine_name, IIOFtpTopPrm() );
		IIITrcsResetCStr();
	}
	else /* ( (*stmt&ILM_MASK) == IL_CALLPL ) */ /* call local procedure */
	{
		char		*name;

		/*
		** Make the first instruction in the local procedure
		** be the next statement to be executed.
		*/
		IIOsiSetIl(ILgetOperand(stmt, 2));

		/*
		** I make a "safe" copy of the routine name,
		** because it seems to be the char const in the frame,
		** which might get kicked out of memory if we call another frame
		** [But doesn't this concern also apply to IIITtcsToCStr above?]
		**
		** We use the same memory tag that we used for the parameter
		** list structure.  If there was no parameter list, we must
		** get a tag now.
		*/
		tag = IIOFgtGetPrmTag();
		if (tag == 0)
		{
			tag = FEgettag();
			IIOFgtSetPrmTag(tag);
		}
		name = (char *)IIOgvGetVal(ILgetOperand(stmt, 1), 
					DB_CHR_TYPE, (i4 *)NULL);
		routine_name = (char *)FEtsalloc(tag, name);
		if (routine_name == NULL)
		{
			IIOerError(ILE_STMT, ILE_NOMEM, (char *) NULL);
			IIOstmt = IIOFgbGetCodeblock() + IIOFgaGetAddr();
			IIOFfpFreePrm();
			return;
		}

		/*
		** Run the local procedure.  IIARlpcLocalProcCall
		** pushes and pops an entry on the ABF run-time stack,
		** and re-invokes the main interpreter loop.
		*/
		IIARlpcLocalProcCall(routine_name, IIOFtpTopPrm(),
				     (i4 (*)())IIOilInterpLoop,
				     IIOFdgDbdvGet(1));
	}

	/*
	** Upon return from called OSL frame or procedure, we must:
	** (1) Restore the current address for the calling frame.
	** (2) Free the parameter list.
	** (3) Recompute pointers to FDESC structures in case the frame
	**     got kicked out of memory and subsequently reloaded
	**     at a different location.
	*/
	IIOstmt = IIOFgbGetCodeblock() + IIOFgaGetAddr();
	IIOfrmErr = FALSE;
	IIOFfpFreePrm();
	IIOFscSetFdesc();
	return;
}

/*{
** Name:	IIITsuCalSubSysEx() -	Execute OSL 'call subsystem'
**
** Description:
**	Executes the OSL 'call subsystem' statement.
**
**	Does this by calling IIARprogram.
**
** IL statements:
**
**	IL_CALSUBSY	strvalProgramName strvalFormatString  int#OfParams
**	[
**		{IL_TL1ELM	valParameterValue}
**	]
**
**	The valParameterValues are all guaranteed to be variables, and
**	not any constants.  This is good, since IIARprogram needs a DBV.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	aug-1986	Written. (agh)
**	05/89 (jhw)  Removed clear/redisplay after sub-system call.  This is now
**		handled by 'IIARprogram()'.  JupBug #6107.
*/
VOID
IIITsuCalSubSysEx ( stmt )
register IL	*stmt;
{
    i4			num_of_args;
    DB_DATA_VALUE	*dbvs = NULL;
    DB_DATA_VALUE	*dp;
    register IL		*next;

    num_of_args = (i4) ILgetOperand(stmt, 3);
    if (num_of_args != 0)
    {
	dbvs = (DB_DATA_VALUE *)MEreqmem(0,
					(u_i2)sizeof(DB_DATA_VALUE)*num_of_args,
					FALSE, (STATUS *) NULL
	);
	if (dbvs == NULL)
	{
	    IIOerError(ILE_STMT, ILE_NOMEM, (char *) NULL);
	    return;
	}
	dp = dbvs;
	next = IIOgiGetIl(stmt);
	if ((*next&ILM_MASK) != IL_TL1ELM)
	{
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("IL_TL1ELM in CallSubSys"),
		       (char *) NULL);
	    MEfree((PTR) dbvs);
	    return;
	}
	do 
	{
	    MEcopy((PTR) IIOFdgDbdvGet(ILgetOperand(next, 1)),
		    sizeof(DB_DATA_VALUE),
		    (PTR) dp);
	    dp++;
	    next = IIOgiGetIl(next);
	} while ((*next&ILM_MASK) == IL_TL1ELM && dp < dbvs + num_of_args);
	if ((*next&ILM_MASK) != IL_ENDLIST || dbvs + num_of_args != dp)
	{
	    IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist in CallSubSys"),
		       (char *) NULL);
	    MEfree((PTR) dbvs);
	    return;
	}
    }
    IIclrscr();
    IIARprogram(IIITtcsToCStr(ILgetOperand(stmt, 1)),
		IIITtcsToCStr(ILgetOperand(stmt, 2)),
		num_of_args,
		dbvs
	);
    if (dbvs != NULL)
    {
	MEfree((PTR) dbvs);
    }
}

/*{
** Name:	IIITsyCalSysEx() -	Execute OSL 'call system'
**
** Description:
**	Executes the OSL 'call system' statement.
**
** IL statements:
**	CALSYS VALUE
**
**	The VALUE is the operating system command to be executed,
**	or the NULL VALUE if no command is specified.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	aug-1986	Written. (agh)
**	05/89 (jhw)  Removed clear/redisplay after call.
*/
VOID
IIITsyCalSysEx ( stmt )
register IL	*stmt;
{
	IIclrscr();
# ifdef CDOS
        /*
        **      Temporarily close the interp file for Concurrent DOS.  C-DOS
        **      closes the parent's file handles when the child does an exit.
        **      Therefore we must close the file and reopen it after we come
        **      back.
        */
        IIORssSessSwap(&il_irblk);
        QG_shutdown();
# endif /* CDOS */
	_VOID_ abexesys( IIITtcsToCStr(ILgetOperand(stmt, 1)) );
# ifdef CDOS
        /*
        **      Reopen the interp file.
        */
        if (QG_restart() || IIORsuSessUnswap(&il_irblk))
        {
                char    bufr[81];
                FEprompt("Couldn't restart 4GL interpreter.  Press ENTER",
                        FALSE,80,bufr);
        }
# endif
}

/*{
** Name:	IIITreturnEx() -	return from an OSL frame or proc
**
** Description:
**	Execute a 'return' statement when returning from an OSL frame
**	or procedure.
**
** IL statements:
**	RETFRM VALUE
**	[ RETPROC VALUE ]
**
**		The VALUE is the value being returned, or NULL if none.
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Change the way _returnEx gets the routine name.
**		Also change the way IIOfrFrmReturn decides if it's in a proc.
*/
VOID
IIITreturnEx ( stmt )
IL	*stmt;
{
	DB_DATA_VALUE	*ret;

	ret = ( ILNULLVAL(ILgetOperand(stmt, 1)) )
	    	? NULL : IIOFdgDbdvGet(ILgetOperand(stmt, 1));

	_returnEx((*stmt&ILM_MASK), ret);
}

/*{
** Name:	IIOfrFrmReturn() -	return from an OSL frame or proc.
**
** Description:
**	Return without a value, emulating a 'return' statement without a value. 
**	This handles exiting on timeout, and other situations where we
**	"drop off" the end of a display loop without executing an explicit
**	"return" statement.
*/
IIOfrFrmReturn()
{
	if (IIOFfnFormname() == NULL || IIOFilpInLocalProc())
	{
		_returnEx( IL_RETPROC, (DB_DATA_VALUE *)NULL );
	}
	else
	{
		_returnEx( IL_RETFRM, (DB_DATA_VALUE *)NULL );
	}
}

static
_returnEx(IL retstmt, DB_DATA_VALUE *ret)
{
	ABRTSPRM	*prm;
	ER_MSGID	type;
    
	prm = IIOFgpGetPrm();
	if (retstmt == IL_RETFRM)
	{
		IIendfrm();
		IIARfclFrmClose(prm, IIOFdgDbdvGet(1));
		type = F_IT0003_frame;
	}
	else
	{
		IIARpclProcClose(prm, IIOFdgDbdvGet(1));
		type = F_IT0004_procedure;
	}
	IIARrvlReturnVal(ret, prm, IIOFrnRoutinename(), ERget(type));
			 
	/*
	** Set the next statement to execute to NULL since this is
	** a return.
	*/
	IIITsniSetNextIl((IL *) NULL);
}

/*{
** Name:	IIITexitEx() -	exit from an application
**
** Description:
**	Executes the OSL 'exit' statement by exiting from an application.
**
** IL statements:
**	EXIT
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
VOID
IIITexitEx ( stmt )
IL	*stmt;
{
#ifdef lint
	stmt = stmt;
#endif /* lint */

	IIOexExitInterp(OK);
}

/*{
** Name:	IIITstopEx() -	Execute IL 'stop' statement
**
** Description:
**	Executes the IL 'stop' statement, used for testing and debugging.
**	This statement is available, within RTI only, to stop (close)
**	an application.
**
** IL statements:
**	STOP
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
*/
IIITstopEx ( stmt )
IL	*stmt;
{
#ifdef lint
	stmt = stmt;
#endif /* lint */

	IIOexExitInterp(FAIL);
}

/*{
** Name:	IIITprBuildPrmEx() -	Build ABRTSPRM struct for a call
**
** Description:
**	Builds an ABRTSPRM structure for a call on another frame or procedure.
**
** IL statements:
**	IL_PARAM valReturnLoc* valQueryStruct*
**               int#Formals int#PVs boolContainsAll
**	[
**          {
**	         IL_TL1ELM  valFormalName
**          }
**	    IL_ENDLIST
**      ]
**      [
**          {
**	         IL_PVELM valActualValue boolByRef
**	    }
**          IL_ENDLIST
**      ]
**
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	aug-1986	Written. (agh)
*/
VOID
IIITprBuildPrmEx ( stmt )
register IL	*stmt;
{
	register IL		*next;
	register ABRTSPRM	*prm;
	register ABRTSPV	*pv;
	register ABRTSOPRM	*oprm;
	char			*block;
	char			**formalp;
	i4			size = 0;
	i4			fsize;		/* number of formals */
	i4			pvsize;		/* number of PV structures */
	IL			retdbdv;	/* index of return value's
						   DB_DATA_VALUE */
	i2			tag;

	tag = FEgettag();
	size = sizeof(ABRTSPRM) + sizeof(ABRTSOPRM);
	fsize = ILgetOperand(stmt, 3);
	pvsize = ILgetOperand(stmt, 4);

	/*
	** Allow for space needed for list of formals (each a
	** character pointer), plus list of PV structures.
	*/
	size += (pvsize * sizeof(char *))	/* for formals */
		+ (pvsize * sizeof(ABRTSPV));	/* for parameter vector */

	/*
	** Allocate memory.
	*/
	block = (char *)FEreqmem(tag, (u_i2) size, TRUE, (STATUS *) NULL);
	if (block == NULL)
	{
		IIOerError(ILE_STMT, ILE_NOMEM, (char *) NULL);
		return;
	}

	/*
	** Fill in ABRTSPRM and ABRTSOPRM structures, and the vectors
	** their elements point to.
	** First, set up pointers to first elements to be filled in.
	*/
	prm = (ABRTSPRM *) block;
	size = sizeof(ABRTSPRM);
	oprm = (ABRTSOPRM *) (block + size);
	size += sizeof(ABRTSOPRM);
	pv = (ABRTSPV *) (block + size);	/* ptr into array of PVs */
	size += (pvsize * sizeof(ABRTSPV));
	formalp = (char **) (block + size);	/* ptr into list of formals */

	/*
	** Next, actually assign values to the members of the ABRTSPRM
	** and ABRTSOPRM structures, and fill in the vectors.
	** The values for these vectors become known as
	** successive IL statements are read in.
	*/
	prm->pr_zero = 0;
	prm->pr_version = PR_VERSION;
	prm->pr_oldprm = oprm;
	prm->pr_actuals = pv;
	prm->pr_formals = formalp;
	prm->pr_argcnt = pvsize;
	prm->pr_flags = (ILgetOperand(stmt, 5) == 0) ? 0 : ABPIGNORE;

	retdbdv = ILgetOperand(stmt, 1);
	if (ILNULLVAL(retdbdv))
	{
		prm->pr_rettype = 0;
		prm->pr_retvalue = (i4 *) NULL;
	}
	else
	{
		prm->pr_rettype = DB_DBV_TYPE;
		prm->pr_retvalue = (i4 *) IIOFdgDbdvGet(retdbdv);
	}

	/*
	** Fill in the ABRTSOPRM structure.  All of its elements, except
	** possibly for a query being passed between frames, are NULL.
	*/
	oprm->pr_tlist = NULL;
	oprm->pr_argv = NULL;
	oprm->pr_tfld = NULL;
	oprm->pr_qry = (ILNULLVAL(ILgetOperand(stmt, 2)) ? (QRY *) NULL
		: (QRY *) IIOgvGetVal(ILgetOperand(stmt, 2), 
					DB_QUE_TYPE, (i4 *)NULL));

	next = IIOgiGetIl(stmt);
	if (fsize == 0)
	{
		prm->pr_formals = (char **) NULL;
	}
	else
	{
		register i4	i;

		/*
		** If there are more PVs than formals, fill in NULL for
		** the non-corresponding formals, which must come first
		** in the array.  This would be relevant in a future
		** version of OSL which allowed mixed lists of parameters
		** with both positional and keyword-designated elements.
		*/
		for ( i = pvsize - fsize ; i-- > 0 ; ++formalp )
			*formalp = (char *) NULL;
		/*
		** Read in the 1-valued list of values of formals.
		*/
		while ((*next&ILM_MASK) == IL_TL1ELM)
		{
			*formalp++ = FEtsalloc(tag,
					     IIOgvGetVal(ILgetOperand(next, 1),
						     DB_CHR_TYPE, (i4 *)NULL)
			);
			next = IIOgiGetIl(next);
		}
		if ((*next&ILM_MASK) != IL_ENDLIST || prm->pr_formals + pvsize != formalp)
		{
			IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"),
				(char *) NULL);
			return;
		}
		next = IIOgiGetIl(next);
	}

	/*
	** Read in the 2-valued list of PV value, and whether passed
	** by reference (1) or by value (0).
	*/
	while ((*next&ILM_MASK) == IL_PVELM)
	{
		pv->abpvtype = (ILgetOperand(next, 2) == 1)
			       ? -DB_DBV_TYPE : DB_DBV_TYPE;
		pv->abpvsize = 0;
		/*
		** If this points at a constant, won't it go away?
		*/
		pv->abpvvalue = (char *) IIOFdgDbdvGet(ILgetOperand(next, 1));
		++pv;
		next = IIOgiGetIl(next);
	}
	if ((*next&ILM_MASK) != IL_ENDLIST || prm->pr_actuals + pvsize != pv)
		IIOerError(ILE_STMT, ILE_ILMISSING, ERx("endlist"), (char *) NULL);

	/*
	** Set the (ABRTSPRM *) element of the control section of the
	** current frame to point to the newly built param.
	*/
	IIOFsmSetPrm(prm, tag);

	return;
}

/*{
** Name:	IIOrfRunFrame	-	Get and run an OSL frame
**
** Description:
**	Gets and runs an OSL frame.
**	Called from the ABF runtime system.
**	The name of this routine was previously passed to the ABF RTS
**	via abrtsinterp().
**
** Inputs:
**	name	Name of the frame.
**	param	The runtime parameter structure passed to the frame.
**		This argument is not actually used by the interpreter,
**		since the ABRTSPRM structure is already pointed to from
**		the interpreter's frame stack.
**	frm	The entry for the frame on ABF's runtime stack.
**
**
** History
**	29-sep-1988 (Joe)
**		Changed call to IIORfgFrmGet to account for new
**		ilrf interface in 6.0.  Now the pointer
**		to the first IL statement and the pointer to the
**		symbol table are returned in the frminfo structure.
*/
PTR
IIOrfRunFrame(param, name, frm)
ABRTSPRM	*param;
char		*name;
ABRTSFRM	*frm;
{
	FID	fid;

    	ABRTSPRM *nrtsprm;
    	PTR     iiar_Rtsprm;
    	FUNC_EXTERN PTR IIARgarGetArRtsprm();
    	FUNC_EXTERN VOID IIARcarClrArRtsprm();
    	TAGID tag;
    	STATUS rval;
        bool pop = FALSE;


#ifdef lint
	param = param;
#endif /* lint */

	fid.name = name;
	fid.id = frm->abrfrnuser.abrfid;

        iiar_Rtsprm = IIARgarGetArRtsprm();
        if ( iiar_Rtsprm != NULL )
        {
           pop = TRUE;
      	   nrtsprm = (ABRTSPRM *)iiar_Rtsprm;
           tag = FEgettag();

           /* If the frame pointer is NULL, this indicates that the
           ** frame is being called by a "start-up" 3GL procedure
           ** via an EXEC 4GL CALLFRAME. We need to initialize the
           ** frame pointer to an initial "stub" value. To do otherwise
           ** will cause a push onto the contral stack to fail, as it
           ** assumes a valid frame pointer already exists
           */
           rval = IIOFufPushFrame(&fid, (char *)NULL, (bool)FALSE, (bool)TRUE);
           if ( rval != OK )
           {
	       IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
	       return;
           }  
           rval = IIOFucPushControl( name, 0, NULL, 0, 0, 0, 0, 0, 0 ); 
           if ( rval != OK )
           {
	       IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
	       return;
           }  
           IIOFsmSetPrm( nrtsprm, tag );
           IIARcarClrArRtsprm();
        } 

	IIITeilExecuteIL( &fid,
			ERget(F_IT0003_frame), 
			frm->abrform->abrfoname,
			(bool) frm->abrfrnuser.abrstatic != 0
	);
        if (pop)  {
            IIOFpcPopControl(); 
	    IIOFpfPopFrame();
        }
             
	return NULL;  
}

/*{
** Name:	IIOrpRunProc	-	Get and run an OSL procedure
**
** Description:
**	Gets and runs an OSL procedure.
**	Called from the ABF runtime system.
**	The name of this routine was previously passed to the ABF RTS
**	via abrtsinterp().
**
** Inputs:
**	name	Name of the procedure.
**	param	The runtime parameter structure passed to the procedure.
**	proc	The entry for the procedure on ABF's runtime stack.
**
*/
PTR
IIOrpRunProc(param, name, proc)
ABRTSPRM	*param;
char		*name;
ABRTSPRO	*proc;
{
    FID	fid;
    ABRTSPRM *nrtsprm;
    PTR     iiar_Rtsprm;
    FUNC_EXTERN PTR IIARgarGetArRtsprm();
    FUNC_EXTERN VOID IIARcarClrArRtsprm();
    TAGID tag;
    STATUS rval;
    bool pop = FALSE;


#ifdef lint
	param = param;
#endif /* lint */

    fid.name = name;
    fid.id = proc->abrpfid;

    iiar_Rtsprm = IIARgarGetArRtsprm();
    if ( iiar_Rtsprm != NULL )
    {
       pop = TRUE;
       nrtsprm = (ABRTSPRM *)iiar_Rtsprm;
       tag = FEgettag();

       /* If the frame pointer is NULL, this indicates that the
       ** frame is being called by a "start-up" 3GL procedure
       ** via an EXEC 4GL CALLFRAME. We need to initialize the
       ** frame pointer to an initial "stub" value. To do otherwise
       ** will cause a push onto the contral stack to fail, as it
       ** assumes a valid frame pointer already exists
       */
       rval = IIOFufPushFrame(&fid, (char *)NULL, (bool)FALSE, (bool)TRUE);
       if ( rval != OK )
       {
	   IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
	   return;
       }
       rval = IIOFucPushControl( name, 0, NULL, 0, 0, 0, 0, 0, 0 );
       if ( rval != OK )
       {
	   IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
	   return;
       }
       IIOFsmSetPrm( nrtsprm, tag );
       IIARcarClrArRtsprm();
    }

    IIITeilExecuteIL(&fid, ERget(F_IT0004_procedure), (char *) NULL, FALSE);

    if (pop) {	
        IIOFpcPopControl();
	IIOFpfPopFrame();
    }
    return NULL;
}

/*{
** Name:	IIITeilExecuteIL  - Execute an IL object.
**
** Description:
**	This routine executes an object (where object means a frame
**	or a procedure in this context) that is a block of IL.  It
**	only needs the fid for the object, and them some minor information
**	about the object.
**
**	This routine allocates a stack frame for the object, sets
**	up the stack frame, gets the IL for the object from ILRF and
**	then executes the IL for the object by calling the interpreter
**	main loop.
**
** Inputs:
**	fid		The FID of the object to execute.
**
**	kind_of_object	The kind of object this FID represents.  This
**			is just a character pointer to a string that
**			will be used in an error message.
**
**	formname	If the object has a form associated with it, this
**			is the name of that form.  It must be put on
**			the stack frame for the object.
**
**	is_static	indicates whether the frame is static.
**
** Side Effects:
**	Sets the IIOstmt global variable.
**	Tries to handle allocation errors.
**
** History:
**	29-sep-1988 (Joe)
**		Created by taking the common code of IIOrfRunFrame and
**		IIOrpRunProc and making them this routine called with
**		the correct parameters.
**	11/89 (jhw) -- Moved data initialization here from 'IIITinitializeEx()'.
**
**	11/89 (jhw) -- Added session switch for fetching IL (brackets frame get
**		'IIORfgFrmGet()'.)  Bug #8364.
**	01/14/91 (emerson)
**		Call IIARhf2HidFldInitV2 instead of IIARhfiHidFldInit;
**		call it for static frames in addition to non-static frames
**		(but only on the first call). (Bug 34840).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Call IIOFufPushFrame and IIOFpfPopFrame to manipulate
**		frame stack and to get the new frame and restore the old.
**		Added logic to switch session when restoring the old frame.
*/

STATUS	IIARhf2HidFldInitV2();

VOID
IIITeilExecuteIL(fid, kind_of_object, formname, is_static)
FID	*fid;
char	*kind_of_object;
char	*formname;
bool	is_static;
{
EXEC SQL BEGIN DECLARE SECTION;
    i4		saved_id;
EXEC SQL END DECLARE SECTION;

    u_i4	rval;

    /* Get current session ID */
    EXEC SQL INQUIRE_INGRES ( :saved_id = SESSION );

    /* Switch to Interpreter/Front-End Session */
    EXEC SQL SET_INGRES ( SESSION = :UI_SESSION );

    /* Push a new frame stack entry and retrieve the new frame */
    rval = IIOFufPushFrame( fid, formname, is_static, FALSE );

    /* Switch to (saved) User/Application Session */
    IIARrsiRestoreSessID(saved_id);

    if ( rval != OK )
    {
	i4 level = 0;

	IIOerError(0, ILE_CALBAD, kind_of_object,  fid->name,
		      (char *) &rval, (char *) NULL
	);

	if (rval == ILE_CFRAME || rval == ILE_CLIENT)
	{
		/*
		** Can't even recover calling object.  Must exit from interp.
		*/
		level = (ILE_FATAL | ILE_SYSTEM);
	}
	IIOerError(level, (ER_MSGID) rval, (char *) NULL);

	/*
	** NOTREACHED if level was set to FATAL|SYSTEM.  Otherwise,
	** at this point, the frame couldn't be gotten.  The caller
	** is waiting for a return from here.  Once the return happens,
	** the caller will restore itself.  See the code in IIOcfCallfExec
	** to see how caller restores itself using the Return address.
	*/
    }
    else
    {
	IIITsniSetNextIl( IIOFgbGetCodeblock() );

	IIOFssSetStatic( is_static );

	/*
	** Execute the IL statements for the object.
	*/
	IIOilInterpLoop();

	if (!IIOfrmErr && is_static && !IIOFiesIsExecutedStatic())
	{
		/* 
		** Tell ILRF that this frame is static.  From now
		** on ILRF will save it; static is set only once.
		*/
		IIOFsesSetExecutedStatic();
	}

	/* Get current session ID */
	EXEC SQL INQUIRE_INGRES ( :saved_id = SESSION );

	/* Switch to Interpreter/Front-End Session */
	EXEC SQL SET_INGRES ( SESSION = :UI_SESSION );

	/* Pop the frame stack entry and recover previous frame (if any) */
	rval = IIOFpfPopFrame( );

	/* Switch to (saved) User/Application Session */
	IIARrsiRestoreSessID(saved_id);
    }
}

/*{
** Name:	IIITmpMainProc() -	Start a frame or main procedure 
**
** Description:
**	Executes the OSL 'start' and 'main procedure' statements.
**
** IL statement:
**	START
**	MAINPROC VALUE1 VALUE2 VALUE3 VALUE4
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the frame or procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the frame or procedure.
**	VALUE3	The number of VDESC entries for the frame or procedure.
**	VALUE4	The total number of entries in the dbdv array
**		for the frame or procedure and any local procedures it may call.
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
VOID
IIITmpMainProc( stmt )
IL		*stmt;
{
	STATUS	rval;
	i4	num_dbds;
	i4	stacksize;
	i4	fdesc_size;
	i4	vdesc_size;

	/*
	** Push an entry onto control stack.
	*/
	if ( ( *stmt & ILM_MASK ) == IL_START )
	{
		stacksize  = IIOFgfssGetFrameStackSize( );
		fdesc_size = IIOFgffsGetFrameFdescSize( );
		vdesc_size = IIOFgfvsGetFrameVdescSize( );
		num_dbds   = vdesc_size;
	}
	else /* IL_MAINPROC */
	{
		stacksize  = *( (i4 *)IIOgvGetVal( ILgetOperand( stmt, 1 ),
						   DB_INT_TYPE, (i4 *)NULL) );
		fdesc_size = ILgetOperand( stmt, 2 );
		vdesc_size = ILgetOperand( stmt, 3 );
		num_dbds   = ILgetOperand( stmt, 4 );
	}

	rval = IIOFucPushControl
	( 
		IIOFfrFramename( ),
		num_dbds,
		IIOFgsGetStaticData( ),
		stacksize,
		fdesc_size,
		vdesc_size,
		0,
		0,
		0
	);
	if ( rval != OK )
	{
		IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
		return;
	}

	/*
	** Initialize hidden fields if this is a procedure,
	** or if this is a frame, and the frame is not static.
	** Also initialize them if this frame is static and this
	** is the first call to the frame.  (In that case, we initialize
	** complex objects as if they were global).
	*/
	if ( IIOFiesIsExecutedStatic() )
	{
		return;
	}
	rval = IIARhf2HidFldInitV2( IIOFgcGetFdesc( ), IIOFdgDbdvGet( 1 ),
				    (i4)IIOFisIsStatic( ) );
	if ( rval != OK )
	{
		/*
		** Runtime system has already reported the error
		*/
		IIOfrmErr = TRUE;
	}
	return;
}

/*{
** Name:	IIITlpLocalProc() -	Start a local procedure
**
** Description:
**	Executes the OSL 'local procedure' statements.
**
** IL statement:
**	LOCALPROC VALUE1 VALUE2 VALUE3 VALUE4 VALUE5 VALUE6 VALUE7 VALUE8
**
**	The values have the following meanings:
**
**	VALUE1	The ILREF of an integer constant containing the stack size
**		required by the local procedure (not including dbdv's).
**	VALUE2	The number of FDESC entries for the local procedure.
**	VALUE3	The number of VDESC entries for the local procedure.
**	VALUE4	The offset of the FDESC entries for the local procedure
**		(within the composite FDESC for the entire frame or procedure).
**	VALUE5	The offset of the VDESC entries for the local procedure
**		(within the composite VDESC array for the entire frame or proc).
**	VALUE6	The offset of the dbdv's for the local procedure
**		(within the composite dbdv array for the entire frame or proc).
**	VALUE7	A reference to a char constant containing the name of
**		the local procedure.  Not used by the interpreter.
**	VALUE8	The SID of the IL_MAINPROC or IL_LOCALPROC statement
**		of the "parent" main or local procedure (i.e, the procedure
**		in which this procedure was declared).
**		Not used by the interpreter.
** Inputs:
**	stmt	{IL *}  The IL statement to execute.
**
** History:
**	04/07/91 (emerson)
**		Created for local procedures.
*/
VOID
IIITlpLocalProc( stmt )
IL		*stmt;
{
	STATUS	rval;

	/*
	** Push an entry onto control stack.
	*/
	rval = IIOFucPushControl
	( 
		routine_name,
		-1,
		NULL,
		*( (i4 *)IIOgvGetVal( ILgetOperand( stmt, 1 ), 
					DB_INT_TYPE, (i4 *)NULL) ),
		(i4)ILgetOperand( stmt, 2 ),
		(i4)ILgetOperand( stmt, 3 ),
		(i4)ILgetOperand( stmt, 4 ),
		(i4)ILgetOperand( stmt, 5 ),
		(i4)ILgetOperand( stmt, 6 )
	);
	if ( rval != OK )
	{
		IIOerError( ILE_FRAME, ILE_NOMEM, (char *)NULL );
		return;
	}

	/*
	** Initialize hidden fields for local procedure.
	*/
	rval = IIARhf2HidFldInitV2( IIOFgcGetFdesc( ), IIOFdgDbdvGet( 1 ),
				    (i4)FALSE );
	if ( rval != OK )
	{
		/*
		** Runtime system has already reported the error
		*/
		IIOfrmErr = TRUE;
	}
		
	return;
}

# ifdef CDOS
/*
**      Temporarily close the interp file for Concurrent DOS.  C-DOS
**      closes the parent's file handles when the child does an exit.
**      Therefore we must close the file and reopen it after we come
**      back.
*/
VOID
SwapFile()
{
        IIORssSessSwap(&il_irblk);
        QG_shutdown();
}

/*
**      Reopen the interp file.
*/
static VOID
UnSwapFile()
{
        if (QG_restart() || IIORsuSessUnswap(&il_irblk))
        {
                char    bufr[81];
                FEprompt("Couldn't restart 4GL interpreter.  Press ENTER",
                        FALSE,80,bufr);
        }
}
# endif

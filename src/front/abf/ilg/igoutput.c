/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<si.h>
#include	<ex.h>
#include	<tm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ilops.h>
#include	<iltypes.h>
#include	<erit.h>
# include <ooclass.h>

/**
** Name:	igoutput.c -	OSL Interpreted Frame Object Generator
**					Output Module.
** Description:
**	Contains the routine that outputs an interpreted frame object
**	into storage.  Defines:
**
**	IGoutput()	output interpreted frame object.
**
** History:
**	Revision 5.1  86/10/17  16:17:56  wong
**	Initial revision.
**
**	Revision 6.2  89/07  wong
**	Test for IL size before output and report excess.  JupBug #6367.
**
**	Revision 6.3/03/00  89/09  bobm
**	Pass source-code file date to IL write routine so IL dates will be at
**	least up-to-date with the source-code file.  (IL and source-code may
**	be on different systems, which may be out-of-synchronization with each
**	other.)
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Removed declarations for deleted function iiIGgetOffset
**		(for bugs 39581, 41013, and 41014).
**
**	Revision 6.5
**	21-aug-92 (davel)
**		Add call to symbol table dump routine ossymdump() for debugging.
**	06-oct-1993 (mgw)
**		Added <fe.h> include because of new TAGID dependancy of <ug.h>
**		on it.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF	char *osIfilname;

/*{
** Name:	IGoutput() -	Output Interpreted Frame Object.
**
** Description:
**	This routine outputs an interpreted frame object through the calls to
**	internal ILG structure output routines and to the IAOM module.  Memory
**	use of the former is block tagged here and freed after output through
**	IAOM.
**
** Input:
**	fid	{i4}  Interpreted frame object identifier.
**	dfile_p	{FILE *}  Debug file pointer.
**	time	{SYSTIME *}  The earliest time-stamp for the IFO.
**
** Output:
**	framep  {PTR *} pointer to frame pointer.  IAOM sets this to point
**		to the written frame, if framep isn't null.  The frame is
**		an AOMFRAME, unknown outside of IAOM, so it's a PTR here.
**
** History:
**	09/86 (jhw) -- Written.
**	07/89 (jhw) -- Test IL size against maximum before output and report
**		excess.  JupBug #6367.
**	03/90 (jhw) -- Pass in time-stamp.
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		Take out check for too many statements since the number of
**      	statements is now MAXI4.  (Currently, no check is made;
**		one could be added in IGgenStmt).
*/

VOID
IGoutput ( fid, dfile_p, time, framep )
i4	fid;
FILE	*dfile_p;
SYSTIME	*time;
PTR	*framep;
{
	STATUS	status;

	IGgenStmt(IL_EOF, (IGSID *) NULL, 0);

	/* create frame object */
	IIAMwoWrtOpen();
	IGoutStmts(dfile_p);
	IGgenConsts(dfile_p);
	IGgenRefs(dfile_p);
	if (dfile_p != NULL)
	{
		ossymdump(dfile_p);
		SIflush(dfile_p);
	}

	/* write frame object to database */
	if ( fid != 0 && (status = IIAMwcWrtCommit(fid, time, framep)) != OK )
	{
		IIUGerr(status, UG_ERR_ERROR, 0, (PTR)NULL);
		EXsignal(EXFEBUG, 1, ERx("IGoutput"));
		/*NOT REACHED*/
	}
}

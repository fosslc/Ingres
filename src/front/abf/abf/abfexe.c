/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ut.h>
# include	<si.h>
# include	<lo.h>
# include	<er.h>
# include	<uf.h>
# include	<ug.h>
# include	"erab.h"

/**
** Name:	abfexe.c -	ABF Program Execution Module.
**
** Description:
**	Contains the routines that execute ABF program functions.  Defines:
**
**	abexeedit()	edit a file.
**	abexecompile()	compile a file.
**
** History:
**	Revision 6.0  88/01/21  wong
**	Extracted from "abfrt/abrtexe.c".
**
**	3/21/90 (Mike S)
**	Redirect compiler output to a file
**
**	4-feb-1992 (mgw) Bug #38790
**		Report OS level error messages from failed compiles if
**		available. Print UT errors returned from UTcompile() too.
**	22-may-1992 (mgw)
**		Change ERlookup() call to not request a timestamp. It messes
**		up Testing canons and seems to cause ERlookup to report
**		success when it shouldn't on Unix.
**	12-oct-1992 (daver)
**		Included header file <fe.h>; re-ordered headers according 
**		to guidelines put in place once CL was prototyped.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

VOID	IIARutxUTerr();

/*
** OPTIONS
*/
GLOBALDEF char	IIabOptedit[MAX_LOC+1] = {EOS};



STATUS
abexeedit (file)
register LOCATION	*file;
{
	extern char	IIabOptedit[];

	STATUS	rval;

	IIUFrtmRestoreTerminalMode(IIUFNORMAL);
	/*
	** BUG 1892
	** Use IIabOptedit for editor.
	*/
	if ((rval = UTedit(IIabOptedit, file)) != OK)
		IIARutxUTerr(IIabOptedit[0] == EOS ? ERx("editor") : IIabOptedit, rval, TRUE);
	else
		IIUFrtmRestoreTerminalMode(IIUFFORMS);

	return rval;
}

STATUS
abexecompile (sfile, ofile, lfile)
register LOCATION	*sfile,		/* Source file */
			*ofile,		/* Object file */
			*lfile;		/* Listing File */
{
	STATUS	rval;
	bool	pristine;	/* Must screen be refreshed */
	CL_ERR_DESC clerror;

	/* Compile the bloody thing */
	rval = UTcompile(sfile, ofile, lfile, &pristine, &clerror);
	if (rval != OK)
	{
		char *name;
		i4 errstat = rval;

		/*
		** Bug 38790 - print OS error messages if available
		**
		** Use lang 1 which is guaranteed to be ok. The system
		** will return OS errors in a system dependent way
		** regardless of our language.
		*/
		i4	lang = 1;
		i4	msg_len = ER_MAX_LEN;
		char	msg_buf[ER_MAX_LEN];
		CL_ERR_DESC	sys_err;

		msg_buf[0] = EOS;
		if (ERlookup(0, &clerror, 0, NULL, msg_buf, msg_len, lang,
			     &msg_len, &sys_err, 0, NULL) == OK)
		{
			if (msg_buf[0] != EOS)
				IIUGerr(E_AB0155_OSCompErr, UG_ERR_ERROR, 1,
				        msg_buf);
		}
		/* Print UT errors too */
		if (((ER_MSGID) errstat - E_CL_MASK - E_UT_MASK) < 0x100)
			IIUGerr((ER_MSGID) errstat, 0, 0);

		LOtos(sfile, &name);
		IIUGerr(E_AB001E_CompFailure, 0, 2, name, &errstat);
	}

	/* If the screen has been overwritten, restore it */
	if (! pristine)
		IIABrsRestoreScreen();

	/* Output listing file (to tablefield or stdout) */
	if (lfile != NULL)
		IIABdcfDispCompFile(lfile);

	return (rval == OK) ? OK : FAIL;
}

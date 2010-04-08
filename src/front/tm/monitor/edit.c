# include	<compat.h>
# include	<lo.h>
# include	<si.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<ut.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"ermo.h"
# include	"monitor.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**  CALL TEXT EDITOR
**
**	The text editor is called.  The actual call is to $ING_EDIT, then
**	the macro {editor}.  If that fails, the default editor
**	defined in UTedit() is called.
**
**	85/07/12  17:05:00	roger
**		AT&T SIR (RTI bug #4047).  Create edit buffer file in
**		ING_TEMP on "\e" with no arguments.
**	85/08/14	 18:58:01  source
**		llama code freeze 08/14/85
**	85/10/04	 11:51:12  roger
**		ibs integration ("SIprintf" => "putprintf").
**	85/11/01  05:47:44  mikem
**		Initial revision
**	85/11/26  13:02:03  daved
**		print prompt after errors.
**	86/07/23  16:49:55  peterk
**		6.0 baseline from dragon:/40/back/rplus
**	08-oct-86 (tau)
**		Add logic to ignore re-opening of an empty edit file. IBM CMS
**		does not create a file with no data in it. This will cause
**		SYSERR ("Can't reopen ...") when opening a non-existing edit
**		file. (IBM porting change 12/21/89)
**	86/10/14  14:40:34  peterk
**		working version 2.
**		eliminate BE include files and Sccsid string.
**	86/12/29  16:21:34  peterk
**		change over to 6.0 CL headers
**	87/04/08  01:37:45  joe
**		Added compat, dbms and fe.h
**	87/08/12  15:47:50  daver
**		Message Extraction
**	87/08/13  16:51:32  daver
**		put quotes around ermo.h
**	87/10/19  11:45:27  peterk
**		remove syserrs; change calls to ipanic to take MSGID's
**	87/12/07  18:19:49  peterk
**		reinitialize IT line drawing char set selection after return
**		from editor.
**	89/03/10  18:47:50  kathryn
**		Bug 4875	- Use the editor specified by ING_EDIT first.
**		Only if ING_EDIT has not been defined use macro(editor).
**	01-dec-89 (wolf)
**		Moved NMgtAt call out of if stmt; it's VOID! (IBM porting
**		change 12/21/89)
**	06-feb-90 (bruceb)	Fix for bug 1563.
**		Changed from LOdelete to LOpurge to remove all versions
**		of the temporary edit file.
**	06-feb-90 (bruceb)
**		Neatened this history section.
**	07-feb-90 (teresal)
**		Added a file extension (".edt") to the temporary edit
**		filename(QUExxxx) to allow the Language-Sensitive Editor
**		to be compatible with the TM. Otherwise, if no file extension
**		was given, LSE might apply a default file extension and thus
**		edit the wrong file. Bug 20984
**      19-mar-90 (teresal)
**              Removed 'Query buffer full' warning message - no longer needed
**              with dynamically allocated query buffer. (bugs 9489 9037)
**      02-aug-91 (kathryn) Bug 38893
**              Reset line drawing characters using saved string
**              IIMOildInitLnDraw, rather than calling ITldopen.
**      07-Apr-93 (mrelac)
**		As per the 6.4 CL spec, this change replaces the call to 
**		NMt_open with calls to NMloc and SIfopen.  This change fixes
**		bug 50495 on MPE/iX but the fix is appropriate to all
**		platforms.  The temporary edit file created by this routine
**		needs to be opened in SI_TXT mode, but NMt_open lacks such a
**		mode parameter.  Also changed SIopen()s to SIfopen()s.
**		Changed i4s to STATUS, i4  or i4 where appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

FUNC_EXTERN i4 q_getc();

edit()
{
	register char		*p;
	LOCATION		t_loc,
				loc;
	FILE			*fp;
	char			errbuf[ER_MAX_LEN + 1];
	char			input[MAX_LOC + 1];
	char			*editfile;
	char			*ingedit;
	i4			rflag;
	STATUS			status;
	i4			c;
#ifdef CMS
	i4			Notempty = 1;
#endif /* CMS */

	GLOBALREF	char	*IIMOildInitLnDraw;

	FUNC_EXTERN 	char		*getfilenm();
	FUNC_EXTERN 	char		*macro();


	if (!Newline)
	{
		q_putc(Qryiop, '\n');
		Newline = TRUE;
	}

	Autoclear	= 0;
	editfile	= getfilenm();
	rflag		= 0;

	if (*editfile == '\0')
	{
		rflag	 = 1;
		input[0] = EOS;


		if ((status = NMloc( TEMP, PATH, (char *)NULL, &t_loc)) ||
		    (status = LOuniq( ERx( "query" ), ERx( "edt" ), &t_loc)))
		{
			STcopy( ERx( "queryXXX" ), input );
		}
		else
		{
			LOcopy( &t_loc, input, &loc );
		}

		status = SIfopen( &loc, ERx( "w" ), (i4)SI_TXT,
				  (i4)SI_MAX_TXT_REC, &fp );
	}
	else
	{
		/* use the path and file name the user gave you */

		STcopy(editfile, input);

		if (!(status = LOfroms(PATH & FILENAME, input, &loc)))
			status = SIfopen( &loc, ERx("w"), (i4)SI_TXT,
					  (i4)SI_MAX_TXT_REC, &fp );
	}

	if (status)
	{
		if (status == FAIL)
			errbuf[0] = '\0';
		else
			ERreport(status, errbuf);

		putprintf(ERget(F_MO000C_Cant_create_qry_file),
				input, errbuf);

		cgprompt();
		return(0);
	}

	if (q_ropen(Qryiop, 'r') == (struct qbuf *)NULL)
		/* edit: q_ropen 1 */
		ipanic(E_MO0044_1500400);

	while ((c = (i4)q_getc(Qryiop)) > 0)
		SIputc((char)c, fp);

	SIclose(fp);

	if (Nodayfile >= 0)
	{
		putprintf(ERget(F_MO000D_editor_prompt));
		SIflush(stdout);
	}

	/*
	**	macro returns NULL if undefined, UTedit uses
	**		default editor if passed NULL.
	*/

	/* Bug 4875	-	Use editor defined by environment variable
		ING_EDIT. If that is not set then use the macro(editor). 
	*/

	NMgtAt((ERx("ING_EDIT")), &ingedit);
	if ( ingedit != NULL && *ingedit != EOS )
		p = ingedit;
	else

		p = macro(ERx("{editor}"));

	if (status = UTedit(p, &loc))
	{
		ERreport(status, errbuf);
		putprintf(ERget(F_MO000E_Can_t_start_up_editor), errbuf);

		cgprompt();
		return(0);
	}

	if (!rflag)
	{
		if (q_ropen(Qryiop, 'a') == (struct qbuf *)NULL)
			/* edit: q_ropen 2 */
			ipanic(E_MO0045_1500401);
	}
	else
	{
		if (q_ropen(Qryiop, 'w') == (struct qbuf *)NULL)
			/* edit: q_ropen 3 */
			ipanic(E_MO0046_1500402);

		if (status = SIfopen( &loc, ERx("r"), (i4)SI_TXT,
				      (i4)SI_MAX_TXT_REC, &fp ))
#ifdef CMS
		{
			Notempty = 0;
		}
#else
		{
			ERreport(status, errbuf);
			/* can't reopen editfile %s: %s\n */
			ipanic(E_MO0047_1500403,
				editfile, errbuf);
		}
#endif /* CMS */

#ifdef CMS
		if (Notempty)
		{
#endif /* CMS */
		Notnull = 0;

		while ((c = SIgetc(fp)) != EOF)
		{
			if (status)
			{
				ERreport(status, errbuf);
				/* Error reading edit file: %s\n */
				ipanic(E_MO0048_1500404, errbuf);
			}

			Notnull = 1;

			q_putc(Qryiop, (char)c);
		}

		SIclose(fp);


		if (status = LOpurge(&loc, 0))
		{
			ERreport(status, errbuf);
			putprintf(ERget(F_MO000F_Cant_delete_file), editfile, errbuf);
		}
#ifdef CMS
		} /* endif Notempty */
#endif /* CMS */
	}

#ifndef FT3270
	/* Re-Initialize IT line drawing */
	{
	    if (Outisterm && IIMOildInitLnDraw != NULL)
		SIprintf(ERx("%s"), IIMOildInitLnDraw);
	}
#endif

	cgprompt();

	return(0);
}

/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<ut.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>
# include	<si.h>
# include	<nm.h>
# include	<termdr.h>
# include	<er.h>
# include	<erft.h>

/**
** Name:	ftprnscr.c	- print current screen to file.
**
** Description:
**	This file defines:
**
**	FTprnscr	print screen to file.
**	FTprvfwin	Special entry for VIFRED/RBF to use a different window.
**
** History:
**	03/07/87 (dkh) - Jupiter changes.
**	08/14/87 (dkh) - ER changes.
**	28-mar-88 (bruceb)
**		Refresh the screen if running on DG systems.
**	14-jun-88 (bruceb)
**		Refresh the screen (if DG) earlier, immediately
**		after the LOfroms but before ANY returns can occur.
**	07/23/88 (dkh) - Modifed to handle popups.
**	11/11/88 (dkh) - Fixed calls to FTmessage.
**	02/01/89 (dkh) - Fixed venus bug 4583.
**	26-may-89 (bruceb)
**		Use LOexist instead of SIopen(read mode) to determine
**		if the print-file will be written or appended-to.
**		Moved the ifdef DG TDrefresh code until after the SIopen.
**	09-aug-89 (sylviap)
**		Added NMgtAt (ING_PRINT) and extra parameters for UTprint.
**	09/21/89 (dkh) - Porting changes integration.
**	06-oct-89 (kenl)
**		Broke apart if statement containing SIopen() so on DG we
**		can immediately refresh the screen.
**	19-Mar-90 (mrelac) (hp9_mpe only)
**		MPE/XL-specific change: change LOfroms() call first parameter
**		from FILENAME to PATH & FILENAME, as the filename comes in
**		fully-qualified.
**	26-apr-1991 (mgw) Bug 36841
**		Don't send full path of file name to UTprint for the title
**		arg (header page name on VMS, job name on Unix). Just send
**		the file name. >39 chars causes UTprint failure on VMS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	Some definitions for this routine.
*/

# define	NAM_LN_PRN	7

GLOBALREF	WINDOW	*FTwin;		/* current screen */

FUNC_EXTERN	WINDOW	*IIFTpbPrscrBld();
FUNC_EXTERN	VOID	IIFTpcPrscrCleanup();

static	bool	ftprvf = FALSE;
static	WINDOW	*vfwin = NULL;

/*{
**  Name:	FTprvfwin	- set up to print special VIFRED window.
**
**  Description:
**	Sets up ftprvf and vfwin so that when FTprscr is called
**	it will use vfwin instead of FTwin.
**
**  Input:
**	WINDOW to use.
**
**  History:
**	11/15/85 - Written. (dkh)
*/

FTprvfwin(win)
WINDOW	*win;
{
	vfwin = win;
	ftprvf = TRUE;
}


/*{
** Name:	FTprnscr	- print current screen to file.
**
** Description:
**	Write out the current screen to a file, with the
**	current data included.	This will write out to a
**	file specified, if the 'filename' parameter is
**	given.	The file name can be 'printer,' in which
**	case the file is printed and deleted.
**
**	If no (or a NULL) 'filename' is given, and if the
**	INGRES name 'II_PRINTSCREEN_FILE' is set, that file
**	name will be used.  If the name is not defined,
**	then the user will be prompted for a file name.
**
**	In any case, the current screen is appended to the
**	named file, if it exists, or created if it does not.
**
** Inputs:
**	filename	- valid file name, or 'printer' or NULL.
**
** Outputs:
**	Returns:
**		VOID.
**	Exceptions:
**		none.
**
** Side Effects:
**	Resets ftprvf if it is set.
**
** History:
**	07-aug-85 (peter)	written.
**	03/07/87 (dkh) - Jupiter changes.
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
VOID
FTprnscr(filename)
char	*filename;
{
	LOCATION	loc;		/* temporary location */
	LOCATION	tloc;
	FILE		*fp;
	char		*mode;
	WINDOW		*usewin;	/* window to print */
	char		filbuf[MAX_LOC + 1];	/* File name buffer */
	char		*nameptr;	/* name pointer */
	bool		is_print;	/* TRUE if printer */
	STATUS		retstat;		/* open file return */
	char		*printer;
	char		*nm_prmpt;
	char		msgbuf[512];

	printer = ERget(F_FT000E_printer);
	nm_prmpt = ERget(S_FT000B_Enter_file_name_);

	if ((filename == NULL) || (*filename =='\0'))
	{	/* No file name specified */
		NMgtAt(ERx("II_PRINTSCREEN_FILE"), &nameptr);
		if ((nameptr != NULL) && (*nameptr != '\0'))
		{	/* Name Specified */
			STlcopy(nameptr, filbuf, sizeof(filbuf)-1);
		}
		else
		{	/* No name yet.	 Prompt for one. */
			FTprmthdlr(nm_prmpt, (u_char *) filbuf,
				(i4) TRUE, NULL);
			if (*filbuf == '\0')
			{	/* Return on no response */
				return;
			}
		}
	}
	else
	{
		STlcopy(filename, filbuf, sizeof(filbuf)-1);
	}

	/* Got the file name.  Now see if it is a printer. */

	if (STbcompare(filbuf, 0, printer, 0, TRUE) == 0)
	{	/* Printer.  Use temp file for now */
		is_print = TRUE;
		NMloc(TEMP, PATH, NULL, &tloc);
		LOuniq(ERx("prsc"), ERx("lis"), &tloc);
		LOcopy(&tloc, filbuf, &loc);
		nameptr = printer;	/* point to file name */
	}
	else
	{	/* Not a printer */
		is_print = FALSE;
# ifdef hp9_mpe
		LOfroms(PATH & FILENAME, filbuf, &loc);
# else
		LOfroms(FILENAME, filbuf, &loc);
# endif
		nameptr = filbuf;
	}

	/* Check to see if file exists for message. */

	if (LOexist(&loc) != OK)
	{	/* File does NOT exist */
		mode = ERx("w");
	}
	else
	{	/* File DOES exist. */
		mode = ERx("a");
	}

# ifdef hp9_mpe
	if ((retstat = SIfopen(&loc, mode, SI_TXT, 256, &fp)) != OK)
# else
	retstat = SIopen(&loc, mode, &fp);

# ifdef	DGC_AOS
		TDrefresh(curscr);
# endif

	if (retstat != OK)
# endif
	{	/* Could not open file */
		STprintf(msgbuf, ERget(S_FT000C_Error_opening_file___), filbuf);
		FTmessage(msgbuf, FALSE, TRUE);
		return;
	}

	if (*mode == 'w')
	{	/* New file */
		STprintf(msgbuf, ERget(S_FT000D_Writing_screen_to_fil),nameptr);
		FTmessage(msgbuf, FALSE, FALSE);
	}
	else
	{	/* Old file */
		STprintf(msgbuf, ERget(S_FT000E_Appending_screen_to_f),nameptr);
		FTmessage(msgbuf, FALSE, FALSE);
	}
	PCsleep((i4)2000);

	if (ftprvf)
	{
		usewin = vfwin;
	}
	else
	{
		usewin = IIFTpbPrscrBld();
	}
	ftprvf = FALSE;
	vfwin = NULL;

	if (usewin == NULL)
	{	/* No current window. */
		FTmessage(ERget(E_FT000F_Error___Current_scree), FALSE, TRUE);
		return;
	}

	TDprnwin(usewin, fp);

	if (ftprvf)
	{
		IIFTpcPrscrCleanup();
	}

	if ((retstat = SIclose(fp)) != OK)
	{
		STprintf(msgbuf, ERget(S_FT0010_Error_closing_file___),filbuf);
		FTmessage(msgbuf, FALSE, TRUE);
		return;
	}

	/* If printer, send to printer and quit. */

	if (is_print == TRUE)
	{
                char   *u_printer;
		char   user_print[MAX_LOC];      /* stores printer command */
		char   dummy[MAX_LOC+1];	 /* These 3 used to get */
		char   ftpref[MAX_LOC+1];	 /* just file name for */
		char   ftsuff[MAX_LOC+1];	 /* header page of print job */

		NMgtAt(ERx("ING_PRINT"), &u_printer);
		if (u_printer != (char *)NULL && *u_printer != '\0')
		{
			STcopy(u_printer, user_print);
		}
		else
		{   
			*user_print = EOS;
		}

		/* Get just the file name for the header page of print job */
		LOdetail(&loc, dummy, dummy, ftpref, ftsuff, dummy);
		STprintf(dummy, "%s.%s", ftpref, ftsuff);

		retstat = UTprint(user_print, &loc, TRUE, 1, dummy, NULL );

		if (retstat != OK)
		{	/* Could not print file */
			FTmessage(ERget(S_FT0011_Error_attempting_to_p),
				FALSE, TRUE);
			return;
		}
	}

	return;
}


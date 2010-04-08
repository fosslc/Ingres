/*
**  keys30.c
**
**  Function key routines to support 3.0 functionality.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/09/85 (dkh)
**	87/04/08  joe - Added compat, dbms and fe.h
**	86/11/18  dave - Initial60 edit of files
**	86/01/03  dave - Fix for BUG 7353. (dkh)
**	85/11/04  dave - Changed mapping file from termcap to
**			 use DEF_BY_TCAP level. (dkh)
**	85/09/18  john - Changed i4=>nat, CVla=>CVna, CVal=>CVan,
**			 unsigned char=>u_char
**	85/09/07  dave - Fixed code to work correctly. (dkh)
**	85/08/09  dave - Last set of changes for function key support. (dkh)
**	85/07/19  dave - Initial revision
**
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changed error numbers from 800 series to local ones.
**	24-sep-87 (bruceb)
**		Moved KEYDEFS initialization from FK30init_keys to FKinit_keys
**		so as to allow 3.0 style macros in an MF mapping file.
**	10/22/87 (dkh) - Allow for VMS or UNIX specific mapping files.
**	01/21/88 (dkh) - Changed ifdefs for map file extensions.
**	05/05/88 (dkh) - Fixed 5.0 bug 13731.
**	07/27/88 (dkh) - Changed to use .unx instead of .unix
**	10/23/88 (dkh) - Performance changes.
**	01/12/90 (dkh) - Changed FKputerrmsg to wait for return.
**	01/24/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	22-mar-90 (bruceb)
**		Added locator support.  Clicking anywhere on the message line
**		terminates the waiting.  Also, since FKputerrmsg now waits,
**		added a return on timeout.
**	09/05/90 (dkh) - Integrated round 3 of MACWS changes.
**	10/14/90 (dkh) - Integrated round 4 of macws changes.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT;
**		Don't add ".vms" to file names just cuz it ain't UNXI,
**		because the world DOES consist of more than just VMS and UNIX.
**	10/25/92 (dkh) - Turn off mapping of menu key to ESC if function
**			 keys are part of the termcap description.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC to PMFE section to prevent having and extension
**              added to the map file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Avoid buffer overruns.
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/


# include	<compat.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<tdkeys.h>
# include	<mapping.h>
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	"ftframe.h"
# include	"ftfuncs.h"
# include	<frsctblk.h>
# include	<er.h>
# include	<erft.h>
# include	<ug.h>

# define	DEF_MAPPING		ERx("frs.map")

GLOBALREF	i4	parsing;
GLOBALREF	i4	keyerrcnt;
GLOBALREF	bool	no_errfile;
GLOBALREF	FILE	*erfile;
GLOBALREF	char	FKerfnm[];
GLOBALREF       i4      FKEYS;

FUNC_EXTERN	VOID	FK30init_keys();
FUNC_EXTERN	bool	IIFTtoTimeOut();
FUNC_EXTERN	VOID	IIFTwksWviewKeystrokeSetup();

FKputerrmsg(errnum)
i4	errnum;
{
	KEYSTRUCT	*ks;
	char		errbuf[ER_MAX_LEN + 1];
	IICOORD		done;
	FRS_EVCB	dummy;

	FTbell();
	STlcopy(ERget(errnum), errbuf, sizeof(errbuf)-1);
# ifdef DATAVIEW
	_VOID_ IIMWfmFlashMsg(errbuf, FALSE);
# endif /* DATAVIEW */
	TDwinmsg(errbuf);

	done.begy = done.endy = stdmsg->_begy;
	done.begx = stdmsg->_begx;
	done.endx = stdmsg->_begx + stdmsg->_maxx - 1;
	IITDpciPrtCoordInfo(ERx("FTputerrmsg"), ERx("done"), &done);

	for (;;)
	{
		ks = FTgetch();
		if (IIFTtoTimeOut(&dummy)
		    || ks->ks_ch[0] == '\r' || ks->ks_ch[0] == '\n')
		{
			break;
		}
		if (ks->ks_fk == KS_LOCATOR)
		{
		    if (ks->ks_p1 == done.begy)
		    {
			IITDposPrtObjSelected(ERx("done"));
			break;
		    }
		    else
		    {
			FTbell();
		    }
		}
	}
}



FILE *
FKopenkfile(fn)
char	*fn;
{
	FILE		*fptr;
	LOCATION	keydefloc;
	char		buf[MAX_LOC + 1];

	STlcopy(fn, buf, sizeof(buf)-1);
	LOfroms(PATH & FILENAME, buf, &keydefloc);
	if (SIopen(&keydefloc, ERx("r"), &fptr) != OK)
	{
		fptr = NULL;
	}
	return(fptr);
}


FKinit_keys()
{
	reg i4		i;
	FILE		*fptr;
	char		mfile[200];
	i4		mffound = TRUE;
	LOCATION	defmaploc;


	/*
	**  If function keys are on, turn off menu mapping for the
	**  ESCAPE key.  WARNING: This code assumes that the mapping
	**  of the ESCAPE key to the menu command is before the
	**  the maping of PF1 to the menu command.  If this assumption
	**  changes, this code MUST change.
	*/
	if (KY)
	{
		FKumapcmd(fdopMENU, DEF_INIT);
	}

	for (i = 0; i < FKEYS; i++)
	{
		KEYDEFS[i] = (u_char *) KEYNULLDEF;
	}

	/*
	**  Try to find default stuff from FILES directory.
	*/

	NMloc(FILES, FILENAME, DEF_MAPPING, &defmaploc);
	if (SIopen(&defmaploc, ERx("r"), &fptr) != OK)
	{
		/*
		**  Could not open default mapping file.
		*/

		FKputerrmsg(E_FT002E_NODEFMAP);
	}
	else
	{
		FK40parse(fptr, (i4) DEF_BY_SYSTEM);
		SIclose(fptr);
	}

	if (MF)
	{
		STlcopy(MF, mfile, sizeof(mfile)-1);
# if !defined(PMFE) && !defined(NT_GENERIC)
# ifdef  UNIX
		STcat(mfile, ERx(".unx"));
# else
		STcat(mfile, ERx(".vms"));
# endif /* UNIX */
# endif /* PMFE */
# ifdef DATAVIEW
		IIMWkfnKeyFileName(mfile);
# endif	/* DATAVIEW */
		NMloc(FILES, FILENAME, mfile, &defmaploc);
		if (SIopen(&defmaploc, ERx("r"), &fptr) != OK)
		{
# ifdef DATAVIEW
			IIMWkfnKeyFileName(MF);
# endif	/* DATAVIEW */
			NMloc(FILES, FILENAME, MF, &defmaploc);
			if (SIopen(&defmaploc, ERx("r"), &fptr) != OK)
			{
				mffound = FALSE;
				FKputerrmsg(E_FT001E_CANTOPEN);
			}
		}
		if (mffound)
		{
			FK40parse(fptr, (i4) DEF_BY_TCAP);
			SIclose(fptr);
		}
	}
	FK30init_keys();

	/*
	**  Setup special keystroke information for wview.
	*/
	IIFTwksWviewKeystrokeSetup();
}




VOID
FK30init_keys()
{
	FILE		*fptr;
	char		*fn;
	char		errbuf[ER_MAX_LEN + 1];

	NMgtAt(ERx("INGRES_KEYS"), &fn);
	if (fn == NULL || *fn == '\0')
	{
		;
	}
	else
	{
		if ((fptr = FKopenkfile(fn)) == NULL)
		{
			return;
		}
		parsing = AFILE;
		FK40parse(fptr, (i4) DEF_BY_USER);
		parsing = INTERACTIVE;
		if (keyerrcnt != 0)
		{
			/*
			**  Fix for BUG 7353. (dkh)
			*/
			IIUGfmt(errbuf, ER_MAX_LEN, ERget(E_FT001F_ERRSFND),
				2, FKerfnm, ERget(FE_HitEnter));
			FTmessage(errbuf, TRUE, TRUE);
			if (!no_errfile)
			{
				SIclose(erfile);
			}
		}
		SIclose(fptr);
	}
}



char *
FKgetline(cp, n, fp)
u_char	*cp;
i4		n;
register FILE	*fp;
{
	reg	char	*np;
	reg	STATUS	stat;

	np = (char *) cp;
	stat = SIgetrec(np, n, fp);
	switch(stat)
	{
		case OK:
			cp[STlength((char *)cp) - 1] = '\0';
			return((char *) cp);

		default:
		case FAIL:
			return(NULL);
	}
}

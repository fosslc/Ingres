/*
**  keys.c - Routines to handle cursor and function keys.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	written 12-8-82 dkh
**	10/24/83 -Added explicit return(1) for FK2parse	 (nml).
**	03/26/86 - (scl) EBCDIC support for VT/ASCII.
**	10/03/86 (a.dea) -- #ifdef KFE around KFcountout() call
**		 so that QA test in CMS doesn't pick this up.
**	24-oct-86 (bab) Fix for bug 10175
**		Remove inappropriate menuline labels when
**		a 3.0 macro overrides a 4.0 function key.
**		(Done here, in termdr, since this code will
**		be extensively modified for next release;
**		otherwise, keymap and menumap should only
**		be known about at the FT level.)
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	01/27/87 (KY)  -- Changed return value to KEYSTRUCT structure
**			    for handling Double Bytes characters.
**	19-jun-87 (bab) Code cleanup.
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	15-sep-87 (bab)
**		FKgetinput changed to return the control character rather
**		then the function key designator for cursor keys that are
**		equivalent to single control characters.  E.g. on DG,
**		^W is the up-arrow.  This avoids both multiple mappings
**		to the same cursor motion command, and the inability to
**		move in specific directions.
**	09/29/87 (dkh) - Integrated fix for 3.0 macros broken by Kanji.
**	11/11/87 (dkh) - Code cleanup.
**	06/18/88 (dkh) - Integrated MVS changes.
**	10/22/88 (dkh) - Performance changes.
**	9-oct-89 (mgw) - Removed definition for BELL in the non-EBCDIC case
**			 since it's already defined properly in <cm.h>.
**	12/01/89 (dkh) - Added support for WindowView GOTOs.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01/22/90 (dkh) - Fixed VMS compilation problems.
**	01/23/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	02/02/90 (dkh) - Fixed to correctly parse version number from
**			 wview.
**	03/01/90 (Eduardo) - Added capability of recognizing EOF when
**			     reading input through FKgetinput (used when
**			     dealing with terminals that support function
**			     keys)
**	09-mar-90 (bruceb)
**		Added support for locator sequence.  Current support expects
**		the sequence to begin with an ESC.
**	08/15/90 (dkh) - Fixed bug 21670.
**	08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**	09/04/90 (dkh) - Removed KFE stuff which has been superseded by SEP.
**	10-jan-1991 (mgw)
**		Remove VMS specific FUNC_EXTERN for TEget() - not called here.
**		This was causing build problems on VMS.
**	29-nov-91 (seg)
**	    declaration here of SIgetrec and MEclose should be in the CL hdr files.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
**	24-Aug-2009 (kschendel) 121804
**	    Need ug.h to satisfy gcc 4.3.
*/


# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<kst.h>
# include	<ftdefs.h>
# include	<tdkeys.h>
# include	<si.h>
# include	<st.h>
# include	<lo.h>
# include	<cm.h>
# include	<te.h>
# include	<mapping.h>
# include	<me.h>
# include	<er.h>
# include	<ex.h>
# include	"ertd.h"
# include	<termdr.h>
# include	<ug.h>
# include	<windex.h>

#ifndef CMS
# define	CAST_PUNSCHAR	(u_char *)
#else /* CMS */
# define	CAST_PUNSCHAR
#endif /* CMS */

GLOBALREF u_char	*KEYSEQ[KEYTBLSZ + 4];

GLOBALREF u_char	*KEYNULLDEF;

GLOBALREF i4	MAXFKEYS;
GLOBALREF i4	FKEYS;
GLOBALREF i4	MENUCHAR;
GLOBALREF char	FKerfnm[20];



# define	CAPCNTRL	0100
# define	LOWCNTRL	0140

# ifndef EBCDIC
# define	ESCAPE		'\033'
# define	DELETE		'\177'		/* BUG 5353 (dkh) */
# else
# undef BELL
# define	BELL		'\057'
# define	ESCAPE		'\047'
# define	DELETE		'\007'
# endif /* EBCDIC */


GLOBALREF u_char	*KEYDEFS[KEYTBLSZ];


GLOBALREF char	KEYUSE[KEYTBLSZ]; 
GLOBALREF char	INBUF[KBUFSIZE];
GLOBALREF char	*MACBUF;
GLOBALREF char	*MACBUFEND;
GLOBALREF u_char *mp;
GLOBALREF i4	kbufp;
GLOBALREF KEYSTRUCT	*(*in_func)();
GLOBALREF i4	F3INUSE;
GLOBALREF i4	parsing;
GLOBALREF i4	keyerrcnt;
GLOBALREF FILE	*erfile;
GLOBALREF i4	beingdef;
GLOBALREF bool	infde_getch;
GLOBALREF bool	no_errfile;

static	char	*online = NULL;
static	char	*linewas = NULL;

GLOBALREF	bool	macexp[];
GLOBALREF	i4	IITDwvWVVersion;
GLOBALREF	bool	Rcv_disabled;
GLOBALREF	bool	fdrecover;
GLOBALREF	bool	IITDlsLocSupport;

FUNC_EXTERN	KEYSTRUCT	*ITin();
FUNC_EXTERN	KEYSTRUCT	*IITDplsParseLocString();

KEYSTRUCT   *
FKgetc(fp)
FILE	*fp;
{
	KEYSTRUCT   *ks;

	if (fp == stdin)
	{
		return(ITin());
	}
	else
	{
		ks = TDgetKS();
		if (kbufp > 0)
		{
			TDsetKS(ks, INBUF[--kbufp], 1);
			if (CMdbl1st(ks->ks_ch))
			{
				/*
				**  We are definitely assuming that
				**  both bytes of a double byte char
				**  were pushed back.
				*/
				TDsetKS(ks, INBUF[--kbufp], 2);
			}
		}
		else
		{
			TDsetKS(ks, SIgetc(fp), 1);
			if (CMdbl1st(ks->ks_ch))
			{
				TDsetKS(ks, SIgetc(fp), 2);
			}
		}
		return(ks);
	}
}



VOID
FKmacmsg(sp)
char	*sp;
{
	reg	i4	oly;
		i4	olx;
	extern	WINDOW	*_win;
	char	msgline[MAX_TERM_SIZE + 1];

	oly = curscr->_cury;
	olx = curscr->_curx;
	TDsaveLast(msgline);

	TEput(BELL);

	TDwinmsg(sp);
	TDwinmsg(msgline);
	TDrstcur(curscr->_cury + curscr->_begy, curscr->_curx + curscr->_begx,
		oly + curscr->_begy, olx + curscr->_begx);
	curscr->_cury = oly;
	curscr->_curx = olx;
}

VOID
FKerrmsg(sp)
char	*sp;
{
	LOCATION	erfileloc;
	char		errbuf[ER_MAX_LEN + 1];

	if (parsing == AFILE)
	{
		if (keyerrcnt == 0)
		{
			STcopy(FKerfnm, errbuf);
			LOfroms(FILENAME, errbuf, &erfileloc);
			if (SIopen(&erfileloc, ERx("w"), &erfile) != OK)
			{
				TEput(BELL);
				TDwinmsg(ERget(E_TD0007_OPEN_ERR));
				PCsleep((u_i4)2000);
				parsing = INTERACTIVE;
				TDwinmsg(sp);
				PCsleep((u_i4)2000);
				no_errfile = TRUE;
				return;
			}
		}
		keyerrcnt++;
		if (!no_errfile)
		{
			SIfprintf(erfile, ERx("%s\n"), sp);
		}
	}
	else
	{
		TEput(BELL);
		TDwinmsg(sp);
		PCsleep((u_i4)2000);
	}
}

i4
FK2parse(coded, decoded)
u_char	*coded;
u_char	*decoded;
{
	char	keynum[3];
	i4	key;
	u_char	*cp;
	u_char	*dp;
	char	errbuf[ER_MAX_LEN + 1];
	i4	akey;

	cp = CAST_PUNSCHAR coded;
	dp = CAST_PUNSCHAR decoded;
	for (; *cp != '\0'; cp++)
	{
		if (*cp == '^')
		{
			if (*++cp != '\0')
			{
				if (*cp == '[')
				{
					*dp++ = ESCAPE;
					break;
				}

				/*
				**  Next statement for BUG 5353 (dkh)
				*/
				if (*cp == '?')
				{
					*dp++ = DELETE;
					break;
				}

				if (*cp < 0177 && CMalpha(cp))
				{
					if (CMupper(cp))
					{
						*dp++ = *cp - CAPCNTRL;
					}
					else
					{
						*dp++ = *cp - LOWCNTRL;
					}
				}
				else
				{
					STcopy(ERget(E_TD0008_BADHAT), errbuf);
					FKerrmsg(errbuf);
					return(0);
				}
			}
			else
			{
				STcopy(ERget(E_TD001B_AFHAT), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
		}
		else if (*cp == '\\')
		{
			if (*++cp != '\0')
			{
				reg	char	nxtch;
				reg	char	dstch;

				nxtch = *cp;

				if (nxtch == '\\')
				{
					dstch = '\\';
				}
				else if (nxtch == '^')
				{
					dstch = '^';
				}
				else if (nxtch == 'r')
				{
					dstch = '\r';
				}
				else if (nxtch == 'n')
				{
					dstch = '\n';
				}
				else if (nxtch == 't')
				{
					dstch = '\t';
				}
				else if (nxtch == '#')
				{
					dstch = '#';
				}
				else if (nxtch == 'e' || nxtch == 'E')
				{
					dstch = ESCAPE;
				}
				else
				{
					if (nxtch < 0177)
					{
						dstch = nxtch;
					}
					else
					{
						STcopy(ERget(E_TD0009_BADBSQ),
							errbuf);
						FKerrmsg(errbuf);
						return(0);
					}
				}
				*dp++ = dstch;
			}
			else
			{
				STcopy(ERget(E_TD000A_AFBSQ), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
		}
		else if (*cp == '#')
		{
			if (*++cp == '\0')
			{
				STcopy(ERget(E_TD000B_NONUM), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
			if (!CMdigit(cp))
			{
				STcopy(ERget(E_TD000C_BADNUM), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
			if (*++cp == '\0' || !CMdigit(cp))
			{
				keynum[0] = *(cp-1);
				keynum[1] = '\0';
				cp--;
			}
			else
			{
				keynum[0] = *(cp-1);
				keynum[1] = *cp;
				keynum[2] = '\0';
			}
			if (CVan(keynum, &key) != 0)
			{
				STcopy(ERget(E_TD000D_NUMCONV), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
			key--;
			if (key < 0 || key >= KEYNUMS)
			{
				akey = key + 1;
				IIUGfmt(errbuf, ER_MAX_LEN,
					ERget(E_TD000E_NOKEY), 1, &akey);
				FKerrmsg(errbuf);
				return(0);
			}
			if (key == DEFKEY_1)
			{
				akey = DEFKEY;
				IIUGfmt(errbuf, ER_MAX_LEN,
					ERget(E_TD000F_NOALW), 1, &akey);
				FKerrmsg(errbuf);
				return(0);
			}
			if (key == beingdef)
			{
				STcopy(ERget(E_TD0010_KEYSELF), errbuf);
				FKerrmsg(errbuf);
				return(0);
			}
			*dp++ = FKEY_LEADIN;
			*dp++ = (char) (key + 1);
		}
		else
		{
			*dp++ = *cp;
		}
	}
	*dp = '\0';
	return(1);	/* Put in explicit return(1) code (10/24/83) -nml*/
}



VOID
FKprserr(msg, macline, line)
char	*msg;
i4	macline;
u_char	*line;
{
	char	msgline[150];

	if (online == NULL)
	{
		online = ERget(F_TD0001_Error_occured_in_line);
	}
	if (linewas == NULL)
	{
		linewas = ERget(F_TD0002_Line_was);
	}
	FKerrmsg(msg);
	STprintf(msgline, online, macline);
	FKerrmsg(msgline);
	if (*line != '\0')
	{
		STprintf(msgline, linewas, line);
		FKerrmsg(msgline);
	}
}




VOID
FK30parse(buffer, macline)
u_char	*buffer;
i4		macline;
{
	char	keynum[3];
	i4	key;
	u_char	outbuf[KBUFSIZE];
	u_char	*cp;
	char	msg[50];
	char	msgline[150];
	char	errbuf[ER_MAX_LEN + 1];
	i4	key1;
	i4	key2;

	cp = buffer;
	if (*cp == 0)
	{
		return;
	}
	if (*cp == '!')
	{
		return;
	}
	if (!KY)
	{
		STcopy(ERget(E_TD0011_NOPFK), errbuf);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (*cp != '#')
	{
		STcopy(ERget(E_TD0012_FIRSTCH), errbuf);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (!CMdigit(++cp))
	{
		STcopy(ERget(E_TD0013_DIGITS), errbuf);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (*++cp == '\0' || !CMdigit(cp))
	{
		keynum[0] = *(cp-1);
		keynum[1] = '\0';
		cp--;
	}
	else
	{
		keynum[0] = *(cp-1);
		keynum[1] = *cp;
		keynum[2] = '\0';
	}
	if (*++cp != '=')
	{
		STcopy(ERget(E_TD0014_EQUAL), errbuf);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (CVan(keynum, &key) != 0)
	{
		STcopy(ERget(E_TD000D_NUMCONV), errbuf);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (key == MENUKEY || key == DEFKEY)
	{
		key1 = MENUKEY;
		key2 = DEFKEY;
		IIUGfmt(errbuf, ER_MAX_LEN, ERget(E_TD0015_KEYRES), 2,
			&key1, &key2);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	key--;
	if (key < 0 || key >= KEYNUMS)
	{
		key1 = key + 1;
		IIUGfmt(errbuf, ER_MAX_LEN, ERget(E_TD000E_NOKEY),1, &key1);
		FKprserr(errbuf, macline, buffer);
		return;
	}
	if (*++cp == '\0')
	{
		if (*(KEYDEFS[key]) != '\0')
		{
			MEfree((PTR)(KEYDEFS[key]));
			KEYDEFS[key] = CAST_PUNSCHAR KEYNULLDEF;
		}

		/*
		**  Fix for BUG 9449. (dkh)
		*/

		TDonmacro(key);
		return;
	}
	beingdef = key;
	if (!FK2parse(cp, outbuf))
	{
		if (online == NULL)
		{
			online = ERget(F_TD0001_Error_occured_in_line);
		}
		if (linewas == NULL)
		{
			linewas = ERget(F_TD0002_Line_was);
		}
		STprintf(msg, online, macline);
		FKerrmsg(msg);
		STprintf(msgline, linewas, buffer);
		FKerrmsg(msgline);
		return;
	}
	if ((KEYDEFS[key] = (u_char *)MEreqmem((u_i4)0,
	    (u_i4)(STlength((char *)outbuf) + 1),
	    TRUE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("FK30parse"));
	}
	STcopy((char *) outbuf, (char *) KEYDEFS[key]);

	/*
	**  Fix for BUG 9449. (dkh)
	*/
	TDonmacro(key);

	/*
	**  Fix for Bug 10175. (bab)
	*/
	FKclrlbl(key);
}


/*
**	FKclrlbl -
**		remove labels from forms system display when
**		version 4.0 function keys are over-ridden by
**		version 3.0 macros.
**
**	written - 24-oct-86 (bab)
*/
VOID
FKclrlbl(key)
i4	key;
{
	i4	index;
	i4	cmdval;

	index = key + MAX_CTRL_KEYS + CURSOROFFSET;
	keymap[index].label = NULL;

	cmdval = keymap[index].cmdval;
	if (cmdval >= ftMUOFFSET && cmdval < ftFRSOFFSET)
		menumap[cmdval - ftMUOFFSET] = NULL;
}


KEYSTRUCT   *
FKnxtch()
{
	KEYSTRUCT	*ks;

	if (kbufp > 0)
	{
		ks = TDgetKS();
		TDsetKS(ks, INBUF[--kbufp], 1);
		if (CMdbl1st(ks->ks_ch))
		{
			TDsetKS(ks, INBUF[--kbufp], 2);
	}
		return(ks);
	}
	else
	{
		ks = ITin();

		/* check for EOF */

		if (((ks->ks_ch[0]) & 0377) == (EOF & 0377))
			EXsignal(EXTEEOF, 0);

		if (!infde_getch && fdrecover)
		{
			TDrcvstr(ks->ks_ch);
		}
		return(ks);
	}
}

VOID
FKungetch(c, count)
char	*c;
i4	count;
{
	char	*cp;
	char	errbuf[ER_MAX_LEN + 1];

	if ((kbufp + count) > KBUFSIZE)
	{
		STcopy(ERget(E_TD0016_INSOVFL), errbuf);
		FKmacmsg(errbuf);
	}
	else
	{
		for (cp = c; *cp != '\0'; cp++)
		{
		}
		cp--;
		for (; cp >= c; cp--)
		{
			INBUF[kbufp++] = *cp;
		}
	}
}


/*
**  This routine does the work of checking wheterh the input
**  matchings a cursor/function key sequence.
*/

i4
FKmapfkeys(cp)
u_char	*cp;
{
	reg	char		flag;
	reg	char		*cp1;
	reg	char		*cp2;
	reg	i4		keyp;
	reg	char		*tp1;
	reg	KEYSTRUCT	*ks;
		char		buf[20];

	keyp = 0;
	CMcpychar(cp, buf);
	cp1 = buf;
	for (cp2 = (char *) KEYSEQ[keyp]; keyp < MAXFKEYS; keyp++)
	{
		cp2 = (char *) KEYSEQ[keyp];
		flag = TRUE;
		if (!CMcmpcase(cp1, cp2))   /* (*cp1 == *cp2)	*/
		{
			CMnext(cp2);
			while (*cp2 != '\0')
			{
				ks = FKnxtch();
				CMnext(cp1);
				CMcpychar(ks->ks_ch, cp1);
				if (!CMcmpcase(cp1, cp2))  /* (*cp1 == *cp2) */
				{
					CMnext(cp2);
				}
				else {
					flag = FALSE;
					break;
				}
			}
			if (flag)
			{
				return(keyp+KEYOFFSET);
			}
			else
			{
				CMnext(cp1);
				*cp1 = '\0';
				tp1 = cp1;
				cp1 = buf;
				CMnext(cp1);
				FKungetch(cp1, tp1 - cp1);
				cp1 = buf;
				flag = TRUE;
			}
		}
	}
	CMnext(cp1);
	*cp1 = '\0';
	FKungetch(buf, cp1 - buf);
	return(NULLKEY);
}

i4
FKmacro(index)
i4	index;
{
	i4		keynum;
	u_char	*cp;
	char		errbuf[ER_MAX_LEN + 1];

	cp = KEYDEFS[index];
	if (KEYUSE[index]++ > 1)
	{
		FKmacmsg(ERget(E_TD0002_Cyclic_key_definition));
		return(0);
	}
	for (; *cp != '\0'; cp++)
	{
		if (*cp == FKEY_LEADIN)
		{
			/*
			**  Yes, we are blinding assuming that there
			**  is a valid functionkey number following
			**  and that we are not at the end of the string.
			*/

			keynum = *++cp;
			if (keynum < 1 || keynum > KEYTBLSZ)
			{
				STcopy(ERget(E_TD0017_BNUMEX), errbuf);
				FKmacmsg(errbuf);
				return(0);
			}
			keynum--;
			if (keynum == 0)
			{
				*mp++ = (u_char) MENU_KEY;
				if (mp >= (u_char *) MACBUFEND)
				{
					STcopy(ERget(E_TD0018_OVFL), errbuf);
					FKmacmsg(errbuf);
					return(0);
				}
			}
			else if (*(KEYDEFS[keynum]) == '\0')
			{
				STcopy(ERget(E_TD0019_REFDEF), errbuf);
				FKmacmsg(errbuf);
				return(0);
			}
			else if (!FKmacro((i4) (keynum)))
			{
				return(0);
			}
		}
		else
		{
			*mp++ = (u_char) *cp;
			if (mp >= CAST_PUNSCHAR MACBUFEND)
			{
				STcopy(ERget(E_TD0018_OVFL), errbuf);
				FKmacmsg(errbuf);
			}
		}
	}
	KEYUSE[index]--;
	return(1);
}


KEYSTRUCT   *
FKexpmac(k)
i4	k;
{
	i4	i;
	i4	index;
	char	errbuf[ER_MAX_LEN + 1];

	index = k - KEYOFFSET - CURSOROFFSET;
	if (*(KEYDEFS[index]) == '\0')
	{
		return(0);
	}
	else
	{
		*MACBUF = '\0';
		mp = CAST_PUNSCHAR MACBUF;
		MACBUFEND = MACBUF + KBUFSIZE;
		if (!FKmacro(index))
		{
			for (i = 0; i < FKEYS; i++)
			{
				KEYUSE[i] = '\0';
			}
			return(0);
		}
		*mp = '\0';
		if (STlength(MACBUF) == 0)
		{
			STcopy(ERget(E_TD001A_NULLEXP), errbuf);
			FKmacmsg(errbuf);
			return(0);
		}
		FKungetch(MACBUF, (i4) STlength(MACBUF));
		for (i = 0; i < FKEYS; i++)
		{
			KEYUSE[i] = '\0';
		}
		return(FKnxtch());
	}
}



KEYSTRUCT   *
FKgetinput(file)
FILE	*file;
{
	i4		key;
	KEYSTRUCT	*macro ,*ks;
	char		BUFMAC[KBUFSIZE];

	if (file != stdin)
	{
		ks = TDgetKS();
		TDsetKS(ks, SIgetc(file), 1);
		if (CMdbl1st(ks->ks_ch))
		{
			TDsetKS(ks, SIgetc(file), 2);
		}
		return(ks);
	}
	for (;;)
	{
		ks = FKnxtch();
		if (ks->ks_ch[0] < FIRSTPRINTABLE)
		{
			if ((key = FKmapfkeys(ks->ks_ch)) == NULLKEY)
			{
				ks = FKnxtch();
				if (ks->ks_ch[0] == ESCAPE)
				{
				    /*
				    ** ESC is special when using function
				    ** keys only if locator support is on.
				    */
				    if (IITDlsLocSupport)
				    {
					ks = IITDplsParseLocString();
					if (ks->ks_fk == KS_LOCATOR)
						return(ks);
					else
						continue;
				    }
				    else
				    {
					continue;
				    }
				}
				else
				{
					if (ks->ks_ch[0] == (u_char) MENU_KEY)
					{
						return(TDsetKS(ks, F0KEY, 1));
					}
					return(ks);
				}
			}
			else
			{
				switch(key)
				{
					case UPKEY:
					case DOWNKEY:
					case LEFTKEY:
					case RIGHTKEY:
						/*
						** If cursor keys are single
						** control chars, return the
						** character rather than the
						** function key designation.
						** For DG.  (bab)
						*/
						if (XF)
						{
							return(ks);
						}
						else
						{
							return(TDsetKS(ks, key, 1));
						}

					default:
						if (!macexp[key - KEYOFFSET - CURSOROFFSET])
						{
							if (key == MENU_KEY)
							{
								return(TDsetKS(ks, F0KEY, 1));
							}
							return(TDsetKS(ks, key, 1));
						}
						MACBUF = BUFMAC;
						if ((macro = FKexpmac(key)) != 0)
						{
							if (ks->ks_ch[0] == MENU_KEY)
							{
								return(TDsetKS(macro, F0KEY, 1));
							}
							return(macro);
						}
						else
						{
							return(TDsetKS(ks, key, 1));
						}
				}
			}
		}
		else
		{
			if (ks->ks_ch[0] == MENU_KEY)
			{
				return(TDsetKS(ks, F0KEY, 1));
			}
			return(ks);
		}
	}
}



/*{
** Name:	IITDpwiParseWViewInput - Parse Wview input strings.
**
** Description:
**	Does the actual job of parsing/breaking up an input
**	command string from Wview.
**
** TO DO:
**
**	- Error recovery (pushing chars back into input buf) due
**	  to parsing error.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ks	Pointer to KEYSTRUCT structure describing input.
**	Exceptions:
**		None.
**
** Side Effects:
**	Buffers are pushed back into input stream if parsing of input fails.
**
** History:
**	11/24/89 (dkh) - Initial version.
*/
KEYSTRUCT *
IITDpwiParseWViewInput()
{
	KEYSTRUCT	*ks;
	u_char		tbuf[KBUFSIZE];
	u_char		numbuf[KBUFSIZE];
	u_char		*tptr = tbuf;
	u_char		*np = numbuf;
	u_char		term_ch = ',';
	u_char		command;
	i4		menu_fast = FALSE;
	i4		num1;
	i4		num2;
	i4		num3;
	bool		old_disabled;

	/*
	**  Store away original control right bracket.  We know
	**  there was one since we are in this routine.
	*/
	*tptr++ = INSUFIX;

	ks = (*in_func)(stdin);

	/*
	**  We expect to see a string of the "^]^][I,G,A,H]num[,num,num]^]".
	**  So we expect the next character to be control right bracket
	**  (we have **  already seen the first control right bracket).
	**  If next character is not the control right bracket character, then
	**  just return that value.
	*/
	if ((*tptr++ = ks->ks_ch[0]) != INSUFIX)
	{
		*tptr = EOS;
		FKungetch(tbuf, (i4) STlength((char *) tbuf));
		return((*in_func)(stdin));
	}
	ks = (*in_func)(stdin);
	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);

	command = ks->ks_ch[0];

	if (command != MU_FAST && command != GOTOFLD &&
		command != TURNON && command != HOTSPOT)
	{
		*tptr = EOS;
		FKungetch(tbuf, (i4) STlength((char *) tbuf));
		return((*in_func)(stdin));
	}

	if (command == MU_FAST)
	{
		menu_fast = TRUE;
	}
	else if (command == TURNON)
	{
		term_ch = ')';
	}
	else if (command == HOTSPOT)
	{
		term_ch = INSUFIX;
	}

	/*
	**  Find and drop the opening '(' for a TURNON
	**  command.
	*/
	if (command == TURNON)
	{
		ks = (*in_func)(stdin);
		if (CMcmpnocase(ks->ks_ch, ERx("(")))
		{
			CMcpychar(ks->ks_ch, tptr);
			CMnext(tptr);
			*tptr = EOS;
			FKungetch(tbuf, (i4) STlength((char *) tbuf));
			return((*in_func)(stdin));
		}
	}

	/*
	**  Now start looking for the digits;
	*/
	np = numbuf;
	*np = EOS;
	ks = (*in_func)(stdin);
	while (CMdigit(ks->ks_ch))
	{
		CMcpychar(ks->ks_ch, np);
		CMcpychar(ks->ks_ch, tptr);
		CMnext(np);
		CMnext(tptr);
		ks = (*in_func)(stdin);
	}
	*np = EOS;
	if (np == numbuf || ks->ks_ch[0] != term_ch ||
		CVan(numbuf, &num1) != OK)
	{
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		*tptr = EOS;
		FKungetch(tbuf, (i4) STlength((char *) tbuf));
		return((*in_func)(stdin));
	}

	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);

	/*
	**  Look for INSUFIX after the closing ')' for a
	**  TURNON command.
	*/
	if (command == TURNON)
	{
		ks = (*in_func)(stdin);
		if (ks->ks_ch[0] != INSUFIX)
		{
			CMcpychar(ks->ks_ch, tptr);
			CMnext(tptr);
			*tptr = EOS;
			FKungetch(tbuf, (i4) STlength((char *) tbuf));
			return((*in_func)(stdin));
		}
	}

	if (command == TURNON || command == HOTSPOT)
	{
		*tptr = EOS;
		if (fdrecover)
		{
			old_disabled = Rcv_disabled;
			Rcv_disabled = FALSE;
			TDrcvstr(tbuf);
			Rcv_disabled = old_disabled;
		}
		if (command == TURNON)
		{
			ks->ks_fk = KS_TURNON;
			ks->ks_ch[0] = EOS;
		}
		else
		{
			ks->ks_fk = KS_HOTSPOT;
		}
		ks->ks_p1 = num1;

		return(ks);
	}
	else if (menu_fast)
	{
		term_ch = INSUFIX;
	}
	np = numbuf;
	*np = EOS;
	ks = (*in_func)(stdin);
	while (CMdigit(ks->ks_ch))
	{
		CMcpychar(ks->ks_ch, np);
		CMnext(np);
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		ks = (*in_func)(stdin);
	}
	*np = EOS;
	if (np == numbuf || ks->ks_ch[0] != term_ch ||
		CVan(numbuf, &num2) != OK)
	{
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		*tptr = EOS;
		FKungetch(tbuf, (i4) STlength((char *) tbuf));
		return((*in_func)(stdin));
	}
	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);
	if (menu_fast)
	{
		*tptr = EOS;
		if (fdrecover)
		{
			old_disabled = Rcv_disabled;
			Rcv_disabled = FALSE;
			TDrcvstr(tbuf);
			Rcv_disabled = old_disabled;
		}

		/*
		**  Set up return info for a menu fast path command.
		*/
		ks->ks_fk = KS_MFAST;
		ks->ks_p1 = num1;
		ks->ks_p2 = num2;

		return(ks);
	}

	term_ch = INSUFIX;
	np = numbuf;
	*np = EOS;
	ks = (*in_func)(stdin);
	while (CMdigit(ks->ks_ch))
	{
		CMcpychar(ks->ks_ch, np);
		CMnext(np);
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		ks = (*in_func)(stdin);
	}
	*np = EOS;
	if (np == numbuf || ks->ks_ch[0] != term_ch ||
		CVan(numbuf, &num3) != OK)
	{
		CMcpychar(ks->ks_ch, tptr);
		CMnext(tptr);
		*tptr = EOS;
		FKungetch(tbuf, (i4) STlength((char *) tbuf));
		return((*in_func)(stdin));
	}

	CMcpychar(ks->ks_ch, tptr);
	CMnext(tptr);
	*tptr = EOS;
	if (fdrecover)
	{
		old_disabled = Rcv_disabled;
		Rcv_disabled = FALSE;
		TDrcvstr(tbuf);
		Rcv_disabled = old_disabled;
	}

	/*
	**  Set up return info for a goto command from windex.
	*/
	ks->ks_fk = KS_GOTO;
	ks->ks_p1 = num1;
	ks->ks_p2 = num2;
	ks->ks_p3 = num3;

	return(ks);
}



/*{
** Name:	IITDwiWviewInput - Wview input processing routine.
**
** Description:
**	Checks to see if input stream contains anything interested
**	from Wview (such as menu selection or goto a field fast path.
**	Actually calls IITDpwiParseWViewInput() to do the dirty work.
**
**	Will trap a version number coming from WView and not pass
**	it back to caller.
**
** Inputs:
**	None.
**
** Outputs:
**
**	Returns:
**		ks	Pointer to KEYSTRUCT structure describing input.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/24/89 (dkh) - Initial version.
*/
KEYSTRUCT *
IITDwiWviewInput()
{
	KEYSTRUCT	*ks;

	for (;;)
	{
		ks = IITDpwiParseWViewInput();
		if (ks->ks_fk == KS_TURNON)
		{
			IITDwvWVVersion = ks->ks_p1;
			ks = (*in_func)(stdin);

			/*
			**  This needs to be rethought.
			*/
			if (ks->ks_ch[0] == INSUFIX)
			{
				continue;
			}
		}
		break;
		
	}

	return(ks);
}



/*{
** Name:	IITDmfkMapFuncKeys - Map a function key number to keystrokes.
**
** Description:
**	Return the keystroke sequence for a function key.  Assumes
**	that this will not be called if function keys are not
**	available (i.e., KY is false).
**
** Inputs:
**	funckey		Function key index.  Zero to three are the arrrow keys
**			up, down, right and left.  First function key starts
**			at value 4.
**
** Outputs:
**	pcp		Pointer to keystroke string or empty string
**			if the passed in number is out of bounds.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01/23/90 (dkh) - Initial version.
*/
VOID
IITDmfkMapFuncKeys(funckey, pcp)
i4	funckey;
u_char	**pcp;
{
	u_char	*cp;

	if (funckey < 0 || funckey > KEYTBLSZ + 4)
	{
		cp = (u_char *) ERx("");
	}
	else
	{
		cp = KEYSEQ[funckey];
	}

	*pcp = cp;
}

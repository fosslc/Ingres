/*
**  funckeys.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/05/85 (dkh)
**	87/04/15  dave - Integrated Kanji changes. (dkh)
**	87/04/08  joe - Added compat, dbms and fe.h
**	86/11/18  dave - Initial60 edit of files
**	86/02/04  dave - Fix for BUG 8106. (dkh)
**	86/01/03  dave - Changed comment to identify the correct
**			 bug number.  Code had 7751 instead of 7715. (dkh)
**	86/01/03  dave - Fix for BUG 7715. (dkh)
**	85/11/05  dave - extern to FUNC_EXTERN for routines. (dkh)
**	85/10/25  dave - Added detection of control chars in labels. (dkh)
**	85/10/18  dave - Initial revision
**	85/10/11  peter - Allow tab at start of lines.
**	85/10/05  dave - Added support for print screen and
**			 skip field commands. (dkh)
**	85/09/26  dave - Added some bullet proofing. (dkh)
**	85/09/26  dave - Added some bullet proofing. (dkh)
**	85/08/09  dave - Last set of changes for function key support. (dkh)
**	85/07/19  dave - Initial revision
**	10/20/86 (KY)  -- Changed CH.h to CM.h.
**	06/19/87 (dkh) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/25/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	24-sep-87 (bab)
**		Added recognition of the shell command.
**	02/27/88 (dkh) - Added support for nextitem command.
**	10/27/88 (dkh) - Performance changes.
**	08/03/89 (dkh) - Added support for mapping arrow keys.
**	08/31/93 (dkh) - Fixed up compiler complaints due to prototyping.
**      23-sep-96 (mcgem01)
**              Global data moved to ftdata.c
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
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<termdr.h>
# include	<tdkeys.h>
# include	<mapping.h>
# include	"ftframe.h"
# include	<menu.h>
# include	<cm.h>
# include	<si.h>
# include	<er.h>
# include	<erft.h>
# include	<ug.h>

GLOBALREF	i4	FKdebug;

FUNC_EXTERN	char	*FKgetline();
GLOBALREF	i4	parsing;
GLOBALREF	char	*ZZA;
GLOBALREF 	i4     FKEYS;


/*
**  Find the beginning of a mapping function throwing out any
**  leading blanks and comments.  Sets what ppc points to to
**  the beginning of any good data.
*/

i4
FKfindbeg(ppc)
u_char	**ppc;
{
	i4		retval;
	u_char	*cp = *ppc;

	while (*cp != '\0')
	{
		while (CMwhite(cp))
		{
			CMnext(cp);
		}

		if (*cp == '\0' || *(cp + 1) == '\0')
		{
			break;
		}

		if (*cp == '/' && *(cp + 1) == '*')
		{
			for (cp += 2; *cp; CMnext(cp))
			{
				if (*cp == '*')
				{
					if (*++cp == '/')
					{
						cp++;
						break;
					}
					else
					{
						cp--;
					}
				}
			}
		}
		else
		{
			break;
		}
	}

	if (*cp == '\0')
	{
		retval = ALLCOMMENT;
	}
	else
	{
		retval = FOUND;
		*ppc = cp;
	}
	return(retval);
}


/*
**  Find a string of alphabetic characters, ignoring any leading
**  blanks or comments.
*/

i4
FKgetword(ppc, buf)
u_char	**ppc;
u_char	*buf;
{
	i4		retval = FOUND;
	u_char	*cp;
	u_char	*bp = buf;

	*bp = '\0';
	if (FKfindbeg(ppc) == ALLCOMMENT)
	{
		retval = NOTFOUND;
	}
	else
	{
		cp = *ppc;
		while (CMnmstart(cp))
		{
			CMcpyinc(cp, bp);
		}
		*bp = '\0';
		if (bp == buf)
		{
			retval = NOTFOUND;
		}
		else
		{
			*ppc = cp;
		}
	}
	return(retval);
}


/*
**  Find to see if string in buf is a valid command and what type it is.
*/

i4
FKcmdtype(buf, cmdtype, num, nxtch)
u_char	*buf;
i4	*cmdtype;
i4	*num;
u_char	*nxtch;
{
	i4		retval = FDUNKNOWN;
	i4		length;
	u_char	*cp;
	bool		found_ctrl;
	char		errbuf[ER_MAX_LEN + 1];

	CVlower(buf);

	switch (*buf)
	{
		case 'c':
			found_ctrl = FALSE;
			length = STlength((char *) buf);
			if (STcompare(buf, ERx("clear")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftCLEAR;
			}
			else if (STcompare(buf, ERx("clearrest")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftCLRREST;
			}
			else if (STbcompare((char *)buf, length, ERx("control"),
				7, TRUE) == 0)
			{
				found_ctrl = TRUE;
				cp = buf + 7;
			}
			else if (STbcompare((char *) buf, length, ERx("ctrl"),
				4, TRUE) == 0)
			{
				found_ctrl = TRUE;
				cp = buf + 4;
			}
			if (found_ctrl)
			{
				length = STlength((char *) cp);
				if (length != 1 && length != 3)
				{
					retval = FDUNKNOWN;
				}
				else
				{
					if (length == 3)
					{
						if (STcompare(cp,
							ERx("esc")) == 0)
						{
							/*
							**  Check if function
							**  key enabled and
							**  if international
							**  support has an
							**  escape as the
							**  leadin char.
							*/

							if (KY || (ZZA != NULL
							  && *ZZA == '\033'))
							{
					STcopy(ERget(E_FT0036_NOESC), errbuf);
							    FKerrmsg(errbuf);
							    retval = FDUNKNOWN;
							    break;
							}
							*num = CTRL_ESC;
						}
						else if (STcompare(cp,
							ERx("del")) == 0)
						{
							*num = CTRL_DEL;
						}
						else
						{
							retval = FDUNKNOWN;
							break;
						}
					}
					else
					{
						*num = *cp - 'a';
					}
					retval = FDCTRL;
					*cmdtype = FDCTRL;
				}
			}
			break;

		case 'd':
			if (STcompare(buf, ERx("deletechar")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftDELCHAR;
			}
			else if (STcompare(buf, ERx("downline")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftDNLINE;
			}
			else if (STcompare(buf, ERx("duplicate")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftAUTODUP;
			}
			else if (STcompare(buf, ERx("downarrow")) == 0)
			{
				retval = FDARROW;
				*cmdtype = ftDNARROW;
			}
			break;

		case 'e':
			if (STcompare(buf, ERx("editor")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftEDITOR;
			}
			break;

		case 'f':
			if (STcompare(buf, ERx("frskey")) == 0)
			{
				retval = FDFRSK;
				*cmdtype = FDFRSK;
			}
			break;

		case 'l':
			if (STcompare(buf, ERx("leftchar")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftLEFTCH;
			}
			else if (STcompare(buf, ERx("leftarrow")) == 0)
			{
				retval = FDARROW;
				*cmdtype = ftLTARROW;
			}
			break;

		case 'm':
			if (STcompare(buf, ERx("menu")) == 0)
			{
				/*
				**  Fix for BUG 7715. (dkh)
				*/
				if (CMdigit(nxtch))
				{
					retval = FDMENUITEM;
					*cmdtype = FDMENUITEM;
				}
				else
				{
					retval = FDCMD;
					*cmdtype = ftMENU;
				}
			}
			else if (STcompare(buf, ERx("menuitem")) == 0)
			{
				retval = FDMENUITEM;
				*cmdtype = FDMENUITEM;
			}
			else if (STcompare(buf, ERx("mode")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftMODE40;
			}
			break;

		case 'n':
			if (STcompare(buf, ERx("nextfield")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftNXTFLD;
			}
			else if (STcompare(buf, ERx("nextword")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftNXTWORD;
			}
			else if (STcompare(buf, ERx("newrow")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftNEWROW;
			}
			else if (STcompare(buf, ERx("nextitem")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftNXITEM;
			}
			break;

		case 'o':
			if (STcompare(buf, ERx("off")) == 0)
			{
				retval = FDOFF;
				*cmdtype = FDOFF;
			}
			break;

		case 'p':
			if (STcompare(buf, ERx("previousfield")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftPRVFLD;
			}
			else if (STcompare(buf, ERx("previousword")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftPRVWORD;
			}
			else if (STcompare(buf, ERx("pf")) == 0)
			{
				retval = FDPFKEY;
				*cmdtype = FDPFKEY;
			}
			else if (STcompare(buf, ERx("printscreen")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftPRSCR;
			}
			break;

		case 'r':
			if (STcompare(buf, ERx("redraw")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftREDRAW;
			}
			else if (STcompare(buf, ERx("rubout")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftRUBOUT;
			}
			else if (STcompare(buf, ERx("rightchar")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftRGHTCH;
			}
			else if (STcompare(buf, ERx("rightarrow")) == 0)
			{	retval = FDARROW;
				*cmdtype = ftRTARROW;
			}
			break;

		case 's':
			if (STcompare(buf, ERx("scrollup")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftUPSCRLL;
			}
			else if (STcompare(buf, ERx("scrolldown")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftDNSCRLL;
			}
			else if (STcompare(buf, ERx("scrollleft")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftLTSCRLL;
			}
			else if (STcompare(buf, ERx("scrollright")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftRTSCRLL;
			}
			else if (STcompare(buf, ERx("shell")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftSHELL;
			}
			else if (STcompare(buf, ERx("skipfield")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftSKPFLD;
			}
			break;

		case 'u':
			if (STcompare(buf, ERx("upline")) == 0)
			{
				retval = FDCMD;
				*cmdtype = ftUPLINE;
			}
			else if (STcompare(buf, ERx("uparrow")) == 0)
			{
				retval = FDARROW;
				*cmdtype = ftUPARROW;
			}
			break;

		default:
			retval = FDUNKNOWN;
			break;
	}

	
	return(retval);
}


/*
**  Get a number starting at *ppc.  No leading blanks or
**  comments are allowed.
*/

i4
FKgetnum(ppc, num)
u_char	**ppc;
i4		*num;
{
	i4		retval = FOUND;
	u_char	*cp;
	u_char	*bp;
	u_char	buf[LINESIZE];
	char		errbuf[ER_MAX_LEN + 1];

	cp = *ppc;
	bp = buf;
	*bp = '\0';
	while (*cp != '\0' && CMdigit(cp))
	{
		*bp++ = *cp++;
	}
	if (bp == buf)
	{
		retval = NOTFOUND;
	}
	else
	{
		*bp = '\0';
		if (CVan(buf, num) != OK)
		{
			STcopy(ERget(E_FT0020_NUMCONV), errbuf);
			FKerrmsg(errbuf);
			retval = NOTFOUND;
		}
		else
		{
			*ppc = cp;
		}
	}

	return(retval);
}


i4
FKgetequal(ppc)
u_char	**ppc;
{
	i4		retval = FOUND;
	u_char	*cp;

	if (FKfindbeg(ppc) == ALLCOMMENT)
	{
		retval = NOTFOUND;
	}
	cp = *ppc;
	if (*cp != '=')
	{
		retval = NOTFOUND;
	}
	else
	{
		*ppc = CMnext(cp);
	}

	return(retval);
}


bool
FKgetany(ppc)
u_char	**ppc;
{
	u_char	*cp;
	bool		retval = TRUE;

	cp = *ppc;
	if (FKfindbeg(ppc) == ALLCOMMENT)
	{
		retval = FALSE;
	}
	*ppc = cp;

	return(retval);
}


i4
FKgetlabel(ppc, buf)
u_char	**ppc;
u_char	*buf;
{
	u_char	*cp;
	u_char	*bp;
	u_char	*ucp;
	i4	retval = FDGOODLBL;
	bool	end_found;

	if (FKfindbeg(ppc) == ALLCOMMENT)
	{
		retval = FDNOLBL;
	}
	else
	{
		cp = *ppc;
		bp = buf;
		if (*cp != '(')
		{
			retval = FDBADLBL;
		}
		else
		{
			CMnext(cp);
			end_found = FALSE;
			while (*cp)
			{
				if (*cp != ')')
				{
					if (CMcntrl(cp))
					{
						ucp = (u_char *) CMunctrl(cp);
						while(*ucp)
						{
							*bp++ = *ucp++;
						}
					}
					else
					{
						CMcpychar(cp, bp);
						CMnext(bp);
					}
					CMnext(cp);
				}
				else
				{
					end_found = TRUE;
					break;
				}
			}
			if (end_found)
			{
				*bp = '\0';
				*ppc = CMnext(cp);
			}
			else
			{
				retval = FDBADLBL;
			}
		}
	}

	return(retval);
}


VOID
FK40parse(fp, level)
FILE	*fp;
i4	level;
{
	i4		macline = 0;
	i4		lcmdtype;
	i4		lcmdgrp;
	i4		lkey_num;
	i4		rcmdtype;
	i4		rcmdgrp;
	i4		rkey_num;
	i4		errnum;
	i4		akey;
	i4		key2;
	i4		maxkeynum;
	u_char	*cp;
	u_char	buffer[KBUFSIZE];
	u_char	word[KBUFSIZE];
	char		errbuf[ER_MAX_LEN + 1];
	bool		found_label;

	parsing = AFILE;
	while (FKgetline(buffer, (i4) LINESIZE, fp) != NULL)
	{
		macline++;
	    /*
		STtrmwhite(buffer);
	    */
		cp = buffer;
		if (*cp == '\0' || *cp == '!')
		{
			continue;
		}
		if (*cp == '#')
		{
			FK30parse(buffer, macline);
			continue;
		}
		if (FKfindbeg(&cp) == ALLCOMMENT)
		{
			continue;
		}
		if (FKgetword(&cp, word) == NOTFOUND)
		{
			/*
			**  Really need to put these messages into
			**  an error file.
			*/

			STcopy(ERget(E_FT0022_BSTART), errbuf);
			FKprserr(errbuf, macline, buffer);
			continue;
		}
		switch (lcmdgrp = FKcmdtype(word, &lcmdtype, &lkey_num, cp))
		{
			case FDCMD:
			case FDCTRL:
			case FDARROW:
				break;
			case FDFRSK:
			case FDPFKEY:
			case FDMENUITEM:
				if (FKgetnum(&cp, &lkey_num) == NOTFOUND)
				{
					STcopy(ERget(E_FT0023_FRSCTRL), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (lkey_num <= 0)
				{
					STcopy(ERget(E_FT0034_KEYZERO), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (lcmdgrp == FDPFKEY)
				{
					/*
					**  Fix for BUG 8106. (dkh)
					*/
					if (!KY)
					{
						STcopy(ERget(E_FT0037_NOPFK),
							errbuf);
						FKprserr(errbuf, macline,
							buffer);
						continue;
					}
					errnum = E_FT0024_FRSKEY;
					maxkeynum = KEYTBLSZ;
				}
				if (lcmdgrp == FDMENUITEM)
				{
					errnum = E_FT0035_MAXMENU;
					maxkeynum = MAX_MENUITEMS;
				}
				if (lcmdgrp == FDFRSK)
				{
					errnum = E_FT0024_FRSKEY;
					maxkeynum = MAX_FRS_KEYS;
				}
				if (lkey_num > maxkeynum)
				{
					key2 = maxkeynum;
					IIUGfmt(errbuf, ER_MAX_LEN,
						ERget(errnum), 1, &key2);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				break;

			default:
				STcopy(ERget(E_FT0025_UNKNOWN), errbuf);
				FKprserr(errbuf, macline, buffer);
				continue;
		}
		if (FKgetequal(&cp) != FOUND)
		{
			STcopy(ERget(E_FT0026_NOEQUAL), errbuf);
			FKprserr(errbuf, macline, buffer);
			continue;
		}
		if (FKgetword(&cp, word) == NOTFOUND)
		{
			STcopy(ERget(E_FT002D_CTRLPF), errbuf);
			FKprserr(errbuf, macline, buffer);
			continue;
		}
		switch(rcmdgrp = FKcmdtype(word, &rcmdtype, &rkey_num, cp))
		{
			case FDARROW:
				if (lcmdgrp == FDCTRL || lcmdgrp == FDPFKEY ||
					lcmdgrp == FDARROW)
				{
					STcopy(ERget(E_FT0029_BADCMD), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				/*
				**  Just break here since one really
				**  can have arrow keys without
				**  having function keys.
				*/
				break;

			case FDPFKEY:
				/*
				**  Fix for BUG 8106. (dkh)
				*/
				if (!KY)
				{
					STcopy(ERget(E_FT0037_NOPFK), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (lcmdgrp == FDCTRL || lcmdgrp == FDPFKEY)
				{
					STcopy(ERget(E_FT0029_BADCMD), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (FKgetnum(&cp, &rkey_num) == NOTFOUND)
				{
					STcopy(ERget(E_FT0027_PFKEY), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (rkey_num <= 0)
				{
					STcopy(ERget(E_FT0034_KEYZERO), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				if (rkey_num > FKEYS)
				{
					akey = rkey_num;
					IIUGfmt(errbuf, ER_MAX_LEN,
						ERget(E_FT0028_BPFKEY),
						1, &akey);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				break;

			case FDCTRL:
				if (lcmdgrp == FDCTRL || lcmdgrp == FDPFKEY)
				{
					STcopy(ERget(E_FT0029_BADCMD), errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;
				}
				break;

			case FDOFF:
				if (lcmdgrp == FDCTRL || lcmdgrp == FDPFKEY ||
					lcmdgrp == FDARROW)
				{
					break;
				}
			default:
				STcopy(ERget(E_FT0029_BADCMD), errbuf);
				FKprserr(errbuf, macline, buffer);
				continue;
		}

		if (rcmdgrp  == FDOFF)
		{
			if (FKfindbeg(&cp) != ALLCOMMENT)
			{
				STcopy(ERget(E_FT002A_OFFIGN), errbuf);
				FKprserr(errbuf, macline, buffer);
				continue;
			}
		}
		else
		{
			found_label = FALSE;
			switch (FKgetlabel(&cp, word))
			{
				case FDNOLBL:
					word[0] = '\0';
					break;

				case FDBADLBL:
					STcopy(ERget(E_FT002B_BADLABEL),errbuf);
					FKprserr(errbuf, macline, buffer);
					continue;

				case FDGOODLBL:
				default:
					found_label = TRUE;
					if (FKfindbeg(&cp) != ALLCOMMENT)
					{
						STcopy(ERget(E_FT002C_LBLIGN),
							errbuf);
						FKprserr(errbuf, macline,
							buffer);
					}
					break;
			}
		}

		/*
		**  Do something with the information.
		*/

		if (FKdomap(lcmdgrp, lcmdtype, lkey_num, rcmdgrp, rcmdtype,
			rkey_num, word, level, errbuf) != OK)
		{
			FKprserr(errbuf, macline, buffer);
		}

		if (FKdebug)
		{
			/*
			**  Should make these diagnostic mesages.
			*/
			SIprintf(ERx("lcmdgrp = %d, lcmdtype = %d, lkey_num = %d\r\n"), lcmdgrp, lcmdtype, lkey_num);
			SIprintf(ERx("rcmdgrp = %d, rcmdtype = %d, rkey_num = %d\r\n"), rcmdgrp, rcmdtype, rkey_num);
			if (found_label)
			{
				SIprintf(ERx("Label = %s\r\n"), word);
			}
			else
			{
				SIprintf(ERx("No label\r\n"));
			}
		}

	}
}

/*
**  mapping.c
**
**  Do control/function key mapping
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/10/85 (dkh)
**	86/11/18  dave - Initial60 edit of files
**	85/11/12  dave - Fixed to properly identify control/pf
**			 key on a error message. (dkh)
**	85/11/07  dave - Eliminated some unnecessary code and took
**			 away restriction on setting the application
**			 mapping file multiple times. (dkh)
**	85/11/04  dave - Initialized FKdmpfile to "stdout"
**			 instead of NULL. (dkh)
**	85/11/01  dave - Changed the default mapping so that
**			 fdopRUB is only valid for fdcmINSRT mode. (dkh)
**	85/10/17  dave - Fixed problem with menu label get
**			 cleared incorrectly. (dkh)
**	85/10/16  dave - Fixed problem with not unmapping previous
**			 frskey and menuitem definitions when they
**			 are reassigned. (dkh)
**	85/10/05  dave - Added support for print screen and
**			 skip field commands. (dkh)
**	85/10/04  dave - Fixed structure dumping routines to be
**			 more useful. (dkh)
**	85/09/26  dave - Added routine to dump frs key mapping
**			 structures for QA. (dkh)
**	85/09/26  dave - Added FTdmpcfk() to dump frskey mapping
**			 structs for QA. (dkh)
**	85/08/24  dave - Enhanced code to do handle forms system
**			 command mappings better. (dkh)
**	85/08/09  dave - Last set of changes for function key support. (dkh)
**	85/07/19  dave - Initial revision
**	03/04/87 (dkh) - Added support for ADTs.
**	08/14/87 (dkh) - ER changes.
**	08/28/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	24-sep-87 (bab)
**		Added recognition of shell command.
**	02/27/88 (dkh) - Added support for nextitem command.
**	06/18/88 (dkh) - Integrated MVS changes.
**	10/28/88 (dkh) - Performance changes.
**	11/13/88 (dkh) - Rearranged code so non-forms programs can link
**			 in IT* routines.
**	08/03/89 (dkh) - Added support for mapping arrow keys.
**	10/06/89 (dkh) - Changed FKdmpfile to be static and initialized
**			 to NULL.
**	01/26/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	04/04/90 (dkh) - Integrated MACWS changes.
**	05/22/90 (dkh) - Enabled MACWS changes.
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	13-dec-91 (leighb) DeskTop Porting Change:
**		In keymap struct, made control S the same as control U.  This
**		is because control U is a printable character on the IBM/PC.
**	03/01/92 (dkh) - Made above fix specific to the PC world since controlS
**			 is normally one of the flow control characters for
**			 the ASCII world.
**	04-mar-93 (leighb) DeskTop Porting Change:
**		Pass frs4, frs7 and menukey to TE in CL.
**		These are used in MS-Windows to simulate mouse keystrokes.
**		These are ifdefed for PMFEWIN3.
**      14-Jun-95 (fanra01)
**              Added NT_GENERIC to PMFEWIN3 sections.
**	24-sep-95 (tutto01)
**		NT front ends converted to console apps.  Had to remove some
**		calls to functions which no longer exist.
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
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<menu.h>
# include	<mapping.h>
# include	<si.h>
# include	<st.h>
# include	<te.h>
# include	<er.h>
# include	<erft.h>
# include	<ug.h>

GLOBALREF	struct	mapping	keymap[MAX_CTRL_KEYS + KEYTBLSZ + 4];

# define	ESCAPE		'\033'
# define	DELETE		'\177'

GLOBALREF	char	*menumap[MAX_MENUITEMS]; 

GLOBALREF	i4	IIFTltLabelTag;

static		FILE	*FKdmpfile = NULL;

FUNC_EXTERN	VOID	IITDmfkMapFuncKeys();
FUNC_EXTERN	VOID	IITDwksWviewKeySet();

/*
**  Sets the file pointer so we can dump function key mapping to
**  whatever file the file pointer is pointing to.
*/

VOID
FTfkdfile(fp)
FILE	*fp;
{
	FKdmpfile = fp;
}


/*
**  Dump the the "keymap" structure above to file pointed to
**  by "FKdmpfile".
*/

VOID
FTdmpcfk()
{
	i4		i;
	i4		maxkeys;
	struct	mapping	*mptr;
	FILE		*fp = FKdmpfile;
	char		*cp;

	if (fp == NULL)
	{
		return;
	}
	SIfprintf(fp, ERx("%-10s%-10s%-10s%-10s%-10s\n\n"), ERx("Index"),
		ERx("Operator"), ERx("Mode"), ERx("DefLevel"),
		ERx("Label"));
	for (i = 0, maxkeys = MAX_CTRL_KEYS + KEYTBLSZ + 4; i < maxkeys; i++)
	{
		mptr = &(keymap[i]);
		if ((cp = mptr->label) == NULL)
		{
			cp = ERx("<NULL POINTER>");
		}
		SIfprintf(fp, ERx("%-10d%-10d%-10d%-10d%s\n"), i, mptr->cmdval,
			mptr->cmdmode, mptr->who_def, cp);
	}
}



STATUS
FKlvlchk(mptr, level, errbuf, buf)
struct	mapping	*mptr;
i4		level;
char		*errbuf;
char		*buf;
{
	i4	whodef;
	STATUS	retval = OK;

	whodef = mptr->who_def;

	if (level < whodef)
	{
		/*
		**  Treat application and specific key setting levels
		**  the same. (dkh)
		*/
		if (level < DEF_BY_APPL)
		{
			IIUGfmt(errbuf, ER_MAX_LEN, ERget(E_FT0032_OVRRIDE),
				1, buf);
			retval = FAIL;
		}
	}
	else if (level == whodef && level != DEF_BY_APPL)
	{
		if (mptr->cmdval != ftNOTDEF)
		{
			IIUGfmt(errbuf, ER_MAX_LEN, ERget(E_FT0030_INUSE),
				1, buf);
			retval = FAIL;
		}
	}

	return(retval);
}


VOID
FKunmap(mptr)
struct	mapping	*mptr;
{
	i4	cmdval;

	cmdval = mptr->cmdval;

	if (cmdval >= ftMUOFFSET && cmdval < ftFRSOFFSET)
	{
		/*
		**  Was formerly mapped to a menu position.
		*/

		cmdval -= ftMUOFFSET;
		menumap[cmdval] = NULL;
	}
}

VOID
FKxtcmd(cmdtype, cmdval, cmdmode)
i4	cmdtype;
i4	*cmdval;
i4	*cmdmode;
{
	i4	val;
	i4	mode = fdcmINBR;

	switch(cmdtype)
	{
		case ftNXTFLD:
			val = fdopNEXT;
			break;

		case ftPRVFLD:
			val = fdopPREV;
			break;

		case ftNXTWORD:
			val = fdopWORD;
			break;

		case ftPRVWORD:
			val = fdopBKWORD;
			break;

		case ftMODE40:
			val = fdopMODE;
			mode = fdcmINSRT;
			break;

		case ftREDRAW:
			val = fdopREFR;
			break;

		case ftDELCHAR:
			val = fdopDELF;
			mode = fdcmINSRT;
			break;

		case ftRUBOUT:
			val = fdopRUB;
			mode = fdcmINSRT;
			break;

		case ftEDITOR:
			val = fdopVERF;
			mode = fdcmINSRT;
			break;

		case ftLEFTCH:
			val = fdopLEFT;
			break;

		case ftRGHTCH:
			val = fdopRIGHT;
			break;

		case ftDNLINE:
			val = fdopDOWN;
			break;

		case ftUPLINE:
			val = fdopUP;
			break;

		case ftNEWROW:
			val = fdopNROW;
			break;

		case ftCLEAR:
			val = fdopCLRF;
			mode = fdcmINSRT;
			break;

		case ftCLRREST:
			val = fdopRET;
			break;

		case ftMENU:
			val = fdopMENU;
			break;

		case ftUPSCRLL:
			val =  fdopSCRUP;
			break;

		case ftDNSCRLL:
			val = fdopSCRDN;
			break;

		case ftLTSCRLL:
			val = fdopSCRLT;
			break;

		case ftRTSCRLL:
			val = fdopSCRRT;
			break;

		case ftAUTODUP:
			val = fdopADUP;
			mode = fdcmINSRT;
			break;

		case ftPRSCR:
			val = fdopPRSCR;
			break;

		case ftSKPFLD:
			val = fdopSKPFLD;
			break;

		case ftSHELL:
			val = fdopSHELL;
			break;

		case ftNXITEM:
			val = fdopNXITEM;
			break;

		default:
			val = fdopERR;
			mode = fdcmNULL;
			break;
	}
	*cmdval = val;
	*cmdmode = mode;
}


VOID
FKumapcmd(cmdval, level)
i4	cmdval;
i4	level;
{
	i4		i;
	i4		max_keys;
	struct	mapping	*mptr;

	max_keys = MAX_CTRL_KEYS + KEYTBLSZ + 4;
	for (i = 0; i < max_keys; i++)
	{
		mptr = &(keymap[i]);
		if (mptr->cmdval == cmdval)
		{
			mptr->label = NULL;
			mptr->cmdval = ftNOTDEF;
			mptr->cmdmode = fdcmNULL;
			mptr->who_def = level;

# ifdef DATAVIEW
			IIMWrmeRstMapEntry('k', i);
# endif	/* DATAVIEW */

			return;
		}
	}
}


STATUS
FKdomap(lcmdgrp, lcmdtype, lkeynum, rcmdgrp, rcmdtype, rkeynum, label,
	level, errbuf)
i4	lcmdgrp;
i4	lcmdtype;
i4	lkeynum;
i4	rcmdgrp;
i4	rcmdtype;
i4	rkeynum;
char	*label;
i4	level;
char	*errbuf;
{
	i4		index;
	i4		cmdval;
	i4		cmdmode;
	struct	mapping	*mptr;
	char		*cp;
	STATUS		retval = OK;
	char		buf[40];
	char		c[40];

	switch (lcmdgrp)
	{
		case FDCMD:
		case FDFRSK:
		case FDMENUITEM:
			if (rcmdgrp != FDCTRL && rcmdgrp != FDPFKEY &&
				rcmdgrp != FDARROW)
			{
				retval = FAIL;
				break;
			}
			if (rcmdgrp == FDCTRL)
			{
				index = rkeynum;
				if (index == CTRL_ESC)
				{
					STcopy(ERx("ESC"), c);
				}
				else if (index == CTRL_DEL)
				{
					STcopy(ERx("DEL"), c);
				}
				else
				{
					c[0] = (char) (rkeynum + 'A');
					c[1] = '\0';
				}
				STprintf(buf, ERx("Control-%s"), c);
			}
			else if (rcmdgrp == FDARROW)
			{
				index = MAX_CTRL_KEYS - 1;
				if (rcmdtype == ftUPARROW)
				{
					index += 1;
					STcopy(ERget(F_FT000A_UP_ARROW), buf);
				}
				else if (rcmdtype == ftDNARROW)
				{
					index += 2;
					STcopy(ERget(F_FT000B_DOWN_ARROW), buf);
				}
				else if (rcmdtype == ftRTARROW)
				{
					index += 3;
					STcopy(ERget(F_FT000C_RIGHT_ARROW),buf);
				}
				else
				{
					index += 4;
					STcopy(ERget(F_FT000D_LEFT_ARROW),buf);
				}
			}
			else
			{
				/*
				**  Dealing with a function key.
				*/
				TDnomacro(rkeynum - 1);
				index = rkeynum - 1 + MAX_CTRL_KEYS +
					CURSOROFFSET;
				STprintf(buf, ERx("PF%d"), rkeynum);
			}
			mptr = &(keymap[index]);
			if ((retval = FKlvlchk(mptr, level, errbuf, buf)) != OK)
			{
				break;
			}
			FKunmap(mptr);

			if (label == NULL || *label == '\0')
			{
				cp= FEtsalloc(IIFTltLabelTag, buf);
			}
			else
			{
				cp = FEtsalloc(IIFTltLabelTag, label);
			}

			if (lcmdgrp == FDCMD)
			{
				FKxtcmd(lcmdtype, &cmdval, &cmdmode);
				if (cmdval != fdopERR)
				{
					FKumapcmd(cmdval, level);
				}
			}
			else if (lcmdgrp == FDMENUITEM)
			{
				cmdval = lkeynum + ftMUOFFSET - 1;
				FKumapcmd(cmdval, level);
				index = lkeynum - 1;
				menumap[index] = cp;
				cmdmode = fdcmINBR;
			}
			else
			{
				cmdval = lkeynum + ftFRSOFFSET - 1;
				FKumapcmd(cmdval, level);
				cmdmode = fdcmINBR;
			}

			mptr->label = cp;
			mptr->who_def = (i1) level;
			mptr->cmdval = (i2) cmdval;
			mptr->cmdmode = (i1) cmdmode;
			break;

		case FDARROW:
			index = MAX_CTRL_KEYS - 1;
			if (lcmdtype == ftUPARROW)
			{
				index += 1;
			}
			else if (lcmdtype == ftDNARROW)
			{
				index += 2;
			}
			else if (lcmdtype == ftRTARROW)
			{
				index += 3;
			}
			else
			{
				index += 4;
			}
			mptr = &(keymap[index]);
			if (rcmdgrp != FDOFF)
			{
				STcopy(ERget(E_FT0033_BADRCMD), errbuf);
				retval = FAIL;
			}
			else
			{
				FKunmap(mptr);
				mptr->who_def = level;
				mptr->cmdval = (i2) ftKEYLOCKED;
				mptr->label = NULL;
				mptr->cmdmode = (i1) fdcmNULL;
			}
			break;

		case FDCTRL:
		case FDPFKEY:
			if (lcmdgrp == FDCTRL)
			{
				index = lkeynum;
				if (index == CTRL_ESC)
				{
					STcopy(ERx("ESC"), c);
				}
				else if (index == CTRL_DEL)
				{
					STcopy(ERx("DEL"), c);
				}
				else
				{
					c[0] = (char) (lkeynum + 'A');
					c[1] = '\0';
				}
				STprintf(buf, ERx("Control-%s"), c);
			}
			else
			{
				/*
				**  Dealing with a function key.
				*/
				index = lkeynum - 1 + MAX_CTRL_KEYS +
					CURSOROFFSET;
				STprintf(buf, ERx("PF%d"), lkeynum);
			}
			mptr = &(keymap[index]);

			if ((retval = FKlvlchk(mptr, level, errbuf, buf)) != OK)
			{
				break;
			}

			if (rcmdgrp != FDOFF)
			{
				STcopy(ERget(E_FT0033_BADRCMD), errbuf);
				retval = FAIL;
			}
			else
			{
				FKunmap(mptr);
				mptr->who_def = level;
				mptr->cmdval = (i2) ftKEYLOCKED;
				mptr->label = NULL;
				mptr->cmdmode = (i1) fdcmNULL;

				if (lcmdgrp == FDPFKEY)
				{
					TDnomacro(lkeynum - 1);
				}
			}
			break;

		default:
			STcopy(ERget(E_FT0031_BADLCMD), errbuf);
			retval = FAIL;
			break;
	}

# ifdef DATAVIEW
	if (IIMWimIsMws() && retval != FAIL)
		return(IIMWsmeSetMapEntry('k', (mptr - keymap)));
# endif	/* DATAVIEW */

	return(retval);
}




/*{
** Name:	IIFTwksWviewKeystrokeSetup - Set up keystroke for wview.
**
** Description:
**	Sends down keystroke string for certain FRS commands that
**	wview will send back to the FRS.  The commands are:
**	- menu key
**	- up one line
**	- down one line
**	- move one character right
**	- move one character left
**	- nextfield
**	- previousfield
**
**	If command is matched to a function (or arrow) key and the
**	terminal definition does not have function keys, then
**	an empty string is sent down for the command.  Also, if command
**	is not mapped, an empty string is sent down as well.  Note
**	that once a match is found, subsequent matches are ignored.
**	This allows shorter sequences (i.e., control sequences) to
**	have a higher precedence.
**
**	This needs to be called everytime one of the above is remapped
**	dynamically.
**
**	Interaction will DG terminal (and terminal emulators) still
**	needs to be investigated but this is not a real issue at the moment.
**
**	This only works for ASCII based character sets.  It will
**	NOT work on EBCDIC system without modifications.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	01/26/90 (dkh) - Initial version.
*/
VOID
IIFTwksWviewKeystrokeSetup()
{
	i4	i;
	i4	cmdval;
	i4	maxmap = MAX_CTRL_KEYS + KEYTBLSZ + 4;
	u_char	*menu_key = NULL;
	u_char	*up_key = NULL;
	u_char	*down_key = NULL;
	u_char	*right_key = NULL;
	u_char	*left_key = NULL;
	u_char	*next_key = NULL;
	u_char	*prev_key = NULL;
	u_char	*empty = (u_char *) ERx("");
	u_char	*cp;
	u_char	**pcp;
	u_char	buf_menu[30];
	u_char	buf_up[30];
	u_char	buf_down[30];
	u_char	buf_right[30];
	u_char	buf_left[30];
	u_char	buf_next[30];
	u_char	buf_prev[30];

	/*
	**  Find the mapping of the various commands.
	*/
	for (i = 0; i < maxmap; i++)
	{
		/*
		**  Disallow use of just the escape character if
		**  function keys are defined for the terminal
		**  definition.  This is necessary since most
		**  function keys are escape sequences.
		**
		**  This check allows us to sidestep a bunch
		**  of problems.
		*/
		if (i == CTRL_ESC && KY)
		{
			continue;
		}

		cmdval = keymap[i].cmdval;
		switch(cmdval)
		{
#if defined(PMFEWIN3)
			case ftFRS4:
				TEgokey(keymap[i].label);
				continue;

			case ftFRS7:
				TEfindkey(keymap[i].label);
				continue;
#endif
			case fdopMENU:
#if defined(PMFEWIN3)
				TEmenukey(keymap[i].label);
#endif
				if (menu_key != NULL)
				{
					continue;
				}
				pcp = &menu_key;
				menu_key = buf_menu;
				cp = menu_key;
				break;

			case fdopUP:
				if (up_key != NULL)
				{
					continue;
				}
				pcp = &up_key;
				up_key = buf_up;
				cp = up_key;
				break;

			case fdopDOWN:
				if (down_key != NULL)
				{
					continue;
				}
				pcp = &down_key;
				down_key = buf_down;
				cp = down_key;
				break;

			case fdopRIGHT:
				if (right_key != NULL)
				{
					continue;
				}
				pcp = &right_key;
				right_key = buf_right;
				cp = right_key;
				break;

			case fdopLEFT:
				if (left_key != NULL)
				{
					continue;
				}
				pcp = &left_key;
				left_key = buf_left;
				cp = left_key;
				break;

			case fdopNEXT:
				if (next_key != NULL)
				{
					continue;
				}
				pcp = &next_key;
				next_key = buf_next;
				cp = next_key;
				break;

			case fdopPREV:
				if (prev_key != NULL)
				{
					continue;
				}
				pcp = &prev_key;
				prev_key = buf_prev;
				cp = prev_key;
				break;

			default:
				continue;
		}
		if (i >= MAX_CTRL_KEYS)
		{
			if (KY)
			{
				/*
				**  Find escape sequence for this.
				*/
				IITDmfkMapFuncKeys(i - MAX_CTRL_KEYS, pcp);
			}
			else
			{
				/*
				**  No function keys for terminal.
				**  Just set to EOS.
				*/
				*cp = EOS;
			}
		}
		else
		{
			if (i == CTRL_ESC)
			{
				*cp++ = ESCAPE;
			}
			else if (i == CTRL_DEL)
			{
				*cp++ = DELETE;
			}
			else
			{
				*cp++ = i + 1;
			}
			*cp = EOS;
		}
	}

	/*
	**  Set empty string for commands that were not matched.
	*/
	if (menu_key == NULL)
	{
		menu_key = empty;
	}
	if (up_key == NULL)
	{
		up_key = empty;
	}
	if (down_key == NULL)
	{
		down_key = empty;
	}
	if (right_key == NULL)
	{
		right_key = empty;
	}
	if (left_key == NULL)
	{
		left_key = empty;
	}
	if (next_key == NULL)
	{
		next_key = empty;
	}
	if (prev_key == NULL)
	{
		prev_key = empty;
	}
	/*
	**  Send down command string.
	*/
	IITDwksWviewKeySet(menu_key, up_key, down_key, right_key, left_key,
		next_key, prev_key);
}

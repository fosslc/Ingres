/*
**  ftmaplbl.c
**
**  Routines to handle to map and label commands from set and
**  inquire statements.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**	Created - 08/30/85 (dkh)
**	85/09/07  20:51:50  dave
**		Initial revision
**	85/09/13  20:55:49  dave
**		Added support for inquire_frs frs label on frs commands
**		and frskeys. (dkh)
**	85/09/26  17:34:35  dave
**		Make set/inq label stuff work. (dkh)
**	85/09/26  20:26:43  dave
**		Integrated changes made on SPHINX. (dkh)
**	85/11/07  20:39:53  dave
**		Fixed so that labels are properly updated when a command is
**		remapped via set_frs statements. (dkh)
**	85/12/18  19:34:22  dave
**		Changed to use "Control" instead of "Control-" when inquiring
**		about the mapping of a command. (dkh)
**	85/12/24  21:38:34  dave
**		Fix for BUG 7440. (dkh)
**	85/12/24  22:29:26  dave
**		Fix for BUG 7575. (dkh)
**	86/01/02  14:51:10  dave
**		Fixed to correctly set map and label values when we are dealing
**		with frs commands. (dkh)
**	86/01/31  19:49:44  dave
**		Fix for BUG 7719. (dkh)
**	86/02/01  18:26:28  dave
**		Improved fix for BUG 7719. (dkh)
**	86/02/04  18:23:46  dave
**		Fix for BUG 8106. (dkh)
**	86/11/18  21:52:09  dave
**		Initial60 edit of files
**	86/11/21  07:44:33  joe
**		Lowercased FT*.h
**	28-jan-87 (bab)	Fix for bug 9976
**		Generate labels ESC and Delete rather than ControlESC
**		and ControlDEL (with or without a dash '-'.)
**	87/04/08  00:00:40  joe
**		Added compat, dbms and fe.h
**	05/02/87 (dkh) - Changed to call FD routines via passed pointers.
**	06/19/87 (dkh/bab) - Code cleanup.
**	08/14/87 (dkh) - ER changes.
**	08/25/87 (dkh) - Changed error numbers from 8000 series to local ones.
**	07-jan-88 (bab)
**		In FTsetlabel, only call MEfree if mptr->label not NULL.
**	01/21/88 (dkh) - Fixed SUN compilation problems.
**	10/28/88 (dkh) - Performance changes.
**	08/04/89 (dkh) - Added support for mapping arrow keys.
**	01/24/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	04/03/90 (dkh) - Integrated MACWS changes.
**	04/14/90 (dkh) - Changed "# if defined" to "# ifdef".
**	08/29/90 (dkh) - Integrated porting change ingres6202p/131520.
**	01-oct-96 (mcgem01)
**	    extern changed to GLOBALREF to quieten compiler on NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"ftframe.h"
# include	<mapping.h>
# include	<fsicnsts.h>
# include	<menu.h>		/* to get MAX_MENUITEMS */
# include	<st.h>
# include	<er.h>
# include	<erft.h>


FUNC_EXTERN	VOID	FTinqmap();
FUNC_EXTERN     VOID    IIFTwksWviewKeystrokeSetup();



struct mapping *
FTmptrget(cmd, index)
i4	cmd;
i4	*index;
{
	i4		i;
	i4		max_keys;
	struct	mapping *mptr;

	max_keys = MAX_CTRL_KEYS + KEYTBLSZ + 4;
	for (i = 0; i < max_keys; i++)
	{
		mptr = &(keymap[i]);
		if (mptr->cmdval == cmd)
		{
			*index = i;
			return(mptr);
		}
	}
	return(NULL);
}

FTfindlabel(cmd, buf)
i4	cmd;
char	*buf;
{
	char		*label;
	struct	mapping *mptr;
	i4		index;
	i4		frscmd;
	i4		frsmode;

	if (cmd >= ftCMDOFFSET && cmd < ftMUOFFSET)
	{
		FKxtcmd(cmd, &frscmd, &frsmode);
		if (frscmd == fdopERR)
		{
			STcopy(ERx(""), buf);
			return;
		}
	}
	else
	{
		frscmd = cmd;
	}
	mptr = FTmptrget(frscmd, &index);

	if (mptr == NULL)
	{
		STcopy(ERx(""), buf);
	}
	else if (mptr->label == NULL)
	{
		FTinqmap(cmd, buf);
	}
	else
	{
		label = mptr->label;
		STcopy(label, buf);
	}
}


FTnlabel(index, buf, dash)
i4	index;
char	*buf;
bool	dash;
{
	char	*cp;
	char	lbuf[40];

	if (index >= 0 && index < MAX_CTRL_KEYS)
	{
		if (index == MAX_CTRL_KEYS - 2)
		{
			STcopy(ERx("ESC"), buf);
		}
		else if (index == MAX_CTRL_KEYS - 1)
		{
			STcopy(ERx("Delete"), buf);
		}
		else
		{
			if (dash)
			{
				STcopy(ERx("Control-"), buf);
			}
			else
			{
				STcopy(ERx("Control"), buf);
			}
			lbuf[0] = index + 'A';
			lbuf[1] = '\0';
			STcat(buf, lbuf);
		}
	}
	else
	{
		index -= (MAX_CTRL_KEYS + CURSOROFFSET - 1);

		/*
		**  Fix for BUG 7440. (dkh)
		*/
		if (index < 1)
		{
			/*
			**  Return an empty string if we
			**  happen to be looking at the
			**  cursor keys.  This can happen
			**  if leftchar and rightchar are
			**  unmapped from control-H and
			**  control-L, respectively.  This
			**  leaves leftchar and rightchar
			**  mapped to the cursor keys only.
			*/

			/*
			**  Fix for BUG 7719. (dkh)
			**  Special casing for arrow keys.
			*/
			switch(index)
			{
				case -3:
					cp = ERget(F_FT000A_UP_ARROW);
					break;

				case -2:
					cp = ERget(F_FT000B_DOWN_ARROW);
					break;

				case -1:
					cp = ERget(F_FT000C_RIGHT_ARROW);
					break;

				case 0:
					cp = ERget(F_FT000D_LEFT_ARROW);
					break;

				default:
					cp = ERx("");
					break;
			}

			if (KY)
			{
				STcopy(cp, buf);
			}
			else
			{
				buf[0] = '\0';
			}
		}
		else
		{
			STprintf(buf, ERx("PF%d"), index);
		}
	}
}


VOID
FTinqmap(cmd, buf)
i4	cmd;
char	*buf;
{
	struct	mapping *mptr;
	i4		index;
	i4		frscmd;
	i4		frsmode;

	STcopy(ERx(""), buf);

	if (cmd >= ftCMDOFFSET && cmd < ftMUOFFSET)
	{
		FKxtcmd(cmd, &frscmd, &frsmode);
		if (frscmd == fdopERR)
		{
			return;
		}
	}
	else
	{
		frscmd = cmd;
	}

	mptr = FTmptrget(frscmd, &index);
	if (mptr != NULL)
	{
		FTnlabel(index, buf, FALSE);
	}
}


FTinqlabel(cmd, buf)
i4	cmd;
char	*buf;
{
	i4	index;
	char	*label;

	if (cmd >= ftMUOFFSET && cmd < ftFRSOFFSET)
	{
		index = cmd - ftMUOFFSET;
		if (index > MAX_MENUITEMS)
		{
			label = ERx("");
		}
		else if ((label = menumap[index]) == NULL)
		{
			label = ERx("");
		}
		STcopy(label, buf);
	}
	else
	{
		FTfindlabel(cmd, buf);
	}
}


FTsetmap(cmd, val)
i4	cmd;
i4	val;
{
	i4		index;
	i4		muindex;
	i4		cmdval;
	i4		ocmdval;
	i4		cmdmode;
	i4		dummy;
	i4		frscmd;
	i4		frsmode;
	i4		resend_keys = FALSE;
	char		*cp;
	struct	mapping *mptr;
	char		buf[40];

	if (cmd >= ftCMDOFFSET && cmd < ftMUOFFSET)
	{
		FKxtcmd(cmd, &cmdval, &cmdmode);
		switch(cmdval)
		{
			case fdopMENU:
			case fdopUP:
			case fdopDOWN:
			case fdopRIGHT:
			case fdopLEFT:
			case fdopNEXT:
			case fdopPREV:
				resend_keys = TRUE;
				break;

			default:
				break;
		}
	}
	else
	{
		/*
		**  Make sure we have a valid command before proceeding.
		*/

		if ((cmd < ftCMDOFFSET) || (cmd > ftMAXCMD) ||
			(cmd < ftFRSOFFSET && cmd > ftMUOFFSET + MAX_MENUITEMS))
		{
			return(FALSE);
		}
		cmdval = cmd;
		cmdmode = fdcmINBR;
	}
	if (val < FsiPF_OFST)
	{
		/*
		**  control-key
		*/

		/*
		**  Fix for BUG 7575. (dkh)
		*/
		if (val == FsiESC_CTRL)
		{
			GLOBALREF	char	*ZZA;

			if (KY || (ZZA != NULL && *ZZA == '\033'))
			{
				IIUGerr(E_FT0036_NOESC, 0, 0);
				return(FALSE);
			}
		}

		index = val - FsiA_CTRL;
		if (index > MAX_CTRL_KEYS)
		{
			return(FALSE);
		}
	}
	else
	{
		index = MAX_CTRL_KEYS - 1;
		if (val == FsiUPARROW)
		{
			index += 1;
		}
		else if (val == FsiDNARROW)
		{
			index += 2;
		}
		else if (val == FsiRTARROW)
		{
			index += 3;
		}
		else if (val == FsiLTARROW)
		{
			index += 4;
		}
		else
		{
			/*
			**  Fix for BUG 8106. (dkh)
			*/
			if (!KY)
			{
				IIUGerr(E_FT0037_NOPFK, 0, 0);
				return(FALSE);
			}
			index = val - FsiPF_OFST + MAX_CTRL_KEYS + CURSOROFFSET;
			if (index >= MAX_CTRL_KEYS + KEYTBLSZ + CURSOROFFSET)
			{
				return(FALSE);
			}
		}
	}

	if (cmd >= ftCMDOFFSET && cmd < ftMUOFFSET)
	{
		FKxtcmd(cmd, &frscmd, &frsmode);
	}
	else
	{
		frscmd = cmd;
	}
	if ((mptr = FTmptrget(frscmd, &dummy)) != NULL)
	{
		FKunmap(mptr);
		mptr->label = NULL;
		mptr->cmdval = ftNOTDEF;
		mptr->cmdmode = fdcmNULL;
		mptr->who_def = DEF_BY_EQUEL;

# ifdef	DATAVIEW
		IIMWrmeRstMapEntry('k', (mptr - keymap));
# endif	/* DATAVIEW */
	}

	mptr = &(keymap[index]);

	ocmdval = mptr->cmdval;
	if (ocmdval >= ftMUOFFSET && ocmdval < ftFRSOFFSET)
	{
		muindex = ocmdval - ftMUOFFSET;
		if (muindex < MAX_MENUITEMS)
		{
			menumap[muindex] = NULL;
		}
	}


	FTnlabel(index, buf, TRUE);
	cp = STalloc(buf);

	mptr->label = cp;
	mptr->cmdval = cmdval;
	mptr->cmdmode = cmdmode;
	mptr->who_def = DEF_BY_EQUEL;

	if (cmd >= ftMUOFFSET && cmd < ftFRSOFFSET)
	{
		muindex = cmd - ftMUOFFSET;
		if (muindex >= MAX_MENUITEMS)
		{
			return(FALSE);
		}
		menumap[muindex] = mptr->label;
	}

	/*
	**  Resend key mappings to wview if one of the commands
	**  we are interested in was remapped.
	*/
	if (resend_keys)
	{
		IIFTwksWviewKeystrokeSetup();
	}

# ifdef	DATAVIEW
	if (IIMWsmeSetMapEntry('k', (mptr - keymap)) == FAIL)
		return(FALSE);
# endif	/* DATAVIEW */

	return(TRUE);
}


FTsetlabel(cmd, label)
i4	cmd;
char	*label;
{
	i4		muindex;
	i4		mpindex;
	i4		frscmd;
	i4		frsmode;
	struct	mapping *mptr;

	if (cmd >= ftCMDOFFSET && cmd < ftMUOFFSET)
	{
		FKxtcmd(cmd, &frscmd, &frsmode);
	}
	else
	{
		frscmd = cmd;
	}

	/*
	**  Call to FTmptrget will ensure that a command
	**  menuitem or frskey does exist or has been
	**  mapped before proceeding.  If a menu position
	**  or frs key does not exist, then no label
	**  gets set.
	*/
	if ((mptr = FTmptrget(frscmd, &mpindex)) == NULL)
	{
		return(FALSE);
	}

	/*
	**  Only free label if it was from a STalloc.
	**  mptr->who_def tells us this since DEF_BY_EQUEL
	**  is the only one who does an STalloc.
	*/
	if (mptr->who_def == DEF_BY_EQUEL && mptr->label)
	{
		MEfree(mptr->label);
	}
	mptr->label = STalloc(label);
	mptr->who_def = DEF_BY_EQUEL;
	if (cmd >= ftMUOFFSET && cmd < ftFRSOFFSET)
	{
		muindex = cmd - ftMUOFFSET;
		menumap[muindex] = mptr->label;
	}
	return(TRUE);
}

/*
**	Windex command string interface library.
**
**	Copyright (c) 2004 Ingres Corporation
**
**	windex.c
**
**	description:
**		Handle all the windex command strings to tell a windowing
**		system how to use the mouse.
**
**		The window system windex enable is done through
**		the termcap entry boolean operator WS.
**
**	Parameters:
**
**	Side effects:
**
**	Error messages:
**		None needed, if a failure occurs this special mode
**		will be disabled.
**
**	Calls:
**
**	History:
**		11/21/88 (brett) - Created windex.c
**		12/21/88 (brett)
**			Moved IITDfixvsve() from crtty.c to windex.c
**		12/29/88 (brett)
**			Added comments and brought up to CL code specs.
**		01/31/89 (brett)
**			Changed format of the command string, this will
**			help for future expansion if needed.  The new
**			format is ESC ESC OPERATOR [arg] ESC, where
**			OPERATOR can be any character except 0-9.  The
**			operators 0-9 are reserved for future expansion.
**		06/02/89 (brett)
**			Added command strings for hot spots.  Changed
**			the format of the simple field command string.
**		06/08/89 (brett)
**			Added the simple field title command string
**			interface IITDfldtitle().
**		06/12/89 (brett)
**			Added the intrp parameter for field activation
**			information in IITDfield().
**		06/27/89 (brett)
**			Added SIflush(stdout) to all the command strings
**			that could require it.  I think Vifred has a problem
**			with flushing stdout sometimes.
**	07/12/89 (dkh) - Integrated windex code changes from Brett into 6.2.
**		08/08/89 (brett)
**			Changed the format of the new screen command
**			string.  Popup windows use coordinates relative
**			to the local frame of refrence.  It became
**			necessary to pass the frame location.  The size
**			was also included to stream line the mouse selection
**			process.  Added a version string to VS for windex.
**			The present version is 1.0 verse no version at all.
**		08/08/89 (brett)
**			Added a debugging index.  If you send the debugging
**			prefix with a ascii digit debugging will be turned
**			on.  The default level is 1.  The max is 9.
**			PREFIX DEBUGGING arg SUFFIX. arg is '0' - '9'.
**			'0' turns the debugging off.
**		09/15/89 (brett)
**			Added a precedence for hot spots.  This helps
**			when a hot spot overlays an ingres form.
**	09/21/89 (dkh) - Porting changes integration.
**	09/22/89 (dkh) - More porting changes.
**	09/23/89 (dkh) - Yet more porting changes.
**	09/23/89 (dkh) - Even more porting changes.
**	12/27/89 (dkh) - Added support for hot spot trim.
**	01/23/90 (dkh) - Added support for sending wivew various frs command
**			 and keystroke mappings.
**	01/26/90 (brett) - Changed interface to IITDnwscr() to include frame
**			   flags.  This fixes problems with borderless popups.
**	01/31/90 (dkh) - Fixed IITDfixvsve() so that it doesn't corrupt
**			 ME on VMS.
**	02/01/90 (dkh) - Changed to use TE instead of SI due to problems
**			 running from VMS to UNIX.
**	03/04/90 (dkh) - Integrated 6.3 portion of brett's 01/26/90 change
**			 (ingres61b3ug/2335).
**	08/28/90 (dkh) - Integrated change ingres6202p/131215.
**	01/24/91 (dkh) - Fixed typo in IITDwksWviewKeySet() to pass
**			 a buffer as the first parameter to STprintf().
**	29-oct-93 (swm)
**		Removed incorrect, truncating cast of *vs pointer to i4
**		passed to MEfree.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include       <st.h>
# include       <si.h>
# include       <windex.h>
# include	<termdr.h>
# include	<te.h>
# include	<er.h>

# define	ESC	033

GLOBALREF	i4	windex_flag;
GLOBALREF	i4	IITDwvWVVersion;

/*{
** Name:	IITDfixvsve - Fix VS and VE
**
** Description:
**	Append the VS and VE termcaps with the windex command
**	strings.  Then set the windex flag accordingly.
**
** Inputs:
**	Pointers to the VS and VE termcap strings.
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
**	Windex command string mode is enabled if things succeed.
**	Append ESC ESC A ESC on VS, append ESC ESC Z ESC on VE.
**
** History:
**	12/21/88 (brett) - Initial version.
**	08/08/89 (brett) - Added a version string to the startup command
**			string.  Made the memory allocation for VS and
**			VE more general.
**	29-oct-93 (swm)
**		Removed incorrect, truncating cast of *vs pointer to i4
**		passed to MEfree.
*/
VOID
IITDfixvsve(vs, ve)
char **vs, **ve;
{
        /* Temp buffers for adjusting VS and VE */
        char    *VSsav, *VEsav, *buf;
	char	tmp[16];
	reg	i4    len, tmplen;

	/* We may have to put these back if things fail */
	VSsav = *vs;
	VEsav = *ve;

	STprintf(tmp, ERx("%s%c(%s)%c"), PREFIX, TURNON, WVERSION, SUFFIX);
	tmplen = STlength(tmp);

	/*
	** Allocate memory and reassign VS and VE.
	** If anything fails, backup to the normal state and
	** disable windex operation.
	*/
	if (*vs)
		len = STlength(*vs) + tmplen;
	else
		len = tmplen;
	if ((buf = (char *) MEreqmem( (u_i4)0, (u_i4)len + 1, FALSE,
		(STATUS *) NULL)) == NULL)
		{
			windex_flag = 0;
			return;
		}
	if (*vs)
		*vs = STprintf(buf, ERx("%s%s"), *vs, tmp);
	else
		*vs = STprintf(buf, ERx("%s"), tmp);

	STprintf(tmp, ERx("%s%c%c"), PREFIX, TURNOFF, SUFFIX);
	tmplen = STlength(tmp);

	if (*ve)
		len = STlength(*ve) + tmplen;
	else
		len = tmplen;
	if ((buf = (char *) MEreqmem( (u_i4)0, (u_i4)len + 1, FALSE,
		(STATUS *) NULL)) == NULL)
	{
			MEfree( (*vs) );
			*vs = VSsav;
			windex_flag = 0;
			return;
	}
	if (*ve)
		*ve = STprintf(buf, ERx("%s%s"), *ve, tmp);
	else
		*ve = STprintf(buf, ERx("%s"), tmp);

	/* Everything worked so its OK to free the old VS and VE */
	/*
	**  Can't free the original VS and VE string since they point
	**  into middle of a static string.
	MEfree( (i4)VSsav );
	MEfree( (i4)VEsav );
	*/

	windex_flag = 1;
	return;
}

/*{
** Name:	IITDenscrn - Enable screen
**
** Description:
**	Enable screen field selection in the window system.
**	ESC ESC E ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex enable screen command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDenscrn()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, ENSCRN, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDdsscrn - Disable screen
**
** Description:
**	Disable Screen field selection in the window system.
**	ESC ESC D ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex disable screen command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDdsscrn()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, DSSCRN, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDgvmode - Give frame mode
**
** Description:
**	Send frame mode command string to the window system.
**	ESC ESC Q mode ESC, where mode is a numeric ascii string.
**
** Inputs:
**	Mode of the present frame - fdmdUPD, fdmdADD, fdmdFILL, fdmdBROWSE
**
** Outputs:
**	Windex give frame mode command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDgvmode(mode)
i4	mode;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%d%c"), PREFIX, GVMODE, mode, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDxoff - X screen offset
**
** Description:
**	Send screen X offset to the window system.
**	ESC ESC X offset ESC, where offset is a numeric ascii string.
**
** Inputs:
**	None.
**
** Outputs:
**	Windex x offset command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDxoff(offset)
i4	offset;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%d%c"), PREFIX, XOFF, offset, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDyoff - Y screen offset
**
** Description:
**	Send screen Y offset to the window system.
**	ESC ESC Y offset ESC, where offset is a numeric ascii string.
**
** Inputs:
**	None.
**
** Outputs:
**	Windex y offset command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDyoff(offset)
i4	offset;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%d%c"), PREFIX, YOFF, offset, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDnwscrn - New screen
**
** Description:
**	Start a new screen structure in the window system.
**	ESC ESC S key(xstart,ystart,xmax,ymax) ESC, where key
**	  is a character string and addr is a numeric ascii string.
**
** Inputs:
**	Key is the frame name and addr the address of the structure.
**	This gives a unique key to store the screen structure in
**	the window system.
**	
**
** Outputs:
**	Windex new screen command string.
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
**	11/21/88 (brett) - Initial version.
*/
VOID
IITDnwscrn(key, xstart, ystart, xmax, ymax, flags, flags2)
char	*key;
i4	xstart;
i4	ystart;
i4	xmax;
i4	ymax;
i4	flags;
i4	flags2;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%s(%d,%d,%d,%d,%d,%d)%c"), PREFIX,
			NWSCRN, key, xstart, ystart, xmax, ymax,
			flags, flags2, SUFFIX);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDedscrn - End screen
**
** Description:
**	End a screen structure in the window system.
**	ESC ESC s ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex end screen command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDedscrn()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, EDSCRN, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDnwrsec - New regular section
**
** Description:
**	Start a regular section structure in the window system.
**	ESC ESC R addr ESC, where addr is a nat.
**
** Inputs:
**	Addr is the regular field's structure address.
**
** Outputs:
**	Windex new regular section command string.
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
**	11/21/88 (brett) - Initial version.
*/
VOID
IITDnwrsec(addr)
PTR	addr;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c0x%x%c"), PREFIX, NWRSEC, addr, SUFFIX);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDedrsec - End regular section
**
** Description:
**	End a regular section structure in the window system.
**	ESC ESC r ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex end regular section command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDedrsec()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, EDRSEC, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDnwtbl - New table section
**
** Description:
**	Start a table section structure in the window system.
**	ESC ESC T (seq)addr ESC, where seq is a numeric ascii
**	string and key is a character string.
**
** Inputs:
**	Key is the table's structure address and seq is it's
**	sequence number.  Key is used as a storage reference.
**
** Outputs:
**	Windex new table command string.
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
**	11/21/88 (brett) - Initial version.
*/
VOID
IITDnwtbl(seq, addr)
i4	seq;
PTR	addr;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(%d)0x%x%c"), PREFIX, NWTBL, seq,
			addr, SUFFIX);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDedtbl - End table section
**
** Description:
**	End a table section structure in the window system.
**	ESC ESC t ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex end table command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDedtbl()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, EDTBL, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDfield - Field descriptor
**
** Description:
**	Enter a field into a screen structure in the window system.
**	ESC ESC F(x,y,xLen,yLen,seq,flags,flags2,intrp) ESC,
**	where all values are numeric ascii strings.
**
** Inputs:
**	x, y - are coordinates on the screen,
**	xLen - is the length of the field in the x direction,
**	yLen - is the length of the field in the y direction,
**	seq - is the sequence of the field,
**	flags - is the flag word of the field,
**	flags2 - more flags for the field.
**	intrp - activation information, 0 if no activation.
**
** Outputs:
**	Windex field command string.
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
**	11/21/88 (brett) - Initial version.
**	06/12/89 (brett) - added intrp for field activation.
**	08/08/89 (brett) - pass flags to windex as unsigned for clarity.
*/
VOID
IITDfield(x, y, xLen, yLen, seq, flags, flags2, intrp)
i4	x;
i4	y;
i4	xLen;
i4	yLen;
i4	seq;
i4	flags;
i4	flags2;
i4	intrp;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(%d,%d,%d,%d,%d,%d,%d,%d)%c"), PREFIX,
			FIELD, x, y, xLen, yLen, seq, flags, flags2, intrp,
			SUFFIX);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDfldtitle - Field title descriptor
**
** Description:
**	Enter a field title into a screen structure in the window system.
**	ESC ESC B(x,y,title) ESC, where x and y are numeric ascii strings.
**		Title is an ascii text string.
**	Note: This is only valid for simple fields, it will be ignored
**		if used for a table field.
**
** Inputs:
**	x, y - are coordinates on the screen,
**	title - ascii string, title of the simple field.
**
** Outputs:
**	Windex field command string.
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
**	06/08/89 (brett) - Initial version.
*/
VOID
IITDfldtitle(x, y, title)
i4  x;
i4  y;
char *title;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(%d,%d,%s)%c"), PREFIX, FDTITLE, x, y,
			title, SUFFIX);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDnwpdmenu - New pull down menu
**
** Description:
**	Give a pull down menu to the window system.
**	ESC ESC P item1(FuncKey1) item2(FuncKey2) ... itemN(FuncKey3) ESC,
**
**	Because of the way the menu is stored there are three routines
**	used to do the extended termcap.  One to start the command
**	string.  The second for menu items and the third for ending
**	the command string.
**
**	Routine one, start a new pull down menu.
**	ESC ESC P
**
** Inputs:
**	None.
**
** Outputs:
**	Windex new pull down menu command string.
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
**	11/21/88 (brett) - Initial version.
*/
VOID
IITDnwpdmenu()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c"), PREFIX, PDMENU);
		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDpdmenuitem - Pull down menu item
**
** Description:
**	Give a pull down menu to the window system.
**	ESC ESC P item1(FuncKey1) item2(FuncKey2) ... itemN(FuncKey3) ESC
**
**	Because of the way the menu is stored there are three routines
**	used to do the extended termcap.  One to start the command
**	string.  The second for menu items and the third for ending
**	the command string.
**
**	Routine two, send a pull down menu item to the window system.
**	item1(FuncKey1) item2(FuncKey2) ... item3(FuncKey3), where
**	items and FuncKeys are character strings.
**
** Inputs:
**	item     - menu line item.
**	func_key - function key numonic for the menu item, if it exists.
**
** Outputs:
**	Windex pull down menu item command string.
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
**	11/21/88 (brett) - Initial version.
*/
VOID
IITDpdmenuitem(item, func_key)
char	*item;
char	*func_key;
{
	char	buf[200];

	if (windex_flag)
	{
		if (func_key != NULL)
			STprintf(buf, ERx("%s(%s) "), item, func_key);
		else
			STprintf(buf, ERx("%s() "), item);

		TEwrite(buf, STlength(buf));
	}

	return;
}

/*{
** Name:	IITDedpdmenu - End pull down menu
**
** Description:
**	Give a pull down menu to the window system.
**	ESC ESC P item1(FuncKey1) item2(FuncKey2) ... itemN(FuncKey3) ESC
**
**	Because of the way the menu is stored there are three routines
**	used to do the extended termcap.  One to start the command
**	string.  The second for menu items and the third for ending
**	the command string.
**
**	Routine three, end the pull down menu.
**	ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex end pull down menu command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDedpdmenu()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%c"), SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDmnline - Menu line
**
** Description:
**	Give a menu line to the window system.
**	ESC ESC L (x,y)menuline ESC, where x and y are numeric ascii
**	strings and menuline is a character string.
**
** Inputs:
**	x, y     - x and y coordinates for the menu line on the screen
**	           in a numeric ascii string.
**	menuline - the actual menu line on the screen.
**
** Outputs:
**	Windex menu line command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDmnline(x, y, line)
i4	x;
i4	y;
char	*line;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(%d,%d)%s%c"), PREFIX, MNLINE, x, y,
			line, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDenmenus - Enable menus
**
** Description:
**	Enable Pull Down Menus and the Menu Line mouse selection
**	in the window system.
**	ESC ESC M ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex enable menus command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDenmenus()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, ENMENUS, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDdsmenus - Disable menus
**
** Description:
**	Disable Pull Down Menus and the Menu Line mouse selection
**	in the window system.
**	ESC ESC m ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex disable menus command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDdsmenus()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, DSMENUS, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDenvifred - Enable vifred
**
** Description:
**	Enable the special vifred screen handling mode in the
**	window system.
**	ESC ESC V ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex enable vifred command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDenvifred()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, ENVIFRED, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDdsvifred - Disable vifred
**
** Description:
**	Disable the special vifred screen handling in the
**	window system.
**	ESC ESC v ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex disable vifred command string.
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
**	11/21/88 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDdsvifred()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, DSVIFRED, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}


/*{
** Name:	IITDfalsestop - False Stop mode
**
** Description:
**	False stop mode.  Used when ingres is going to give control
**	to another process for a limited amount of time.  When this
**	happens the VE termcap will be received by windex and it will
**	only push it's state instead of shuting down.  When ingres
**	regains control windex will receive the VS termcap which will
**	pop the state and return things to normal.
**	ESC ESC f ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex false stop command string.
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
**	01/31/89 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDfalsestop()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, FALSESTOP, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDhotspot - add a hot spot
**
** Description:
**	ESC ESC H(x,y,xlen,ylen,code,precedence) ESC
**	where x, y, xlen, ylen and precedence are numeric ascii
**	strings, and code is an alphanumeric ascii string.
**
** Inputs:
**	None.
**
** Outputs:
**	Windex hot spot command string.
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
**	06/01/89 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDhotspot(x,y,xlen,ylen,code,precedence)
i4  x;
i4  y;
i4  xlen;
i4  ylen;
char *code;
i4  precedence;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(%d,%d,%d,%d,%s,%d)%c"), PREFIX, HOTSPOT,
			x, y, xlen, ylen, code, precedence, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}

/*{
** Name:	IITDkillspots - kill all hot spots
**
** Description:
**	ESC ESC K ESC
**
** Inputs:
**	None.
**
** Outputs:
**	Windex kill spots command string.
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
**	06/01/89 (brett) - Initial version.
**	06/27/89 (brett) - Added SIflush(stdout).
*/
VOID
IITDkillspots()
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c%c"), PREFIX, KILLSPOTS, SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}

	return;
}


/*{
** Name:	IITDhsHotspotString - Generate hot spot command string.
**
** Description:
**	This routine generates the command string (i.e., string to be
**	returned by wview when the trim is selected with a mouse
**	click event) for a trim that is designated as a hot spot.
**	The key to the command string is the sequence number of
**	the piece of trim.
**
**	We assume that the passed in buffer is large enough.
**
** Inputs:
**	seq		Sequence number of a hot spot trim.
**
** Outputs:
**	cmd_str		Command string for hot spot trim.
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
**	12/27/89 (dkh) - Initial version.
*/
VOID
IITDhsHotspotString(seq, cmd_str)
i4	seq;
char	*cmd_str;
{
	STprintf(cmd_str, ERx("%s%c%d%c"), INPRFIX, HOTSPOT, seq, INSUFIX);
}



/*{
** Name:	IITDwksWviewKeySet - Set keystroke mappings for wview.
**
** Description:
**	Sends down the keystroke mappings for various commands to wview.
**	An empty string is passed down for a command that has no keystroke
**	mapping.
**
** Inputs:
**	menu	- key stroke sequence for menu command.
**	up	- key stroke sequence for up command.
**	down	- key stroke sequence for down command.
**	right	- key stroke sequence for right command.
**	left	- key stroke sequence for left command.
**	next	- key stroke sequence for next command.
**	prev	- key stroke sequence for prev command.
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
**	None.
**
** History:
**	01/23/90 (dkh) - Initial version.
*/
VOID
IITDwksWviewKeySet(menu, up, down, right, left, next, prev)
u_char	*menu;
u_char	*up;
u_char	*down;
u_char	*right;
u_char	*left;
u_char	*next;
u_char	*prev;
{
	char	buf[200];

	if (windex_flag)
	{
		STprintf(buf, ERx("%s%c(`%s`,`%s`,`%s`,`%s`,`%s`,`%s`,`%s`)%c"),
			PREFIX, SETKEYS, menu, up, down, right, left,
			next, prev,SUFFIX);
		TEwrite(buf, STlength(buf));
		TEflush();
	}
}

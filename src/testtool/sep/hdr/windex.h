/*
**	WINDEX -
**		Handle all the extended termcaps to tell a windowing
**		system how to use the mouse.
**
**		The window manager windex enable is done through
**		the VS command string.  The VE command string disables
**		the windex mode.
**		(enable ESC ESC A ESC, disable ESC ESC Z ESC)
**
**		These are the definitions of the command strings.
**
**	Parameters:
**
**	Side effects:
**		Note: Although the prefix for the command string is
**		defined some terminal emulators can make it very hard
**		to use the macro for flexability.  Xterm is one of
**		those emulators, so the string for PREFIX had to be
**		hardwired into Xterm.
**
**	Error messages:
**
**	Calls:
**
**	History:
**		Created by brett, Mon Nov 21 17:12:24 PST 1988
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    
*/

/*
**	static	char	*Sccsid = ERx("%W%	%G%");
*/

/*
**  The following are all the command strings for the windex code.
*/
# define	PREFIX	ERx("\033\033")	/* prefix for command strings */
# define	TURNON	'A'		/* turn on the windex mode */
# define	TURNOFF	'Z'		/* turn off the windex mode */
# define	ENSCRN	'E'		/* enable screen and menus */
# define	DSSCRN	'D'		/* disable screen and menus */
# define	GVMODE	'Q'		/* frame mode */
# define	XOFF	'X'		/* x screen offset */
# define	YOFF	'Y'		/* y screen offset */
# define	NWSCRN	'S'		/* new screen */
# define	EDSCRN	's'		/* end screen */
# define	NWRSEC	'R'		/* new regular section */
# define	EDRSEC	'r'		/* end regular section */
# define	NWTBL	'T'		/* new table section */
# define	EDTBL	't'		/* end table section */
# define	FIELD	'F'		/* data field */
# define	PDMENU	'P'		/* pull down menu */
# define	MNLINE	'L'		/* menu line */
# define	ENMENUS	'M'		/* enable menus */
# define	DSMENUS	'm'		/* disable menus */
# define	ENVIFRED	'V'	/* enable vifred mode */
# define	DSVIFRED	'v'	/* disable vifred mode */
# define	FALSESTOP	'f'	/* false stop mode */
# define	SUFFIX	'\033'		/* command str termination character */


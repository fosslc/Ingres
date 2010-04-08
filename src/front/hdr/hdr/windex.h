/*
**	Copyright (c) 2004 Ingres Corporation
*/
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
**			Added the simple field title command string.
**		06/12/89 (brett)
**			Added the intrp parameter for field activation
**			information in IITDfield().
**	07/12/89 (dkh) - Integrated windex code changes from Brett into 6.2.
**		08/08/89 (brett)
**			Changed the format of the new screen command
**			string.  Popup windows use coordinates relative
**			to the local frame of reference.  It became
**			necessary to pass the frame location.  The size
**			was also included to stream line the mouse selection
**			process.  Added a version string to VS for windex.
**			Changed field command string to pass flags as unsigned.
**			The present version is 1.0 verse no version at all.
**		08/08/89 (brett)
**			Added a debugging index.  If you send the debugging
**			prefix with a ascii digit debugging will be turned
**			on.  The default level is 1.  The max is 9.
**			PREFIX DUBUGGING arg SUFFIX, arg is '0' - '9'.
**			'0' turns the debugging off.
**		08/16/89 (brett)
**			Changed the version number to match the ingres
**			release number.  This number must be an integer.
**	09/22/89 (dkh) - Porting changes integration.
**	12/01/89 (dkh) - Added support for GOTOs.
**	01/24/90 (dkh) - Added definition of SETKEYS for sending key
**			 mappings to wview.
**		01/26/90 (brett)
**			Added goto enhancements and report back from
**			Dave Hung.  Added new command string used for
**			function keys.  This removes map file compat
**			problem in INGRES 64.
**		01/27/90 (brett)
**			Added INGRES 64 compatibilty, func key command
**			string.  Wnat WindowView to have 64 functionality,
**			it won't use it unless you are running 64.
**	09/29/90 (dkh) - Merged ingres6202p and r64 versions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**	static	char	*Sccsid = "%W%	%G%";
*/

/*
**  The following are all the command strings for the windex code.
*/
# define	WVERSION	"64"	/* version string for compatibility */
# define	PREFIX	"\033\033"	/* prefix for command strings */
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
# define	FDTITLE	'B'		/* title for a simple field */
# define	PDMENU	'P'		/* pull down menu */
# define	MNLINE	'L'		/* menu line */
# define	ENMENUS	'M'		/* enable menus */
# define	DSMENUS	'm'		/* disable menus */
# define	ENVIFRED	'V'	/* enable vifred mode */
# define	DSVIFRED	'v'	/* disable vifred mode */
# define	FALSESTOP	'f'	/* false stop mode */
# define	HOTSPOT		'H'	/* add a mouse hot rectangle */
# define	KILLSPOTS	'K'	/* remove all mouse hot rectangles */
# define	DEBUGGING	'?'	/* put windex into debugging mode */

/*
**  New constatnts for r64.
*/
# define	SETKEYS	'a'		/* set key mappings in wview */
# define	MU_FAST	'I'		/* menuitem fast path command */
# define	GOTOFLD	'G'		/* goto a field fast path cmd */
# define	SUFFIX	'\033'		/* command str termination character */
# define	INPRFIX	"\035\035"	/* input prefix */
# define	INSUFIX	'\035'		/* input suffix */
# define	RVERSION	"64"	/* version string for compatibility */

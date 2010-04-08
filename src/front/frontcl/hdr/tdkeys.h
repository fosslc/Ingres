/*
**	static	char	*Sccsid = "@(#)tdkeys.h	30.3	1/1/85";
*/

/*
** TDKEYS.H
**
** Definitions and delcarations to handle cursor and function keys.
**
** Copyright (c) 2004 Ingres Corporation
**
** History:
** written: 12-8-82 dkh
**	08/03/89 (dkh) - Added support for mapping arrow keys.
**	01/11/90 (esd) - Modifications for FT3270.
**	08/19/90 (esd) - Modifications for hp9_mpe within FT3270.
**      24-sep-96 (mcgem01)
**              externs changed to GLOBALREFS
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	INTERACTIVE	0
# define	AFILE		1
# define	LINESIZE		150

# ifndef EBCDIC
# define	FIRSTPRINTABLE		'\040'
# define	LASTPRINTABLE		'\177'
# define	KBUFSIZE		256

# define	MAX_8B_ASCII		256
# define	FKEY_LEADIN		037
# define	MENU_KEY		0377

# define	UPKEY			257
# define	DOWNKEY			258
# define	RIGHTKEY		259
# define	LEFTKEY			260
/*# define	HOMEKEY			0204	*/
# define	F0KEY			261
# define	F1KEY			262
# define	F2KEY			263
# define	F3KEY			264
# define	F4KEY			265
# define	F5KEY			266
# define	F6KEY			267
# define	F7KEY			268
# define	F8KEY			269
# define	F9KEY			270
# define	F10KEY			271
# define	F11KEY			272
# define	F12KEY			273
# define	F13KEY			274
# define	F14KEY			275
# define	F15KEY			276
# define	F16KEY			277
# define	F17KEY			278
# define	NULLKEY			0xffff
# define	KEYOFFSET		257

# define	MENUKEY			1
# define	DEFKEY			4
# define	MENUKEY_1		0
# define	DEFKEY_1		3

#else
# define	FIRSTPRINTABLE		0x40
# define	LASTPRINTABLE		0xf9
# define	KBUFSIZE		256

# define	MAX_8B_ASCII		256
# define	FKEY_LEADIN		037
# define	MENU_KEY		0200

# define	UPKEY		  0x20
# define	DOWNKEY		  0x21
# define	RIGHTKEY	  0x22
# define	LEFTKEY		  0x23
# define	F0KEY		  0x24
# define	F1KEY		  0x15
# define	F2KEY		  0x06
# define	F3KEY		  0x17
# define	F4KEY		  0x28
# define	F5KEY		  0x29
# define	F6KEY		  0x2a
# define	F7KEY		  0x2b
# define	F8KEY		  0x2c
# define	F9KEY		  0x09
# define	F10KEY		  0x0a
# define	F11KEY		  0x1b
# define	F12KEY		  0x30
# define	F13KEY		  0x31
# define	F14KEY		  0x1a
# define	F15KEY		  0x33
# define	F16KEY		  0x34
# define	F17KEY		  0x35
# define	NULLKEY			0377
# define	KEYOFFSET		0200

# define	MENUKEY			1
# define	DEFKEY			4
# define	MENUKEY_1		0
# define	DEFKEY_1		3
#endif /* EBCDIC */

# ifdef  FT3270
# ifdef  hp9_mpe
# define	KEYTBLSZ		9
# else   /* ~hp9_mpe */
# define	KEYTBLSZ		25
# endif  /* hp9_mpe */
# else   /* ~FT3270 */
# define	KEYTBLSZ		40
# endif  /* FT3270 */

# ifdef FT3270
# define	CURSOROFFSET		0
# else  /* ~FT3270 */
# define	CURSOROFFSET		4
# endif /* FT3270 */


# define	FOUND			0
# define	NOTFOUND		1
# define	ALLCOMMENT		2

# define	FDUNKNOWN		0
# define	FDCMD			1
# define	FDFRSK			2
# define	FDMENUITEM		3
# define	FDCTRL			4
# define	FDPFKEY			5
# define	FDOFF			6
# define	FDGOODLBL		7
# define	FDNOLBL			8
# define	FDBADLBL		9
# define	FDARROW			10

GLOBALREF	u_char	*KEYSEQ[], *KEYDEFS[];

GLOBALREF	char	INBUF[];

GLOBALREF	u_char	*KEYNULLDEF;

GLOBALREF	i4	MAXFKEYS;

GLOBALREF	i4	FKEYS;

GLOBALREF	i4	MENUCHAR;

FUNC_EXTERN	i4	FKmapkeys();

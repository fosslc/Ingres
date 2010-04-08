/*
**  ftdefs.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	09/23/89 (dkh) - Porting changes integration.
**	13-mar-90 (bruceb)
**		Additions made for locator support.
**	08/18/90 (dkh) - Fixed bug 31360.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  Structure for holding information about user's terminal.
*/

typedef struct fttrminfo
{
	char	*name;		/* name of terminal */
	i4	cols;		/* number of physical columns */
	i4	lines;		/* number of physical lines */
} FT_TERMINFO;


/*
**  Maximum supported terminal size (both lines and columns).
*/
# define	MAX_TERM_SIZE	1000

/*
**  Arguments to FTfkset
*/

# define	FT_FUNCKEY	(i4) 0	/* set keypad to function key mode */
# define	FT_NUMERIC	(i4) 1	/* set keypad to numeric mode */


/*
**  Arguments to FTrestore
*/

# define	FT_FORMS	(i4) 0	/* go to forms mode */
# define	FT_NORMAL	(i4) 1	/* go to line mode */


/*
**  Arguments to FTfmt
*/

# define	FT_DISPONLY	(i4) 0	/* field is a display only field */
# define	FT_UPDATE	(i4) 1	/* field is updateable */

/*
**  Agruments to FTmarkfield
*/

# define	FT_REGFLD	(i4) 0	/* field is a regular field */
# define	FT_TBLFLD	(i4) 1	/* field is a table field */

/*
**  Arguments to FTuserinp (which is an internal FT routine).
*/

# define	FT_MENUIN	(i4) 0	/* menu input */
# define	FT_PROMPTIN	(i4) 1	/* prompt input */

/*}
** Name:  IICOORD - x,y coordinates to describe a rectangular object space.
**
** Description:
**	This structure contains the 0-indexed x and y coordinates for the
**	top left and bottom right corners of an object.  This is used in
**	conjunction with locator support.
**
** History:
**	13-mar-90 (bruceb)	Written.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed struct name from COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
*/
typedef struct coordinates
{
    i4		begy;	/* Top left corner. */
    i4		begx;
    i4		endy;	/* Bottom right corner. */
    i4		endx;
    struct coordinates	*next;	/* Used only for free list. */
} IICOORD;

/*}
** Name:  LOC	- Describes a field, table field cell or hotspot trim.
**
** Description:
**	Provides information about an object on the form for use with
**	locator support.  If a simple field or a hotspot, row_number and
**	col_number are unused.  If the object overlaps N lines, only one
**	IICOORD struct will be used, but N LOC structs will exist.
**
** History:
**	19-mar-90 (bruceb)	Written.
**	05-jun-92 (leighb) DeskTop Porting Change:
**		Changed COORD to IICOORD to avoid conflict
**		with a structure in wincon.h for Windows/NT
*/
typedef struct locator
{
    IICOORD	*obj;		/* Point to coordinates of this object. */
    i4		objtype;	/* One of FREGULAR, FTABLE or FHOTSPOT. */
    i4		sequence;	/* Field or trim sequence number. */
    i4		row_number;	/* Table field row number. */
    i4		col_number;	/* Table field column number. */
    struct locator	*next;	/* Other locations on this line. */
} LOC;

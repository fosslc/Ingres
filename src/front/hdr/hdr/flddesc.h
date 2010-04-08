/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	flddesc.h	- Define FLDDESC type
**
** Description:
**	FLDDESC describes a form field (or column) to be displayed
**
** History:
**	7/20/89 (Mike S)	Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Add prototypes for gcc 4.3
**/

/* Need frame.h if not included already */

#include	<frame.h>

/*}
** Name:	FLDDESC - field (or column) to display
**
** Description:
**	Information needed to construct a default version of the field 
**	(or column).
**
**	A field (or column) is created with:
**		internal name:	"name"
**		title:	 	"title"
**		datatype:	"type"
**		width:		the default width for type "type"
**		height:		one line
**
**	If the "maxwidth" field is positive, but less than the default width
**	for the datatype, a scrolling field (or column) is created, with
**	"maxwidth" columns displayed.  "maxwidth" is currently ignored for
**	non-character fields, due to forms system limitations.
**	
**	smaller
**
** History:
**	7/20/89 (Mike S)	Initial version
**	8/08/89 (Mike S)	Add scrolling field support
*/
typedef struct _fld_desc
{
	char		*name;		/* name of field/column */
	char		*title;		/* title of field/column */
	DB_DATA_VALUE	*type;		/* type description */
	i4		maxwidth;	/* Max field width (ignored if <= 0) */
} FLD_DESC;

/*}
** Name:	LISTMENU
**
** Description:
**	Menuitem information for Listpick.  The behaviour codes control
**	what listpick will do with the menu item.  The user-provided
**	handler function is called for LPB_[CR]SELECT and LPB_QUIT.
**
**	A menu item flagged as LPB_CRSELECT will also get activated for
**	use of the return key.  Only one LPB_CRSELECT item should appear
**	in the array - if more than one exists, the first one will be
**	activated on a return key.  By specifying one of these with an
**	empty title string, you can get an activation ONLY on return key.
**
**	CAUTION: title and explan must be fast strings!
**
*/

typedef struct
{
	ER_MSGID	title;		/* Menuitem title */
	ER_MSGID	explan;		/* Menuitem explanation */
	i4		behavior;	/* Menuitem behavior */
	i4		frskey;		/* associated FRSkey, 0 for none */
                                                    
#define LPB_SELECT    	0		/* Call Handler with line number */
#define LPB_QUIT	1		/* Call handler with (-1) */
#define LPB_HELP	2		/* Call FEhelp */
#define LPB_TOP		3		/* Top of dataset */
#define LPB_BOTTOM	4		/* Bottom of dataset */
#define LPB_FIND 	5		/* Call FEtabfnd */
#define LPB_CRSELECT	6		/* SELECT, with CR key equivalent */

#define LPB_NO_CHOICE	7	 	/* Number of legal values */
} LISTMENU;

/*}
** Name:	LISTPICK
**
** Description:
**	Control structure for listpick call.  The menuitems array may
**	contain up to 15 items, which will be displayed in the order
**	specified.
**
**	lineno may be specified as <= 0 to start at default location on
**	tablefield (this will be the top on the first call, otherwise
**	whatever row you were on)
**
**	The handler is called on LPB_SELECT and LPB_QUIT as:
**
**	(*handler)(data, choice, res, midx)
**	PTR data;
**	i4 choice;
**	bool *res;
**	i4 midx;
**
**		data - data pointer from structure.
**		choice - 0 based choice, -1 for an LPB_QUIT item.
**		res - resume flag - TRUE to resume display loop, FALSE
**			to break display.
**		midx - index into menu array which caused call.
*/

typedef struct
{
	char 	*form;		/* form name for listpick */
	char	*tf;		/* Tablefield name */
	i4	srow;		/* Start row */	
	i4	scol;		/* Start column */
	i4	lineno;		/* Starting line for tablefield display */
	char	*htitle;	/* Help title */
	char	*hfile;		/* Help file */
	i4	nomenitems;	/* Number of menuitems */
	LISTMENU *menuitems;	/* Menuitems */
	i4	(*handler)();	/* Handler function */	
	PTR	data;		/* Data to pass to handler */
	i4	flags;		/* flags word */
} LISTPICK;

/*
** LISTPICK flags
*/
#define LPK_BORDER 1	/* set a border */

/*
** special encodes used for scol, srow to place listpick in given location
** LPGR_CENTER may be used for either row or column.  BOTTOM/TOP used for
** row, LEFT/RIGHT used for column.  For example srow = LPGR_BOTTOM,
** scol = LPGR_CENTER positions the listpick just above the menuline
** in the center of the screen.  _TOP / _BOTTOM are really equivalent
** to row / column 0, of course, but the #define's are here for completeness.
**
** LPGR_FLOAT MUST be encoded as -1, also, unless logic in listpck.qsc is
** changed.
*/
# define LPGR_FLOAT	-1
# define LPGR_TOP	0
# define LPGR_BOTTOM	-3
# define LPGR_LEFT	0
# define LPGR_RIGHT	-3
# define LPGR_CENTER	-4

/* Prototypes that need flddesc stuff */
FUNC_EXTERN STATUS IIFRmdfMakeDefaultForm(char *, char *, bool, i4, FLD_DESC *, i4, FRAME **);

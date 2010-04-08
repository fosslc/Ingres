/*
**  menu.h
**
**  Copyright (c) 2004 Ingres Corporation
**
**  static	char	*Sccsid = "%W%	%G%";
**
**  History:
**	DKH - 7 May 1985 Added changes for long menus.
**	DKH - 11 Oct 1985 Added typedef for struct com_tab.
**	12/04/86 (drh) Added nested menu support.
**	12/22/86 (dkh) - Added support for new activations.
**	02/18/89 (dkh) - Fixed venus bug 4546.
**	01/10/90 (dkh) - Added CT_TRUNC flag.
**	26-mar-90 (bruceb)
**		Added mu_back, mu_forward and mu_colon for locator support.
**	29-jun-89 (fraser)
**		Added definitions to support ring menus.
**	03/21/91 (dkh) - Integrated changes from PC group.
**	17-aug-91 (leighb) DeskTop Porting Change: Change RING_ATTR's
**	24-mar-92 (leighb) DeskTop Porting Change: Added GUI menu define's:
**		MU_GUI, MU_GUIDIVIDER & MU_GUISUBMU
**	6-may-92 (leighb) DeskTop Porting Change:
**		Moved MU_GUISUBMU to wnmenu.h in cl!hdr
**	06/24/92 (dkh) - Added support for menu renaming and enable/disable
**			 of a menuitem.
**	25-may-95 (emmag)
**		DELETE is defined by Microsoft in MS VC.   Since this 
**		generates a warning for each of our menus.h files in the
**		front end tools, let's undef it here and be done with it,
**		since each of the menus.h files include this one.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# ifdef NT_GENERIC
# ifdef DELETE
# undef DELETE
# endif
# endif

struct menuType {
	char		*mu_prline;	/* image of menu line, unused in 4.0 */
	struct com_tab	*mu_coms;	/* pointer to array of commands */
	i4		mu_flags;	/* attributes of the menu */
	FTINTRN		*mu_prompt;	/* private FT use */
	FTINTRN		*mu_window;	/* private FT use */
	i4		mu_dfirst;	/* private FT use */
	i4		mu_dsecond;	/* private FT use */
	i4		mu_last;	/* number of menu items */
	struct menuType	*mu_prev;	/* previous menu */
	i4		*mu_act;	/* activation save area */
	i4		mu_back;	/* X-coord of '<'; -1 when not used. */
	i4		mu_forward;	/* X-coord of '>'; -1 when not used. */
	i4		mu_colon;	/* X-coord of ':'; -1 when not used. */
	i4		mu_active;	/* number of enabled menuitems */
};

struct com_tab {
	char	*ct_name;		/* the menu item */
	i4	(*ct_func)();		/* function to call if selected */
	i4	ct_enum;		/* value to return if selected */
	i4	ct_flags;		/* attributes of menu item */
	i4	ct_begpos;		/* beginning screen position of item */
	i4	ct_endpos;		/* ending screen position of item */
	char	*description;		/* help text for item, not used */
};

typedef struct	menuType 	*MENU;
typedef	struct	com_tab		COMTAB;

# define	MAXMULEN	(COLS - 15)

# define	MAX_MENUITEMS	25	/* max number of menu items per menu */

# define	MAX_SUBMUCNT	18	/* max pop up menu items */
# define	SUBMU_SPACE	10	/* width padding for pop up menus */

# define	NULLRET		01	/* NULL entry allowed for menu */
# define	INPUT		02	/* Accept input for menu */
# define	MU_RESET	010	/* delete all menu set boundaries */
# define	MU_SUBMENU	020	/* menu is a submenu */
# define	MU_NEW		040	/* new menu passed in */
# define	MU_LONG		0100	/* menu length wider than terminal */
# define	MU_REFORMAT	0200	/* need to reformat menu due to */
					/* change in menu mappings. */
# define	MU_FRSFRM	0400	/* Menu has an associated form. */

# define	CT_BOUNDARY	01	/* menu item is on menu set boundary */
# define	CT_YES_VALID	02	/* validation set for menu item */
# define	CT_NO_VALID	04	/* no validation for menu item */
# define	CT_UF_VALID	010	/* undef validation for menu item */
# define	CT_ALONE	020	/* menu item fits alone on screen */
# define	CT_ACTYES	040	/* activate on field turned on */
# define	CT_ACTNO	0100	/* activate on field turned off */
# define	CT_ACTUF	0200	/* UndeF activation option */
# define	CT_TRUNC	0400	/* Truncate display of item */
# define	CT_DISABLED	01000	/* Menuitem has been disabled */

# define	MU_BOTTOM	0	/* menu on last line of screen */
# define	MU_TOP		1	/* menu on first line of screen */

# define	MU_TYPEIN	0	/* regular menu input */
# define	MU_LOTUS	1	/* lotus style menu input */
# define        MU_RING         2       /* Ring menu display and input */
# define        MU_GUI          3       /* Graphical User Interface menu */

# define	SPACE_BETWEEN	2	/* Space between items in ring menus */
# define	RING_MARGIN	3	/* Margin for ring menus-must be > 1. */

/*
**  Screen must have at least this many lines for ring menus
**  to take over two lines.
*/
# define	RING_SCR_MIN	24

# define	MENU_TYPE_VAR	"II_MENU"    /* Name of environment variable */

# define	MENU_WHERE_VAR	"II_RING_WHERE"    /* environment variable */

/*
**  Ring menu display attributes.
*/
# define RING_ATTR_ACTIVE    0          /* Menu line when menu is active    */
# define RING_ATTR_INACT     _CI_RV     /* Menu line when menu is inactive  */
# define RING_ATTR_HIGH      _COLOR4	/* Current item when highlighted    */
# define RING_ATTR_CURSOR    _RVVID     /* Menu cursor                      */
# define RING_ATTR_ARROW     _CHGINT    /* Arrows                           */
# define RING_ATTR_EXPL      _COLOR4	/* Explanation line                 */

/*
**  GUI menu syntax characters
*/

#define	MU_GUIDIVIDER	';'

/*
**  Copyright (c) 2004 Ingres Corporation
*/

# include   <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include   <fe.h>
# include   "ftframe.h"
# include   <menu.h>
# include   <mapping.h>
# include   <kst.h>
# include   <frsctblk.h>
# include   <cm.h>
# include   <st.h>
# include   <te.h>
# include   <er.h>
# include   <nm.h>
# include   <me.h>


/*
**      INSERT_AUTOMATED_FUNCTION_PROTOTYPES_HERE
*/
FUNC_EXTERN STATUS      FKmapkey();
FUNC_EXTERN char *      FTflfrsmu();
FUNC_EXTERN VOID        FTprnscr();
FUNC_EXTERN KEYSTRUCT * FTTDgetch();
FUNC_EXTERN VOID        IIFTstsSetTmoutSecs();
FUNC_EXTERN bool        IIFTtoTimeOut();
FUNC_EXTERN i4          TDaddstr();
FUNC_EXTERN             TDerase();
FUNC_EXTERN i4          TDmove();
FUNC_EXTERN VOID        TDoverlay();
FUNC_EXTERN i4          TDrefresh();
FUNC_EXTERN             TDtouchwin();


static VOID display_ring();
static VOID adjust_ring();
static i4   fill_to_left();
static i4   fill_to_right();
static i4   ringDummyFunc();

GLOBALREF    FRS_GLCB *	IIFTglcb;	/* FRS Global Control Block	*/
GLOBALREF    u_char	pda;

/*
**  Name: ftring.c - Contains routines for ring menus
**
**  Description:
**
**      An environment variable determines whether menus are
**      displayed in "classic" or ring style.  The name of the
**      environment variable is defined in menu.h (MENU_TYPE_VAR).
**      Currently, the environment variable is defined as
**      II_MENU.  Menus are displayed in ring style if and only if
**      the environment variable is assigned the value "ring" (may
**      be upper or lower case):
**
**          II_MENU=RING
**
**      Another environment variable determines whether ring menus
**      are displayed at the top or bottom of the screen.  The name of
**      the environment variable is defined in menu.h (MENU_WHERE_VAR).
**      Currently, the environment variable is defined as II_RING_WHERE.
**      The values for this environment variable are "TOP" or "BOTTOM".
**      The default position is at the top of the screen.
**
**
**      This file defines:
**
**          FTmuset         - Reads environment variables and sets up
**                            global variables which determine the
**                            menu characteristics.
**
**          FTinring        - Initializes for ring menus by allocating
**                            space for the necessary windows and arrays
**                            used for ring menus.
**
**          FTputring       - Puts a menu on the screen, and makes it
**                            the current menu.  FTputring is analogous
**                            to FTputmenu, and is called by FTputmenu
**                            if ring menus are used.
**
**          IIFTgetring	    - "Activates" the current menu, and returns
**                            with the user's selection.  IIFTgetring is
**                            analogous to FTgetmenu, and is called by
**                            FTgetmenu if ring menus are used.
**
**                            IIFTgetring also returns on a timeout or when
**                            the user presses an FRS key.  For details,
**                            see the description below of IIFTgetring.
**
**          FTsetring       - Sets the menu display attributes
**
**
**        The following are internal (static) functions:
**
**          display_ring    - Actually displays a portion of the current
**                            menu.
**
**          adjust_ring     - Selects the portion of the menu to be displayed.
**
**          fill_to_left    - Used by adjust_ring.
**
**          fill_to_right   - Used by adjust_ring.
**
**
**      Comments on the following page describe the changes made
**      to other files.
**
**  History:
**
**      12-jul-89 (fraser)  First written
**
**      01-nov-89 (fraser)
**          Modified to use only one line when menu is inactive.
**
**	15-nov-91 (leighb) DeskTop Porting Change:
**	    Cleanup Mouse Functinality by ifdefing calls to TE routines.
**	16-dec-91 (leighb) DeskTop Porting Change:
**	    Added following history to make Dave Brower happy:
**	    Changed IIFTgetring() to FTgetring() to match naming convention
**	    of other functions in FT.
**	17-dec-91 (leighb) DeskTop Porting Change:
**	    Change FTgetring() to IIFTgetring() to make Dave Hung happy.
**	19-dec-91 (leighb) DeskTop Porting Change:
**		Changes to allow menu line to be at either top or bottom.
**	6-apr-1992 (fraser--for leighb)
**		Added code to handle a new menu type (MU_GUI) to support
**		"GUI-style" menus for the Vision-under_Windows project.
**	06/01/92 (dkh) - Cast return values from MEreqmem() to
**			 eliminate compilation warnings.
**	10-mar-92 (leighb) Check for MU_GUIDIVIDER character in menu item
**		name and replace with a '\0' if not using GUI menus.
**	01-jul-92 (leighb) DeskTop Porting Change:
**		Don't map keystrokes on TIMEOUT;
**		added II_MENU=button processing.
**	08/24/92 (dkh) - Fixed acc complaints.
**	28-sep-92 (johnr) ULTRIX compiler does not permit integer hex
**	    constant in a character constant.  Replaced \x values
**	    with octal equivalents.
**	28-sep-92 (johnr) Correct the comments for CONTROL_T and TABCHAR
**	18-sep-96 (thoda04) bug 78675
**	    Check string prefix length is a minimum of 4 for II_MENU=ring.
**	28-Mar-2005 (lakvi01)
**	    Corrected function prototypes.
*/

/*
**      The current code contains some definitions and code for
**      "Lotus style" menus.  This code has not been kept current,
**      however, and Lotus-style menus do not work.  Although ring
**      menus are Lotus-style menus, the term "ring menu" is used
**      for the new code so that the code in this module and the few
**      changes necessary in other modules can be easily distinguished
**      from the old code dealing "lotus style" menus.
**
**
**      The following changes were made to other files:
**
**
**          menu.h
**          ------
**          Added new definitions specific to ring menus:
**
**              MU_RING         A new value for mutype
**              SPACE_BETWEEN   The number of spaces between items
**                              when a ring menu is displayed.
**              RING_MARGIN     The amount of space at either end of
**                              the line when a ring menu is displayed.
**                              Must be > 1.
**              MENU_TYPE_VAR   The name of the environment variable which
**                              determines the menu type.
**              MENU_WHERE_VAR  The name of the environment variable which
**                              determines where the menu is displayed.
**
**              RING_ATTR_ACTIVE    Display attributes
**              RING_ATTR_INACT
**              RING_ATTR_HIGH
**              RING_ATTR_CURSOR
**              RING_ATTR_ARROW
**              RING_ATTR_EXPL
**
**
**          ftinit.c [ft]
**          -------------
**
**          Added a call to FTmuset.
**
**
**          ftptmenu.c [ft]
**          ---------------
**
**          FTputmenu - added call to FTputring:
**
**              if (mutype == MU_RING)
**                  return FTputring(mu);
**
**          FTmuinit - added call to FTinring.
**
**          FTvisclumu - do nothing if ring menu.
**
**
**          ftgtmenu.c [ft]
**          ---------------
**
**          FTgetmenu - added call to IIFTgetring:
**
**              if (mutype == MU_RING)
**                  return IIFTgetring(mu, evcb);
**
**
**
**      Future changes.
**
**          The existing menu routines contain some conditionally
**          compiled code which is specifically for graphics support
**          (# ifdef GRAPHICS).  Initially, the ring menu routines
**          do not provide the same graphics support.  For complete
**          compatibility, graphics support will have to be added.
*/

/*
**  How ring menus work.
**  -------------------
**
**  The ring menu routines are based on the menuType and com_tab
**  structures defined in menu.h.  The menu routines are passed
**  a pointer to a menuType structure.
**
**  The ring menu routines expect to find valid information in
**  the following two members of the menuType structure:
**
**      1)  mu_coms -   a pointer to an array of com_tab structure
**      2)  mu_last -   the number of menu items (the size of the
**                      array mu_coms points to)
**
**  The ring menu routines use the members mu_window, mu_dfirst, mu_dsecond,
**  mu_flags, mu_back, and mu_foward but ignore other members of the menuType
**  structure.
**
**  For each com_tab structure (i.e., for each menu item), the ring
**  menu routines expect to find valid information:
**
**      1)  ct_name -   a pointer to the string containing the name
**                      of the menu item
**      2)  ct_func -   a pointer to a function to call if the item
**                      is selected.  Must be NULL if no function is
**                      to be called.
**      3)  ct_enum -   a value to be returned if the item is selected
**      4)  description - a pointer to a string explaining the menu
**                      item.  If the pointer is not NULL, the string
**                      is displayed on a separate line whenever the
**                      menu cursor is on the corresponding menu item.
**
**  The ring menu routines do not use ct_flags.  ct_begpos and ct_endpos
**  are used internally by the ring routines.
**
**  Ring menus use two lines on the screen.  The menu line
**  is used to display menu options.  When a menu is put on the screen,
**  as many options as will fit on one line are displayed.  When the
**  menu is active, one of the displayed items is the current item
**  and is highlighed with the menu cursor.  If there is a description
**  for the current item, it is displayed on a separate line--the
**  explanation line.
**
**  A ring menu uses s second line only when the menu is active.  When
**  the menu becomes active, the contents of the second line are saved
**  and the second line is used to display the explanation for the
**  current menu item.  When the menu becomes inactive, the contents
**  of the second line are restored.
**
**  When a menu is active, the user may browse through the menu by
**  moving the menu cursor.  If the menu does not fit on the menu
**  line, new sections of the menu are displayed as the cursor is
**  moved past the first or last item displayed.
**
**  The ring menus are designed to provide a simple, easy-to-understand
**  interface for the inexperienced user, as well as to provide the
**  experienced user with the ability to select menu options with
**  as few key strokes as possible.
**
**  If a menu is active, the user may select the current menu option
**  by pressing the ENTER key.  The user may also select a menu option
**  by pressing the key corresponding to the first character of the name
**  of the meu item.  If a character uniquely specifies a menu item,
**  pressing the key selects the menu option.
**
**  If the names of two or more items on the menu begin with the
**  same character, pressing the key corresponding to the first character
**  does not uniquely specify an item.  In that case, the key corresponding
**  to the first character functions somewhat like a tab key.  This is best
**  illustrated with an example.
**
**  Suppose the menu contains the following items beginning with "M":
**
**      Monkey ... Mongoose ... | ... Mouse  Marmoset
**
**  The dots indicate that other menu items are between the items
**  shown.  Suppose that the part of the menu before the vertical bar
**  is on the screen.
**
**  The first time the user presses the "m" key, the first (leftmost)
**  item on the screen (Monkey) becomes the current item.  If the
**  user press "m" again without first pressing another key, the next
**  item beginning with "M" (Mongoose) becomes the current item.
**  If the user presses "m" again, "Mouse" is moved onto the screen
**  and is made the current item, and so on.  At any time the user
**  may select the current item by pressing ENTER.
**
**  Note that the ring menu routines ignore case when comparing the
**  first letter of a menu item with a key the user presses.
**
**  The up- and down-arrow keys shift the menu backward and forward.
**
**  The menu key returns the user to the current form without
**  selecting a menu option.
**
**  Notes on screen management.
**  --------------------------
**
**  These routines make use of the fact that the window pointed
**  to by curscr is the entire screen.
**
**  These routines depend on the values of two external variables:
**
**      COLS    contains the number of columns on the screen
**      LINES   contains the number of lines (rows) on the screen
**
*/


# define    MUS_ACTIVE      0       /* Menu states */
# define    MUS_INACTIVE    1

# define    LEFT_ADJUST     0       /* Adjustment types */
# define    RIGHT_ADJUST    1
# define    MID_ADJUST      2
# define    AUTO_ADJUST     3

# define    CONTROL_M       '\015'	/* same as hex 0d	*/
# define    CONTROL_T       '\024'	/* same as hex 14	*/
# define    TAB_CHAR        '\011'	/* same as hex 09	*/


GLOBALREF   i4          mu_where;       /* Either MU_TOP or MU_BOTTOM       */
GLOBALREF   MENU        IIFTcurmenu;    /* pointer to current menu          */
GLOBALREF   WINDOW *    curscr;         /* window corresponding to the      */
                                        /* physical screen                  */
GLOBALREF   WINDOW *    stdmsg;
GLOBALREF   bool        ftshmumap;

GLOBALREF   i4          mutype;
GLOBALREF   i4          mubutton;
GLOBALREF   i4          IITDflu_firstlineused;

static char	mu_guidivider[2] = {MU_GUIDIVIDER, '\0'};

static i4       curritem = 0;       /* Index of current item in ct_coms     */
static i4       ring_row = 0;       /* Row where menu is displayed          */
static i4       expl_row = 0;       /* Row where explanation is displayed   */

static i4       ring_len = 0;       /* Length of ring window                */
static i4       expl_len = 0;       /* Length of explanation window         */
static i4       ring_marg = 0;      /* Margin for menu line                 */
static i4       expl_marg = 0;      /* Margin for explanation line          */
static i4       ring_start_col = 0; /* Starting column for ring line        */
static i4       expl_start_col = 0; /* Starting column for explanation line */

static i4	ignoreNextMenuKey = 0;	/* Ignore next Go to Menu Request   */
static bool     lower_right_ok = FALSE; /* Can use character in lower-right     */
                                        /* corner of screen without scrolling   */

static WINDOW * ringwin = NULL;   /* Pointer to window for menu line        */
static WINDOW * explwin = NULL;   /* Pointer to window for explanation line */
static WINDOW * savewin = NULL;   /* Place to save explanation line         */

static char *   key_name[MAX_MENUITEMS] = {0};  /* Pointer to frs key name  */
static i4       keyn_len[MAX_MENUITEMS] = {0};  /* Length of frs key name   */
static i4       item_len[MAX_MENUITEMS] = {0};  /* Length of menu item      */

/*
**  Memory is allocated for menu_text, expl_text and keyn_text because
**  we don't know in advance how large they must be.
*/
static char *   menu_text   = NULL;  /* Pointer to place to store menu line         */
static char *   expl_text   = NULL;  /* Pointer to place to store explanation line  */


/*
**  Display attributes
*/
static i4   ringataRingAttrActive = RING_ATTR_ACTIVE;   /* Menu line when menu  */
                                                        /* is active            */
static i4   ringatiRingAttrInact  = RING_ATTR_INACT;    /* Menu line when menu  */
                                                        /* is inactive          */
static i4   ringathRingAttrHigh   = RING_ATTR_HIGH;     /* Highlighting current */
                                                        /* item                 */
static i4   ringatcRingAttrCursor = RING_ATTR_CURSOR;   /* Menu cursor (menu    */
                                                        /* active)              */
static i4   ringatrRingAttrArrow  = RING_ATTR_ARROW;    /* Arrows               */

static i4   ringateRingAttrExpl   = RING_ATTR_EXPL;     /* Explanation line     */

/*
**  Number of fields on last FRAME to be run.
*/
static i4    curFrameNbrFlds = 0;	/* # fields on last FRAME to be run */



/*
**    Name:   FTmuset
**
**    Description:
**
*/
VOID
FTmuset()
{
    char    *envvar;

# ifdef PMFE
# ifdef PMFEWIN3
    mutype = MU_GUI;
    ftshmumap = FALSE;
# else
    mutype = MU_RING;
#endif
# else
    mutype = MU_TYPEIN;
# endif

    NMgtAt(ERx(MENU_TYPE_VAR), &envvar);    /* Check environment variable */
    if	   (envvar != NULL)
    {
	if ((i4)STlength(envvar) >= 4  &&  
	    STbcompare(envvar, 4, ERx("ring"), 4, TRUE) == 0)
	{
	    mutype = MU_RING;
	    if (STbcompare(envvar, 0, ERx("ring,nofunc"), 0,TRUE) == 0)
		ftshmumap = FALSE;
	}
        else if (STbcompare(envvar, 0, ERx("gui"), 0, TRUE) == 0)
        {
            mutype                = MU_GUI;
            ftshmumap             = FALSE;      /* NO menu map (externally) */
            mu_where              = MU_BOTTOM;  /* this elimnates bottom line */
            IITDflu_firstlineused = 0;          /* 1st line @ top */
        }
        else if (STbcompare(envvar, 0, ERx("button"), 0, TRUE) == 0)
        {
	    mubutton		  = 1;		/* GUI + buttons only */
            mutype                = MU_GUI;
            ftshmumap             = FALSE;      /* NO menu map (externally) */
            mu_where              = MU_BOTTOM;  /* this elimnates bottom line */
            IITDflu_firstlineused = 0;          /* 1st line @ top */
        }
        else if (STbcompare(envvar, 0, ERx("classic"), 0, TRUE) == 0)
            mutype = MU_TYPEIN;
    }

    if (mutype == MU_TYPEIN)    /* Set parameters for "classic" menus */
    {
        mu_where = MU_BOTTOM;
        IITDflu_firstlineused = 0;
    }
    else                        /* Set parameters for ring menus */
    {
        mu_where = MU_BOTTOM;
        NMgtAt(ERx(MENU_WHERE_VAR), &envvar);
        if ((envvar != NULL)
	 && (STbcompare(envvar, 0, ERx("top"), 0, TRUE) == 0))
	{
	    mu_where = MU_TOP;
            IITDflu_firstlineused = 1;
            ring_row = 0;
            expl_row = 1;
        }
        else                        /* menu at bottom */
        {
            IITDflu_firstlineused = 0;
            ring_row = LINES - 1;
            expl_row = LINES - 2;
        }

        ring_len = expl_len = COLS;
        ring_marg = (RING_MARGIN > 1) ? RING_MARGIN : 2;
        expl_marg = 1;
        ring_start_col = 0;
        expl_start_col = 0;
    }
}

/*
**  Name:   FTinring - initialize windows used for ring menus
**
**  Description:
**
**      FTinring sets up three windows:
**
**          ringwin     Used to display the menu
**          explwin     Used to display the explanation of a menu item
**          savewin     Used to save contents of explanation line
**
**      Also allocates memory for menu and explanation lines.
**
**  Inputs: None.
**
**  Outputs:
**
**      Returns:    OK      if successful
**                  FAIL    if menory for the windows cannot be allocated
**
**  Side effects:
**
**      Sets the pointers ringwin and explwin.
**
**  History:
**
**      12-jul-89 (fraser)  First written
*/

STATUS
FTinring()
{
    STATUS  ret;

    /* Initialize windows */

    ringwin = TDnewwin(1, ring_len, ring_row, ring_start_col);
    explwin = TDnewwin(1, expl_len, expl_row, expl_start_col);
    savewin = TDnewwin(1, expl_len, expl_row, expl_start_col);

    /* Get memory for text buffers */

    menu_text = (char *) MEreqmem(0, COLS + 1, FALSE, NULL);
    expl_text = (char *) MEreqmem(0, COLS + 1, FALSE, NULL);

    ret = (  ringwin == NULL ||   explwin == NULL || savewin   == NULL ||
           menu_text == NULL || expl_text == NULL )
                ? FAIL : OK;

    if (ret == OK)
    {
        ringwin->_relative = explwin->_relative = savewin->_relative = FALSE;
    }

    return ret;
}

/*
**  Name:   FTputring - puts a ring menu on the screen
**
**  Description:
**
**      FTputring puts a menu on the screen in the ring menu style.
**
**      Once a menu is put on the screen with FTputring, the program
**      keeps track of the current item.  Each time the menu is
**      activated with FTgetyring, the menu cursor is put on the
**      current item.  The value of the current item is not preserved
**      when a new menu is put on the screen.
**
**      Before FTputring is called, space must have been allocated for
**      the menu windows.
**
**  Inputs:
**
**      mu      A pointer to a menu structure.
**
**  Outputs:
**
**    Returns:  TRUE
**
**      We want FTputring to return the same value that FTputmenu would
**      return.  FTputmenu returns TRUE except in the case when a call
**      to TDsubwin fails, when FTputmenu returns FALSE.  Since FTputring
**      cannot fail in this way, it always returns TRUE.
**
**    Exceptions:   None.
**
**  Side Effects:
**
**      Sets IIFTcurmenu = mu.
**      Sets curritem = 0 if a new menu.
**
**  History:
**
**      12-jul-89 (fraser)  First written
*/

bool
FTputring(mu)
MENU    mu;
{
    i4      i;
    i4      adjustment;
    char *  p;

    if (mu->mu_last > MAX_MENUITEMS)
    {
        mu->mu_last = MAX_MENUITEMS;
        IIFTcurmenu = NULL;
    }

    for (i = 0; i < mu->mu_last; i++)
    {
        item_len[i] = STlength(mu->mu_coms[i].ct_name);
	if (p = STindex(mu->mu_coms[i].ct_name, mu_guidivider, item_len[i]))
	{
		*p = EOS;
		item_len[i] = STlength(mu->mu_coms[i].ct_name);
	}
        if (ftshmumap)
        {
            p = FTflfrsmu(mu->mu_coms[i].ct_enum);
            if (p == NULL)
                p = menumap[i];
            key_name[i] = (p != NULL) ? p : ERx("");
        }
        else
        {
            key_name[i] = ERx("");
        }
        keyn_len[i] = STlength(key_name[i]);
    }

    if (mu != IIFTcurmenu || (mu->mu_flags & (MU_NEW | MU_REFORMAT)))
    {
        mu->mu_dfirst = mu->mu_last + 1;
        mu->mu_dsecond = -1;
        mu->mu_window = ringwin;
	mu->mu_back = 0;
	mu->mu_forward = COLS - 1;
	mu->mu_colon = -1;

        IIFTcurmenu = mu;
        curritem = 0;
        adjustment = LEFT_ADJUST;
    }
    else
    {
        adjustment = AUTO_ADJUST;
    }
    mu->mu_flags &= ~(MU_NEW | MU_REFORMAT);

    display_ring(IIFTcurmenu, curritem, curritem, adjustment, MUS_INACTIVE);
    return TRUE;
}

/*
**  Name:   IIFTgetring - gets the user's choice from the current menu
**
**  Description:
**
**
**
**  Inputs:
**
**      mu          A pointer to a menu structure.
**      evcb        A pointer to an FRS_EVCB (event control block)
**
**      curritem    The index of the current item.
**
**  Outputs:
**
**      evcb    IIFTgetring sets the following members of the event
**              control block:
**
**                  evcb->event
**                  evcb->intr_val
**                  evcb->mf_num
**
**    Returns:
**
**      The value returned depends on what event triggers the
**      return.  The are several possibilities:
**
**          Event                       Value returned
**          -----                       --------------
**
**          User selects a menu item    ct_enum from the com_tab structure
**                                      for the item
**
**          User presses an FRS key     fdopFRSK
**
**          A timeout occurs            fdopTMOUT
**
**          User presses down-arrow     0
**
**    Exceptions:
**
**  Side Effects:
**
**  History:
**
**      12-jul-89 (fraser)  First written
*/

i4
IIFTgetring(mu, evcb)
MENU        mu;
FRS_EVCB *  evcb;
{
    i4          i;
    i4          ret;            /* The value to return                      */
    i4          nmatch;         /* Counts matches while searching for item  */
    i4          istart;         /* Index for starting search                */
    i4          item1;          /* Index of first matching item             */
    i4          item2;          /* Index of second matchng item             */
    i4          next_item;      /* The next current item                    */
    i4          adjust_item;    /* Item number for adjustment               */
    i4          adjustment;     /* Adjustment type                          */
    i4          code;           /* Parameter for FKmapkey                   */
    i4          flg;            /* Parameter for FKmapkey                   */
    bool        val;            /* Parameter for FKmapkey                   */
    u_char      dummy;          /* Parameter for FKmapkey                   */
    bool        repeat_char;    /* TRUE iff last two keystrokes are the same*/
    bool        done;           /* TRUE when done                           */
    bool        item_selected;  /* TRUE iff item has been selected          */
    COMTAB *    ct;
    KEYSTRUCT * ks;             /* Holds the most recent keystroke          */
    KEYSTRUCT   last_key;       /* Holds the previous keystroke             */
    i4          (*fptr)();

    if (ignoreNextMenuKey == 1)
    {
	ignoreNextMenuKey = 0;
	return 0;
    }

# ifdef	PMFE

    if (TElastKeyWasMouse() || ((curFrameNbrFlds == 0) && !ftshmumap))
    {
	i    = IIFTglcb->intr_frskey0;
	fptr = IIFTglcb->key_intcp;
	IIFTglcb->intr_frskey0 = 1;
	IIFTglcb->key_intcp    = ringDummyFunc;
	ret = FTrunNoFld(mu, evcb);
	IIFTglcb->intr_frskey0 = i;
	IIFTglcb->key_intcp    = fptr;
	if (ret != fdopMENU)
		return(ret);
    }

    TEonRing(1);

# endif	/* PMFE */

    if (mu != IIFTcurmenu)
        FTputring(mu);

# ifdef	PMFE

    if (TEmousePos(&item1, &item2) && (item1 == (LINES - 1)))
    {
	for (i = mu->mu_dfirst; i < mu->mu_dsecond; i++)
	{
            ct = &(mu->mu_coms[i]);
	    if ((ct->ct_begpos <= item2) && (item2 <= ct->ct_endpos))
	    {
		curritem = i;
		break;
	    }
	}
    }

# endif	/* PMFE */

    if (curritem < 0 || curritem >= mu->mu_last)
        curritem = 0;

    done =          FALSE;
    item_selected = FALSE;
    adjustment =    AUTO_ADJUST;
    next_item =     curritem;
    adjust_item =   curritem;

    evcb->event = 0;

    TDoverlay(curscr, expl_row, expl_start_col, savewin, 0, 0); /* Save line */

    last_key.ks_ch[0] = mu->mu_coms[curritem].ct_name[0];

    IIFTstsSetTmoutSecs(evcb);  /* Set timeout value */

    do      /* loop until done */
    {
        display_ring(mu, next_item, adjust_item, adjustment, MUS_ACTIVE);

        ks = FTTDgetch(mu->mu_window);      /* Get keystroke */

        if (!IIFTtoTimeOut(evcb))           /* Map key if not timeout */
	{
            FKmapkey(ks, (char *)NULL, &code, &flg, &val, &dummy, evcb);

            if (ks->ks_fk == 0)
            {
		switch (ks->ks_ch[0])
		{

		case CONTROL_M:
                    evcb->event = fdopRET;
                    break;

		case ' ':
		case TAB_CHAR:
                    evcb->event = fdopRIGHT;
                    break;

		case CONTROL_T:	    /* menumap command */
		    FTsetmmap(ftshmumap ? FALSE : TRUE);
		    IIFTcurmenu = 0;
                    evcb->event = fdopMENU;
		    break;
		}
            }
	}

        switch (evcb->event)
        {

        case fdopTMOUT:             /* timeout */
            ret = fdopTMOUT;
            done = TRUE;
            break;

        case fdopMUITEM:            /* menu item selected */
            if (evcb->mf_num < mu->mu_last)
            {
                curritem = next_item = adjust_item = evcb->mf_num;
                ct = &(mu->mu_coms[curritem]);
                ret = ct->ct_enum;
                done = item_selected = TRUE;
            }
            break;

        case fdopFRSK:              /* FRS key */
            ret = evcb->intr_val;
            done = TRUE;
            break;

        case fdopPRSCR:             /* Print current screen */
            FTprnscr(NULL);
            break;

        case fdopMENU:              /* Menu key - return to form */
            ret = 0;
            done = TRUE;
            break;

        case fdopUP:                /* Shift menu to the left */
            if (mu->mu_dfirst > 0)
            {
                next_item = adjust_item = mu->mu_dfirst;
                adjustment = RIGHT_ADJUST;
                mu->mu_dfirst = mu->mu_last + 1;
                mu->mu_dsecond = -1;
            }
            break;

        case fdopDOWN:              /* Shift menu to the right */
            if (mu->mu_dsecond < mu->mu_last)
            {
                next_item = adjust_item = mu->mu_dsecond - 1;
                adjustment = LEFT_ADJUST;
                mu->mu_dfirst = mu->mu_last + 1;
                mu->mu_dsecond = -1;
            }
            break;

        case fdopLEFT:              /* Move menu cursor left */

            if (curritem > 0)
            {
                adjust_item = curritem;
                next_item = curritem - 1;
                adjustment = RIGHT_ADJUST;
            }
            else
            {
                adjust_item = next_item = mu->mu_last - 1;
                adjustment = LEFT_ADJUST;
            }
            break;

        case fdopRIGHT:             /* Move menu cursor right */

            if (curritem < mu->mu_last - 1)
            {
                adjust_item = curritem;
                next_item = curritem + 1;
                adjustment = LEFT_ADJUST;
            }
            else
            {
                adjust_item = next_item = 0;
                adjustment = RIGHT_ADJUST;
            }
            break;

        case fdopRET:               /* return key */

            ret = mu->mu_coms[curritem].ct_enum;
            item_selected = TRUE;
            done = TRUE;
            break;

        case fdopERR:               /* a "regular" character */
        case fdNOCODE:

            repeat_char = (CMcmpnocase(ks->ks_ch, last_key.ks_ch) == 0);
            istart = repeat_char ? curritem : mu->mu_dfirst;
            nmatch = 0;
            for (i = istart; nmatch < 2 && i < mu->mu_last; i++)
                if (CMcmpnocase(ks->ks_ch, mu->mu_coms[i].ct_name) == 0)
                {
                    nmatch++;
                    if (nmatch == 1)
                        item1 = i;
                    else
                        item2 = i;
                }
            for (i = 0; nmatch < 2 && i < istart; i++)
                if (CMcmpnocase(ks->ks_ch, mu->mu_coms[i].ct_name) == 0)
                {
                    nmatch++;
                    if (nmatch == 1)
                        item1 = i;
                    else
                        item2 = i;
                }
            if (nmatch == 1)
            {
                curritem = next_item = item1;
                ret = mu->mu_coms[item1].ct_enum;
                item_selected = TRUE;
                done = TRUE;
            }
            else if (nmatch > 1)
            {
                next_item = repeat_char ? item2 : item1;
            }
            adjust_item = next_item;
            adjustment = MID_ADJUST;
            break;

        default:
            break;
        }

        ks->ks_ch[2] = EOS;
        STcopy((char *) ks->ks_ch, (char *) last_key.ks_ch);   /* Save character */

    } while (!done);

# ifdef	PMFE
    TEonRing(0);
# endif	/* PMFE */

    display_ring(mu, next_item, adjust_item, adjustment, MUS_INACTIVE);

    TDtouchwin(savewin);   /* Restore previous contents of explanation line */
    pda = 0;
    TDrefresh(savewin);

    if (item_selected)
    {
        if ((fptr = mu->mu_coms[curritem].ct_func) != NULL)
            (fptr)();
        evcb->event    = fdopMUITEM;
        evcb->intr_val = ret;
        evcb->mf_num   = curritem;
    }

    if (mu->mu_flags & MU_RESET)
        IIFTcurmenu = NULL;

    return ret;
}

/*
**  Name:   FTsetring - sets one or more of the menu display attributes
**
**  Description:
**
**      The display attributes for ring menus are held in five static
**      variables in this module.  This routine can be used to change
**      the attributes.
**
**  Inputs:
**
**      active          The display attribute for the active menu line.
**      inactive        The display attribute for the inactive menu line
**      highlight       The display attribute for highlighting an
**                      item on the menu line.
**      cursor          The display attribute for the menu cursor.
**      arrow           The display attribute for the arrows which
**                      indicate that only part of the menu is on
**                      the screen.
**      explanation     The attribute for the explanation line.
**
**      FTsetring will leave an attribute unchanged if the
**      corresponding parameter is -1.
**
**  Outputs:  None.
**
**    Returns:  Nothing.
**
**  Side effects:
**
**      May set one or more of the following (static) globals:
**
**          ringataRingAttrActive
**          ringatiRingAttrInact
**          ringathRingAttrHigh
**          ringatcRingAttrCursor
**          ringatrRingAttrArrow
**          ringateRingAttrExpl
**
**  History:
**      12-jul-89 (fraser)  First written
*/

VOID
FTsetring(active, inactive, highlight, cursor, arrow, explanation)
i4      active;
i4      inactive;
i4      highlight;
i4      cursor;
i4      arrow;
i4      explanation;
{
    if (active != -1)
        ringataRingAttrActive = active;
    if (inactive != -1)
        ringatiRingAttrInact = inactive;
    if (highlight != -1)
        ringathRingAttrHigh = highlight;
    if (cursor != -1)
        ringatcRingAttrCursor = cursor;
    if (arrow != -1)
        ringatrRingAttrArrow = arrow;
    if (explanation != -1)
        ringateRingAttrExpl = explanation;
}

/*
**  Name:   FTringShift - Shift Inactive Ring Menu to Right or Left
**
**  Description:
**
**	After a mouse click upon the '<' or the '>', shift the inactive
**	ring menu to the left or right.
**
**  Inputs:
**
**	direction	etiher fdopSCRRT or fdopSCRLF
**
**  Outputs:  None.
**
**  Returns:  Nothing.
**
**  Side effects:
**
**	Redisplays Inactive Menu
**
**  History:
**      30 may-91 (leighb)  First written
*/

VOID
FTringShift(direction)
i4	direction;
{
	i4         next_item;      /* The next current item	*/
	i4         adjustment;     /* Adjustment type          */

	ignoreNextMenuKey = 1;
        if	(direction == fdopSCRRT)
        {
            if (IIFTcurmenu->mu_dfirst > 0)
            {
                next_item = IIFTcurmenu->mu_dfirst;
                adjustment = RIGHT_ADJUST;
	    }
	    else return;
        }
        else
        {
            if (IIFTcurmenu->mu_dsecond < IIFTcurmenu->mu_last)
            {
                next_item = IIFTcurmenu->mu_dsecond - 1;
                adjustment = LEFT_ADJUST;
	    }
	    else return;
	}
	IIFTcurmenu->mu_dfirst = IIFTcurmenu->mu_last + 1;
        IIFTcurmenu->mu_dsecond = -1;
	display_ring(IIFTcurmenu,next_item,next_item,adjustment,MUS_INACTIVE);
}

/*
**  Name:   display_ring -
**
**  Description:
**
**      display_ring displays the current menu, making sure that
**      the current item is on the screen.
**
**      display_ring depends on adjust_ring to arrange the menu items in the
**      string menu_text for display.
**
**      display_ring does not call adjust_ring if
**
**          mu->mu_dfirst <= item_number < mu->mu_dsecond
**
**      because in that case there is no need to.
**
**  Inputs:
**
**      mu              a pointer to the menu structure
**
**      item_number     the index of the current item
**                      (0 <= item_number < mu->mu_last)
**
**      adjust_item     Item on which to adjust the menu if necessary.
**
**      adjustment      An adjustment type.
**
**      menu_state      MUS_ACTIVE
**
**                          The explanation line is displayed, and the
**                          menu cursor is displayed.
**
**                      MUS_INACTIVE
**
**                          The menu cursor is not displayed, and the
**                          explanation line is not changed.
**
**  Outputs:    None
**
**    Returns:  None.
**
**    Exceptions:  None.
**
**  Side Effects:
**
**      Sets curritem = item_number.
**
**  History:
**
**      12-jul-89 (fraser)  First written
*/

static VOID
display_ring(mu, item_number, adjust_item, adjustment, menu_state)
MENU    mu;
i4      item_number;
i4      adjust_item;
i4      adjustment;
i4      menu_state;
{
    i4      i;
    i4      n;
    i4      attr;
    char *  p;
    bool    left_arrow;
    bool    right_arrow;
    COMTAB *ct;

    if (item_number < mu->mu_dfirst || item_number >= mu->mu_dsecond)
        adjust_ring(mu, adjust_item, adjustment);

    /* Prepare menu line and explanation line for display */

    MEfill(ring_len, ' ', menu_text);
    menu_text[ring_len] = EOS;

    left_arrow = right_arrow = FALSE;
    for (i = mu->mu_dfirst; i < mu->mu_dsecond; i++)
    {
        ct = &(mu->mu_coms[i]);
        p = menu_text + ct->ct_begpos;
        MEcopy(ct->ct_name, item_len[i], p);
        p += item_len[i];
	if (ftshmumap)
	    *p++ = '(';
        MEcopy(key_name[i], keyn_len[i], p);
        p += keyn_len[i];
	if (ftshmumap)
	    *p = ')';
    }

    if (mu->mu_dfirst > 0)              /* There's more stuff to the left */
    {
        menu_text[0] = '<';             /* Show arrow pointing left */
        menu_text[1] = '-';
        left_arrow = TRUE;
    }

    if (mu->mu_dsecond < mu->mu_last)   /* There's more to the right */
    {
        menu_text[ring_len - 1] = '>';  /* Show arrow pointing right */
        menu_text[ring_len - 2] = '-';
        right_arrow = TRUE;
    }

    ct = &(mu->mu_coms[item_number]);

    if (menu_state == MUS_ACTIVE)
    {
        MEfill(expl_len, ' ', expl_text);
        expl_text[expl_len] = EOS;

        if (ct->description != NULL)
        {
            i = expl_marg;
            n = STlength(ct->description);
            if (n > expl_len - 2 * expl_marg)
            {
                n = min(expl_len - 2, n);
                i = 1;
            }
            MEcopy(ct->description, n, expl_text + i);
        }

        TDerase(explwin);
        TDaddstr(explwin, expl_text);
        TDhilite(explwin, 0, expl_len - 1, ringateRingAttrExpl);
        TDtouchwin(explwin);
        TDrefresh(explwin);
    }   /* end: (menu_state == MUS_ACTIVE) */

    TDerase(ringwin);               /* Display menu line */
    TDerase(stdmsg);
    TDaddstr(ringwin, menu_text);
    attr = (menu_state == MUS_ACTIVE) ? ringataRingAttrActive
                                      : ringatiRingAttrInact;
    TDhilite(ringwin, 0, ring_len - 1, attr);

    if (menu_state == MUS_ACTIVE)
    {
        if (left_arrow)
            TDhilite(ringwin, 0, 1, ringatrRingAttrArrow);
        if (right_arrow)
            TDhilite(ringwin, ring_len - 2, ring_len - 1, ringatrRingAttrArrow);
        TDhilite(ringwin, ct->ct_begpos - 1, ct->ct_endpos + 1,
                        ringatcRingAttrCursor);
        TDmove(ringwin, 0, ct->ct_begpos - 1);  /* Put window (real) cursor     */
                                                /* at beginning of menu cursor  */
    }

    TDtouchwin(stdmsg);
    TDtouchwin(ringwin);
    if (mu_where != MU_BOTTOM)
	TDrefresh(stdmsg);
    TDrefresh(ringwin);

    curritem = item_number;
}

/*
**  Name:   adjust_ring
**
**  Description:
**
**      Adjusts the menu so that the specified item is visible.
**      adjust_ring does not actually display the menu, but sets
**      mu->mu_dfirst and mu->mu_dsecond.
**
**      adjust_ring also sets up the menu image in menu_text.
**
**  Inputs:
**
**      mu          -   a pointer to a menuType structure
**
**      item_number -   the number of an item (i.e., its index in
**                      the com_tab array).  Must have
**
**                          0 <= item_number < mu->mu_last
**
**      adjustment  -   an adjustment type, which indicates how the
**                      item is to be placed on the screen.  (See
**                      below for details).
**
**  Outputs:
**
**      mu->mu_dfirst   the index of the first item to be displayed
**      mu->mu_dsecond  1 + (the index of the lst item to be displayed)
**      ct->ct_begpos   for ct in the visible part of the menu.
**
**    Returns:  None.
**
**    Exceptions:  None.
**
**  Side Effects:  None.
**
**  History:
**
**      12-jul-89 (fraser)  First written
**
*/

/*
**  Adjustment types:
**
**      LEFT_ADJUST     item_number will be leftmost item on the
**                      screen, unless more items will fit.
**      RIGHT_ADJUST    item_number will be the rightmost item on the
**                      screen, unless more items will fit.
**      MID_ADJUST      item_number will be approximately at the middle
**                      of the screen, unless more will fit.
**      AUTO_ADJUST     item_number will be left- or right-adjusted,
**                      depending on where item_number is in relation
**                      to what's currently on the screen.
**
**  adjust_ring will always display as many menu items as possible.  Thus,
**  there may be unexpected results on occasion.  Since the lengths of
**  menu items vary, there may, for example, be room to add an item on
**  the left side of the menu, when there is no room to add an item on
**  right side.
*/

static VOID
adjust_ring(mu, item_number, adjustment)
MENU    mu;
i4      item_number;
i4      adjustment;
{
    i4          i;
    i4          delta;
    i4          space_left;
    i4          nextpos;
    bool        left_done;
    bool        right_done;
    COMTAB *    ct;

    if (adjustment == AUTO_ADJUST)
        adjustment = item_number < mu->mu_dfirst ? LEFT_ADJUST : RIGHT_ADJUST;

    space_left = ring_len - (2 * ring_marg) + SPACE_BETWEEN;

    switch (adjustment)
    {

    case LEFT_ADJUST:
        mu->mu_dfirst = mu->mu_dsecond = item_number;
        space_left = fill_to_right(mu, space_left);
        fill_to_left(mu, space_left);
        break;

    case RIGHT_ADJUST:
        mu->mu_dfirst = mu->mu_dsecond = item_number + 1;
        space_left = fill_to_left(mu, space_left);
        fill_to_right(mu, space_left);
        break;

    case MID_ADJUST:
        mu->mu_dfirst = mu->mu_dsecond = item_number + 1;
        left_done = right_done = FALSE;
        while ( !left_done || ! right_done)
        {
            if (!left_done)
            {
                i = mu->mu_dfirst - 1;
                if ( !(left_done =
                        (i < 0 ||
                         space_left < (delta = (SPACE_BETWEEN + (2*ftshmumap) +
                                        item_len[i] + keyn_len[i])))))
                {
                    mu->mu_dfirst--;
                    space_left -= delta;
                }
            }
            if (!right_done)
            {
                i = mu->mu_dsecond;
                if ( !(right_done =
                        (i >= mu->mu_last ||
                         space_left < (delta = (SPACE_BETWEEN + (2*ftshmumap) +
                                     item_len[i] + keyn_len[i])))))
                {
                    mu->mu_dsecond++;
                    space_left -= delta;
                }
            }
        }
        break;
    }

    nextpos = ring_marg;
    for (i = mu->mu_dfirst; i < mu->mu_dsecond; i++)
    {
        ct = &(mu->mu_coms[i]);
        ct->ct_begpos = nextpos;
        nextpos += item_len[i] + keyn_len[i] + (2 * ftshmumap);
        ct->ct_endpos = nextpos - 1;
        nextpos += SPACE_BETWEEN;
    }
    while (i < mu->mu_last)
    {
        ct = &(mu->mu_coms[i++]);
        ct->ct_begpos = COLS + 1;
    }
}

/*
**  fill_to_right "fills" the menu line by incrementing mu->mu_dsecond
**  until the space is exhausted or the last item is reached.
*/

static i4
fill_to_right(mu, space_left)
MENU    mu;
i4      space_left;
{
    i4      i;
    i4      delta;
    bool    done;

    done = FALSE;
    while ((i = mu->mu_dsecond) < mu->mu_last && !done)
    {
        delta = item_len[i] + keyn_len[i] + (2 * ftshmumap) + SPACE_BETWEEN;
        if ( !(done = (delta > space_left)) )
        {
            mu->mu_dsecond++;
            space_left -= delta;
        }
    }
    return space_left;
}


/*
**  fill_to_left "fills" the menu line by decrementing mu->mu_dfirst
**  until the space is exhausted or the first item is reached.
*/

static i4
fill_to_left(mu, space_left)
MENU    mu;
i4      space_left;
{
    i4      i;
    i4      delta;
    bool    done;

    done = FALSE;
    while ((i = mu->mu_dfirst - 1) >= 0 && !done)
    {
        delta = item_len[i] + keyn_len[i] + (2 * ftshmumap) + SPACE_BETWEEN;
        if ( !(done = (delta > space_left)) )
        {
            mu->mu_dfirst--;
            space_left -= delta;
        }
    }
    return space_left;
}

/*
**	Dummy "kyint" (keystroke intercept) routine.
*/

static
i4
ringDummyFunc()
{
	return(0);
}

VOID
IIFTsnfSetNbrFields(frfldno)
i4	frfldno;
{
	curFrameNbrFlds = frfldno;
}

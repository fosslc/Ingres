/*
**	mwmenu.c
**	"@(#)mwmenu.c	1.22"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include <compat.h>
# include <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include <adf.h>
# include <fmt.h>
# include <ft.h>
# include <frame.h>
# include <frsctblk.h>
# include <menu.h>
# include <mapping.h>
# include <fe.h>
# include "mwproto.h"
# include "mwmws.h"
# include "mwintrnl.h"

/**
** Name:	mwmenu.c - User Routines to Support MacWorkStation.
**
** Description:
**	File contains routines to user specific MacWorkStation commands.
**	The commands provided here deal with menus.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
**	Routines defined here are:
**
**	  - IIMWcmmCreateMacMenus	Create the default menubar.
**	  - IIMWmiMenuInit		Set local values for FT stuff.
**	  - IIMWpmPutMenu		Display a menu.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	05/22/90 (dkh) - Changed NULLPTR to NULL for VMS.
**	07/10/90 (dkh) - Integrated more changes into r63 code line.
**	08/29/90 (nasser) - allow FRS keys in menu mode.
**	09/16/90 (nasser) - Removed IIMWgmc().
**	06/08/92 (fredb) - Enclosed file in 'ifdef DATAVIEW' to protect
**		ports that are not using MacWorkStation from extraneous
**		code and data symbols.  Had to include 'fe.h' to get
**		DATAVIEW (possibly) defined.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef DATAVIEW

static	char	 *(*mumapfunc)();
static	char	**mumap = NULL;


/*{
** Name:  IIMWmiMenuInit -- Initialize mw menu module.
**
** Description:
**	Initialize the mw menu module.  This involves setting a
**	couple of local values so that the mw module can access
**	function/array defined in FT.
**
** Inputs:
**	flfrsfunc	pointer to FTflfrsmu function
**	map		pointer to menumap array
**
** Outputs:
** 	Returns:
**		None.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	16-nov-89 (nasser)
**		Initial definition.
*/
VOID
IIMWmiMenuInit(flfrsfunc, map)
char	*(*flfrsfunc)();
char   **map;
{
	mumapfunc = flfrsfunc;
	mumap = map;
}

/*{
** Name:  IIMWcmmCreateMacMenus -- Put entries into the Mac menu bar.
**
** Description:
**	Send commands to MWS to add items to the Mac menu bar.
**	INGRES Exec on the Mac builds its own menu bar, and the
**	purpose of this function is to provide the host a
**	mechanism to add more items.
**
** Inputs:
**	None.
**
** Outputs:
** 	Returns:
**		OK/FAIL.
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	25-sep-89 (bspence)
**		Initial definition.
*/
STATUS
IIMWcmmCreateMacMenus()
{
	return(OK);
}

/*{
** Name:  IIMWpmPutMenu -- Display a menu.
**
** Description:
**	Send command to display a menu.
**
** Inputs:
**	mu	Menu to display
**
** Outputs:
** 	Returns:
**		STATUS
** 	Exceptions:
**		None.
**
** Side Effects:
**
** History:
**	20-sep-89 (nasser)
**		Initial definition.
**      31-jan-01 (fruco01)
**		Added i4 countItemsEnabled.  Only add enabled menu items
**              to the send list.  Bug 103805.
*/
STATUS
IIMWpmPutMenu(mu)
MENU	mu;
{
	char		*label = NULL;
	struct com_tab	*comtab;
	i4 		 i;
	i4		 count;
	u_char 		*pText;
	u_char 		*pItem;
	u_char		 theMenu[512];
        i4	        countItemsEnabled;
 	

	if ( ! IIMWmws)
		return(OK);

	if (mu == (MENU) NULL)
		count = 0;
	else
	{
		count = mu->mu_last;
		comtab = mu->mu_coms; 
	}

	pText = theMenu;
        countItemsEnabled = count;


	for (i = 0; i < countItemsEnabled; ++i)
	{
        
                /* Only wish to display a menu trim if it is switched on. */
		/* If any are off then we have one less trim to display.  */

                if( comtab[i].ct_flags & CT_DISABLED )
	        {
                     --count;
		     continue;
	        }

		pItem = (u_char *) comtab[i].ct_name;
		while (*pText++ = *pItem++)
			;
		*(pText - 1) = ',';
		if ((label = (*mumapfunc)(comtab[i].ct_enum)) == NULL)
			label = mumap[i];
		if ((label != NULL) &&
			(STskipblank(label, (i4) STlength(label)) != NULL))
		{
			while (*pText++ = *label++)
				;
			pText--;
		}
		*pText++ = ';';
	}
	*pText = EOS;

	return(IIMWsimSetIngresMenu(count, theMenu));
}


# endif /* DATAVIEW */

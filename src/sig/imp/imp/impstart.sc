/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:	impstart.sc
**
** Start Menu for the IMA based IMP-like tool
**
## History:
##
## 26-jan-94	(johna)
##		started on RIF day from pervious ad-hoc programs
## 12-apr-94	(johna)
##		completed the change_vnode functionality
## 04-may-94	(johna)
##		some comments
## 02-Jul-2002 (fanra01)
##      Sir 108159
##      Added to distribution in the sig directory.
## 04-Sep-03     (mdf040903)                     
##              Commit after calling imp_reset_domain
##      06-Apr-04    (srisu02) 
##              Sir 108159
##              Integrating changes made by flomi02  
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "imp.h"

exec sql include sqlca;
exec sql include 'impcommon.sc';

exec sql begin declare section;

extern int imp_main_menu;
extern int iidbdbConnection;
extern int inFormsMode;
extern char	*currentVnode;
extern int VnodeNotDefined;            
/*
##mdf040903
*/  

MenuEntry MenuList[] = {
	1,	"server_list", 	"Display a list of servers to examine",
	2,	"lock_info", 	"Display locks and related information",
	3,	"log_info",	"Display information on the logging system",
	0,	"no_entry",	"No Entry"
};

exec sql end declare section;


int
startMenu()
{
	exec sql begin declare section;
	int	i;
	char	buf[60];
	char	nodebuf[60];          
/*
##mdf040903
*/
	exec sql end declare section;

	exec frs display imp_main_menu read;
	exec frs initialize;
	exec frs begin;
		/*exec sql select dbmsinfo('ima_server') into :buf;*/
                if (VnodeNotDefined == TRUE) {   
		    exec sql select dbmsinfo('ima_vnode') into :buf;
		    exec frs putform imp_main_menu (vnode = :buf);
		    currentVnode = buf;
                }                                
                else
                {                                
		    exec frs putform imp_main_menu (vnode = :currentVnode);
                }                                
		exec frs inittable imp_main_menu imp_menu read;
		i = 0;
		while (MenuList[i].item_number != 0) {
			exec frs loadtable imp_main_menu imp_menu (
				item_number = :MenuList[i].item_number,
				short_name = :MenuList[i].short_name,
				long_name = :MenuList[i].long_name );
			i++;
		}
	exec frs end;

	exec frs activate menuitem 'Select', frskey4;
	exec frs begin;
		exec frs getrow imp_main_menu imp_menu (:i = item_number);
		switch (i) {
		case 1:	processServerList();
			break;
		case 2: lockingMenu();
			break;
		case 3: loggingMenu();
			break;
		default:
			sprintf(buf," Unknown Option (%d)",i);
			exec frs message :buf with style = popup;
		}
	exec frs end;

	exec frs activate menuitem 'Change_Vnode';
	exec frs begin;
		exec frs prompt ('Enter Vnode: ',:nodebuf);
		exec sql execute procedure imp_reset_domain;
		exec sql execute procedure imp_set_domain(entry = :nodebuf);
		exec sql commit;             
/* 
##mdf040903 
*/
		currentVnode = nodebuf;
		exec frs putform imp_main_menu (vnode = :nodebuf);
		if (iidbdbConnection) {
			closeIidbdbConnection();
			/*
			** defer making the iidbdb connection until
			** we need it
			*/
			/* makeIidbdbConnection(); */
		}
	exec frs end;

	exec frs activate menuitem 'End', frskey3;
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

	return(TRUE);
}


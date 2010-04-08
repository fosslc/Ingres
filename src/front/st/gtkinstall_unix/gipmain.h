/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

/*
** Name: gipmain.h
**
** Description:
**
**      Header file for the Linux GUI installer main() program
**
** History:
**
**    09-Oct-2006 (hanje04)
**	SIR 116877
**      Created.
*/

int
init_new_install_mode			( void );

gint
init_upgrade_modify_mode		( void );

GtkTreeModel *initialize_instance_list	( void );

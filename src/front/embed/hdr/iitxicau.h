/*--------------------------- iitxicau.h -----------------------------------*/
/* Copyright (c) 2004 Ingres Corporation */
/*
** Name: iitxicau.h
**
** Description:
**	Header file for Tuxedo bridge contains entry points for
**      icas-specific utility functions
**
** History:
**	25-Oct-93	(mhughes)
**	First Implementation
**	20-Dec-1993 (mhughes)
**	Added IItux_check_tuxedo_dir to check that tuxedo log file directory
**	exists and is writeable.
**	23-Dec-1993 (mhughes)
**	Changes for icas recovery mode. Add IItux_open_server_log.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef IITXICAU_H
#define IITXICAU_H


/* ICAS Server log file activities */

/* Tests that II_SYSTEM/ingres/files/tuxedo exists and is writeable.
*/
FUNC_EXTERN DB_STATUS
IItux_check_tuxedo_dir();

/* Builds a file and path name string for the icas server log file 
*/
FUNC_EXTERN DB_STATUS
IItux_build_icas_log_file_name( char 		*icas_log_file_p );

/* Builds a file and path name string for the icas server log file 
*/
FUNC_EXTERN DB_STATUS
IItux_open_server_log( char	*server_log_file_name,
		       char	*mode,
		       FILE	**server_log_file_ptr,
		       LOCATION *server_log_file_loc );

/* Flush all currently active servers to the server log file 
*/
FUNC_EXTERN DB_STATUS
IItux_flush_icas_svrs_to_file( FILE 		   *icas_svr_log_file_p,
			       IITUX_ICAS_SVR_HEAD *icas_svr_list_main_cb_p );



/* Manage Registration & Un-registration of servers (and association 
** of servers with XIDs) 
*/

/* Add a new server to the ICAS server list 
*/
FUNC_EXTERN DB_STATUS
IItux_icas_register_new_server( IITUX_ICAS_SVR_HEAD *icas_svr_list_main_cb_p,
			        i4	            server_id );

/* Remove a defunct server from the ICAS server list
*/
FUNC_EXTERN DB_STATUS
IItux_icas_unregister_server( IITUX_ICAS_SVR_HEAD *icas_server_list_header,
			      i4           	  server_id );

/* ICAS startup. When an AS returns a list of active XID's, record that
** the XID is active and that the AS is working on that XID
*/
FUNC_EXTERN DB_STATUS
IItux_icas_register_XIDs( struct IITUX_XID_LIST *icas_as_xid_list,
			  IITUX_ICAS_SVR_HEAD   *icas_server_list_header );

/* Add this SVN to the list of SVNs working upon an XID
*/
FUNC_EXTERN DB_STATUS
IItux_add_SVN_to_XID( IITUX_ICAS_SVR_HEAD *icas_server_list_header,
		      IICX_CB		  *icas_xn_cx_cb_p, 
		      i4		  server_id);



/* Low-Level server list management functions  
*/

/* Create and initialise the server list header cb 
*/
FUNC_EXTERN DB_STATUS
IItux_init_icas_server_list( IITUX_ICAS_SVR_HEAD **icas_svr_list_main_p_p );

/* Free the server list header cb 
*/
FUNC_EXTERN DB_STATUS
IItux_free_icas_server_list( IITUX_ICAS_SVR_HEAD *icas_svr_list_main_p );

/* Logically delete all icas_svr_cb list elements, given list head
** Free server cbs from either active server list, or an xid server list
** return to icas_svr_list_head free_cb list.
*/
FUNC_EXTERN VOID
IItux_delete_all_svr_cbs( IITUX_ICAS_SVR_HEAD	*icas_svr_list_main_cb_p,
			  IITUX_ICAS_SVR	**icas_svr_list_head );

/* Return a pointer to the server cb of a given server number
** in a particular server list
*/
FUNC_EXTERN VOID
IItux_find_icas_svr_cb( i4  		server_id,
		        IITUX_ICAS_SVR  *icas_svr_list_head_p,
		        IITUX_ICAS_SVR	**icas_svr_cb_p_p );

#endif /* ifndef IITXICAU_H */
/*--------------------------- iitxicau.h -----------------------------------*/



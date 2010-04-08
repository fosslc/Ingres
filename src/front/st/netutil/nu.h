/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**      23-mar-1999 (hweho01)
**        Added prototype declaration for function *nu_locate_attr().  
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-Aug-2005 (fanra01)
**      Bug 115031
**      Increase value of MAX_TXT from 80. Character arrays defined using
**      the MAX_TXT are suffering buffer overrun when used in conjunction
**      with vnodeless connection strings, notably lpstring in create_auth.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added 3 prototypes to eliminate gcc 4.3 warnings.
*/



#define MAX_TXT	256

/*
**  Values for "whichtable" -- which table contains the cursor?
*/

# define NODE_TBL 0
# define AUTH_TBL 1
# define CONN_TBL 2
# define ATTR_TBL 3

/*
**  Flag values for authorization edit -- which may we edit?
*/

# define AUTH_NEW 1
# define AUTH_EDIT 2
# define AUTH_EDITPW 3

/*
**  Return statuses from edit...
*/

# define EDIT_OK 0
# define EDIT_CANCEL 1
# define EDIT_DUPLICATE 2
# define EDIT_EMPTY 3	


typedef struct _VNODE
{
    char *name;
    char *glogin;
    char *plogin;
    char *uid;
    LIST connList;
    LIST attrList;		/* List to hold attribute information */
} VNODE;

typedef struct _CONN
{
    bool private;
    char *address;
    char *protocol;
    char *listen;
} CONN;

typedef struct _ATTR
{
    bool private;
    char *attr_name;
    char *attr_value;
} ATTR;

# define INST_PWD_LOGIN ERx("*")

/*
** Function prototypes...
*/


FUNC_EXTERN STATUS	
  nu_authedit( i4  function, 
	       char *vnode, char *login, char *password, char *prompt_word ); 

FUNC_EXTERN VOID	nu_change_auth(char *node, bool privflag, char *login );

FUNC_EXTERN VOID	
  nu_change_conn( CONN *conn, i4  priv,
		  char *address, char *listen, char *protocol ); 

FUNC_EXTERN char *	nu_comsvr_list(bool flag);

FUNC_EXTERN char *	nu_brgsvr_list( bool flag ); /* For Protocol Bridge */

FUNC_EXTERN STATUS	nu_data_init( char *host ); 

FUNC_EXTERN VOID 	nu_destroy_conn( char *vnode ); 

FUNC_EXTERN VOID	nu_destroy_vnode( char *vnode ); 

FUNC_EXTERN VOID	nu_file( char *filename ); 

FUNC_EXTERN bool	nu_is_private( char *pg ); 

FUNC_EXTERN CONN *	
  nu_locate_conn( char *vnode, i4  private, 
		  char *address, char *listen, char *protocol, i4  *row ); 

FUNC_EXTERN VOID	
  nu_new_conn( char *node, i4  priv,
	       char *address, char *listen, char *protocol ); 

FUNC_EXTERN VOID	nu_remove_conn( char *vnode ); 

FUNC_EXTERN VOID	nu_rename_vnode( char *node, char *newname ); 

FUNC_EXTERN VOID	
  nu_vnode_auth( char *vn_name, char **gl, char **pl ,
		 char **ud ); 

FUNC_EXTERN CONN *	
  nu_vnode_conn( bool start_flag, i4  *priv, char *vn_name, 
		 char **addr, char **list, char **prot ); 

FUNC_EXTERN char *	nu_vnode_list( bool start_flag ); 

FUNC_EXTERN STATUS	
  nv_add_auth( i4  private, char *rem_vnode, char *rem_user, char *rem_pw ); 

FUNC_EXTERN STATUS	
  nv_add_node( i4  private, char *rem_name, char *netware, 
	       char *rem_nodeaddr, char *rem_listenaddr ); 

FUNC_EXTERN STATUS	
  nv_del_auth( i4  private, char *rem_vnode, char *rem_user ); 

FUNC_EXTERN STATUS	
  nv_del_node( i4  private, char *rem_name, char *netware, 
	       char *rem_nodeaddr, char *rem_listenaddr ); 

FUNC_EXTERN STATUS	nv_init( char *host ); 

FUNC_EXTERN STATUS	
  nv_merge_node( i4  private, char *rem_name, char *netware, 
		 char *rem_nodeaddr, char *rem_listenaddr ); 

FUNC_EXTERN char *	nv_response( ); 

FUNC_EXTERN STATUS	
  nv_show_auth( i4  private, char *rem_vnode, char *rem_user ); 

FUNC_EXTERN STATUS	nv_show_comsvr();

FUNC_EXTERN STATUS	nv_show_brgsvr();   /* Protocol Bridge server */

FUNC_EXTERN STATUS	
  nv_show_node( i4  private, char *rem_name, char *netware, 
		char *rem_nodeaddr, char *rem_listenaddr ); 

FUNC_EXTERN STATUS	nu_shutdown(bool quiesce_flag, char *csid );

FUNC_EXTERN STATUS 	nv_shutdown(bool quiesce_flag, char *csid);

FUNC_EXTERN i4		nv_status( ); 

FUNC_EXTERN STATUS	nv_test_host( char *host ); 

FUNC_EXTERN ATTR *	
  nu_vnode_attr( bool start_flag, i4  *priv, char *vn_name, 
		 char **name, char **value ); 
FUNC_EXTERN STATUS	
  nv_merge_attr( i4  private, char *vnode, char *name, char *value ); 

FUNC_EXTERN STATUS	
  nv_del_attr( i4  private, char *vnode, char *name, char *value ); 

FUNC_EXTERN VOID	
  nu_change_attr( ATTR *conn, i4  priv, char *name, char *value ); 

FUNC_EXTERN VOID	nu_remove_attr( char *vnode ); 

FUNC_EXTERN STATUS	
  nv_show_attr( i4  private, char *vnode, char *name, char *value ); 

FUNC_EXTERN VOID	
  nu_new_attr( char *node, i4  priv, char *name, char *value );

FUNC_EXTERN ATTR *	
  nu_locate_attr( char *vnode, i4  private, 
		  char *name, char *value, i4  *row ); 

FUNC_EXTERN i4	nu_attredit();
FUNC_EXTERN i4	nu_connedit(bool, char *cnode, bool *cpriv,
                char *caddr, char *clist, char *cprot);
FUNC_EXTERN VOID nu_destroy_attr();

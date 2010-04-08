/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <iplist.h>
# include <er.h>
# include <st.h>
# include "nu.h"

/*
**  Name: nudata.c -- functions that control the internal data structure
**
**  Description:
**	The data on the name server database is maintained internally
**	by netutil in a hierarchy of linked lists, which mirror the 
**	organization of the main netutil form.  A master list, vnodeList,
**	contains one entry per vnode, corresponding to the vnode table
**	field.  Each vnode entry contains one global login name and one
**	private login name (passwords are not stored in the internal
**	structure), and a list header to a list of connection entries.
**	Each connection list entry contains a private/global flag, and
**	strings representing the network address, protocol name, and listen
**	address for the connection.  The lists are maintained in the
**	order -- alphabetical, mostly -- in which they're displayed.
**
**  Entry points:
**	nu_data_init() -- initialize the internal data structure
**	nu_rename_vnode() -- change vnode name in the internal list
**	nu_change_auth() -- change login name
**	nu_destroy_vnode() -- destroy list entry for a vnode name
**	nu_destroy_conn() -- destroy an entire connection list
**	nu_destroy_attr() -- destroy an entire attribute list
**	nu_remove_conn() -- remove a single connection list entry
**	nu_remove_attr() -- remove a single attribute list entry
**	nu_change_conn() -- change the data in a connection list entry
**	nu_change_attr() -- change the data in a attribute list entry
**	nu_new_conn() -- create a new connection list entry
**	nu_new_attr() -- create a new attribute list entry
**	nu_vnode_list() -- return each vnode name in the list
**	nu_vnode_auth() -- return login IDs from a vnode list entry
**	nu_vnode_conn() -- scan through connection list
**	nu_comsvr_list() -- scan through list of comm servers from iigcn
**	nu_vnode_attr() -- scan through attributes list
**
**  History:
**	30-jun-92 (jonb)
**	    Created.
**	22-mar-93 (jonb)
**	    Made calles to ip_list_* stuff match protocols.
**	29-mar-93 (jonb)
**	    Comments.
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**	04-Oct-1993 (joplin)_
**	    Moved the test function into nuaccess.  Added uid retrieval
**	    from name server responses.
**	07-Feb-1996 (rajus01)
**	    Added nu_brgsvr_list() function for Protocol Bridge Server.
**	 3-Sep-96 (gordy)
**	    Check status from rv_init() (now does NS bedcheck).
**	20-feb-97 (mcgem01)
**	    The uid element in the vnode structure is not used when
**	    processing global auth entries but since the global auth data 
**	    is processed after the private data we wind up overwriting 
**	    our user id with an *. We should only set the uid for private 
**	    auth entries.
**	21-Aug-97 (rajus01)
**	    Added nu_add_attr(), nu_new_attr(), nu_vnode_attr(),
**	    nu_change_attr(), nu_destroy_attr(), nu_remove_attr()
**	    routines.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

static LIST vnodeList;
static VOID nu_add_conn();
static VOID nu_add_attr();
static VNODE *nu_locate_vnode(char *node, bool create_flag);

# define EMPTY ERx("")

/*  nu_data_init() -- initialize the internal data structure
**
**  Reads all name server data for the specified host, and fills the
**  internal lists with corresponding information.
**
**  Returns OK unless there was a problem connecting to the name server
**  on the specified host.
*/

STATUS
nu_data_init(char *host)
{
    i4  priv;
    char *node, *address, *protocol, *listen, *login, *uid;
    char *attr_name, *attr_value;
    VNODE *vn;
    STATUS rtn;

    ip_list_init(&vnodeList);  /* Initialize a new linked list */

    if (OK != (rtn = nv_init(host)))	       /* Connect to the name server */
	return rtn;

    /* The name server requires that requests for private data be segregated
       from global requests, so first we'll do all the private stuff and then
       all the global stuff... */

    for (priv=1; priv >=0; priv--)
    {
	/* Request all connection data for the current type (private/global) */

        if (OK != (rtn = nv_show_node(priv,EMPTY,EMPTY,EMPTY,EMPTY)))
	    break;
        for (;;)  /* Read a series of responses */
	{
	    node = nv_response();
	    address = nv_response();
	    protocol = nv_response();
	    listen = nv_response();
	    if (node==NULL || address==NULL || protocol==NULL || listen==NULL)
	        break;
	    nu_new_conn(node,priv,address,listen,protocol);  /* Add to list */
	}

	/* Request all attribute data for the current type (private/global) */
        if( OK != ( rtn = nv_show_attr( priv, EMPTY, EMPTY, EMPTY ) ) )
	    break;
        for ( ;; )  /* Read a series of responses */
	{
	    node  	= nv_response();
	    attr_name	= nv_response();
	    attr_value	= nv_response();
	    if( node == NULL || attr_name == NULL || attr_value == NULL )
	        break;
	    nu_new_attr( node, priv, attr_name, attr_value );  /* Add to list */
	}

	/* Request all login data for the current type (private/global) */

	if (OK != (rtn = nv_show_auth(priv,EMPTY,EMPTY)))
	    break;
	for (;;)  /* Read a series of responses */
	{
	    node = nv_response();
	    login = nv_response();
	    uid = nv_response();
	    if (node==NULL || login==NULL)
	        break;
	    /* Locate or create the vnode list entry... */
	    vn = nu_locate_vnode(node,TRUE);
	    /* Drop the new login ID into the list entry... */
	    ip_list_assign_string( priv? &vn->plogin: &vn->glogin, login);
	    if (priv)
	        ip_list_assign_string( &vn->uid, uid);
        }
    }
    return rtn;
}

/*  nu_rename_vnode() -- change vnode name in the internal list
**
**  This function changes the name of a vnode.  It only appears in one
**  place, the vnodeList, so all we've gotta do is unlink the entry from
**  the list, change the name, and relink it in its new alphabetical
**  position.
*/

VOID
nu_rename_vnode(char *node, char *newname)
{
    VNODE *vn = nu_locate_vnode(node,FALSE);
    ip_list_unlink(&vnodeList);
    ip_list_assign_string(&vn->name, newname);
    (VOID) nu_locate_vnode(newname,FALSE);
    ip_list_insert_before(&vnodeList,(PTR)vn);
}

/*  nu_change_auth() -- change login name
**
**  This function changes either a global or private login name for a 
**  specified vnode to a new name.
*/

VOID
nu_change_auth(char *node, bool privflag, char *login)
{
    VNODE *vn = nu_locate_vnode(node,TRUE);
    char *resp;

    if (vn != NULL)
    {
	if (privflag)
	{
	    ip_list_assign_string( &vn->plogin, login );

	    /* For private auth's, determine the uid the entry is for */ 
	    if (login != NULL && OK == nv_show_auth(privflag, node, login))
	    {
	    	(void)nv_response();
	    	(void)nv_response();
		if ( NULL != (resp = nv_response()) )
		    ip_list_assign_string( &vn->uid, resp );
	    }
	}
	else
	{
	    ip_list_assign_string( &vn->glogin, login );
	}
    }
}

/*  nu_destroy_vnode() -- destroy list entry for a vnode name
**
**  This function removes the specified vnode and all related information
**  from the internal data structure.
*/

VOID
nu_destroy_vnode(char *vnode)
{
    VNODE *vn = nu_locate_vnode(vnode,FALSE);
    if (vn != NULL)
	ip_list_remove(&vnodeList); /* This does not remove the connect list! */
}

/*  nu_destroy_conn() -- destroy an entire connection list 
**
**  This function destroys the entire list of connection entries for the
**  specified vnode name.
*/

VOID 
nu_destroy_conn(char *vnode)
{
    VNODE *vn = nu_locate_vnode(vnode,FALSE);
    if (vn != NULL)
    { 
	ip_list_destroy(&vn->connList);
    }
}

/*  nu_remove_conn() -- remove a single connection list entry
**
**  This function removes a single connection list entry from the connection
**  list for the indicated vnode.  The list entry to be removed must be the
**  "current" entry, i.e. the one last located by nu_locate_conn.
*/

VOID
nu_remove_conn(char *vnode)
{
    VNODE *vn = nu_locate_vnode(vnode,FALSE);
    if (vn != NULL)
    { 
        ip_list_remove(&vn->connList);
    }
}

/*  nu_change_conn() -- change the data in a connection list entry
**
**  The data in the current connection list entry is changed to new data
**  as indicated by the input parameters.  Note that this function takes
**  a pointer to the connection entry itself as an argument, not a vnode
**  name like virtually every other function in this file.
*/

VOID
nu_change_conn(CONN *conn, i4  priv, 
	       char *address, char *listen, char *protocol)
{
    conn->private = priv;
    ip_list_assign_string(&conn->address, address);
    ip_list_assign_string(&conn->listen, listen);
    ip_list_assign_string(&conn->protocol, protocol);
}


/*  nu_new_conn() -- create a new connection list entry
**
**  This function creates a new connection list entry, fills it with
**  the information passed in the parameters, and links it into the
**  connection list for the specified vnode.
*/

VOID
nu_new_conn(char *node, i4  priv, char *address, char *listen, char *protocol)
{
    CONN *conn;
    VNODE *vn = nu_locate_vnode(node,TRUE);
    conn = (CONN *) ip_alloc_blk(sizeof(CONN));
    conn->address = conn->protocol = conn->listen = NULL;
    conn->private = priv;
    ip_list_assign_string(&conn->address, address);
    ip_list_assign_string(&conn->listen, listen);
    ip_list_assign_string(&conn->protocol, protocol);
    nu_add_conn(conn, vn);
}

/*  nu_add_conn() -- add a connection list entry to a list
**
**  This routine adds a pre-filled connection list entry to the connection
**  list for a specified vnode.  Its two parameters are a pointer to the
**  new connection list entry, and a pointer to the list entry for the
**  vnode.  This function is responsible for the ordering of the connection
**  list.
*/

static VOID
nu_add_conn(CONN *conn, VNODE *vn)
{
    CONN *cp;
    register i4  comp;

    SCAN_LIST(vn->connList, cp)
    {
	comp = STbcompare(conn->address,0,cp->address,0,FALSE);
	if (comp == 0)
	{
	    comp = STbcompare(conn->protocol,0,cp->protocol,0,FALSE);
	    if (comp == 0)
	    {
	        comp = STbcompare(conn->listen,0,cp->listen,0,FALSE);
	    }
	}
	if (comp == 0) return;
	if (comp < 0) break;
    }
    ip_list_insert_before(&vn->connList,(PTR)conn);
}


/*  nu_locate_vnode() -- locate and/or create vnode list entry
**
**  This function will find the vnode list entry which corresponds to
**  the vnode name specified as an argument, and return a pointer to
**  the entry.  The second parameter, create_flag, is a boolean which
**  tells the function whether a new entry is to be created if it isn't
**  found.  If create_flag is TRUE, the entry will be created.  If 
**  create_flag is FALSE and the list entry is not found, NULL is
**  returned; otherwise, the function returns a pointer to the list
**  entry.
*/

static VNODE *
nu_locate_vnode(char *node, bool create_flag)
{
    VNODE *vn;
    register i4  comp;

    /* Scan through the master list looking for a match... */

    SCAN_LIST(vnodeList,vn)
    {
	comp = STbcompare(node,0,vn->name,0,FALSE);
	if (comp == 0) return vn;
	if (comp < 0) break;  /* List is ordered, so we can stop now. */
    }

    /* Didn't find it... */

    if (!create_flag)
	return NULL;

    /* Create a new list entry... */

    vn = (VNODE *) ip_alloc_blk(sizeof(VNODE));
    vn->name = NULL;
    /* ip_list_assign_string(&vn->name, node); */
    vn->name = STalloc(node);
    vn->glogin = vn->plogin = vn->uid = NULL;
    ip_list_init( &vn->connList );
    ip_list_init( &vn->attrList );
    /* Our current position in the list is the first entry that's lexically
       greater than the one we're inserting, so we just want the new entry
       to go right where we're pointing in the list... */
    ip_list_insert_before(&vnodeList,(PTR)vn); 
    return vn;
}


/*  nu_vnode_list() -- return each vnode name in the list
**
**  This function makes it easy for a calling program to write a loop
**  over every vnode name in the data structure.  The single parameter,
**  start_flag, is a boolean which controls whether the function will
**  return the first vnode on the list or the next one.  If start_flag
**  is TRUE, the list is rewound and a character pointer to the name of
**  the first vnode is returned.  If it's FALSE, the next vnode name on
**  the list is returned.  NULL is returned at the end of the list.
*/

char *
nu_vnode_list(bool start_flag)
{
    VNODE *vn;
    if (start_flag)
        ip_list_rewind(&vnodeList);
    if ( NULL == ip_list_scan(&vnodeList, ((PTR *)&vn)) ) 
	return NULL;
    return vn->name;
}


/*  nu_vnode_auth() -- return login IDs from a vnode list entry
**
**  This function finds the list entry for a virtual node, and sets two
**  character pointers to point to the global and private login names
**  and the uid for the specified vnode.
*/

VOID
nu_vnode_auth(char *vn_name, char **gl, char **pl, char **ud)
{
    VNODE *vn = nu_locate_vnode(vn_name,FALSE);
    if (vn == NULL)  /* No such vnode? */
    {
        *gl = *pl = *ud = NULL;
	return;
    }
    *gl = vn->glogin;
    *pl = vn->plogin;
    *ud = vn->uid;
    return;
}

/*  nu_vnode_conn() -- scan through connection list
**
**  This routine allows a calling program to contain a loop which receives
**  the information from each connection entry, in order, for a specified
**  vnode name.  
**
**  Inputs:
**
**    start_flag -- boolean, specifies whether the first (TRUE) or the
**                  next (FALSE) connection entry is to be returned.
**    vn_name -- name of the vnode whose connection list is scanned
**    priv -- pointer to a i4  set to private/global flag for each connection 
**            entry in the list.
**    addr -- pointer to char* set to point to the net address for each
**	      connection entry in the list.
**    list -- pointer to char* set to point to the listen address for each
**	      connection entry in the list.
**    prot -- pointer to char* set to point to the protocol name for each
**	      connection entry in the list.
**
**  Returns:
**
**    In addition to setting the things pointed at by the input parameters
**    as described above, returns a pointer to the actual connection list
**    entry.  Returns NULL if there are no further entries in the list.
*/

CONN *
nu_vnode_conn(bool start_flag, i4  *priv, char *vn_name,
              char **addr, char **list, char **prot)
{
    CONN *conn;
    VNODE *vn = nu_locate_vnode(vn_name,FALSE);
    if (vn == NULL)
	return NULL;
    if (start_flag)
        ip_list_rewind(&vn->connList);
    if ( NULL == ip_list_scan(&vn->connList, ((PTR *)&conn)) )
	return NULL;
    *addr = conn->address;
    *list = conn->listen;
    *prot = conn->protocol;
    *priv = conn->private;
    return conn;
}

/*  nu_vnode_attr() -- scan through attribute list
**
**  This routine allows a calling program to contain a loop which receives
**  the information from each attribute entry, in order, for a specified
**  vnode name.  
**
**  Inputs:
**
**    start_flag -- boolean, specifies whether the first (TRUE) or the
**                  next (FALSE) attribute entry is to be returned.
**    vn_name 	 -- name of the vnode whose attribute list is scanned
**    priv 	 -- pointer to a i4  set to private/global flag for each
**		    attribute entry in the list.
**    name 	 -- pointer to char* set to point to the attribute name for
**		    each attribute entry in the list.
**    value 	 -- pointer to char* set to point to the attribute value
**		    for each attribute entry in the list.
**
**  Returns:
**
**    In addition to setting the things pointed at by the input parameters
**    as described above, returns a pointer to the actual attribute list
**    entry.  Returns NULL if there are no further entries in the list.
*/
ATTR *
nu_vnode_attr( bool start_flag, i4  *priv, char *vn_name,
              char **name, char **value )
{
    ATTR 	*attr;
    VNODE 	*vn = nu_locate_vnode(vn_name,FALSE);

    if( vn == NULL )
	return NULL;
    if( start_flag )
        ip_list_rewind( &vn->attrList );
    if( NULL == ip_list_scan( &vn->attrList, ( (PTR *)&attr ) ) )
	return NULL;
    *name  = attr->attr_name;
    *value = attr->attr_value;
    *priv  = attr->private;
    return attr;
}

/* nu_comsvr_list -- return an ID for each operational communication server.
**
** The "start_flag" parameter is TRUE if this is the first in the
** series of calls, FALSE otherwise.  Returns a pointer to a character
** string for the next comm server ID in the list; returns NULL when there
** are no more servers in the list.
*/

char *
nu_comsvr_list(bool start_flag)
{
    if (start_flag && OK != nv_show_comsvr())
	return NULL;

    return nv_response();
}


/* nu_brgsvr_list -- return an ID for each operational bridge server.
**
** The "start_flag" parameter is TRUE if this is the first in the
** series of calls, FALSE otherwise.  Returns a pointer to a character
** string for the next bridge server ID in the list; returns NULL when there
** are no more servers in the list.
*/

char *
nu_brgsvr_list( bool start_flag )
{
    if ( start_flag && OK != nv_show_brgsvr() )
	return NULL;

    return nv_response();
}


/*  nu_change_attr() -- change the data in a attribute list entry
**
**  The data in the current attribute list entry is changed to new data
**  as indicated by the input parameters.  Note that this function takes
**  a pointer to the attribute entry itself as an argument, not a vnode
**  name like virtually every other function in this file.
*/
VOID
nu_change_attr( ATTR *attr, i4  priv, char *name, char *value )
{
    attr->private = priv;
    ip_list_assign_string( &attr->attr_name, name );
    ip_list_assign_string( &attr->attr_value, value );
}

/*  nu_remove_attr() -- remove a single attribute list entry
**
**  This function removes a single attribute list entry from the attribute
**  list for the indicated vnode.  The list entry to be removed must be the
**  "current" entry, i.e. the one last located by nu_locate_attr().
*/
VOID
nu_remove_attr( char *vnode )
{
    VNODE *vn = nu_locate_vnode( vnode, FALSE );
    if ( vn != NULL )
        ip_list_remove( &vn->attrList );
}

/*  nu_destroy_attr() -- destroy an entire attribute list 
**
**  This function destroys the entire list of attribute entries for the
**  specified vnode name.
*/
VOID 
nu_destroy_attr( char *vnode )
{
    VNODE *vn = nu_locate_vnode( vnode, FALSE );
    if (vn != NULL)
	ip_list_destroy( &vn->attrList );
}

/*  nu_new_attr() -- create a new attribute list entry
**
**  This function creates a new attribute list entry, fills it with
**  the information passed in the parameters, and links it into the
**  attribute list for the specified vnode.
*/
VOID
nu_new_attr( char *node, i4  priv, char *name, char *value )
{
    ATTR *attr;
    VNODE *vn;
    vn   = nu_locate_vnode( node, TRUE );
    attr = (ATTR *) ip_alloc_blk( sizeof( ATTR ) );
    attr->attr_name = attr->attr_value = NULL;
    attr->private   = priv;
    ip_list_assign_string( &attr->attr_name, name );
    ip_list_assign_string( &attr->attr_value, value );
    nu_add_attr( attr, vn );
}

/*  nu_add_attr() -- add an attribute list entry to a list
**
**  This routine adds a pre-filled attribute list entry to the  attribute
**  list for a specified vnode.  Its two parameters are a pointer to the
**  new attribute list entry, and a pointer to the list entry for the
**  vnode.  This function is responsible for the ordering of the attribute
**  list.
*/
static VOID
nu_add_attr( ATTR *attr, VNODE *vn, char *vnode )
{
    ATTR 	*ap;
    ATTR 	*scan_ap;
    register i4  comp;

    SCAN_LIST( vn->attrList, ap )
    {
	comp = STbcompare( attr->attr_name, 0, ap->attr_name, 0, FALSE );
	if( comp == 0 )
	    comp = STbcompare( attr->attr_value, 0, ap->attr_value, 0, FALSE );
	if( comp == 0 ) return;
	if( comp < 0 ) break;
    }
    ip_list_insert_before( &vn->attrList, (PTR)attr );
}

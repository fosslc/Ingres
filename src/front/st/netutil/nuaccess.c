/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <st.h>
# include <er.h>
# include <erst.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <gca.h>
# include <gcn.h>
# include <gcu.h>
# include <iplist.h>

/*
**  Name: nuaccess -- functions providing access to the name server data base
**
**  Entry points:
**
**    nv_add_auth -- Add an authorization entry.
**    nv_add_node -- Add a new vnode entry.
**    nv_add_attr -- Add a new attribute entry.
**    nv_del_auth -- Delete an authorization entry.
**    nv_del_node -- Delete a vnode entry.
**    nv_del_attr -- Delete a attribute entry.
**    nv_init -- Initialize interface to the name server database.
**    nv_merge_node -- Add new information to a vnode entry.
**    nv_merge_attr -- Add new attribute information to a vnode entry.
**    nv_response -- Get a line of response information.
**    nv_show_auth -- Request information about an authorization entry.
**    nv_show_comsvr -- Request a list of comm server IDs.
**    nv_show_node -- Request information about a vnode entry.
**    nv_show_attr -- Request information about an attribute entry.
**    nv_shutdown -- Quiesce or shut down a communication server.
**    nv_status -- Return status from the last operation.
**    nv_test_host -- Test a connection to an alternate host.
**
**  History:
**	30-jun-92 (jonb)
**	    Created.
**      18-feb-93 (jonb)
**          Changed reference to a private GCA header file.
**	29-mar-93 (jonb)
**	    Added a few comments.
**	30-mar-93 (jonb)
**	    Fix test function to restore host name properly.
**      02-sep-93 (joplin)
**          Added the required GCN_UID_FLAG request flag when impersonating
**          another user, fixed lost GCA_ERRORs, and handle name server
**          error messages that were otherwise display to standard out.  
**          Changed test function to call gcn_testaddr().
**	29-Nov-95 (gordy)
**	    Standardize the name server client interface.
**	07-Feb-96 (rajus01)
**	    Added nv_show_brgsvr() for Protocol Bridge Server.
**	10-apr-96 (chech02)
**	    In nv_init(), need to call to IIgcn_check() for win 3.1 port.
**	 3-Sep-96 (gordy)
**	    Call IIgcn_check() unconditionally, the local Name Server must
**	    be running.
**	26-Feb-97 (gordy)
**	    Some gcn utility functions moved to gcu.
**	21-Aug-97 (rajus01)
**	    Added nv_add_attr(), nv_del_attr(), nv_merge_attr, nv_show_attr(),
**	    nv_attr_call() routines.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for requests to Name Server network database.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-mar-2001 (loera01) Bug 104195
**          Initialize gcn_parm.gcn_install with SystemCfgPrefix instead of
**          II_INSTALLATION, or else system-level VMS installations will break.
**	29-Mar-02 (gordy)
**	    Use GCA class definitions rather than GCN.
*/

/*
**  Forward and/or External function references.
*/
static i4	gcn_response();
static i4	gcn_respsh();
static VOID	gcn_resperr();
static i4	gcn_err_func();

GLOBALREF char	username[];

static i4	lastOpStat;
static char	*hostName = NULL;
GLOBALREF bool	is_setuid;

/*
** The next few functions are concerned with handling the text that's
** returned from name server requests.  The text is kept in a list
** of char arrays, one per line.
*/

static LIST respList;  /* The header structure for the list of responses. */
static STATUS nv_add_attr( bool mflag, i4  private, char *vnode,
					char *name, char *value ); 
/*
**  nv_stash_line -- allocate space for a text line, stash it, and add
**  it to the list.
*/

static VOID
nv_stash_line(char *line)
{
    char *str;
    str = (char *) ip_alloc_blk(1+STlength(line));
    STcopy(line,str);
    ip_list_append(&respList,str);
}

/*
**  nv_response -- return one line worth of the currently active response.
*/

char *
nv_response()
{
    char *rtn;
    if (NULL != ip_list_scan(&respList, &rtn))
	return rtn;
    ip_list_destroy(&respList);
    return NULL;
}

/*
**  nv_init -- initialization prior to accessing the name server.
*/

STATUS
nv_init(char *host)
{
    STATUS rtn;

    hostName = host;
    gcn_seterr_func( gcn_err_func );

    if (OK != (rtn = IIGCn_call( GCN_INITIATE, NULL )))
	return rtn;

    if ( (rtn= IIgcn_check()) != OK )
    {
	IIGCn_call( GCN_TERMINATE, NULL );
  	return rtn;
    }

    ip_list_init(&respList);

    return OK;
}

/***************************************************************************/

/*
**  gcn_response -- handle a response from the name server.
**
**  This function is called by the name server to handle the response to
**  any function.  It parses out the text and calls gcn_respsh for each
**  line.
*/

static i4
gcn_response(i4 msg_type, char *buf, i4 len)
{
        i4  row_count, op = 0;

        /* Handle the message responses from name server. */
        switch( msg_type )
        {
            case GCN_RESULT:
                /* Pull off the results from the operation */
                buf += gcu_get_int( buf, &op );
                if ( op == GCN_RET )
                {
	            buf += gcu_get_int( buf, &row_count );
                        while( row_count-- )
	            buf += gcn_respsh( buf );
                }
                lastOpStat = OK;
                break;
 
            case GCA_ERROR:
                /* The name server has flunked the request. */ 
                gcn_resperr(buf, len);
                break;
 
            default:
                break;
        }
        return op;
}

/*
** Name: gcn_respsh() - decode one row of show command output.  Figures
** out what needs to be stashed away based on what sort of command is
** being processed, and calls the text-list handler to save away the
** relevant information.
**
** Returns the number of bytes of the buffer which were processed by this
** call.
*/

static i4
gcn_respsh(char *buf)
{
	char		*type, *uid, *obj, *val;
	char		*pv[5];
	char		*obuf = buf;
	i4 wcnt;

	/* get type, tuple parts */

	buf += gcu_get_str( buf, &type );
	buf += gcu_get_str( buf, &uid );
	buf += gcu_get_str( buf, &obj );
	buf += gcu_get_str( buf, &val );

	if ( !STbcompare( type, 0, GCN_NODE, 0, TRUE ) )
	{
	    nv_stash_line(obj);
	    for( wcnt=gcu_words(val, (char *)NULL, pv, ',', 3 ) ;
		 wcnt > 0; wcnt-- )
	    {
		nv_stash_line(pv[3-wcnt]);
	    }
	} 
	if ( !STbcompare( type, 0, GCN_ATTR, 0, TRUE ) )
	{
	    nv_stash_line( obj );
	    _VOID_ gcu_words( val, (char *)NULL, pv, ',', 3 ) ;
	    nv_stash_line( pv[0] );
	    nv_stash_line( pv[1] );
	} 

	else if ( !STbcompare( type, 0, GCN_LOGIN, 0, TRUE ) )
	{
	    nv_stash_line(obj);
	    _VOID_ gcu_words( val, (char *)NULL, pv, ',', 3 );
	    nv_stash_line(pv[0]);
	    nv_stash_line(uid);
	}

	else if ( !STbcompare( type, 0, GCA_COMSVR_CLASS, 0, TRUE ) )
	{
	    nv_stash_line(val);
	}
	else if ( !STbcompare( type, 0, GCA_BRIDGESVR_CLASS, 0, TRUE ) )
	{
	    nv_stash_line(val);
	}


	return buf - obuf;
}


/*
** Name: gcn_resperr() - decodes a GCA_ERROR from the name server.
** If error code(s) have been placed in the GCA_ER_DATA use the first
** error code as the lastOpStat value to indicate the status of the request.
**
*/

static VOID
gcn_resperr(char *buf, i4 len)
{
    GCA_ER_DATA	    *ptr = (GCA_ER_DATA*) buf;

    if ( ptr->gca_l_e_element > 0 )
        lastOpStat = ptr->gca_e_element[0].gca_id_error; 
}


/*
** Name: gcn_err_func() - handles the error messages sent back from the
** name server.  Since we explicitly handle the messages generated from
** the returned status values, these messages can be tossed.  In fact
** the only reason for having this function is to keep the name server
** routines from displaying error messages to standard out and messing
** up the display forms.
*/
 
static i4
gcn_err_func(char *msg)
{
    return 0 ;
} 

/***************************************************************************/

/* Generic API interfaces */


/*  nv_api_call -- generic call to the name server interface.
**
**  Inputs:
**
**    flag    -- Flag parameter for GCN_CALL_PARMS.
**                 GCN_PUB_FLAG - for global operations
**                 GCN_UID_FLAG - if impersonating another user.
**    obj     -- the thing being operated on.
**    opcode  -- GCN_ADD, GCN_DEL, etc., specifying the operation to be done.
**    value   -- the value being changed to or added.
**    type    -- GCN_NODE or GCN_LOGIN depending on what we're operating on.
*/

static STATUS
nv_api_call(i4 flag, char *obj, i4  opcode, char *value, char *type)
{
    GCN_CALL_PARMS gcn_parm;
    STATUS status;

    gcn_parm.gcn_response_func = gcn_response;  /* Handle response locally. */

    gcn_parm.gcn_uid = username;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = obj;
    gcn_parm.gcn_type = type;
    gcn_parm.gcn_value = value;

    gcn_parm.gcn_install = SystemCfgPrefix;


    gcn_parm.gcn_host = hostName;

    /* Assume GCA request will fail, response handling may update this value */
    lastOpStat = E_ST0050_gca_error;

    status = IIGCn_call(opcode,&gcn_parm);	
    ip_list_rewind(&respList);
    if (status != OK)
        return status;
    else 
        return lastOpStat;
}

/* nv_auth_call -- make a name server interface call for a login operation. */

static STATUS
nv_auth_call(i4 flag, char *obj, i4  opcode, char *value)
{
    return nv_api_call(flag, obj, opcode, value, GCN_LOGIN);
}

/* nv_node_call -- make a name server interface call for a connection 
** operation.
*/

static STATUS
nv_node_call(i4 flag, char *obj, i4  opcode, char *value)
{
    return nv_api_call(flag, obj, opcode, value, GCN_NODE);
}

static STATUS
nv_attr_call( i4  flag, char *obj, i4  opcode, char *value )
{
    return nv_api_call( flag, obj, opcode, value, GCN_ATTR );
}

/* Authorization stuff */

/* nv_add_auth -- add an authorization entry. */

STATUS
nv_add_auth(i4 private, char *rem_vnode, char *rem_user, char *rem_pw)
{
    char encrp_pwd[128];
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    if(rem_pw[0] != EOS)
    	gcu_encode( rem_user, rem_pw, encrp_pwd );
    else
    	encrp_pwd[0] = EOS;
    STpolycat(3,rem_user,",",encrp_pwd,value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;

    return nv_auth_call(flag, rem_vnode, GCN_ADD, value);
}

/* nv_del_auth -- delete an authorization entry.  */

STATUS
nv_del_auth(i4 private, char *rem_vnode, char *rem_user)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    STpolycat(2,rem_user,",",value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;

    return nv_auth_call(flag, rem_vnode, GCN_DEL, value);
}

/* nv_show_auth -- get information about an authorization entry. */

STATUS
nv_show_auth(i4 private, char *rem_vnode, char *rem_user)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    STpolycat(2,rem_user,",",value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;

    return nv_auth_call(flag, rem_vnode, GCN_RET, value);
}


/* Node stuff */

/* mv_add_merge -- add or merge a node entry.  "Adding" is when you 
** add the first entry for a particular vnode; each subsequent record
** for that vnode has to be a "merge".
*/

static STATUS
nv_add_merge(bool mflag, i4  private, 
	     char *rem_name, char *netware, 
	     char *rem_nodeaddr, char *rem_listenaddr)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    STpolycat(5,rem_nodeaddr,",",netware,",",rem_listenaddr,value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;
    if (mflag)
        flag |= GCN_MRG_FLAG;

    return nv_node_call(flag, rem_name, GCN_ADD, value);
}

/* nv_add_node -- add a new vnode */

STATUS
nv_add_node(i4 private, 
	    char *rem_name, char *netware, 
	    char *rem_nodeaddr, char *rem_listenaddr)
{
    return nv_add_merge(FALSE,
                        private,rem_name,netware,rem_nodeaddr,rem_listenaddr);
}

/* nv_merge_node -- add a new entry for an existing vnode. */

STATUS
nv_merge_node(i4 private, 
	      char *rem_name, char *netware, 
	      char *rem_nodeaddr, char *rem_listenaddr)
{
    return nv_add_merge(TRUE,
			private,rem_name,netware,rem_nodeaddr,rem_listenaddr);
}

/* nv_del_node -- delete all information related to a vnode. */

STATUS
nv_del_node(i4 private, 
            char *rem_name, char *netware, 
	    char *rem_nodeaddr, char *rem_listenaddr)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    STpolycat(5,rem_nodeaddr,",",netware,",",rem_listenaddr,value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;

    return nv_node_call(flag, rem_name, GCN_DEL, value);
}

/* nv_show_node -- return information about a vnode. */

STATUS
nv_show_node(i4 private, 
	     char *rem_name, char *netware, 
	     char *rem_nodeaddr, char *rem_listenaddr)
{
    char value[GCN_VAL_MAX_LEN+1];
    u_i4 flag;

    STpolycat(5,rem_nodeaddr,",",netware,",",rem_listenaddr,value);

    flag = GCN_NET_FLAG;
    if (!private)
        flag |= GCN_PUB_FLAG;
    if (is_setuid)
        flag |= GCN_UID_FLAG;

    return nv_node_call(flag, rem_name, GCN_RET, value);
}

/* nv_test_host -- tests a connection to a different vnode.  Uses the name
** function gcn_testaddr() to perform the actual test.
*/

STATUS
nv_test_host(char *host)
{
    char target[GCN_EXTNAME_MAX_LEN + 1]; 
    STATUS rtn;

    STpolycat( 2, host, ERx("::/IINMSVR"), target );
 
    if (is_setuid)
	rtn = gcn_testaddr( target, 0, username );
    else
	rtn = gcn_testaddr( target, 0 , NULL );

    return rtn;
}

/* nv_show_comsvr -- request a list of all comm server IDs. */

STATUS
nv_show_comsvr()
{
    ip_list_init(&respList);
    return( nv_api_call( GCN_PUB_FLAG, ERx(""), GCN_RET, 
			 ERx(""), GCA_COMSVR_CLASS ) );
}

/* nv_show_brgsvr -- request a list of all bridge server IDs. */

STATUS
nv_show_brgsvr()
{
    ip_list_init(&respList);
    return( nv_api_call( GCN_PUB_FLAG, ERx(""), GCN_RET, 
			 ERx(""), GCA_BRIDGESVR_CLASS ) );
}

/* nv_shutdown -- request a shutdown or quiesce operation against the
** specified comm server.  The "quiesce_flag" parameter specifies whether
** this is a quiesce (TRUE) or shutdown (FALSE) operation.
*/

STATUS
nv_shutdown(bool quiesce_flag, char *csid)
{
    STATUS rtn;

    rtn = gcn_fastselect( GCA_NO_XLATE | 
			    (quiesce_flag? GCA_CS_QUIESCE: GCA_CS_SHUTDOWN), 
			  csid );

    if (rtn == E_GC0040_CS_OK)
	rtn = OK;

    return rtn;
}

/* nv_merge_attr -- add a new entry for an existing attribute. */

STATUS
nv_merge_attr( i4  private, 
	      char *vnode, char *name, char *value ) 
{
    return nv_add_attr( TRUE, private, vnode, name, value );
}

/* nv_add_attr -- add an attribute for a vnode. */

static STATUS
nv_add_attr( bool mflag, i4  private, 
	     char *vnode, char *name, char *value ) 
{
    char 	val[GCN_VAL_MAX_LEN+1];
    u_i4 	flag;

    STpolycat( 3, name, ",", value, val );

    flag = GCN_NET_FLAG;
    if( !private )
        flag |= GCN_PUB_FLAG;
    if ( is_setuid )
        flag |= GCN_UID_FLAG;
    if ( mflag )
        flag |= GCN_MRG_FLAG;

    return nv_attr_call( flag, vnode, GCN_ADD, val );
}

/* nv_del_attr -- delete all attribute information related to a vnode. */

STATUS
nv_del_attr( i4  private, 
            char *vnode, char *name, char *value ) 
{
    char 	val[GCN_VAL_MAX_LEN+1];
    u_i4 	flag;

    STpolycat( 3, name, ",", value, val );

    flag = GCN_NET_FLAG;
    if( !private )
        flag |= GCN_PUB_FLAG;
    if( is_setuid )
        flag |= GCN_UID_FLAG;

    return nv_attr_call( flag, vnode, GCN_DEL, val );
}

/* nv_show_node -- return attribute information about a vnode. */

STATUS
nv_show_attr( i4  private, 
	     char *vnode, char *name, char *value ) 
{
    char 	val[GCN_VAL_MAX_LEN+1];
    u_i4 	flag;

    STpolycat( 3, name, ",", value, val );

    flag = GCN_NET_FLAG;
    if( !private )
        flag |= GCN_PUB_FLAG;
    if( is_setuid )
        flag |= GCN_UID_FLAG;

    return nv_attr_call( flag, vnode, GCN_RET, val );
}

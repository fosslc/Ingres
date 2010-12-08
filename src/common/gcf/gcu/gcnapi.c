/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <gc.h>
#include <id.h>
#include <me.h>
#include <nm.h>
#include <qu.h>
#include <st.h>
#include <tm.h>
#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcu.h>
#include <gcnint.h>
#include <gcnapi.h>


/*
** Name:        gcnapi.c
**
** Description:
**      Contains the II_GCNapi_ModifyNode API function for adding, deleting 
**	or editing a vnode.
**
** History:
**      05-oct-98 (mcgem01)
**          Created for the Unicenter team.
**	05-mar-99 (devjo01)
**	    Formally incorporated into Ingres.
**	26-apr-1999 (mcgem01)
**	    Updated to compile with Ingres 2.5
**	04-may-1999 (mcgem01)
**	    Add a node test function, similar to Shift-F5 in Netutil.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-Jan-2005 (fanra01)
**      Sir 113724
**      Suppress default message action by supplying a callback function.
**  11-Jan-2005 (fanra01)
**      Correct minor typos.
**  12-Jan-2005 (fanra01)
**      Manual Cross integration of changes 474444 and 474447.
**  12-Jan-2005 (fanra01)
**      Bug 113735
**      Ensure that password is not part of a delete operation.
**	11-Aug-09 (gordy)
**	    Remove string length restrictions.  Provide a more 
**	    appropriate size of username buffer.
**  15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**      Changes to eliminate compiler prototype warnings.
*/

static char	gUserID[ GC_USERNAME_MAX + 1 ];
static bool	bIngresWasInit = FALSE;
static bool node_found = FALSE;

static int InitGCNInterface (void);
static int gcn_api_call (int flag, PTR obj, int opcode, PTR value, PTR type);
static i4 gcn_err_func (char * msg) { return 0; }
static i4 gcn_api_response(i4 msg_type, char *buf, i4 len);
static i4 gcn_api_silent(i4 msg_type, char *buf, i4 len);
static i4 gcnapi_get_int( char *p, i4 *i );

/*
** Name: gcn_api_silent - suppress default message behaviour
**
** Description:
**  A callback response function passed to gcn_api_call that performs a noop.
**
** Inputs:
**  msg_type        GCA message primitive type (ignored).
**  buf             Response message (ignored).
**  len             Length of response message (ignored).
**
** Outputs:
**  None
**
**  Returns:
**      0
**
**  History:
**      11-01-2005 (fanra01)
**          Created.
*/
static i4
gcn_api_silent(i4 msg_type, char *buf, i4 len)
{
    switch( msg_type )
    {
        case GCN_RESULT:
            break;
        default:
            break;
    }
    return(0);
}

/*
**
**	Name:	II_GCNapi_ModifyNode
**
**	Description:
**
**	This function adds and deletes virtual nodes in the 
**	IILOGIN and IINODE files via the Ingres name server interface.
**
**	opcode values are either ADD_NODE or DELETE_NODE.
**
**	The virtual node name may be concatenated with the database 
**	name (i.e., vnode::database).
**
**	This function returns a status code as:  0 = no error, 
**	0xC0021 = server unavailabe, or some other error number.
**
** History:
**      12-Jan-2005 (fanra01)
**          Ensure that the encrypted password is not appended to the value
**          for a delete operation.
**	11-Aug-09 (gordy)
**	    Declare buffers for standard size operations.  Use dynamic
**	    storage if lengths exceed default size.
*/

int 
II_GCNapi_ModifyNode( int opcode, 
	int    entryType,
	char * virtualNode, 
	char * netAddress, 
	char * protocol, 
	char * listenAddress, 
	char * username, 
	char * password)
{
	char	vbuff[ 64 ];
	char	*vnode = vbuff;
	char	valbuf[ 256 ];
	char	*value = valbuf;
	char	pwdbuf[ 128 ];
	char	*encrypted = pwdbuf;
	int	status;
	int	len;

	status = InitGCNInterface();
	if (status != OK) goto xit;
	
 	len = strcspn(virtualNode, ":");
	
	if ( len >= sizeof( vbuff )  &&
	     ! (vnode = (char *)MEreqmem( 0, len + 1, FALSE, NULL )) )
	{
	    status = E_GC0013_ASSFL_MEM;
	    goto xit;
	}

	strncpy(vnode, virtualNode, len);
	vnode[len] = 0;

    if ((username != NULL) && (password != NULL))
    {
	/*
	** The following is a simplified estimate of the length of the 
	** encrypted password.  It accounts for the padding added to 
	** last block (more significant for short passwords) and the 
	** expansion factor of text conversion (more significant for
	** longer passwords).
	*/
	len = (STlength( password ) + 8) * 2;

	if ( len > sizeof( pwdbuf )  &&
	     ! (encrypted = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	    status = E_GC0013_ASSFL_MEM;
	else
	    status = gcu_encode( username, password, encrypted );
    }
    else
    {
        status = FAIL;
    }
	if (status != OK) goto xit;

	len = STlength( netAddress ) + 
	      STlength( protocol ) + STlength( listenAddress ) + 3;

	if ( len > sizeof( valbuf )  &&
	     ! (value = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	{
	    status = E_GC0013_ASSFL_MEM;
	    goto xit;
	}

	STcopy(netAddress, value);
	STcat(value, ",");
	STcat(value, protocol);
	STcat(value, ",");
	STcat(value, listenAddress);

	status = gcn_api_call(entryType, vnode, opcode, value, GCN_NODE);
	if (status != OK) goto xit;

	if ( value != valbuf )  MEfree( (PTR)valbuf );
	value = valbuf;

    if( opcode == DELETE_NODE )
    {
	len = STlength( username ) + STlength( encrypted ) + 2;
	if ( len > sizeof( valbuf )  &&
	     ! (value = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	{
	    status = E_GC0013_ASSFL_MEM;
	    goto xit;
	}

	STcopy(username, value);
	STcat(value, ",");
        STcat(value, encrypted);
    }
    else
    {
	len = STlength( username ) + 2;
	if ( len > sizeof( valbuf )  &&
	     ! (value = (char *)MEreqmem( 0, len, FALSE, NULL )) )
	{
	    status = E_GC0013_ASSFL_MEM;
	    goto xit;
	}

	STcopy(username, value);
	STcat(value, ",");
    }    
    
	status = gcn_api_call(entryType, vnode, opcode, value, GCN_LOGIN);
	if (status != OK) 
	    goto xit;

	status = IIGCn_call(GCN_TERMINATE, NULL);
	if ( encrypted  &&  encrypted != pwdbuf )  MEfree( (PTR)encrypted );
	if ( value  &&  value != valbuf )  MEfree( (PTR)valbuf );
	if ( vnode  &&  vnode != vbuff )  MEfree( (PTR)vnode );
	return status;

xit:
	IIGCn_call(GCN_TERMINATE, NULL);
	if ( encrypted  &&  encrypted != pwdbuf )  MEfree( (PTR)encrypted );
	if ( value  &&  value != valbuf )  MEfree( (PTR)valbuf );
	if ( vnode  &&  vnode != vbuff )  MEfree( (PTR)vnode );
	return status;
}

/*
**
**      Name:   II_GCNapi_TestNode
**
**      Description:
**		Test the name server connection to a given vnode.
**
** History:
**	11-Aug-09 (gordy)
**	    Use dynamic storage if target string exceeds default size.
*/

int 
II_GCNapi_TestNode( char *vnode) 
{
	char	tbuff[ 65 ];
	char 	*target;
	i4	len;
	int	status;

	len = STlength( vnode ) + 12;
	target = (len <= sizeof( tbuff ))
		 ? tbuff : (char *)MEreqmem( 0, len, FALSE, NULL );

	if ( ! target )  return( E_GC0013_ASSFL_MEM );

	STpolycat( 2, vnode, ERx("::/IINMSVR"), target );

        status = InitGCNInterface();
        if (status != OK)  goto xit;

	status = gcn_testaddr( target, 0 , NULL );
        if (status != OK)  goto xit;

        status = IIGCn_call(GCN_TERMINATE, NULL);
	if ( target != tbuff )  MEfree( (PTR)target );
        return status;

xit:
	IIGCn_call(GCN_TERMINATE, NULL);
	if ( target != tbuff )  MEfree( (PTR)target );
	return status;
}

/*
**
** Name: InitGCNInterface
**
** Description:
**	Initialize the name server client interface.
**
*/
static int 
InitGCNInterface(void)
{
    int	status;
    char	*user;

    IIGCn_call(GCN_TERMINATE, NULL);

    if (! bIngresWasInit)
    {
        status = MEadvise(ME_INGRES_ALLOC);

        if (status != OK) 
            return status;

        SIeqinit();

        status = IIGCn_call(GCN_INITIATE, NULL);

        if (status != OK) 
            return status;

        gcn_seterr_func(gcn_err_func);

        user = gUserID;
        IDname(&user);

        bIngresWasInit = TRUE;
    }
    else
    {
        status = IIGCn_call(GCN_INITIATE, NULL);
        if (status != OK) 
            return status;
    }

    return OK;
}

/*  
**  Name: gcn_api_call 
**
**  Description: generic call to the name server interface scarfed
**		from the nv_api_call function in netutil.
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
**
**  History:
**      11-Jan-2005 (fanra01)
**          Add silent response function as parameter.
*/

static int 
gcn_api_call(int flag, 
	char *obj, 
	int opcode, 
	char *value, 
	char *type)
{
    GCN_CALL_PARMS	gcn_parm;
    int			status;

    if (opcode == ADD_NODE)
	opcode = GCN_ADD;
    else if (opcode == DELETE_NODE)
	opcode = GCN_DEL;

    gcn_parm.gcn_response_func = gcn_api_silent;
    gcn_parm.gcn_uid = gUserID;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = obj;
    gcn_parm.gcn_type = type;
    gcn_parm.gcn_value = value;
    gcn_parm.gcn_host = NULL;

    NMgtAt("II_INSTALLATION", &gcn_parm.gcn_install);

    status = IIGCn_call(opcode, &gcn_parm);
    return status;
}


/*
**
**   Name:   II_GCNapi_Node_Exists
**
**   Description:
**	Test to see if a vnode Exists. This will not 
**	test to see if the vnode is valid. This will
**	only work with globally defined vnodes.
**
**   Inputs:
**
**    vnode    -- Character string of the vnode you wish
**                to  test.
**   Outputs:
**   
**    boolean  -- True if a vnode name exists.
**
**	 History:
**      22-may-2000 (rodjo04)
**          Created for the Unicenter team.
*/

bool 
II_GCNapi_Node_Exists( char *vnode) 
{
    GCN_CALL_PARMS gcn_parm;
    STATUS status;
    char    *username;
        
    char *ntype = "NODE";   /* create local vars instead of a buffer. */
    char *ltype = "LOGIN";  /* We will save calls to STcopy */
    char *nvalue = ",,";
    char *lvalue = ",";     
    int opcode = GCN_RET;  
    node_found = FALSE; /* assume not found */

    status = IIGCn_call(GCN_INITIATE, NULL);

    if (status != OK) 
        return FALSE;

    gcn_parm.gcn_response_func = gcn_api_response;  /* Handle response locally. */

    username = gUserID;
    IDname(&username);

    gcn_parm.gcn_uid = username;
    gcn_parm.gcn_flag = GCN_PUB_FLAG;  
    gcn_parm.gcn_obj = vnode;
    gcn_parm.gcn_type = ltype;
    gcn_parm.gcn_value = lvalue;

   
    gcn_parm.gcn_install = "\0";
    gcn_parm.gcn_host = "\0";

    status = IIGCn_call(opcode,&gcn_parm);  /* check for Login info */

    if (node_found)
    {
        IIGCn_call(GCN_TERMINATE, NULL);
        return (node_found);  /* exit if we find Login info */
    }
     
    gcn_parm.gcn_type = ntype;
    gcn_parm.gcn_value = nvalue;

    status = IIGCn_call(opcode,&gcn_parm);  /* check for Connection info */
   
    IIGCn_call(GCN_TERMINATE, NULL);
    return (node_found);
}
	

/*
**  gcn_api_response -- handle a response from the name server.
**
**  This function is called by the name server to handle the response to
**  function II_GCNapi_Node_Exists.  This is a modified version of gcn_responce
**  used in neutil. It will only test to see if a single row was returned. 
*/

static i4
gcn_api_response(i4 msg_type, char *buf, i4 len)
{
        i4 row_count = 0, op = 0;
        
        /* Handle the message responses from name server. */
        switch( msg_type )
        {
            case GCN_RESULT:
         /*        Pull off the results from the operation  */
                buf += gcnapi_get_int( buf, &op );
                if ( op == GCN_RET )
                {
                    buf += gcnapi_get_int( buf, &row_count );
                    if (row_count != 1)
                        node_found = FALSE;
                    else             
                        node_found = TRUE;
                }
                break;
            default:
                break;
        }
        return op;
}   
	
/*
** Name: gcnapi_get_int() - get integer from buffer.
**
** Description:
**	Gets integer i from buffer p.
**  Ripped from netutil.
**
** Returns:
**	Amount of buffer used.
*/

static i4
gcnapi_get_int( char *p, i4 *i )
{
	i4	i4val;

	I4ASSIGN_MACRO( *p, i4val );

	*i = i4val;

	return sizeof( i4val );
}
	
	
	
	
	
	
	
	
	
	
	
	
	

/*
** Copyright (c) 1998 Ingres Corporation
*/

#include <compat.h>
#include <me.h>
#include <st.h>
#include <gl.h>
#include <gcn.h>
#include <qu.h>
#include <tm.h>
#include <nm.h>
#include <gcnint.h>
#include <idcl.h>
#include "oigcnapi.h"

/*
** Name:        oigcnapi.c
**
** Description:
**      Contains the II_GCNapi_ModifyNode API function for adding, deleting 
**	or editing a vnode.
**
** History:
**      05-oct-98 (mcgem01)
**          Created for the Unicenter team.
**      11-Jan-2005 (fanra01)
**          Sir 113724
**          Suppress default message action by supplying a callback function.
**      11-Jan-2005 (fanra01)
**          Correct minor typos.
**      12-Jan-2005 (fanra01)
**          Bug 113735
**          Ensure that password is not part of a delete operation.
*/

static char	gUserID[GCN_UID_MAX_LEN];
static BOOL	bIngresWasInit = FALSE;

static int InitGCNInterface (void);
static int gcn_api_call (int flag, LPTSTR obj, int opcode, LPTSTR value, LPTSTR type);
static long gcn_err_func (char * msg) { return 0; }
static i4	gcn_api_slient();

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
*/

int 
II_GCNapi_ModifyNode ( int opcode, 
	int    entryType,
	char * virtualNode, 
	char * netAddress, 
	char * protocol, 
	char * listenAddress, 
	char * username, 
	char * password)
{
	char	vnode[GCN_VAL_MAX_LEN];
	char	value[GCN_VAL_MAX_LEN];
	char	encrypted[GCN_VAL_MAX_LEN];
	int	status;
	int	len;

	status = InitGCNInterface();

	if (status != OK) 
	    goto xit;
	
 	len = strcspn (virtualNode, ":");
	strncpy (vnode, virtualNode, len);
	vnode[len] = 0;

    if ((username != NULL) && (password != NULL))
    {
        status = gcn_encrypt ((LPTSTR) username, (LPTSTR) password, encrypted);
    }
    else
    {
        status = FAIL;
    }
	if (status != OK) 
	    goto xit;

	STcopy (netAddress, value);
	STcat (value, ",");
	STcat (value, protocol);
	STcat (value, ",");
	STcat (value, listenAddress);

	status = gcn_api_call (entryType, vnode, opcode, value, GCN_NODE);
	if (status != OK) 
	    goto xit;

	STcopy (username, value);
	STcat (value, ",");
    if(opcode != DELETE_NODE)
    {
        STcat (value, encrypted);
    }

	status = gcn_api_call (entryType, vnode, opcode, value, GCN_LOGIN);
	if (status != OK) 
	    goto xit;

	status = IIGCn_call (GCN_TERMINATE, NULL);
	return status;

xit:
	IIGCn_call (GCN_TERMINATE, NULL);
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
InitGCNInterface (void)
{
    int	status;
    char	*user;

    IIGCn_call (GCN_TERMINATE, NULL);

    if (! bIngresWasInit)
    {
        status = MEadvise(ME_INGRES_ALLOC);

        if (status != OK) 
            return status;

        SIeqinit();

        status = IIGCn_call (GCN_INITIATE, NULL);

        if (status != OK) 
            return status;

        gcn_seterr_func (gcn_err_func);

        user = gUserID;
        IDname (&user);

        bIngresWasInit = TRUE;
    }
    else
    {
        status = IIGCn_call (GCN_INITIATE, NULL);
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
gcn_api_call (int flag, 
	char *obj, 
	int opcode, 
	char *value, 
	char *type)
{
    GCN_CALL_PARMS	gcn_parm;
    int			status;

    if (opcode == GCN_REC_ADDED)
	opcode = GCN_ADD;
    else if (opcode == GCN_REC_DELETED)
	opcode = GCN_DEL;

    gcn_parm.gcn_response_func = gcn_api_silent;
    gcn_parm.gcn_uid = gUserID;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = obj;
    gcn_parm.gcn_type = type;
    gcn_parm.gcn_value = value;
    gcn_parm.gcn_host = NULL;

    NMgtAt ("II_INSTALLATION", &gcn_parm.gcn_install);

    status = IIGCn_call (opcode, &gcn_parm);
    return status;
}

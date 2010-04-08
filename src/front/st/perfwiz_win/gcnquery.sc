/*
** Copyright (c) 1999 Ingres Corporation all rights reserved
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <id.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <er.h>
#include <pm.h>
#include <cv.h>
#include <qu.h>
#include <st.h>
#include <gc.h>
#include <gcccl.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcu.h>
#include "gcnquery.h"

exec sql include sqlca;

/*
**  Name: gcnquery.c
**
**  Description:
**	This file houses all routines necessary to talk to the Name Server.
**
**  History:
**	27-aug-1999 (somsa01)
**	    Created, using gcnquery.sc as a base (from front!st!gwdm).
**	21-oct-1999 (somsa01)
**	    In gcn_respsh(), strip off the domain name which may be tacked
**	    onto the host name.
**	29-oct-1999 (somsa01)
**	    Added CheckQuery(), which checks a given query to see if it
**	    returns a single numeric value (used for personal counter
**	    definitions).
*/

/* Local defines */
static i4		msg_max;
static char		*msg_buff;
static int		lastOpStat;

/* Global defines */
struct nodelist		*nodeptr;

/* Declaration of local functions */
static i4	gcn_response();
static i4	gcn_respsh();
static VOID	gcn_resperr();
static STATUS	gca_request(char *target, i4 *assoc_id);
static STATUS	gca_release(i4 assoc_id);
static STATUS	gca_receive(i4 assoc_id, i4 *msg_type,
			    i4 *msg_len, char *msg_buff);
static STATUS	gca_send(i4 assoc_id, i4 msg_type, i4 msg_len, char *msg_buff);
static STATUS	gca_disassoc(i4 assoc_id);
static VOID	ZapBack(char *workbuf);


/*
**  Name: GetVnodes()
**
**  Description:
**	This function sets up a linked list of all vnodes.
**
**  Exit Status:
**	0	Success.
**	1	Cannot get to local Name Server.
**	2	Query of local Name Server for nodes failed.
**	3	No nodes were found.
**	4	GCA_INITIATE failed.
**	5	Memory allocation failed.
*/
int 
GetVnodes()
{
    GCN_CALL_PARMS	gcn_parm;
    char		value[GCN_VAL_MAX_LEN+1];
    char		username[33], *uptr = username;
    STATUS		status;
    u_i4		flag;
    struct nodelist	*node;

    IDname(&uptr);

    /*
    ** First, add the local node to the list.
    */
    node = (struct nodelist *)MEreqmem(0, sizeof(struct nodelist), TRUE, NULL);
    node->next = NULL;
    STcopy(PMhost(), node->hostname);
    STcopy("(local)", node->nodename);
    nodeptr = node;

    /*
    ** Now, retrieve all defined nodes in the current installation.
    */
    
    if ((IIGCn_call(GCN_INITIATE, NULL)) != OK)
	return(1);

    /* Get all private nodes */
    STcopy(",,", value);
    flag = GCN_NET_FLAG;
    gcn_parm.gcn_response_func = gcn_response;
    gcn_parm.gcn_uid = username;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = "";
    gcn_parm.gcn_type = GCN_NODE;
    gcn_parm.gcn_value = value;
    NMgtAt(ERx("II_INSTALLATION"),&gcn_parm.gcn_install);
    gcn_parm.gcn_host = 0;
    status = IIGCn_call(GCN_RET, &gcn_parm);

    /* Now, get all public nodes. */
    flag = (GCN_PUB_FLAG | GCN_NET_FLAG);
    gcn_parm.gcn_response_func = gcn_response;
    gcn_parm.gcn_uid = username;
    gcn_parm.gcn_flag = flag;
    gcn_parm.gcn_obj = "";
    gcn_parm.gcn_type = GCN_NODE;
    gcn_parm.gcn_value = value;
    NMgtAt(ERx("II_INSTALLATION"),&gcn_parm.gcn_install);
    gcn_parm.gcn_host = 0;
    status = IIGCn_call(GCN_RET, &gcn_parm);

    IIGCn_call(GCN_TERMINATE, NULL);
    if (status != OK)
	return(2);

    if (nodeptr == NULL)
	return(3);

    return(0);
}

/*
**  Name: gcn_response -- handle a response from the name server.
**
**  Description:
**	This function is called by the name server to handle the response to
**	any function. It parses out the text and calls gcn_respsh for each
**	line.
*/
static i4
gcn_response(i4 msg_type, char *buf, i4 len)
{
    i4	row_count, op = 0;

    /* Handle the message responses from name server. */
    switch(msg_type)
    {
	case GCN_RESULT:
	    /* Pull off the results from the operation */
	    buf += gcu_get_int(buf, &op);
	    if (op == GCN_RET)
	    {
		buf += gcu_get_int(buf, &row_count);
		while(row_count--)
		    buf += gcn_respsh(buf);
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
**  Name: gcn_resperr() - decodes a GCA_ERROR from the name server.
**
**  Description:
**	If error code(s) have been placed in the GCA_ER_DATA use the first
**	error code as the lastOpStat value to indicate the status of the
**	request.
*/
static VOID
gcn_resperr(char *buf, i4 len)
{
    GCA_ER_DATA	*ptr = (GCA_ER_DATA *)buf;

    if (ptr->gca_l_e_element > 0)
	lastOpStat = ptr->gca_e_element[0].gca_id_error;
}

/*
**  Name: gcn_respsh() - decode one row of show command output.
**
**  Description:
**	Figures out what needs to be stashed away based on what sort of
**	command is being processed, and calls the text-list handler to
**	save away the relevant information.
**
**	Returns the number of bytes of the buffer which were processed by
**	this call.
**
**  History:
**	21-oct-1999 (somsa01)
**	    Strip off the domain name which may be tacked onto the host name.
*/
static i4
gcn_respsh(char *buf)
{
    char		*type, *uid, *obj, *val, *cptr;
    i4			nbytes = (i4)buf;
    struct nodelist	*node, *nptr;
    int			i;

    /* get type, tuple parts */
    buf += gcu_get_str(buf, &type);
    buf += gcu_get_str(buf, &uid);
    buf += gcu_get_str(buf, &obj);
    buf += gcu_get_str(buf, &val);
    if (!STbcompare(type, 0, GCN_NODE, 0, TRUE))
    {
	/* Add the node to our list. */
	node = (struct nodelist *)MEreqmem( 0, sizeof(struct nodelist),
					    TRUE, NULL);
	node->next = NULL;
	cptr = val;
	i = 0;
	while ((*cptr != ',') && (*cptr != '.'))
	{
	    node->hostname[i] = *cptr;
	    cptr++;
	    i++;
	} 
	node->hostname[i] = '\0';

	STcopy(obj, node->nodename);
	if (nodeptr == NULL)
	    nodeptr = node;
	else
	{
	    nptr = nodeptr;
	    while (nptr->next)
		nptr = nptr->next;
	    nptr->next = node;
	}
    }

    nbytes = (i4)buf - nbytes;
    return(nbytes);
}

static STATUS
gca_request(char *target, i4 *assoc_id)
{
    GCA_RQ_PARMS	rq_parms;
    STATUS		status, call_status;

    MEfill(sizeof(rq_parms), 0, (PTR)&rq_parms);
    rq_parms.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
    rq_parms.gca_partner_name = target;

    call_status = IIGCa_call( GCA_REQUEST, (GCA_PARMLIST *)&rq_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    *assoc_id = rq_parms.gca_assoc_id;

    if ((call_status == OK) && (rq_parms.gca_status != OK))
	status = rq_parms.gca_status, call_status = FAIL;

    if (call_status == OK)
    {
	if (rq_parms.gca_peer_protocol < GCA_PROTOCOL_LEVEL_50)
	{
	    gca_release(*assoc_id);
	    call_status = FAIL;
	}
    }

    return(call_status);
}

static STATUS
gca_send(i4 assoc_id, i4 msg_type, i4 msg_len, char *msg_buff)
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status, call_status;

    MEfill(sizeof(sd_parms), 0, (PTR)&sd_parms);
    sd_parms.gca_association_id = assoc_id;
    sd_parms.gca_message_type = msg_type;
    sd_parms.gca_msg_length = msg_len;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ((call_status == OK) && (sd_parms.gca_status != OK))
	status = sd_parms.gca_status, call_status = FAIL;

    if (call_status != OK)
    {
	/* The value of call_status will be returned. */
    }

    return(call_status);
}

static STATUS
gca_receive(i4 assoc_id, i4 *msg_type, i4 *msg_len, char *msg_buff)
{
    GCA_RV_PARMS	rv_parms;
    STATUS		status, call_status;

    MEfill(sizeof(rv_parms), 0, (PTR)&rv_parms);
    rv_parms.gca_assoc_id = assoc_id;
    rv_parms.gca_b_length = *msg_len;
    rv_parms.gca_buffer = msg_buff;

    call_status = IIGCa_call( GCA_RECEIVE, (GCA_PARMLIST *)&rv_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ((call_status == OK) && (rv_parms.gca_status != OK))
	status = rv_parms.gca_status, call_status = FAIL;

    if (call_status != OK)
    {
	/* The value of call_status will be returned. */
    }
    else if (!rv_parms.gca_end_of_data)
    {
	call_status = FAIL;
    }
    else
    {
	*msg_type = rv_parms.gca_message_type;
	*msg_len = rv_parms.gca_d_length;
    }

    return(call_status);
}

static STATUS
gca_release(i4 assoc_id)
{
    GCA_SD_PARMS	sd_parms;
    STATUS		status, call_status;
    char		*ptr = msg_buff;

    ptr += gcu_put_int( ptr, 0 );

    MEfill(sizeof(sd_parms), 0, (PTR)&sd_parms);
    sd_parms.gca_assoc_id = assoc_id;
    sd_parms.gca_message_type = GCA_RELEASE;
    sd_parms.gca_buffer = msg_buff;
    sd_parms.gca_msg_length = (i4)(ptr - msg_buff);
    sd_parms.gca_end_of_data = TRUE;

    call_status = IIGCa_call( GCA_SEND, (GCA_PARMLIST *)&sd_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ((call_status == OK) && (sd_parms.gca_status != OK))
	status = sd_parms.gca_status, call_status = FAIL;

    return(call_status);
}

static STATUS
gca_disassoc(i4 assoc_id)
{
    GCA_DA_PARMS	da_parms;
    STATUS		status, call_status;

    MEfill(sizeof(da_parms), 0, (PTR)&da_parms);
    da_parms.gca_association_id = assoc_id;

    call_status = IIGCa_call( GCA_DISASSOC, (GCA_PARMLIST *)&da_parms,
			      GCA_SYNC_FLAG, NULL, -1, &status );

    if ((call_status == OK) && (da_parms.gca_status != OK))
	status = da_parms.gca_status, call_status = FAIL;

    if (call_status != OK)
    {
	/* The value of call_status will be returned. */
    }

    return(call_status);
}

/*
**  Name: GetServerList()
**
**  Description:
**	This procedure connects to IMADB and retrieves the list of registered
**	servers.
**
**  Returns:
**	0	Success.
**	1	Unable to connect.
**	n < 0	SQL state error.
*/

int
GetServerList(char target[128])
{
    struct nodelist	*nptr = nodeptr;
    struct serverlist	*node, *tmpptr;
    exec sql begin declare section;
	char		dbname[128];
	varchar struct {
	    short	vch_count;
	    char	vch_data[33];
	} server_class;
	varchar struct {
	    short	vch_count;
	    char	vch_data[65];
	} server_id;
    exec sql end declare section;

    while (STcompare(target, nptr->nodename))
	nptr = nptr->next;
    nptr->svrptr = NULL;

    if (!STcompare(target, "(local)"))
	STcopy("imadb",  dbname);
    else
	STprintf(dbname, "%s::imadb", target);
    exec sql connect :dbname;
    if (sqlca.sqlcode < 0)
	return (1);

    exec sql execute procedure ima_set_vnode_domain;
    if (sqlca.sqlcode < 0)
    {
	exec sql disconnect;
	return(sqlca.sqlcode);
    }

    exec sql select server_class, listen_address
	     into :server_class, :server_id
	     from ima_gcn_registrations
	     where server_class in ('IUSVR', 'INGRES', 'STAR', 'ICESVR')
	     order by server_class;
    exec sql begin;
    {
	node = (struct serverlist *)MEreqmem( 0, sizeof(struct serverlist),
					      TRUE, NULL );
	server_class.vch_data[server_class.vch_count] = '\0';
	server_id.vch_data[server_id.vch_count] = '\0';
	ZapBack(server_class.vch_data);
	ZapBack(server_id.vch_data);
	STcopy(server_class.vch_data, node->server_class);
	STcopy(server_id.vch_data, node->server_id);
	node->next = NULL;

	if (!nptr->svrptr)
	    nptr->svrptr = node;
	else
	{
	    tmpptr = nptr->svrptr;
	    while (tmpptr->next)
		tmpptr = tmpptr->next;
	    tmpptr->next = node;
	}
    }
    exec sql end;

    if (sqlca.sqlcode < 0)
    {
	exec sql disconnect;
	return(sqlca.sqlcode);
    }

    exec sql disconnect;
    return (0);
}

/*
**  Name: ListDestructor()
**
**  Description:
**	Destroys the node linked list.
**
*/
void
ListDestructor()
{
    struct nodelist	*nptr = nodeptr, *tmpptr;
    struct serverlist	*svrptr, *tmpptr2;

    while (nptr)
    {
	svrptr = nptr->svrptr;
	while (svrptr)
	{
	    tmpptr2 = svrptr->next;
	    MEfree((PTR)svrptr);
	    svrptr = tmpptr2;
	}
	nptr->svrptr = NULL;

	tmpptr = nptr->next;
	MEfree((PTR)nptr);
	nptr = tmpptr;
    }
    nodeptr = NULL;
}

static VOID
ZapBack(char *workbuf)
{
    if (STlength(workbuf) < 1)
	return;
    while (workbuf[STlength(workbuf)-1] == ' ')
    {
	workbuf[STlength(workbuf)-1] = '\0';
	if (STlength(workbuf) < 1)
	    break;
    }
}

int
CheckQuery(dbname, qry, errtxt)
exec sql begin declare section;
    char	*errtxt;
    char	*dbname;
    char	*qry;
    char	*errtxt;
exec sql end declare section;
{
    exec sql begin declare section;
	long	value;
    exec sql end declare section;

    exec sql connect :dbname;
    if (sqlca.sqlcode != 0)
    {
	exec sql inquire_ingres(:errtxt = ERRORTEXT);
	return(sqlca.sqlcode);
    }

    exec sql execute immediate :qry into :value;
    if (sqlca.sqlcode != 0)
    {
	int	status = sqlca.sqlcode;

	exec sql inquire_ingres(:errtxt = ERRORTEXT);
	exec sql disconnect;
	return(status);
    }
    
    exec sql disconnect;
    return(0);
}

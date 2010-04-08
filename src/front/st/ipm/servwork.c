/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** includes and defines
*/
#include    <compat.h>
#include    <dbms.h>
#include    <fe.h>
#include    <me.h>
#include    <st.h>
#include    <gca.h>
#include    "dba.h"
#include    "servinfo.h"
#include    "msgs.h"

#define _DUMMY_AID_VALUE -9999

FUNC_EXTERN VOID	geting_error();
STATUS			getsrvdata();


/*
**
**  File: servwork.c
**
**  Purpose - this file contains the routines which get server
**      information and session information.  This routine also
**	can shut down servers (iimonitor equivalent capability).
**
**  This file contains:
**	dosrvoper - perform requested operation on a server - calls getsrvinfo
**	getsrvdata - get info/perform operations on a given INGRES server
**
**  History
**	1/6/89		tomt		created
**	3/21/89		tomt		did more work on routines
**	3/27/89		tomt		more work on routines
**	3/27/89		tomt		added new operation
**	4/6/89		tomt		had to add new operation - terminate
**	6/23/89		tomt		changed exit(..) to PCexit()
**	10/1/89		tomt		integrated 6.3 changes
**	28-Feb-1994 (fredv)
**		Fix bug 60153, ipm core dumped when selecting the server list 
**		for display. The problem was due to an unalign buffer. This 
**		bug showed up on Solaris, but it can be potential showed on
**		any alignment restrictive machines such as HP8.
**		The fix is to make sure the buffer is aligned to the most
**		restrictive alignment by using an ALIGN_RESTRICT array instead 
**		of a char array.
**      07-Dec-1994 (liibi01)
**              Cross integration from 6.4 -- Bug 46411, 46455 and 49084.
**              Fixed string length to GCN
**	10-mar-1995 (reijo01)
** 		Changed the message displayed during a hard shutdown. 
**	17-nov-1995 (canor01)
**		Cast the return from STlength() to a i4  to prevent
**		bad signed to unsigned promotion on Windows NT.
**      18-Jan-1996 (cbradley)
**	  	Fixed a problem caused by an implicit type promotion in an expression 
**		in catbuf() that eventually caused strings to be copied to unallocated
**		storage. Also removed memory leak produced when infobuf is grown in
**		the same routine.
**	16-Feb-1992 (cbradley)
**		Fixed a problem where infobuf was being de-referenced as a NULL
**		pointer, and so changed the way it was initialized, and removed
**		the test for infobuf being NULL.

**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    25-Oct-2005 (hanje04)
**        Add prototype for catbuf() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**    23-Sep-2009 (hanal04) bug 115316
**        Add GET_SERVER_CAP to retrieve server capabilities.
*/

/* local prototypes */
static catbuf();

STATUS
dosrvoper(oper, arg1)
int	oper;	/* pointer to character string which specifies action */
char 	*arg1;	/* needed for some operations */
{
    char	action[256];
    char        server_name[GCN_VAL_MAX_LEN + 1];
    STATUS	status;

    switch(oper) 
    {
      case GC_SERV_INIT:
      case GC_SERV_DISCONNECT:
      case GC_SERV_TERMINATE:
	action[0] = EOS;
        break;

      case GC_SERV_CONNECT:
	if (arg1 == NULL) /* no value passed - an error */
	    return(FAIL);
	STlcopy(arg1, server_name, sizeof(server_name) - 1);
	action[0] = EOS;
	break;

      case GET_SERVER_INFO:	/* get server info from DBMS server(s) */
        STlcopy("show server", action, sizeof(action));
	break;

      case GET_SERVER_CAP:      /* get server capabilities from DBMS */
        STlcopy("show server capabilities", action, sizeof(action));
        break;

      case GET_SESSION_INFO:	/* get session info from DBMS server(s) */
	if (flag_internal_sess)
	{
	    STlcopy("show all sessions", action, sizeof(action));
	}
	else
	{
	    STlcopy("show sessions", action, sizeof(action));
	}
	break;

      case GET_FORMAT_SESSION:	/* get curr. session format in curr. server */
	if (arg1 == NULL) /* no value passed - an error */
	    return(FAIL);
	STprintf(action, "format %s", arg1);
	break;

      case SET_SERVER_SHUT:	/* spin down current DBMS server nice */
        STlcopy("set server shut", action, sizeof(action) - 1);
	break;

      case STOP_SERVER:		/* stop current DBMS server hard */
	STlcopy("stop server", action, sizeof(action) - 1);
	break;

      case SUSPEND_SESSION:	/* suspend current session in current server */
	if (arg1 == NULL) /* no value passed - an error */
	    return(FAIL);
	STprintf(action, "suspend %s", arg1);
	break;

      case RESUME_SESSION:	/* resume current session in current server */
	if (arg1 == NULL) /* no value passed - an error */
	    return(FAIL);
	STprintf(action, "suspend %s", arg1);
	break;

      case REMOVE_SESSION:	/* stop current session in current server */
	if (arg1 == NULL) /* no value passed - an error */
	    return(FAIL);
	STprintf(action, "remove %s", arg1);
	break;

    default:
	return(FAIL);
    }
    status = getsrvdata(oper, server_name, action);
    return status;
}

static bool		dbms_died = FALSE;
static ALIGN_RESTRICT		buf[2048/sizeof(ALIGN_RESTRICT)+1];
static i4		aid = -9999;
/* 
** if infobuf is initialized to a string of length > 0, then the test in 
** catbuf() will need to be changed accordingly....
*/
static char		*infobuf = "";		
static i4		infobuf_len = 0;
#define		IF_QUANTUM	4096

STATUS
getsrvdata(opcode, server_name, action)
int	    opcode;
char	    *server_name;
char	    *action;
{
    i4			error;
    STATUS		status;
    i4			count;
    char		errmsg[ERBUFSZ + 100];
    char		errmsg1[ERBUFSZ];
    GCA_DATA_VALUE	*input;
    GCA_PARMLIST	gca_parms;
    GCA_DA_PARMS	*disassoc;

    switch(opcode) 
    {
      case GC_SERV_INIT:
	/*
	**   set up connection GCA
	*/
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	if ((status = IIGCa_call(GCA_INITIATE,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error)) != OK)
	{
	    terminate(FAIL, "getsrvdata: GCA_INITIATE failed (%d)\n", error);
	}
	break;

      case GC_SERV_CONNECT:
	/*
	**   Request assocation with specified server
	*/
        dbms_died = FALSE;
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);

	gca_parms.gca_rq_parms.gca_partner_name = server_name;
	gca_parms.gca_rq_parms.gca_modifiers = GCA_NO_XLATE | GCA_CS_CMD_SESSN;
	status = IIGCa_call(GCA_REQUEST,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error);

	if ((status != OK) ||
	    ((error = gca_parms.gca_rq_parms.gca_status) != E_GC0000_OK))
	{
	    geting_error(error, errmsg1);
	    STprintf(errmsg,
"Could not connect to server.  Specified server may not exist or is not \
allowing connects.  INGRES error message is %s", errmsg1);
	    POP_MSG(errmsg);
	    return(FAIL);
	}
	aid = gca_parms.gca_rq_parms.gca_assoc_id;

	/*
	**   Get response from server - begin negotiation
	*/
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_rv_parms.gca_buffer = (char *)buf;
	gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
	gca_parms.gca_rv_parms.gca_association_id = aid;
	gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
	status = IIGCa_call(GCA_RECEIVE,
		   &gca_parms, GCA_SYNC_FLAG, 0, -1, &error);
	if ((status != OK) ||
	    ((error = gca_parms.gca_rv_parms.gca_status) != E_GC0000_OK))
	{
	    geting_error(error, errmsg1);
	    STprintf(errmsg, "GCA_RECEIVE returned status %x.  Error is %s",
		error, errmsg1);
	    POP_MSG(errmsg);
	    return(FAIL);
	}

	/*
	**   No errors, get the data sent by the server
	*/
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_it_parms.gca_buffer = (char *)buf;
	status = IIGCa_call(GCA_INTERPRET,
			&gca_parms, GCA_SYNC_FLAG, 0, -1, &error);
	if (status != OK)
	{
	    geting_error(error, errmsg1);
	    STprintf(errmsg, "GCA_INTERPRET returned status %x.  Error is %s",
		error, errmsg1);
	    POP_MSG(errmsg);
	    return(FAIL);
	}

	if (gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
	{
	    i4	id_err;

	    id_err = ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
			gca_e_element[0].gca_id_error;

	    /*
	    **	If this is the generic error range, send the local error
	    */
	    /* XXX there are probably generic defines for this. */
	    if ((id_err > 30000) && id_err < 50000)
	    {
		geting_error(
		   ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
		   gca_e_element[0].gca_local_error, errmsg1);
	    }
	    else
	    {
		geting_error(error, errmsg1);
	    }
	    STprintf(errmsg,
"DBMS server refused connection.  INGRES error message is %s", errmsg1);
	    POP_MSG(errmsg);
	    return(FAIL);
	}

	/*
	**   server accepted connect - indicate this is a monitor session
	*/
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_fo_parms.gca_buffer = (char *)buf;
	gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
	if ((status = IIGCa_call(GCA_FORMAT,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error)) != OK)
	{
	    terminate(FAIL, "getsrvdata: GCA_FORMAT status %x\n", error);
	}

	((GCA_SESSION_PARMS *) gca_parms.gca_fo_parms.gca_data_area)->gca_l_user_data = 1;
	((GCA_SESSION_PARMS *) gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_index = GCA_SVTYPE;
        ((GCA_SESSION_PARMS *)
            gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.gca_type = GCA_TYPE_INT;
        ((GCA_SESSION_PARMS *)
            gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.gca_l_value  = sizeof(i4);
	*((i4 *) ((GCA_SESSION_PARMS *)
	    gca_parms.gca_fo_parms.gca_data_area)->gca_user_data[0].gca_p_value.
	    gca_value) = GCA_MONITOR;
    
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_sd_parms.gca_buffer = (char *)buf;
	gca_parms.gca_sd_parms.gca_msg_length =
	    sizeof(GCA_SESSION_PARMS) + sizeof(i4) - sizeof(char); 
	gca_parms.gca_sd_parms.gca_message_type = GCA_MD_ASSOC;
	gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
	gca_parms.gca_sd_parms.gca_association_id = aid;
	status = IIGCa_call(GCA_SEND,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error);
	if ((status != OK) ||
	    ((error = gca_parms.gca_sd_parms.gca_status) != E_GC0000_OK))
	{
	    terminate(FAIL, "getsrvdata: GCA_SEND failed (status %x)", error);
        }

	/* Get the server response */
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_rv_parms.gca_buffer = (char *)buf;
	gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
	gca_parms.gca_rv_parms.gca_association_id = aid;
	gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
	status = IIGCa_call(GCA_RECEIVE,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error);
	if ((status != OK) ||
	    ((error = gca_parms.gca_rv_parms.gca_status) != E_GC0000_OK))
	{
	    terminate(FAIL, "getsrvdata: GCA_RECEIVE failed (%x)", error);
	}

	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_it_parms.gca_buffer = (char *)buf;
	if ((status = IIGCa_call(GCA_INTERPRET,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error)) != OK)
	{
	    terminate(FAIL, "getsrvdata: GCA_INTERPRET failed (%x)", error);
	}

	if (gca_parms.gca_it_parms.gca_message_type != GCA_ACCEPT)
	{
	    i4	id_err;
	    id_err = ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
		gca_e_element[0].gca_id_error;

	    /*
	    **	If this is the generic error range, send the local error
	    */
	    if ((id_err > 30000) && id_err < 50000)
	    {
		geting_error(
		   ((GCA_ER_DATA *) gca_parms.gca_it_parms.gca_data_area)->
		   gca_e_element[0].gca_local_error, errmsg1);
	    }
	    else
	    {
		geting_error(error, errmsg1);
	    }
	    STprintf(errmsg,
"DBMS server refused connection.  INGRES error message is %s", errmsg1);
	    POP_MSG(errmsg);
	    return(FAIL);
        }
	break;

      case GET_SERVER_INFO:
      case GET_SERVER_CAP:
      case GET_SESSION_INFO:
      case GET_FORMAT_SESSION:
      case SET_SERVER_SHUT:
      case STOP_SERVER:
      case SUSPEND_SESSION:
      case RESUME_SESSION:
      case REMOVE_SESSION:

	/* Connection established, process the passed request */
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_fo_parms.gca_buffer = (char *)buf;
	gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
	if ((status = IIGCa_call(GCA_FORMAT, &gca_parms, GCA_SYNC_FLAG, 0, -1,
	    &error)) != OK)
	{
	    terminate(FAIL, "getsrvdata: GCA_FORMAT failed (%x)", error);
	}

	MEfill((u_i2) gca_parms.gca_fo_parms.gca_d_length, EOS,
	    (PTR) gca_parms.gca_fo_parms.gca_data_area);

	input = (GCA_DATA_VALUE *)
	    ((char *) gca_parms.gca_fo_parms.gca_data_area
	    + sizeof(i4) + sizeof(i4));
	input->gca_type = DB_QTXT_TYPE;
	input->gca_l_value = gca_parms.gca_fo_parms.gca_d_length;

	/* process user request	*/
        STcopy(action, input->gca_value);
        count = STlength(action);
	input->gca_value[count] = 0;
	input->gca_l_value = count;

	MEfill((u_i2)sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_sd_parms.gca_buffer = (char *)buf;
	gca_parms.gca_sd_parms.gca_msg_length = (2 * sizeof(i4)) + count +
	    sizeof(input->gca_type) + sizeof(input->gca_precision)
	    + sizeof(input->gca_l_value);
	gca_parms.gca_sd_parms.gca_message_type = GCA_QUERY;
	gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
	gca_parms.gca_sd_parms.gca_association_id = aid;
	/* problem occurs AFTER here -- jpk */
	status = IIGCa_call(GCA_SEND,
	    &gca_parms, GCA_SYNC_FLAG, 0, -1, &error);
	if ((status != OK) ||
	    ((error = gca_parms.gca_sd_parms.gca_status) != E_GC0000_OK))
	{
	    if (error == E_GC0001_ASSOC_FAIL)
	    {
		geting_error(error, errmsg1);
		STprintf(errmsg,
"DBMS server terminated, connection dropped.  INGRES error message is %s", errmsg1);
		POP_MSG(errmsg);
		dbms_died = TRUE;
		return(FAIL);
	    }
	    terminate(FAIL, "getsrvdata: GCA_SEND failed with error = %x\n",
		gca_parms.gca_sd_parms.gca_status);
	}

	do
	{
	    MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	    gca_parms.gca_rv_parms.gca_buffer = (char *)buf;
	    gca_parms.gca_rv_parms.gca_b_length = sizeof(buf);
	    gca_parms.gca_rv_parms.gca_association_id = aid;
	    gca_parms.gca_rv_parms.gca_flow_type_indicator = GCA_NORMAL;
	    status = IIGCa_call(GCA_RECEIVE, &gca_parms, GCA_SYNC_FLAG, 0,
		-1, &error);
	    if (status != OK || gca_parms.gca_rv_parms.gca_status != E_GC0000_OK)
	    {
		if (gca_parms.gca_rv_parms.gca_status == E_GC0001_ASSOC_FAIL)
		{
		    geting_error(error, errmsg1);
		    STprintf(errmsg,
"DBMS server terminated, connection dropped.  INGRES error message is %s", errmsg1);
		    POP_MSG(errmsg);
		    dbms_died = TRUE;
		    return(FAIL);
		}
	        terminate(FAIL, "getsrvdata: GCA_RECEIVE failed (%x)", 
		    gca_parms.gca_rv_parms.gca_status);
	    }

	    MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	    gca_parms.gca_it_parms.gca_buffer = (char *)buf;
	    if ((status = IIGCa_call(GCA_INTERPRET, &gca_parms, GCA_SYNC_FLAG,
				     0, -1, &error)) != OK)
	    {
		terminate(FAIL, "getsrvdata: GCA_INTERPRET returned (%x)", error);
	    }
	    if (gca_parms.gca_it_parms.gca_message_type == GCA_TDESCR)
	    {
		continue;
	    }
	    else if ((gca_parms.gca_it_parms.gca_message_type == GCA_RELEASE)
		|| (gca_parms.gca_it_parms.gca_status == E_GC0001_ASSOC_FAIL)
		|| (gca_parms.gca_it_parms.gca_status == E_GC0023_ASSOC_RLSED))
	    {
		/*
		 * Bug fix 67376   
		 * 	reijo01
		 *  If it is a hard termination, the connection will drop without
		 *  an error status. So just inform user that the server terminated 
		 *  and forget looking for a more detailed error message.
		 */
			if (opcode == STOP_SERVER)
			{
			dbms_died = TRUE;
			return(OK);
/* 			STprintf(errmsg,"DBMS server terminated, connection dropped. "); */ 
			}
			else
			{
			geting_error(error, errmsg1);
			STprintf(errmsg,
"DBMS server terminated, connection dropped.  INGRES error message is %s", errmsg1);
			}
		POP_MSG(errmsg);
		dbms_died = TRUE;
		return(FAIL);
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type == GCA_TUPLES)
	    {
		DB_TEXT_STRING	*txt;

		txt = (DB_TEXT_STRING *) gca_parms.gca_it_parms.gca_data_area;
		txt->db_t_text[txt->db_t_count] = EOS;
		catbuf((char*)txt->db_t_text);
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type == GCA_TRACE)
	    {
		gca_parms.gca_it_parms.gca_data_area[
		    gca_parms.gca_it_parms.gca_d_length] = EOS;
		catbuf(gca_parms.gca_it_parms.gca_data_area);
	    }
	    else if (gca_parms.gca_it_parms.gca_message_type != GCA_RESPONSE)
	    {
		STprintf(errmsg, "Unexpected message type %x (%<%d.) -- ignored",
				gca_parms.gca_it_parms.gca_message_type);
		POP_MSG(errmsg);
		dbms_died = TRUE;
		return(FAIL);
	    }
	} while (gca_parms.gca_it_parms.gca_message_type != GCA_RESPONSE
			&& !dbms_died);

	if (infobuf[0] != EOS && !dbms_died)
	{
#ifdef DEBUG
	    static int		ctr = 0;

	    ++ctr;
	    d_print("<#%d#%s#%d#>\n", ctr, infobuf, ctr);
#endif
	    st_servinfo(opcode, infobuf);
	    infobuf[0] = EOS;
	}

	break;

      case GC_SERV_DISCONNECT:
	if (!dbms_died)
	{
	    if (aid == _DUMMY_AID_VALUE)
		return(OK);

	    MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	    gca_parms.gca_fo_parms.gca_buffer = (char *)buf;
	    gca_parms.gca_fo_parms.gca_b_length = sizeof(buf);
	    if ((status = IIGCa_call(GCA_FORMAT, &gca_parms, GCA_SYNC_FLAG, 0, 
		-1, &error)) != OK)
	    {
		terminate(FAIL, "getsrvdata: GCA_FORMAT failed (%x)", error);
	    }

	    ((GCA_ER_DATA *) gca_parms.gca_fo_parms.gca_data_area)->gca_l_e_element  = 0;
	
	    MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	    gca_parms.gca_sd_parms.gca_buffer = (char *)buf;
	    gca_parms.gca_sd_parms.gca_msg_length = sizeof(GCA_ER_DATA);
	    gca_parms.gca_sd_parms.gca_message_type = GCA_RELEASE;
	    gca_parms.gca_sd_parms.gca_end_of_data = TRUE;
	    gca_parms.gca_sd_parms.gca_association_id = aid;
	    if ((status = IIGCa_call(GCA_SEND,
		&gca_parms, GCA_SYNC_FLAG, 0, -1, &error)) != OK)
	    {
		terminate(FAIL, "getsrvdata: GCA_RECEIVE failed (%x)", error);
 	    }
	}
	/*
	**   Invoke GCA_DISASSOC service to clean up released association.
	*/
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	disassoc = &gca_parms.gca_da_parms;
	disassoc->gca_association_id = aid;
	disassoc->gca_status = E_GC0000_OK;
	status = IIGCa_call(GCA_DISASSOC, &gca_parms, GCA_SYNC_FLAG,
	    0, (i4) -1, &status);
	break;

      case GC_SERV_TERMINATE:
	MEfill((u_i2) sizeof(gca_parms), EOS, (PTR) &gca_parms);
	gca_parms.gca_sd_parms.gca_association_id = aid;
	gca_parms.gca_sd_parms.gca_end_of_data = TRUE;	/* final segment */
	/*
	**  Make GCA_TERMINATE service call 
	*/
	status = IIGCa_call(GCA_TERMINATE, &gca_parms, GCA_SYNC_FLAG, 
	    0, (i4) -1, &status);
	if (status != OK)
	    return(FAIL);
	break;
    }
    return(OK);
}

static 
catbuf(s)
    char *s;
{
    if ((i4)(STlength(s) + STlength(infobuf)) > ( infobuf_len - 1))
    {
	char	*old, *new;
	STATUS	stat = OK;

	d_print("\tre-allocating buf:\n");
	/* buffer not big enough. */

	if (infobuf_len == 0)
	    infobuf_len = IF_QUANTUM;
	else
	    infobuf_len *= 2;

	old = infobuf;
	new = (char *)MEreqmem((u_i4)0, (u_i4)infobuf_len, FALSE, &stat);
	if (new == NULL)
	{
	    terminate(FAIL, "getsrvdata: reqmem error %d in catbuf", stat);
	}
	STcopy(old,new);
	infobuf = new;
	if ((i4)STlength(infobuf) > 0)
	  MEfree(old);
    }
    STcat(infobuf, s);
}

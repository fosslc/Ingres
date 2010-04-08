/*
** Copyright (c) 2004 Ingres Corporation
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

#include <compat.h>
#include <dbms.h>
#include <me.h>
#include <si.h>
#include <st.h>
#include <qu.h>
#include <tr.h>
#include <gca.h>
#include <fe.h>
#include <nm.h>
#include <id.h>
#include "gcn.h"
#include "servinfo.h"

/*
**
**  Name: namework.c
**
**  Description:
**	This is the Lockinfo interface to the name server.  These routines
**	obtain the DBMS server names from the name server.  These routines
**	are adaptations of the RTI name server management routines.
**	It contains the following module and functions:
**
**  Routines:
**	get_name_info - modified version of iinamu top level routine
**	gcn_complete - Callback routine
**	gcn_u_r - Modify GCF Server REGISTRATION Information.
** 	IIGCn_call - GCN external entry function modified for LOCKINFO
** 	IIGCn_nm_oper - Execute an user specified operation on the Name
**	gcn_request - Connect to Name Server
**	gcn_send - Send message to Name Server
**	gcn_release - Disconnect from Name Server
** 	gcn_respond - formats the data for output
**	gcn_errhdl - formats error output
**	gcn_checkerr - check for GCA errors
**	name_connect - connects to the GCA.
**
**
**  History:    $Log-for RCS$
**	    
**      08-Sep-87   (lin)
**          Initial creation as NSMU (Combined NAMU and NETU).
**	20-Mar-88   (lin)
**	    Formally coded the routines for each function.
**	05-Sep-88   (jorge)
**	    Converted NSMU to NAMU only functionality.
**	21-mar-89   (tomt)
**	    made changes to work with LOCKINFO R6
**	14-jun-89   (tomt)
**	    look for II_DBMS_SERVER.  If defined, get_name_info will call
**	    st_nameinfo directly instead of attempting to connect to the
**	    name server.
**	01-oct-89   (tomt)
**	    integrated 6.3 changes
**	02-oct-89   (tomt)
**	    created 2 operations that were defined in 6.1 gcn.h file.
**	    These were:
**		GCN_CONNECT
**		GCN_TERMINATE
**	    To avoid conflict if the numbers previously assigned are reissued,
**	    they are being renamed and assigned numbers well out of range of
**	    the currently assigned numbers.  The new names are
**		IPMGCN_CONNECT
**		IPMGCN_TERMINATE
**	14-dec-89   (jbrady)
**	    Added code to handle BYTE_ALIGN situation for systems
**	    where that is a problem.  Use indirect variables for 
**	    integer assignments and references and byte copies.  
**	    Could still be better though, I think.
**	16-dec-89   (kimman)
**	    Integrating jbrady's BYTE_ALIGN changes of 14-dec-89
**	    for systems which needed it (such as the DECstation 3100).
**	 1-Dec-92 (jpk)
**	    made gcn_request() call type correct (needs second str param)
**	26-Aug-1993 (fredv)
**		Included <st.h>.
**      08-Dec-94 (liibi01)
**          some string buffers incorrect lengths - corrected.
**          Cross integration from 6.4. (Bug 46411, 46455 and 49084.)
**	11-Sep-96 (gordy)
**	    It's a no-no to access internal GCN data/routines.  This
**	    file is a mess anyway, so just duplicated the GCN stuff
**	    here and made everything static.  Removed a buncha kruft.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of fe.h, nm.h and id.h to eliminate gcc 4.3 warnings.
*/

/*
**  Constant definitions
*/
#define IPMGCN_CONNECT		200
#define IPMGCN_TERMINATE	201

/*
**  Forward and/or External function references.
*/
static	GCN_CALL_PARMS	Init_status;
static	i4		gcn_trace = 0;
static	i4		Assoc_no;
static  PTR		Out_comm_buf;
static	i4		Out_l_comm_buf;
static	PTR		Out_data_area;
static  i4		Out_l_data_area;
static  PTR		In_comm_buf;
static	i4		In_l_comm_buf;
static	PTR		In_data_area;
static  i4		In_l_data_area;
static	i4		through_once = 0;

static	char		gcn_complete();
static	STATUS		gcn_u_r();
static	STATUS		IIGCn_call();
static	STATUS		IIGCn_nm_oper();
static	STATUS		gcn_request();
static	STATUS		gcn_send();
static	STATUS		gcn_release();
static	STATUS		gcn_respond();
static	VOID		gcn_errhdl();
static  VOID		gcn_checkerr();
static	STATUS		name_connect();

/*
** Name: get_name_info - entry routine for name server processing routines.
**		All requests pass thru here.
**
** Description:
**	This routine calls the appropriate lower level routines to fulfill
**	the request.
**
** Inputs:
**	The desired action.
**
** Outputs:
**	The outpu messages for the desired action.
**
** Returns:
**	Zero exit completion code if sucessfull.
**
** Exceptions:
**	    None
**
** Side Effects:
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	22-mar-89 (tomt)
**	    modified to work with LOCKINFO
**	14-jun-89 (tomt)
**	    look for II_DBMS_SERVER.  If defined, stuff that into the
**	    linked list via st_nameinfo.  In that case, the name server
**	    will never be connected to.
**	01-oct-89 (tomt)
**	    integrated 6.3 changes
**	17-Jun-2004 (schka24)
**	    Safer env variable handling.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**
[@history_template@]...
*/

/*main()
**{
**  for (;;) {
**    get_name_info(GC_NAME_INIT, (char *)0);
**    get_name_info(GC_NAME_CONNECT, (char *)0);
**    get_name_info(NAME_SHOW, (char *)0);
**    get_name_info(NAME_SHOW, (char *)0);
**    get_name_info(NAME_SHOW, (char *)0);
**    get_name_info(GC_NAME_DISCONNECT, (char *)0);
**  }
**}
*/
STATUS
get_name_info(request, arg2)
int request;
char *arg2;
{
    STATUS	status;
    char	uid[GCN_UID_MAX_LEN];
    PTR		puid = uid;

    static bool	serv_defined;
    i4  	ntoken;
    PTR 	token[2];
    PTR		serverptr;
    static char server_name[GCN_VAL_MAX_LEN + 1];

    serverptr = NULL;
    status = 0;

    IDname(&puid);    
    STzapblank(uid,uid);

    ntoken = 1;
    if (request == 0)	/* error - bad operation passed */
	return FAIL;

    if (arg2 != NULL) {
	ntoken = 2;
	token[1] = arg2;
    }

    switch(request) {
    case GC_NAME_INIT:
	/* Check if II_DBMS_SERVER is defined */
	NMgtAt("II_DBMS_SERVER", &serverptr);
	if ((serverptr == 0) || (*serverptr == 0))
	{
	    serv_defined = FALSE;	/* II_DBMS_SERVER is NOT defined */
	}				/* or defined to empty string */
	else
	{
	    serv_defined = TRUE;
	    STlcopy(serverptr, server_name, sizeof(server_name)-1);
	    break;
	}

	if ((status = IIGCn_call(GCN_INITIATE, (PTR)&Init_status)) != E_DB_OK)
        {
	    terminate(FAIL, "get_namu: GCN_INITIATE returned an error of %x",
	        status);
	}
        break;

    case GC_NAME_CONNECT:
	if (serv_defined)
	    break;	/* II_DBMS_SERVER is defined, don't connect to nmsrv */

	if ((status = IIGCn_call(IPMGCN_CONNECT, (PTR) &Init_status))
	    != E_DB_OK) {
	    terminate(FAIL, "get_namu: GCN_CONNECT returned an error of %x\n",
	        status);
	}
        break;

    case NAME_SHOW:
	/*
	** If II_DBMS_INFO is defined, determined above, then pass the
	** servername stored in II_DBMS_INFO to be stored.  Note that
	** gcn_u_r contains the call to st_nameinfo usually.  In this case,
	** we call st_nameinfo directly.
	*/
	if (serv_defined)
	{   /* pass info for storage */
    	    st_nameinfo("INGRES", "<unknown>", server_name);
	    break;
	}

	if (ntoken < 2)
	{
	    token[1] = "INGRES";
	    ntoken = 2;
	}
	if (gcn_u_r(GCN_RET, uid, ntoken-1, &token[1]) != OK)
	{
	    return FAIL;
	}
        break;

    case GC_NAME_DISCONNECT:
	if (serv_defined)
	    break;	/* II_DBMS_SERVER is defined, don't connect to nmsrv */

	if ((status = IIGCn_call(IPMGCN_TERMINATE, (PTR) &Init_status)) !=
	    E_DB_OK) {
	    terminate(FAIL, "get_namu: GCN_TERMINATE returned an error of %x",
	        status);
	}
	break;

    default:
	return FAIL;
    }

    return status;
}

static char
gcn_complete()
{
}


/*{
** Name: gcn_u_r - This routine handles the request of REGISTRATION information.
**
** Description:
**	The user is prompted for adding, deleting, or retrieving
**	information other than any frame defined in the main routine.
**	The user should enter NAME, TYPE, and VALUE of the information entity.
**	For retrieving and deleting, the specified field is taken as the 
**	qualification of searching the record.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
**  Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
** Exceptions:
**	    None
**
** Side Effects:
**	    The database of the TYPE in the Name Server may be updated.
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	25-Mar-88 (lin)
**	    Formally reorganized from the original code.
**
[@history_template@]...
*/
static STATUS
gcn_u_r(opcode, uid, nparm, parm)
i4      opcode;
char   *uid;
#define TYPEPARM	0
#define OBJPARM		1
#define ADDRPARM	2
#define NULLPARMV	""
i4	nparm;
char   *parm[];
{
    GCN_CALL_PARMS gcn_parm;
    STATUS	status;

    switch(opcode)
    {
    	case GCN_RET:
		if (nparm != 1)
			return EOF;
    		gcn_parm.gcn_obj = NULLPARMV;
    		gcn_parm.gcn_value = NULLPARMV;
	break;

	default:
			return EOF;
    }      
			/* REGISTERED servers are always GLOBAL */
    gcn_parm.gcn_flag = GCN_DEF_FLAG | GCN_PUB_FLAG;

    gcn_parm.gcn_uid = uid;
    gcn_parm.gcn_type = parm[TYPEPARM];
    gcn_parm.gcn_install = "ii";
    gcn_parm.async_id = gcn_complete;
    if (status = IIGCn_call(opcode,&gcn_parm))
	return status;
    return OK;
}

/**
**
**  Name: IIgcn_call()
**
**  Description:
**          User entry routine for GCN
**
**
**  History:    $Log-for RCS$
**	    
**      03-Sep-87   (lin)
**          Initial creation.
**      21-mar-89   (tmt)
**	    made changes for LOCKINFO R6. 
[@history_template@]...
**/

/*
**  Forward and/or External function references.
*/

/*{
** Name: IIGCn_call	- GCN external entry function
**
** Description:
**	This routine is the common interface entry providing Name Service to
**	the caller. Currently it is only called by the Name Server 
**	Management Utility (NSMU). No other user program should use this
**	interface.
**
** Inputs:
**      service_code			Identifier of the requested GCN service.
**      parameter_list                  Pointer to the data structure containing
**                                      the parameters required by the requested
**                                      service.
**
** Outputs:
**
**
**      status                          Result of the service request execution.
**                                      The following values may be returned:
**
**		E_GCN000_OK		Request accepted for execution.
**		E_GCN003_INV_SVC_CODE	Invalid service code
**		E_GCN004_INV_PLIST_PTR	Invalid parameter list pointer
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      03-Sep-87 (Lin)
**          Initial function creation.
**      13-Dec-92 (jpk)
**          call gcn_request with correct number of parameters
**
[@history_template@]...
*/

static STATUS
IIGCn_call (service_code, parameter_list)
i4                 service_code;
GCN_CALL_PARMS	   *parameter_list;
{
    GCA_PARMLIST	parms;
    GCA_SD_PARMS	*send;
    STATUS		return_code;
    STATUS		status;
    i4			i;

    return_code = E_DB_OK;		/* Initialize return code */

/* Set up the parameters and call GCA_SEND */
    switch(service_code)
    {
	case GCN_INITIATE:
		return_code = name_connect((PTR) parameter_list);
		break;

	case IPMGCN_CONNECT: /* build the association */
		gcn_request(&Assoc_no);
		break;    

	case GCN_DEL:
	case GCN_ADD:
	case GCN_RET:
	case GCN_SHUTDOWN:
	case GCN_SECURITY:
		return_code = IIGCn_nm_oper(service_code, parameter_list);
		break;

	case IPMGCN_TERMINATE: /* release the association */
		gcn_release(Assoc_no, TRUE, 0);
		MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);
		send = &parms.gca_sd_parms;
		send->gca_association_id = Assoc_no;
/*		send->gca_buffer = Out_comm_buf;
**		send->gca_message_type = GCN_NS_OPER;
**		send->gca_msg_length = 0;
*/		send->gca_end_of_data = TRUE;	/* Final segment */

		/* Make GCA_TERMINATE service call */
		return_code = IIGCa_call(GCA_TERMINATE, &parms, GCA_SYNC_FLAG, 
			    0, (i4) -1, &status);
		if (return_code)
		    return return_code;
/*		checkerr("GCA_TERMINATE", return_code, status,
		     &send->gca_status, 0); */
		/* free up memory allocated in name_connect */
		status = MEfree(Out_comm_buf);
		if (status != OK)
		    return status;

		status = MEfree(In_comm_buf);
		if (status != OK)
		    return status;

		In_comm_buf = Out_comm_buf = NULL;
		break;

	default:
		return_code = E_DB_ERROR;
		break;
    }

    return (return_code);
} /* end IIGCn_call */

/*{
** Name: IIGCn_nm_oper - Execute an user specified operation on the Name
**			  Database.
**
** Description:
**	This routine takes the parameters specified by the user, formats
**	them in the message block and sends it to the Name Server. It also
**	receives the returned message from the Name Server and takes action
**	based on the returned message type. If the requested operation is
**	add or delete, the number of rows is returned in the returned message.
**	This number is printed on the ternimal. If the requested operation is
**	retrieve, the result of the retrieved data is printed on the terminal.
**
** Inputs:
**      service_code			Identifier of the requested GCA service.
**      parameter_list                  Pointer to the data structure containing
**                                      the parameters required by the requested
**                                      service.
**
** Outputs:
**
**
**      status                          Result of the service request execution.
**                                      The following values may be returned:
**
**		E_GCN000_OK		Request accepted for execution.
**		E_GCN003_INV_SVC_CODE	Invalid service code
**		E_GCN004_INV_PLIST_PTR	Invalid parameter list pointer
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      03-Sep-87 (Lin)
**          Initial function creation.
**	21-mar-89 (tmt)
**	    Modified for use with LOCKINFO R6
**      12-mar-2002 (horda03) Bug 107259
**          The information supplied by the NAME server
**          can span multiple buffers, so keep reading
**          the data until the end of data is reached.
**          Otherwise, we get a hang.
**      11-Feb-2003 (horda03) Bug 107259
**          Extension to above fix. If the DB list is
**          large (>20 say), then the data for one of the
**          DBs in the list may straddle two GCA buffers.
**          Therefore, need to remember the information
**          from the previous buffer, and concatenate it
**          with the information in the current buffer.
**          Added MSG_DATA to preserve information from
**          current buffer, if information is in the next
**          buffer.
**     20-Jun-2005   (wanfr01)
**	    Bug 114712, INGCBT571
**	    Added diag lines for increased -L output
[@history_template@]...
*/
typedef struct
{
   i4  row_cnt;   /* How many rows are still to process */
   PTR buf;       /* Pointer to the buffer */
   PTR buf_end;   /* End of data in buf */
   i4  gcn_op;    /* OPeration being performed */
}
MSG_DATA;

static STATUS
IIGCn_nm_oper(service_code, parameter_list)
i4                 service_code;
GCN_CALL_PARMS	   *parameter_list;
{
    GCA_PARMLIST	parms;
    GCA_SD_PARMS	*send;
    GCA_RV_PARMS	*recv;
    GCA_IT_PARMS	*intrp;
    GCA_DA_PARMS	*disassoc;
    STATUS		status;
    STATUS		rstat;
    GCN_D_OPER		*oper_blk;
    GCN_NM_DATA		*nm_data;
    GCN_REQ_TUPLE	*nm_tup;
    GCN_VAL		*nm_val;
    PTR			p;
    PTR			save_pos;
    i4		msg_size = 0;
    i4		length;
    i4			param_size ;
#ifdef BYTE_ALIGN
    i4		temp_nat ;
#endif
    MSG_DATA msg_data = { 0, 0, 0, 0 };

    status = E_DB_OK;

    oper_blk = (GCN_D_OPER *) Out_data_area;
    oper_blk->gcn_flag = parameter_list->gcn_flag; 
    oper_blk->gcn_opcode = service_code;
    msg_size = sizeof(oper_blk->gcn_opcode)+ sizeof(oper_blk->gcn_flag);

/* Put the installation id in the message buffer */
    p = (PTR) parameter_list->gcn_install;
    save_pos = (PTR) &oper_blk->gcn_install;
    nm_val = (GCN_VAL *) save_pos;
    length = STlength(p);
    length = (length == 0) ? 0 : length + 1;
    nm_val->gcn_l_item = length;
    MEcopy(p, (u_i2)nm_val->gcn_l_item, (PTR)nm_val->gcn_value);
    msg_size += (sizeof(i4) + nm_val->gcn_l_item);

    nm_data = (GCN_NM_DATA *) (Out_data_area + msg_size);

/* send one request at a time */
# ifdef BYTE_ALIGN
    temp_nat = 1;
    MEcopy ((PTR) &temp_nat, (u_i2)sizeof(temp_nat), 
			(PTR)&nm_data->gcn_tup_cnt) ;
# else
    nm_data->gcn_tup_cnt = 1;
# endif
    msg_size += sizeof(i4);
    nm_tup = &nm_data->gcn_tuple[0];

/* put the service class */
    p = (PTR) parameter_list->gcn_type;
    save_pos = (PTR) &nm_tup->gcn_type;
    nm_val = (GCN_VAL *) save_pos;
    length = STlength(p);
    length = (length == 0) ? 0 : length + 1;
# ifdef BYTE_ALIGN
    temp_nat = length;
    MEcopy ((PTR)&temp_nat, (u_i2)sizeof (temp_nat), (PTR)&nm_val->gcn_l_item);
    MEcopy(p, (u_i2)temp_nat, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + temp_nat) ;
# else
    nm_val->gcn_l_item = length;
    MEcopy(p, (u_i2)nm_val->gcn_l_item, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + nm_val->gcn_l_item);
# endif
    msg_size += param_size ;

/* put the local uid */
    p = (PTR) parameter_list->gcn_uid;
    save_pos  = save_pos + param_size ;
    nm_val = (GCN_VAL *) save_pos;
    length = STlength(p);
    length = (length == 0) ? 0 : length + 1;
# ifdef BYTE_ALIGN
    temp_nat = length;
    MEcopy ((PTR)&temp_nat, (u_i2)sizeof (temp_nat), (PTR)&nm_val->gcn_l_item);
    MEcopy(p, (u_i2)temp_nat, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + temp_nat) ;
# else
    nm_val->gcn_l_item = length;
    MEcopy(p, (u_i2)nm_val->gcn_l_item, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + nm_val->gcn_l_item);
# endif
    msg_size += param_size ;

/* put the object item*/
    p = (PTR) parameter_list->gcn_obj;
    save_pos  = save_pos + param_size ;
    nm_val = (GCN_VAL *) save_pos;
    length = STlength(p);
    length = (length == 0) ? 0 : length + 1;
# ifdef BYTE_ALIGN
    temp_nat = length;
    MEcopy ((PTR)&temp_nat, (u_i2)sizeof (temp_nat), (PTR)&nm_val->gcn_l_item);
    MEcopy(p, (u_i2)temp_nat, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + temp_nat) ;
# else
    nm_val->gcn_l_item = length;
    MEcopy(p, (u_i2)nm_val->gcn_l_item, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + nm_val->gcn_l_item);
# endif
    msg_size += param_size ;

/* put the value item */
    p = (PTR) parameter_list->gcn_value;
    save_pos = save_pos + param_size ;
    nm_val = (GCN_VAL *) save_pos;
    length = STlength(p);
    length = (length == 0) ? 0 : length + 1;
# ifdef BYTE_ALIGN
    temp_nat = length;
    MEcopy ((PTR)&temp_nat, (u_i2)sizeof (temp_nat), (PTR)&nm_val->gcn_l_item);
    MEcopy(p, (u_i2)temp_nat, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + temp_nat) ;
# else
    nm_val->gcn_l_item = length;
    MEcopy(p, (u_i2)nm_val->gcn_l_item, (PTR)nm_val->gcn_value);
    param_size = (sizeof(i4) + nm_val->gcn_l_item);
# endif
    msg_size += param_size ;

    MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);
    send = &parms.gca_sd_parms;
    send->gca_association_id = Assoc_no;
    send->gca_buffer = Out_comm_buf;
    send->gca_message_type = GCN_NS_OPER;
    send->gca_msg_length = msg_size;
    send->gca_end_of_data = TRUE;	/* Final segment */

/* Make GCA_SEND service call */
    rstat = IIGCa_call(GCA_SEND, &parms, GCA_SYNC_FLAG, 
			    0, (i4) -1, &status);
    if (rstat)
	return rstat;
/*    checkerr("GCA_SEND", rstat, status, &send->gca_status, 0);
*/
    for(;;)
    {
       /* Clear the parameter list */
       MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

       /* Prepare for GCA_RECEIVE service call */
       recv = &parms.gca_rv_parms;
       recv->gca_association_id = Assoc_no;	/* Where to get it from */
       recv->gca_buffer = In_comm_buf;		/* Where we want it */
       recv->gca_b_length = In_l_comm_buf;		/* Our buffer size */
       /* We are requesting a "normal flow" message */
       recv->gca_flow_type_indicator = GCA_NORMAL;

	/* Make GCA_RECEIVE service call */
	rstat = IIGCa_call(GCA_RECEIVE, &parms, GCA_SYNC_FLAG, 
			    0, (i4) -1, &status);	
	if (rstat)
	    return rstat;
	/* checkerr("GCA_RECEIVE", rstat, status, &recv->gca_status, 0);
	*/
	/* Clear the parameter list */
	MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

       /* checkerr("GCA_RECEIVE", rstat, status, &recv->gca_status, 0);
       */
       /* Clear the parameter list */
       MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

       /* Prepare for GCA_INTERPRET service call */
       intrp = &parms.gca_it_parms;
       intrp->gca_buffer = In_comm_buf;

       /* Make GCA_INTERPRET service call */
       rstat = IIGCa_call(GCA_INTERPRET, &parms,
    		    GCA_SYNC_FLAG,	/* synch. operation */
		    0,			/* Parm. for completion rtn. */
		    (i4) 10, &status);
       if (rstat)
          return rstat;
       /*    checkerr("GCA_INTERPRET", rstat, status, &intrp->gca_status, 0);
       */

       /* Get the returned information about location/size of message area */
       In_data_area = intrp->gca_data_area;
       In_l_data_area = intrp->gca_d_length;

	d_print ("got message type:  %d, buf = %p\n",intrp->gca_message_type,
			msg_data.buf);

       switch(intrp->gca_message_type)
       {
          case	GCN_RESULT:
		status = gcn_respond(In_data_area,In_l_data_area, &msg_data);		
		break;
	  case	GCA_ERROR:
		gcn_errhdl(In_data_area, In_l_data_area);
		break;
	  case	GCA_RELEASE:
		gcn_errhdl(In_data_area, In_l_data_area);
	        /* Clear the parameter list */
	        MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

	        /* Invoke GCA_DISASSOC service to clean up released association. */
	        disassoc = &parms.gca_da_parms;
	        disassoc->gca_association_id = Assoc_no;
	        status = IIGCa_call(GCA_DISASSOC, &parms, GCA_SYNC_FLAG, 
			    0, (i4) -1, &status);
		return status;

	   default:
			break;
       }

       if (status || intrp->gca_end_of_data)
       {
	d_print ("done?  status = %d, end = %d\n",status,intrp->gca_end_of_data);
         /* This is the last message, so exit loop */
         break;
       }
    }

    if (msg_data.buf)
    {
       MEfree( msg_data.buf );
    }

    return (status);
} /* end IIGCn_nm_oper */

/*{
** Name: gcn_request - request connection to the name server.
**
** Description:
**	This routine is called by the Name Server Management Utility to set
**	connection with the Name Server. The external name specified in the
**	GCA_REQUEST is "/IINMSVR", which is the Name Server's service class.
**
** Inputs:
**
** Outputs:
**	assoc_id			Association identifier to be used for
**					all subsequent communication.
**      .gca_status                     Success or failure indication.
**	    E_GC0000_OK			Normal completion.
**	    E_GC0001_ASSOC_FAIL		Association establishment attempt failed
**	    E_GC0013_ASSFL_MEM		Insufficient memory available.
**	    E_GC0025_NM_SRVR_ID_ERR	Unable to find the Name Server.
**	    E_GC0026_NM_SRVR_ERR	Communication w/ NS failed.
**			
**	Returns:
**	    none
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      04-Oct-93 (joplin)
**          Released assocations with back levels of gca_peer_protocol. 
**          These could be left open, which hung the MVS name server.
**
*/

static STATUS
gcn_request( assoc_no )
i4	    *assoc_no;
{
    GCA_PARMLIST	parms;
    GCA_RQ_PARMS	*request;
    STATUS		status;
    char		target[ GCN_EXTNAME_MAX_LEN + 1 ];

    /* Clear the parameter list */
    MEfill( sizeof(parms), '\0', (PTR) &parms);

    /* Prepare for GCA_REQUEST service call 
    */
    request = &parms.gca_rq_parms;
    request->gca_partner_name = target;

    STprintf( target, "/IINMSVR" );

    /* Make GCA_REQUEST service call 
    */
    IIGCa_call( 
	    GCA_REQUEST,
	    &parms,
	    GCA_SYNC_FLAG,
	    (PTR)0,
	    (i4) -1, 
	    &status );

    gcn_checkerr( "GCA_REQUEST", &status, 
	request->gca_status, &request->gca_os_status );

    *assoc_no = request->gca_assoc_id;

    return status;
}


/*{
** Name: gcn_send - Issue GCA_SEND to send the specified buffer to the
**		specified association.
**
** Description:
**	This routine is called by the Name Server or NAMU to send the message 
**	to each other. The sync_flag is set when called from the NAMU; otherwise
**	from the Name Server.
**
** Inputs:
**
**	msg_type		message type.
**	assoc_id		Association id.
**	msg_buf			message buffer.
**	msg_size		message size.
**	sync_flag		sync or async
**	eod			end of data flag.
**
** Outputs:
**
**	None
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.  Added eod flag.  Always synchronous for now.
**
*/

static STATUS
gcn_send( msg_type, assoc_id, msg_buf, msg_size, sync_flag, eod )
i4	msg_type;
i4	assoc_id;
char	*msg_buf;
i4	msg_size;
bool	sync_flag;
bool	eod;
{

	GCA_PARMLIST    parms;
	STATUS		status;

	/* Clear the parameter list */

	MEfill( sizeof(parms), 0, (PTR) &parms);

	/* Prepare for GCA_SEND service call */

	parms.gca_sd_parms.gca_association_id = assoc_id;
	parms.gca_sd_parms.gca_buffer = msg_buf;
	parms.gca_sd_parms.gca_message_type = msg_type;
	parms.gca_sd_parms.gca_msg_length = msg_size;
	parms.gca_sd_parms.gca_end_of_data = eod;	/* Final segment */

	/* Make GCA_SEND service call */

	sync_flag = 1;

	IIGCa_call(
		GCA_SEND,
		&parms,
		sync_flag ? GCA_SYNC_FLAG : 0,
		(PTR)0, 			/* no completion routine */
		(i4) -1, 
		&status );
	gcn_checkerr( "GCA_SEND", &status, 
		parms.gca_sd_parms.gca_status,
		&parms.gca_sd_parms.gca_os_status );

	return status;
}


/*{
** Name: gcn_release - Send GCA_RELEASE message and terminate the session.
**
** Description:
**	This routine is called by the Name Server or NAMU to terminate
**	the connection. The sync_flag is set when called from the NAMU; 
**	otherwise from the Name Server.
**
** Inputs:
**
**	assoc_id		Association id.
**	send_flag		send release message? or just disassociate?
**	sync_flag		sync or async
**	length			size of the release message
**
** Outputs:
**
**      .gca_status                     Result of service request completion.
**                                      The following values may be returned:
**	    E_GC0000_OK                 Normal completion
**	    E_GC0005_INV_ASSOC_ID       Invalid association identifier
**
**	Returns:
**	    None
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	01-Mar-89 (seiwald)
**	    Overhauled.  Added send_flag.
**
*/

static STATUS
gcn_release( assoc_no, send_flag, sync_flag )
i4	    assoc_no;
bool	    send_flag;
bool	    sync_flag;
{
	GCA_PARMLIST	parms;
	STATUS		status;

	if( send_flag )
	{
		status = gcn_send( GCA_RELEASE, assoc_no, 
			Out_comm_buf, 0, sync_flag, TRUE );
		if ( status != OK )
			return status;
	}

	/* Clear the parameter list */

	MEfill( sizeof(parms), '\0', (PTR) &parms);

	/* Invoke GCA_DISASSOC service to clean up released association. */

	parms.gca_da_parms.gca_association_id = assoc_no;

	IIGCa_call(
		GCA_DISASSOC,
		&parms,
		GCA_SYNC_FLAG, 
		(PTR)0, 
		(i4) -1, 
		&status);
	gcn_checkerr("GCA_DISASSOC", &status, 
		parms.gca_da_parms.gca_status,
		&parms.gca_da_parms.gca_os_status );

	return status;
}


/*
** Name: gcn_respond - formats the data for output
**
** This routine will format the output and call a routine to place it
** on the appropriate screen.
**
** History
**	3/21/89		tmt	changed for lockinfo R6
**	3/29/89		tmt	call st_nameinfo to place info
**				in linked list.
**     12/8/94       liibi01    fixed string lengths in various buffers
**     11-Feb-2003   (horda03)   Bug 107259.
**        Added MSG_DATA to handle multiple message
**        buffers from name server. Also, the function
**        now returns E_DB_ERROR, if it fails to
**        allocate a buffer for unprocessed data.
**     20-Jun-2005   (wanfr01)
**	  Bug 114712, INGCBT571
**	  Added diag lines for increased -L output
**	  Break out of loop if a split row detected
**	  
*/

static STATUS
gcn_respond(buf, leng, msg_data)
PTR	buf;
i4	leng;
MSG_DATA *msg_data;
{
    GCN_OPER_DATA   *result;
    i4		    row_cnt;
    PTR		    ptr;
    GCN_VAL	    *val_ptr;
#define 	MAXBUF		500
    char	    buffer[MAXBUF];
    char 	    *netware;
    char 	    *laddr;
    i4		    len;
#define 	NETU_NODE	1
#define 	NETU_AUTHORIZE  2
#define 	NETU_DELIMITER  ','
    i4		    netu = FALSE;
    char	    delimit = NETU_DELIMITER;
    char    	    *p;
    char            ctype[GCN_TYP_MAX_LEN + 1];
    char            cobject[GCN_OBJ_MAX_LEN + 1];
    char            cdata[GCN_VAL_MAX_LEN + 1];
    i4              gcn_op;
    i4              data_len;
    i4              buf_len;
    PTR             start;
    STATUS          status;

    if (msg_data->buf)
    {
       /* The current buffer, continues on from the
       ** previous buffer. So, copy the data to the
       ** end of the last buffer.
       */

       MEcopy( buf, leng, msg_data->buf_end );

       row_cnt = msg_data->row_cnt;

       val_ptr = (GCN_VAL *) msg_data->buf;

       gcn_op  = msg_data->gcn_op;

       buf_len = data_len = leng + (msg_data->buf_end - msg_data->buf);
    }
    else
    {
       result  = (GCN_OPER_DATA *)buf;

       val_ptr = &result->gcn_tuple[0].gcn_type;

       row_cnt = result->gcn_tup_cnt;

       gcn_op  = result->gcn_op;

       data_len = leng - CL_OFFSETOF(GCN_OPER_DATA, gcn_tuple);

       buf_len = leng;
    }

    d_print ("gcn_op = %d\n",gcn_op);
    switch(gcn_op)
    {
	case	GCN_RET:
		ptr = (PTR) val_ptr;
		d_print ("Total rows:  %d\n",row_cnt);
		for(; row_cnt--; )
		{
		    /* copy the type field */
		    val_ptr = (GCN_VAL *)ptr;

		    /* Remember where this tuple started, incase the tuple
		    ** spans message buffers.
		    */
                    start = ptr;

		    d_print ("data_len = %d,  sizeoflen= %d\n",data_len,sizeof(len));
                    if (data_len <= sizeof( len ) ) break;
                    data_len -= sizeof( len );

# ifdef BYTE_ALIGN
                    MEcopy ((PTR)&val_ptr->gcn_l_item, (u_i2)sizeof (len), (PTR)&len) ;
# else
                    len = val_ptr->gcn_l_item;
# endif

		    d_print ("%d: data_len = %d,  len= %d\n",__LINE__,data_len,len);
                    if (data_len <= len ) break;

                    data_len -= len;

   		    MEcopy((PTR)val_ptr->gcn_value, (u_i2)len, (PTR)buffer);
		    buffer[len] = EOS;
		    if(!STbcompare(buffer,0,"NODE",0,TRUE))
			netu = NETU_NODE;
		    if(!STbcompare(buffer,0,"LOGIN",0,TRUE))
		 	netu = NETU_AUTHORIZE;
		    if(netu != FALSE)
		        STcopy(" ", ctype);
		    else
		        STprintf(ctype, "%s", buffer);
		    ptr += len + sizeof(i4);

		    /* Skip the uid field */
		    val_ptr = (GCN_VAL *)ptr;

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,sizeof(len));
                    if (data_len <= sizeof( len ) ) break;
                    data_len -= sizeof( len );

# ifdef BYTE_ALIGN
   		    MEcopy ((PTR)&val_ptr->gcn_l_item, (u_i2)sizeof (len),
			(PTR)&len);
# else
   		    len = val_ptr->gcn_l_item;
# endif

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,len);
                    if (data_len <= len ) break;
                    data_len -= len;

   		    ptr += sizeof(i4) + len ;

		    /* copy the object name field */
		    val_ptr = (GCN_VAL *)ptr;

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,len);
                    if (data_len <= sizeof( len ) ) break;
                    data_len -= sizeof( len );

# ifdef BYTE_ALIGN
   		    MEcopy ((PTR)&val_ptr->gcn_l_item, (u_i2)sizeof (len),
			(PTR)&len);
# else
   		    len = val_ptr->gcn_l_item;
# endif

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,len);
                    if (data_len <= len ) break;
                    data_len -= len;

   		    MEcopy((PTR)val_ptr->gcn_value, (u_i2)len, (PTR)buffer);
   		    buffer[len] = EOS;
   		    STprintf(cobject, "%s", buffer);
   		    ptr += len + sizeof(i4);

		    /* copy the data value field */
    		    val_ptr = (GCN_VAL *)ptr;

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,sizeof(len));
                    if (data_len <= sizeof( len ) ) break;
                    data_len -= sizeof( len );

# ifdef BYTE_ALIGN
   		    MEcopy ((PTR)&val_ptr->gcn_l_item, (u_i2)sizeof (len),
			(PTR)&len);
# else
   		    len = val_ptr->gcn_l_item;
# endif

		    d_print ("%d: data_len = %d,  sizeoflen= %d\n",__LINE__,data_len,len);
                    if (data_len < len ) break;
                    data_len -= len;

       		    MEcopy((PTR)val_ptr->gcn_value, (u_i2)len, (PTR)buffer);
    		    buffer[len] = EOS;	/* The end of buffer is terminated */
		    d_print ("netu = %d\n",netu);
		    if(netu != FALSE)
		    {		/* re format as appropriate */
			switch(netu)
			{
			  case NETU_NODE:
			    if (p = (char *) STindex(buffer, &delimit, len))
			    {
				*p = EOS; 	/* buffer now has node */
    		    		netware = ++p;
			    	if (p = (char *) STindex(netware, &delimit,
					 len - (p - buffer)))
				{
				    *p = EOS;	/* netware terminated */
				    laddr = ++p;
				    STcopy(netware, cdata);
    		            	    STprintf(cdata, "%s %s", cdata, buffer);
    		            	    STprintf(cdata, "%s %s", cdata ,laddr);
				}
				else
				{
			    		/* only contains node,netware */
				STcopy(netware, cdata);
    		            	STprintf(cdata, "%s", cdata, buffer);
	 			break;
				}
			    }
			    else
			    {	/* only contains node */
    		            	STcopy(buffer, cdata);
	 			break;
			    }
			  break;
			  case NETU_AUTHORIZE:
			    if (p = (char *) STindex(buffer, &delimit, len))
				*p = EOS; 	/* buffer now has user name */
    		            STcopy(buffer, cdata);
			  break;
			}
		    }
		    else   /* end of IF(netu != FALSE) */
	     	    {
    		        STcopy(buffer, cdata);
		    }
    		    ptr += len + sizeof(i4);
                    cdata[GCN_VAL_MAX_LEN] = EOS; /* chop off anything beyond */
		    /* pass info for storage */
		    d_print ("cnt: %d:  %s  %s\n",row_cnt,cdata,cobject);
    		    st_nameinfo(ctype, cobject, cdata);

		}	/* End of FOR loop */

                if (data_len)
                {
                   /* Message incomplete, so copy the data to a temp buffer */

                   if (!msg_data->buf)
                   {
                      msg_data->buf = MEreqmem(0, 2 * leng, FALSE, &status);

                      if (!msg_data->buf)
                      {
                         /* Add message */
                         return E_DB_ERROR;
                      }

                      data_len = buf_len - (start - buf);
                   }
                   else
                   {
                      data_len = buf_len - (start - msg_data->buf);
                   }

                   msg_data->row_cnt = row_cnt + 1;
                   msg_data->gcn_op  = gcn_op;

                   MEcopy(start, data_len, msg_data->buf);

                   msg_data->buf_end = (PTR) (msg_data->buf + data_len);
                }
                else if (msg_data->buf)
                {
                   MEfree( msg_data->buf );

                   MEfill( sizeof(MSG_DATA), 0, msg_data );
                }
		break;
    }

    return E_DB_OK;
}

static VOID
gcn_errhdl(buf, leng)
PTR	buf;
i4	leng;
{
    GCA_ER_DATA	    *ptr = (GCA_ER_DATA *) buf;
    GCA_E_ELEMENT   *element = (GCA_E_ELEMENT *)&ptr->gca_e_element[0];
    i4	    ele_count = *((i4 *) buf);

    while (ele_count--)
    {
	SIprintf("%s\n",element->gca_error_parm[0].gca_value);
	element += sizeof(GCA_E_ELEMENT) - 1 
		   +  element->gca_error_parm[0].gca_l_value;
    }

}


/*
** Name: gcn_checkerr() - GCA error checker for IINAMU, NETU, or NETUTIL
**
** Description:
**      Check returns from GCA calls; format and optionally print or
**      invoke a message handler for an error message.
**
** Inputs:
**      service - name of GCA call (ignored)
**      status, gca_status, cl_status - status milieu
**
** Outputs:
**      status - OR or !OK
**
*/

static VOID
gcn_checkerr( char *service, STATUS *status, 
	      STATUS gca_status, CL_ERR_DESC *cl_error )
{
    if( *status == OK &&
        gca_status != E_GC0000_OK  &&
	gca_status != E_GCFFFF_IN_PROCESS )
    {
	*status = gca_status;
    }
}

/*
** name_connect - this routine connects to the GCA.
**
** Input - gcn parameter struct
**
** Output - Success or error code
**
** History
**	3/23/89		(tmt)	created from IIgcn_request.  Needed to
**				be able to deallocate memory - 
**				which GCA uses for communication buffers are
**				allocated in the IIgcn_request routine are
**				deallocated in gcn_call.  
*/
static STATUS
name_connect(init_status)
PTR	init_status;
{
    GCA_PARMLIST	parms;
    GCA_IN_PARMS	*init;
    GCA_FO_PARMS	*format;
    STATUS		return_code;
    STATUS		status;

#ifdef lint
    init_status = init_status;
#endif

    /* Allocate Communication buffer */
    Out_l_comm_buf = 1024;  /* is not set to any value */
    Out_comm_buf = MEreqmem((u_i4)0, (u_i4)Out_l_comm_buf, FALSE, &status);
    if (Out_comm_buf == NULL)
	return status;

    In_l_comm_buf = 1024;
    In_comm_buf = MEreqmem((u_i4)0, (u_i4)In_l_comm_buf, FALSE, &status);
    if (In_comm_buf == NULL)
	return status;

/* Clear the parameter list */
    MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

/* Prepare for GCA_INITIATE service call */
    init = &parms.gca_in_parms;
    init->gca_normal_completion_exit = NULL;
    init->gca_expedited_completion_exit = NULL;

/* Make GCA_INITIATE service call */
    return_code = IIGCa_call(GCA_INITIATE, &parms, GCA_SYNC_FLAG, 0,
	(i4) -1, &status);
    if (return_code)
	return return_code;
/*    checkerr("GCA_INITIATE", return_code, status, &init->gca_status, 0);
*/

/* Clear the parameter list */
    MEfill((u_i2)sizeof(parms), '\0', (PTR) &parms);

/* Prepare for GCA_FORMAT service call */
    format = &parms.gca_fo_parms;
    format->gca_buffer = Out_comm_buf;
    format->gca_b_length = Out_l_comm_buf;

/* Make GCA_FORMAT service call */
    return_code = IIGCa_call(GCA_FORMAT, &parms, GCA_SYNC_FLAG, 
			0, (i4) -1, &status);
    if (return_code)
	return return_code;
/*    checkerr("GCA_FORMAT", return_code, status, &format->gca_status, 0);
*/

/* Get the returned information about useable area */
    Out_data_area = format->gca_data_area;
    Out_l_data_area = format->gca_d_length;

    return return_code;
}

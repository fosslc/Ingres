/*
** Copyright (c) 2004 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <cm.h>
# include <tm.h>
# include <gl.h>
# include <me.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "conn.h"
# include "repevent.h"

/**
** Name:	event.c - register and process events
**
** Description:
**	Defines
**		RSdbevents_register	- register dbevents
**		RSdbevent_get		- get dbevent
**		RSdbevent_get_nowait	- get dbevent with no wait
**		evtCallback		- catchEvent callback
**		getEvent		- get a dbevent
**
** History:
**	16-dec-96 (joea)
**		Created based on event.sc and eventit.sc in replicator library.
**	25-mar-97 (joea)
**		Remove message 1255.
**	07-may-97 (joea)
**		Correct limit conditions in for loops.  Only display message
**		1017 if a dbevent was received.
**	03-feb-98 (joea)
**		Replace SELECTs to determine timeout time by TM functions calls.
**	21-apr-98 (joea)
**		Convert to use OpenAPI instead of ESQL.
**	15-jan-99 (abbjo03)
**		Replace IIsw_getEvent with IIsw_with IIsw_catchEvent and
**		IIsw_waitEvent. Add code to evtCallback to queue events for
**		later retrieval. Move RSgo_again here from replicat.c. Add
**		getEvent function.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	02-mar-99 (abbjo03)
**		Replace II_PTR error handle parameters with IIAPI_GETEINFOPARM
**		error parameter blocks.
**	22-mar-99 (abbjo03)
**		Adjust from seconds to milliseconds in getEvent. Only call
**		IIsw_catchEvent if the previous callback succeeds.
**	25-mar-99 (abbjo03) bug 96067
**		In RSdbevent_get_nowait, return 2 if a distribute event is
**		received and the server is quiet. If a -EVT set_server event is
**		received reset timeout_secs.
**	18-may-99 (abbjo03)
**		When shutting down the server, cancel the catchEvent request.
**	21-jul-99 (abbjo03)
**		Replace length of event text by DB_EVDATA_MAX.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-mar-2002 (kinte01)
**	    Add include of me.h to pick up definition of mereqmem
**	30-mar-2004 (gupsh01)
**	    Added getQinfoParm as input parameter to II_sw.. calls.
**      22-Nov-2007 (kibro01) b119245
**          Add a flag to requiet a server if it was previously quiet
**          but has been unquieted for a connection retry.
**/

# define NLKUP_EVENTS		10	/* number of server lookup events */


typedef struct
{
	char	dbevent[DB_MAXNAME+1];
	i2	action;
	i2	s_flag;
} RS_EVENT;

typedef struct q_evt
{
	struct q_evt	*next;
	char		evtName[DB_MAXNAME+1];
	SW_EVT_TEXT	evtText;
} RS_QUEUE_EVENT;


GLOBALDEF i4	RSdbevent_timeout = 0;
GLOBALDEF bool	RSgo_again = FALSE;


static RS_EVENT	lookup[NLKUP_EVENTS];
static i4	timeout_secs;
static i4	num_events;

static RS_QUEUE_EVENT		*evq_head;
static RS_QUEUE_EVENT		*evq_tail;
static IIAPI_CATCHEVENTPARM	catchParm;
static bool			event_caught = FALSE;


static II_VOID II_FAR II_CALLBACK evtCallback(II_PTR closure, II_PTR parmBlock);
static void getEvent(i4 timeout, char *evtName, char *evtText);


/*{
** Name:	RSdbevents_register - register dbevents
**
** Description:
**	Registers db events.  Select events from the dd_events table and load
**	the lookup array with events and the actions they are to take.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK	- successfully registered events
**	FAIL	- error selecting or registering events
*/
STATUS
RSdbevents_register()
{
	i4			i;
	i4			end_array;
	char			stmt[100];
	RS_CONN			*conn = &RSconns[NOTIFY_CONN];
	IIAPI_DATAVALUE		cdv[3];
	IIAPI_STATUS		status;
	IIAPI_GETEINFOPARM	errParm;
	II_PTR			stmtHandle;
	IIAPI_GETQINFOPARM	getQinfoParm;

	stmtHandle = NULL;
	for (i = 0; ; ++i)
	{
		SW_COLDATA_INIT(cdv[0], lookup[i].dbevent);
		SW_COLDATA_INIT(cdv[1], lookup[i].action);
		SW_COLDATA_INIT(cdv[2], lookup[i].s_flag);
		status = IIsw_selectLoop(RSlocal_conn.connHandle,
			&RSlocal_conn.tranHandle,
			ERx("SELECT dbevent, action, s_flag FROM dd_events \
WHERE action > 0"),
			0, NULL, NULL, 3, cdv, &stmtHandle, &getQinfoParm, 
			&errParm);
		if (status == IIAPI_ST_NO_DATA || status != IIAPI_ST_SUCCESS)
			break;
		SW_CHA_TERM(lookup[i].dbevent, cdv[0]);
	}
	if (status != IIAPI_ST_NO_DATA && status != IIAPI_ST_SUCCESS)
	{
		/* DBMS error %d selecting events */
		messageit(1, 1249, errParm.ge_errorCode, errParm.ge_message);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		return (FAIL);
	}
	if (!IIsw_getRowCount(&getQinfoParm))
	{
		messageit(1, 1250);
		IIsw_close(stmtHandle, &errParm);
		IIsw_rollback(&RSlocal_conn.tranHandle, &errParm);
		/* No events found */
		return (FAIL);
	}
	IIsw_close(stmtHandle, &errParm);
	IIsw_commit(&RSlocal_conn.tranHandle, &errParm);
	end_array = num_events = i;

	for (i = 0; i < num_events; ++i)
		if (lookup[i].s_flag)
		{
			STprintf(lookup[end_array].dbevent, ERx("%s%d"),
				lookup[i].dbevent, (i4)RSserver_no);
			lookup[end_array].action = lookup[i].action;
			lookup[end_array].s_flag = lookup[i].s_flag;
			++end_array;
		}
	num_events = end_array;

	for (i = 0; i < num_events; ++i)
	{
		STprintf(stmt, ERx("REGISTER DBEVENT %s"), lookup[i].dbevent);
		if (IIsw_queryNP(conn->connHandle, &conn->tranHandle, stmt,
			&stmtHandle, &getQinfoParm, &errParm) != IIAPI_ST_SUCCESS)
		{
			/* DBMS error %d registering event %s */
			messageit(3, 1252, errParm.ge_errorCode,
				lookup[i].dbevent, errParm.ge_message);
			IIsw_close(stmtHandle, &errParm);
			IIsw_rollback(&conn->tranHandle, &errParm);
			return (FAIL);
		}
		IIsw_close(stmtHandle, &errParm);
		IIsw_commit(&conn->tranHandle, &errParm);
	}

	catchParm.ce_eventHandle = NULL;
	return (IIsw_catchEvent(conn->connHandle, &catchParm, evtCallback, NULL,
		&catchParm.ce_eventHandle, &errParm));
}


/*{
** Name:	RSdbevent_get - get dbevent
**
** Description:
**	Receives and processes dbevents.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	0	- shutdown the server (dbevent dd_stop_serverN(2) or error)
**	1	- process the replication (dbevent dd_distribute(4), or
**		  timed out on get dbevent
**	2	- out of events.  Look in the proper queue to see if a
**		  replication needs to be processed.
*/
STATUS
RSdbevent_get()
{
	char	tmp[DB_EVDATA_MAX+1];
	i4	action;
	i4	i;
	STATUS	stat;
	i4	timeout;
	char	evtText[DB_EVDATA_MAX+1];
	char	evtName[DB_MAXNAME+1];
	IIAPI_GETEINFOPARM	errParm;

	while (TRUE)	/* event loop */
	{
		stat = RSdbevent_get_nowait();
		if (stat != 2)
			return (stat);

		if (RSdbevent_timeout && timeout_secs == 0)
			timeout_secs = TMsecs() + RSdbevent_timeout;
		if (RSdbevent_timeout)
		{
			if (!RSquiet)
			{
				if (RSrequiet)
				{
					RSquiet = TRUE;
					RSrequiet = FALSE;
				}
				timeout = RSdbevent_timeout;
			}
			else
			{
				timeout = timeout_secs - TMsecs();
				if (timeout < 0)
					timeout = timeout_secs = 0;
			}
		}
		else
		{
			timeout = -1;
		}
		getEvent(timeout, evtName, evtText);
		if (*evtName == EOS)	/* timed out -- replicate */
		{
			timeout_secs = 0;
			return (2);
		}

		action = 1;
		/* find the action of the received event */
		for (i = 0; i < num_events; ++i)
		{
			if (STbcompare(evtName, 0, lookup[i].dbevent, 0, TRUE)
				== 0)
			{
				action = (i4)lookup[i].action;
				break;
			}
		}

		/* processing event %s with action %d */
		messageit(4, 1256, evtName, action);
		switch (action)
		{
		case 1:	/*
			** a record has been manipulated and may need to be
			** replicated.
			*/
			if (!RSquiet)
			{
				if (RSrequiet) 
				{
					RSquiet = TRUE;
					RSrequiet = FALSE;
				}
				return (1);
			}
			break;

		case 2:		/* stop the server */
			IIsw_cancelEvent(catchParm.ce_eventHandle, &errParm);
			IIsw_close(catchParm.ce_eventHandle, &errParm);
			return (0);

		case 3:		/* set a server parameter */
			if (*evtText != EOS && CMcmpcase(evtText, ERx("-"))
					!= 0)
				STprintf(tmp, ERx("-%s"), evtText);
			else
				STcopy(evtText, tmp);
			set_flags(tmp);
			if (STbcompare(tmp, 4, ERx("-EVT"), 4, TRUE) == 0)
				timeout_secs = 0;
			RSping(FALSE);
			break;

		case 4:		/* process the replication */
			return (1);

		case 5:		/* respond to ping from Replication Monitor */
			RSping(TRUE);
			break;

		default:
			return (0);
		}
	}
}


/*{
** Name:	RSdbevent_get_nowait - get dbevent with no wait
**
** Description:
**	Clears out the database events that are in the queue for this server.
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	0	- shutdown the server (dbevent dd_stop_serverN(2) or error)
**	1	- process the replication (dbevent dd_distribute(4))
**	2	- out of events. Look in the proper queue to see if a
**		  replication needs to be processed.
**
**     22-Sep-2009 (coomi01) b112552
**         DbEvents are temporarily ignored whilst the distribution queue
**         is busy. This causes problems when the queue is large.
*/
STATUS
RSdbevent_get_nowait()
{
	i4	action;
	i4	i;
	STATUS	ret_val = 2;
	char	tmp[DB_EVDATA_MAX+1];
	char	evtText[DB_EVDATA_MAX+1];
	char	evtName[DB_MAXNAME+1];
	IIAPI_GETEINFOPARM	errParm;
        static int wfctr=1;

	while (TRUE)	/* event loop */
	{
		wfctr++;

		if ( wfctr > 100 ) 
		{
        		/*
			** Bug 112552 - add a delay every now and then
        		** Replicator events get sent regularly, so this has minimal effect
        		*/
			getEvent(1000, evtName, evtText);
			wfctr = 0;
		}
		else
		{
			getEvent(0, evtName, evtText);
		}

		if (*evtName == EOS)
			return (ret_val);

		action = 1;
		/* find the action of the received event */
		for (i = 0; i <= num_events; ++i)
			if (STbcompare(evtName, 0, lookup[i].dbevent, 0, TRUE)
				== 0)
			{
				action = (i4)lookup[i].action;
				break;
			}

		/* processing event %s with action %d */
		messageit(4, 1256, evtName, action);
		switch (action)
		{
		case 1:
			if (RSquiet)
			{
				if (RSrequiet)
				{
					RSquiet = TRUE;
					RSrequiet = FALSE;
				}
				return (2);
			}
			ret_val = 1;
			RSgo_again = TRUE;
			break;

		case 2:		/* stop the server */
			IIsw_cancelEvent(catchParm.ce_eventHandle, &errParm);
			IIsw_close(catchParm.ce_eventHandle, &errParm);
			return (0);

		case 3:		/* set a server parameter */
			if (*evtText != EOS && CMcmpcase(evtText, ERx("-"))
					!= 0)
				STprintf(tmp, ERx("-%s"), evtText);
			else
				STcopy(evtText, tmp);
			set_flags(tmp);
			if (STbcompare(tmp, 4, ERx("-EVT"), 4, TRUE) == 0)
				timeout_secs = 0;
			RSping(FALSE);	/* ignore if -NMO */
			break;

		case 4:		/* process the replication */
			ret_val = 1;
			break;

		case 5:		/* respond to ping from Replication Monitor */
			RSping(TRUE);	/* respond even if -NMO */
			break;

		default:
			return (0);
		}
	}
}


/*{
** Name:	evtCallback - catchEvent callback
**
** Description:
**	Called when an event arrives or there is an event timeout.
**
** Inputs:
**	closure		- closure parameter
**	parmBlock	- catchEvent parameter block
**
** Outputs:
**	none
**
** Returns:
**	none
*/
static II_VOID II_FAR II_CALLBACK
evtCallback(
II_PTR	closure,
II_PTR	parmBlock)
{
	IIAPI_CATCHEVENTPARM	*catchParm = (IIAPI_CATCHEVENTPARM *)parmBlock;
	IIAPI_GETEINFOPARM	errParm;
	RS_QUEUE_EVENT		*qevt;

	if (catchParm->ce_genParm.gp_status == IIAPI_ST_SUCCESS)
	{
		event_caught = TRUE;
		qevt = (RS_QUEUE_EVENT *)MEreqmem(0,
			(u_i4)sizeof(RS_QUEUE_EVENT), TRUE, (STATUS *)NULL);
		if (!qevt)
			return;
		STcopy(catchParm->ce_eventName, qevt->evtName);
		if (catchParm->ce_eventInfoAvail)
			IIsw_getEventInfo(catchParm->ce_eventHandle,
				&qevt->evtText, &errParm);
		qevt->next = NULL;
		if (evq_head == NULL)
		{
			evq_head = evq_tail = qevt;
		}
		else
		{
			evq_tail->next = qevt;
			evq_tail = qevt;
		}
	}
}


/*{
** Name:	getEvent - get a dbevent
**
** Description:
**	Wait for a database event. If one arrives, queue a catchEvent.
**
** Inputs:
**	timeout	- number of seconds to wait
**
** Outputs:
**	evtName	- event name
**	evtText	- event text
**
** Returns:
**	none
** History:
**      22-Sep-2009 (coomi01) b112552
**         On zero timeout requests add a few milli-seconds to allow events through.
**	
*/
static void
getEvent(
i4	timeout,
char	*evtName,
char	*evtText)
{
	RS_QUEUE_EVENT	*evt;
	char		*p;
	RS_CONN		*conn = &RSconns[NOTIFY_CONN];
	IIAPI_GETEINFOPARM	errParm;
	static int waitctr=0;

	if (timeout && timeout != -1)
		timeout *= 1000;

	if (timeout == 0)
	{
		IIsw_waitEvent(conn->connHandle, (II_LONG)waitctr, &errParm);
	}
	else
	{
		IIsw_waitEvent(conn->connHandle, (II_LONG)timeout, &errParm);
	}

	if (event_caught)
	{
		event_caught = FALSE;
		IIsw_catchEvent(conn->connHandle, &catchParm, evtCallback, NULL,
			&catchParm.ce_eventHandle, &errParm);
	}

	if (evq_head == NULL)
	{
		if (timeout == 0)
		{
			waitctr++;
		}
		*evtName = *evtText = EOS;
		return;
	}

	/* 
	** Reset wait count whenever an event is detected 
	*/
	waitctr = 0;

	evt = evq_head;
	evq_head = evt->next;
	if (evq_head == NULL)
		evq_tail = NULL;
	evt->next = NULL;
	STcopy(evt->evtName, evtName);
	STlcopy(evt->evtText.text, evtText, evt->evtText.len);

	/* remove possible trailing text introduced by VMS event propagator */
	p = STindex(evtText, ERx("|"), 0);
	if (p != NULL)
		*p = EOS;
}

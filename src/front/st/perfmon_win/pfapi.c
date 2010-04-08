/*
** Copyright (c) 1998, 2004 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <st.h>
# include <iicommon.h>
# include <iiapi.h>
# include <sapiw.h>
# include "pfapi.h"
# include "pfctrmsg.h"	/* error messages */
# include "pfmsg.h"	/* error message macros and definitions */

/*
** Name:    pfapi.c - Communications API interface for the performance DLL
**
** Description:
** 	This interface provides a synchronous interface to an Ingres DBMS
** 	Server via the Synchronous API Wrapper (SAW). I leave it to
** 	a better day (or better person) to do the asynchronous version.
** 
** 	The following routines are contained:
** 
** 	pfApiInit - Initializes the service
** 	pfApiTerm - Terminates the service
** 	pfApiConnect - Connects to an Ingres server
** 	pfApiDisconnect - Disconnects from an Ingres server
**	pfApiSelectCache - A select loop for a database query
** 	pfApiSelectThreads - Use SQL to select CSsampler IMA thread statistics
** 	pfApiSelectLocks - Select IMA lock statistics
** 	pfApiSelectSampler - Select CSsampler block IMA object
**	pfApiExecSelect - Execute a given select statement which returns one
**			  value
** 
** History:
**      15-oct-1998 (wonst02)
**          Created.
** 	19-dec-1998 (wonst02)
** 	    Added pfApiSelectSampler() function.
** 	07-jan-1999 (wonst02)
** 	    Add definitions for cssampler event and lock objects.
**	02-mar-1999 (abbjo03)
**	    Replace II_PTR error handle parameters by IIAPI_GETEINFOPARM error
**	    parameter block pointers.
** 	21-apr-1999 (wonst02)
** 	    Add connection parameter. Re-instate pfApiSelectSampler( ) routine.
**	23-aug-1999 (somsa01)
**	    Added code to retrieve information on a per server basis, as well
**	    as to start/stop the Sampler thread in that server.
**	22-oct-1999 (somsa01)
**	    Added pfApiExecSelect() for execution of a given singleton select
**	    statement. Also, added another parameter to pfApiConnect() and
**	    pfApiDisconnect() to tell if we are connecting to imadb or not.
**	31-jan-2000 (somsa01)
**	    Changed ServerID to be dynamic.
**	11-apr-2000 (somsa01)
**	    Due to architechural bugs in using the OpenAPI execute procedure
**	    function, this has been changed to use the OpenAPI execute
**	    query function.
**      29-nov-2004 (rigka01) bug 111394, problem INGSRV2624
**          Cross the part of change 466624 from main to ingres26 that
**          applies to bug 111394 (change 466196).
**	31-mar-2004 (penga03)
**	    Added an argument &qinfoParm for IIsw_query, IIsw_selectSingleton, 
**	    IIsw_selectLoop and IIsw_selectSingletonRepeated.
**	30-mar-2004 (somsa01)
**	    Added IIAPI_GETQINFOPARM argument to IIsw_ functions.
**	05-Apr-04 (penga03)
**	    Removed the redefinition of IIsw_selectLoop.
*/

/* the Server ID to watch */
extern char	*ServerID;

bool		SamplerStarted = FALSE;

/* Space for 1023-char, double-null-terminated wide-character message */
# define MAXMESSAGE	((1023 + 1) * 2)


/*
**  Name: pfApiInit - Initializes the service
**
**  Description:
**      Performs any initialization required by the pfApi service.
**
**  Inputs:
**
**  Outputs:
**
**  Returns:
**      OK if successful.
** 	FAIL, if not.
**
**  History:
**      25-oct-1998 (wonst02)
**          Created.
** 	16-apr-1999 (wonst02)
** 	    Add connection parameter.
**	26-jan-2004 (penga03)
**	    Added an argument for IIsw_initialize.  
*/
STATUS II_EXPORT
pfApiInit (PFAPI_CONN *conn)
{
	conn->state = PFCTRS_STAT_INITING;
	if (IIsw_initialize(&conn->envHandle,0) == IIAPI_ST_SUCCESS)
	{
	    conn->state = PFCTRS_STAT_INITED;
	    return OK;
	}
	else
	{
	    return FAIL;
	}
}


/*
**  Name: pfApiTerm - Terminates the service
**
**  Description:
**      Performs any termination required by the pfApi service.
**
**  Inputs:
**
**  Outputs:
**
**  Returns:
**      OK if successful.
** 	FAIL, if not.
**
**  History:
**      27-oct-1998 (wonst02)
**          Created.
** 	16-apr-1999 (wonst02)
** 	    Add connection parameter.
** 
*/
STATUS II_EXPORT
pfApiTerm (PFAPI_CONN *conn)
{

	if (conn->state < PFCTRS_STAT_INITED)
	    return FAIL;
	conn->state = PFCTRS_STAT_TERMING;
	if (IIsw_terminate(conn->envHandle) == IIAPI_ST_SUCCESS)
	{	
	    conn->state = PFCTRS_STAT_INVALID;
	    return OK;
	}
	else
	{	
	    return FAIL;
	}
}


/*
**  Name: pfApiConnect - Connects to an Ingres server
**
**  Description:
**      Connects to the server via the Synchronous API Wrapper (SAW).
**
**  Inputs:
**      dbName          - Database name to connect to
**	flags		- Database connect options
**      user            - Registered client name
**
**  Outputs:
**     	conn          - Pointer to connection handle
**
**  Returns:
**      OK if successful.
**
**  History:
**      15-oct-1998 (wonst02)
**          Created.
**	23-aug-1999 (somsa01)
**	    Added enabling data from all servers and the starting of the
**	    Sampler thread in a particular server. If the Sampler thread
**	    is already started, do not restart it.
**	30-oct-1999 (somsa01)
**	    Added another parameter to tell if we are connecting to imadb.
**	11-apr-2000 (somsa01)
**	    Due to architechural bugs in using the OpenAPI execute procedure
**	    function, this has been changed to use the OpenAPI execute
**	    query function.
*/
STATUS II_EXPORT
pfApiConnect (char *dbName, char *userid, char *password, PFAPI_CONN *conn, bool IsImaDb)
{
	IIAPI_GETEINFOPARM	errParm;
	char			stmt[2048];
    IIAPI_GETQINFOPARM	qinfoParm;

	if (conn->state != PFCTRS_STAT_INITED)
	    return FAIL;
	conn->connHandle = NULL;
	conn->tranHandle = NULL;
	conn->state = PFCTRS_STAT_CONNECTING;
        conn->status = IIsw_connect (dbName, userid, password, 
		&conn->connHandle, &conn->tranHandle, &errParm);
    	if ((conn->status == IIAPI_ST_SUCCESS) && conn->connHandle)
        {
	    conn->state = PFCTRS_STAT_CONNECTED;
        }
	else
	{
            REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CONNECT, LOG_USER,
                &conn->status, sizeof(conn->status),
	    	&errParm.ge_message);
	    return FAIL;
	}

	/*
	** Now, enable getting data from ALL registered servers, and turn
	** on the Sampler thread in a perticular server.
	*/
	if (IsImaDb && strcmp(ServerID, ""))
	{
	    IIAPI_DATAVALUE	cdata[1];
	    int			numsamples = 0;
	    IIAPI_GETQINFOPARM	qinfoParm;

	    STcopy("execute procedure ima_set_vnode_domain", stmt);
	    conn->status = IIsw_query(conn->connHandle, &conn->tranHandle,
				      stmt, 0, NULL, NULL, NULL, NULL,
				      &conn->stmtHandle, &qinfoParm, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    {
		REPORT_ERROR_DATA_A (PFAPI_EXEC_PROCEDURE_ERROR, LOG_USER,
				     &errParm.ge_errorCode,
				     sizeof(errParm.ge_errorCode), 
				     &errParm.ge_message);
	    }

	    conn->status = IIsw_close(conn->stmtHandle, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    {
		REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
				     &errParm.ge_errorCode,
				     sizeof(errParm.ge_errorCode), 
				     &errParm.ge_message);
	    }
	    conn->stmtHandle = NULL;


	    /*
	    ** See if the Sampler thread is already started.
	    */
	    STprintf(stmt,
		"select numsamples from ima_cssampler_stats where server='%s'",
		ServerID);
	    SW_COLDATA_INIT(cdata[0], numsamples);
	    conn->status = IIsw_selectSingleton(conn->connHandle,
						&conn->tranHandle, stmt, 0,
						NULL, NULL, 1, cdata, NULL,
						NULL, &conn->stmtHandle,
						&qinfoParm, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    {
		REPORT_ERROR_DATA_A (PFAPI_SELECT_ERROR, LOG_USER,
				     &errParm.ge_errorCode,
				     sizeof(errParm.ge_errorCode), 
				     &errParm.ge_message);
		conn->stmtHandle = NULL;
		SamplerStarted = FALSE;
	    }
	    else
	    {
		conn->status = IIsw_close(conn->stmtHandle, &errParm);
		if (conn->status != IIAPI_ST_SUCCESS)
		{
		    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
					 &errParm.ge_errorCode,
					 sizeof(errParm.ge_errorCode), 
					 &errParm.ge_message);
		}
		conn->stmtHandle = NULL;

		SamplerStarted = (numsamples == 0 ? FALSE : TRUE);
	    }


	    /*
	    ** Now, turn on the Sampler thread.
	    */
	    if (!SamplerStarted)
	    {
		STprintf(stmt,
			"execute procedure ima_start_sampler(server_id=\'%s\')",
			ServerID);
		conn->status = IIsw_query(conn->connHandle, &conn->tranHandle,
					  stmt, 0, NULL, NULL, NULL, NULL,
					  &conn->stmtHandle, &qinfoParm,
					  &errParm);
		if (conn->status != IIAPI_ST_SUCCESS)
		{
		    REPORT_ERROR_DATA_A (PFAPI_EXEC_PROCEDURE_ERROR, LOG_USER,
					 &errParm.ge_errorCode,
					 sizeof(errParm.ge_errorCode), 
					 &errParm.ge_message);
		}

		conn->status = IIsw_close(conn->stmtHandle, &errParm);
		if (conn->status != IIAPI_ST_SUCCESS)
		{
		    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
					 &errParm.ge_errorCode,
					 sizeof(errParm.ge_errorCode), 
					 &errParm.ge_message);
		}
		conn->stmtHandle = NULL;
	    }


	    /*
	    ** Close out the transaction.
	    */
	    IIsw_commit(&conn->tranHandle, &errParm);
	    conn->tranHandle = NULL;
	}

	return OK; 
}


/*
**  Name: pfApiDisconnect - Disconnects from an Ingres server
**
**  Description:
**      Disconnects from a server via the Synchronous API Wrapper (SAW).
**
**  Inputs:
**	conn		- -> to the Connection handle.
**	bCommit		- True -> commit the connection
**			  False -> rollback the connection
**
**  Outputs:
**
**  Returns:
**      OK if successful.
**
**  History:
**      15-oct-1998 (wonst02)
**          Created.
**	24-aug-1999 (somsa01)
**	    Added code to shut down the Sampler thread if it was not already
**	    started.
**	30-oct-1999 (somsa01)
**	    Added another parameter to tell if we are disconnecting from imadb.
**	11-apr-2000 (somsa01)
**	    Due to architechural bugs in using the OpenAPI execute procedure
**	    function, this has been changed to use the OpenAPI execute
**	    query function.
*/
void II_EXPORT
pfApiDisconnect (PFAPI_CONN *conn, bool IsImaDb)
{
	IIAPI_GETQINFOPARM qinfoParm;
	IIAPI_GETEINFOPARM errParm;
	STATUS		ret = IIAPI_ST_SUCCESS;
	char		stmt[2048];

	if (conn->state < PFCTRS_STAT_CONNECTED)
	    return;

	/*
	** Turn off the Sampler thread.
	*/
	if (IsImaDb && strcmp(ServerID, "") && !SamplerStarted)
	{
	    STprintf(stmt,
		     "execute procedure ima_stop_sampler(server_id=\'%s\')",
		     ServerID);
	    conn->status = IIsw_query(conn->connHandle, &conn->tranHandle,
				      stmt, 0, NULL, NULL, NULL, NULL,
				      &conn->stmtHandle, &qinfoParm, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    {
		REPORT_ERROR_DATA_A (PFAPI_EXEC_PROCEDURE_ERROR, LOG_USER,
				     &errParm.ge_errorCode,
				     sizeof(errParm.ge_errorCode), 
				     &errParm.ge_message);
	    }

	    conn->status = IIsw_close(conn->stmtHandle, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    {
		REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
				     &errParm.ge_errorCode,
				     sizeof(errParm.ge_errorCode), 
				     &errParm.ge_message);
	    }
	    conn->stmtHandle = NULL;


	    /*
	    ** Close out the transaction.
	    */
	    IIsw_commit(&conn->tranHandle, &errParm);
	    conn->tranHandle = NULL;
	}

	conn->state = PFCTRS_STAT_DISCONNECTING;
    	conn->status = IIsw_disconnect (&conn->connHandle, &errParm);
    	if (conn->status == IIAPI_ST_SUCCESS)
        {
	    conn->state = PFCTRS_STAT_INITED;
        }
	else
	{
            REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_DISCONNECT, LOG_USER,
                &conn->status, sizeof(conn->status),
	    	&errParm.ge_message);
	}
	conn->connHandle = NULL;

    	return;
}


/*
**  Name: pfApiSelectCache - An IMA select loop for Ingres Server cache statistics
**
**  Description:
**      Uses a Select Loop via the Synchronous API Wrapper (SAW). 
**
**  Inputs:
** 	conn			API connection block
** 	c_rows			Cache table
** 	num_rows		(Maximum) Number of rows in the table
**
**  Outputs:
** 	c_rows			Cache table with filled in rows
** 	num_rows		Actual number of rows in the table
**
**  Returns:
**      OK if successful.
**
**  History:
**      15-oct-1998 (wonst02)
**          Created.
** 
*/
STATUS II_EXPORT
pfApiSelectCache (PFAPI_CONN *conn, IMA_DMF_CACHE_STATS	c_rows[], DWORD *num_rows)
{
# define NUM_COLS		22
# define STMT			0
        IIAPI_DATAVALUE         cdata[NUM_COLS];/* columns in result row */
	IMA_DMF_CACHE_STATS	c_row;		/* one row */
	char			stmt[2048];
	IIAPI_GETQINFOPARM	qinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	DWORD			i;
	STATUS			status;
	
	STprintf(stmt, "\
SELECT page_size,\
 force_count, io_wait_count, group_buffer_read_count, group_buffer_write_count,\
 fix_count, unfix_count, read_count, write_count, hit_count, dirty_unfix_count,\
 free_buffer_count, free_buffer_waiters, fixed_buffer_count,\
 modified_buffer_count, free_group_buffer_count, fixed_group_buffer_count,\
 modified_group_buffer_count, flimit, mlimit, wbstart, wbend \
FROM ima_dmf_cache_stats");
	if (strcmp(ServerID, ""))
	{
	    STcat(stmt, " WHERE server = '");
	    STcat(stmt, ServerID);
	    STcat(stmt, "'");
	}
	STcat(stmt, " ORDER BY page_size");

	SW_COLDATA_INIT(cdata[0], c_row.page_size);
	SW_COLDATA_INIT(cdata[1], c_row.force_count);
	SW_COLDATA_INIT(cdata[2], c_row.io_wait_count);
	SW_COLDATA_INIT(cdata[3], c_row.group_buffer_read_count);
	SW_COLDATA_INIT(cdata[4], c_row.group_buffer_write_count);
	SW_COLDATA_INIT(cdata[5], c_row.fix_count);
	SW_COLDATA_INIT(cdata[6], c_row.unfix_count);
	SW_COLDATA_INIT(cdata[7], c_row.read_count);
	SW_COLDATA_INIT(cdata[8], c_row.write_count);
	SW_COLDATA_INIT(cdata[9], c_row.hit_count);
	SW_COLDATA_INIT(cdata[10], c_row.dirty_unfix_count);
	SW_COLDATA_INIT(cdata[11], c_row.free_buffer_count);
	SW_COLDATA_INIT(cdata[12], c_row.free_buffer_waiters);
	SW_COLDATA_INIT(cdata[13], c_row.fixed_buffer_count);
	SW_COLDATA_INIT(cdata[14], c_row.modified_buffer_count);
	SW_COLDATA_INIT(cdata[15], c_row.free_group_buffer_count);
	SW_COLDATA_INIT(cdata[16], c_row.fixed_group_buffer_count);
	SW_COLDATA_INIT(cdata[17], c_row.modified_group_buffer_count);
	SW_COLDATA_INIT(cdata[18], c_row.flimit);
	SW_COLDATA_INIT(cdata[19], c_row.mlimit);
	SW_COLDATA_INIT(cdata[20], c_row.wbstart);
	SW_COLDATA_INIT(cdata[21], c_row.wbend);

	conn->stmtHandle = NULL;
	/*
	** Loop to num_rows instead of num_rows - 1 to get the NO_DATA
	** status... otherwise, IIsw_close would not behave itself.
	*/
	for (i = 0; i <= *num_rows; ++i)
	{
	    conn->status = IIsw_selectLoop(conn->connHandle,
	    	&conn->tranHandle, stmt, 0, NULL, NULL, NUM_COLS, cdata,
	    	&conn->stmtHandle, &qinfoParm, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    	break;
	    if (i < *num_rows)
	    	STRUCT_ASSIGN_MACRO(c_row, c_rows[i]);	/* copy struct */
	}
	if ((conn->status != IIAPI_ST_SUCCESS) && 
	    (conn->status != IIAPI_ST_NO_DATA))
	{
	    /* Error selecting connection information */
	    REPORT_ERROR_DATA_A (PFAPI_SELECT_ERROR, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	}
	else
	{
	    if (i < *num_rows)
	    	*num_rows = i;
	    conn->status = IIAPI_ST_SUCCESS;
	}

	status = IIsw_close(conn->stmtHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
	{
	    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}	
	conn->stmtHandle = NULL;

	if (conn->status == IIAPI_ST_SUCCESS)
	    conn->status = status = IIsw_commit(&conn->tranHandle, &errParm);
	else
	    status = IIsw_rollback(&conn->tranHandle, &errParm);

	if (status != IIAPI_ST_SUCCESS)
	{
	    /* Error doing commit or rollback */
	    REPORT_ERROR_DATA_A (
	    	(conn->status == IIAPI_ST_SUCCESS) ? PFAPI_UNABLE_TO_COMMIT :
	    	                                     PFAPI_UNABLE_TO_ROLLBACK, 
		LOG_USER, &errParm.ge_errorCode, sizeof(errParm.ge_errorCode),
		&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}
	conn->tranHandle = NULL;

	return (conn->status == IIAPI_ST_SUCCESS ? OK : FAIL);

# undef	STMT
# undef	NUM_COLS		       
}

/*
**  Name: pfApiSelectLocks - Select IMA lock statistics
** 
**
**  Description:
** 	This function uses a singleton select statement to calculate:
** 	1.  The count of current threads waiting for locks (resources),
** 	2.  The count of locks being waited on. This will likely be
** 	    different that the count in number 1, because multiple 
** 	    threads could be waiting for the same lock (resource).
**      Uses a Repeated Select via the Synchronous API Wrapper (SAW). 
**
**  Inputs:
** 	conn			API connection block
** 	l_rows			Lock table
** 	num_rows		(Maximum) Number of rows in the table
**
**  Outputs:
** 	l_rows			Lock table with filled in rows
** 	num_rows		Actual number of rows in the table
**
**  Returns:
**      OK, if successful,
** 	FAIL, if not.
**
**  History:
**      12-Jan-1999 (wonst02)
**          Added.
** 
*/
STATUS II_EXPORT
pfApiSelectLocks (PFAPI_CONN *conn, IMA_LOCKS l_rows[], DWORD *num_rows)
{
# define STMT		1
# define NUM_COLS	2

        IIAPI_DATAVALUE         cdata[NUM_COLS];/* column in result row */
	IMA_LOCKS		l_row;		/* row containing the counts */
	IIAPI_GETQINFOPARM	qinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	STATUS			status;
	STATUS			close_status;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("OIPFima_locks")};
	char			stmt[] = ERx("\
SELECT count(resource_id) as threadcount,\
 count(distinct resource_id) as resourcecount\
 FROM  ima_locks\
 WHERE lock_state != 'GRANTED'");
	
	conn->stmtHandle = NULL;
	SW_COLDATA_INIT(cdata[0], l_row.threadcount);
	SW_COLDATA_INIT(cdata[1], l_row.resourcecount);
	conn->status = IIsw_selectSingletonRepeated(conn->connHandle, 
		&conn->tranHandle, stmt, 0, NULL, NULL, 
		&qryId, &conn->qryHandle[STMT], NUM_COLS, cdata, 
		&conn->stmtHandle, &qinfoParm, &errParm);
	if ((conn->status != IIAPI_ST_SUCCESS) && 
	    (conn->status != IIAPI_ST_NO_DATA))
	{
	    /* Error selecting connection information */
	    REPORT_ERROR_DATA_A (PFAPI_REPEATED_SELECT_ERROR, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(l_row, l_rows[0]);	/* copy struct */
	    *num_rows = 1;
	    conn->status = IIAPI_ST_SUCCESS;
	}

	close_status = IIsw_close(conn->stmtHandle, &errParm);
	conn->stmtHandle = NULL;
	if (close_status != IIAPI_ST_SUCCESS)
	{
	    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = close_status;
	}	

	if (conn->status == IIAPI_ST_SUCCESS)
	    conn->status = status = IIsw_commit(&conn->tranHandle, &errParm);
	else
	    status = IIsw_rollback(&conn->tranHandle, &errParm);

	if (status != IIAPI_ST_SUCCESS)
	{
	    /* Error doing commit or rollback */
	    REPORT_ERROR_DATA_A (
	    	(conn->status == IIAPI_ST_SUCCESS) ? PFAPI_UNABLE_TO_COMMIT :
	    	                                     PFAPI_UNABLE_TO_ROLLBACK, 
	    	LOG_USER, &errParm.ge_errorCode, sizeof(errParm.ge_errorCode),
		&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}
	conn->tranHandle = NULL;

	return (conn->status == IIAPI_ST_SUCCESS ? OK : FAIL);

# undef	STMT
# undef	NUM_COLS		       
}

/*
**  Name: pfApiSelectThreads - Use SQL to select CSsampler IMA thread statistics
**
**  Description:
**      Uses a Select via the Synchronous API Wrapper (SAW). 
**
**  Inputs:
** 	conn			API connection block
** 	s_rows			Sampler table
** 	num_rows		(Maximum) Number of rows in the table
**
**  Outputs:
** 	s_rows			Sampler table with filled in rows
** 	num_rows		Actual number of rows in the table
**
**  Returns:
**      OK if successful.
**
**  History:
**      15-oct-1998 (wonst02)
**          Created.
** 	10-jan-1999 (wonst02)
** 	    Add definitions for cssampler event objects.
** 
*/
STATUS II_EXPORT
pfApiSelectThreads (PFAPI_CONN *conn, IMA_CSSAMPLER_THREADS s_rows[], DWORD *num_rows)
{
# define STMT		2
# define NUM_COLS	28
        IIAPI_DATAVALUE         cdata[NUM_COLS];/* columns in result row */
	IMA_CSSAMPLER_THREADS	s_row;		/* one row */
	char			stmt[2048];
	IIAPI_GETQINFOPARM	qinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	DWORD			i;
	STATUS			status;
	
	STprintf(stmt, "\
SELECT numthreadsamples,\
 stateFREE, stateCOMPUTABLE, stateEVENT_WAIT, stateMUTEX,\
 facilityCLF, facilityADF, facilityDMF, facilityOPF,\
 facilityPSF, facilityQEF, facilityQSF, facilityRDF,\
 facilitySCF, facilityULF, facilityDUF, facilityGCF,\
 facilityRQF, facilityTPF, facilityGWF, facilitySXF,\
 eventDISKIO, eventLOGIO, eventMESSAGEIO,\
 eventLOG, eventLOCK, eventLOGEVENT, eventLOCKEVENT \
FROM ima_cssampler_threads");
	if (strcmp(ServerID, ""))
	{
	    STcat(stmt, " WHERE server = '");
	    STcat(stmt, ServerID);
	    STcat(stmt, "'");
	}

	SW_COLDATA_INIT(cdata [0], s_row.numthreadsamples);
	SW_COLDATA_INIT(cdata [1], s_row.stateFREE);
	SW_COLDATA_INIT(cdata [2], s_row.stateCOMPUTABLE);
	SW_COLDATA_INIT(cdata [3], s_row.stateEVENT_WAIT);
	SW_COLDATA_INIT(cdata [4], s_row.stateMUTEX);
	SW_COLDATA_INIT(cdata [5], s_row.facilityCLF);
	SW_COLDATA_INIT(cdata [6], s_row.facilityADF);
	SW_COLDATA_INIT(cdata [7], s_row.facilityDMF);
	SW_COLDATA_INIT(cdata [8], s_row.facilityOPF);
	SW_COLDATA_INIT(cdata [9], s_row.facilityPSF);
	SW_COLDATA_INIT(cdata[10], s_row.facilityQEF);
	SW_COLDATA_INIT(cdata[11], s_row.facilityQSF);
	SW_COLDATA_INIT(cdata[12], s_row.facilityRDF);
	SW_COLDATA_INIT(cdata[13], s_row.facilitySCF);
	SW_COLDATA_INIT(cdata[14], s_row.facilityULF);
	SW_COLDATA_INIT(cdata[15], s_row.facilityDUF);
	SW_COLDATA_INIT(cdata[16], s_row.facilityGCF);
	SW_COLDATA_INIT(cdata[17], s_row.facilityRQF);
	SW_COLDATA_INIT(cdata[18], s_row.facilityTPF);
	SW_COLDATA_INIT(cdata[19], s_row.facilityGWF);
	SW_COLDATA_INIT(cdata[20], s_row.facilitySXF);
	SW_COLDATA_INIT(cdata[21], s_row.eventDISKIO);
	SW_COLDATA_INIT(cdata[22], s_row.eventLOGIO);
	SW_COLDATA_INIT(cdata[23], s_row.eventMESSAGEIO);
	SW_COLDATA_INIT(cdata[24], s_row.eventLOG);
	SW_COLDATA_INIT(cdata[25], s_row.eventLOCK);
	SW_COLDATA_INIT(cdata[26], s_row.eventLOGEVENT);
	SW_COLDATA_INIT(cdata[27], s_row.eventLOCKEVENT);
	conn->stmtHandle = NULL;
	/*
	** Loop to num_rows instead of num_rows - 1 to get the NO_DATA
	** status... otherwise, IIsw_close would not behave itself.
	*/
	for (i = 0; i <= *num_rows; ++i)
	{
	    conn->status = IIsw_selectLoop(conn->connHandle,
	    	&conn->tranHandle, stmt, 0, NULL, NULL, NUM_COLS, cdata,
	    	&conn->stmtHandle, &qinfoParm, &errParm);
	    if (conn->status != IIAPI_ST_SUCCESS)
	    	break;
	    if (i < *num_rows)
		STRUCT_ASSIGN_MACRO(s_row, s_rows[i]);	/* copy struct */
	}
	if ((conn->status != IIAPI_ST_SUCCESS) && 
	    (conn->status != IIAPI_ST_NO_DATA))
	{
	    /* Error selecting connection information */
	    REPORT_ERROR_DATA_A (PFAPI_SELECT_ERROR, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	}
	else
	{
	    if (i < *num_rows)
	    	*num_rows = i;	      
	    conn->status = IIAPI_ST_SUCCESS;
	}

	status = IIsw_close(conn->stmtHandle, &errParm);
	if (status != IIAPI_ST_SUCCESS)
	{
	    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}	
	conn->stmtHandle = NULL;

	if (conn->status == IIAPI_ST_SUCCESS)
	    conn->status = status = IIsw_commit(&conn->tranHandle, &errParm);
	else
	    status = IIsw_rollback(&conn->tranHandle, &errParm);

	if (status != IIAPI_ST_SUCCESS)
	{
	    /* Error doing commit or rollback */
	    REPORT_ERROR_DATA_A (
	    	(conn->status == IIAPI_ST_SUCCESS) ? PFAPI_UNABLE_TO_COMMIT :
	    	                                     PFAPI_UNABLE_TO_ROLLBACK, 
	    	LOG_USER, &errParm.ge_errorCode, sizeof(errParm.ge_errorCode),
		&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}
	conn->tranHandle = NULL;

	return (conn->status == IIAPI_ST_SUCCESS ? OK : FAIL);

# undef	STMT
# undef	NUM_COLS		       
}

/*
**  Name: pfApiSelectSampler - Select CSsampler block IMA object
** 
**
**  Description:
** 	This function uses a singleton select statement to retrieve 
** 	the CSsampler block IMA object.
** 
**      Uses a Repeated Select via the Synchronous API Wrapper (SAW). 
**
**  Inputs:
** 	conn			API connection block
** 	l_rows			Sampler table
** 	num_rows		(Maximum) Number of rows in the table
**
**  Outputs:
** 	l_rows			Sampler table with filled in rows
** 	num_rows		Actual number of rows in the table
**
**  Returns:
**      OK, if successful,
** 	FAIL, if not.
**
**  History:
**      22-Jan-1999 (wonst02)
**          Added.
** 	21-apr-1999 (wonst02)
** 	    Re-instated.
** 
*/
STATUS II_EXPORT
pfApiSelectSampler (PFAPI_CONN *conn, IMA_CSSAMPLER l_rows[], DWORD *num_rows)
{
# define STMT		3
# define NUM_COLS	7

        IIAPI_DATAVALUE         cdata[NUM_COLS];/* column in result row */
	IMA_CSSAMPLER		l_row;		/* row containing the counts */
	IIAPI_GETQINFOPARM	qinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	STATUS			status;
	STATUS			close_status;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, ERx("OIPFima_cssampler_stats")};
	char			stmt[2048];

	STprintf(stmt, "\
SELECT numsamples, numtlssamples, numtlsslots, numtlsprobes,\
 numtlsreads, numtlsdirty, numtlswrites\
 FROM ima_cssampler_stats");
	if (strcmp(ServerID, ""))
	{
	    STcat(stmt, " WHERE server = '");
	    STcat(stmt, ServerID);
	    STcat(stmt, "'");
	}
	
	conn->stmtHandle = NULL;
	SW_COLDATA_INIT(cdata[0], l_row.numsamples_count);
	SW_COLDATA_INIT(cdata[1], l_row.numtlssamples_count);
	SW_COLDATA_INIT(cdata[2], l_row.numtlsslots_count);
	SW_COLDATA_INIT(cdata[3], l_row.numtlsprobes_count);
	SW_COLDATA_INIT(cdata[4], l_row.numtlsreads_count);
	SW_COLDATA_INIT(cdata[5], l_row.numtlsdirty_count);
	SW_COLDATA_INIT(cdata[6], l_row.numtlswrites_count);
	conn->status = IIsw_selectSingletonRepeated(conn->connHandle, 
		&conn->tranHandle, stmt, 0, NULL, NULL, 
		&qryId, &conn->qryHandle[STMT], NUM_COLS, cdata, 
		&conn->stmtHandle, &qinfoParm, &errParm);
	if ((conn->status != IIAPI_ST_SUCCESS) && 
	    (conn->status != IIAPI_ST_NO_DATA))
	{
	    /* Error selecting connection information */
	    REPORT_ERROR_DATA_A (PFAPI_REPEATED_SELECT_ERROR, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(l_row, l_rows[0]);	/* copy struct */
	    *num_rows = 1;
	    conn->status = IIAPI_ST_SUCCESS;
	}

	close_status = IIsw_close(conn->stmtHandle, &errParm);
	conn->stmtHandle = NULL;
	if (close_status != IIAPI_ST_SUCCESS)
	{
	    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = close_status;
	}	

	if (conn->status == IIAPI_ST_SUCCESS)
	    conn->status = status = IIsw_commit(&conn->tranHandle, &errParm);
	else
	    status = IIsw_rollback(&conn->tranHandle, &errParm);

	if (status != IIAPI_ST_SUCCESS)
	{
	    /* Error doing commit or rollback */
	    REPORT_ERROR_DATA_A (
	    	(conn->status == IIAPI_ST_SUCCESS) ? PFAPI_UNABLE_TO_COMMIT :
	    	                                     PFAPI_UNABLE_TO_ROLLBACK, 
	    	LOG_USER, &errParm.ge_errorCode, sizeof(errParm.ge_errorCode),
		&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}
	conn->tranHandle = NULL;

	return (conn->status == IIAPI_ST_SUCCESS ? OK : FAIL);

# undef	STMT
# undef	NUM_COLS		       
}

/*
**  Name: pfApiExecSelect - Execute a given singleton select statement
** 
**
**  Description:
**	This function executes a given singleton select statement and
**	returns its value. It is used in obtaining values for personal
**	counters. Uses a Repeated Select via the Synchronous API
**	Wrapper (SAW). 
**
**  Inputs:
** 	conn			API connection block
**	qryid			The repeated query id
**	stmt			The singleton select statement to execute
**
**  Outputs:
** 	value			The value of the select statement
**
**  Returns:
**      OK, if successful,
** 	FAIL, if not.
**
**  History:
**      22-oct-1999 (somsa01)
**          Created.
** 
*/
STATUS II_EXPORT
pfApiExecSelect (PFAPI_CONN *conn, char *qryid, char *stmt, DWORD *value)
{
# define STMT		1
# define NUM_COLS	1

	IIAPI_DATAVALUE         cdata[NUM_COLS];/* column in result row */
	DWORD			l_row;		/* row containing the count */
	IIAPI_GETQINFOPARM	qinfoParm;
	IIAPI_GETEINFOPARM	errParm;
	STATUS			status;
	STATUS			close_status;
	SW_REPEAT_QUERY_ID	qryId = { 1, 0, qryid};
	
	conn->stmtHandle = NULL;
	SW_COLDATA_INIT(cdata[0], l_row);

	/*
	** For now, we will not use a compile-time repeat query id, but rather
	** a run-time repeat query id. A run-time repeat query id is more
	** maintainable when the query changes.
	*/
	conn->status = IIsw_selectSingletonRepeated(conn->connHandle,
		&conn->tranHandle, stmt, 0, NULL, NULL,
		NULL, &conn->qryHandle[STMT], NUM_COLS, cdata,
		&conn->stmtHandle, &qinfoParm, &errParm);
	if ((conn->status != IIAPI_ST_SUCCESS) && 
	    (conn->status != IIAPI_ST_NO_DATA))
	{
	    /* Error selecting connection information */
	    REPORT_ERROR_DATA_A (PFAPI_REPEATED_SELECT_ERROR, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	}
	else
	{
	    *value = l_row;
	    conn->status = IIAPI_ST_SUCCESS;
	}

	close_status = IIsw_close(conn->stmtHandle, &errParm);
	conn->stmtHandle = NULL;
	if (close_status != IIAPI_ST_SUCCESS)
	{
	    REPORT_ERROR_DATA_A (PFAPI_UNABLE_TO_CLOSE_STMT, LOG_USER,
	    	&errParm.ge_errorCode, sizeof(errParm.ge_errorCode), 
	    	&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = close_status;
	}	

	if (conn->status == IIAPI_ST_SUCCESS)
	    conn->status = status = IIsw_commit(&conn->tranHandle, &errParm);
	else
	    status = IIsw_rollback(&conn->tranHandle, &errParm);

	if (status != IIAPI_ST_SUCCESS)
	{
	    /* Error doing commit or rollback */
	    REPORT_ERROR_DATA_A (
	    	(conn->status == IIAPI_ST_SUCCESS) ? PFAPI_UNABLE_TO_COMMIT :
	    	                                     PFAPI_UNABLE_TO_ROLLBACK, 
	    	LOG_USER, &errParm.ge_errorCode, sizeof(errParm.ge_errorCode),
		&errParm.ge_message);
	    if (conn->status == IIAPI_ST_SUCCESS)
	    	conn->status = status;
	}
	conn->tranHandle = NULL;

	return (conn->status == IIAPI_ST_SUCCESS ? OK : FAIL);

# undef	STMT
# undef	NUM_COLS		       
}

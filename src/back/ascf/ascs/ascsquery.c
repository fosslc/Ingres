/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name: SCSUQERY.C -- query processing routines
** 
** Description:
**    This module contains routines which are called to process
**    a query request 
** History:
**      09-Jun-98 (fanra01)
**          Integrate the ice interface.  Add forward references for
**          ascs_get_language_type and scs_process_ice_query.
**          Modify scs_process_query to switch on language id from gca message.
**      18-Jan-1999 (fanra01)
**          Renamed scs_dispatch_query, scs_process_query to ascs entry points.
**          Renamed scs functions now found in ascs.
**	18-May-1999 (matbe01)
**	    Added casting to int for WTSExecute() to resolve compile error
**	    for rs4_us5.
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Add processing for DB_ICE message.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Correct return type of wts-execute function.
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>
#include    <urs.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <cui.h>

#include		<wsf.h>
#include    <erwsf.h>

/*
** Forward/Extern references 
*/
FUNC_EXTERN DB_STATUS WTSExecute(SCD_SCB *scb);

/*
** Global variables owned by this file
*/


/*
** Name: ascs_process_query -- entry point
**
** Descripition:
**      Processes a request recevied in a GCA message.
**      Uses the language type within the message to determine the processing
**      path.
**
** Input:
**      scb     session control block
**
** Output:
**      none.
**
** Return:
**      E_DB_OK     query process successfully
**      E_DB_ERROR  failed
**
** History:
**      29-May-98 (fanra01)
**          Modified to integrate ice specific changes.
**      13-Jun-2000 (fanra01)
**          Add new DB_ICE message type processing.
*/
DB_STATUS 
ascs_process_query(SCD_SCB *scb)
{
    DB_STATUS   status = E_DB_OK;
    u_i4       langid;

    status = ascs_get_language_type (scb, &langid);
    if (DB_SUCCESS_MACRO(status))
    {
        switch (langid)
        {
            case DB_ODQL:
                /*
                ** Build the query representation from the
                ** GCA_QUERY message.
                */
                status = ascs_bld_query(scb,NULL);
                if (DB_FAILURE_MACRO(status))
                    break;

                /*
                ** Dispatch the query request to
                ** right part of the logical engine
                */
                status = ascs_dispatch_query(scb);
                break;

            case DB_ICE:
                /*
                **  process the request from an ICE client.
                */
                status = WTSExecute (scb);
                break;

            case DB_SQL:
                /*
                **  process the request from an ICE client.
                */
                status = WTSExecute (scb);
                break;

            case DB_NOLANG:
            default:
                sc0e_0_put(E_SC0500_INVALID_LANGCODE, 0);
                status = E_DB_ERROR;
                break;
        }
    }

    return (status);
}


/*
** Name: ascs_dispatch_query -- dispatch GCA_QUERY
** Descripition:
** Input:
** Output:
** Return:
** History:
**    07-sep-1996 (tchdi01)
**        created for Jasmine
*/
DB_STATUS 
ascs_dispatch_query(SCD_SCB *scb)
{
    DB_STATUS   status  = E_DB_OK, mem_stat = E_DB_OK;
    DB_STATUS   ret_val = E_DB_OK;
#if 0
    OMF_CB      *omf_cb;
    OMQS_ENTRY  *omqs; 
    i4          qry_status = GCA_OK_MASK;
    SCC_GCMSG	*message;
    char        word[DB_MAXNAME];
    i4          keyword, token;
    i4          response = TRUE;
    i4          response_type = GCA_RESPONSE;
    bool        free_entry = FALSE;
    i4          code = 0;
    bool        intr_pending = FALSE;

    omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;
    omqs   = omf_cb->omf_omqs;

    switch (omqs->omqs_flags & OMQS_ODQL_MASK)
    {
    case OMQS_ODQL_EXEC_ODQL:
	status = omf_call(OMF_EXEC_ODQL, (PTR)omf_cb->omf_rcb);
	if (DB_FAILURE_MACRO(status))
	    break;
	free_entry = TRUE;
	code       = OMF_EXEC_ODQL;

	break;

    case OMQS_ODQL_SET_VAR:
	status = ascs_get_next_token(&omqs->omqs_stmt_text.pos,
	    omqs->omqs_stmt_text.end, &token, &keyword, word);
	if (DB_FAILURE_MACRO(status))
	    break;

	switch (keyword)
	{
	case ODBSETVALUES_STATEMENT:
	    status = omf_call(OMF_SET_VALUES, (PTR)omf_cb->omf_rcb);
	    code   = OMF_SET_VALUES;
	    break;

	case ODBADDMMDATA_STATEMENT:
	    status = omf_call(OMF_ADD_MMDATA, (PTR)omf_cb->omf_rcb);
	    code   = OMF_ADD_MMDATA;

	    if (omf_cb->omf_omqs->omqs_qresult)
		status = scs_format_results(scb);

	    break;

	case ODBREPMMDATA_STATEMENT:
	    status = omf_call(OMF_REP_MMDATA, (PTR)omf_cb->omf_rcb);
	    code = OMF_REP_MMDATA;

	    if (omf_cb->omf_omqs->omqs_qresult)
		status = scs_format_results(scb);

	    break;

	default:
	    status = omf_call(OMF_SET_VAR, (PTR)omf_cb->omf_rcb);
	    code   = OMF_SET_VAR;
	}

	if (DB_FAILURE_MACRO(status))
	    break;

	free_entry = TRUE;
	break;

    case OMQS_ODQL_GET_VAR:
	status = ascs_get_next_token(&omqs->omqs_stmt_text.pos,
	    omqs->omqs_stmt_text.end, &token, &keyword, word);
	if (DB_FAILURE_MACRO(status))
	    break;

	switch (keyword)
	{
	case ODBGETVALUES_STATEMENT:
	    status = omf_call(OMF_GET_VALUES, (PTR)omf_cb->omf_rcb);
	    code   = OMF_GET_VALUES;
	    break;
	case ODBGETMMDATA_STATEMENT:
	    status = omf_call(OMF_GET_MMDATA, (PTR)omf_cb->omf_rcb);
	    code   = OMF_GET_MMDATA;
	    break;
	case ODBGETOBJSVERS_STATEMENT:
	    status = omf_call(OMF_GET_OBJSVERS, (PTR)omf_cb->omf_rcb);
	    code   = OMF_GET_OBJSVERS;
	    break;
	default:
	    status = omf_call(OMF_GET_VAR, (PTR)omf_cb->omf_rcb);
	    code   = OMF_GET_VAR;
	}

	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** Format and send results to FE
	*/
	if (omf_cb->omf_omqs->omqs_qresult)
	{
	    status = scs_format_results(scb);
	    if (DB_FAILURE_MACRO(status))
		break;
	}

	free_entry = TRUE;
	break;

    case OMQS_ODQL_OPEN_SCAN:
	status = omf_call(OMF_OPEN_SCAN, (PTR)omf_cb->omf_rcb);
	code   = OMF_OPEN_SCAN;
	if (DB_FAILURE_MACRO(status))
	    break;

	/*
	** Format the scanid message 
	** and queue it for sending 
	*/
	status = scs_format_scanid(scb);
	break;

    default:
	{
	    i4  line = __LINE__;
	    sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		     sizeof(__FILE__), __FILE__,
		     sizeof(line), (PTR)&line,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
            scb->scb_sscb.sscb_state = SCS_TERMINATE;

	    return(E_DB_SEVERE);
	}
    }

    /*
    ** Check the status of the call.
    */
    if (DB_FAILURE_MACRO(status))
    {
	ret_val = status;
	free_entry = TRUE;
	qry_status = GCA_FAIL_MASK;
	qry_status |= GCA_CONTINUE_MASK;
	qry_status |= GCA_END_QUERY_MASK;
    }
    else
    {
	ret_val = E_DB_OK;
    }

    if (   (scb->scb_sscb.sscb_state == SCS_PROCESS)
	|| (scb->scb_sscb.sscb_interrupt)
	|| (qry_status == GCA_FAIL_MASK))
    {
	if (scb->scb_sscb.sscb_interrupt == 0)
	{
	    /*
	    ** Prepare and format the response message
	    */
	    status = ascs_format_response(scb, response_type, 
		qry_status, omqs->omqs_rowcount);
	    if (DB_FAILURE_MACRO(status))
	    {
		ret_val |= status;
		goto func_exit;
	    }

	}
	else  /* interrupted */
	{
	    status = ascs_process_interrupt(scb);
	    if (DB_FAILURE_MACRO(status))
	    {
		ret_val |= status;
		goto func_exit;
	    }
	}
    }

    /*
    ** Send the result back to the client
    */
    ret_val |= ascs_gca_send(scb);

 func_exit:
    /*
    ** Free the memory occupied by the query result
    */
    ret_val |= omqs_free_qresult(code, omqs);

    if (free_entry)
	ret_val |= omqs_free_entry(omf_cb, omqs);

#endif /* 0 */

    return (ret_val);
}


/*
** Name: ascs_get_language_type
**
** Descripition:
**      Retrieves a pointer to the GCA header and outputs the language id
**      from the message.
**
** Input:
**      scb         session control block
**
** Output:
**      langid      language id.
**
** Return:
**      E_DB_OK     completed sucessfully
**      E_DB_ERROR  failed
**
** History:
**      29-May-98 (fanra01)
**          Created.
*/
DB_STATUS
ascs_get_language_type ( SCD_SCB *scb, u_i4* langid)
{
    DB_STATUS   status = E_DB_OK;
    GCA_Q_DATA  query_hdr;

    *langid = DB_NOLANG;

    status = ascs_gca_get(scb, (PTR)&query_hdr,
        sizeof(GCA_Q_DATA) - sizeof(GCA_DATA_VALUE));
    if (DB_SUCCESS_MACRO(status))
    {
        *langid = query_hdr.gca_language_id;
    }
    return (status);
}

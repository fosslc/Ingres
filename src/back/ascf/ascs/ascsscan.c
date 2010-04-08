/*
**Copyright (c) 2004 Ingres Corporation
*/

/* 
** Name: scsscan.c -- process scan request 
** 
** Description:
**    Routines in this module process the FETCH request from the 
**    client
** History:
**      18-Jan-1999 (fanra01)
**          Renamed scs_process_fetch and scs_process_close to ascs equivalent.
**          Renamed referenced scs functions to ascs equivalents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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

/*
** Macros and constants
*/
# define FETCH_LIMIT_POS  32


/*
** Forward/Extern references 
*/


/*
** Global variables owned by this file
*/


/*
** Name: ascs_process_fetch -- process fetch request
** Descripition:
** Input:
** Output:
** Return:
** History:
**    11-sep-1996 (tchdi01)
**        created for Jasmine
**    29-jan-1997 (reijo01)
**        fixed bug that caused jasgcc to loop when the odbScanFetch was 
**        being executed.
*/
DB_STATUS
ascs_process_fetch(SCD_SCB *scb)
{
    DB_STATUS   status = E_DB_OK;
#if 0
    GCA_ID      gca_id;
    OMQS_ENTRY  *omqs;
    OMF_CB      *omf_cb;
    i4          qrystatus = GCA_OK_MASK;
    DB_STATUS   mem_stat  = E_DB_OK;
    i4          mode;

    mode   = scb->scb_cscb.cscb_gcp.gca_it_parms.gca_message_type;
    omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;

    /*
    ** Retrieve the scan ID 
    */
    status = ascs_gca_get(scb, &gca_id, sizeof(GCA_ID));
    if (DB_FAILURE_MACRO(status))
	goto func_exit;
    

    /*
    ** Locate the QS entry 
    */
    omqs = omqs_find_entry(omf_cb, 
	sizeof(gca_id.gca_index[1]), (PTR)&gca_id.gca_index[1]);
    if (omqs == NULL)
    {
	/*
	** There is no open scan with this id
	*/
	sc0e_0_uput(E_SC021E_CURSOR_NOT_FOUND, 0);
	goto func_exit;
    }
    omf_cb->omf_omqs = omqs;

    /*
    ** Make sure that the scan was opened 
    */
    if (omqs->omqs_flags & OMQS_CURSOR_OPEN == 0)
    {
	/*
	** This is an internal error. This entry should 
	** not be here if the scan was not open before
	*/
	i4 line = __LINE__;
	sc0e_put(E_SC0501_INTERNAL_ERROR, 0, 2,
		 sizeof(__FILE__), __FILE__,
		 sizeof(line), (PTR)&line,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	status = E_DB_ERROR;
	goto func_exit;
    }

    /*
    ** Decode the number of items to fetch
    */
    omqs->omqs_rowcount = 1;

    if ( mode == GCA1_FETCH )
    {
       GCA1_FT_DATA *ft = (GCA1_FT_DATA *)
                          scb->scb_cscb.cscb_gcp.gca_it_parms.gca_data_area;
       omqs->omqs_rowcount = ft->gca_rowcount;
    }

    else

    if (CMdigit(&gca_id.gca_name[FETCH_LIMIT_POS]))
    {
	gca_id.gca_name[GCA_MAXNAME - 1] = EOS;
	CVan(&gca_id.gca_name[FETCH_LIMIT_POS], &omqs->omqs_rowcount);
    }

    status = omf_call(OMF_FETCH, (PTR)omf_cb->omf_rcb);

func_exit:

    /*
    **  If the call was a success but no rows were retrieved then
    **  tell the client that there are no more rows to fetch and the
    **  query is finished.
    */
    if (DB_SUCCESS_MACRO(status) && omqs->omqs_rowcount == 0)
    {
	qrystatus = GCA_END_QUERY_MASK;
    }
    else if (DB_SUCCESS_MACRO(status))
    {
	/*
	** Put the resulting rows in the GCA_TUPLES 
	** message and queue the message for sending
	*/
	status = ascs_format_tuples(scb);
	qrystatus = GCA_OK_MASK;
    }
    else
    {
	qrystatus = GCA_FAIL_MASK;
	qrystatus |= GCA_CONTINUE_MASK;
	omqs->omqs_rowcount = 0;
    }

    /*
    ** Format and queue the response message 
    */
    status = ascs_format_response(scb, GCA_RESPONSE, 
	qrystatus, omqs->omqs_rowcount);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /*
    ** Send the messages to the front end
    */
    status = ascs_gca_send(scb);

    /*
    ** Free memory allocated to hold the query result
    */
    mem_stat = omqs_free_qresult(OMF_FETCH, omqs);
    if (mem_stat > status)
	status = mem_stat;

#endif /* 0 */

    return (status);
}


/*
** Name: ascs_process_close -- close the scan
** Descripition:
** Input:
** Output:
** Return:
** History:
*/
DB_STATUS 
ascs_process_close(SCD_SCB *scb)
{
    DB_STATUS   status = E_DB_OK;
#if 0
    GCA_ID      gca_id;
    OMQS_ENTRY  *omqs;
    OMF_CB      *omf_cb;
    i4          qrystatus = GCA_OK_MASK;
    DB_STATUS   mem_stat  = E_DB_OK;

    omf_cb = (OMF_CB *)scb->scb_sscb.sscb_omscb;

    /*
    ** Retrieve the scan ID 
    */
    status = ascs_gca_get(scb, &gca_id, sizeof(GCA_ID));
    if (DB_FAILURE_MACRO(status))
	goto func_exit;

    /*
    ** Locate the QS entry 
    */
    omqs = omqs_find_entry(omf_cb, 
	sizeof(gca_id.gca_index[1]), (PTR)&gca_id.gca_index[1]);
    if (omqs == NULL)
    {
	/*
	** There is no open scan with this id
	*/
	sc0e_0_uput(E_SC021E_CURSOR_NOT_FOUND, 0);
	goto func_exit;
    }
    omf_cb->omf_omqs = omqs;

    /*
    ** Close the scan
    */
    status = omf_call(OMF_CLOSE, (PTR)omf_cb->omf_rcb);

func_exit:

    if (DB_FAILURE_MACRO(status))
    {
	qrystatus = GCA_FAIL_MASK;
	qrystatus |= GCA_CONTINUE_MASK;
	omqs->omqs_rowcount = 0;
    }
    else
    {
	qrystatus = GCA_OK_MASK;
    }

    /*
    ** Format and queue the response message 
    */
    status = ascs_format_response(scb, GCA_RESPONSE, 
	qrystatus, omqs->omqs_rowcount);
    if (DB_FAILURE_MACRO(status))
	return (status);

    /*
    ** Send the messages to the front end
    */
    status = ascs_gca_send(scb);

    /*
    ** Free memory allocated to hold the query result
    */
    mem_stat = omqs_free_qresult(OMF_CLOSE, omqs);
    if (mem_stat > status)
	status = mem_stat;

    /*
    ** Free the query storage entry 
    */
    mem_stat = omqs_free_entry(omf_cb, omqs);
    if (mem_stat > status)
	status = mem_stat;

#endif /* 0 */

    return (status);
}



/*
** Name:
** Descripition:
** Input:
** Output:
** Return:
** History:
*/

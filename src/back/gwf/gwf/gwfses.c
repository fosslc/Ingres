/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <gwf.h>
#include    <gwfint.h>
#include    <er.h>
#include    <cui.h>

/**
** Name: GWFSES.C - session-level generic gateway functions.
**
** Description:
**      This file contains session-level functions for the gateway facility.
**
**      This file defines the following external functions:
**    
**	gws_init() - session initiliazation
**	gws_term() - session termination
**	gws_alter() - alter session characteristics
**      gws_check_interrupt()
**      gws_interrupt_handler()
**      gws_register_intr_handler()
**	gws_gt_gw_sess() - locate my GW_SESSION
**
** History:
**	04-apr-90 (linda)
**	    Created -- broken out from gwf.c which is now obsolete.
**	9-apr-90 (bryanp)
**	    Upgraded error handling to new GWF scheme.
**	18-apr-90 (bryanp)
**	    Return E_GW0600_NO_MEM when we get an error allocating memory.
**	08-may-90 (alan)
**	    Stuff session username into GW_SESSION structure.
**	25-may-90 (alan)
**	    Get Real Username (SCI_RUSERNAME), not Effective Username
**      18-may-92 (schang)
**          GW merge
**	    4-feb-91 (linda)
**	        Code cleanup to reflect new structure member names.
**	7-oct-92 (daveb)
**	    Making GWF a first class class facility, managed by SCF.
**	    Take session cb as input from caller (allocated by SCF)
**	    rather than allocating it here.  Add calls to per-exit
**	    GWX_STERM routines so they can cleanup their exit-private
**	    SCBs.
**	17-nov-92 (robf)
**	    Moved gwf_session_id() here from gwftrace.c at schang's
**	    suggestion 
**      12-apr-93 (schang)
**           code in GWF user interrupt handler
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add me.h, tr.h
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Added gws_gt_gw_sess().
**	    Added gws_alter() to change session-specific information.
**	    Changed gws_init() to store case-translation related session
**	    specific information in the session control block.
**	28-jul-1993 (ralph)
**	    Changed GM_error() call to gwf_error() in gws_gt_gw_sess()
**	04-aug-1993 (shailaja)
**	    Fixed argument incompatibilities.
**	23-apr-1996 (canor01)
**	    if gws_interrupt_handler(), if session not completely 
**	    initialized when interrupt received (as in remove session), 
**	    gwf_ses_cb can be null.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Forward or global definitions.
*/



/*{
** Name: gws_check_interrupt - per session interrupt checking
**
** Description:
**      This function is to be called within GWF to check for interrupt.
**
** Inputs:
**	gwf_ses_cb             GWF session control block.
**           gws_interrupted   interrupt flag.
**
** Output:
**	gwf_ses_cb             GWF session control block.
**           gws_interrupted   interrupt flag cleared if set.
**      
** Return:
**      E_DB_OK                if no interrupt.
**      E_DB_ERROR             if interrupt detected.
**
** History:
**      12-apr-93 (schang)
**           code in GWF user interrupt handler
*/
DB_STATUS
gws_check_interrupt(GW_SESSION *gwf_ses_cb)
{
    if (gwf_ses_cb->gws_interrupted)
    {
	gwf_ses_cb->gws_interrupted = FALSE;
	return (E_DB_ERROR);
    }
    return (E_DB_OK);
}


/*{
** Name: gws_interrupt_handler
**
** Description:
**      This function is to be called by SCF to set interrupt flag.
**
** Inputs:
**      eventid                Ignored in this routine
** Output:
**	gwf_ses_cb             GWF session control block.
**           gws_interrupted   interrupt flag.
**      
** Return:
**      None
**
** History:
**      12-apr-93 (schang)
**           code in GWF user interrupt handler
**	23-apr-1996 (canor01)
**	     if session not completely initialized when interrupt
**	     received (as in remove session), gwf_ses_cb can be null.
**	20-Apr-2004 (jenjo02)
**	     Facility handlers return STATUS, not void.
*/
static STATUS gws_interrupt_handler(i4 eventid, GW_SESSION *gwf_ses_cb)
{
    if ( gwf_ses_cb != NULL )
	gwf_ses_cb->gws_interrupted = TRUE;
    return(OK);
}



/*
/*{
** Name: gws_register_intr_handler
**
** Description:
**      This function is to make SCF aware that GWF want to be notified
**      when interrupt occurs.
**
** Inputs:
**      None
** Output:
**	gwf_ses_cb             GWF session control block.
**           gws_interrupted   interrupt flag cleared.
**      
** Return:
**      E_DB_OK
**      E_DB_ERROR             if failed in scf_call
**
** History:
**      12-apr-93 (schang)
**           code in GWF user interrupt handler
**	20-Apr-2004 (jenjo02)
**	     Facility handlers return STATUS, not void.
*/
static DB_STATUS
gws_register_intr_handler(GW_SESSION *gwf_ses_cb)
{
    SCF_CB    scf_cb;
    DB_STATUS scf_status;
    i4   err;

    gwf_ses_cb->gws_interrupted = FALSE;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_ptr_union.scf_afcn = gws_interrupt_handler;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_nbr_union.scf_amask = SCS_AIC_MASK|SCS_ALWAYS_MASK;
 
    scf_status = scf_call(SCS_DECLARE, &scf_cb);

    if (DB_FAILURE_MACRO(scf_status))
    {
        return(E_DB_ERROR);
    }
#if 0
    scf_cb.scf_nbr_union.scf_amask = SCS_PAINE_MASK;
    scf_status = scf_call(SCS_DECLARE, &scf_cb);

    if (DB_FAILURE_MACRO(scf_status))
    {
	return(E_DB_ERROR);
    }
#endif
    return(E_DB_OK);
}
/*{
** Name: gws_init - GW session initialization
**
** Description:
**	This function performs session initialization: session variables  are
**	allocated and/or initialized.  gwf_init() must have preceded this
**	request.  The function should be requested during server session
**	initialization.
**
**	Note that GWF's session control block must be remembered by the caller.
**	It is also the caller's responsibility to guarantee that the GWF is
**	called at session termination to deallocate this structure, and
**	everything that exits may have hung onto it.
**
** Inputs:
**	gw_rcb			GWF request control block
**	    gwr_scf_session_id	SCF session id
**	    gwr_session_id	GW_SESSION allocated by SCF for us.
**
** Output:
**	gw_rcb->
**          gwr_error.err_code	One of the following error numbers.
**				E_GW0600_NO_MEM
**				E_GW0202_GWS_INIT_ERROR
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	23-dec-89 (paul)
**	    Elaborated on Session control block usage.
**	26-mar-90 (linda)
**	    Added error handling; single return point.
**	18-apr-90 (bryanp)
**	    Return E_GW0600_NO_MEM when we get an error allocating memory.
**	08-may-90 (alan)
**	    Stuff session username into GW_SESSION structure.
**	25-may-90 (alan)
**	    Get Real Username (SCI_RUSERNAME), not Effective Username
**	7-oct-92 (daveb)
**	    Call this from SCF rather than DMF; Now the GW_SESSION
**	    is allocated by SCF for us (as for every other facility)
**	    rather than our allocating it and handing it back.  We
**	    now have per-exit SCBs as well.  Clear them out.  Prototyped.
**	09-oct-92 (robf)
**	    Initialize per-session trace vectors
**	19-nov-92 (robf)
**	     Add a check to make sure the session id is passed in, later
**	     code assumes its valid - without this check could get an AV.
**      12-apr-93 (schang)
**           code in GWF user interrupt handler
**	25-oct-93 (vijay)
**	     cast session_id to PTR.
**	14-Apr-2008 (kschendel)
**	    Init the qual-function pointers.
*/
DB_STATUS
gws_init( GW_RCB *gw_rcb )
{
    DB_STATUS	status = E_DB_OK;
    i4		i;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    SIZE_TYPE	memleft = GWF_SESMEM_DEFAULT;
    ULM_RCB	ulm_rcb;
    SCF_CB	scf_cb;
    SCF_SCI	sci_list[2];
    u_i4	l_id;
    DB_ERROR	error;
    
    for (;;)	/* the usual "fake loop" to break out of... */
    {
	/* Make sure session id is not null */
	if (session==NULL)
	{
		gwf_error(E_GW0403_NULL_SESSION_ID, GWF_INTERR, 0);
		gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
		status=E_DB_ERROR;
		break;
	}
	/* copy ulm_rcb */
	STRUCT_ASSIGN_MACRO(Gwf_facility->gwf_ulm_rcb, ulm_rcb);
	ulm_rcb.ulm_memleft = &memleft;
	ulm_rcb.ulm_blocksize = SCU_MPAGESIZE;

	/*
	** The ULM_RCB in Gwf_facility->gwf_ulm_rcb was initialized by
	** default to ULM_SHARED streams. Here we override that,
	** defining private streams for each session.
	*/
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM;

	/* clear the per-exit SCB slots */
	for (i=0; i < GW_GW_COUNT; ++i)
	    session->gws_exit_scb[i] = NULL;

	/* allocate session stream */
	if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0312_ULM_OPENSTREAM_ERROR, GWF_INTERR, 3,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft,
		      sizeof(ulm_rcb.ulm_sizepool),
		      &ulm_rcb.ulm_sizepool,
		      sizeof(ulm_rcb.ulm_blocksize),
		      &ulm_rcb.ulm_blocksize);
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		gw_rcb->gwr_error.err_code = E_GW0600_NO_MEM;
	    else
		gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
	    break;
	}

	session = (GW_SESSION *)gw_rcb->gwr_session_id;
	STRUCT_ASSIGN_MACRO(ulm_rcb, session->gws_ulm_rcb);
	session->gws_memleft = memleft;
	session->gws_ulm_rcb.ulm_memleft = &session->gws_memleft;
	session->gws_rsb_list = NULL;
	session->gws_txn_begun = FALSE;
	session->gws_scf_session_id = (PTR) gw_rcb->gwr_scf_session_id;
	session->gws_qfun_adfcb = NULL;
	session->gws_qfun_errptr = NULL;

	/* Get the ADF_CB and username for this session */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_facility = DB_ADF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
	scf_cb.scf_len_union.scf_ilength = 2;
	sci_list[0].sci_length = sizeof(PTR);
	sci_list[0].sci_code = SCI_SCB;
	sci_list[0].sci_aresult = (PTR)(&session->gws_adf_cb);
	sci_list[0].sci_rlength = NULL;
	sci_list[1].sci_length = sizeof(session->gws_username);
	sci_list[1].sci_code = SCI_RUSERNAME;
	sci_list[1].sci_aresult = (char *)(session->gws_username);
	sci_list[1].sci_rlength = NULL;
	if ((status = scf_call(SCU_INFORMATION, &scf_cb)) != E_DB_OK)
	{
	    gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0302_SCU_INFORMATION_ERROR, GWF_INTERR, 0);
	    gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
	    break;
	}

	/*@FIX_ME@ What if the case of the username changes??? */

	session->gws_username[DB_OWN_MAXNAME] = '\0';
	/*
	**	Blank out trace vectors
	*/
	MEfill(sizeof(session->gws_trace),
			(unsigned char)0,(PTR)session->gws_trace);

        /*
        **  schang: code in GWF user interrupt handler
        */
	status = gws_register_intr_handler(session);

	/*
	** Initialize session specific values wrt case translation semantics
	*/
	session->gws_cat_owner = gw_rcb->gwr_cat_owner;
	session->gws_dbxlate = gw_rcb->gwr_dbxlate;

	/*
	** Translate the gateway core catalog names based upon
	** the revised session case translation semantics.
	*/
	for (i=0; i < GW_GW_COUNT; ++i)
	{
	    if (!Gwf_facility->gwf_gw_info[i].gwf_gw_exist)
		continue;

	    /* Translate iigwxx_relation */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xrel_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xrel_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xrel_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xrel_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Relation Catalog"))-1,
			  ERx("Gateway Relation Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xrel_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
		break;
	    }

	    /* Translate iigwxx_attribute */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xatt_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xatt_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xatt_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xatt_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Attribute Catalog"))-1,
			  ERx("Gateway Attribute Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xatt_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
		break;
	    }

	    /* Translate iigwxx_index */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xidx_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xidx_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xidx_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xidx_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Index Catalog"))-1,
			  ERx("Gateway Index Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xidx_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0202_GWS_INIT_ERROR;
		break;
	    }

	}

	/*
	** End of block
	*/
	break;
    }

    return(status);
}

/*{
** Name: gws_term - GW session termination
**
** Description:
**	This function performs session termination:  session variables are
**	deallocated.  gwf_init() must have preceded this request.
**
** Inputs:
**	gw_rcb->
**	    gwr_session_id	gateway session id
**
** Output:
**	gw_rcb->
**	    gwr_error.err_code	Error code for this function, one of:
**				E_GW0203_GWS_TERM_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
**	23-dec-89 (paul)
**	    Added deallocation for memory streams associated with open record
**	    streams. Just in case these are left over from query processing.
**	26-mar-90 (linda)
**	    Added error handling and a single return point.
**	9-apr-90 (bryanp)
**	    Upgraded error handling to new GWF scheme.
**	7-oct-92 (daveb)
**	    call exit session shutdown too, with the session id.
*/
DB_STATUS
gws_term( GW_RCB *gw_rcb )
{
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    GW_RSB	*rsb;
    GWX_RCB	gwx_rcb;
    DB_STATUS	status = E_DB_OK;
    ULM_RCB	ulm_rcb;
    DB_ERROR	err;
    i4		i;
    
    for (;;)	/* The usual... */
    {
	/* terminate each initiated gateway by calling its termination entry. */
	for (i=0; i < GW_GW_COUNT; ++i)
	{
	    if (Gwf_facility->gwf_gw_info[i].gwf_gw_exist &&
		session->gws_exit_scb[i] != NULL )
	    {
		gwx_rcb.xrcb_gw_session = session;
		status =
		    (*Gwf_facility->gwf_gw_info[i].gwf_gw_exits[GWX_VSTERM])
			(&gwx_rcb);
		if (status != E_DB_OK)
		{
		    /* On error report, log and continue shutting down gateways */
		    gwf_error(gwx_rcb.xrcb_error.err_code, GWF_INTERR, 0);
		    gwf_error(E_GW0201_GWF_TERM_ERROR, GWF_INTERR, 1,
			      sizeof(i), &i);
		}
	    }
	}

	/*
	** All memory associated with the session must be deallocated.  This
	** includes any streams associated with existing RSB's (ie. open tables)
	** and the session control block itself.
	**
	** First deallocate any memory associated with open tables.  The record
	** control block and any information allocated by it is removed with
	** this closestream.
	*/

	while (session->gws_rsb_list != NULL)
	{
	    rsb = session->gws_rsb_list;
	    session->gws_rsb_list = rsb->gwrsb_next;

	    if ((status = gwu_delrsb(session, rsb, &err)) != E_DB_OK)
	    {
		gwf_error(err.err_code, GWF_INTERR, 0);
		gw_rcb->gwr_error.err_code = E_GW0203_GWS_TERM_ERROR;
		break;
	    }
	}
    
	/* Close the stream containing the session control block */
	STRUCT_ASSIGN_MACRO(session->gws_ulm_rcb, ulm_rcb);
	if ((status = ulm_closestream(&ulm_rcb)) != E_DB_OK)
	{
	    gwf_error(ulm_rcb.ulm_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0313_ULM_CLOSESTREAM_ERROR, GWF_INTERR, 1,
		      sizeof(*ulm_rcb.ulm_memleft),
		      ulm_rcb.ulm_memleft);
	    gw_rcb->gwr_error.err_code = E_GW0203_GWS_TERM_ERROR;
	    break;
	}
	break;
    }

    return(status);
}
/*{
** Name: gwf_session_id	- returns the GWF session id.
**
** Description:
**      Returns the GWF session id for this session.
**
** Inputs:
**	Pointer to session id to return.
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** History:
**	24-sep-92 (robf)
**		Created
**	30-oct-92 (robf)
**		Get session id directly from SCF rather than DMF
*/
DB_STATUS
gwf_session_id(GW_SESSION **session)
{
	SCF_CB	    scf_cb;
	SCF_SCI	    sci_list;
	GW_SESSION  *gwscb;
	DB_STATUS   status;

	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_GWF_ID;	
	scf_cb.scf_session = DB_NOSESSION;	/* Current Session */
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*)&sci_list;
        scf_cb.scf_len_union.scf_ilength = 1;
	sci_list.sci_length = sizeof(SCF_CB);
	sci_list.sci_code = SCI_SCB;
	sci_list.sci_aresult = (PTR) &gwscb;
	sci_list.sci_rlength = 0;
	status = scf_call(SCU_INFORMATION, &scf_cb);

	if (status != E_DB_OK || gwscb==NULL)
	{
		TRdisplay("GWF_SESSION_ID: SCF error trying to get GWF session id\n");
		*session=(GW_SESSION*)0;
		return E_DB_ERROR;
	}
	else 
	{
		*session = gwscb;
		return E_DB_OK;
	}
}

/*{
** Name: gws_alter - Alter GW session
**
** Description:
**	This function alters session information: session variables  are
**	altered.  gws_init() must have preceded this request. 
**
**	This function was added to provide a way to change a session's
**	case translation semantics after they are determined by the server.
**	This routine should alter all structures that depend upon case
**	translation semantics if those semantics are altered.
**
**	Note that GWF's session control block must be remembered by the caller.
**
** Inputs:
**	gw_rcb			GWF request control block
**
**	    .gwr_dbxlate	    Pointer to session's case translation
**				    semantics masks.  If NULL, semantics
**				    are not changed; otherwise, the mask
**				    is changed, as any internal variables
**				    that rely upon the case translation mask.
**
**	    .gwr_cat_owner	    Pointer to session's catalog owner name.
**				    This will be '$inrges' if regular id's 
**				    are lower cased, or '$INGRES' if regular
**				    id's are upper cased.
**
** Output:
**	gw_rcb->
**          gwr_error.err_code	One of the following error numbers.
**				E_GW0221_GWS_ALTER_ERROR  
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      21-Apr-1989 (alexh)
**          Created.
*/
DB_STATUS
gws_alter( GW_RCB *gw_rcb )
{
    DB_STATUS	status = E_DB_OK;
    i4		i;
    GW_SESSION	*session = (GW_SESSION *)gw_rcb->gwr_session_id;
    DB_ERROR	error;
    u_i4	l_id;
    
    for (;;)	/* the usual "fake loop" to break out of... */
    {
	/*
	** Alter the catalog owner name if provided
	*/
	if (gw_rcb->gwr_cat_owner != NULL)
	    session->gws_cat_owner = gw_rcb->gwr_cat_owner;

	if (session->gws_dbxlate == NULL)
	    break;

	/*
	** Alter the session's case translation semantics mask pointer
	** if provided.
	*/
	session->gws_dbxlate = gw_rcb->gwr_dbxlate;

	/*
	** Translate the gateway core catalog names based upon
	** the revised session case translation semantics.
	*/

	for (i=0; i < GW_GW_COUNT; ++i)
	{
	    if (!Gwf_facility->gwf_gw_info[i].gwf_gw_exist)
		continue;

	    /* Translate iigwxx_relation */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xrel_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xrel_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xrel_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xrel_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Relation Catalog"))-1,
			  ERx("Gateway Relation Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xrel_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0221_GWS_ALTER_ERROR;
		break;
	    }

	    /* Translate iigwxx_attribute */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xatt_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xatt_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xatt_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xatt_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Attribute Catalog"))-1,
			  ERx("Gateway Attribute Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xatt_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0221_GWS_ALTER_ERROR;
		break;
	    }

	    /* Translate iigwxx_index */

	    (VOID)MEcopy(
		(PTR)&Gwf_facility->gwf_gw_info[i].gwf_xidx_tab_name,
		sizeof(Gwf_facility->gwf_gw_info[i].gwf_xidx_tab_name),
		(PTR)&session->gws_gw_info[i].gws_xidx_tab_name);

	    l_id = 0;

	    status = cui_idxlate(
			(u_char *)&session->gws_gw_info[i].gws_xidx_tab_name,
			&l_id,
			(u_char *)NULL,
			(u_i4 *)NULL,
			*(session->gws_dbxlate) | CUI_ID_NORM | CUI_ID_STRIP,
			(u_i4 *)NULL,
			&error);
	    if (status != E_DB_OK)
	    {
		gwf_error(error.err_code, GWF_INTERR, 2,
			  sizeof(ERx("Gateway Index Catalog"))-1,
			  ERx("Gateway Index Catalog"),
			  l_id, &session->gws_gw_info[i].gws_xidx_tab_name);
		gw_rcb->gwr_error.err_code = E_GW0221_GWS_ALTER_ERROR;
		break;
	    }

	}

	/*
	** End of block
	*/
	break;
    }

    return(status);
}

/*{
** Name:	gws_gt_gw_sess	- locate my GW_SESSION
**
** Description:
**	Gets the GW_SESSION from SCF.  Should verify this is a GW session
**	in some way.  FIXME.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	none.
**
** Outputs:
**	None.
**
** Returns:
**	The GW_SESSION,  or NULL if not there.
**
** History:
**	22-sep-92 (daveb)
**	    created.
**	11-Nov-1992 (daveb)
**	    Must pas DB_NOSESSION as input.
**	04-jun-1993 (ralph)
**	    DELIM_IDENT:
**	    Plagarized this from GM_gt_gw_sess().
**	28-jul-1993 (ralph)
**	    Changed GM_error() call to gwf_error() in gws_gt_gw_sess()
*/
GW_SESSION *
gws_gt_gw_sess(void)
{
    GW_SESSION	*gw_sess;
    SCF_CB	scf_cb;
    SCF_SCI	scf_sci;
    DB_STATUS	db_stat;

    scf_sci.sci_code = SCI_SCB;
    scf_sci.sci_length = sizeof( *gw_sess );
    scf_sci.sci_aresult = (PTR)&gw_sess;
    scf_sci.sci_rlength = 0;

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof( scf_cb );
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_len_union.scf_ilength = 1;
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)&scf_sci;

    db_stat = scf_call( SCU_INFORMATION, &scf_cb );
    if( db_stat != E_DB_OK )
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gw_sess = NULL;
    }

    return( gw_sess );
}


/*
**Copyright (c) 2004 Ingres Corporation
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
#include    <ci.h>

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

#include    <urs.h>
#include    <ascd.h>
#include    <ascs.h>

#include    <cui.h>

/**
**
**  Name: SCSINIT.C - Initiation and Termination services for sessions
**
**  Description:
**      This file contains the functions necessary to initiate and 
**      terminate a session.  These include the initialization of the
**      session control block (sequencer part), the determination of 
**      necessary information from the "dbdb session," the logging,
**      if appropriate, of a session starting up or shutting down, 
**      the acquisition and disposition of the requisite resources, 
**      etc.
**
**          ascs_initiate - Initialize a session
**          ascs_terminate - Terminate a session
**          scs_dbdb_info - fetch dbdb information about the session
**	    ascs_disassoc() - Lower level dissconnect
**
**
**  History:
**	12-May-1998 (shero03)
**	    Support Factotum thread change 9-Mar-1998 by jenjo02
**	03-Aug-1998 (farna01)
**	    Add casting to remove compiler warnings on unix.
**      18-Jan-1999 (fanra01)
**          Removed rdf include.
**          Renamed scs_initate, scs_disassoc, conv_to_struct_xa_id
**          as ascs functions.
**          Renamed scs_ctlc, scs_log_event, scs_check_password and 
**          scs_atfactot for deletion.
**      12-Feb-99 (fanra01)
**          Replace previously deleted scs_atfactot to resolve call in ascf
**          facility.  Replace previously deleted scs_ctlc to resolve call.
**          Renamed scd_note and scs_disassoc to ascd and ascs eqivalents.
**      15-Feb-1999 (fanra01)
**          Moved setup of collation to ensure validity of adf_cb pointer.
**      23-Feb-99 (fanra01)
**          Hand integration of change 439795 by nanpr01.
**          Changed the sc0m_deallocate to pass pointer to a pointer.
**	10-mar-1999 (mcgem01)
**	    Add a server-side license check for OpenROAD.
**	22-nov-1999 (somsa01)
**	    Manual integration of change 441277;
**	    Bug 90634. Whilst terminating a session which failed to complete
**	    its initialisation, a E_SC012B_SESSION_TERMINATE error may be
**	    generated because the status varible is undefined.
**      12-Jun-2000 (fanra01)
**          Bug 101345
**          Add DB_ICE as the session language id.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-Apr-2004 (bonro01)
**	    Allow Ingres to be installed as a user other than 'ingres'
**      13-Jul-2004 (lazro01)
**          Bug 112647
**          Hand integration of change 467781 by inkdo01.
**          Added support for BIGINT.
**	22-Feb-2005 (schka24)
**	    sscb_ics changes for i4-tid compatibility need to be reflected
**	    here, not sure why I didn't get compile errors before.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Forward and/or External function references.
*/

/* Static routine called within this module. This routine is supposed */
/* to be consistent with the routine IICXconv_to_struct_xa_xid in the */
/* IICX sub-component of LIBQXA. They will become one if/when IICX is */
/* moved to 'common'                                                  */
/* 22-sep-1993 (iyer)                                                 */
/*  Changed the prototype to accept pointer to DB_XA_EXTD_DIS_TRAN_ID */

DB_STATUS aconv_to_struct_xa_xid( char               *xid_text,
                                 i4             xid_str_max_len,
                                 DB_XA_EXTD_DIS_TRAN_ID  *xa_extd_xid_p );

static DB_STATUS scs_usr_expired ( 
    SCD_SCB	*scb);

static DB_STATUS scs_check_app_code(
    SCD_SCB	*scb);

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* Central structure for SCF */


/*{
** Name: ascs_initiate	- Start up a session
**
** Description:
**      This routine performs the initialization necessary to get a session 
**      going within the Ingres DBMS.   At the point this routine is called, 
**      the dispatcher will  have already  established a context in which to 
**      run the session;  that is, the scb will have already been allocated, 
**      and the communication links required will have been set up (that is, 
**      the status channel back to the frontend will be open).
** 
**      This routine, then, must  
**	    - log the begin time for the session 
**          - move the interesting information from the crb into the sscb 
**	    - find the interesting information from the dbdb session
**          - create this session for the other facilities 
**          - return the status to the caller so that the session may be 
**		be allowed to proceed.
**
** Inputs:
**      scb				the session control block
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-Dec-1997 (shero03)
**	    Accept SQL language
**      15-Feb-1999 (fanra01)
**          Moved setup of collation to ensure validity of adf_cb pointer.
**      12-Jun-2000 (fanra01)
**          Add DB_ICE as the session language id and an allowable language.
**	14-Sep-2006 (gupsh01)
**	    Added support for date_type_alias to configure whether date 
**	    datatype is synonymous to ingresdate or ansidate types.
**	03-Oct-2006 (gupsh01)
**	    Moved the storage of date_type_alias to Sc_main_cb.
**	10-Jul-2007 (kibro01) b118702
**	    Ensure that date_type_alias defaults to ingresdate
**	23-Jul-2007 (smeke01) b118798.
**	    Added adf_proto_level flag value AD_I8_PROTO. adf_proto_level 
**	    has AD_I8_PROTO set iff the client session can cope with i8s, 
**	    which is the case if the client session is at GCA_PROTOCOL_LEVEL_65 
**	    or greater. 
**	03-Aug-2007 (gupsh01)
**	    Support date_alias based on protocol level.
**	8-Jul-2008 (kibro01) b120584
**	    Added AD_ANSIDATE_PROTO flag if data type for "date" has been
**	    amended to "ingresdate" or "ansidate" at this protocol level.
**      26-Nov-2009 (hanal04) Bug 122938
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
*/
i4
ascs_initiate(SCD_SCB *scb )
{
    i4			i;
    i4			dash_u = 0;
    i4			tbl_structure = 0;
    i4			index_structure = 0;
    i4			flag_value;
    GCA_USER_DATA	*flag;
    DB_STATUS		status = E_DB_OK;
    ADF_CB		*adf_cb;
    GCA_SESSION_PARMS	*crb;
    DB_ERROR		error;
    char		node[1] = {'\0'};
    i4		p_index;
    i4		l_value;
    bool                timezone_recd = FALSE;
    bool                year_cutoff_recd = FALSE;
    char                tempstr[DB_MAXNAME+1];
    bool                cdb_deferred = FALSE;
    char		sem_name[ CS_SEM_NAME_LEN ];
    bool		want_updsyscat=FALSE;
    char		*env = NULL;
    char                *env_root = NULL;
    i4			env_len;
    char		pmvalue[7];
    bool		date_alias_recd = FALSE;

    error.err_code = 0;
    error.err_data = 0;

    Sc_main_cb = (SC_MAIN_CB *) scb->cs_scb.cs_svcb;

    ule_initiate(node,
    		    STlength(node),
    		    Sc_main_cb->sc_sname,
    		    sizeof(Sc_main_cb->sc_sname));
    
	ult_init_macro(&scb->scb_sscb.sscb_trace, SCS_TBIT_COUNT, SCS_TVALUE_COUNT, SCS_TVALUE_COUNT);

    /* No alarms initially */
    scb->scb_sscb.sscb_alcb=NULL;
    
	/*
    ** Session connect time, as SYSTIME
    */
    TMnow(&scb->scb_sscb.sscb_connect_time);
	
    /* Initialize alert structures */
    scb->scb_sscb.sscb_alerts = NULL;
    scb->scb_sscb.sscb_aflags = 0;
    scb->scb_sscb.sscb_astate = SCS_ASDEFAULT;
    CSw_semaphore(&scb->scb_sscb.sscb_asemaphore, CS_SEM_SINGLE,
		    STprintf( sem_name, "SSCB %p ASEM", scb ));

    /*
    ** Initialize sscb_raat_message to NULL.  Memory will be allocated by
    ** the RAAT API if it is used.  This memory will be freed in
    ** scs_terminate.
    */
    scb->scb_sscb.sscb_raat_message = NULL;
    scb->scb_sscb.sscb_big_buffer   = NULL;
    scb->scb_sscb.sscb_raat_buffer_used = 0;
    scb->scb_sscb.sscb_raat_workarea = NULL;

    /*
    ** Initialize Ingres startup and session parameters.
    **
	** If this is a Factotum thread, the creating (parent)
    ** thread's sscb_ics has been copied to this 
    ** thread's sscb_ics and must be preserved.
    */

    /* Update ics_sessid with this thread's id */
    (VOID)CVptrax((PTR)scb->cs_scb.cs_self, (PTR)tempstr);

    (VOID)MEcopy((PTR)tempstr,
		 (u_i2)sizeof(scb->scb_sscb.sscb_ics.ics_sessid),
		 (PTR)scb->scb_sscb.sscb_ics.ics_sessid);

    if (scb->scb_sscb.sscb_stype != SCS_SFACTOTUM)
    {
    scb->scb_sscb.sscb_ics.ics_db_access_mode = DMC_A_WRITE;
    scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_SHARE;
    scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_NOWAIT;

    /*
    ** Initialize all dbprivs in scb, this is reset later in scs_dbdb_info
    */
    scb->scb_sscb.sscb_ics.ics_ctl_dbpriv  = DBPR_ALL;
    scb->scb_sscb.sscb_ics.ics_fl_dbpriv   = (DBPR_BINARY|DBPR_PRIORITY_LIMIT);
    scb->scb_sscb.sscb_ics.ics_qrow_limit  = -1;
    scb->scb_sscb.sscb_ics.ics_qdio_limit  = -1;
    scb->scb_sscb.sscb_ics.ics_qcpu_limit  = -1;
    scb->scb_sscb.sscb_ics.ics_qpage_limit = -1;
    scb->scb_sscb.sscb_ics.ics_qcost_limit = -1;
    scb->scb_sscb.sscb_ics.ics_idle_limit    = -1;
    scb->scb_sscb.sscb_ics.ics_connect_limit = -1;
    scb->scb_sscb.sscb_ics.ics_priority_limit=  -1;
    scb->scb_sscb.sscb_ics.ics_cur_idle_limit    = -1;
    scb->scb_sscb.sscb_ics.ics_cur_connect_limit = -1;

    /*
    **	This is needed for MONITOR sessions where there may be
    **  no specific database named. (e.g. for auditing)
    */
    MEmove(sizeof("<no_database>")-1, "<no_database>", ' ',
	sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
	(PTR)&scb->scb_sscb.sscb_ics.ics_dbname);

    scb->scb_sscb.sscb_ics.ics_language = 1;
	scb->scb_sscb.sscb_ics.ics_qlang = DB_ICE;
    scb->scb_sscb.sscb_ics.ics_decimal.db_decimal = '.';
    scb->scb_sscb.sscb_ics.ics_decimal.db_decspec = TRUE;
    scb->scb_sscb.sscb_ics.ics_date_format = DB_US_DFMT;
    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym[0] = '$',
    	scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym[1] = ' ';
    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_lort = DB_LEAD_MONY;
    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_prec = 2;
    scb->scb_sscb.sscb_ics.ics_idxstruct = DB_HASH_STORE;
    scb->scb_sscb.sscb_ics.ics_parser_compat = 0;
    scb->scb_sscb.sscb_ics.ics_mathex = (i4) ADX_ERR_MATHEX;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_c0width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_i1width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_i2width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_i4width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_i8width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f4width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f8width = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f4prec = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f8prec = -1;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f4style = 0;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_f8style = 0;
    scb->scb_sscb.sscb_ics.ics_outarg.ad_t0width = -1;
    scb->scb_sscb.sscb_ics.ics_cur_idle_limit    = -1;
    scb->scb_sscb.sscb_ics.ics_cur_connect_limit = -1;

	(VOID)MEcopy((PTR)DB_ABSENT_NAME,
		     (u_i2)sizeof(scb->scb_sscb.sscb_ics.ics_xegrpid),
		     (PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_egrpid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_xeaplid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_eaplid);

	(VOID)MEfill((u_i2)sizeof(scb->scb_sscb.sscb_ics.ics_eapass),
		     (u_char)' ', 
		     (PTR)&scb->scb_sscb.sscb_ics.ics_eapass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_egpass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_iupass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_rupass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_srupass);

    (VOID)MEmove(sizeof(DB_INGRES_NAME)-1, (PTR)DB_INGRES_NAME, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_cat_owner),
		 (PTR)&scb->scb_sscb.sscb_ics.ics_cat_owner);
	
	scb->scb_sscb.sscb_ics.ics_dbxlate = 0x00;

	/*
	** Point the effective userid to the SQL session authid
	*/
	scb->scb_sscb.sscb_ics.ics_eusername =
	    &scb->scb_sscb.sscb_ics.ics_susername;
	

	scb->scb_sscb.sscb_ics.ics_strtrunc = (i4) ADF_IGN_STRTRUNC;
    }

    /*
    ** If there is a Front End process associated with this session, then
    ** process the session startup parameters.
    */
    if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
    {
	scb->scb_sscb.sscb_facility |= (1 << DB_GCF_ID);
	if (scb->scb_cscb.cscb_gci.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(scb->scb_cscb.cscb_gci.gca_status,
		    &scb->scb_cscb.cscb_gci.gca_os_status );
	    sc0e_0_put(E_SC022F_BAD_GCA_READ, 0 );
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0 );
	    error.err_code = E_SC022F_BAD_GCA_READ;
	    return(E_SC022F_BAD_GCA_READ);
	}    
	error.err_code = E_DB_OK;
	if (scb->scb_cscb.cscb_gci.gca_message_type == GCA_RELEASE)
	{
	    /* client send a GCA_RELEASE as the first message - honor it */
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	    error.err_code = E_SC0123_SESSION_INITIATE;
	    return(E_SC0123_SESSION_INITIATE);
	}
	if (scb->scb_cscb.cscb_gci.gca_message_type != GCA_MD_ASSOC)
	{
	    sc0e_put(E_SC022D_1ST_BLOCK_NOT_MD_ASSOC, 0, 1,
		    sizeof(scb->scb_cscb.cscb_gci.gca_message_type),
		    (PTR)&scb->scb_cscb.cscb_gci.gca_message_type,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
	    sc0e_0_put(E_SC022B_PROTOCOL_VIOLATION, 0);
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	    error.err_code = E_SC022B_PROTOCOL_VIOLATION;
	    return(E_SC022B_PROTOCOL_VIOLATION);
	}
	else if ((!scb->scb_cscb.cscb_gci.gca_end_of_data)
		    || (scb->scb_sscb.sscb_tptr))
	{
	    /*								    */
	    /* In these cases, we allocate memory to store the entire block as  */
	    /* one entity.  Only when we have the entire entity do we bother to */
	    /* process any of it.						    */
	
	    if (scb->scb_sscb.sscb_tnleft <
		    scb->scb_cscb.cscb_gci.gca_d_length)
	    {
		PTR	place;
		
		status = sc0m_allocate(0,
					(i4)(sizeof(SC0M_OBJECT) +
					    (scb->scb_sscb.sscb_tsize +
		      (2 * scb->scb_cscb.cscb_gci.gca_d_length))),
					DB_SCF_ID,
					(PTR) SCS_MEM,
					CV_C_CONST_MACRO('s', 'c', 's', 'i'),
					&place);
		if (status)
		    return(status);
		if (scb->scb_sscb.sscb_tptr)
		{
		    MECOPY_VAR_MACRO(scb->scb_sscb.sscb_troot,
			scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft,
			((char *)place + sizeof(SC0M_OBJECT)));
		}
		scb->scb_sscb.sscb_tnleft +=
			(2 * scb->scb_cscb.cscb_gci.gca_d_length);
		scb->scb_sscb.sscb_tsize += 
			(2 * scb->scb_cscb.cscb_gci.gca_d_length);
		scb->scb_sscb.sscb_troot =
		    (PTR)((char *)place + sizeof(SC0M_OBJECT));
		if (scb->scb_sscb.sscb_tptr)
		{
		    sc0m_deallocate(0, &scb->scb_sscb.sscb_tptr);
		}
		scb->scb_sscb.sscb_tptr = place;
		MECOPY_VAR_MACRO(scb->scb_cscb.cscb_gci.gca_data_area,
			scb->scb_cscb.cscb_gci.gca_d_length,
			((char *) scb->scb_sscb.sscb_troot +
				scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft));
		scb->scb_sscb.sscb_tnleft -=
			    scb->scb_cscb.cscb_gci.gca_d_length;
	    }
	    else if (scb->scb_sscb.sscb_tptr)
	    {
		MECOPY_VAR_MACRO(scb->scb_cscb.cscb_gci.gca_data_area,
			scb->scb_cscb.cscb_gci.gca_d_length,
			((char *) scb->scb_sscb.sscb_troot +
				scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft));
		scb->scb_sscb.sscb_tnleft -=
			    scb->scb_cscb.cscb_gci.gca_d_length;
	    }
	    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	    scb->scb_sscb.sscb_state = SCS_INITIATE;
	    if (!scb->scb_cscb.cscb_gci.gca_end_of_data)
		return(E_DB_OK);
	}
	
	scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	if (scb->scb_sscb.sscb_tptr)
	    crb = (GCA_SESSION_PARMS *) scb->scb_sscb.sscb_troot;
	else
	    crb = (GCA_SESSION_PARMS *)
			scb->scb_cscb.cscb_gci.gca_data_area;
	
	/*
	** Copy over the ics portion of the scb from the crb.
	** This is the ingres control stuff such as database name,
	** username, query language, etc.
	** Go thru the command line and update things as necessary.
	*/

	flag = &crb->gca_user_data[0];
	for ( i = 0;
		i < crb->gca_l_user_data && !status;
		i++
	    )
	{
	    MECOPY_CONST_MACRO((PTR)&flag->gca_p_index, sizeof (p_index), 
				(PTR)&p_index);
	    MECOPY_CONST_MACRO((PTR)&flag->gca_p_value.gca_l_value, 
			sizeof (l_value), (PTR)&l_value);
	    switch (p_index)
	    {
		case    GCA_VERSION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (flag_value != GCA_V_60)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
		    }
		    break;
		case    GCA_RUPASS:
            MEmove(l_value,
                   flag->gca_p_value.gca_value,
                   ' ',
                   sizeof(scb->scb_sscb.sscb_ics.ics_rupass),
                   (PTR)&scb->scb_sscb.sscb_ics.ics_rupass);
			break;

	        case    GCA_EUSER:
		{
		    i4  tmp_value;

		    /*
		    ** The username should be a null-terminated string.
		    ** It seems that l_value will include the length of EOS.s
		    */
		    if(l_value < DB_OWN_MAXNAME)
			tmp_value = l_value;
		    else
			tmp_value = DB_OWN_MAXNAME-1;
		    MEcopy(flag->gca_p_value.gca_value, tmp_value,
			   scb->scb_sscb.sscb_ics.ics_eusername);
		    break;
		}

		case    GCA_EXCLUSIVE:
		    scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_EXCLUSIVE;
		    break;

		case    GCA_WAIT_LOCK:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (flag_value == GCA_ON)
			scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_NOWAIT;
		    break;

		case    GCA_IDX_STRUCT:
		case    GCA_RES_STRUCT:
		{
		    char	value[16];

		    if (l_value >= sizeof(value))
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }

		    MECOPY_VAR_MACRO(flag->gca_p_value.gca_value, l_value, value);
		    value[l_value] = '\0';
		    CVlower(value);
		    if (STcompare("hash", value) == 0)
			tbl_structure = DB_HASH_STORE;
		    else if (STcompare("chash", value) == 0)
			tbl_structure = -DB_HASH_STORE;
		    else if (STcompare("btree", value) == 0)
			tbl_structure = DB_BTRE_STORE;
		    else if (STcompare("cbtree", value) == 0)
			tbl_structure = -DB_BTRE_STORE;
		    else if (STcompare("heap", value) == 0)
			tbl_structure = DB_HEAP_STORE;
		    else if (STcompare("cheap", value) == 0)
			tbl_structure = -DB_HEAP_STORE;
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    if (p_index == GCA_IDX_STRUCT)
		    {
			if (abs(tbl_structure) == DB_HEAP_STORE)
			{
			    status = E_DB_ERROR;
			    error.err_code = E_US0022_FLAG_FORMAT;
			    break;
			}
			scb->scb_sscb.sscb_ics.ics_idxstruct = tbl_structure;
			tbl_structure = 0;
		    }
		    else
		    {
			index_structure = tbl_structure;
		    }
		    break;
		}

		case    GCA_MATHEX:
		    if (flag->gca_p_value.gca_value[0] == 'w')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_WRN_MATHEX;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_ERR_MATHEX;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'i')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_IGN_MATHEX;
		    }
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    break;

		case    GCA_CWIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_c0width);    
		    break;

		case    GCA_TWIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_t0width);    
		    break;

		case    GCA_I1WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_i1width);
		    break;

		case    GCA_I2WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i2width);
		    break;

		case    GCA_I4WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i4width);
		    break;

		case    GCA_I8WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i8width);
		    break;

		case    GCA_F4WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f4width);
		    break;

		case    GCA_F8WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f8width);
		    break;

		case    GCA_F4STYLE:
		    scb->scb_sscb.sscb_ics.ics_outarg.ad_f4style = 
			flag->gca_p_value.gca_value[0];
		    break;

		case    GCA_F8STYLE:
		    scb->scb_sscb.sscb_ics.ics_outarg.ad_f8style =    
		    	flag->gca_p_value.gca_value[0];
		    break;

		case    GCA_F4PRECISION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f4prec);    
		    break;

		case    GCA_F8PRECISION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f8prec);    
		    break;

		case    GCA_QLANGUAGE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_qlang = (DB_LANG) flag_value;
		    if ((scb->scb_sscb.sscb_ics.ics_qlang != DB_SQL) &&
		        (scb->scb_sscb.sscb_ics.ics_qlang != DB_ODQL) &&
                        (scb->scb_sscb.sscb_ics.ics_qlang != DB_ICE))
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
		    }
		    break;

		case    GCA_DBNAME:
		    /* Fold database name to lower case */
		    MEmove(l_value,
			flag->gca_p_value.gca_value,
			' ', sizeof(tempstr)-1, tempstr);
		    tempstr[sizeof(tempstr)-1] = '\0';
		    CVlower(tempstr);

		    MEmove(sizeof(tempstr)-1, tempstr, ' ',
			sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
			(PTR)&scb->scb_sscb.sscb_ics.ics_dbname);
		    break;

		case    GCA_NLANGUAGE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_language = (i4) flag_value;
		    break;
	    
		case    GCA_MPREC:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_prec =
			    (i4) flag_value;
		    break;
			
		case    GCA_MLORT:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_lort =
			    (i4) flag_value;
		    break;

		case    GCA_DATE_FORMAT:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_date_format = (DB_DATE_FMT) flag_value;
		    break;
		    
#ifdef GCA_TIMEZONE
                case    GCA_TIMEZONE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if(flag_value > 0)
			STprintf(tempstr, "GMT-%dM", flag_value);
		    else if(flag_value < 0)
			STprintf(tempstr, "GMT%dM", abs(flag_value));
		    else
			STcopy("GMT", tempstr);
		    MEcopy(tempstr, STlength(tempstr)+1,
			   scb->scb_sscb.sscb_ics.ics_tz_name);
		    timezone_recd = TRUE;
		    break;

                case    GCA_TIMEZONE_NAME:
		{
		    i4  tmp_value;
			
		    if(l_value < DB_TYPE_MAXLEN)
			tmp_value = l_value;
		    else
			tmp_value = DB_TYPE_MAXLEN-1;
		    MEcopy(flag->gca_p_value.gca_value, tmp_value,
			   scb->scb_sscb.sscb_ics.ics_tz_name);
		    scb->scb_sscb.sscb_ics.ics_tz_name[tmp_value] = EOS;
		    timezone_recd = TRUE;
		    break;
		}
#endif

		case	GCA_YEAR_CUTOFF:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_year_cutoff = (i4)flag_value;
		    year_cutoff_recd = TRUE;
		    break;

		case    GCA_MSIGN:
		    MEcopy(flag->gca_p_value.gca_value,
			    l_value,
			    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym);
			    scb->scb_sscb.sscb_ics.ics_money_format.
						    db_mny_sym[l_value] = '\0'; 
		    break;

		case    GCA_DECIMAL:
		    /*
		    ** Even though db_decimal is a 'nat', gca transmits it
		    ** as a single byte character string, so we need to
		    ** convert it back from a character to a nat.
		    */

		    scb->scb_sscb.sscb_ics.ics_decimal.db_decimal =
		        I1_CHECK_MACRO(*(i1 *)&flag->gca_p_value.gca_value[0]);
		    scb->scb_sscb.sscb_ics.ics_decimal.db_decspec = TRUE;
		    break;

		case    GCA_SVTYPE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (flag_value == GCA_MONITOR)
			scb->scb_sscb.sscb_stype = SCS_SMONITOR;
		    break;

		case    GCA_RESTART:
		case    GCA_XA_RESTART:
		{
                    char        *xid_str;
		    char	txid[50];
		    char 	*index;
                    char        c;
		    i4	        length;

   	       /* Parse the distributed transaction ID from the gca_value. */

                    xid_str = (char *)(flag->gca_p_value.gca_value);

                    /* Now switch between the Ingres Dist XID and XA XID */
                    if (p_index == GCA_RESTART)
		    {
  
                       /* Ingres Dist transaction id */
                       scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id_type =
                                DB_INGRES_DIS_TRAN_ID_TYPE;
                       
  		       if ((index = STindex(xid_str,
                                            ":", 
                                            l_value)) == NULL)
		       {
                           status = E_DB_ERROR;
  			   error.err_code = E_US0009_BAD_PARAM;
		 	   break;
		       }

                       c = *index;
                       *index = EOS;

   		       length = index - xid_str;

		       CVal(xid_str, 
			(int*)&scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
				db_ingres_dis_tran_id.db_tran_id.db_high_tran);

                       *index = c;

		       MEcopy(index+1, l_value-length-1,txid);
                       txid[ l_value - length - 1 ] = EOS;

		       CVal(txid,
			(int*)&scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
				db_ingres_dis_tran_id.db_tran_id.db_low_tran);

		     }
                     else
		     {

                        scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id_type =
                                DB_XA_DIS_TRAN_ID_TYPE;

                        status = aconv_to_struct_xa_xid( xid_str,
                           l_value,
                           &scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
                              db_xa_extd_dis_tran_id );
                        if (DB_FAILURE_MACRO( status ))
    		        {
                           status = E_DB_ERROR;
  			   error.err_code = E_US0009_BAD_PARAM;
		 	   break;
		        }
                     }

                    scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_NLK_SESS;
		    break;
		}	
			
		case    GCA_DECFLOAT:
		    
		    if (flag->gca_p_value.gca_value[0] == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_parser_compat =
							    0;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'd')
		    {
			scb->scb_sscb.sscb_ics.ics_parser_compat =
							    0;
		    }
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    break;

                 case    GCA_DATE_ALIAS:
                 {
                          i4  tmp_value;
                          char  tmp_chars[DB_TYPE_MAXLEN];

                     if(l_value < DB_TYPE_MAXLEN)
                         tmp_value = l_value;
                     else
                         tmp_value = DB_TYPE_MAXLEN-1;

                     MEcopy(flag->gca_p_value.gca_value, tmp_value, tmp_chars);
                     tmp_chars[tmp_value] = EOS;

                     if (STncasecmp(tmp_chars, "ansidate", 8) == 0)
                       scb->scb_sscb.sscb_ics.ics_date_alias =
                                         AD_DATE_TYPE_ALIAS_ANSI;
                     else
                       scb->scb_sscb.sscb_ics.ics_date_alias =
                                         AD_DATE_TYPE_ALIAS_INGRES;

                     date_alias_recd = TRUE;
                     break;
                }



		/* case    GCA_DECFLOAT: */
		default:
		    if(l_value > SCS_ENV_SIZE)
		    {
                        status = E_DB_ERROR;
  		    	error.err_code = E_US0009_BAD_PARAM;
		    }
		    status = sc0m_allocate(0,
			sizeof(SC0M_OBJECT) + l_value,
		    	DB_SCF_ID,
		    	(PTR) SCS_MEM,
		    	SCG_TAG,
		    	&env_root);
		    env_len = l_value;
		    env     = env_root + sizeof(SC0M_OBJECT);
		    if (status)
		    {
                        status = E_DB_ERROR;
  		    	error.err_code = E_US0009_BAD_PARAM;
	    		break;
		    }
		    MEcopy(flag->gca_p_value.gca_value, l_value, env);
		    env[l_value] = EOS; 
		    break;
		    
		    /*
		    ** @FixMeJasmine@
		    **
		    ** Some GCA codes that Open API sends we just
		    ** ignore for the moment
		    status = E_DB_ERROR;
		    error.err_code = E_US0022_FLAG_FORMAT;
		    break;
		    */
	    }

	    if (i < crb->gca_l_user_data && !status)
	    {
		flag = (GCA_USER_DATA *)(flag->gca_p_value.gca_value + l_value);
	    }
	}
    }


    /*
    ** Do checks and processing specific to
    ** normal threads (eg. the ones connected to FE)
    */
    if (scb->scb_sscb.sscb_stype == SCS_SNORMAL)
    {
	/* normal sessions not valid for recovery server */
	if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
	{
	    status = E_DB_ERROR;
	    error.err_code = E_SC0327_RECOVERY_SERVER_NOCNCT;
	}
    }

    for (;status == 0;)
    {
	/*
	** Prime the expedited GCA channel to handle attn interrupts.
	** Do this early in case we hang waiting on some resource
	** (e.g., fetching iidbdb info) and the user opts to break
	** out of the front end.
	*/
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
	    scc_iprime(scb);

	/*
	** The facilities needed for this session have been 
	** determined by scd_alloc_cbs() and can be found in
	** sscb_need_facilities.
	*/
	

	if ((   (scb->scb_sscb.sscb_stype == SCS_SMONITOR)
	     ||	(scb->scb_sscb.sscb_stype == SCS_SNORMAL)))
	{
	    /* Then this is a user session.  See if we are ready yet. If    */
	    /* not, wait, then release...				    */
	    while (Sc_main_cb->sc_state == SC_INIT)
		(VOID) CSsuspend(CS_TIMEOUT_MASK, 1, 0);
	}
	
    if (scb->scb_sscb.sscb_stype == GCA_MONITOR)
    {
        /*
        ** DO NOT REMOVE,COMMENT OUT, or OTHERWISE OBLITERATE THIS CODE.
        ** IT IS IMPORTANT TO BE ABLE TO IIMONITOR THINGS WHEN THE SYSTEM
        ** IS `HUNG', AND THAT'S WHAT THIS DOES.  DO NOT REMOVE IT.
        **
        ** IF THERE ARE BUGS, OR WHATEVER, EITHER FIX THEM OR TALK
        ** TO FRED.  I HAVE PUT THIS STUFF IN AT LEAST 10 TIMES, AND
        ** HAD IT REMOVED EACH AND EVERY TIME, WITH NO COMMENTS
        ** ABOUT WHY IT WAS DONE.  REMOVAL OF THIS CAPABILITY PUTS A
		** SEVERE CRIMP IN OUR ABILITY TO DEBUG THE SYSTEM.
        **
        ** PLEASE DO NOT REMOVE THIS CODE.
        */

        scb->scb_sscb.sscb_state = SCS_INPUT;
        status = 0;         /* Just in case iidbdb didn't exist */
        error.err_code = 0;
	    scb->scb_sscb.sscb_ics.ics_rustat = (DU_UALL_PRIVS);
	    scb->scb_sscb.sscb_ics.ics_maxustat = (DU_UALL_PRIVS);
    }


	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_ADF_ID))
	{
	    adf_cb = (ADF_CB *) scb->scb_sscb.sscb_adscb;  /* note scb NOT ccb */
	    adf_cb->adf_slang = scb->scb_sscb.sscb_ics.ics_language;
	    adf_cb->adf_nullstr.nlst_length = 0;
	    adf_cb->adf_nullstr.nlst_string = 0;
	    /* adf_cb->adf_qlang = scb->scb_sscb.sscb_ics.ics_qlang;*/
	    adf_cb->adf_qlang = DB_SQL;


	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_date_format, adf_cb->adf_dfmt);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_money_format, adf_cb->adf_mfmt);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_decimal, adf_cb->adf_decimal);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_date_alias, adf_cb->adf_date_type_alias);

	    adf_cb->adf_exmathopt =
		    (ADX_MATHEX_OPT) scb->scb_sscb.sscb_ics.ics_mathex;
	    adf_cb->adf_strtrunc_opt = 0;
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_outarg, adf_cb->adf_outarg);
	    adf_cb->adf_errcb.ad_ebuflen = DB_ERR_SIZE;
	    adf_cb->adf_errcb.ad_errmsgp = (char *) adf_cb + sizeof(ADF_CB);
	    adf_cb->adf_maxstring = DB_MAXSTRING;
	    /* Add 65 datatype support */
	    adf_cb->adf_proto_level = 0;
	    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_60)
		adf_cb->adf_proto_level |= (AD_DECIMAL_PROTO | AD_BYTE_PROTO); 

	    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_60)
	    {
		adf_cb->adf_proto_level |= (AD_DECIMAL_PROTO | AD_BYTE_PROTO); 
	    }

	    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_65)
	    {
		adf_cb->adf_proto_level |= AD_I8_PROTO; 
	    }

	    if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_66)
	    {
		adf_cb->adf_proto_level |= AD_ANSIDATE_PROTO; 
	    }

            if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_67)
            {
                adf_cb->adf_max_decprec = CL_MAX_DECPREC_39;
            }
            else
            {
                adf_cb->adf_max_decprec = CL_MAX_DECPREC_31;
            }

            if (date_alias_recd == FALSE)
            {
        	/* Default value of date_alias is 'ingresdate'. eg:
                ** If Client is older than 2006r2 then date_alias is automatically set
                ** to ingresdate.
                ** If Client is of 2006r2 level and date_alias is either not set or 
		** set to ingersdate then date_alias is set to ingresdate.
                ** else for newer clients, if date_alias is not provided in the
                ** config.dat file (unusual scenario) it is always set to ingresdate.
                */
                if ((scb->scb_cscb.cscb_version == GCA_PROTOCOL_LEVEL_66) &&
                        (Sc_main_cb->sc_date_type_alias == SC_DATE_ANSIDATE))
		{
                   scb->scb_sscb.sscb_ics.ics_date_alias = AD_DATE_TYPE_ALIAS_ANSI;
                   adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_ANSI;
		}
                else
		{
                   scb->scb_sscb.sscb_ics.ics_date_alias = AD_DATE_TYPE_ALIAS_INGRES;
                   adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
		}
            }

	    /* If front-end didn't send time zone, use our own time zone */
	    if (timezone_recd == FALSE)
		status = TMtz_init(&adf_cb->adf_tzcb);
	    else
	    {
		/* Search for correct timezone name */
		status = TMtz_lookup( scb->scb_sscb.sscb_ics.ics_tz_name,
				      &adf_cb->adf_tzcb);
		if( status == TM_TZLKUP_FAIL)
		{
		    /* Load new table */
		    CSp_semaphore(0, &Sc_main_cb->sc_tz_semaphore);
		    status = TMtz_load(scb->scb_sscb.sscb_ics.ics_tz_name,
				       &adf_cb->adf_tzcb);
		    CSv_semaphore(&Sc_main_cb->sc_tz_semaphore);
		}
	    }

	    if(status != OK)
	    {
		error.err_code = status;
		status = E_DB_ERROR;
		sc0e_uput(E_SC033D_TZNAME_INVALID, 0, 3,
				 STlength(scb->scb_sscb.sscb_ics.ics_tz_name),
				 (PTR)scb->scb_sscb.sscb_ics.ics_tz_name,
                                  STlength(SystemVarPrefix), SystemVarPrefix,
                                  STlength(SystemVarPrefix), SystemVarPrefix,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);		
		break;
	    }
	   
	    if (year_cutoff_recd == FALSE)
	    {
		status = TMtz_year_cutoff(&adf_cb->adf_year_cutoff);
	    }
	    else
	    {
		if ((scb->scb_sscb.sscb_ics.ics_year_cutoff 
			<= TM_DEF_YEAR_CUTOFF) ||
		    (scb->scb_sscb.sscb_ics.ics_year_cutoff 
			> TM_MAX_YEAR_CUTOFF))
		{
		    status = TM_YEARCUT_BAD;
		}
		else
		{
		    adf_cb->adf_year_cutoff = 
			scb->scb_sscb.sscb_ics.ics_year_cutoff;
		}
	    }

	    if (status != OK)
	    {
		error.err_code = status;
		status = E_DB_ERROR;
		sc0e_uput(E_SC037B_BAD_DATE_CUTOFF, 0, 2,
                                  STlength(SystemVarPrefix), SystemVarPrefix,
				  0, 
				  (PTR)(SCALARP)scb->scb_sscb.sscb_ics.ics_year_cutoff,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);		
		break;
	    }

	    adf_cb->adf_srv_cb = Sc_main_cb->sc_advcb;
	    status = adg_init(adf_cb);
	    if (status)
	    {
		error.err_code = adf_cb->adf_errcb.ad_errcode;
		if (error.err_code == E_AD1006_BAD_OUTARG_VAL)
		    error.err_code = E_US0022_FLAG_FORMAT;
		break;
	    }
            /* Move collation into adf_cb */
            /*
            ** Moved setup of collation to ensure we have a valid adf_cb
            */
            adf_cb->adf_collation = scb->scb_sscb.sscb_ics.ics_coldesc;

	    scb->scb_sscb.sscb_facility |= (1 << DB_ADF_ID);
	}


	/*
	** by now the user (a value different from the Real user could be
	** specified via -u) identifier which will be in effect at the session
	** startup have been stored as the SQL-session <auth id>.
	** Since the SQL-session <auth id> may get changed in the course of the
	** session, save it as the Initial Session <auth id>.
	*/
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_iusername);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_musername);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_iucode);
	
	/*
	** Check if user account has expired, only do this if there's a
	** frontend and the user is valid.
	*/
	if ( scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID) &&
	    (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_NOSUCHUSER) == 0 )
	{
	    status=scs_usr_expired(scb);
	    if(status!=E_DB_OK)
	    {
	    	error.err_code=E_US0061_USER_EXPIRED;
	    	status=E_DB_ERROR;
	    	break;
	    }
	}
	
	/*
	** If it's a monitor session, let it in. 
	*/
	if( scb->scb_sscb.sscb_stype == SCS_SMONITOR) 
	{
	    status = E_DB_OK;
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    error.err_code = 0;
	    break;
    	}


	/*
	** If re-connect to a session, resume the transacction context.
	*/

	if (scb->scb_sscb.sscb_ics.ics_dmf_flags & DMC_NLK_SESS)
	{
	    /* DPL_FIX_ME Must resume transaction */;
	}

	/*
	** Get ready to handle user password if required
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_RPASSWD)
	{
	    /*
	    ** Can only prompt for password if there is a frontend,
	    ** so reject if no frontend
	    */
	    if ((scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) == 0)
	    {
	        error.err_code = E_US18FF_BAD_USER_ID;
		status=E_DB_ERROR;
		break;
	    }
	    /*
	    ** Setup to prompt for password
	    */
	    scb->scb_sscb.sscb_state = SCS_SPASSWORD;
	}
	else if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
	{
	    /*
	    ** If Connected to a front end then prime for input.
	    */
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	}
	break;
    } 
    
    if (status)
    {
	if (error.err_code >> 16)
	    ascd_note(status, error.err_code >> 16);
	else
	    ascd_note(status, DB_SCF_ID);

        /* --- liibi01 Bug 60932                     --- */
	 /* --- Attach the <dbname> as one argument   --- */
        /* --- for error code E_US0010.              --- */
        /* --- Write the error message to error log. --- */

	if (status && error.err_code == 16) 
	    sc0e_put(error.err_code, 0, 1,
                     sizeof(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name),
		     (PTR) scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
		     0, (PTR) 0,
                     0, (PTR) 0,
		     0, (PTR) 0,
                     0, (PTR) 0,
		     0, (PTR) 0 );
        else if (status)
           sc0e_0_put(error.err_code, 0);
           	     
	/* else reason for this has already been logged */

	sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	if (error.err_code == 0)
	    error.err_code = E_US0004_CANNOT_ACCESS_DB;
    }
    else
    {
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_SESSION_LOG, 0, 0))
	{
	    sc0e_put(E_SC0233_SESSION_START, 0, 5,
		     ((char *)
		      STindex((PTR)scb->scb_sscb.sscb_ics.ics_eusername,
			      " ", 0)  -
		      (char *)
		      scb->scb_sscb.sscb_ics.ics_eusername),
		     (PTR)scb->scb_sscb.sscb_ics.ics_eusername,
		     ((char *)
		      STindex((PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
			      " ", 0)  -
		      (char *)
		      &scb->scb_sscb.sscb_ics.ics_rusername),
		     (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (scb->scb_sscb.sscb_stype == SCS_SMONITOR
		      ? sizeof("<Monitor Session>") - 1
		      :((char *)
			STindex((PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
				" ", 0)  -
			(char *)
			&scb->scb_sscb.sscb_ics.ics_dbname)),
		     (PTR)(scb->scb_sscb.sscb_stype == SCS_SMONITOR
			   ? "<Monitor Session>"
			   : (char *) &scb->scb_sscb.sscb_ics.ics_dbname),
		     ((scb->scb_sscb.sscb_ics.ics_lock_mode
		       == DMC_L_EXCLUSIVE)
		      ? sizeof("Exclusive") - 1
		      : sizeof("Shared") - 1),
		     (PTR)((scb->scb_sscb.sscb_ics.ics_lock_mode
			    == DMC_L_EXCLUSIVE)
			   ? "Exclusive"
			   : "Shared"),
		     0, (PTR)0,
		     0, (PTR)0);
	}
    }

    return(error.err_code);
}

/*{
** Name: ascs_terminate	- Terminate the session marked as current.
**
** Description:
**      This routine terminates the current session.  It essentially undoes 
**      whatever has been done above. 
[@comment_line@]...
**
** Inputs:
**	scb				Session control block to terminate
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-nov-1999 (somsa01)
**	    Manual integration of change 441277;
**	    Bug 90634 - Initialise STATUS before use. It is possible for
**	    status to remain undefined before it is tested to see if the
**	    E_SC012B_SESSION_TERMINATE message should be raised.
**  01-May-2008 (smeke01) b118780
**      Corrected name of sc_avgrows to sc_totrows.
**	13-May-2009 (kschendel)
**	    Fix bad call to free-pages; wants ptr *, not ptr.
*/
DB_STATUS
ascs_terminate(SCD_SCB *scb )
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		bad_status = E_DB_OK;
    DB_STATUS		err = E_DB_OK;
    ADF_CB		*adf_cb;
    SCS_MCB			*mcb;
    SCS_MCB			*new_mcb;
    DB_ERROR		error;

    MEcopy("Terminating session", 19, scb->scb_sscb.sscb_ics.ics_act1);
    scb->scb_sscb.sscb_ics.ics_l_act1 = 19;
    
    /*
    ** If this is a Factotum thread and 
    ** the Factotum thread execution function was
    ** invoked, invoke the thread exit function
    ** (if any) after first creating a SCF_FTX
    ** containing:
    **
    **	o thread-creator-supplied data, if any
    **	o length of that data
    **	o the thread creator's thread_id
    **	o status and error info from the
    **	  thread execution code
    */
    if (scb->scb_sscb.sscb_stype == SCS_SFACTOTUM &&
	scb->scb_sscb.sscb_factotum_parms.ftc_state == FTC_ENTRY_CODE_CALLED &&
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit)
    {
	SCF_FTX	ftx;

	ftx.ftx_data = scb->scb_sscb.sscb_factotum_parms.ftc_data;
	ftx.ftx_data_length = scb->scb_sscb.sscb_factotum_parms.ftc_data_length;
	ftx.ftx_thread_id = scb->scb_sscb.sscb_factotum_parms.ftc_thread_id;
	ftx.ftx_status = scb->scb_sscb.sscb_factotum_parms.ftc_status;
	ftx.ftx_error.err_code = scb->scb_sscb.sscb_factotum_parms.ftc_error.err_code;
	ftx.ftx_error.err_data = scb->scb_sscb.sscb_factotum_parms.ftc_error.err_data;

	scb->scb_sscb.sscb_factotum_parms.ftc_state = FTC_EXIT_CODE_CALLED;

	(*(scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit))( &ftx );
    }

    /* Connectionless threads don't havee alarms, alerts, or RAAT api's */
    if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
    {
	/* The session is terminating, while this flag is set scc_gcomplete()
	** will not deliver communication related CSresume()'s to this
	** thread.
	*/

	scb->scb_sscb.sscb_terminating = 1;

	/* clear out possible outstanding GCA resumes, which could incorrectly
	** resume other suspends encountered during session shutdown
	*/
	CScancelled(0);
	CSintr_ack();

#ifdef FOR_EXAMPLE_ONLY

	/*
	** Remove any alert registrations.  This is done early on in
	** scs_terminate specifically to avoid cases where the alert
	** data structures still point at already-disposed thread session
	** ids.  This is explained in sce_end_session.
	*/
	sce_end_session(scb, (SCF_SESSION)scb->cs_scb.cs_self);
#endif /* FOR_EXAMPLE_ONLY */
	}

	if (scb->scb_sscb.sscb_facility & (1 << DB_ADF_ID))
	{
	    adf_cb = (ADF_CB *) scb->scb_sscb.sscb_adscb;  /* note scb NOT ccb */
	    status = adg_release(adf_cb);
	    if (status)
	    {
		error.err_code = status;
		sc0e_0_put(error.err_code, 0);
		status = E_DB_ERROR;
	    }
	}

    /* re-enable CSresume()'s from scc_gcomplete so session rundown works */
    scb->scb_sscb.sscb_terminating = 0;

	/* don't dissasoc again if we already did. */

	if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID) &&
	    (scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA) == 0 )
	{
	    status = ascs_disassoc(scb->scb_cscb.cscb_assoc_id);
	    if (status)
	    {
		sc0e_0_put(status, 0);
		error.err_code = status;
	    }
	}

    if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID))
	scc_dispose(scb);	/* trash remainder of messages, if any */
    scb->scb_sscb.sscb_facility = 0;

    /*
    ** Run down list of this session's owned memory
    */

    for (mcb = scb->scb_sscb.sscb_mcb.mcb_sque.sque_next;
	    mcb != &scb->scb_sscb.sscb_mcb;
	    mcb = new_mcb)
    {
	sc0e_put(E_SC0227_MEM_NOT_FREE, 0, 1, 
		    sizeof (mcb->mcb_facility), (PTR)&mcb->mcb_facility,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );

	/* first take it off the session queue, if appropriate */

	new_mcb = mcb->mcb_sque.sque_next;
	if (mcb->mcb_session != DB_NOSESSION)
	{
	    mcb->mcb_sque.sque_next->mcb_sque.sque_prev
		    = mcb->mcb_sque.sque_prev;
	    mcb->mcb_sque.sque_prev->mcb_sque.sque_next
		    = mcb->mcb_sque.sque_next;
	}

	status = sc0m_free_pages(mcb->mcb_page_count, &mcb->mcb_start_address);
	if (status != E_SC_OK)
	{
	    sc0e_0_put(status, 0);
	}
	
	/* now unlink the mcb from the general mcb list and return it */

	mcb->mcb_next->mcb_prev = mcb->mcb_prev;
	mcb->mcb_prev->mcb_next = mcb->mcb_next;
	status = sc0m_deallocate(0, (PTR*) &mcb);
	if (status)
	{
	    sc0e_0_put(status, 0);
	}
    }

    /* stop leak! */
    CSr_semaphore( &scb->scb_sscb.sscb_asemaphore );

    if (status)
    {
	sc0e_0_put(E_SC012B_SESSION_TERMINATE, 0);
    }
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_SESSION_LOG, 0, 0))
    {
	sc0e_0_put(E_SC0234_SESSION_END, 0);
    }
    if ((Sc_main_cb->sc_totrows > MAXI4)
	|| (Sc_main_cb->sc_selcnt > MAXI4))
    {
	i4		avg;

	avg = (Sc_main_cb->sc_selcnt
		    ? Sc_main_cb->sc_totrows / Sc_main_cb->sc_selcnt
		    : 0);
	sc0e_put(E_SC0235_AVGROWS, 0, 2,
		 sizeof(Sc_main_cb->sc_selcnt),
		 (PTR)&Sc_main_cb->sc_selcnt,
		 sizeof(avg),
		 (PTR)&avg,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	Sc_main_cb->sc_totrows= 0;
	Sc_main_cb->sc_selcnt = 0;
    }

    if (bad_status)
    	ascd_note(bad_status, error.err_code >> 16);
    return(status);
}

/*{
** Name: ascs_ctlc	- Handle Control-C type interrupts to the DBMS
**
** Description:
**      This routine translates the control-c interrupt from GCF into 
**      a form understandable by CSattn().  That is, the call is 
**      passed to CSattn() with an event id of CS_ATTN_EVENT.
**
** Inputs:
**      sid                             Session id of recipient.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    The session identified is notified of the outstanding event.
**
** History:
**      12-Feb-99 (fanra01)
**          Readd function.
*/
VOID
ascs_ctlc(SCD_SCB *scb)
{
    CSattn(CS_ATTN_EVENT, scb->cs_scb.cs_self);
    return;
}

/*{
** Name: ascs_disassoc - Low level GCA disconnect
**
** Description:
**      This routine is provided so that the GCA_DISASSOC call
**      can be easily used.
**
** Inputs:
**	assoc_id	GCA association id
**
** Outputs:
**      none
**
**	Returns:
**	    STATUS	error from GCA
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/
STATUS
ascs_disassoc(i4 assoc_id )
{
    STATUS		status;
    GCA_DA_PARMS	da_parms;
    i4			indicators;
    STATUS		gca_error;
    CS_SCB		*cs_scb;

    CSget_scb(&cs_scb);

    da_parms.gca_association_id = assoc_id;
    indicators = 0;
    do {
	status = IIGCa_call(GCA_DISASSOC,
			    (GCA_PARMLIST *)&da_parms,
			    indicators,
			    (PTR)cs_scb,
			    (i4)-1,
			    &gca_error);

	if (status)
	    return( gca_error );
    
	status = CSsuspend(CS_BIO_MASK, 0, 0);

	if (status)
	    return status;

	indicators = GCA_RESUME;

    } while (da_parms.gca_status == E_GCFFFE_INCOMPLETE);

    return da_parms.gca_status;
}



/*{
** Name: conv_to_struct_xa_xid
**
** Description:
**      This routine converts an XA XID from a string form to the XID
**      structure format. Is called at the time of GCA_XA_RESTART of an
**      Ingres session, over which the XA XN is sub-sequently committed 
**      or rolled back.
**
** Inputs:
**      xid_text        - the xid in text string format.
**      xid_str_max_len - the max length of the xid string.
**      xa_xid_p        - pointer to an XA XID.
**
** Outputs:
**      none
**
**	Returns:
**	    DB_STATUS	if the routine is successful or not.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

DB_STATUS aconv_to_struct_xa_xid( char               *xid_text,
                                 i4            xid_str_max_len,
                                 DB_XA_EXTD_DIS_TRAN_ID  *xa_extd_xid_p )
{


   char        *cp, *next_cp, *prev_cp;
   char        *tp;
   char        c;
   i4          j, num_xid_words;
   STATUS      cl_status;


   if ((cp = STindex(xid_text, ":", xid_str_max_len)) == NULL)
   {
       return( E_DB_ERROR ); 
   }

   c = *cp;
   *cp = EOS;

   if ((cl_status = CVahxl(xid_text, (u_i4 *)
			  (&xa_extd_xid_p->db_xa_dis_tran_id.formatID))) != OK)
   {
      return( E_DB_ERROR );
   }

   *cp = c;    

   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = EOS;

   if ((cl_status = CVal(cp + 1, 
			(i4 *)&xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length)) != OK)
   {
      return( E_DB_ERROR );
   }

   *next_cp = c;    

   if ((cp = STindex(next_cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *cp;
   *cp = EOS;

   if ((cl_status = CVal(next_cp + 1, 
                        (i4 *)&xa_extd_xid_p->db_xa_dis_tran_id.bqual_length)) != OK)
   {
      return( E_DB_ERROR );
   }

   *cp = c;    

    /* Now copy all the data                            */
    /* We now go through all the hex fields separated by */
    /* ":"s.  We convert it back to long and store it in */
    /* the data field of the distributed transaction id  */
    /*     cp      - points to a ":"                       */
    /*     prev_cp - points to the ":" previous to the     */
    /*               ":" cp points to, as we get into the  */
    /*               loop that converts the XID data field */
    /*     tp    -   points to the data field of the       */
    /*               distributed transaction id            */

    prev_cp = cp;
    tp = &xa_extd_xid_p->db_xa_dis_tran_id.data[0];
    num_xid_words = (xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length + 
		     xa_extd_xid_p->db_xa_dis_tran_id.bqual_length +
                     sizeof(u_i4)-1) / sizeof(u_i4);
    for (j=0;
         j< num_xid_words;
         j++)
    {

         if ((cp = STindex(prev_cp + 1, ":", 0)) == NULL)
         {
             return( E_DB_ERROR );
         }
         c = *cp;
         *cp = EOS;
         if ((cl_status = CVahxl(prev_cp + 1, (u_i4 *)tp)) != OK)
         {
             return( E_DB_ERROR );
         }
         tp += sizeof(u_i4);
         *cp = c;
         prev_cp = cp;
    }

    next_cp = cp;                           /* Store last ":" position */
                                            /* Skip "XA" & look for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" before branch seq */
             return( E_DB_ERROR );
       }

    next_cp = cp;                          /* Skip branch seq for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" after branch seq */
             return( E_DB_ERROR );
       }

    c = *cp;                               /* Store actual char in that POS */
    *cp = EOS;                             /* Pretend end of string */

    if ((cl_status = CVan(next_cp + 1, 
                          &xa_extd_xid_p->branch_seqnum)) != OK)
    {
      return( E_DB_ERROR );
    }

   *cp = c;                              /* Restore char in that Position */
    
   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = EOS;

   if ((cl_status = CVan(cp + 1, 
			&xa_extd_xid_p->branch_flag)) != OK)
   {
      return( E_DB_ERROR );
   }

   *next_cp = c;    

   return( E_DB_OK );

} /* conv_to_struct_xa_xid */


/*{
** Name: scs_usr_expired
**
** Description:
**      This routine checks if the current user account has expired or not.
**
** Inputs:
**      scb		- SCB, session control block.
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_OK	- Not expired
**	    E_DB_WARN   - Expired
**	    E_DB_ERROR  - Error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-jun-93  (robf)
**	    Created for Secure 2.0
*/
static DB_STATUS
scs_usr_expired(
	SCD_SCB            *scb
)
{
	DB_STATUS status;
	ADF_CB		    *adf_scb = (ADF_CB *) scb->scb_sscb.sscb_adscb;  
	ADF_CB		    adf_cb ;
	DB_DATA_VALUE	    tdv;
	DB_DATA_VALUE	    fdv;
	DB_DATE		    date_now;
	DB_DATE		    date_empty;
	i4		    cmp;

	/* Copy the session ADF block into local one */
	STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);

	adf_cb.adf_errcb.ad_ebuflen = 0;
	adf_cb.adf_errcb.ad_errmsgp = 0;
	/*
	** Check if date is "empty" or not, if its the "empty" date we
	** ignore.
	*/
	tdv.db_prec	    = 0;
	tdv.db_length	    = DB_DTE_LEN;
	tdv.db_datatype	    = DB_DTE_TYPE;
	tdv.db_data         = (PTR)&date_empty;

	status=adc_getempty( &adf_cb, &tdv);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;
	
	/*
	** Now compare the two dates
	*/
	fdv.db_prec	    = 0;
	fdv.db_length	    = DB_DTE_LEN;
	fdv.db_datatype	    = DB_DTE_TYPE;
	fdv.db_data         = (PTR)&scb->scb_sscb.sscb_ics.ics_expdate;

	status=adc_compare(&adf_cb,  &fdv, &tdv, &cmp);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;

	if(!cmp)
	{
		/*
		** Expire date is empty, so return OK
		*/
		return E_DB_OK;
	}
	/*
	** Generate date "now"
	*/
	tdv.db_prec	    = 0;
	tdv.db_length	    = DB_DTE_LEN;
	tdv.db_datatype	    = DB_DTE_TYPE;
	tdv.db_data         = (PTR)&date_now;
	status=adu_datenow(&adf_cb, &tdv);
	if(status!=E_DB_OK)
	{
		return E_DB_ERROR;
	}

	/*
	**  Compare against expire date
	*/
	fdv.db_prec	    = 0;
	fdv.db_length	    = DB_DTE_LEN;
	fdv.db_datatype	    = DB_DTE_TYPE;
	fdv.db_data         = (PTR)&scb->scb_sscb.sscb_ics.ics_expdate;

	status=adc_compare(&adf_cb,  &fdv, &tdv, &cmp);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;

	if(cmp>0)
	{
		/*
		** Expire date > current date, OK
		*/
		return E_DB_OK;
	}
	else
	{
		/*
		** Expire date <= current date, fail
		*/
		return E_DB_WARN;
	}
}

/*
** Name: ascs_check_password
**
** Description:
**	This checks a user password following a prompt, performing
**	any required security auditing
**
** Inputs:
**	scb	- SCB
**
**	passwd	- Password, not encrypted
**
**	pwlen   - Length of passwd
**
**	do_alarm_audit - TRUE if fire database alarms, do auditing
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK	- Password is valid
**
**	E_DB_ERROR  -Password is not  valid
**
** History:
**	5-nov-93 (robf)
**          Created
**	14-dec-93 (robf)
**          Fire security alarms on successful password prompt.
**	15-mar-04 (robf)
**          Made external, check for external passwords, add
**	    do_alarm_audit parameter
**	16-jul-96 (stephenb)
**	    Add efective username to scs_check_external_password() parameters.
*/
DB_STATUS
ascs_check_password( SCD_SCB *scb, 
		char *passwd, 
		i4 pwlen,
		bool do_alarm_audit)
{
    DB_STATUS 	status=E_DB_OK;
    bool  	loop=FALSE;
    SCF_CB	scf_cb;
    DB_STATUS   local_status;
    SXF_ACCESS	sxfaccess;

    do
    {
	/*
	** If no password then invalid
	*/
	if(pwlen<=0)
	{
		status=E_DB_ERROR;
		break;
	}
	/*
	** Save input password
	*/
	MEmove(pwlen, passwd, ' ',
		sizeof(scb->scb_sscb.sscb_ics.ics_rupass),
		(PTR)&scb->scb_sscb.sscb_ics.ics_rupass);
	/*
	** Check if user has external password or not
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_REXTPASSWD)
	{
		status = ascs_check_external_password (
			scb,
			&scb->scb_sscb.sscb_ics.ics_rusername,
			scb->scb_sscb.sscb_ics.ics_eusername,
			&scb->scb_sscb.sscb_ics.ics_rupass,
			FALSE);
		if(status==E_DB_WARN)
		{
		     /*
		     ** Access is denied
		     */
		     status=E_DB_ERROR;
		     break;
		}
		else if (status!=E_DB_OK)
		{
		     /*
		     ** Some kind of error
		     */
		     sc0e_0_put(E_SC035C_USER_CHECK_EXT_PWD, 0);
		     break;

		}
		else
		{
		    /*
		    ** Access is allowed
		    */
		    break;
		}
	}
	/*
	** Encrypt password
	*/

	if (STskipblank((char*)&scb->scb_sscb.sscb_ics.ics_rupass,
	      sizeof(scb->scb_sscb.sscb_ics.ics_rupass))
	    != NULL)
	{
	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type   = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_SCF_ID;
	    scf_cb.scf_session  = DB_NOSESSION;
	    scf_cb.scf_nbr_union.scf_xpasskey =
	        (PTR)scb->scb_sscb.sscb_ics.ics_rusername.db_own_name;
	    scf_cb.scf_ptr_union.scf_xpassword =
	        (PTR)scb->scb_sscb.sscb_ics.ics_rupass.db_password;
	    scf_cb.scf_len_union.scf_xpwdlen =
	        sizeof(scb->scb_sscb.sscb_ics.ics_rupass);
	    status = scf_call(SCU_XENCODE,&scf_cb);
	    if (status)
	    {
	        status = (status > E_DB_ERROR) ?
		      status : E_DB_ERROR;
		sc0e_0_put(E_SC0260_XENCODE_ERROR, 0);
	        break;
	    }
	}
	/*
	** If no such user then invalid
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_NOSUCHUSER)
	{
		status=E_DB_ERROR;
		break;
	}
	/*
	** Now password is encrypted, compare it to real password
	*/
	if(MEcmp(
	       (PTR)&scb->scb_sscb.sscb_ics.ics_rupass,
	       (PTR)&scb->scb_sscb.sscb_ics.ics_srupass,
	       (u_i2)sizeof(scb->scb_sscb.sscb_ics.ics_rupass))
		    != 0)
	{
		/*
		** Password doesn't match
		*/
		status=E_DB_ERROR;
		break;
	}
    } while(loop);

    return status;
}

/*
** Name: scs_check_app_code - Check application code usage
**
** Description:
**	This routine checks whether a session may use a requested application
**	code.
**
** Inputs:
**	SCB
**
** Returns:
**	E_DB_OK - Access is allowed
**
**	E_DB_ERROR - Access not allowed
**
** History:
**	18-mar-94 (robf)
**          Modularized out of scs_dbdb_info()
**	24-mar-94 (robf)
**          Reworked DBA_DESTROYDB. This operates by connecting to the
**	    iidbdb, so we restrict to that.
**	12-dec-94 (forky01)
**	    Fix bug 65946 - Now checks for ingres (case insensitive)user for
**	    authorization to use createdb iidbdb and also upgradedb.
**	    Previously just did MEcmp on ingres.
*/
static DB_STATUS
scs_check_app_code(SCD_SCB *scb)
{
    char 	tmpbuf[32];
    DB_STATUS 	status=E_DB_OK;

    if (scb->scb_sscb.sscb_ics.ics_appl_code >= DBA_MIN_APPLICATION &&
	 scb->scb_sscb.sscb_ics.ics_appl_code <= DBA_MAX_APPLICATION )
    {
	switch (scb->scb_sscb.sscb_ics.ics_appl_code)
	{
	case DBA_CREATEDB:
	    /*
	    ** These are DBA-related operations, so may only be  specified
	    ** by the DBA, or a user with DB_ADMIN priv, or a user  with
	    ** CREATEDB priv.
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_UCREATEDB)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"DBA-related application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		status=E_DB_ERROR;
	    }
	    break;
	case DBA_OPTIMIZEDB:
	    /*
	    ** Optimizedb is allowed for DBA, DB_ADMIN, TABLE_STATISTICS
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
	       (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_TABLE_STATS)
		||
	       (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"Optimizedb/Statdump code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_PRETEND_VFDB:
	case DBA_FORCE_VFDB:
	case DBA_PURGE_VFDB :
	case DBA_SYSMOD:
	case DBA_NORMAL_VFDB:
	    /*
	    ** These are DBA-related operations, so may only be  specified
	    ** by the DBA, or a user with DB_ADMIN priv, or a user  with
	    ** OPERATOR/SECURITY priv.
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_UOPERATOR)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"DBA-related application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );

		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_DESTROYDB:
	    /*
	    ** Destroydb connects to iidbdb to destroy a database. 
	    ** Restrict access to SECURITY privilege or connections to
	    ** iidbdb.
	    */
    	    if (!(MEcmp((PTR)DB_DBDB_NAME,  
			(PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
			sizeof(scb->scb_sscb.sscb_ics.ics_dbname))==0
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
	       ))
	    {
		STprintf(tmpbuf,"Application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );

		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_CONVERT:
    	    /*
    	    ** Restrict upgradedb to either DBA or users with SECURITY
	    ** privilege
    	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN))
	       )
       	    {
	    	 sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     sizeof("Createdb IIDBDB application code")-1, 
			"Createdb IIDBDB application code",
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_IIFE:
	   /*
	   ** This is a normal INGRES frontend, no extra checks
	   */
	   break;
	case DBA_CREATE_DBDBF1:
	    break;
	case DBA_FINDDBS:
	case DBA_VERIFYDB:
	default:
	    /*
	    ** Unknown/unused app code, reject
	    */
	    STprintf(tmpbuf,"Invalid application code %d",
			scb->scb_sscb.sscb_ics.ics_appl_code);
	    sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		 sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
	         STlength(tmpbuf),(PTR)tmpbuf,
		 sizeof(SystemProductName), SystemProductName,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	     status=E_DB_ERROR;
	     break;
	}
   }
   return status;
}

/*{
** Name: ascs_atfactot	- attach a Factotum thread
**
** Description:
**
**	This function is called via scf_call(SCS_ATFACTOT).
**
**	Its purpose is to create and invoke a Factotum 
**	(fac - do, totum - all) thread. Factotum threads are intended
**	to be short-lived internal threads which have no connection to
**	a front end and endure only for the specific task for which they
**	were invoked.
**
** Inputs:
**      scf				the session control block
**					with an embedded
**		SCF_FTC
**			ftc_facilities	what facilities are needed by this thread?
**					If SCF_CURFACILITIES, the factotum
**					thread will assume the same facilities
**					as the creating thread.
**	 		ftc_data	optional pointer to thread-specific
**					input data.
**			ftc_data_length	optional length of that data. If present,
**					memory for "data" will be allocated with
**					the factotum thread's SCB and "data"
**					copied to that location. This should be
**					used for non-durable "data" that may
**					dissolve if the creating thread terminates
**					before the factotum thread.
**			ftc_thread_name pointer to a character string to identify
**					this thread in iimonitor, etc.
**			ftc_thread_entry thread execution function pointer.
**			ftc_thread_exit	optional function to be executed when
**					the factotum thread terminates. This
**					function will be called if and only if
**					ftc_thread_entry was invoked.
**			ftc_priority	priority at which thread is to run
**					If SCF_CURPRIORITY, the factotum thread
**					will assume the same priority as the
**					creating thread.
**
** Outputs:
**			ftc_thread_id   contains the new thread's SID.
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Another thread is added to the server
**
** History:
**	09-Mar-1998 (jenjo02)
**	    Invented.
**      12-Feb-99 (fanra01)
**          Readd function.
*/
STATUS
ascs_atfactot(SCF_CB *scf )
{
    SCF_FTC	*ftc = scf->scf_ptr_union.scf_ftc;
    STATUS	status;
    CL_ERR_DESC err;

    /* Use attaching threads's priority and/or facilities? */
    if (ftc->ftc_priority == SCF_CURPRIORITY ||
	ftc->ftc_facilities == SCF_CURFACILITIES)
    {
	SCD_SCB		*scb;

	CSget_scb((CS_SCB **)&scb);

	if (ftc->ftc_priority == SCF_CURPRIORITY)
	    ftc->ftc_priority = scb->cs_scb.cs_priority;
	if (ftc->ftc_facilities == SCF_CURFACILITIES)
	    ftc->ftc_facilities = scb->scb_sscb.sscb_facility;
    }

    return(CSadd_thread(ftc->ftc_priority, (PTR)ftc, SCS_SFACTOTUM, 
			&ftc->ftc_thread_id, &err));

}

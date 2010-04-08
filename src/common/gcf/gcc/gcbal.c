/*
** Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>

# include    <cv.h>
# include    <er.h>
# include    <gc.h>
# include    <gcccl.h>
# include    <me.h>
# include    <mu.h>
# include    <qu.h>
# include    <si.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>

# include    <sl.h>
# include    <iicommon.h>
# include    <gca.h>
# include    <gcaint.h>
# include    <gcatrace.h>
# include    <gcc.h>
# include    <gccal.h>
# include    <gcr.h>
# include    <gccpci.h>
# include    <gccpb.h>
# include    <gccer.h>
# include    <gccgref.h>
# include    <gcxdebug.h>
# include    <gcn.h>
# include    <gcnint.h>

/* 
**  INSERT_AUTOMATED_FUNCTION_PROTOTYPES_HERE
*/

/*
**
**  Name: GCBAL.C - GCF Bridge server Application Layer module
**
**  Description:
**      GCBAL.C contains all routines which constitute the Application Layer 
**      of the GCF Bridge server. 
**
**          gcb_al - Main entry point for the Application Layer.
**          gcb_alinit - PBAL initialization routine
**          gcb_alterm - PBAL termination routine
**	    gcb_gca_exit - first level exit routine for GCA request completions
**          gcb_al_evinp - FSM input event evaluator
**          gcb_alout_exec - FSM output event execution processor
**          gcb_alactn_exec - FSM action execution processor
**	    gcb_al_abort - AL internal abort routine
**
**
**  History:
**      21-Jan-96 (rajus01)
**          Initial module creation
**	20-Aug-97 (gordy)
**	    Replaced gcb_lcl_id with gcc_lcl_id, and GCB_STOP with GCC_STOP.  
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-jun-2001 (somsa01)
**	    In gcb_alinit(), write out the GCB server id to standard output.
**      26-Jun-2001 (wansh01) 
**          replace gcxlevel with IIGCc_global->gcc_trace_level.
*/


/*
**  Forward and/or External function references.
*/

static u_i4	gcb_al_evinp( GCC_MDE *mde, STATUS *generic_status );
static STATUS	gcb_alout_exec( i4  output_id, GCC_MDE *mde, GCC_CCB *ccb );
static STATUS	gcb_alactn_exec( u_i4 action_code, GCC_MDE *mde,
							GCC_CCB *ccb );
static VOID	gcb_al_abort( GCC_MDE *mde, GCC_CCB *ccb, STATUS *status, 
			      CL_ERR_DESC *system_status, GCC_ERLIST *erlist );

/*
** Name: gcb_al	- Entry point for the Application Layer
**
** Description:
**	gcb_al is the entry point for the application layer.  It invokes
**	the services of subsidiary routines to perform its requisite functions.
**	It is invoked with a Message Data Element (MDE) as its input.
**	The MDE  comes from GCA .
**
**	PBAL operation may be divided into three phases: initialization,
**	association management and shutdown.  Initialization is performed once
**	at Bridge server startup time.  The static global table is built, a GCA
**	listen is established, and some miscellaneous other housekeeping is
**	done, in preparation for association management,the real meat of Bridge 
**	server PBAL operation.
**
**	Shutdown phase is entered as a result of an external command, from the
**	comm server management utility.  Shutdown may be specified as immediate
**	or graceful.  Immediate shutdown simply terminates Bridge server
**	operation by invoking GCshut(), and all existing associations terminate
**	with error conditions.  It is a commanded bridge server crash.  No
**	attempt is made to bring things down in an orderly manner.
**
**	If graceful shutdown is commanded, the bridge server enters a quiescing
**	state.  

**	States, input events, output events and actions are symbolized
**	respectively as SBAxxx, IBAxxx, OBAxxx and ABAxxx.
**
**	Output events and actions are marked with a '*' if they pass 
**	the incoming MDE to another layer, or if they delete the MDE.
**	There should be exactly one such output event or action per
**	state cell.  Output events and actions are marked with a '=' if
**	they allocate a new MDE and (necessarily) pass it to another layer.
**
**	In the FSM definition matrix which follows, the parenthesized integers
**	are the designators for the valid entries in the matrix, and are used in
**	the representation of the FSM in GCCPB.H as the entries in PBALFSM_TBL
**	which are the indices into the entry array PBALFSM_ENTRY.  See GCCPB.H
**	for further details.

**      Association initiation is indicated in one of two ways:
**
**	by receipt of an A_ASSOCIATE MDE constructed and sent by gcb_gca_exit
**	when a GCA_LISTEN has completed;
**
**      GCA_DISASSOC is invoked to terminate the GCA association.
** 
**	The receipt of an A_ABORT request indicates the failure of a GCA data 
**	transfer operation (GCA_SEND or GCA_RECEIVE), and hence the failure 
**	of the local association, was detected by the appropriate async 
**      completion_exit.

**	FOR the Application Layer, the following states, output events and
**	actions are defined, with the state characterizing an individual
**	association:
**
**	States:
**
**	    SBACLD   - Idle, association does not exist.
**	    SBASHT   - Server shutdown in process
**          SBAQSC   - Server quiesce in process
**          SBACLG   - Association is closing
**  	    SBARJA   - IAL: GCA_RQRESP (reject) pending
**
**	Input Events:
**
**	    IBALSN   -  A_LISTEN request (place GCA_LISTEN)
**	    IBAABQ   -  A_ABORT request (Local association has failed)
**	    IBAACE   -  A_COMPLETE request && MSG_Q_Empty (GCA_SEND or 
**                      GCA_RQRESP has completed)                   
**	    IBAARQ   -  A_RELEASE request (GCA_RELEASE message received)
**	    IBASHT   -  A_SHUTDOWN request (terminate execution NOW)
**	    IBAQSC   -  A_QUIESCE request (no new sessions)
**	    IBAABT   -  A_P_ABORT request (AL failure)
**          IBAAFN   -  A_FINISH request (GCA_DISASSOC has completed)
**          IBAREJI  -  A_I_REJECT request (IAL local reject) 
**

**	Actions:
**
**	    ABALSN   -  GCA_LISTEN
**	    ABADIS=  -  GCA_DISASSOC
**          ABARSP*  -  GCA_RQRESP
**	    ABAMDE*  -  Discard the input MDE
**	    ABACCB   -  Doom or delete the CCB
**	    ABASHT   -  Signal immediate Bridge Server shutdown
**	    ABAQSC   -  Set "quiesce and shutdown" indicator

**  Server Control State Table
**  --------------------------
** ( Please refer gccpb.c for further details. )
**
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Jan-96 (rajus01)
**	   Initial Implementation. Culled from gccal.c
**      29-Mar-96 (rajus01)
**	   Fixed debug statements.
**      20-Jun-96 (rajus01)
**	    Don't post LISTEN again, when GCC_A_DISASSOC request
**	    completes. This caused problems on VMS platform.
**	22-Oct-97 (rajus01)
**	    Replaced gcc_get_obj(), gcc_del_obj() with
**	    gcc_add_mde(), gcc_del_mde() calls respectively.
*/


STATUS
gcb_al( GCC_MDE *mde )
{
    u_i4	input_event;
    STATUS	status;

    /*
    ** Invoke  the FSM executor, passing it the current state and the 
    ** input event, to execute the output event(s), the action(s)
    ** and return the new state.  The input event is determined by the
    ** output of gcb_al_evinp().
    */

    status = OK;
    input_event = gcb_al_evinp( mde, &status );

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "gcb_al: primitive %s %s event %s size %d\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look( gcx_mde_primitive, mde->primitive_type ),
		gcx_look( gcx_gcb_ial, input_event ),
		mde->p2 - mde->p1 );
# endif /* GCXDEBUG */

    if( status == OK )
    {
	i4		output_event;
	u_i4           action_code;
	i4		i;
	i4		j;
	GCC_CCB		*ccb = mde->ccb;
	u_i4		old_state;

	/*
	** Determine the state transition for this state and input event and
	** return the new state.
	*/

	old_state = ccb->ccb_aae.a_state;
	j 	  = pbalfsm_tbl[ input_event ][ old_state ];

	/* Make sure we have a legal transition */
	if( j != 0 )
	{
	    ccb->ccb_aae.a_state = pbalfsm_entry[ j ].state;

	    /* do tracing... */
	    GCX_TRACER_3_MACRO("gcb_alfsm>", mde, "event = %d, state = %d->%d",
			     input_event, old_state, ccb->ccb_aae.a_state);

# ifdef GCXDEBUG
	    if( IIGCc_global->gcc_trace_level >=1 && ccb->ccb_aae.a_state != old_state )
		TRdisplay("gcb_alfsm: new state=%s->%s\n", 
			    gcx_look( gcx_gcb_sal, old_state ), 
			    gcx_look( gcx_gcb_sal, ccb->ccb_aae.a_state ) );
# endif /* GCXDEBUG */

	    /*
	    ** For each output event in the list for this state and input event,
	    ** call the output executor.
	    */

	    for( i = 0;
		 i < PB_OUT_MAX
		 &&
		 ( output_event =
		    pbalfsm_entry[ j ].output[ i ] )
		 > 0;
		 i++
	       )

	    {
		if( ( status = gcb_alout_exec( output_event, mde, ccb ) )
								  != OK )
		{
		    /*
		    ** Execution of the output event has failed. Aborting the 
		    ** association will already have been handled so we just get
		    ** out of here.
		    */
		    return( status );
		} /* end if */
	    } /* end for */

	    /*
	    ** For each action in the list for this state and input event,
	    ** call the action executor.
	    */

	    for( i = 0;
		 i < PB_ACTN_MAX
		 &&
		 ( action_code =
		 pbalfsm_entry[ j ].action[ i ] ) != 0;
		 i++
	       )

	    {
		if( ( status = gcb_alactn_exec( action_code, mde, ccb ) )
							!= OK )
		{
		    /*
		    ** Execution of the action has failed. Aborting the 
		    ** association will already have been handled so we just get
		    ** out of here.
		    */
		    return( status );
		} /* end if */
	    } /* end for */
	} /* end if */
	else
	{
	    GCC_ERLIST		erlist;

	    /*
	    ** This is an illegal transition.  This is a program bug or a
	    ** protocol violation.
	    ** Log the error.  The message contains 2 parms: the input event and
	    ** the current state.
	    */

	    erlist.gcc_parm_cnt = 2;
	    erlist.gcc_erparm[0].size = sizeof(input_event);
	    erlist.gcc_erparm[0].value = (PTR)&input_event;
	    erlist.gcc_erparm[1].size = sizeof(old_state);
	    erlist.gcc_erparm[1].value = (PTR)&old_state;
	    status = E_GC2A06_PB_FSM_STATE;

	    /* Abort the association */
	    gcb_al_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL, &erlist );
	    status = FAIL;
	}
    }
    else
    {
	/*
	** Evaluation of the input event has failed.  Log the status
	** returned by gcb_al_evinp, and abort FSM execution.
	*/
	gcb_al_abort( mde, mde->ccb, &status, (CL_ERR_DESC *) NULL,
		      (GCC_ERLIST *) NULL );
	status = FAIL;
    }

    return( status );

}  /* end gcb_al */


/*{
** Name: gcb_alinit	- Bridge Application Layer initialization routine
**
** Description:
**      gcb_alinit is invoked by ILC on bridge server startup.  It initializes
**      the PBAL and returns.  The following functions are performed: 
** 
**      - Initialize use of GCA by issuing GCA_INITIATE 
**      - Register as a server by issuing GCA_REGISTER 
**      - Establish an outstanding GCA_LISTEN through the local GCA 
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Jan-96 (rajus01)
**          Initial routine creation. Culled from gccalinit.c.
**	06-jun-2001 (somsa01)
**	    Write out the GCB server id to standard output.
*/
STATUS
gcb_alinit
(
    STATUS 		*generic_status,
    CL_ERR_DESC 	*system_status 
)
{
    GCA_IN_PARMS        in_parms;
    GCA_RG_PARMS        rg_parms;
    GCC_CCB		*ccb;
    GCC_MDE		*mde;
    STATUS              status;
    STATUS              tmp_status;
    STATUS              call_status;
    STATUS              return_code = OK;
    char		buffer[512];

    *generic_status = OK;

    /*
    ** Invoke GCA_INITIATE service to initialize use of GCA.
    ** Fill in the parm list, then call IIGCa_call.  
    */

    in_parms.gca_expedited_completion_exit = gcb_gca_exit;
    in_parms.gca_normal_completion_exit = gcb_gca_exit;
    in_parms.gca_alloc_mgr = NULL;
    in_parms.gca_dealloc_mgr = NULL;
    in_parms.gca_p_semaphore = NULL;
    in_parms.gca_v_semaphore = NULL;
 
    /*
    ** We need to specify the api version that we're using and that we're
    ** using asynchronous services.
    */

    in_parms.gca_modifiers = GCA_API_VERSION_SPECD | GCA_ASY_SVC;
    in_parms.gca_local_protocol = GCC_GCA_PROTOCOL_LEVEL;
    in_parms.gca_api_version = GCA_API_LEVEL_4;
    call_status = IIGCa_call(	GCA_INITIATE, 
				(GCA_PARMLIST *)&in_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status);

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if( call_status != OK )
    {
        *generic_status = status;
	return FAIL;
    } /* end if */
    if( in_parms.gca_status != OK )
    {
	*generic_status = in_parms.gca_status;
	STRUCT_ASSIGN_MACRO(in_parms.gca_os_status, *system_status);
	return FAIL;
    } /* end if */
    else
    {
	IIGCc_global->gcc_gca_hdr_lng = in_parms.gca_header_length;
    } /* end else */

    /*
    ** Register as a server with Name Server.
    ** Fill in the parm list, then call IIGCa_call.  
    */

    rg_parms.gca_l_so_vector = 0;
    rg_parms.gca_served_object_vector = NULL;
    rg_parms.gca_srvr_class = GCA_BRIDGESVR_CLASS;
    rg_parms.gca_installn_id = NULL;
    rg_parms.gca_process_id = NULL;
    rg_parms.gca_no_connected = 0;
    rg_parms.gca_no_active = 0;
    rg_parms.gca_modifiers = GCA_RG_BRIDGESVR;
    rg_parms.gca_listen_id = NULL;
    call_status = IIGCa_call(	GCA_REGISTER, 
				(GCA_PARMLIST *)&rg_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status );

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if( call_status != OK )
    {
        *generic_status = status;
	return FAIL;
    } /* end if */
    if( rg_parms.gca_status != OK )
    {
	tmp_status = E_GC2219_GCA_REGISTER_FAIL;
	gcc_er_log( &tmp_status,
		    (CL_ERR_DESC *)NULL,
		    (GCC_MDE *)NULL,
		    (GCC_ERLIST *)NULL );
	*generic_status = rg_parms.gca_status;
	STRUCT_ASSIGN_MACRO(rg_parms.gca_os_status, *system_status);
	return FAIL;
    } /* end if */	

    /*
    ** Save the listen id in the global data area IIGCc_global.
    */
    if ( rg_parms.gca_listen_id != NULL )
	STncpy( IIGCc_global->gcc_lcl_id, rg_parms.gca_listen_id,
		 GCC_L_LCL_ID );

    gcc_er_init( ERx( "IIGCB" ), IIGCc_global->gcc_lcl_id );

    STprintf(buffer, "\nGCB Server = %s\n", IIGCc_global->gcc_lcl_id);
    SIstd_write(SI_STD_OUT, buffer);

    /*
    ** Invoke the AL FSM to initialize it and establish an outstanding
    ** GCA_LISTEN.
    */
    mde = gcc_add_mde( ( GCC_CCB * ) NULL, 0 );
    ccb = gcc_add_ccb();

    if ( ! mde  ||  ! ccb )
	return_code = FAIL;
    else
    {
	mde->ccb = ccb;
	mde->service_primitive = A_LISTEN;
	mde->primitive_type = RQS;
	ccb->ccb_aae.a_state = SBACLD;
	if ( gcb_al(mde) != OK )  return_code = FAIL;
    }

    return( return_code );

}  /* end gcb_alinit */


/*
** Name: gcb_alterm	-  Bridge Application Layer termination routine.
**
** Description:
**      gcb_alterm is invoked on Bridge server shutdown.  It cleans up 
**      the PBAL and returns.  The following functions are performed: 
** 
**      - Issue GCA_TERMINATE to clean up GCA.
**
** Inputs:
**      None
**
** Outputs:
**      generic_status	    Mainline status code for initialization error.
**	system_status	    System_specific status code.
**
**	Returns:
**	    OK
**	    FAIL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Jan-96 (rajus01)
**          Initial routine creation. Culled from gcc_al.c
*/
STATUS
gcb_alterm
(
    STATUS 		*generic_status,
    CL_ERR_DESC 	*system_status
)
{
    GCA_TE_PARMS        te_parms;
    STATUS              call_status;
    STATUS              status;
    STATUS              return_code = OK;

    *generic_status = OK;

    /*
    ** Invoke GCA_TERMINATE service to terminate use of GCA.
    */

    IIGCc_global->gcc_flags |= GCC_STOP;
    call_status = IIGCa_call(	GCA_TERMINATE, 
				(GCA_PARMLIST *)&te_parms, 
				(i4) GCA_SYNC_FLAG,	/* Synchronous */
				(PTR) 0,		/* async id */
				(i4) -1,		/* no timeout */
				&status );

    /*
    ** If there is a failure as indicated by returned status, fill in the
    ** generic_status and system_status codes and return.
    */

    if( te_parms.gca_status != OK )
    {
	*generic_status = te_parms.gca_status;
	STRUCT_ASSIGN_MACRO( te_parms.gca_os_status, *system_status );
	return_code = FAIL;
    } /* end if */

    /*
    ** Termination is complete.  Return to caller.
    */

    return( return_code );

}  /* end gcb_alterm */


/*
** Name: gcb_gca_exit	- Asynchronous GCA service exit handler
**
** Description:
**      gcb_gca_exit is the first level exit handler invoked for completion 
**      of asynchronous GCA service requests.  It identifies the service 
**	and the association from the identifier passed to it by GCA.  
**	
**	The MDE is filled in from the GCA parm list as appropriate for the
**	particular GCA service which completed.  In some cases it may be
**	necessary to transform the MDE to a different SDU.  This would
**	typically be the case when a failure associated with the async event
**	has occurred, e.g. failure of a GCA service request.
**
** Inputs:
**      async_id                        The MDE associated with the request,
**					casted to a i4 for GCA.
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
**	    none
**
** History:
**      21-Jan-96 (rajus01)
**          Initial routine creation. Culled from gccal.c 
**      20-Jun-96 (rajus01)
**	    Don't post LISTEN again, when GCC_A_DISASSOC request
**	    completes. This caused problems on VMS platform.
*/
VOID
gcb_gca_exit( PTR async_id )
{
    i4                  cei;
    i4                  gca_rq_type;
    GCC_MDE             *mde;
    GCC_CCB             *ccb;
    STATUS		status = OK;
    STATUS		call_status;
    STATUS		generic_status;
    STATUS              tmp_status;

    /*
    ** Get the CCB, and the MDE.
    */

    mde = (GCC_MDE *)async_id;
    gca_rq_type = mde->gca_rq_type;
    ccb = mde->ccb;
    cei = ccb->ccb_hdr.lcl_cei;

    /*
    ** Perform the processing appropriate to the completed service 
    ** request.
    */

    /* do tracing... */
    GCX_TRACER_2_MACRO("gcb_gca_exit>", mde,
	"primitive = %d, cei = %d", gca_rq_type, cei);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
    {
	TRdisplay( "----- DOWN\n" );
	TRdisplay( "gcb_gca_exit: primitive %s cei %d\n",
		gcx_look( gcx_gca_rq, gca_rq_type ),
                cei );
    }
# endif /* GCXDEBUG */

    switch( gca_rq_type )
    {
    	case GCC_A_LISTEN:
    	{
	    GCA_LS_PARMS	    *ls_parms =
	      			 (GCA_LS_PARMS *) &mde->mde_act_parms.gca_parms;
    	    GCA_AUX_DATA	    *aux_data;
    	    GCC_MDE 	            *lsn_mde;
    	    GCC_CCB	    	    *lsn_ccb;

            /* check to make sure request has completed */
            if( ls_parms->gca_status == E_GCFFFE_INCOMPLETE )
	    {
# ifdef GCXDEBUG
            	if( IIGCc_global->gcc_trace_level >= 1 )
            	{
   	            TRdisplay(
			  "gcb_gca_exit: resuming GCA_LISTEN operation...\n");
                }
# endif /* GCXDEBUG */

                /* call again... */
                call_status = IIGCa_call( GCA_LISTEN, 
				          (GCA_PARMLIST *)ls_parms,
				          (i4) GCA_ASYNC_FLAG|GCA_RESUME,
                                                            /* Asynchronous */
				          (PTR)mde,   	    /* async id */
				          (i4) -1,	    /* no timeout */
				          &status );

    	    	/* 
    	    	** Check whether listen has yet to be resumed. Note: the check
    	    	** is done _after_ the call to avoid unneeded work in the case
    	    	** where GCA_LISTEN repeatedly completes synchronously. In that
    	    	** case, there is no need to repost the listen until such time
    	    	** as we unwind back to the return from the last resumption of
    	    	** the call.
    	    	*/
    	    	if( ! (ccb->ccb_aae.a_flags & GCBAL_LSNRES ) )
	    	{
		    ccb->ccb_aae.a_flags |= GCBAL_LSNRES;

    	    	    /*
    	    	    ** Unless we're at OBMAX, invoke the AL FSM to establish
    	    	    ** another outstanding GCA_LISTEN.
    	    	    */

    	    	    if( ! ( IIGCc_global->gcc_flags & GCC_TRANSIENT ) )
		    {
		    	lsn_mde = gcc_add_mde( ( GCC_CCB *) NULL, 0 );
    	    	    	lsn_ccb = gcc_add_ccb();
    	    	    	if( ( lsn_mde == NULL ) || ( lsn_ccb == NULL ) )
    	    	    	{
 	            	    /* Log an error and abort the association */
		    	    generic_status = E_GC2004_ALLOCN_FAIL;
	            	    gcb_al_abort( lsn_mde, lsn_ccb, &generic_status, 
    	    	    	    	    (CL_ERR_DESC *) NULL, (GCC_ERLIST *) NULL );
    	    	        } /* end if */
    	    	    	else
		    	{
    	    	    	    lsn_mde->ccb = lsn_ccb;
    	    	    	    lsn_mde->service_primitive = A_LISTEN;
    	    	    	    lsn_mde->primitive_type = RQS;
    	    	    	    lsn_ccb->ccb_aae.a_state = SBACLD;
    	    	    	    (VOID) gcb_al( lsn_mde );
		        } /* end of else */
		    }
	        }

	        /*
	        ** If there is a failure on the GCA_LISTEN resumption, log
	        ** an error and abort the association.
	        */

	        if (call_status != OK)
	        {
		    generic_status = status;
	            gcb_al_abort( mde, ccb, &generic_status,
				  (CL_ERR_DESC *) NULL,
			     	  (GCC_ERLIST *) NULL);
	        }
# ifdef GCXDEBUG
	        if( IIGCc_global->gcc_trace_level >= 1 )
		    TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
                return;
            }

            /* 
            ** Store the assoc id in the CCB regardless of whether there's an 
            ** error. We need to do this 'cos even on the error case we 
            ** subsequently do a disassociate on the association.
            */
	    ccb->ccb_aae.assoc_id = ls_parms->gca_assoc_id;

	    if (ls_parms->gca_status == OK)
	    {

	    	/*
	    	** determine from the aux_data type indication what sort of 
		** listen completion this is.  The options are the following:
	    	**
	    	**    GCA_ID_SHUTDOWN - This is a "Shut down now" datagram.
	    	**    GCA_ID_QUIESCE  - This is a "Quiesce, then shut down"
	    	**			datagram.
	        */

	    	aux_data = (GCA_AUX_DATA *)ls_parms->gca_aux_data;
	    	switch( aux_data->type_aux_data )
	    	{
	    	    case GCA_ID_SHUTDOWN:
	    	    {
			mde->service_primitive = A_SHUT;
                	mde->mde_generic_status[0] = E_GC0040_CS_OK;
			break;
	    	    }
	    	    case GCA_ID_QUIESCE:
	    	    {
			mde->service_primitive = A_QUIESCE;
                	mde->mde_generic_status[0] = E_GC0040_CS_OK;
			break;
	    	    }
	    	    default:
		    {
                    	/* unrecognized aux data type */
                    	status = FAIL;
		    	break;
		    }
	        } /* end switch */
	        if( status != OK )
	        {
		    /*
		    ** The listen processing has failed.  Log out the failure
		    ** and set the primitive to A_I_REJECT request.
		    */

		    tmp_status = E_GC2216_GCA_LSN_FAIL;
		    gcc_er_log( &tmp_status,
			   	(CL_ERR_DESC *)NULL,
			   	(GCC_MDE *)NULL,
			   	(GCC_ERLIST *)NULL);
		    mde->service_primitive = A_I_REJECT;
		    mde->primitive_type = RQS;
                    mde->mde_generic_status[0] = tmp_status;
	    	} /* end if */
	    } /* end if */
	    else
	    {
	    	/*
	    	** The listen has failed.  Log out the failure and set the
		** primitive to A_LSNCLNUP request.
	    	*/

	    	if( ls_parms->gca_status != E_GC0032_NO_PEER )
		    gcc_er_log( &ls_parms->gca_status,
			    	&ls_parms->gca_os_status,
			    	mde,
			    	(GCC_ERLIST *)NULL );
	    	    mde->service_primitive = A_LSNCLNUP;
	            mde->primitive_type = RQS;
	    } /* end else */				      

	    break;
        } /* end case GCC_A_LISTEN */
    	case GCC_A_RESPONSE:
    	{
            GCA_RR_PARMS        *rr_parms =
                                (GCA_RR_PARMS *) &mde->mde_act_parms.gca_parms;

            if( rr_parms->gca_status != OK )
            {
                /* The GCA_RQRESP failed. Construct an A_ABORT SDU in the MDE
		** for the failed request. Fill in the status elements
		** returned from the GCA service request in the user data area 
		** of the SDU.
            	*/

	        mde->service_primitive = A_ABORT;
	        mde->primitive_type = RQS;
	        tmp_status = E_GC2217_GCA_RQRESP_FAIL;
	        gcc_er_log( &tmp_status,
		            (CL_ERR_DESC *)NULL,
		            mde,
		            (GCC_ERLIST *)NULL);
	        gcc_er_log( &rr_parms->gca_status,
			    &rr_parms->gca_os_status,
			    mde,
			    (GCC_ERLIST *)NULL );
            }

            break;
        } /* end case GCC_A_RESPONSE */
        case GCC_A_DISASSOC:
        {
	    GCA_DA_PARMS	*da_parms =
                             (GCA_DA_PARMS *) &mde->mde_act_parms.gca_parms;
    	    GCC_MDE             *lsn_mde;
    	    GCC_CCB	    	*lsn_ccb;

            /* check to make sure request has completed */
            if (da_parms->gca_status == E_GCFFFE_INCOMPLETE)
	    {
# ifdef GCXDEBUG
                if( IIGCc_global->gcc_trace_level >= 1 )
                {
   	            TRdisplay(
		       "gcb_gca_exit: resuming GCA_DISASSOC operation...\n");
                }
# endif /* GCXDEBUG */

                /* call again... */
                call_status = IIGCa_call( GCA_DISASSOC,
				     	  (GCA_PARMLIST *)da_parms,
				     	  (i4) GCA_ASYNC_FLAG|GCA_RESUME,
                                                            /* Asynchronous */
				     	  (PTR)mde,    	    /* async id */
				          (i4) GCC_TIMEOUT,
				          &status );
	        /*
	        ** If there was a failure on the GCA_DISASSOC resumption, log
	        ** an error and abort the association.
	        */

	        if (call_status != OK)
	        {
		    generic_status = status;
	            gcb_al_abort( mde, ccb, &generic_status,
				  (CL_ERR_DESC *) NULL,
			          (GCC_ERLIST *) NULL);
	        }
# ifdef GCXDEBUG
	        if( IIGCc_global->gcc_trace_level >= 1 )
		    TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */
                return;
            }

            if (da_parms->gca_status != OK)
            {
            	/* The GCA_DISASSOC failed. There's not much we can do
		** about this other than log the error and carry on as though
		** nothing had happened...
                */

	    	tmp_status = E_GC2218_GCA_DISASSOC_FAIL;
	    	gcc_er_log( &tmp_status,
		       	    (CL_ERR_DESC *)NULL,
		            mde,
		            (GCC_ERLIST *)NULL );
	    	gcc_er_log( &da_parms->gca_status,
		            &da_parms->gca_os_status,
		            mde,
		            (GCC_ERLIST *)NULL );
            }
            else
	    {

    	    	/* Log session completion */
	
	    	tmp_status = E_GC2201_ASSOC_END;
	    	gcc_er_log( &tmp_status,
		       	    (CL_ERR_DESC *)NULL,
		       	    mde,
		       	    (GCC_ERLIST *)NULL );
	    }
            break;
	} /* end case GCC_A_DISASSOC */
	default:
	{
	    /*
	    ** Unrecognized GCA operation.  This is a serious program bug, of
	    ** the type that normally occurs only in early development.  However
	    ** if it ever did happen, there's really nothing to be done about
	    ** it.
	    ** All we can do is log it out and abort the association.
	    */

	    mde->service_primitive = A_ABORT;
	    mde->primitive_type = RQS;
	    tmp_status = E_GC220F_AL_INTERNAL_ERROR;
	    gcc_er_log(&tmp_status,
		       (CL_ERR_DESC *)NULL,
		       mde,
		       (GCC_ERLIST *)NULL);
	    break;
	} /* end default */
    }/* end switch */

    /*
    ** Send the MDE to AL.
    */

    (VOID) gcb_al( mde );
# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 1 )
	TRdisplay( "----- DOWN-END\n" );
# endif /* GCXDEBUG */

    return;

} /* end gcb_gca_exit */


/*{
** Name: gcb_al_evinp	- PBAL FSM input event id evaluation routine
**
** Description:
**      gcb_al_evinp accepts as input an MDE received by the PBAL.  It  
**      evaluates the input event based on the MDE type and possibly on  
**      some other status information.  It returns as its function value the  
**      ID of the input event to be used by the FSM engine. 
** 
** Inputs:
**      mde			Pointer to dequeued Message Data Element
**
** Outputs:
**      generic_status		Mainline status code.
**
**	Returns:
**	    u_i4	input_event_id
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      22-Jan-96 (rajus01)
**          Initial function implementation. Culled from gccal.c
*/
static u_i4
gcb_al_evinp
( 
    GCC_MDE 		*mde,
    STATUS 		*generic_status 
)
{
    u_i4		input_event;

    /* do tracing... */
    GCX_TRACER_3_MACRO("gcb_al_evinp>", mde,
	"primitive = %d/%d, size = %d",
	    mde->service_primitive,
	    mde->primitive_type,
	    mde->p2 - mde->p1 );

    switch( mde->service_primitive )
    {
    	case A_LISTEN:
    	{
	    /* Initial event */
	    input_event = IBALSN;
	    break;
    	}
    	case A_ASSOCIATE:
    	{
	    if( mde->primitive_type == RQS )
	    {
	    	/* A_ASSOCIATE request */	    
            	input_event = IBALSN;
	    }
	    break;
    	}  /* end case A_ASSOCIATE */
    	case A_SHUT:
    	{
	    input_event = IBASHT;
	    break;
    	}
    	case A_QUIESCE:
    	{
	    input_event = IBAQSC;
	    break;
        }
        case A_ABORT:
        {
            input_event = IBAABQ;
	    break;
        }
        case A_LSNCLNUP:
        {
	    /*
	    ** The local listen has failed.  
	    */

	    input_event = IBALSF;
	    break;
        }
    	case A_I_REJECT:
    	{
	    /*
	    ** The IAL listen processing has failed.
	    */

	    input_event = IBAREJI;
	    break;
    	}
    	case A_P_ABORT:
    	{
	    input_event = IBAABT;
	    break;
        }
    	case A_COMPLETE:
    	{
            input_event = IBAACE;
	    break;
    	}  /* end case A_COMPLETE */
    	case A_RELEASE:
    	{
	    input_event = IBAARQ;
	    break;
    	}
    	case A_FINISH:
    	{
	    /* A_FINISH has only a request form */
	 
	    input_event = IBAAFN;
	    break;
    	} /* end case A_FINISH */
    	default:
    	{
	    input_event = IBAMAX;  /* an illegal event ... */
	    *generic_status = E_GC2A06_PB_FSM_STATE;
	    break;
    	}
    }  /* end switch */

    return( input_event );

} /* end gcb_al_evinp */


/*
** Name: gcb_alout_exec	- PBAL FSM output event execution routine
**
** Description:
**	gcb_alout_exec accepts as input an output event identifier.  It
**	executes this output event and returns.
**
**	It also accepts as input a pointer to an MDE or NULL.  If the pointer
**	is non-null, the MDE pointed to is used for the output event.
**	Otherwise, a new one is obtained.
** 
** Inputs:
**	output_event_id		Identifier of the output event
**      mde			Pointer to dequeued Message Data Element
**
**	Returns:
**	    STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      21-Jan-96 (rajus01)
**          Initial function implementation
*/
static STATUS
gcb_alout_exec
(
    i4  	output_event_id,
    GCC_MDE 	*mde, 
    GCC_CCB 	*ccb
)
{
    STATUS	status;
    STATUS	call_status = OK;

    GCX_TRACER_1_MACRO("gcb_alout_exec>", mde,
	    "output_event = %d", output_event_id);

# ifdef GCXDEBUG
    if( IIGCc_global->gcc_trace_level >= 2 )
	TRdisplay( "gcb_alout_exec: service_primitive %s %s\n",
		gcx_look( gcx_mde_service, mde->service_primitive ),
                gcx_look( gcx_mde_primitive, mde->primitive_type ));
# endif /* GCXDEBUG */

    return( call_status );

} /* end gcb_alout_exec */


/*
** Name: gcb_alactn_exec	- PBAL FSM action execution routine
**
** Description:
**      gcb_alactn_exec accepts as input an action identifier.  It
**      executes the action correspoinding to the identifier and returns.
**
**	It also accepts as input a pointer to an 
**	MDE or NULL.  If the pointer is non-null, the MDE pointed to is
**	used for the action.  Otherwise, a new one is obtained.
**
** Inputs:
**	action_code		Identifier of the action to be performed
**      mde			Pointer to dequeued Message Data Element
**
**	Returns:
**	    STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**      21-Jan-96 (rajus01)
**          Initial function implementation
*/
static STATUS
gcb_alactn_exec
( 
    u_i4 		action_code,
    GCC_MDE 		*mde,
    GCC_CCB 		*ccb
)
{
    STATUS		status = OK;
    STATUS		call_status = OK;

    status = OK;

    GCX_TRACER_1_MACRO("gcb_alactn_exec>", mde, "action = %d", action_code);

# ifdef GCXDEBUG
        if( IIGCc_global->gcc_trace_level >= 1 )
	    TRdisplay( "gcb_alactn_exec: action %s\n",
			gcx_look( gcx_gcc_apb, action_code ) );
# endif /* GCXDEBUG */
    switch( action_code )
    {
    	case ABARSP:	/* GCA_RQRESP */
    	{
	    GCA_RR_PARMS	*rr_parms;
            AL_AARE_PCI     	aare_pci;
            u_i2            	req_result;
            STATUS          	temp_gnrc_status;
    	    char		*pci;
            bool                log;

	    /*
	    ** Issue GCA_RQRESP to association initiator. The MDE may be:
            **
            ** THE PCI and the user data areas of the MDE contain parameters for
            ** the call to GCA_RQRESP. The MDE becomes an A_COMPLETE RQS when
	    ** the GCA_RQRESP is complete.
            **
            ** (v) an A_SHUT, A_QUIESCE or A_I_REJECT which are local errors
            ** with a generic error in the mde header.
            **
            ** Switch on the primitive type to see what application layer 
	    ** version our peer is at. 
	    */

            log = FALSE;
            switch( mde->service_primitive )
            {
        	case A_SHUT:
        	case A_QUIESCE:
        	case A_I_REJECT:
        	{
            	    /* Take the error out of the mde header */
            	    temp_gnrc_status = mde->mde_generic_status[0];
            	    break;
        	} /* end cases A_SHUT, A_QUIESCE and A_I_REJECT */

        	default:
        	{
            	    break;
		} /* end default */
            } /* end switch */

            /* See whether we need to log an error */
            if( ( log ) && ( temp_gnrc_status != OK ) )
	    {
            	gcc_er_log( &temp_gnrc_status,
                       	    (CL_ERR_DESC *) NULL,
                            mde,
                            (GCC_ERLIST *) NULL );
            }

	    /* Strip out the log level */
	    temp_gnrc_status = GCC_LGLVL_STRIP_MACRO(temp_gnrc_status);

            /* Allocate the response parameter structure */

            /* fill in the parameter list */

	    rr_parms = (GCA_RR_PARMS *)&mde->mde_act_parms.gca_parms;
            rr_parms->gca_assoc_id = ccb->ccb_aae.assoc_id;
            rr_parms->gca_local_protocol = ccb->ccb_hdr.gca_protocol;
            rr_parms->gca_request_status = temp_gnrc_status;
            rr_parms->gca_modifiers = 0;

            /*
	    ** Invoke GCA_RQRESP service.  
	    */

            mde->service_primitive = A_COMPLETE;
            mde->primitive_type = RQS;
	    mde->gca_rq_type = GCC_A_RESPONSE;

            call_status = IIGCa_call( GCA_RQRESP, 
				 (GCA_PARMLIST *)rr_parms,
				 (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				 (PTR)mde,    	    	/* async id */
				 (i4) -1,		/* no timeout */
				 &status );

	    /*
	    ** If there is a failure as indicated by returned status, log 
	    ** an error and abort the association.
	    */

	    if( call_status != OK )
	    {
            	/* Log an error and abort the association */
	    	gcb_al_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			     (GCC_ERLIST *) NULL );
	    	status = FAIL;
	    	break;
	    }
	    break;
    	} /* end case ABARSP */
    	case ABADIS:	/* GCA_DISASSOC */
    	{
            GCC_MDE         	*da_mde;
	    GCA_DA_PARMS	*dao_parms;

	    /* If we're stopping the server, don't do anything */
	    if( IIGCc_global->gcc_flags & GCC_STOP )
	    	break;

	    /*
	    ** Issue GCA_DISASSOC to clean up the local association.
            **
            ** Allocate a new MDE for the disassociate operation.
	    */
  	    da_mde = gcc_add_mde( ccb, 0 );
	    if( da_mde == NULL )
	    {
                /* Log an error and abort the association */
                status = E_GC2004_ALLOCN_FAIL;
	        gcb_al_abort( da_mde, ccb, &status, (CL_ERR_DESC *) NULL,
	    		 (GCC_ERLIST *) NULL );
	        status = FAIL;
	        break;
	    } /* end if */


            da_mde->ccb = ccb;
	    da_mde->service_primitive = A_FINISH;
	    da_mde->primitive_type = RQS;
	    da_mde->gca_rq_type = GCC_A_DISASSOC;

	    dao_parms = (GCA_DA_PARMS *)&da_mde->mde_act_parms.gca_parms;
	    dao_parms->gca_association_id = ccb->ccb_aae.assoc_id;

	    call_status = IIGCa_call( GCA_DISASSOC, 
				      (GCA_PARMLIST *)dao_parms,
				      (i4) GCA_ASYNC_FLAG,   /* Asynchronous */
				      (PTR)da_mde,	      /* async id */
				      (i4) GCC_TIMEOUT,  
				      &status );

	    /*
	    ** If there is a failure as indicated by returned status, log
	    ** an error and abort the association.
	    */

	    if( call_status != OK )
	    {
	        /* Log an error and abort the association */
	        gcb_al_abort( da_mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL );
	        status = FAIL;
	    }
	    break;
        } /* end case AADIS */
        case ABALSN:	/* GCA_LISTEN */
        {
    	    GCA_LS_PARMS    *ls_parms;
	    i4		i;

	    /*
	    ** Issue GCA_LISTEN to establish an open listen to allow clients 
	    ** to invoke comm server services.
	    */

    	    ccb->ccb_hdr.async_cnt += 1;

            /* Set the application layer protocol version number */

            ccb->ccb_aae.a_version = PBAL_PROTOCOL_VRSN_12;

	    mde->service_primitive = A_ASSOCIATE;
	    mde->primitive_type = RQS;
	    mde->p1 = mde->p2 = mde->papp;
	    mde->gca_rq_type = GCC_A_LISTEN;

	    /*
	    ** Invoke GCA_LISTEN service. 
	    */

            ls_parms = (GCA_LS_PARMS *)&mde->mde_act_parms.gca_parms;

    	    call_status = IIGCa_call( GCA_LISTEN, 
				      (GCA_PARMLIST *)ls_parms,
				      (i4) GCA_ASYNC_FLAG,  /* Asynchronous */
				      (PTR)mde,		     /* async id */
				      (i4) -1,	     /* no timeout */
				      &status );

	    /*
	    ** If there is a failure as indicated by returned status, log
            ** an error and abort ther association.
	    */

	    if( call_status != OK )
	    {
                /* Log an error and abort the association */
	        gcb_al_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
	    		      (GCC_ERLIST *) NULL );
	        status = FAIL;
	        break;
	    }
	    break;
        } /* end case AALSN */
        case ABAMDE:	/* Return the MDE */
        {
	    /*
	    ** Return the MDE to the scrapyard.
	    */
	    gcc_del_mde( mde );
	    break;
        } /* end case ABAMDE */
        case ABACCB:	/* Return the CCB */
        {
	    /* Discard the CCB pointed to by the input MDE */

    	    /* Decrement and check the CCB use count. If 0, the CCB is clean.  
    	    ** Release it. Otherwise, it is still in use, so let the other
	    ** user(s) release it.  This is to prevent a race condition
	    ** releasing the CCB when an association is complete. 
	    */

	    if( --ccb->ccb_hdr.async_cnt <= 0 )
	    {
	        gcc_del_ccb( ccb );

	        /*
	        ** Check for Bridge Server in "quiescing" state.  If so,
		** and this is the last association, shut the Bridge Server
		** down.
	        */

# ifdef GCXDEBUG
	    	if( IIGCc_global->gcc_trace_level >= 1 )
	    	    TRdisplay( "gcb_alactn_exec: ob conn ct %d conn ct %d\n",
		                IIGCc_global->gcc_ob_conn_ct,
		                IIGCc_global->gcc_conn_ct );
# endif /* GCXDEBUG */

	        if( ! IIGCc_global->gcc_conn_ct && 
	    	     IIGCc_global->gcc_flags & GCC_QUIESCE )
		        GCshut();
	    }
	    break;
        } /* end case ABACCB */
        case ABASHT:	/* Shut down the Comm Server */
        {
	    GCshut();
	    break;
        } /* end case AASHT */
        case ABAQSC:	/* Quiesce the Comm Server */
        {
            /* 
            ** If there are no extant associations we can shut down the server
            ** straight away as in the shutdown case. Otherwise we set the
	    ** quiesce flag and wait for the last association to bite the
	    ** dust...
            */
            if( IIGCc_global->gcc_conn_ct == 0 )
	    {
  	        GCshut();
	    }
    	    else
	    {
    	        IIGCc_global->gcc_flags |= GCC_QUIESCE;
	    }
	    break;
        } /* end case ABAQSC */
        default:
	{
	    /*
	    ** Unknown action event ID.  This is a program bug.
	    ** Log an error and abort the association 
            */
	    status = E_GC220F_AL_INTERNAL_ERROR;
	    gcb_al_abort( mde, ccb, &status, (CL_ERR_DESC *) NULL,
			 (GCC_ERLIST *) NULL );
	    status = FAIL;
	    break;
	}
    } /* end switch */

    return( status );

} /* end gcb_alactn_exec */

/*
** Name: gcb_al_abort	- Bridge Application Layer internal abort
**
** Description:
**	gcb_al_abort handles the processing of internally generated aborts.
** 
** Inputs:
**      mde		    Pointer to mde
**      ccb		    Pointer to ccb
**	generic_status      Mainline status code
**      system_status       System specfic status code
**      erlist		    Error parameter list
**
** Outputs:
**      none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Jan-96 (rajus01)
**          Initial routine creation. Culled from gccal.c 
*/
static VOID
gcb_al_abort
(
    GCC_MDE 		*mde,
    GCC_CCB 		*ccb, 
    STATUS 		*generic_status,
    CL_ERR_DESC 	*system_status,
    GCC_ERLIST 		*erlist
)
{
    /* Make sure there's actually a CCB */
    if( ccb == NULL )
    {
	/* Free up the mde and get out of here */
	gcc_del_mde( mde );
	return;
    }

    if( ! ( ccb->ccb_aae.a_flags & GCCPB_ABT ) )
    {
	/* Log an error */
	gcc_er_log( generic_status, system_status, mde, erlist );

	/* Set abort flag so we don't get in an endless aborting loop */
        ccb->ccb_aae.a_flags |= GCCPB_ABT;    

        /* Issue the abort */
    	if( mde != NULL )
    	{
    	    mde->service_primitive = A_P_ABORT;
	    mde->primitive_type = RQS;
    	    mde->p1 = mde->p2 = mde->papp;
            (VOID) gcb_al( mde );
    	}
    }

}  /* end gcb_al_abort */

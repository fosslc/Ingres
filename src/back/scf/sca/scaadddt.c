/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <sl.h>
#include    <pc.h>
#include    <cs.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>
#include    <ci.h>

#include    <dbms.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <adf.h>
#include    <add.h>
#include    <adp.h>
#include    <adfops.h>	    /* For debug/test */
#include    <dmf.h>
#include    <dmccb.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>
#include    <scf.h>
#include    <qsf.h>

/* added for scs.h prototypes, ugh! */
#include <ulm.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <sc0m.h>
#include    <scfudt.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <sc0e.h>

/**
**
**  Name: SCAADDDT.C - Add new datatypes to the DBMS Server system
**
**  Description:
**
**	This file contains the routine to add new datatypes to the INGRES system
**
**          sca_add_datatypes() - Add datatype to the server.
**          sca_check() - Check that the proposed configuration matches the
**			    known one.
**	    sca_obtain_id() - obtain major/minor id for current datatype system.
**
**
**  History:
**      01-Feb-89 (fred)
**          Created.
**      07-jun-89 (fred)
**          Removed message about nonuse of User ADT's in default case.  This
**	    will now come out only as an error.  Other developers seemed to find
**	    it annoying;  furthermore, at the time that this code is run, DMF
**	    hasn't been initialized yet.  Consequently, SCF doesn't know the
**	    node name, etc, so it hasn't called ule_initiate().  Thus, any
**	    message which comes out of here lacks `normal' formatting -- this
**	    cannot be helped, but it is confusing.
**	22-jun-89 (jrb)
**	    Added check to ensure site is licensed to add dts/ops/funcs.
**	01-Feb-1990 (greg)
**	    Change args from major, minor to major_id, minor_id for Pyramid
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    29-Mar-90 (alan)
**		Don't require CI_USER_ADT authorization if RMS Gateway.
**	    10-apr-1990 (fred)
**		Added user tracing.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**	12-Mar-1993 (daveb)
**	    Add include <st.h>
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    include <tr.h>, correctly cast arg to CSget_scb.
**	30-jun-1993 (shailaja)
**	    Cast parameters to match prototypes.
**	2-Jul-1993 (daveb)
**	    remove unused variable adf_cb, remove func externs now
**	    in <scfudt.h>
**       6-Jul-1993 (fred)
**          Added function pointers to ome registration call to
**          support better error handling and large object
**          functionality. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      26-Jul-1993 (fred)
**          Fixed up ADD_CALLBACKS initialization to be type correct.
**          Problem in mechanism for some prototypes due to differences
**          header inclusions...
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**      20-sep-1993 (stevet)
**          Changed sca_adddatatype to support INGRES class object.  
**      10-nov-1993 (stevet)
**          deallocate memory the second time not working when loading class
**          library and udt together.
**	16-dec-1994 (wonst02)
**	    Suppress msgs E_SC0263, SC0265, SC024D & SC0268 in non-servers,
**	    e.g., ckpdb, infodb, etc.  (bug# 63990)
**	04-Jan-2001 (jenjo02)
**	    Pass CS_SCB* in scf_session to scf_call() rather than
**	    DB_NOSESSION.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Add sc0e.h include to pick up function prototypes.
**	    Use new form sc0ePut.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**/

/*
**  Forward and/or External function references.
*/


/*
** Definition of all global variables used by this file.
*/

GLOBALREF SC_MAIN_CB     *Sc_main_cb;	 /* Core structure for SCF */

/*{
** Name: sca_trace	- Add TRACE capability for user defined datatypes
**
** Description:
**      This routine disposes of a trace message provided by the user defined
**	datatype code.  This allows users to send messages to either the
**	frontend program, the II_DBMS_LOG file, or the system error log. 
**
** Inputs:
**      dispose_mask                    A bitmask indicating where the message
**					should be sent.  Bit values are
**					    ADD_FRONTEND_MASK
**					    ADD_TRACE_FILE_MASK
**					    ADD_ERROR_LOG_MASK
**      length                          Length of the message
**      message                         Ptr to the message
**
** Outputs:
**      (None)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-Apr-1990 (fred)
**          Created.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/

DB_STATUS
sca_trace( i4  dispose_mask, i4  length, char *message )
{
    SCF_CB		*scf_cb;
    SCD_SCB		*scb;
    DB_STATUS		scf_status;

    if (    (dispose_mask &
		~(ADD_FRONTEND_MASK | ADD_ERROR_LOG_MASK | ADD_TRACE_FILE_MASK))
	||  !message
	||  (length <= 0)
	||  (length > ER_MAX_LEN))
    {
	return(E_DB_ERROR);
    }
	
    if ((dispose_mask & ADD_FRONTEND_MASK)
		&& (Sc_main_cb->sc_state == SC_OPERATIONAL))
    {
	CSget_scb((CS_SCB **)&scb);

	scf_cb = scb->scb_sscb.sscb_scccb;
	scf_cb->scf_length = sizeof(*scf_cb);
	scf_cb->scf_type = SCF_CB_TYPE;
	scf_cb->scf_facility = DB_PSF_ID;
	scf_cb->scf_len_union.scf_blength = (u_i4) length;
	scf_cb->scf_ptr_union.scf_buffer = message;
	scf_cb->scf_session = (SCF_SESSION)scb;  /* print to current session */

	scf_status = scf_call(SCC_TRACE, scf_cb);

	if (scf_status != E_DB_OK)
	{
	    TRdisplay(
		"SCA_TRACE: SCF error %d. displaying query text to user\n",
		scf_cb->scf_error.err_code);
	}
    }

    if ((dispose_mask & (ADD_TRACE_FILE_MASK | ADD_ERROR_LOG_MASK))
	    == ADD_TRACE_FILE_MASK)
    {
	TRdisplay("%t\n", length, message);
    }

    if (dispose_mask & ADD_ERROR_LOG_MASK)
    {
	sc0ePut(NULL, E_SC0318_SCA_USER_TRACE, 0, 1, length, message);
    }
    return(E_DB_OK);
}

/*{
** Name: scd_add_datatypes	- Add new datatypes, functions, and operators
**
** Description:
**      This routine adds new datatypes, functions, and/or operators to the DBMS
**	server.  It does this by
**	    1) Getting the ADD_DEFINITION block from IIudadt_register
**	    2) Ask ADF for the amount of space required to add these new options
**	    3) Allocate that space, and
**	    4) Add the new options to the system.
**
**	Failures are logged but not returned.  The system currently ignores
**	failures in finding new objects.
**
** Inputs:
**	scb				Session control block to use.  0 if not
**					available.
**	adf_svcb			Addr of old server control block.  NOTE:
**					IT IS ASSUMED THAT THIS MEMORY IS
**					SCU_MALLOC'D MEMORY.  IT WILL BE
**					DEALLOCATED IF THE ROUTINE IS
**					SUCCESSFUL AND DEALLOCATION IS
**					INDICATED.
**	adf_size			Size of old server control block.
**	deallocate_flag			Indicates whether the old memory should
**					be deallocated.  The deallocate flag, if
**					set, contains the facility id
**					(DB_??F_ID) of the facility to which the
**					memory is and was charged (which
**					allocated it...)  See note below.
**	error				Addr of error block to fill in.
**	new_svcb			Addr**2 of new server block.  This
**					routine, if successful will place a
**					pointer to the new adf server control
**					block here.
**	new_size			Pointer to i4  to be filled with number
**					of pages used by new_svcb.  Can be zero
**					if caller is not interested.
**
** Outputs:
**	*error				Filled with error if appropos
**	*new_svcb			Filled with pointer to new adf server
**					control block.  This field is always
**					set.  If no change is necessary, then
**					this field is set to the value of
**					adf_svcb.
**	*new_size			Filled with page count if non-zero.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	Sc_main_cb->sc_{major,minor}_adf are set to reflect the user adt major
**	and minor id's.  These are used later to check (sca_compatible())
**	whether the user defined ADT image is compatible with the remainder of
**	the running INGRES installation.
**
**	ADF is updated (assuming correctness) to run with new user defined
**	datatypes.
**
**	If so indicated (by deallocate_flag being non-zero), the old pages are
**	deallocated.
**
** History:
**      31-Jan-1989 (fred)
**          Created.
**      22-Mar-1989 (fred)
**          Added capability to scan IIDBDB for datatypes.
**      19-Apr-1989 (fred)
**	    Removed IIDBDB stuff.  All information now determined by function
**	    calls to the user.  Added support for returning major/minor id for
**	    user defined structure.
**	22-jun-89 (jrb)
**	    Added check to ensure site is licensed to add dts/ops/funcs.
**	09-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    29-mar-90 (alan)
**		Don't require CI_USER_ADT authorization if RMS Gateway.
**	2-Jul-1993 (daveb)
**	    remove unused variable adf_cb.
**	2-Jul-1993 (daveb)
**	    prototyped.
**       6-Jul-1993 (fred)
**          Added more user callable functions to the callback
**          structure. 
**      26-Jul-1993 (fred)
**          Fixed up ADD_CALLBACKS initialization to be type correct.
**          Problem in mechanism for some prototypes due to differences
**          header inclusions...
**      28-aug-1993 (stevet)
**          Added support for INGRES class objects.  The adu_agument()
**          are call twice now, onec for class objects and once for
**          UDT.
**      10-nov-1993 (stevet)
**          deallocate memory the second time not working when loading class
**          library and udt together.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototyped, SCD_SCB *scb now void *scbp - called from DMF.
[@history_template@]...
*/
DB_STATUS
sca_add_datatypes(void *scbp,
		  PTR adf_svcb,
		  i4  adf_size,
		  i4  deallocate_flag,
		  DB_ERROR *error,
		  PTR *new_svcb,
		  i4  *new_size )
{
    SCD_SCB		*scb = (SCD_SCB*)scbp;
    ADD_DEFINITION	*add_block;
    PTR                 new_adf_block = (PTR) 0;
    PTR			old_adf_block = (PTR) 0;
    DB_STATUS		status;
    DB_STATUS		int_status;
    i4                  old_size;
    i4                  cur_size;
    i4		size;
    SCF_CB		scf_cb;
    i4			l_ustring;
    i4                  i;
    char		*ustring;
        /*
        **  This structure is made static to protect against
        **  users who keep the pointer to it.  They are told not
        **  to, but...
        */
    static  ADD_CALLBACKS   callbacks = { ADD_T_V2,
					      sca_trace,
					      adu_ome_error,
					      (ADD_LO_HANDLER *)
					     		adu_peripheral,
					      (ADD_INIT_FILTER *)
							adu_0lo_setup_workspace,
					      (ADD_LO_FILTER *) 
							adu_lo_filter};

    status = E_DB_OK;
    CLRDBERR(error);

    *new_svcb = adf_svcb;   /* Start out with no change */
    old_size = adf_size;
    old_adf_block = adf_svcb;
    Sc_main_cb->sc_major_adf = ADD_INGRES_ORIGINAL;
    /* Loop 2 time so that we are load class obj as well as udt */
    for ( i = 0; i < 2; i++)
    {
	add_block = (ADD_DEFINITION *) 0;
	if( i)
	{
	    status = IIudadt_register( &add_block, &callbacks);
	}
	else
	{
	    status = IIclsadt_register( &add_block, &callbacks); 
	}

	if (status)
	{
	    SETDBERR(error, 0, E_SC026A_SCA_REGISTER_ERROR);
	    status = E_DB_ERROR;
	    break;
	}


	if (!add_block)
	{
	    /* No new datatypes for this register, try next */
    	    continue;
	}

	if ((add_block->add_risk_consistency == ADD_INCONSISTENT) &&
    	    (Sc_main_cb->sc_capabilities & (SC_INGRES_SERVER | SC_STAR_SERVER)))
	{
	    sc0ePut(NULL, E_SC0263_SCA_RISK_CONSISTENCY, NULL, 0);
	}

	if (add_block->add_major_id <= 0)
	{
	    sc0ePut(error, E_SC0264_SCA_ILLEGAL_MAJOR, NULL, 1,
			    sizeof(add_block->add_major_id),
			    &add_block->add_major_id);
	    break;
	}
	    
	/* Now, figure out the size necessary for the new ADF block */
	
	size = adg_sz_augment(add_block, error);
	if (error->err_code)
	    break;
	
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = (deallocate_flag ? deallocate_flag : DB_SCF_ID);
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages = ((size + SCU_MPAGESIZE - 1)
	    & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;

	status = scf_call(SCU_MALLOC, &scf_cb);

	if (status != OK)
	{
	    *error = scf_cb.scf_error;
	    break;
	}
	new_adf_block = (PTR) scf_cb.scf_scm.scm_addr;

	if (	(add_block->add_l_ustring && !add_block->add_ustring)
	   ||	(!add_block->add_l_ustring && add_block->add_ustring))
	{
	    /* This is an error -- note error in block */
	    l_ustring = STlength(ustring = "*** INVALID USER STRING or LENGTH ***");
	}
	else
	{
	    l_ustring = add_block->add_l_ustring;
	    ustring = add_block->add_ustring;
	}
    	if (Sc_main_cb->sc_capabilities & (SC_INGRES_SERVER | SC_STAR_SERVER))
	{
	    sc0ePut(NULL, E_SC0265_SCA_STATE, 0, 3,
				    sizeof(add_block->add_major_id),
				    &add_block->add_major_id,
				    sizeof(add_block->add_minor_id),
				    &add_block->add_minor_id,
				    l_ustring,
				    ustring);

	    sc0ePut(NULL, E_SC024D_SCA_ADDING, 0, 4,
		    sizeof(add_block->add_count),
			&add_block->add_count,
		    sizeof(add_block->add_dt_cnt),
			&add_block->add_dt_cnt,
		    sizeof(add_block->add_fo_cnt),
			&add_block->add_fo_cnt,
		    sizeof(add_block->add_fi_cnt),
			&add_block->add_fi_cnt);
	} /* if ... SC_INGRES_SERVER | SC_STAR_SERVER ... */

	status = adg_augment(add_block, size, new_adf_block, error);
	if (status && (error->err_code))
	    break;

	Sc_main_cb->sc_major_adf = add_block->add_major_id;
	Sc_main_cb->sc_minor_adf = add_block->add_minor_id;
	Sc_main_cb->sc_risk_inconsistency = add_block->add_risk_consistency;
	
	*new_svcb = new_adf_block;
	cur_size = scf_cb.scf_scm.scm_in_pages;
	if (new_size)
	    *new_size = scf_cb.scf_scm.scm_in_pages;

	if (old_adf_block && deallocate_flag)
	{
	    /* Remainder of control block setup above */
	
	    scf_cb.scf_scm.scm_in_pages = old_size;
	    scf_cb.scf_scm.scm_addr = old_adf_block;
	    int_status = scf_call(SCU_MFREE, &scf_cb);
	    if (int_status)
	    {
		sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
		sc0ePut(&scf_cb.scf_error, E_SC024C_SCA_DEALLOCATE, NULL, 0);
	    }
	}
	old_size = cur_size;
	old_adf_block = new_adf_block;
	new_adf_block = 0;
    }

    if (status && error->err_code)
    {
	sc0ePut(error, 0, NULL, 0);
	sc0ePut(error, E_SC024E_SCA_NOT_ADDED, NULL, 0);
    }

    if (new_adf_block)
    {
	int_status = scf_call(SCU_MFREE, &scf_cb);
	if (int_status)
	{
	    sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
	    sc0ePut(NULL, E_SC024C_SCA_DEALLOCATE, NULL, 0);
	}
    }

    if (status && add_block && (add_block->add_trace & ADD_T_FAIL_MASK))
    {
	sc0ePut(NULL, E_SC0266_SCA_USER_SHUTDOWN, NULL, 0);
	status = E_DB_FATAL;
    }
    else
    {
	status = E_DB_OK;
    }
    return(status);
}

/*{
** Name: sca_check	- Check compatibity with ADF shared images
**
** Description:
**      This routine determines whether the current UDADT code is compatible
**	with the operational INGRES installation.  Compatible is defined as
**	major ids being the same, and the minor id of the current code greater
**	than or equal to the operating installation.  Thus, if the operational
**	system is running with <major, minor> == <4,5>,
**
**	    <3,0>, <3,5>, <3,6>, <20,5>, and <4,4> are incompatible, but
**	    <4,5>, <4,6>, ..., <4, +infinitity> are compatible.
**
**	BUT.  If the user has set the risk inconsistency flag in their add
**	block, then compatibility is guaranteed.  This flag is provided for
**	users to debug their images, without having to kill & restart an entire
**	installation each time.
**
**	Users are expected to follow the rule that if they add anything, then
**	the major id should be incremented.  If they remove anything, then it is
**	permissible to bump only the minor id.  Thus, with these rules and this
**	compabibility model, it will be the case that the installation always
**	knows about at least as much "stuff" as any servers running within it.
**	And this is the desirable situation.
**
** Inputs:
**      major                           The major id of the installation.
**      minor                           The minor id of the installation.
**
** Outputs:
**	None.
**
**	Returns:
**	    i4  (Boolean) ==> 0 is incompatible, non-zero is compatible.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-Apr-1989 (fred)
**          Created.
**	01-Feb-1990 (greg)
**	    Change args from major, minor to major_id, minor_id for Pyramid
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
i4
sca_check(i4 major_id, i4 minor_id)
{
    i4		status = 0;

    if (Sc_main_cb && !Sc_main_cb->sc_major_adf)
    {
	Sc_main_cb->sc_major_adf = ADD_INGRES_ORIGINAL;
    }
    else if (!Sc_main_cb)
    {
	return(status);
    }

    if ((major_id == DMC_C_UNKNOWN) || (minor_id == DMC_C_UNKNOWN) || (!major_id))
    {
	TRdisplay("SCA_CHECK: Unable to determine RCP UDADT state\n");
	sc0ePut(NULL, E_SC0267_SCA_ID_UNKNOWN, 0, 0);
	major_id = ADD_INGRES_ORIGINAL;
	minor_id = 0;
    }
    if (	(Sc_main_cb->sc_major_adf == major_id)
	    &&	(Sc_main_cb->sc_minor_adf >= minor_id))
    {
	status = 1;
    }
    else if (Sc_main_cb->sc_risk_inconsistency)
    {
    	if (Sc_main_cb->sc_capabilities & (SC_INGRES_SERVER | SC_STAR_SERVER))
	    sc0ePut(NULL, E_SC0268_SCA_RISK_INVOKED, 0, 0);
	status = 1;
    }
    return(status);
}

/*{
** Name: sca_obtain_id	- Get current major/minor UDADT id settings
**
** Description:
**      This routine simply returns the major and minor id settings in force
**	at the time of the call.   
**
** Inputs:
**      major_id                        Ptr to i4 into which to place the
**					major id
**      minor_id                        Ditto for minor id
**
** Outputs:
**      *major_id                       UDADT major id at time of call
**      *minor_id                       Ditto for minor
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
**      21-apr-89 (fred)
**          Created.
[@history_template@]...
*/
VOID
sca_obtain_id( i4 *major_id, i4 *minor_id )
{

    if (Sc_main_cb)
    {
	if (Sc_main_cb->sc_major_adf)
	    *major_id = Sc_main_cb->sc_major_adf;
	else
	    *major_id = Sc_main_cb->sc_major_adf = ADD_INGRES_ORIGINAL;
	*minor_id = Sc_main_cb->sc_minor_adf;
    }
    else
    {
	*major_id = ADD_INGRES_ORIGINAL;
	*minor_id = 0;
    }
}


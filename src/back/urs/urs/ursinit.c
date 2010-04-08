/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dl.h>
#include    <er.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>

#include    <ursm.h>

/**
**
**  Name: URSINIT.C - Startup and shutdown of User Request Services
**
**  Description:
**      This file contains initialization and termination routines for the
**      User Request Services Manager of the Frontier Application Server.
**
**	    URS_STARTUP  - Start User Request Services and application servers
**	    URS_SHUTDOWN - Shut down User Request Services & application servers
**
**
**  History:
**      03-Nov-1997 wonst02
**          Original User Request Services Manager.
** 	11-Dec-1997 shero03
** 	    Allocate/free URS_facility and startup/shutdown the ULM storage pool.
** 	16-Dec-1997 wonst02
** 	    Allocate memory for the application server and application blocks.
** 	17-Mar-1998 wonst02
** 	    Add data dictionary services. Add missing "Interface" layer.
** 	29-may-1998 (wonst02)
** 	    Moved code from ursinit.c to ursrepos.c
**	11-Aug-1998 (fanra01)
**	    Fixed incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Global variables
*/

GLOBALREF   URS_MGR_CB		*Urs_mgr_cb;     /* URS Manager control block */


/*
**  Forward and/or External function references.
*/
static DB_STATUS
srvr_startup(URS_MGR_CB	*ursm,
	     URSB	*ursb);

static DB_STATUS
srvr_shutdown(URS_MGR_CB	*ursm,
	      URSB		*ursb);

static DB_STATUS
srvr_memory(URS_MGR_CB	*ursm,
	    URSB	*ursb);

static DB_STATUS
free_srvr_memory(URS_MGR_CB	*ursm,
		 URSB		*ursb);

static DB_STATUS
appl_startup(URS_MGR_CB	*ursm,
	     FAS_APPL	*appl,
	     URSB	*ursb);
static DB_STATUS
appl_shutdown(URS_MGR_CB	*ursm,
	      FAS_APPL		*appl,
	      URSB		*ursb);
static DB_STATUS
free_appl_memory(URS_MGR_CB	*ursm,
		 FAS_APPL	*appl,
		 URSB		*ursb);
DB_STATUS
intfc_startup(URS_MGR_CB	*ursm,
	      FAS_INTFC		*intfc,
	      URSB		*ursb);

/*{
** Name: URS_STARTUP - Start User Request Services and application servers
**
** Description:
**      Start the User Request Services Manager. Initialize every application.
** 
**	The information about the server and its applications, such
** 	as the name and path of the DLLs and the types (OpenRoad, C/C++), is
** 	retrieved from the repository (data dictionary). 
**
** Inputs:
** 	URSB				User Request Services Block
**
** Outputs:
** 
**	Returns:
** 		E_DB_OK			URSM started OK
** 		E_DB_WARN		URSM not specified to start up
** 		E_DB_ERROR		URSM could not start because of error:
** 					see ursb->ursb_error for info.
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	06-Nov-1997 wonst02
** 	    Original.
** 	11-Dec-1997 shero03
** 	    Allocate URS_facility and startup the ULM storage pool.
** 	31-Dec-1997 wonst02
** 	    Allocate memory for the application server and application blocks.
** 	17-Mar-1998 wonst02
** 	    Add data dictionary services.
*/
DB_STATUS
urs_startup(URSB		*ursb)
{
	SCF_CB		scf_cb ZERO_FILL;
	ULM_RCB		*ulm;
	DB_STATUS	status;

	if (Urs_mgr_cb)
	{
	    urs_error(E_UR0204_URS_INIT_ALREADY, URS_INTERR, 1,
	    	      sizeof Urs_mgr_cb, &Urs_mgr_cb);
	    SET_URSB_ERROR(ursb, E_UR0200_URS_INIT_ERROR, E_DB_ERROR)
	    return(E_DB_ERROR);
	}

	/* 
	** Allocate memory for the URS main control block
	*/
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_URS_ID;
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages = (sizeof(URS_MGR_CB)/SCU_MPAGESIZE+1);
	if ((status = scf_call(SCU_MALLOC, &scf_cb)) != E_DB_OK)
	{
	    urs_error(scf_cb.scf_error.err_code, URS_INTERR, 0,
	    	      0,0,0,0,0,0);
	    urs_error(E_UR0300_SCU_MALLOC_ERROR, URS_INTERR, 1,
	    	      sizeof scf_cb.scf_scm.scm_in_pages,
		      	  &scf_cb.scf_scm.scm_in_pages, 0,0,0,0);
	    if (scf_cb.scf_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_SEVERE)
	    else
	    	SET_URSB_ERROR(ursb, E_UR0200_URS_INIT_ERROR, E_DB_SEVERE)
	    return(E_DB_SEVERE);
	}

	Urs_mgr_cb = (URS_MGR_CB *)scf_cb.scf_scm.scm_addr;

	/*
	** Initialize the pool from which ULM streams will be allocated. 
	** The ULM_PRIVATE_STREAM flag means that calls to ulm routines 
	** for the memory stream are not protected by semaphore, which 
	** is OK if the memory is allocated once at startup and freed
	** at shutdown or otherwise protected against concurrent access.
	*/
	ulm = &Urs_mgr_cb->ursm_ulm_rcb;
	ulm->ulm_facility = DB_URS_ID;
	ulm->ulm_blocksize = SCU_MPAGESIZE;
	ulm->ulm_sizepool = (i4)64*1024;
	ulm->ulm_flags = ULM_PRIVATE_STREAM;
	status = ulm_startup(ulm);
	if (status != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0,
	    	      0,0,0,0,0,0);
	    urs_error(E_UR0310_ULM_STARTUP_ERROR, URS_INTERR, 1,
	    	      sizeof ulm->ulm_sizepool,
		      &ulm->ulm_sizepool, 0,0,0,0);
	    if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_SEVERE)
	    else
	    	SET_URSB_ERROR(ursb, E_UR0200_URS_INIT_ERROR, E_DB_SEVERE)
	    return(E_DB_SEVERE);
	}
	/*
	** Allocate a memory stream for the URSM
	*/
	ulm->ulm_flags     = ULM_PRIVATE_STREAM;
	ulm->ulm_memleft   = &Urs_mgr_cb->ursm_memleft;
	Urs_mgr_cb->ursm_memleft = URS_APPMEM_DEFAULT;
	if ((status = ulm_openstream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0312_ULM_OPENSTREAM_ERROR, URS_INTERR, 3,
	    	      sizeof(*ulm->ulm_memleft), ulm->ulm_memleft,
	    	      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
	    	      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize);
	    if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_SEVERE)
	    else
	    	SET_URSB_ERROR(ursb, E_UR0200_URS_INIT_ERROR, E_DB_SEVERE)
	    return E_DB_SEVERE;
	}

	Urs_mgr_cb->ursm_flags = ursb->ursb_flags;
	Urs_mgr_cb->ursm_srvr_id = 1;

	/*
	** Initialize the data dictionary services.
	*/
	if (!DB_FAILURE_MACRO(status)) 
	    status = urd_ddg_init(Urs_mgr_cb, ursb);

	/*
	** Start the application server and its applications
	*/
	if (status == E_DB_OK) 
	    status = srvr_startup(Urs_mgr_cb, ursb);

	if (DB_FAILURE_MACRO(status))
	{	
	    urd_ddg_term(Urs_mgr_cb, ursb);
	}

	return status;
}

/*{
** Name: URS_SHUTDOWN - Shut down User Request Services and application servers
**
** Description:
**      Terminate every application.
** 	Shut down the User Request Services Manager.
**
** Inputs:
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	06-Nov-1997 wonst02
** 	    Original.
** 	11-Dec-1997 shero03
** 	    Shutdown the ULM storage pool and free the URS_facility.
** 	17-Dec-1997 wonst02
** 	    Shut down the applications.
*/
DB_STATUS
urs_shutdown(URSB	*ursb)
{
	SCF_CB		scf_cb;
	DB_STATUS	status = E_DB_OK;
	bool		badstat = FALSE;

	if (!Urs_mgr_cb)
	    return E_DB_WARN;

	/*  Shut down the applications */
	status = srvr_shutdown(Urs_mgr_cb, ursb);
	if ( DB_FAILURE_MACRO(status) )
	    SET_URSB_ERROR(ursb, E_UR0130_SRVR_SHUTDOWN, status)

	urd_ddg_term(Urs_mgr_cb, ursb);

	/* get rid of the memory pool */
	if ((status = ulm_shutdown(&Urs_mgr_cb->ursm_ulm_rcb)) != E_DB_OK)
	    SET_URSB_ERROR(ursb, Urs_mgr_cb->ursm_ulm_rcb.ulm_error.err_code, 
	    		   status)

	/* deallocate Gwf_facility memory */
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_facility = DB_URS_ID;
	scf_cb.scf_scm.scm_in_pages = (sizeof(URS_MGR_CB)/SCU_MPAGESIZE+1);
	scf_cb.scf_scm.scm_addr = (char *)Urs_mgr_cb;
	status = scf_call(SCU_MFREE, &scf_cb);
	if (status != E_DB_OK)
	    SET_URSB_ERROR(ursb, scf_cb.scf_error.err_code, status);

	Urs_mgr_cb = NULL;

	return ursb->ursb_error.err_code;
}

/*{
** Name: SRVR_STARTUP - Start the Application Server and its Applications
**
** Description:
**      Start the Application Server and its applications. The applications are
**      defined in the repository for this Frontier Application Server.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 	    E_DB_OK			Applications started OK
** 	    E_DB_WARN			No applications defined; none started
** 	    E_DB_ERROR			Error starting applications (the
** 							ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	16-Dec-1997 wonst02
** 	    Original.
** 	13-Jan-1998 wonst02
** 	    Application block memory comes out of server ULM stream.
** 	21-Mar-1998 wonst02)
** 	    Add the missing Interface layer.
*/
DB_STATUS
srvr_startup(URS_MGR_CB	*ursm,
	     URSB	*ursb)
{
	FAS_SRVR	*fas;    		/* Ptr to the appl srvr block */
	FAS_APPL	*curappl;		/* Current application block */
	DB_STATUS	status = E_DB_OK;

	/*
	** Allocate & init memory for the application server block.
	*/
	if ( srvr_memory(ursm, ursb) != E_DB_OK )
	    return E_DB_ERROR;
	fas = ursm->ursm_srvr;
	fas->fas_srvr_id = 1;
	CSw_semaphore(&fas->fas_srvr_applsem, CS_SEM_SINGLE,
		      "FAS_SRVR_APPLSEM");

	/*
	** Read the repository information and build control blocks.
	*/
	if (urd_connect(ursm, ursb) != E_DB_OK)
	    return E_DB_ERROR;
	status = urs_readrepository(ursm, ursb);
	if (urd_disconnect(ursm) != E_DB_OK || DB_FAILURE_MACRO(status))
	    return E_DB_ERROR;

	if ( fas->fas_srvr_appl )
	{	
	    for (curappl = fas->fas_srvr_appl; 
	     	 curappl; 
	     	 curappl = curappl->fas_appl_next)
	    {	
		DB_STATUS	s;

	    	/*
	    	** Load the DLL and start the application
	    	*/
	    	s = appl_startup(ursm, curappl, ursb);
		if (s > E_DB_ERROR)
		    return s;
		else if (s != E_DB_OK)
		    curappl->fas_appl_flags = FAS_DISABLED;
		else
		    curappl->fas_appl_flags = FAS_STARTED;
	    }
	}
	else
	{
	    urs_error(E_UR0106_APPL_NOT_DEFINED, URS_INTERR, 0);
	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_WARN)
	    status = E_DB_WARN;		/* No applications defined */
	}

	return status;
}

/*{
** Name: SRVR_SHUTDOWN - Shut Down the Application Server and Applications
**
** Description:
**      Stop the Application Server. Terminate every application that is
** 	running. Unload the dll.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	16-Dec-1997 wonst02
** 	    Original.
** 	13-Jan-1998 wonst02
** 	    Application block memory comes out of server ULM stream.
*/
DB_STATUS
srvr_shutdown(URS_MGR_CB	*ursm,
	      URSB		*ursb)
{
	FAS_SRVR		*fas;    	/* Ptr to appl srvr block */
	FAS_APPL		*appl;		/* Application block */
	DB_STATUS		ret_stat = E_DB_OK,
					status = E_DB_OK;

	if ((fas = ursm->ursm_srvr) == NULL)
	    return E_DB_WARN;
	if ((appl = fas->fas_srvr_appl) == NULL) /* 1st appl addr */
	    ret_stat = E_DB_WARN;
	/*
	** Terminate and deallocate memory for all the applications.
	*/
	while ( appl )
	{
	    /*  Shut down the application */
	    status = appl_shutdown(ursm, appl, ursb);
	    if (DB_FAILURE_MACRO(status) && status > ret_stat)
		ret_stat = status;
	    appl = appl->fas_appl_next;  /* Next appl addr  */
	};

	CSr_semaphore( &fas->fas_srvr_applsem );

	status = free_srvr_memory(ursm, ursb);
	if (DB_FAILURE_MACRO(status) && status > ret_stat)
	    ret_stat = status;

	return ret_stat;
}

/*{
** Name: SRVR_MEMORY - Allocate and init memory for appl. server block
**
** Description:
**      Use a ULM stream to allocate memory for the application server block.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
** 	17-Dec-1997 wonst02
** 	    Original.
*/
DB_STATUS
srvr_memory(URS_MGR_CB	*ursm,
	    URSB	*ursb)
{
	ULM_RCB		*ulm = &ursm->ursm_ulm_rcb; 	/* ULM Request block */
	FAS_SRVR	*fas;
	DB_STATUS	status = E_DB_OK;

	/*
	** Allocate a memory stream for the application server block
	*/
	ulm->ulm_blocksize = sizeof(FAS_SRVR);
	ulm->ulm_psize 	   = sizeof(FAS_SRVR);
	ulm->ulm_flags     = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm->ulm_memleft   = &ursm->ursm_memleft;
	ursm->ursm_memleft = URS_APPMEM_DEFAULT;
	if ((status = ulm_openstream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0315_ULM_OPENSTREAM_PALLOC, URS_INTERR, 4,
	    	      sizeof(*ulm->ulm_memleft), ulm->ulm_memleft,
	    	      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
	    	      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize,
                  sizeof(ulm->ulm_psize), &ulm->ulm_psize);
        if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_ERROR)
        else
	    	SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	fas = ursm->ursm_srvr = (FAS_SRVR *)ulm->ulm_pptr;

	/*
	** Open a memory stream for the application blocks
	*/
	ulm = &fas->fas_srvr_ulm_rcb;
	STRUCT_ASSIGN_MACRO(ursm->ursm_ulm_rcb, *ulm);
	ulm->ulm_blocksize = sizeof(FAS_APPL) * 8;
	ulm->ulm_flags = ULM_PRIVATE_STREAM;
	ulm->ulm_memleft = &fas->fas_srvr_memleft;
	fas->fas_srvr_memleft = URS_APPMEM_DEFAULT;
	if ((status = ulm_openstream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0312_ULM_OPENSTREAM_ERROR, URS_INTERR, 3,
	    	      sizeof(*ulm->ulm_memleft), ulm->ulm_memleft,
	    	      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
	    	      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize);
            if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_ERROR)
            else
	    	SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	return E_DB_OK;
}

/*{
** Name: FREE_SRVR_MEMORY - De-allocate application server memory
**
** Description:
**      Free memory for the application server block from the ULM stream.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	31-Dec-1997 wonst02
** 	    Original.
*/
DB_STATUS
free_srvr_memory(URS_MGR_CB	*ursm,
		 URSB		*ursb)
{
	ULM_RCB		*ulm = &ursm->ursm_srvr->fas_srvr_ulm_rcb;
	DB_STATUS	status = E_DB_OK;

	/*
	** De-Allocate memory stream for the application server block.
	*/
	if ((status = ulm_closestream(ulm)) != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0313_ULM_CLOSESTREAM_ERROR, URS_INTERR, 1,
		      sizeof(ulm->ulm_error.err_code),
		      &ulm->ulm_error.err_code);
		SET_URSB_ERROR(ursb, E_UR0202_URS_APPL_FREE_ERROR, status)
	}
	return status;
}

/*{
** Name: APPL_STARTUP - Load and start the application
**
** Description:
**      Start the Application. Initialize every interface defined in the
** 	repository for this application.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 	    E_DB_OK			Application started OK
** 	    E_DB_WARN			No interfaces defined; none started
** 	    E_DB_ERROR			Error starting application (the
** 							ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	21-Mar-1998 wonst02
** 	    Original.
*/
DB_STATUS
appl_startup(URS_MGR_CB		*ursm,
	     FAS_APPL		*appl,
	     URSB		*ursb)
{
	ULM_RCB		*ulm = &appl->fas_appl_ulm_rcb; /* ULM Request block */
	FAS_INTFC	*curintfc;	        /* Current interface block */
	DB_STATUS	status;

	/*
	** Load the application DLL
	*/
	status = urs_load_dll(ursm, appl, ursb);
	if (DB_FAILURE_MACRO(status)) 
	    return E_DB_ERROR;
	/*
	** Open a memory stream for the interface blocks
	*/
	STRUCT_ASSIGN_MACRO(ursm->ursm_ulm_rcb, *ulm);
	ulm->ulm_blocksize = sizeof(FAS_INTFC) * 8;
	ulm->ulm_flags = ULM_PRIVATE_STREAM;
	ulm->ulm_memleft = &appl->fas_appl_memleft;
	appl->fas_appl_memleft = URS_APPMEM_DEFAULT;
	if ((status = ulm_openstream(ulm)) != E_DB_OK)
	{
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0312_ULM_OPENSTREAM_ERROR, URS_INTERR, 3,
	    	      sizeof(*ulm->ulm_memleft), ulm->ulm_memleft,
	    	      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
	    	      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize);
            if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
		SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_ERROR)
            else
	    	SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
	    return E_DB_ERROR;
	}

	if ( appl->fas_appl_intfc == NULL )
	{
	    urs_error(E_UR0116_NO_INTFC_DEFINED, URS_INTERR, 1,
                  sizeof appl->fas_appl_name, &appl->fas_appl_name);
	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_WARN)
	    return E_DB_WARN;		/* No interfaces defined */
	}

	for ( curintfc = appl->fas_appl_intfc; 
	      curintfc; 
	      curintfc = curintfc->fas_intfc_next)
	{
	    /*
	    ** Start the interface
	    */
	    status = intfc_startup(ursm, curintfc, ursb);
	    if (status > E_DB_ERROR)
	    	return status;
	    else if (status)
	    	curintfc->fas_intfc_flags = FAS_DISABLED;
	    else
	    	curintfc->fas_intfc_flags = FAS_STARTED;
	}

	return E_DB_OK;
}

/*{
** Name: APPL_SHUTDOWN - Stop the application and free resources
**
** Description:
**      Stop the Application. Terminate every interface defined for this 
** 	application.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	appl				The application control block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 	    E_DB_OK			Application stopped OK
** 	    E_DB_WARN			No interfaces started; none stopped
** 	    E_DB_ERROR			Error stopping application (the
** 							ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	25-Mar-1998 wonst02
** 	    Original.
*/
DB_STATUS
appl_shutdown(URS_MGR_CB	*ursm,
	      FAS_APPL		*appl,
	      URSB		*ursb)
{
	FAS_INTFC	*intfc;
	DB_STATUS	status = E_DB_OK;
	DB_STATUS	ret_stat = E_DB_OK;

	if (appl == NULL)
	    return E_DB_WARN;
	if ((intfc = appl->fas_appl_intfc) == NULL) /* 1st interface block */
	    ret_stat = E_DB_WARN;
	/*
	** Terminate all the interfaces in this application.
	*/
	if (appl->fas_appl_type == FAS_APPL_OPENROAD)
	{	
	    while ( intfc )
	    {
	    	/*  Shut down the interface */
	    	status = urs_shut_intfc(ursm, intfc, ursb);
	    	if (DB_FAILURE_MACRO(status) && status > ret_stat)
			ret_stat = status;
	    	intfc = intfc->fas_intfc_next;  /* Next interface block  */
	    }
	}

	status = urs_shut_appl(ursm, appl, ursb);
	if (DB_FAILURE_MACRO(status) && status > ret_stat)
	    ret_stat = status;

	status = free_appl_memory(ursm, appl, ursb);
	if (DB_FAILURE_MACRO(status) && status > ret_stat)
	    ret_stat = status;

	return ret_stat;
}

/*{
** Name: FREE_APPL_MEMORY - De-allocate application memory
**
** Description:
**      Free memory for the application block from the ULM stream.
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	25-mar-1998 wonst02
** 	    Original.
*/
DB_STATUS
free_appl_memory(URS_MGR_CB	*ursm,
		 FAS_APPL	*appl,
		 URSB		*ursb)
{
	ULM_RCB		*ulm = &appl->fas_appl_ulm_rcb;
	DB_STATUS	status = E_DB_OK;

	/*
	** De-Allocate memory stream for the application block.
	*/
	if ((status = ulm_closestream(ulm)) != E_DB_OK)
	{
	    status = E_DB_ERROR;
	    urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
	    urs_error(E_UR0313_ULM_CLOSESTREAM_ERROR, URS_INTERR, 1,
		      sizeof(ulm->ulm_error.err_code),
		      &ulm->ulm_error.err_code);
		SET_URSB_ERROR(ursb, E_UR0202_URS_APPL_FREE_ERROR, status)
	}
	return status;
}

/*{
** Name: INTFC_STARTUP - Load and start the interface
**
** Description:
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 	    E_DB_OK			Application started OK
** 	    E_DB_WARN			No interfaces defined; none started
** 	    E_DB_ERROR			Error starting application (the
** 					ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	28-Mar-1998 wonst02
** 	    Original.
*/
DB_STATUS
intfc_startup(URS_MGR_CB	*ursm,
	      FAS_INTFC		*intfc,
	      URSB		*ursb)
{
	FAS_APPL	*appl = intfc->fas_intfc_appl;
	FAS_METHOD	*curmethod;		/* Current method block */
	CL_ERR_DESC	err ZERO_FILL;
	DB_STATUS	status;

	if (appl->fas_appl_type == FAS_APPL_OPENROAD)
	{	
	    status = urs_start_intfc(ursm, intfc);
	    if ( status )
	    {
	    	SET_URSB_ERROR(ursb, E_UR0110_INTFC_STARTUP, status);	  
	    	return status;
	    }
	}

	if ( intfc->fas_intfc_method == NULL )
	{
	    urs_error(E_UR0151_NO_METHOD_DEFINED, URS_INTERR, 1,
                      sizeof appl->fas_appl_name, &appl->fas_appl_name);
	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_WARN)
	    return E_DB_WARN;		/* No methods defined */
	}

	/*
	** For non-OpenRoad applications, get the methods' entry points.
	*/
	if (appl->fas_appl_type != FAS_APPL_OPENROAD)
	{
	    for (curmethod = intfc->fas_intfc_method; 
	      	 curmethod; 
	      	 curmethod = curmethod->fas_method_next)
	    {
		char		method_name[sizeof curmethod->
					    fas_method_name.db_name + 1];
		char		*name = curmethod->fas_method_name.db_name;
		char		*nameend;
		STATUS		s;

		nameend = STindex(name, ERx(" "), 
				  sizeof curmethod->fas_method_name.db_name);
		if (nameend == NULL)
		    nameend = name + sizeof curmethod->fas_method_name.db_name;
		STlcopy(name, method_name, nameend - name);
    	    	if (s = DLbind(appl->fas_appl_handle, 
			       method_name,
			       &curmethod->fas_method_addr, &err) != OK)
    	    	{
	    	    urs_error(E_UR0373_DLL_BIND, URS_INTERR, 3,
	    	    	      sizeof(s), &s,
			      STlength(method_name), method_name,
			      STlength(appl->fas_appl_loc.string), 
			      appl->fas_appl_loc.string);
	    	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
		    return E_DB_ERROR;
    	    	}
	    }
	}

	return E_DB_OK;
}

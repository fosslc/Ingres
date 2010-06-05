/*
** Copyright (c) 1987, 2005 Ingres Corporation
**
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <ci.h>
#include    <sl.h>
#include    <cm.h>
#include    <er.h>
#include    <ex.h>
#include    <id.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <me.h>
#include    <cs.h>
#include    <lk.h>
#include    <cx.h>
#include    <cv.h>
#include    <gc.h>
#include    <gcccl.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <erusf.h>
#include    <ulf.h>
#include    <gca.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <ddb.h>
#include    <ulm.h>

/* added for scs.h prototypes, ugh! */
#include    <dudbms.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <qefqeu.h>

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

#include    <urs.h>
#include    <ascf.h>
#include    <ascs.h>
#include    <ascd.h>

/*
**
**  Name: SCDMAIN.C - Entry point and support routines for the DBMS Server
**
**  Description:
**      This file contains the main entry point (main) for the server 
**      as well as the associated support routines.
**
**          main() - main entry point
**          ascd_alloc_scb() - Allocate an SCB
**          ascd_dealloc_scb() - Deallocate an SCB
**	    scd_reject_assoc() - Reject an association
**	    scd_get_assoc() - Await new association
**	    scd_disconnect() - Disconnect an association
**
**
**  History:
**	03-dec-1997 (canor01)
**	    Add SC_ICE_SERVER.
**      04-Mar-98 (fanra01)
**          Move the call to scs_scb_attach into the cs via the Cs_srv_block.
** 	12-Mar-1998 (wonst02)
** 	    Initialize scb tuple counts as they are in the Ingres server.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started as
**	    factotum threads.
** 	23-Jun-1998 (wonst02)
** 	    Reordered/removed some include & NEEDLIBS. Removed ref to DB_GWF_ID.
**	03-Aug-1998 (fanra01)
**	    Add castings to remove compiler warning on unix.
**      16-Oct-98 (fanra01)
**          Set server name if not default.
**	11-Jan-1999 (fanra01)
**          Rename ice server specific functions.
**          scd_alloc_scb
**          scd_dealloc_scb
**          scd_disconnect
**          Add ascf and ascd headers for ice specific prototypes.
**          Update ming hints.
**          Renamed Asc_server_name back to Sc_server_name.
**          Make default server_type icesvr.
**      10-Feb-1999 (fanra01)
**          Add initialisation of cs_get_rcp_pid to NULL.  CSMTp_semahore
**          calls this function if it is not NULL. 
**      12-Feb-99 (fanra01)
**          Rename scd_reject_assoc and scs_chk_priv to their ascd and ascs
**          equivalents.
**      23-Feb-99 (fanra01)
**          Hand integration of change 439795 by nanpr01.
**          Changed the sc0m_deallocate to pass pointer to a pointer.
**      25-Feb-1999 (fanra01)
**          Add tracing thread.
**      04-Jan-2000 (fanra01)
**          Hand integration of 437760
**          15-sep-1998 (somsa01)
**              Bug #93332
**              We were playing with sc_misc_semaphore AFTER releasing the SCB.
**              Relocated the code which releases the SCB to the end.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**	11-sep-2002 (somsa01)
**	    Prior change is not for Windows.
**	25-Sep-2002 (hanch04)
**	    PCexeclp64 needs to only be called from the 32-bit version of
**	    the exe only in a 32/64 bit build environment.
**	03-Oct-2002 (somsa01)
**	    If CSinitiate() fails, make sure we print out "FAIL".
**	03-oct-2002 (devjo01)
**	    Set NUMA context.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	15-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**      18-Jun-2004 (hanch04)
**          Removed reference to LICENSE for the open source release.
**	27-Jul-2004 (hanje04)
**	    Swapped order fo GCFLIB and APILIB in NEEDLIBS so that icesrv
**	    links after removing gcu_encode() references from DDFLIB.
**	27-Jul-2004 (hanje04)
**	    Remove reference to SDLIB
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS, added ICELIB which is a collection of ice
**	    static libaries and is defined in Jamdefs.
**	09-may-2005 (devjo01)
**	    Add useless ref to SCS_SDISTDEADLOCK for foolish consistency..
**	10-Sep-2008 (jonj)
**	    SIR 120874: Replace sc0e_put() with sc0ePutAsFcn() in cs_elog.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Replace sc0ePutAsFcn with sc0e_putAsFcn
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
*/

/*
** mkming hints
NEEDLIBS =      SCFLIB COMPATLIB CUFLIB ULFLIB DMFLIB \
		GCFLIB ICELIB 

OWNER =		INGUSER

DEST =		bin

PROGRAM =	icesvr

*/

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF   SC_MAIN_CB       *Sc_main_cb; /* Scf main ds */
GLOBALREF   i4	     Sc_server_type; /* for handing to scd_initiate */
GLOBALREF   PTR              Sc_server_name;
# ifdef UNIX
GLOBALREF   i4	    Cs_numprocessors;
# endif /* UNIX */
#undef II_DMF_MERGE

/*
**  Forward and/or External function references.
*/

static STATUS ascd_allow_connect( GCA_LS_PARMS *crb );

# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	iidbms_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

static PM_ERR_FUNC	ascd_pmerr_func;

/*{
** Name: main	- Main entry point for the DBMS Server
**
** Description:
**      This routine provides the central entry point for the system. 
**      When entering the system, this routine initializes the dispatcher 
**      CS\CLF module, and then tells it to go.  Control never returns
**      to this module once CSdispatch() is called.
**
** Inputs:
**      None
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**	03-dec-1997 (canor01)
**	    Add SC_ICE_SERVER.
**      04-Mar-98 (fanra01)
**          Move the call to scs_scb_attach into the cs via the Cs_srv_block.
**	03-Aug-1998 (fanra01)
**	    Add casting to function pointer assignment to remove compiler
**	    warning on unix.
**      16-Oct-98 (fanra01)
**          Set server name if not default.
**      10-Feb-1999 (fanra01)
**          Add initialisation of cs_get_rcp_pid to NULL.  CSMTp_semahore
**          calls this function if it is not NULL. 
**	03-Oct-2002 (somsa01)
**	    If CSinitiate() fails, make sure we print out "FAIL".
**	03-oct-2002 (devjo01)
**	    Set NUMA context.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/
int
# ifdef	II_DMF_MERGE
    MAIN(argc, argv)
# else
main(argc, argv)
# endif
i4		argc;
char		**argv;
{
    DB_STATUS           status;
    STATUS              ret_status;
    CL_ERR_DESC		cl_err;
    CS_CB		ccb;
    i4             err_code;
    i4			num_sessions = 0;
    char		*tran_place = 0;
    char		*server_type = "icesvr";
    char		*server_flavor = "*";
    char		un_env[MAX_LOC-20];	/* Room for a %d */
    char                un_str[MAX_LOC];
    char                *pid_index = NULL;
    PID                 un_num;
    LOCATION		exeloc;
    LOCATION		fileloc;
    char		locbuf[MAX_LOC+1];
    char		filebuf[MAX_LOC+1] = "";
    char		*fname;
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    char		*lp64enabled;
#endif

    MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)

    /*
    ** Try to exec the 64-bit version
    */
    NMgtAt("II_LP64_ENABLED", &lp64enabled);
    if ( (lp64enabled && *lp64enabled) &&
       ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
	 !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
    {
	PCexeclp64(argc, argv);
    }
#endif  /* conf_BUILD_ARCH_32_64 */

    /*
    ** Stash args.
    */

    if( argc > 1 )
	server_type = argv[1];

    Sc_server_name = ERx("default");

    if( argc > 2 )
    {
	server_flavor = argv[2];
        Sc_server_name = argv[2];
    }
    if ( argc == 1 )
    {
	char *cp;

	STcopy( argv[0], locbuf );
        LOfroms( PATH & FILENAME, locbuf, &exeloc );
        LOfroms( FILENAME, filebuf, &fileloc );
        LOgtfile( &exeloc, &fileloc );
	LOtos( &fileloc, &fname );
	if ( (cp = STrindex( fname, ".", 0 )) != NULL )
	    *cp = EOS;
	server_type = fname;
    }

    /* Set CM character set stuff */

    ret_status = CMset_charset(&cl_err);
    if ( ret_status != OK)
    {
        ule_format(E_UL0016_CHAR_INIT, &cl_err, ULE_LOG ,
            NULL, (char * )0, 0L, (i4 *)0, &err_code, 0);
        PCexit(FAIL);
    }

    /*
    ** Divine server type.  This mapping is for historical reasons.
    **
    **	dbms 	-> INGRES
    **	rms 	-> RMS
    **	star 	-> STAR
    **	recovery -> RCP
    */

    if( !STbcompare( server_type, 0, "dbms", 0, TRUE ) )
    {
	Sc_server_type = SC_INGRES_SERVER;
    }
    else if( !STbcompare( server_type, 0, "rms", 0, TRUE ) )
    {
	Sc_server_type = SC_RMS_SERVER;
    }
    else if( !STbcompare( server_type, 0, "star", 0, TRUE ) )
    {
	Sc_server_type = SC_STAR_SERVER;
    }
    else if( !STbcompare( server_type, 0, "recovery", 0, TRUE ) )
    {
	Sc_server_type = SC_RECOVERY_SERVER;
    }
    else if( !STbcompare( server_type, 0, "iomaster", 0, TRUE ) )
    {
	Sc_server_type = SC_IOMASTER_SERVER;  
    }
    else if( !STbcompare( server_type, 0, "icesvr", 0, TRUE ) )
    {
	Sc_server_type = SC_ICE_SERVER;  
    }
    else
    {
	sc0e_put(E_SC0346_DBMS_PARAMETER_ERROR, (CL_ERR_DESC*)0,
		(i4)1, (i4)0, (PTR)server_type,
		(i4)0, (PTR)0, (i4)0, (PTR)0, (i4)0, (PTR)0,
		(i4)0, (PTR)0, (i4)0, (PTR)0);
	sc0e_0_put(E_SC0124_SERVER_INITIATE, (CL_ERR_DESC *)0 );
	PCexit(FAIL);
    }

    /*
    **  Initialise Paramater Management interface
    */
    PMinit();
    switch( (status = PMload((LOCATION *)NULL, ascd_pmerr_func)))
    {
	case OK:
	    /* Loaded sucessfully */
	    break;
	case PM_FILE_BAD:
	    /* syntax error */
	    sc0e_0_put(E_SC032B_BAD_PM_FILE, 0);
            PCexit(FAIL);
	default: 
	    /* unable to open file */
	    sc0e_0_put(status, 0);
	    sc0e_0_put(E_SC032C_NO_PM_FILE, 0);
            PCexit(FAIL);
    }
    /*
    ** Next set up the first few default parameters for subsequent PMget 
    ** calls based upon the name of the local node...
    */
    PMsetDefault( 0, SYSTEM_CFG_PREFIX );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, server_type );
    PMsetDefault( 3, server_flavor );

    if ( CXnuma_cluster_configured() )
    {
	i4	rad;

	/* RAD affiliation passed in installation code as argv[3]. */
	if ( argc < 4 || ( STlength(argv[3]) < 4 ) ||
	     '.' != *(argv[3] + 2) ||
	     OK != CVan(argv[3] + 3, &rad) ||
	     OK != CXset_context(NULL, rad) )
	{
	    sc0e_put(E_CL2C44_CX_E_NO_NUMA_CONTEXT, (CL_ERR_DESC*)0,
		    (i4)1, (i4)0, (PTR)server_type,
		    (i4)0, (PTR)0, (i4)0, (PTR)0, (i4)0, (PTR)0,
		    (i4)0, (PTR)0, (i4)0, (PTR)0);
	    sc0e_0_put(E_SC0124_SERVER_INITIATE, (CL_ERR_DESC *)0 );
	    PCexit(FAIL);
	}
    }
    /*
    ** Pick up other paramaters..
    */

    num_sessions = 32;   /* {@fix_me@} random at the moment */
    
    /* For Star, check II_STAR_LOG first, use II_DBMS_LOG if former
    ** is not defined.
    */

    switch( Sc_server_type )
    {
    case SC_INGRES_SERVER:	NMgtAt("II_DBMS_LOG", &tran_place); break;
    case SC_RECOVERY_SERVER:	NMgtAt("II_DBMS_LOG", &tran_place); break;
    case SC_RMS_SERVER:		NMgtAt("II_RMS_LOG", &tran_place); break;
    case SC_STAR_SERVER:	NMgtAt("II_STAR_LOG", &tran_place); break;
    case SC_IOMASTER_SERVER:	NMgtAt("II_IOM_LOG", &tran_place); break;
    case SC_ICE_SERVER:		NMgtAt("II_ICE_LOG", &tran_place); break;
    }

    /*
    ** Make a unique file-name if $II_DBMS_LOG contains exactly one
    ** "%p" (no other %-things):
    ** replace "%p" with process id 
    */
    if ((tran_place) && (*tran_place))
    {
	/* Potentially truncate... */
	STlcopy(tran_place, &un_env[0], sizeof(un_env)-1);
	pid_index = STchr(un_env, '%');
	if((pid_index != NULLPTR) && (*(++pid_index) == 'p')
	  && STchr(pid_index+1, '%') == NULL)
	{
	    *pid_index = 'd';
	    PCpid(&un_num);
	    STprintf(un_str, un_env, un_num);
	    TRset_file(TR_F_OPEN, un_str, STlength(un_str), &cl_err);
        }
	else TRset_file(TR_F_OPEN, un_env, STlength(un_env), &cl_err);
    }

/* This defined should be removed once EXdumpInit has been added to
** the CL for VMS 
*/
# ifdef UNIX
    EXdumpInit(); /* Initialise server diagnostics */ 
# endif /* UNIX */

    for (;;)
    {

	ccb.cs_scnt = num_sessions;
	ccb.cs_ascnt = num_sessions;
	ccb.cs_stksize = 32768;		 /* {@fix_me@} random at the moment */
	ccb.cs_elog = (VOID(*)())sc0e_putAsFcn;
	ccb.cs_attn = ascs_attn;
	ccb.cs_format = ascs_format;
	ccb.cs_facility = ascs_facility;
	ccb.cs_read = scc_recv;
	ccb.cs_write = scc_send;
	ccb.cs_saddr = ascd_get_assoc;
	ccb.cs_reject = ascd_reject_assoc;
        ccb.cs_scbattach = (VOID(*)())ascs_scb_attach;
        ccb.cs_scbdetach = (VOID(*)())ascs_scb_detach;
	ccb.cs_get_rcp_pid = (i4(*)()) NULL;

	ccb.cs_scballoc = ascd_alloc_scb;
	ccb.cs_scbdealloc = ascd_dealloc_scb;
	ccb.cs_process = ascs_sequencer;
	ccb.cs_startup = ascd_initiate;
	ccb.cs_shutdown = ascd_terminate;
	ccb.cs_disconnect = (VOID(*)())ascd_disconnect;
# ifdef UNIX
	ccb.cs_diag = scd_diag;
# endif /* UNIX */
	ccb.cs_format_lkkey = LKkey_to_string;

	argc = 1;

	status = CSinitiate(&argc, &argv, &ccb);
	if (status)
	{
	    SIstd_write(SI_STD_OUT, "FAIL\n");
	    ule_format(status, (CL_ERR_DESC *)0, ULE_LOG, (DB_SQLSTATE *) NULL,
	  	(char *)0, (i4)0, (i4 *)0, &err_code, 0);
	    break;
	}

# ifdef UNIX
	/*
	** ??? Note:  This code should be generalized by adding the test to
	** ??? every CL's version of CSdispatch() or CSinitiate().  Or, a new
	** ??? CL interface routine (added on all platforms) could return the
	** ??? required information.  Do not document for users 'til this is
	** ??? fixed.
	*/
	if (Sc_server_type == SC_IOMASTER_SERVER && Cs_numprocessors <=1)
	{
	    /* do not allow IOMASTER server to run on uni-processor */
	    sc0e_0_put(E_SC0369_IOMASTER_BADCPU, 0);
	    PCexit(FAIL);
	}
# endif /* UNIX */

	ascs_mo_init();
	status = CSdispatch();
	TRdisplay("CSdispatch() = %x\n", status);
	break;
    }

    TRset_file(TR_F_CLOSE, 0, 0, &cl_err);
    PCexit(OK);
    /* NOTREACHED */
    return(FAIL);
}

/*{
** Name: ascd_alloc_scb	- Allocate an SCB for the server
**
** Description:
**      This routine allocates a session control block for use by  
**      the remainder of the server.  In addition to simple allocation, 
**      this routine will fill in the initial values for the scb, as well 
**      as allocate any associated queue headers which belong with this 
**      entity.
**
** Inputs:
**	crb				Create request block.
**	thread_type			Type of thread -
**					SCS_SNORMAL (0) - user session.
**					SCS_SMONITOR - monitor program.
**					SCS_SFAST_COMMIT - fast commit thread.
**					SCS_SERVER_INIT - server init thread.
**					SCS_SEVENT	- event handling thread
**			-or-
**	ftc				Factotum Thread Create block
**	thread_type			SCS_SFACTOTUM
**
** Outputs:
**      scb_ptr                         Address of place to put the scb
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**	06-may-1998 (canor01)
**	    Add license checking thread.
**	18-May-1998 (shero03)
**	    Remove all DMF facilities from the needed set
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started as
**	    factotum threads.
**      15-Feb-1999 (fanra01)
**          Add cleanup checking thread.
**      25-Feb-1999 (fanra01)
**          Add tracing thread.
**	22-Apr-2010 (kschendel)
**	    Rd-lo-next/prev gone, fix here.
*/
STATUS
ascd_alloc_scb( SCD_SCB  **scb_ptr, GCA_LS_PARMS  *input_crb, i4  thread_type )
{
    STATUS              status = OK;
    SCS_MCB		*mcb_head;
    i4			i;
    char		*cp;
    i4		mem_needed;
    i4		scb_size;
    i4			thread_flags;
    DB_ERROR		error;
    SCF_FTC		*ftc;
    GCA_LS_PARMS	*crb;
    SCD_SCB		*scb;
    char		*block;
    char		*user_name, *terminal_name;

    /*
    ** Offsets to major control blocks and buffers.
    ** See comments below.
    */
    i4			need_facilities;	/* Facilities needed by session */
    i4		offset_adf;		/* Facilities' cbs, allocated   */
    i4		offset_dmf = 0;		/* based on need_facilities     */
    i4		offset_gwf;
    i4		offset_sxf;
    i4		offset_opf;
    i4		offset_psf;
    i4		offset_qef;
    i4		offset_qsf;
    i4		offset_rdf;
    i4		offset_scf;
    i4             offset_csitbl;          /* Offset of SCS_CSITBL     */
    i4             offset_inbuffer;        /* Offset of inbuffer       */
    i4             offset_outbuffer;       /* Offset of outbuffer	    */
    i4             offset_tuples;          /* Offset of tuples	    */
    i4             offset_rdata;           /* Offset of rdata          */
    i4             offset_rpdata;          /* Offset of rpdata         */
    i4             offset_evdata;          /* Offset of evdata         */
    i4             offset_dbuffer;         /* Offset of dbuffer        */
    i4		offset_facdata;		/* Offset of factotum data  */
    
    /*
    ** Are we allocating a Factotum thread?
    ** If so, there's no *crb, but a *ftc instead.
    */
    if (thread_type == SCS_SFACTOTUM)
    {
	ftc = (SCF_FTC*)input_crb;
	/*
	** If GCF is identified as a needed facility, the protocol
	** assumes that ftc_data, if non-NULL, contains a pointer to a CRB:
	*/
	if (ftc->ftc_facilities & (1 << DB_GCF_ID))
	{
	    crb = (GCA_LS_PARMS*)ftc->ftc_data;
	}
	else
	    crb = (GCA_LS_PARMS*)NULL;
    }
    else 
	crb = input_crb;

    if (crb && crb->gca_status && crb->gca_status != E_GC0000_OK)
    {
	/* don't dissasoc or log an in-process listen */
    
	if ( crb->gca_status != E_GCFFFE_INCOMPLETE )
	{
	    if (crb->gca_status != E_GC0032_NO_PEER)
		sc0e_0_put(crb->gca_status, &crb->gca_os_status);
	
	    status = ascs_disassoc(crb->gca_assoc_id);
	    if (status)
	    {
		sc0e_0_put(status, 0);
	    }
	}
	return(crb->gca_status);
    }

    /*
    ** Determine the attributes of and facilities needed
    ** by this thread:
    */
    switch (thread_type)
    {
    case SCS_SFACTOTUM:
	/* 
	** Factotum threads may or may not have a gca connection and
	** are neither vital nor permanent
	**
	** The Factotum creator specifies the facilities.
	*/
	thread_flags = SCS_INTERNAL_THREAD;
	need_facilities = ftc->ftc_facilities;
	break;

    case SCS_SFAST_COMMIT:
	/*
	** Fast Commit Thread has special attributes:
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to live through the life of the server.
	*    - it needs only DMF
	*/
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SRECOVERY:
    case SCS_SFORCE_ABORT:
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SLOGWRITER:
    case SCS_SCHECK_DEAD:
    case SCS_SGROUP_COMMIT:
    case SCS_SCP_TIMER:
    case SCS_SLK_CALLBACK:
    case SCS_SDEADLOCK:
    case SCS_SDISTDEADLOCK:
	/*
	** The logging & locking system special threads have special attributes:
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to live through the life of the server.
	**   - need only DMF
	*/
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SWRITE_ALONG:  
    case SCS_SREAD_AHEAD:    
	/*
	** Write Behind, Write Along, and Read Ahead threads:
	**   - are expected to live through the life of the server.
	**   - need only DMF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SERVER_INIT:
	/* server init has no GCA -- it is not permanent		    */
	thread_flags = (SCS_VITAL | SCS_INTERNAL_THREAD);
	/* Server init vital that it be run.  No problem if exits	    */
	need_facilities = 0;
	break;

    case SCS_SEVENT:
	/*
	** Event handling thread has special attributes:
	**  - lives for life of the server
	**  - needs nothing but DMF.
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_S2PC:
	/*
	** The STAR 2PC thread has special attributes:
	**   - it has no FE gca connection.
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to recover all orphan DX's and then disappear.
	**   - needs ADF and QEF
	*/
	thread_flags = (SCS_VITAL | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_QEF_ID) );
	break;

    case SCS_SAUDIT:
	/* 
	** The Audit thread:
	**   - is permanent
	**   - it is a server fatal condition if the thread is terminated.
	**   - needs SXF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD | SCS_VITAL);
	need_facilities = ( (1 << DB_SXF_ID) | (1 << DB_DMF_ID) |
	     		    (1 << DB_ADF_ID) );

	break;

    case SCS_SCHECK_TERM:
	/* 
	** The Terminator thread:
	**   - is permanent
	**   - needs SXF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_SXF_ID) | (1 << DB_DMF_ID) |
	     		    (1 << DB_ADF_ID) );

	break;

    case SCS_SSECAUD_WRITER:
	/* 
	** The Security Audid Writer thread:
	**   - is permanent
	**   - needs SXF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_SXF_ID) | (1 << DB_DMF_ID) |
	     		    (1 << DB_ADF_ID) );
	break;

    case SCS_REP_QMAN:
	/*
	** The Replicator Queue Management thread:
	**   - is permanent
	**   - needs QEF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_DMF_ID) |
			    (1 << DB_QEF_ID) );
	break;

    case SCS_SSAMPLER:
	/*
	** The sampler thread has no GCA and is transient
	**   - is transient
	**   - needs no facilities
	*/
	thread_flags = SCS_INTERNAL_THREAD;
	need_facilities = 0;
	break;

    case SCS_PERIODIC_CLEAN:
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = 0;
        break;

    case SCS_TRACE_LOG:
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = 0;
        break;

    case SCS_SMONITOR:
    case SCS_SNORMAL:
    default:

	/* User thread -- check to see if we should allow him to connect.
	** Don't check PM file if it's OK in the normal case.
	*/

	status = ascd_allow_connect( crb );
	if( status != OK )
	{
	    ascd_reject_assoc( crb, status );
	    return( status );
	}
	thread_flags = 0;

	if (thread_type == SCS_SMONITOR)
	    /*
	    ** Monitor session only needs ADF and QSF to run, but
	    ** may need DMF, QEF and SXF to look up user info at
	    ** startup from the IIDBDB.
	    */
	    need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_GCF_ID) );
	else
	    /* The Normal and default cases need all facilities */
	    need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_GCF_ID) );
	break;
    }

    /*									    */
    /*	Herein, we allocate the session control block.  To reduce overhead, */
    /*	we also allocate some buffers which are used on each query at the   */
    /*	same time.  These are						    */
    /*    - The SCD_SCB session control block                               */
    /*    - Control blocks for each needed facility			    */
    /*	  - The cursor control area -- variable sized at server startup	    */
    /*	  - The area into which to read data from the frontend partner	    */
    /*	  - The standard output buffer which we will write to the FE	    */
    /*		(This is different from the input buffer because for	    */
    /*		repeat queries, we use the parameters directly from the	    */
    /*		input area while we are filling the output area)	    */
    /*	  - Space for the standard response block to terminate each query.  */
    /*	  - Space for the standard response block to terminate each proc.   */
    /*	  - Space for a maximum size event message block for user alerts.   */
    /*	  - 512 bytes for small descriptors -- These are used for ``small'' */
    /*		select/retrieve's (approximately 20 attributes/no att names */
    /*		then rounded up nicely).				    */
    /*                                                                      */
    /*  Factotum threads only need memory for SCD_SCB, the Facilities,      */
    /*  and the (optional) data to be passed to the thread entry code.      */
    /*                                                                      */
    /* *** NOTE: ***                                                        */
    /*                                                                      */
    /* The above control block/buffers should be aligned on ALIGN_RESTRICT  */
    /* boundaries.  Specifically, the aligned areas include:                */
    /*                                                                      */
    /*              o SCD_SCB           (SCF's  session control block)      */
    /*              o SCS_CSITBL        (cursor control area)               */
    /*              o *cscb_inbuffer    (input buffer)                      */
    /*              o *cscb_outbuffer   (output buffer - SCC_GCMSG)         */
    /*              o *cscb_tuples      (formatted location)                */
    /*              o *cscb_rdata       (normal response block)             */
    /*              o *cscb_rpdata      (procedure response block)          */
    /*              o *cscb_evdata      (event message notification block)  */
    /*              o *cscb_dbuffer     (512-byte descriptor buffer)        */
    /*                                                                      */
    /* As a result of alignment requirements, padding may be allocated      */
    /* between each of these data areas.                                    */

    /*
    ** Calculate offsets to the major control blocks and buffers
    */
    if (need_facilities & (1 << DB_GCF_ID))
	mem_needed = (i4)sizeof(SCD_SCB);
    else
	/* Don't allocate SCC_CSCB part of SCD_SCB if no GCA */
	mem_needed = (i4)sizeof(SCD_SCB) - (i4)sizeof(SCC_CSCB);

    scb_size = mem_needed;

    /* After the SCB comes the Facilities cb area: */

    /* SCF, everyone needs SCF */
    offset_scf = mem_needed;
    mem_needed += sizeof(SCF_CB);
    mem_needed = DB_ALIGN_MACRO(mem_needed);

    /* ADF */
    if (need_facilities & (1 << DB_ADF_ID))
    {
	offset_adf = mem_needed;
	mem_needed += sizeof(ADF_CB) + DB_ERR_SIZE;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* After the facilities memory comes Factotum data... */
    if (thread_type == SCS_SFACTOTUM)
    {
	if (ftc->ftc_data && ftc->ftc_data_length)
	{
	    offset_facdata = mem_needed;
	    mem_needed = offset_facdata + ftc->ftc_data_length;
	    mem_needed = DB_ALIGN_MACRO(mem_needed);
	}
    }
    
    /* Then all the stuff for f/e GCA communications, if needed: */
    if (need_facilities & (1 << DB_GCF_ID)) 
    {
	offset_csitbl   = mem_needed;
	offset_inbuffer = offset_csitbl +
			  (Sc_main_cb->sc_acc * sizeof(SCS_CSI));
	offset_inbuffer = DB_ALIGN_MACRO(offset_inbuffer);

	offset_outbuffer= offset_inbuffer +
			  (crb->gca_size_advise ?
				    crb->gca_size_advise : 256);
	offset_outbuffer= DB_ALIGN_MACRO(offset_outbuffer);
	offset_tuples   = offset_outbuffer + (i4)sizeof(SCC_GCMSG);
	offset_tuples   = DB_ALIGN_MACRO(offset_tuples);
	offset_rdata    = offset_tuples +
			  (i4)Sc_main_cb->sc_maxtup;
	offset_rdata    = DB_ALIGN_MACRO(offset_rdata);
	offset_rpdata   = offset_rdata +
			  (i4)sizeof(GCA_RE_DATA);
	offset_rpdata   = DB_ALIGN_MACRO(offset_rpdata);
	offset_evdata   = offset_rpdata +
			  (i4)sizeof(GCA_RP_DATA);
	offset_evdata   = DB_ALIGN_MACRO(offset_evdata);
	offset_dbuffer  = offset_evdata +
			  (i4)sizeof(SCC_GCEV_MSG);
	offset_dbuffer  = DB_ALIGN_MACRO(offset_dbuffer);

	mem_needed = offset_dbuffer + 512;
    }
    /*
    ** lint truncation warning about truncating cast (within CS_SCB_TYPE
    ** define), but code valid
    */
    status = sc0m_allocate(0,
			   mem_needed, CS_SCB_TYPE,
			   (PTR) SCD_MEM, CS_TAG, (PTR *) scb_ptr);
    if (status != E_DB_OK)
	return(status);

    /*
    ** Initialize the major control blocks and buffers.
    */
    scb = *scb_ptr;
    block = (char *)scb;

    /* Zero-fill the SCB, all but the sc0m header portion */
    MEfill(scb_size - sizeof(SC0M_OBJECT), 0,
	    (char *)scb + sizeof(SC0M_OBJECT));

    scb->cs_scb.cs_client_type = SCD_SCB_TYPE;
    scb->scb_sscb.sscb_stype = thread_type;
    scb->scb_sscb.sscb_flags = thread_flags;
    scb->scb_sscb.sscb_need_facilities = need_facilities;

    /* Initialize the MCB */
    mcb_head = &scb->scb_sscb.sscb_mcb;
    mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;
    mcb_head->mcb_sque.sque_next = mcb_head->mcb_sque.sque_prev = mcb_head;
    mcb_head->mcb_session = (SCF_SESSION) scb;
    mcb_head->mcb_facility = DB_SCF_ID;

    /* Set pointers to the Facilities' control blocks and areas: */

    /* SCF */
    scb->scb_sscb.sscb_scccb = (SCF_CB *) (block + offset_scf);
    scb->scb_sscb.sscb_scscb = scb;

    /* ADF */
    if (need_facilities & (1 << DB_ADF_ID))
    {
	scb->scb_sscb.sscb_adscb = (ADF_CB *) (block + offset_adf);
    }

    /* DMF */
    if (need_facilities & (1 << DB_DMF_ID))
    {
	scb->scb_sscb.sscb_dmccb = (DMC_CB *) (block + offset_dmf);
	scb->scb_sscb.sscb_dmscb = (Sc_main_cb->sc_dmbytes > 0)
			? (char *)scb->scb_sscb.sscb_dmccb + sizeof(DMC_CB)
			: NULL;
	/* DMF is buggy if its ccb and scb areas aren't primed with zeroes */
	MEfill(sizeof(DMC_CB) + Sc_main_cb->sc_dmbytes, 0, 
			scb->scb_sscb.sscb_dmccb);
    }

    /* Factotum threads try to be very lightweight */
    if (thread_type == SCS_SFACTOTUM)
    {
	SCD_SCB		*parent_scb;

	/* Copy ICS information from parent (creating) session's SCB */
	CSget_scb((CS_SCB**)&parent_scb);

	MEcopy(&parent_scb->scb_sscb.sscb_ics,
	       sizeof(parent_scb->scb_sscb.sscb_ics),
	       &scb->scb_sscb.sscb_ics);

	scb->scb_sscb.sscb_ics.ics_gw_parms = (PTR)NULL;
	scb->scb_sscb.sscb_ics.ics_alloc_gw_parms = (PTR)NULL;

	/* Use factotum thread name as "user name" */
	user_name = ftc->ftc_thread_name;
	terminal_name = "<none>\0";

	/*
	** Copy the remaining thread startup info 
	** that SCS will need to complete thread initialization:
	**
	**   thread entry code
	**   thread exit code
	**   thread input data
	**   sid of this creating (parent) thread
	*/
	scb->scb_sscb.sscb_factotum_parms.ftc_state = 0;
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_entry = 
		ftc->ftc_thread_entry;
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit = 
		ftc->ftc_thread_exit;
	scb->scb_sscb.sscb_factotum_parms.ftc_data = 
		ftc->ftc_data;
	scb->scb_sscb.sscb_factotum_parms.ftc_data_length = 
		ftc->ftc_data_length;
	CSget_sid(&scb->scb_sscb.sscb_factotum_parms.ftc_thread_id); 

	/* Copy the input data for the thread */
	if (ftc->ftc_data && ftc->ftc_data_length)
	{
	    scb->scb_sscb.sscb_factotum_parms.ftc_data = 
			block + offset_facdata;
	    scb->scb_sscb.sscb_factotum_parms.ftc_data_length =
			ftc->ftc_data_length;

	    MEcopy(ftc->ftc_data,
		   ftc->ftc_data_length,
		   scb->scb_sscb.sscb_factotum_parms.ftc_data);
	}
    }

    /* If CRB, use its user name and terminal name */
    if (crb)
    {
	user_name = crb->gca_user_name;
	terminal_name = crb->gca_access_point_identifier;
    }

    /* Initialize user_name stuff in the ICS: */
    if (user_name)
    {
	/* calculate name length in i */
	for( i = 0, cp = user_name; *cp; i++, cp++ ) ;

	/*
	** For Star get the username.
	** The new Installation Password scheme requires Star to pass the username
	** and a NULL password to the GCA_REQUEST. The Name Server will fill in
	** the installation password for us. So, stop propogating the gca_listen
	** password, and stop looking at the aux data.
	*/
	if (thread_flags & SCS_STAR)
	{
	    DD_USR_DESC	*ruser;

	    ruser    = &scb->scb_sscb.sscb_ics.ics_real_user;

	    /* clear real user structure */
	    ruser->dd_u1_usrname[0] = EOS;
	    ruser->dd_u3_passwd[0]  = EOS;
	    ruser->dd_u4_status     = DD_0STATUS_UNKNOWN;

	    /* When dd_u2_usepw_b is FALSE, rqf will connect and then
	    ** modify the association like -u
	    */
	    ruser->dd_u2_usepw_b    = TRUE;
	    if (thread_type == SCS_S2PC)
		ruser->dd_u2_usepw_b    = FALSE;

	    MEmove( i, user_name, ' ',
		    sizeof(ruser->dd_u1_usrname), ruser->dd_u1_usrname );
	}

	/*
	** make ics_eusername point at the slot for the untranslated username 
	** (ics_xeusername).
	** later, this will be update to point to the slot for SQL-session <auth id>
	** (ics_susername).
	*/
	scb->scb_sscb.sscb_ics.ics_eusername =
	    &scb->scb_sscb.sscb_ics.ics_xeusername;

	/*
	** Mark the effective userid as delimited.
	** This may change later, if the user specified an effective userid
	** without delimiters.
	*/
	scb->scb_sscb.sscb_ics.ics_xmap = SCS_XDLM_EUSER;

	/*
	** store the untranslated username into the slots for SQL-session <auth id>
	** and system <auth id>; later, these will be updated to be the translated
	** versions (if appropriate).
	**
	** "i" holds the length of user_name.
	*/

	MEmove(i,
		    user_name,
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_xeusername),
		    (char *)scb->scb_sscb.sscb_ics.ics_eusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_xrusername),
		    &scb->scb_sscb.sscb_ics.ics_xrusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_susername),
		    &scb->scb_sscb.sscb_ics.ics_susername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
		    &scb->scb_sscb.sscb_ics.ics_rusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->cs_scb.cs_username),
		    scb->cs_scb.cs_username);

    }

    if (terminal_name)
    {
	/* calculate length of access_point_identifier in i */
	for (i = 0, cp = terminal_name; cp && *cp; i++, cp++) ;
		
	if (i)
	    MEmove(i,
		    terminal_name,
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_terminal),
		    (char *)&scb->scb_sscb.sscb_ics.ics_terminal);
	else
	    MEmove(9,
		    "<unknown>",
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_terminal),
		    (char *)&scb->scb_sscb.sscb_ics.ics_terminal);
    }

    /*
    ** Initalize GCA unless this thread is not connected to a front end
    ** process.
    */
    if (need_facilities & (1 << DB_GCF_ID))
    {
	scb->scb_sscb.sscb_cursor.curs_csi =
		    (SCS_CSITBL *) (block + offset_csitbl);
	scb->scb_sscb.sscb_cursor.curs_limit = Sc_main_cb->sc_acc;
	scb->scb_sscb.sscb_cursor.curs_curno = 0;
	scb->scb_sscb.sscb_cursor.curs_frows = 0;

	for (i = 0; i < scb->scb_sscb.sscb_cursor.curs_limit; i++)
	    scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_state = 0;

	scb->scb_sscb.sscb_cquery.cur_row_count = GCA_NO_ROW_COUNT;
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = 0;

	STRUCT_ASSIGN_MACRO(*crb, scb->scb_cscb.cscb_gcp.gca_ls_parms);
	scb->scb_cscb.cscb_assoc_id = crb->gca_assoc_id;
	scb->scb_cscb.cscb_comm_size = (crb->gca_size_advise ?
					   crb->gca_size_advise : 256);
	scb->scb_cscb.cscb_inbuffer =
		((char *) scb + offset_inbuffer);
	scb->scb_cscb.cscb_version = crb->gca_partner_protocol;
	if (((crb->gca_flags & GCA_DESCR_RQD) == GCA_DESCR_RQD) ||
	    (thread_flags & SCS_STAR))
	{
	    scb->scb_cscb.cscb_different = TRUE;
	}
	else
	{
	    scb->scb_cscb.cscb_different = FALSE;
	}

	scb->scb_cscb.cscb_outbuffer = (SCC_GCMSG *)(block + offset_outbuffer);
	scb->scb_cscb.cscb_outbuffer->scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_outbuffer->scg_marea = (block + offset_tuples);

	scb->scb_cscb.cscb_tuples = scb->scb_cscb.cscb_outbuffer->scg_marea;
	scb->scb_cscb.cscb_tuples_max = Sc_main_cb->sc_maxtup;
	scb->scb_cscb.cscb_tuples_len = 0;

	scb->scb_cscb.cscb_rspmsg.scg_marea = (block + offset_rdata);
	scb->scb_cscb.cscb_rspmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_rdata = (GCA_RE_DATA *)scb->scb_cscb.cscb_rspmsg.scg_marea;
	    
	scb->scb_cscb.cscb_rpmmsg.scg_marea = (block + offset_rpdata);
	scb->scb_cscb.cscb_rpmmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_rpdata = (GCA_RP_DATA *)scb->scb_cscb.cscb_rpmmsg.scg_marea;

	/* Allocate/preformat the dummy GCA_EVENT message out of the buffer */
	if ((thread_flags & SCS_STAR) == 0)
	{
	    scb->scb_cscb.cscb_evmsg.scg_marea = (block + offset_evdata);
	    scb->scb_cscb.cscb_evmsg.scg_mask = SCG_NODEALLOC_MASK;
	    scb->scb_cscb.cscb_evmsg.scg_type = GCA_EVENT;
	    scb->scb_cscb.cscb_evdata =
		(GCA_EV_DATA *)scb->scb_cscb.cscb_evmsg.scg_marea;
	}

	scb->scb_cscb.cscb_dscmsg.scg_marea = 
	    scb->scb_cscb.cscb_dbuffer = (block + offset_dbuffer);
	scb->scb_cscb.cscb_dscmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_dscmsg.scg_type = GCA_TDESCR;
	
	scb->scb_cscb.cscb_darea = (char *)scb->scb_cscb.cscb_dbuffer;
	scb->scb_cscb.cscb_dsize = 512;

	scb->scb_cscb.cscb_mnext.scg_prev =
		scb->scb_cscb.cscb_mnext.scg_next =
			(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext; 
	scb->scb_cscb.cscb_qdone.scg_prev =
		scb->scb_cscb.cscb_qdone.scg_next =
			(SCC_GCMSG *) &scb->scb_cscb.cscb_qdone; 
    }

    /* Update session/thread counts */
    CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );

    Sc_main_cb->sc_session_count[thread_type].created++;

    if (++Sc_main_cb->sc_session_count[thread_type].current >
	Sc_main_cb->sc_session_count[thread_type].hwm)
    {
	Sc_main_cb->sc_session_count[thread_type].hwm = 
            Sc_main_cb->sc_session_count[thread_type].current;
    }

    /*
    ** The sequencer state for normal sessions is SCS_ACCEPT.  For special
    ** threads which have dedicated purpose, set SCS_SPECIAL state.
    */
    if (thread_flags & SCS_INTERNAL_THREAD)
    {
	scb->scb_sscb.sscb_state = SCS_SPECIAL;
    }
    else
    {
	/* There's a long gap between decision to accept this
	   connection and bumping the count here.  This may be a problem
	   if someone else can re-enter this code. */

	/* Note that sc_current_conns includes both NORMAL and MONITOR sessions */
	if ( ++Sc_main_cb->sc_current_conns > Sc_main_cb->sc_highwater_conns )
	   Sc_main_cb->sc_highwater_conns = Sc_main_cb->sc_current_conns;

	scb->scb_sscb.sscb_state = SCS_ACCEPT;
    }

    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    return(E_DB_OK);
}

/*{
** Name: ascd_dealloc_scb - toss out scb
**
** Description:
**      This routine deallocates the scb passed into it.
**
** Inputs:
**      scb                             Pointer to area to deallocate
**
** Outputs:
**      none
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Mar-1998 (jenjo02)
**	    scb_sscb.sscb_mcb is now embedded in SCS_SSCB and does not
**	    need to be deallocated.
**      23-Feb-99 (fanra01)
**          Hand integration of change by nanpr01.
**          Changed the sc0m_deallocate to pass pointer to a pointer.
**      04-Jan-2000 (fanra01)
**          Hand integration of 437760
**          15-sep-1998 (somsa01)
**              Bug #93332
**              We were playing with sc_misc_semaphore AFTER releasing the SCB.
**              Relocated the code which releases the SCB to the end.
*/
STATUS
ascd_dealloc_scb( SCD_SCB *scb )
{
    STATUS	 status;
    i4		 count;
    i4	 	 thread_type = scb->scb_sscb.sscb_stype;


    (void) ascs_scb_detach( scb );

    /* Decrement current session count by session type */
    CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );

    Sc_main_cb->sc_session_count[thread_type].current--;

    if (thread_type == SCS_SNORMAL ||
	thread_type == SCS_SMONITOR)
    {
	Sc_main_cb->sc_current_conns--;

	/* MONITOR session are also counted as "normal" - go figure */
	if (thread_type == SCS_SMONITOR)
	    Sc_main_cb->sc_session_count[SCS_SNORMAL].current--;
    }

    if ( Sc_main_cb->sc_current_conns == 0 &&
         Sc_main_cb->sc_listen_mask & SC_LSN_TERM_IDLE )
    {
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	/* FIXME log something here?  Check status? */
	(void) CSterminate( CS_CLOSE, &count );
    }
    else
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    /*
    ** Release the SCB
    */
    if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc)
    {
	PTR                 block;

	block = (char *)scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc -
		     sizeof(SC0M_OBJECT);
	status = sc0m_deallocate(0, &block);
    }
    
    status = sc0m_deallocate(0, (PTR*)&scb);

    return( status );
}

/*{
** Name: ascd_reject_assoc - Reject an association for reason given
**
** Description:
**      This routine is provided so that the underlying dispatcher 
**      can easily reject a session.
**
** Inputs:
**      crb                             Connection buffer
**      error                           Why rejected.
**
** Outputs:
**      none
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/
STATUS
ascd_reject_assoc( GCA_LS_PARMS  *crb, STATUS error )
{
    GCA_RR_PARMS        rrparms;
    DB_STATUS		status; 
    STATUS		gca_status;
    CS_SCB		*scb;
    
    /* report reason for rejection */
    if ( error )
	sc0e_0_put( error, NULL );

    if (crb->gca_status && crb->gca_status != E_GC0000_OK)
    {
	if (crb->gca_status != E_GC0032_NO_PEER)
	    sc0e_0_put(crb->gca_status, &crb->gca_os_status);

	status = ascs_disassoc(crb->gca_assoc_id);
	if (status)
	{
	    sc0e_0_put(status, 0);
	}
	crb->gca_status = status;
	return(crb->gca_status);
    }

    CSget_scb(&scb);

    rrparms.gca_assoc_id = crb->gca_assoc_id;
    rrparms.gca_local_protocol = crb->gca_partner_protocol;
    rrparms.gca_modifiers = 0;
    rrparms.gca_request_status = error;

    status = IIGCa_call(GCA_RQRESP,
			    (GCA_PARMLIST *)&rrparms,
			    0,
			    (PTR)scb, 
			    (i4)-1,
			    &gca_status);

    if (status == OK)
	(VOID) CSsuspend(CS_BIO_MASK, 0, 0);
    else
	sc0e_0_put(gca_status, &rrparms.gca_os_status);

    status = ascs_disassoc(crb->gca_assoc_id);

    return(status);
}

/*{r
** Name: ascd_get_assoc	- Listen for an association request
**
** Description:
**      This routine asks GCF to give it an association request. 
**	It is pretty much just an interface to GCA_LISTEN, but 
**      exists to allow the dispatcher (in the CL) to call GCF. 
**      The listen buffer is passed in, so that no stack area 
**      is used, since this routine doesn't know its context.
**
** Inputs:
**      crb                             Pointer to listen buffer.
**      sync				Should this be a synchr. request.
**
** Outputs:
**      none
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**
*/
STATUS
ascd_get_assoc( GCA_LS_PARMS *crb, i4  sync )
{
    DB_STATUS		status;
    STATUS		error;
    i4                  count;
    i4			resume;

    resume = (crb->gca_status == E_GCFFFE_INCOMPLETE) ? GCA_RESUME : 0;

    status = IIGCa_call(GCA_LISTEN,
			(GCA_PARMLIST *)crb,	
			(sync ? GCA_SYNC_FLAG : 0) | resume,
			(PTR)CS_ADDER_ID,
			-1,
			&error);
    if (status)
	sc0e_0_put(error, 0);

    else if (crb->gca_status &&
             crb->gca_status != E_GC0000_OK &&
             crb->gca_status != E_GCFFFE_INCOMPLETE &&
             crb->gca_status != E_GCFFFF_IN_PROCESS
# ifdef NT_GENERIC
             && crb->gca_status != E_GC0032_NO_PEER
# endif
	    )
    {
        /*
        ** If too many consecutive GCA_LISTEN failures,
        ** then shut the server down gracefully.
        */
        if (Sc_main_cb->sc_gclisten_fail++ >= SC_GCA_LISTEN_MAX)
        {
	    sc0e_0_put(E_SC023A_GCA_LISTEN_FAIL, 0);
            CSterminate(CS_CLOSE, &count);
        }
    }

    else
    {
        /*
        ** Since we had a successful GCA_LISTEN, reset the
        ** consecutive failure counter.
        */
        Sc_main_cb->sc_gclisten_fail = 0;
    }

    return(status);
}

/*{
** Name: ascd_disconnect - Disconnect an association for no particular reason
**
** Description:
**      This routine is provided so that the underlying dispatcher 
**      can easily disconnect a session.
**
** Inputs:
**      scb                             Session control block
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/
VOID
ascd_disconnect( SCD_SCB  *scb )
{
    GCA_ER_DATA		*ebuf;
    DB_STATUS		status;
    CS_SCB              *myscb;
    STATUS		gca_status;
    /* lint truncation warning about (i2) cast, but code valid */
    if ((scb->cs_scb.cs_type != (i2) CS_SCB_TYPE) ||
	    (scb->cs_scb.cs_client_type  != SCD_SCB_TYPE))
	return;	/* this is not an SCF scb */
    if ((scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID)) == 0)
	return;	/* Never started GCF, don't end it */

    {
	ebuf = (GCA_ER_DATA *)scb->scb_cscb.cscb_inbuffer;
	ebuf->gca_l_e_element = 1;

	if (scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2)
	{
	    ebuf->gca_e_element[0].gca_local_error = 0;
	    ebuf->gca_e_element[0].gca_id_error = E_SC0206_CANNOT_PROCESS;
	}
	else if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
	{
	    ebuf->gca_e_element[0].gca_local_error = E_SC0206_CANNOT_PROCESS;
	    ebuf->gca_e_element[0].gca_id_error = E_GE98BC_OTHER_ERROR;
	}
	else
	{
	    ebuf->gca_e_element[0].gca_local_error = E_SC0206_CANNOT_PROCESS;
	    ebuf->gca_e_element[0].gca_id_error =
		ss_encode(SS50000_MISC_ERRORS);
	}
	
	ebuf->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
	ebuf->gca_e_element[0].gca_severity = 0;
	ebuf->gca_e_element[0].gca_l_error_parm = 0;
	CSp_semaphore(TRUE, &Sc_main_cb->sc_gcsem);
	Sc_main_cb->sc_gcdc_sdparms.gca_association_id =
	    scb->scb_cscb.cscb_assoc_id;
	Sc_main_cb->sc_gcdc_sdparms.gca_buffer = scb->scb_cscb.cscb_inbuffer;
	Sc_main_cb->sc_gcdc_sdparms.gca_message_type = GCA_RELEASE;
	Sc_main_cb->sc_gcdc_sdparms.gca_msg_length = sizeof(GCA_ER_DATA) -
						     sizeof(GCA_DATA_VALUE);
	Sc_main_cb->sc_gcdc_sdparms.gca_end_of_data = TRUE;
	Sc_main_cb->sc_gcdc_sdparms.gca_modifiers = 0;
	CSget_scb(&myscb);
	status = IIGCa_call(GCA_SEND,
			    (GCA_PARMLIST *)&Sc_main_cb->sc_gcdc_sdparms,
			    GCA_NO_ASYNC_EXIT,
			    (PTR)myscb,
			    -1,
			&gca_status);
	CSv_semaphore(&Sc_main_cb->sc_gcsem);
    }

    if (status)
    {
	sc0e_0_put(gca_status, 0);
    }
}



/*{
** Name:	ascd_allow_connect -- OK for user to connect?
**
** Description:
**	Decides whether the user in the CRB is allowed to make a special
**	connection.  This is based on the SC_LSN_SPECIAL_OK state,
**	and on the user's privileges as determined by the PM file.  We
**	put off looking at the PM file as long as possible to avoid the
**	expense.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	crb		The connection block.
**
** Outputs:
**	none.
**
** Returns:
**	OK
**	error status	to reject the request with.
**
** History:
**	06-may-1998 (canor01)
**	    Check the license before allowing session to connect.
**      15-Feb-1999 (fanra01)
**          Change check license to ice before session connect.
*/

static STATUS
ascd_allow_connect( GCA_LS_PARMS *crb )
{
    STATUS stat;

    CSp_semaphore( 0, &Sc_main_cb->sc_misc_semaphore ); /* shared */
    if( (Sc_main_cb->sc_listen_mask & SC_LSN_REGULAR_OK) &&
       Sc_main_cb->sc_current_conns < Sc_main_cb->sc_max_conns )
    {
	stat = OK;
    }
    else			/* too many regular users */
    {
	stat = E_SC0519_CONN_LIMIT_EXCEEDED;
    }
    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    if ( stat != OK && (Sc_main_cb->sc_listen_mask & SC_LSN_SPECIAL_OK) )
    {
	CSp_semaphore( 0, &Sc_main_cb->sc_misc_semaphore ); /* shared */
	if( Sc_main_cb->sc_current_conns <
	   (Sc_main_cb->sc_max_conns + Sc_main_cb->sc_rsrvd_conns ) )
	{
	    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	    /* is this a deserving user? */
	    
	    if( ascs_chk_priv( crb->gca_user_name, "SERVER_CONTROL" ) )
	    {
		/* Log special connect? */
		stat = OK;
	    }
	    else 
	    {
		stat = E_SC0345_SERVER_CLOSED;
	    }
	}
	else
	    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
    }
    return( stat );
}

static VOID
ascd_pmerr_func(STATUS err_number, i4  num_arguments, ER_ARGUMENT *args)
{
    switch (num_arguments)
    {
	case 0:
	    sc0e_0_put(err_number, (CL_ERR_DESC *)0);
	    break;
	case 1:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    (i4)0, (PTR)0, (i4)0, (PTR)0, (i4)0, (PTR)0,
		    (i4)0, (PTR)0, (i4)0, (PTR)0);
	    break;
	case 2:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    args[1].er_size, args[1].er_value,
		    (i4)0, (PTR)0, (i4)0, (PTR)0, (i4)0, (PTR)0,
		    (i4)0, (PTR)0);
	    break;
	case 3:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    args[1].er_size, args[1].er_value,
		    args[2].er_size, args[2].er_value,
		    (i4)0, (PTR)0, (i4)0, (PTR)0, (i4)0, (PTR)0);
	    break;
	case 4:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    args[1].er_size, args[1].er_value,
		    args[2].er_size, args[2].er_value,
		    args[3].er_size, args[3].er_value,
		    (i4)0, (PTR)0, (i4)0, (PTR)0);
	    break;
	case 5:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    args[1].er_size, args[1].er_value,
		    args[2].er_size, args[2].er_value,
		    args[3].er_size, args[3].er_value,
		    args[4].er_size, args[4].er_value,
		    (i4)0, (PTR)0);
	    break;
	case 6:
	    sc0e_put(err_number, (CL_ERR_DESC *)0, num_arguments,
		    args[0].er_size, args[0].er_value,
		    args[1].er_size, args[1].er_value,
		    args[2].er_size, args[2].er_value,
		    args[3].er_size, args[3].er_value,
		    args[4].er_size, args[4].er_value,
		    args[5].er_size, args[5].er_value);
	    break;
	default:
	    sc0e_0_put(err_number, (CL_ERR_DESC *)0);
	    break;
    }
    return;
}


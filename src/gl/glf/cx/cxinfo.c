/*
** Copyright (c) 2001, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <cv.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <lk.h>
#include    <pm.h>
#include    <st.h>
#include    <tr.h>
#include    <cx.h>
#include    <cxprivate.h>
#include    <sicl.h>

#ifdef VMS
#include <starlet.h>
#include <efndef.h>
#include <iledef.h>
#endif
#ifdef axp_osf
#include    <dlfcn.h>
#endif

/**
**
**  Name: CXINFO.C - Miscellaneous CX informational functions.
**
**  Description:
**      This module contains routines which return information about
**	entities managed by the CX facility.  Routines here can be
**	called even if no "connection" to the CX is active, except
**	for CXunique_id.
**
**
**	Public routines
**
**          CXshm_required	- Bytes required for CX shared memeory.
**
**	    CXcluster_support   - Is cluster supported & configured on
**                                current node at the hardware level.
**
**	    CXcluster_enabled	- Is cluster supported & configured on
**                                current node at hardware, oS, AND 
**				  Ingres level.
**
**	    CXconfig_settings	- Query boolean behavior settings.
**
**	    CXunique_id		- Return a clusterwide unique id.
**
**	Internal functions
**
**	    cx_translate_status	- Translate OS specific error code to
**				  an Ingres error code.
**
**	    cx_config_opt	- Gets CX parameter as an integer index.
**
**  History:
**      06-feb-2001 (devjo01)
**          Created.
**	12-sep-2002 (devjo01)
**	    Add CX_HAS_NUMA_SUPPORT attribute to CXconfig_settings.
**	23-oct-2003 (kinte01)
**	    Add missing sicl.h header file
**	08-jan-2004 (devjo01)
**	    Linux support.
**	11-feb-2004 (devjo01)
**	    Report unexpected osstatus inputs in cx_translate_status().
**	01-apr-2004 (devjo01)
**	    Add CX_HAS_FAST_CLM to CXconfig_settings.
**	    In cx_translate_status have DLM_BAD_DEVICE_PATH yield
**	    E_CL2C20_CX_E_NODLM.
**      04-May-2004 (hanje04)
**          Only pull in DLM specific code on Linux if we're building
**          CLUSTER_ENABLED. Regular Linux builds should not include
**          this code.
**	07-May-2004 (devjo01)
**	    hanje04 inadvertently used CLUSTER_ENABLED where
**	    conf_CLUSTER_BUILD was intended.  Corrected this, and
**	    reworked logic so this is used for all platforms.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	15-apr-2005 (devjo01)
**	    Add CX_HAS_DLM_GLC to CXconfig_settings.
**	22-apr-2005 (devjo01)
**	    VMS porting change submission.
**	12-may-2005 (devjo01)
**	    Restore CX_HAS_DLM_GLC case label lost in last change.
**      21-jun-2005 (devjo01)
**	    Reserve space for CX_GLC_MSG structures, and save some
**	    of the parameters passed in CXshm_required into the CX_PROC_CB.
**      30-Jun-2005 (hweho01)
**          Make the inclusion of Tru64 implementations dependent on
**          define string conf_CLUSTER_BUILD for axp_osf platform.
**	27-oct-2006 (abbjo03)
**	    Add case for SS$_IVLOCKID in cx_translate_status.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**/


/*
**	OS independent declarations
*/
GLOBALREF	CX_PROC_CB	CX_Proc_CB;
GLOBALREF	CX_STATS	CX_Stats;



/*{
** Name: cx_config_opt - Lookup configuration option, and convert to index.
**
** Description:
**      This routine calls PM facility to get a value setting for the
**	requested configuration parameter.  If value is set, a case
**	insensitive comparison is made against elements from a space
**	delimited list of passed choices, prefixed with an implicit 
**	zero'th option of "DEFAULT".   If a match is found, index of
**	selection is returned.  If no match is found an error status
**	is returned.
**
**	If value is not set, selection 0 is implicitly chosen.
**
** Inputs:
**      cfgrsc		- Configuration parameter to lookup.
**	choicelist	- Space delimited list of valid parameter settings.
**
** Outputs:
**	selection	- Buffer to be updated with index of option
**			  selected, or 0 if parameter is not in config.dat
**
**	Returns:
**		OK			- param value valid, or not specified.
**		E_CL2C17_CX_E_BADCONFIG	- param value not in choice list.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-feb-2001 (devjo01)
**	    Created.
*/
STATUS
cx_config_opt( char *cfgrsc, char *choicelist, i4 *selection )
{
#   define CX_MAX_CHOICE_LIST_LEN	128
#   define CX_MAX_CHOICES		 16
    STATUS	 status;
    char	*pmval;
    i4		 choicecnt;
    char	*choices[CX_MAX_CHOICES];
    char	 choicebuf[CX_MAX_CHOICE_LIST_LEN];

    /* Get value from Config file */
    status = PMget( cfgrsc, &pmval );
    if ( status == OK )
    {
	/* copy choice list into writeable buffer */
	(void)STcopy( ERx("DEFAULT "), choicebuf );
	(void)STlcopy( choicelist, choicebuf + 8, CX_MAX_CHOICE_LIST_LEN - 9 );

	/* split choices into words */
	choicecnt = CX_MAX_CHOICES;
	STgetwords( choicebuf, &choicecnt, choices );

	/* Be pessimistic */
	status = E_CL2C17_CX_E_BADCONFIG;

	/* Find choice selected */
	while ( --choicecnt >= 0 )
	{
	    if ( 0 == STbcompare( pmval, 0, choices[choicecnt], 0, TRUE ) )
	    {
		*selection = choicecnt;
		 status = OK;
		break;
	    }
	}
    }
    else
    {
	/* If not specified, then implicitly default to zero. */
	*selection = 0;
	status = OK;
    }
    return status;
} /*cx_config_opt*/





/*{
** Name: cx_translate_status - Convert OS status value to Ingres status.
**
** Description:
**      This routine hopefully simplifies error processing by centralizing
**	conversion of OS specific error codes to their generic Ingres
**	equivalents.   Also error codes are written to the II_DBMS_LOG
**	to assist in field debugging, excepting those codes which
**	are emitted in normal operation
**
**	Care should be taken to be sure that the error codes from
**	different vendor services do not overlap.   If a case arises
**	where they do, we may need to add a context argument.
**
** Inputs:
**      osstatus	- Status code returned by call to vendor function.
**
** Outputs:
**	none
**
**	Returns:
**		Generic Ingres status code equivalent to osstatus.
**		E_CL2C30_CX_E_OS_UNEX_FAIL - Completely unexpected OS status.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-feb-2001 (devjo01)
**	    Created.
**	01-apr-2004 (devjo01)
**	    Have DLM_BAD_DEVICE_PATH yield E_CL2C20_CX_E_NODLM.
**	07-dec-2004 (devjo01)
**	    Have DLM_NEED_MORE_RECOVERY, DLM_DENIED_RD_FAULTS
**	    return E_CL2C2A_CX_E_DLM_RD_DIRTY for OpenDLM.
*/
STATUS
cx_translate_status( i4 osstatus )
{
    STATUS	status;
    char	buf[8];
    i4		rpterr = 0;

    switch ( osstatus )
    {
    case OK:
	/* Zero in is always success */
	status = OK;
	break;

	/*
	**	Start OS dependent declarations.
	*/
#ifdef	CX_NATIVE_GENERIC
	/*
	**	Native generic implementation
	*/
#elif defined(axp_osf) && defined(conf_CLUSTER_BUILD)
	/*
	**	Tru64 implementation
	**
	**  Used with DLM, and IMC functions.
	*/

    case DLM_SYNCH:
	/* Synchronous lock grant. */
	status = E_CL2C01_CX_I_OKSYNC;
	break;

    case DLM_SUCCVALNOTVALID:	/* 2 */
	/* Lock granted, but value block marked invalid */
    case DLM_SYNCVALNOTVALID:	/* 3 */
	/* Lock synchronously granted, but value block marked invalid */
	status = E_CL2C08_CX_W_SUCC_IVB;
	break;

    case DLM_MORE_TO_RECOVER:	/* 4 */
    case DLM_MORE_TO_TRAVERSE:	/* 5 */
	/* Unexpected, since we do not traverse recovery domain */
    case DLM_CANCELWAIT:	/* 21 */
	/* We always specify DLM_CANCELWAITOK to dlm_cancel */
    case DLM_LKBUSY:		/* 32 */
	/* Only happen in dlm_[que]cvt, and is trapped in CXconvert. */
    case DLM_NOLOCKID:		/* 34 */
	/* Defined but Unused */
    case DLM_PARNOTGRANT:	/* 37 */
	/* CX doesn't use parent locks */
    case DLM_RETRY:		/* 38 */
	/* Defined but Unused */
    case DLM_SUBLOCKS:		/* 39 */
	/* CX doesn't use parent locks */
    case DLM_TIMEOUT:		/* 40 */
	/* CX never specifies a time out */
    case DLM_UNEXPERR:		/* 42 */
	/* Defined but Unused (not seen in man pages) */
    case DLM_HASPROCS:		/* 43 */
	/* We never call dlm_destroy. */
    case DLM_KILLFAIL:		/* 44 */
	/* GLC is not "owned" by a particular process, so KILLFAIL
	   should never be emitted when detaching from a GLC. */
    case DLM_IVHNDL:		/* 53 */
	/* CX does not call dlm_rd_collect */
    case DLM_EALREADY:		/* 54 */
	/* CX does not call dlm_rd_collect */
    case DLM_EINPROGRESS:	/* 5
5 */
	/* CX does not call dlm_rd_collect */
    case DLM_PUBNS_DISALLOW:	/* 56 */
	/* CX doesn't use public NS */
	status = E_CL2C30_CX_E_OS_UNEX_FAIL;
	rpterr = 1;
	break;

    case DLM_ABORT:		/* 16 */
	/* lock was unlocked while waiting for grant or cvt. */
    case DLM_CANCEL:		/* 19 */
	/* conversion request was canceled. */
	status = E_CL2C22_CX_E_DLM_CANCEL;
	break;

    case DLM_ATTACHED:		/* 17 */
    case DLM_ENOENT:		/* 25 */
    case DLM_NOACCESS:		/* 33 */
    case DLM_NOTATTACHED:	/* 35 */
    case DLM_TOOMANYNS:		/* 41 */
    case DLM_ATTACHEDGROUP:	/* 45 */
    case DLM_NO_GROUP:		/* 46 */
    case DLM_ENAMETOOLONG:	/* 47 */
    case DLM_TOOMANYRD:		/* 48 */
	/* 
	** NSP, GLC, RD problem, see man pages for dlm_nsjoin|leave,
	** dlm_glc_attach|detach|create, and/or dlm_rd_attach. 
	** Internal logic error in CXconnect/CXdisconnect.
	*/
	status = E_CL2C23_CX_E_DLM_CONNECTION;
	rpterr = 1;
	break;

    case DLM_HASLOCKS:		/* 27 */
	status = E_CL2C23_CX_E_DLM_CONNECTION;
	/* 
	** FIX-ME 
	**
	** We get this because we do not explicitly release
	** some of the locks taken by CX during shut down.
	** These locks die when the process leaves so everything
	** does get cleaned up, but in the interum we get
	** this error.  Still setting status, but surpressing
	** error report.
	*/
	break;

    case DLM_BADPARAM:		/* 18 */
	/* Bad parameter to OS DLM */
    case DLM_IVBUFLEN:		/* 30 */
	/* Resource name length is illegal. */ 
    case DLM_IVLOCKID:		/* 31 */
	/* Bad lock ID */
    case DLM_IS_PR:		/* 49 */
	/* Locking persistent resource as non-persistent */
    case DLM_NOT_PR:		/* 50 */
	/* Locking non-persistent resource as persistent */
    case DLM_IVRDID:		/* 51 */
	/* Invalid recovery domain id */
    case DLM_PR_RD_MISMATCH:	/* 52 */
	/* Locking existing PR with mismatch RD */
	status = E_CL2C32_CX_E_OS_BADPARAM;
	rpterr = 1;
	break;

    case DLM_CANCELGRANT:	/* 20 */
	/* Request was granted before it could be canceled. */
	status = E_CL2C09_CX_W_CANT_CAN;
	break;

    case DLM_CVTUNGRANT:	/* 22 */
	/* Converting a lock not granted to us. CXconvert logic error. */
	status = E_CL2C25_CX_E_DLM_NOTHELD;
	rpterr = 1;
	break;

    case DLM_DEADLOCK:		/* 23 */
	/* What do you think? */
	status = E_CL2C21_CX_E_DLM_DEADLOCK;
	CX_Stats.cx_dlm_deadlocks++;
	break;

    case DLM_EFAULT:		/* 24 */
	/* Internal SEGV to DLM facility.  OS problem? */
	status = E_CL2C31_CX_E_OS_EFAULT;
	rpterr = 1;
	break;

    case DLM_ENOSYS:		/* 26 */
	/* DLM is not up or configured. */
	status = E_CL2C20_CX_E_NODLM;
	rpterr = 1;
	break;

    case DLM_INSFMEM:		/* 28 */
	/* DLM could not alloc mem for its purposes. */
    case IMC_NOMEM:		/*-46 */
	/*Unable to malloc sufficient memory */
    case IMC_INSUFFMEM:		/*-48 */
	/*Not enough memory to hold data being returned */
	status = E_CL2C12_CX_E_INSMEM;
	rpterr = 1;
	break;

    case DLM_INTR:		/* 29 */
	/* dlm_get_rsbinfo, or dlm_rd_attach interrupted */
	status = E_CL2C26_CX_E_DLM_ABORTED;
	rpterr = 1;
	break;

    case DLM_NOTQUEUED:		/* 36 */
	/* LK_NOWAIT specified, and request or conversion could not
	   be synchronously granted. */
	status = E_CL2C27_CX_E_DLM_NOGRANT;
	break;

    case IMC_NOTINIT:		/*-14 */
	/*IMCGSA not initalised*/
	status = E_CL2C33_CX_E_OS_NOINIT;
	break;

    case IMC_MCFULL:		/* -1 */
	/*MC resources exhausted*/
    case IMC_BADPARM:		/* -2 */
	/*Invalid parameter*/
    case IMC_EXISTS:		/* -3 */
	/*spinlock set already created*/
    case IMC_ATTACHED:		/* -4 */
	/*MC region is mapped by some process*/
    case IMC_NOLOCKGOT:		/* -5 */
	/*Lock acquire returned without lock*/
    case IMC_RXFULL:		/* -6 */
	/*Receive mapping fully used*/
    case IMC_BADLOCK:		/* -7 */
	/*Spinlock set does not exist*/
    case IMC_SHARERR:		/* -8 */
	/*Region already mapped as shared*/
    case IMC_NONSHARERR:	/* -9 */
	/*Region already mapped as non-shared*/
    case IMC_PERMIT:		/*-10 */
	/*Permission violation*/
    case IMC_NOTALLOC:		/*-11 */
	/*Cannot map; region not allocated*/
    case IMC_PRIOR:		/*-12 */
	/*Region already allocated by this process*/
    case IMC_LOCKACTIVE:	/*-13 */
	/*Cannot delete spinlock set;lock held*/
    case IMC_NOTSUPER:		/*-15 */
	/*Not super user*/
    case IMC_BADSIZE:		/*-16 */
	/*Invalid size parameter */
    case IMC_OPENERR:		/*-17 */
	/*Bad open on imc device*/
    case IMC_WRITEERR:		/*-18 */
	/*Bad write on imc device*/
    case IMC_CLOSERR:		/*-19 */
	/*Bad close on imc device*/
    case IMC_BADREGION:		/*-20 */
	/*Region not found*/
    case IMC_RECMAPPED:		/*-21 */
	/*Region already mapped for receive by process*/
    case IMC_XMITMAPPED:	/*-22 */
	/*Region already mapped for xmit by process*/
    case IMC_LOCKNOTHELD:	/*-23 */
	/*Tried to release a spinlock not held*/
    case IMC_MC_ERROR:		/*-24 */
	/*Memory channel error */
    case IMC_TIMEOUT:		/*-25 */
	/*Remote node timeout */
    case IMC_NOMCSPACE:		/*-26 */
	/*No memory channel space allocated*/
    case IMC_EINVAL:		/*-27 */
	/*The signal parameter is not a valid signal number*/
    case IMC_ESRCH:		/*-28 */
	/*No process can be found matching the process parameter*/
    case IMC_BADSERVICE:	/*-29 */
	/*Service id is out of range*/
    case IMC_NOTREGISTERED:	/*-30 */
	/*Service is not registered on the slave host */
    case IMC_EPERM:		/*-31 */
	/*Calling process does not have appropriate privilege */
    case IMC_NOROOT:		/*-32 */
	/*Root can not send a remote signal to another host*/
    case IMC_COHERENCYERR:	/*-33 */
	/*Inconsistent coherency flag specification*/
    case IMC_UNDEFINED:		/*-34 */
	/*Undefined return code from RM subsystem*/
    case IMC_KERN_FAULT:	/*-35 */
	/*Operation could not be completed because of a kernel fault */
    case IMC_NORESOURCES:	/*-36 */
	/*Attempt to form coherent allocation on remote node failed*/
    case IMC_LATEJOIN:		/*-37 */
	/*Attempt by late join node to do coherent alloc*/
    case IMC_LOOPBACKERR:	/*-38 */
	/*Region already mapped with a different loopback setting */
    case IMC_BADHOST:		/*-39 */
	/*Host not found in imcgsa_control.hosttable*/
    case IMC_INITERR:		/*-40 */
	/*2nd call to imc_init could not alter max_alloc or max_map_r*/
    case IMC_LOCKPRIOR:		/*-41 */
	/*2nd attempt to acquire a spinlock already owned */
    case IMC_COMPLEMENTERR:	/*-42 */
	/*Coherency complement error*/
    case IMC_BADMAP:		/*-44 */
	/*Attempt to mix kernel and user space locks*/
    case IMC_INTR:		/*-45 */
	/*Function call interrupted by a signal */
    case IMC_CORRUPTLOCK:	/*-47 */
	/*Lockset has been corrupted */
    case IMC_PTPERR:		/*-49 */
	/*Attach PTP Violation*/
    case IMC_NOTSUPPORTED:	/*-50 */
	/*Operation not supported*/
    case IMC_WIRED_LIMIT:	/*-51 */
	/*System wired memory limit encountered*/
    case IMC_MAPENTRIES:	/*-52 */
	/*Process map entry limit exceeded*/
    case IMC_BADADDR:		/*-53 */
	/*User specified address incorrect*/
    case IMC_RAIL_CHANGE:	/*-54 */
	/*Logical Rail config change */
    case IMC_HOST_CHANGE:	/*-55 */
	/*Cluster members change */
    case IMC_MULTIPLE_CHANGE:	/*-56 */
	/*More than one change */
    case IMC_BADRAIL:		/*-57 */
	/*Invalid logical rail number */
    case IMC_WRONGRAIL:		/*-58 */
	/*Region allocated on another rail */
    case IMC_FAILEDOVER:	/*-59 */
	/*Rail failover has occurred*/
    case IMC_NOMAPPER:		/*-60 */
	/*Mapper daemon does not exist*/
    case IMC_INIT_PENDING:	/*-61 */
	/*imc initialisation is waiting on RM layer*/
	/*
	** All the myriad IMC error codes above, are
	** currently mapped to unexpected subsys err.
	**
	** Most of them should be impossible for the
	** subset of IMC routines used.  If experience
	** proves that some of these errors may occur
	** with noticable frequency, they should be
	** broken out to a separate status.
	*/
	status = E_CL2C34_CX_E_OS_SUBSYSERR;
	rpterr = 1;
	break;

#elif defined(LNX) && defined(conf_CLUSTER_BUILD)
    /*
    **	OpenDLM LNX implementation.
    */

    case DLM_DENIED:		/* Attempted to unlock/convert a lock not   */
	status = E_CL2C25_CX_E_DLM_NOTHELD; /* currently in granted status. */
	break;
    case DLM_DENIED_NOLOCKS:    /* request denied, out of system resources */
	status = E_CL2C39_CX_E_OS_CONFIG_ERR;

	break;
    case DLM_SYSERR:		/* System error in async lock request */
	status = E_CL2C30_CX_E_OS_UNEX_FAIL;
	break;
    case DLM_CANCELGRANT:	/* Can't cancel convert: already granted */
	status = E_CL2C09_CX_W_CANT_CAN;
	break;
    case DLM_IVLOCKID:		/* Bad lockid passed to unlock routine. */
	status = E_CL2C25_CX_E_DLM_NOTHELD;
	break;
    case DLM_SYNC:		/* Request granted synchronously. */
	status = E_CL2C01_CX_I_OKSYNC;
	break;
    case DLM_BADTYPE:		/* Bad resource type */
    case DLM_BADRESOURCE:	/* Bad resource handle */
    case DLM_IVBUFLEN:		/* Invalid resource name length */
    case DLM_BADARGS:		/* Bad args passed to API. */ 
    case DLM_BADPARAM:		/* Invalid lock mode specified */
    case DLM_IVGROUPID:		/* invalid group specification */
	status = E_CL2C32_CX_E_OS_BADPARAM;
	rpterr = 1;
	break;
    case DLM_MAXHANDLES:	/* No more resource handles. */
	status = E_CL2C3A_CX_E_OS_CONFIG_ERR;
	rpterr = 1;
	break;
    case DLM_NOLOCKMGR:		/* Can't contact lock manager */
    case DLM_BAD_DEVICE_PATH:	/* DLM not up, or permissions! */
	status = E_CL2C20_CX_E_NODLM;
	break;
    case DLM_NOTQUEUED:		/* NOQUEUE was specified and request failed */
	status = E_CL2C27_CX_E_DLM_NOGRANT;
	break;
    case DLM_CVTUNGRANT:	/* Attempted to convert ungranted lock */  
	status = E_CL2C25_CX_E_DLM_NOTHELD;
	break;
    case DLM_VALNOTVALID:	/* Value block has been invalidated */
	status = E_CL2C08_CX_W_SUCC_IVB;
	break;
    case DLM_REJECTED:          /* Request rejected, unknown client. */
	status = E_CL2C38_CX_E_OS_MISC_ERR; /* May be due to DLM restart */
	break;
    case DLM_ABORT:		/* Blocked lock request cancelled. */
    case DLM_CANCEL:		/* Conversion request cancelled */
	status = E_CL2C22_CX_E_DLM_CANCEL;
	break;
    case DLM_DEADLOCK:		/* Deadlock detection refused this request. */
	status = E_CL2C21_CX_E_DLM_DEADLOCK;
	break;
    case DLM_DENIED_NOASTS:	/* Failed to allocate AST */
	status = E_CL2C3B_CX_E_OS_CONFIG_ERR;
	rpterr = 1;
	break;
    case DLM_TIMEOUT:		/* Lock request timed out. */
	status = E_CL2C0A_CX_W_TIMEOUT;
	break;
    case DLM_NEED_MORE_RECOVERY:
    case DLM_DENIED_RD_FAULTS:
	status = E_CL2C2A_CX_E_DLM_RD_DIRTY;
	break;

#elif defined(VMS) /* Start _VMS_ */
    case SS$_NORMAL:		/* Normal successful completion */
	status = OK;
	break;
    case SS$_SYNCH:		/* Lock synchronously granted, expect no AST */
	status = E_CL2C01_CX_I_OKSYNC;
	break;
    case SS$_VALNOTVALID:	/* Value block has been invalidated */
	status = E_CL2C08_CX_W_SUCC_IVB;
	break;
    case SS$_CANCELGRANT:	/* Can't cancel convert: already granted */
	status = E_CL2C09_CX_W_CANT_CAN;
	break;
    case SS$_DEADLOCK:		/* Deadlock detection refused this request. */
	status = E_CL2C21_CX_E_DLM_DEADLOCK;
	break;
    case SS$_ABORT:		/* Canceled before it could be granted. */
    case SS$_CANCEL:		/* Conversion canceled before granted. */
	status = E_CL2C22_CX_E_DLM_CANCEL;
	break;
    case SS$_CVTUNGRANT:	/* Converting a lock not held. */
	status = E_CL2C25_CX_E_DLM_NOTHELD;
	CS_breakpoint();
	break;
    case SS$_NOTQUEUED:		/* Can't grant immediately & NOWAIT set. */
	status = E_CL2C27_CX_E_DLM_NOGRANT;
	break;
    case SS$_ACCVIO:		/* LSB or resource name is bad pointer. */
    case SS$_EXDEPTH:		/* Parent locks !used, should NEVER see this */
    case SS$_PARNOTGRANT:	/* Parent locks !used, should NEVER see this */
    case SS$_SUBLOCKS:		/* Parent locks !used, should NEVER see this */
    case SS$_IVBUFLEN:		/* Bad resource name length. (0, or > 31)
    case SS$_NOSYSLCK:		/* Never ask for syslocks, should NEVER see */
    case SS$_ILLRSDM:		/* Illegal attempt to modify a LVB */
    case SS$_IVLOCKID:		/* Invalid lock ID for that process */
	status = E_CL2C25_CX_E_DLM_NOTHELD;
	break;
    case SS$_NOLOCKID:		/* Bad lock ID or privilege problem. */
	status = E_CL2C39_CX_E_OS_CONFIG_ERR;
	break;
    case SS$_INSFMEM:		/* System dynamic memory exhausted. */
	status = E_CL2C3A_CX_E_OS_CONFIG_ERR;
	rpterr = 1;
	break;
    case SS$_EXENQLM:		/* Process enqueue limit exceeded. */
	status = E_CL2C3B_CX_E_OS_CONFIG_ERR;
	break;
    /* end _VMS_ */
#else
    /*
    **	All other platforms
    */
#endif /*OS SPECIFIX*/
    /*
    **	End OS dependent declarations.
    */

    default:
	status = E_CL2C30_CX_E_OS_UNEX_FAIL;
	rpterr = 1;
    }
    if ( rpterr )
    {
	CX_Stats.cx_severe_errors++;
	sprintf( buf,"%04X", (unsigned short)(status & 0x0ffffl) );
	(void)TRdisplay( 
	 ERx("%@ Error E_CL%s in CX facility. OS status code = %d.\n"),
	 buf, osstatus );
	SIfprintf(stderr,"Error E_CL%s in CX facility. OS status code = %d.\n",
	 buf, osstatus );
    }
    return status;
} /*cx_translate_status*/



/*{
** Name: CXshm_required - Return amount of shared memory required by CX.
**
** Description:
**      Calculate amount of shared memory required by the CX facility.
**
**	Currently, this is simply the size of CX_NODE_CB.
**
** Inputs:
**      flags		- Bitmask of valid flags.  (currently ignored.)
**	nodes		- Max # nodes to support. (currently ignored.)
**	transactions	- Max # transactions to support. (currently ignored.)
**	locks		- Max # locks to support. (currently ignored.)
**	resources	- Max # lockable resources. (currently ignored.)
**
** Outputs:
**	none.
**
**	Returns:
**		number of bytes required.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-feb-2001 (devjo01)
**	    Created.
*/
u_i4
CXshm_required( u_i4 flags, u_i4 nodes, u_i4 transactions,
	        u_i4 locks, u_i4 resources )
{
    u_i4	bytes;

    /*
    **	Start OS dependent declarations.
    */
#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */
    bytes = 0;
#elif defined(axp_osf) || defined(LNX) || defined(VMS)
    /*
    **	Linux & Tru64 implementations
    */
    bytes = sizeof(CX_NODE_CB);

    /* Stuff values in CX_Proc_CB, JIC we need them later. */
    CX_Proc_CB.cx_transactions_required = transactions;
    CX_Proc_CB.cx_locks_required = locks;
    CX_Proc_CB.cx_resources_required = resources;
#   if defined (VMS)

    /* VMS allocates additional memory for CX_GLC_MESSAGES */
    bytes += (1 + transactions) * sizeof(CX_GLC_MSG);

#   endif /* VMS */
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    bytes = 0;
#endif /*OS SPECIFIX*/
    /*
    **	End OS dependent declarations.
    */
    return bytes;
} /*CXshm_required*/





/*{
** Name: CXcluster_support	- Report if OS cluster support available.
**
** Description:
**
**	This routine tests to see if clustering is available based on the OS,
**	hardware and vendor configuration.  This must return TRUE for
**	Ingres cluster support to be enabled, even if Ingres clustering
**	is specified in the Ingres configuration.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Machine is properly configured for OS cluster.
**		FALSE	- No cluster support either for OS as a whole,
**			  or just the current machine.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-feb-2001 (devjo01)
**	    Created.
**	26-jul-2001 (devjo01)
**	    Dynamically load clu_info.  We don't bother keeping
**	    handle to libclu open, since this is typically only
**	    called once per process.
*/
bool
CXcluster_support( void )
{
    bool	clusterok = FALSE;

    /*
    **	Start OS dependent declarations.
    */
#if !defined(conf_CLUSTER_BUILD)
    /*
    **	If not a cluster build, NO cluster.
    */
#elif defined(axp_osf)
    /*
    **	Tru64 implementation
    */
    {
	memberid_t	 memberid;
	char		 memberstatus[CLU_MAX_MEMBERID + 1];
	void		*libhandle;
	int		 (*pclu_info)(int, ... );

	libhandle = dlopen( ERx("libclu.so"), RTLD_LAZY );
	if ( NULL != libhandle )
	{
	    pclu_info = (int (*)(int, ... ))dlsym( libhandle, ERx("clu_info") );
	    if ( NULL != pclu_info )
	    {
		if ( OK == (*pclu_info)( CLU_INFO_MY_ID, &memberid ) )
		{
		    if ( memberid > 0 && memberid <= CLU_MAX_MEMBERID )
		    {
			memberstatus[memberid] = CLU_MEMB_CONF_DOWN;
			(void)(*pclu_info)( CLU_INFO_MEMBSTATE,
			  sizeof(memberstatus), memberstatus );
			if ( CLU_MEMB_CONF_UP == memberstatus[memberid] )
			    clusterok = TRUE;
		    }
		}
	    }
	    (void)dlclose( libhandle );
	}
    }
#elif defined(LNX)
    /*
    **  Linux implementation.
    */
    /* <<<FIX-ME>>> - Don't know how to check, so be giddily optimistic. */
    clusterok = TRUE;

#elif defined(VMS) /* Start _VMS_ */
    /*
    **  OpenVMS implementation.
    */
	
    /* Check for test override */
    if ( CX_PF_ONE_NODE_OK & CX_Proc_CB.cx_flags )
    {
	clusterok = TRUE;
    }
    else
    {
#	define VMS_FAIL(x) (((x) & 1) == 0)

	int	osstatus;
	unsigned char cmb;
	ILE3  syi_list[2] = {   
		{ sizeof(cmb), SYI$_CLUSTER_MEMBER, &cmb, 0 },
		{ 0, 0, 0, 0 }
	};
	struct _iosb	myiosb;
	char	*evp;

	NMgtAt( ERx("II_CLUSTER"), &evp );
	if ( NULL != evp && *evp != '\0' )
	{
	    clusterok = TRUE;
	}
	else
	{
	    osstatus = sys$getsyiw(EFN$C_ENF, NULL, NULL, syi_list, &myiosb, 0, 0);
	    if ( VMS_FAIL(osstatus) || VMS_FAIL(myiosb.iosb$w_status) ||
		 !(0x1 & cmb) )
		clusterok = FALSE;
	    else
		clusterok = TRUE;
	}
    }

    /* end _VMS_ */
#else
    /*
    **	All other platforms
    */
#endif /*OS SPECIFIX*/
    /*
    **	End OS dependent declarations.
    */
    return clusterok;
} /* CXcluster_support */




/*{
** Name: CXcluster_enabled	- Report if OS cluster support available
**				  and Ingres cluster configured.
**
** Description:
**
**	This routine calls CXcluster_support to see if clustering is 
**	available based on the OS, hardware and vendor configuration,
**	and then checks to see if Ingres is configured to exploit this.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- Installation & Machine are configured for clustering.
**		FALSE	- Either OS, machine, or Ingres was not configured
**			  for cluster support.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    CX_Proc_CB.cx_clustering set if not already done so.
**
** History:
**	07-mar-2001 (devjo01)
**	    Created.
*/
bool
CXcluster_enabled( void )
{
    if ( !(CX_Proc_CB.cx_flags & CX_PF_ENABLE_FLAG_VALID) )
    {
	if ( ( CXnode_number( NULL ) > 0 ) && CXcluster_support() )
	    CX_Proc_CB.cx_flags |= CX_PF_ENABLE_FLAG;
	CX_Proc_CB.cx_flags |= CX_PF_ENABLE_FLAG_VALID;
    }
    /* Note: following depends on CX_PF_ENABLE_FLAG being bit 0. */
    return (bool)( CX_Proc_CB.cx_flags & CX_PF_ENABLE_FLAG );
} /* CXcluster_enabled */




/*{
** Name: CXconfig_settings	- Get value for various TRUE/FALSE settings.
**
** Description:
**
**	This routine is the interface for querying certain TRUE/FALSE
**	config options.   Settings are sometimes fixed by OS, sometimes
**	are controlled by configuration options, and sometimes are
**	determined by the run-time circumstances of the calling process.
**
** Inputs:
**	setting		- One of the following values:
**
**	    CX_HAS_OS_CLUSTER_SUPPORT	OS potentially supports Ingres clusters.
**	    CX_HAS_OS_CLUSTER_ENABLED	OS cluster support is enabled.
**	    CX_HAS_CX_CLUSTER_ENABLED	OS & CX cluster support is enabled.
**	    CX_HAS_PCONTEXT_LOCKS_ONLY	Only process context locks supported.
**	    CX_HAS_DEADLOCK_CHECK	CX will take care of deadlock check.
**	    CX_HAS_CSP_ROLE		CX has CSP role for calling process.
**	    CX_HAS_1ST_CSP_ROLE		CX was 1st CSP in a cluster.
**	    CX_HAS_MASTER_CSP_ROLE	CX is current master CSP.
**	    CX_HAS_NUMA_SUPPORT		Machine & OS has NUMA arch. & support.
**	    CX_HAS_FAST_CLM		Configured for fast CX long messages.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- CX has attribute.
**		FALSE	- CX lacks attribute.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	23-apr-2001 (devjo01)
**	    Created.
**	12-sep-2002 (devjo01)
**	    Add CX_HAS_NUMA_SUPPORT.
**	01-apr-2004 (devjo01)
**	    Add CX_HAS_FAST_CLM.
**	15-apr-2005 (devjo01)
**	    Add CX_HAS_DLM_GLC.
*/
bool
CXconfig_settings( u_i4 setting )
{
    switch ( setting )
    {
# if defined (axp_osf) || defined(VMS) || defined(LNX)
    case CX_HAS_OS_CLUSTER_SUPPORT:
	/* Ingres CAN exploit clustering on this platform */
	return TRUE;
# endif
# if defined (axp_osf) || defined(LNX)
    case CX_HAS_DLM_GLC:
	/* Platform uses a DLM providing a group lock container */
	/* drop into next case */

    case CX_HAS_DEADLOCK_CHECK:
	/* Platform uses a DLM providing transaction support */
	return TRUE;
# endif

    case CX_HAS_OS_CLUSTER_ENABLED:
	return CXcluster_support();

    case CX_HAS_CX_CLUSTER_ENABLED:
	return CXcluster_enabled();

    case CX_HAS_CSP_ROLE:
	return ( CX_Proc_CB.cx_flags & CX_PF_IS_CSP ) != 0;

    case CX_HAS_1ST_CSP_ROLE:
	return ( CX_Proc_CB.cx_flags & CX_PF_FIRSTCSP ) != 0;

    case CX_HAS_MASTER_CSP_ROLE:
	return ( CX_Proc_CB.cx_flags & CX_PF_MASTERCSP ) != 0;

    case CX_HAS_NUMA_SUPPORT:
	return CXnuma_support();

    case CX_HAS_FAST_CLM:
	return (CX_CLM_IMC == CX_Proc_CB.cx_clm_type);

    default:
	return FALSE;
    }
} /* CXconfig_settings */



/*{
** Name: CXunique_id	- Get a cluster wide unique ID.
**
** Description:
**
**	This creates a cluster wide unique ID, by putting the
**	node number of the callers node into the high order
**	part of the passed ID buffer, then setting the lower
**	order portion to a monitonically increasing sequence
**	number.  Since we are reserving 56 bits for this seqence,
**	we won't worry about overflow.
**	
**	We could have used CMO for this, but since we only assert
**	uniqueness, and not adherence to any order, this was not
**	necessary, and the overhead is greater.
**
**	Note: In contrast to all the other functions in this module
**	this should not be called until CXconnect has been called.
**
** Inputs:
**	
**	punique	- Pointer to buffer to hold unique id.
**
** Outputs:
**
**	*punique - updated with unique ID.
**
**	Returns:
**		OK			 - Normal successful completion.
**		E_CL2C1B_CX_E_BADCONTEXT - CXconnect not yet called.
**		E_CL2C24_CX_E_DLM_SPINOUT - spin count exhausted
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none.
**
** History:
**	05-mar-2001 (devjo01)
**	    Created.
*/
STATUS
CXunique_id( LK_UNIQUE *punique )
{
    i4		 i;
    CX_NODE_CB	*ncb;

    if ( !( ncb = CX_Proc_CB.cx_ncb ) )
    {
	return E_CL2C1B_CX_E_BADCONTEXT;
    }
    punique->lk_uhigh = (CX_Proc_CB.cx_node_num << 24);
    i = (1<<20);
    while ( --i > 0 )
    {
	if ( CS_TAS( &ncb->cx_uniqprot ) )
	{
	    punique->lk_uhigh |= ncb->cx_unique.lk_uhigh;
	    if ( 0 == ( punique->lk_ulow = ++ncb->cx_unique.lk_ulow ) )
		punique->lk_uhigh++;
	    CS_ACLR( &CX_Proc_CB.cx_ncb->cx_uniqprot );
	    break;
	}
    }
    if ( !i )
	return E_CL2C24_CX_E_DLM_SPINOUT;
    return OK;
} /*CXunique_id*/

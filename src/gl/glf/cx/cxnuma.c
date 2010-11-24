/*
** Copyright (c) 2002, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <cv.h>
#include    <pc.h>
#include    <cs.h>
#include    <er.h>
#include    <nm.h>
#include    <lk.h>
#include    <pm.h>
#include    <st.h>
#include    <tr.h>
#include    <me.h>
#include    <cx.h>
#include    <cxprivate.h>
#include "cxlocal.h"

/* If non-zero, allow user to override NUMA shared library name. */
#define ENABLE_NUMA_DEBUG_LIB	0	/* Turn off before ship! */

/**
**
**  Name: CXNUMA.C - Non-Uniform Memory Access architecture support.
**
**  Description:
**      This module contains routines which manage NUMA aspects of
**	hardware platforms and OS'es that support NUMA.
**
**	Ingres NUMA configurations come in two flavors.   In the
**	first, only one Ingres instance is configured per box, either
**	as a stand-alone Ingres instance, or as one node of an Ingres
**	cluster, and the NUMA aspect of the configuration only effects
**	memory allocation strategies, and processor affinity.
**
**	In the second style, multiple Ingres instances exist on the
**	same box (only one GCN though), with each Ingres instance
**	performing as a virtual node of an Ingres cluster.  These
**	virtual nodes may or may not be part of a larger Ingres
**	cluster.   This allows exploitation of multiple RADs within
**	a box while keeping memory accesses local.
**
**	Note: A few NUMA routines which need not reference CS, or
**	esoteric OS libraries, are part of cxapi.c to alow them to be
**	part of libingres.
**
**	Public routines
**
**	    CXnuma_support	- NUMA is supported & configured on
**                                current node at hardware, OS levels, 
**				  and SUPPORTED at the Ingres level.
**
**	    CXnuma_rad_count	- Number of RADs on current box. (0
**				  if no NUMA support)
**
**	    CXnuma_cluster_rad	- Returns RAD ID of caller if running
**				  NUMA clusters, else 0
**
**	    CXnuma_shmget	- Shared memory allocator that
**				  hides the nasty little details
**				  of allocating shared memory
**				  with a particular allocation
**				  policy.
**
**	    CXnuma_bind_to_rad	- Set process (and process thread)
**				  affinity to a specific RAD.
**
**	    CXnuma_rad_configured - Check if RAD is configured by ingres.
**
**	    CXnuma_get_info	- Fill in a CX_RAD_INFO structure
**				  for the given RAD.
**
**	    CXnuma_free_info	- Free memory resources that may be
**			          attached to a CX_RAD_INFO filled
**				  in by CXnuma_get_info.
**
**	Depreciated routines:
**
**	    CXnuma_enabled	- NUMA is SUPPORTED & CONFIGURED on
**                                current node at hardware, OS, and 
**				  Ingres levels.
**
**	    CXnuma_cluster_enabled - NUMA is supported & configured on
**                                current node at hardware, OS, AND 
**				  Ingres level to support multiple
**				  virtual nodes on current box.
**
**  History:
**	12-sep-2002 (devjo01)
**	    Created.
**	27-oct-2003 (devjo01)
**	    In CXnuma_shmget, rather than simply do an unbound memory
**	    allocation if NUMA not supported, or configured, fail, and
**	    set *perrno to ENOSYS, and let caller decide what to do.
**	06-jan-2004 (devjo01)
**	    Rename cx_axp_dl_load_lib to cx_dl_load_lib for Linux support.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      30-Jun-2005 (hweho01)
**          Make inclusion of Tru64 implementation dependent on  
**          conf_CLUSTER_BUILD define string for axp_osf platform.
**	07-jan-2008 (joea)
**	    Discontinue use of cluster nicknames.
**      03-nov-2010 (joea)
**          Include cxlocal.h for prototypes.
**/


/*
**	OS independent declarations
*/
GLOBALREF	CX_PROC_CB	CX_Proc_CB;
GLOBALREF	CX_STATS	CX_Stats;

/*
**	OS dependent declarations
*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD) 
static radid_t cx_axp_numa_rad_to_osradset( i4, radset_t * );

static char * cx_axp_rad_id_decoder( PTR, char *, i4 );
#endif


/*{
** Name: CXnuma_support	- Report if NUMA support available.
**
** Description:
**
**	This routine tests to see if NUMA specific operations are
**	available at the Hardware, OS, and Ingres levels.
**
**	On Tru64, this should be called on entry to any routine
**	making use of OS NUMA functions, since the first time it is
**	called it resolves all the required entry points into the
**	NUMA shared library.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- CX NUMA functionality is available.
**		FALSE	- NUMA is not supported.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    Under Tru64 function pointers to all utilized OS NUMA
**	    functions are resolved.
**
** Note:
**	    This can be called before CXinitialize.
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_support( void )
{
    bool	numaok;

    if ( CX_Proc_CB.cx_flags & CX_PF_HAVENUMA_FLAG_VALID )
    {
	return 0 != ( CX_Proc_CB.cx_flags & CX_PF_HAVENUMA_FLAG );
    }

    numaok = FALSE;

    /*
    **	Start OS dependent declarations.
    */
#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */

#else /*CX_NATIVE_GENERIC*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	static char *cxaxp_numa_fnames[] = {
	    "rad_get_num", "nshmget", "memalloc_attr",
	    "radsetcreate", "radsetdestroy", "radaddset",
	    "radfillset", "rad_foreach", "radisemptyset",
	    "rad_attach_pid", "rad_get_info", "cpusetcreate",
	    "cpusetdestroy", "cpucountset"
	};

	STATUS	status;
	char	*numalib = "libnuma.so";

#if	ENABLE_NUMA_DEBUG_LIB
	{
	    char	*numadlname;

	    NMgtAt("II_NUMA_DEBUG_LIB", &numadlname);
	    if ( numadlname && *numadlname )
		numalib = numadlname;
	}
#endif /*ENABLE_NUMA_DEBUG_LIB*/
	
	status = cx_dl_load_lib( numalib,
		     &CX_Proc_CB.cxaxp_numa_lib_hndl,
		     CX_Proc_CB.cxaxp_numa_funcs, cxaxp_numa_fnames,
		     sizeof(cxaxp_numa_fnames)/sizeof(cxaxp_numa_fnames[0]));
	if ( OK == status )
	{
	    if ( RAD_GET_NUM() > 1 )
	    {
		CX_Proc_CB.cx_flags |= CX_PF_HAVENUMA_FLAG;
		numaok = TRUE;
	    }
	}
    }
 
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
#endif /*axp_osf (Tru64)*/
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */
    CX_Proc_CB.cx_flags |= CX_PF_HAVENUMA_FLAG_VALID;
    return numaok;
} /* CXnuma_support */


# if 0	/* Depreciated functions */
/*{
** Name: CXnuma_enabled	- Report if NUMA is enabled for this NODE.
**
** Description:
**
**	Depreciated 
**
**	We really are only interested in two attributes.  "Is NUMA supported?",
**	and "Is NUMA clustering configured?".   Oddly enough, there really is
**	no physical dependency between the two.  We could manually edit the
**	configure to make a NUMA cluster node on a non-NUMA box (a useless
**	exercise in the field).   Conversely, we can take advantage of
**	RAD affinity even without configuring for clusters.  Having a separate
**	CXnuma_enabled function simply muddied things.
**
**	Old desc:
**
**	This routine tests to see if NUMA specific operations are
**	available at the Hardware, OS, and Ingres levels, and
**	if so, whether Ingres is configured to exploit it.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- CX NUMA functionality is enabled.
**		FALSE	- NUMA is not supported, or is not configured.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    node (see CXnuma_support)
**
** Note:
**	    This can be called before CXinitialize.
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_enabled( void )
{
    if ( !(CX_Proc_CB.cx_flags & CX_PF_USENUMA_FLAG_VALID) )
    {
	if ( CXnuma_support() && CXnuma_configured() )
	    CX_Proc_CB.cx_flags |= CX_PF_USENUMA_FLAG;
	CX_Proc_CB.cx_flags |= CX_PF_USENUMA_FLAG_VALID;
    }
    return 0 != ( CX_Proc_CB.cx_flags & CX_PF_USENUMA_FLAG );
} /* CXnuma_enabled */


/*{
** Name: CXnuma_cluster_enabled	- Report if NUMA clustering is on for this NODE.
**
** Description:
**
**	Depreciated.
**
**	All we really care is if it is configured!
**
**	Concievably, a client could migrate from NUMA to non-NUMA
**	keeping his old disks.  Ingres better keep the same shared
**	memory and file mapping characteristics as before.
**
**	Old desc:
**
**	This routine tests to see if NUMA clustering operations are
**	available at the Hardware, OS, and Ingres levels, and
**	if so, whether Ingres is configured to exploit it.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE	- CX NUMA clustering is enabled.
**		FALSE	- NUMA clustering is not supported,
**			  or is not configured.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    node (see CXnuma_support)
**
** Note:
**	    This can be called before CXinitialize.
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_cluster_enabled( void )
{
    if ( !(CX_Proc_CB.cx_flags & CX_PF_USENUMACLUST_FLAG_VALID) )
    {
	if ( CXnuma_support() && CXnuma_cluster_configured() )
	    CX_Proc_CB.cx_flags |= CX_PF_USENUMACLUST_FLAG;
	CX_Proc_CB.cx_flags |= CX_PF_USENUMACLUST_FLAG_VALID;
    }
    return 0 != ( CX_Proc_CB.cx_flags & CX_PF_USENUMACLUST_FLAG );
} /* CXnuma_cluster_enabled */

# endif /* depreciated */


/*{
** Name: CXnuma_rad_count	- Report if NUMA support available.
**
** Description:
**
**	Return the number of Resource Affinity Domains on the
**	current hardware.   If no NUMA support, then zero (0)
**	is returned.   By convention, Ingres will identify
**	each RAD with an identifyer in the range 1 .. 'n'
**	where 'n' is the RAD count.  If needed, mapping is
**	performed, to and from this Ingres identifyer to
**	the ident used by the OS.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		0	- No NUMA support.
**		1	- NUMA concepts are in place, but resources
**		          on this box are eqi-distant (non-NUMA box).
**		> 1	- Number of RADs on NUMA organized hardware.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** Note:
**	    This can be called before CXinitialize.
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
*/
i4
CXnuma_rad_count( void )
{
    i4  	rads = 0;

    if ( CXnuma_support() )
    {

	/*
	**	Start OS dependent declarations.
	*/
#ifdef	CX_NATIVE_GENERIC
	/*
	**	Native generic implementation
	*/
	;
#else /*CX_NATIVE_GENERIC*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD) 
	/*
	**	Tru64 implementation
	*/
	rads = RAD_GET_NUM();
 
#else /*axp_osf (Tru64)*/
	/*
	**	All other platforms
	*/
	;
#endif /*axp_osf (Tru64)*/
#endif /*CX_NATIVE_GENERIC*/
	/*
	**	End OS dependent declarations.
	*/
    }
    return rads;
} /* CXnuma_rad_count */


/*{
** Name: CXnuma_user_rad	- Return the RAD affiliation.
**
** Description:
**
**	If NUMA clustering is in use, then RAD affiliation is a rigid
**	and defining characteristic of a NUMA cluster node, and
**	CXnuma_user_rad will return the same number as CXnuma_cluster_rad.
**
**	However, RAD affiliation can also be set without being configured
**	for NUMA clusters.  In this case, we want the memory and process
**	affiliation, without changing the way files and shared memory
**	are mapped.  In these configurations CXnuma_cluster_rad will
**	return 0, while CXnuma_user_rad will return the RAD assignment.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		0	- No RAD affiliation.
**		> 1	- RAD affiliation of caller.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** Note:
**	    This can be called before CXinitialize.
**
** History:
**	25-sep-2002 (devjo01)
**	    Created.
*/
i4
CXnuma_user_rad( void )
{
    return CX_Proc_CB.cx_numa_user_rad;
} /* CXnuma_user_rad */


/*{
** Name: CXnuma_cluster_rad	- Report NUMA clustering RAD assignment.
**
** Description:
**
**	Return the RAD affiliation of the caller if host is configured
**	for NUMA clustering.
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**		0	- No NUMA clustering on this host.
**		> 1	- RAD ID of caller.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** Note:
**	    This can be called before CXinitialize.
**	    RAD context is initially established through
**	    CXset_context, usually through CXget_context.
**	    This routine will return always return 0
**	    if CXset_context has not been called.
**
** History:
**	25-sep-2002 (devjo01)
**	    Created.
*/
i4
CXnuma_cluster_rad( void )
{
    return CX_Proc_CB.cx_numa_cluster_rad;
} /* CXnuma_cluster_rad */


/*{
** Name: CXnuma_shmget	- Allocate shared memory from mem on current RAD.
**
** Description:
**
**	This routine allocates shared memory using whatever OS facilities
**	are available to force/suggest that this memory be taken from
**	physical memory local to the RAD assigned to the calling process.
**
**	CXnuma_bind_to_rad() must be called before this routine, to
**	set the home RAD for the process.
**
** Inputs:
**	i4	key	- see man page for shmget.
**	i4	size	- Amount of memory to allocate.
**	i4	flags	- Mixture of allocation & permission flags.
**
** Outputs:
**	i4	*perrno - updated with errno if failure, or 0 if
**		          success.
**
**	Returns:
**		-1	- Failure.
**		shmid	- integer value representing new shared
**			  memory segment.  Must be attached to
**			  before an address to this is available.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** Notes:
**	    This is literally nothing more than shmget() on platforms
**	    and configurations which do not support NUMA.
**
**	    The decision to put this here rather than in ME caused
**	    much agony, but since either way would have been ugly,
**	    this module won out as less ME code wound up in CX than
**	    visa-versa.
**
** Note:
**	    This CAN be called before CXinitialize.  This is
**	    an important consideration, since CXinitialize
**	    takes a pointer to a shared memory area to use
**	    for it's own purposes and we want this memory
**	    local to the RAD. 
**	
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
**	27-oct-2003 (devjo01)
**	    Rather than simply do an unbound memory allocation
**	    if NUMA not supported, or configured, fail, and set
**	    *perrno to ENOSYS, and let caller decide what to do.
*/
i4
CXnuma_shmget( i4 key, i4 size, i4 flags, i4 *perrno )
{
    i4		shmid;

    shmid = -1;
    *perrno = EINVAL;

    if ( !CX_Proc_CB.cx_numa_user_rad || !CXnuma_support() )
    {
	*perrno = ENOSYS;
	return shmid;
    }

    /*
    **	Start OS dependent declarations.
    */
#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */

#else /*CX_NATIVE_GENERIC*/
#if defined(axp_osf)  && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	memalloc_attr_t attr; 

	MEfill(sizeof(attr), '\0', (PTR)&attr);

	/* 1st populate a rad set consisting only of our RAD */
	attr.mattr_rad =
	  cx_axp_numa_rad_to_osradset(CX_Proc_CB.cx_numa_user_rad,
	  &attr.mattr_radset);

	if ( RAD_NONE == attr.mattr_rad )
	{
	    shmid = shmget(key, size, flags);
	}
	else
	{
	    /* Populate memalloc_attr_t for an arbitrary address. */
	    (void)MEMALLOC_ATTR((vm_offset_t)&attr, &attr); 

	    /* Fill in what we want set */
	    attr.mattr_policy = MPOL_DIRECTED;

	    /* Allocate with our new policy */
	    shmid = NSHMGET( (key_t)key, (size_t)size, (int)flags, &attr );
	}
	*perrno = errno;

	RADSETDESTROY(&attr.mattr_radset); 
    }
 
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
#endif /*axp_osf (Tru64)*/
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */
    return shmid;
} /* CXnuma_shmget */


/*{
** Name: CXnuma_rad_configured - Check if RAD configured for a given host
**
** Description:
**
**	This routine checks to set if a rad has been configured for
**	use with NUMA clusters.
**
** Inputs:
**	char	host	- Host to check.  If NULL, then current host.
**	i4	rad	- Ingres RAD ID.
**
** Outputs:
**	none.
**
**	Returns:
**		TRUE			- RAD is configured.
**		FALSE			- RAD is not configured.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-sep-2002 (devjo01)
**	    Created.
*/
bool
CXnuma_rad_configured( char *host, i4 rad )
{
    i4                   i, result = FALSE;
    CX_CONFIGURATION	*cluster_config;
    CX_NODE_INFO	*pni;

    (void)CXcluster_nodes( NULL, &cluster_config );

    if ( !host ) host = PMhost();

    for ( i = 1; i <= cluster_config->cx_node_cnt; i++ )
    {
        pni = cluster_config->cx_nodes + cluster_config->cx_xref[i];
        if ( ( pni->cx_host_name_l && 0 == STxcompare( host, 0,
	       pni->cx_host_name, pni->cx_host_name_l, TRUE, FALSE ) ) ||
             ( pni->cx_node_name_l && 0 == STxcompare( host, 0,
	       pni->cx_node_name, pni->cx_node_name_l, TRUE, FALSE ) ) )
        {
            if ( rad == pni->cx_rad_id )
	    {
		result = TRUE;
	    }
        }
    }
    return result;
} /* CXnuma_rad_configured */


/*{
** Name: CXnuma_bind_to_rad - Set process affinity.
**
** Description:
**
**	This routine sets a process affinity to a specific RAD for
**	the executing process.
**
** Inputs:
**	i4	rad	- Ingres RAD ID.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- All is well.
**		E_CL2C2E_CX_E_NONUMA	- NUMA not supported/configured
**		E_CL2C2F_CX_E_BADRAD	- RAD out of range.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    OS is asked that process (and all component threads) 
**	    be run on the specified RAD.   Details of OS scheduler
**	    will determine how well this is respected.
**
** History:
**	12-sep-2002 (devjo01)
**	    Created.
*/
i4
CXnuma_bind_to_rad( i4 rad )
{
    STATUS	status = OK;

    if ( !CXnuma_support() )
    {
	return E_CL2C2E_CX_E_NONUMA;
    }

    /*
    **	Start OS dependent declarations.
    */
#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */
    return E_CL2C2E_CX_E_NONUMA;

#else /*CX_NATIVE_GENERIC*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	radset_t	radset;
	radid_t		radid;

	radid = cx_axp_numa_rad_to_osradset(rad, &radset);

	if ( RAD_NONE == radid )
	{
	    status = E_CL2C2F_CX_E_BADRAD;
	}
	else
	{
	    if ( 0 != RAD_ATTACH_PID( (pid_t)0, radset, 0 ) )
	    {
		status = E_CL2C2D_CX_E_NOBIND;
	    }
	    else
	    {
		CX_Proc_CB.cx_numa_user_rad = rad;
	    }
	}
		
	RADSETDESTROY(&radset); 
    }
 
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    return E_CL2C2E_CX_E_NONUMA;
#endif /*axp_osf (Tru64)*/
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */
    return status;
} /* CXnuma_bind_to_rad */


/*{
** Name: CXnuma_get_info - Return info about a RAD.
**
** Description:
**
**	This routine fills in a CX_RAD_INFO struct for the requested RAD.
**
** Inputs:
**	i4		rad	- Ingres RAD ID.
**	CX_RAD_INFO *radinfobuf - Buffer to hold information.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- All is well.
**		E_CL2C2E_CX_E_NONUMA	- NUMA not supported/configured
**		E_CL2C2F_CX_E_BADRAD	- RAD out of range.
**		E_CL2C38_CX_E_OS_MISC_ERR - Unexpected failure.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    OS is asked that process (and all component threads) 
**	    be run on the specified RAD.   Details of OS scheduler
**	    will determine how well this is respected.
**
** History:
**	24-sep-2002 (devjo01)
**	    Created.
*/
STATUS
CXnuma_get_info( i4 rad, CX_RAD_INFO *radinfobuf )
{
    STATUS	status = OK;

    if ( !CXnuma_support() )
    {
	return E_CL2C2E_CX_E_NONUMA;
    }

    /*
    **	Start OS dependent declarations.
    */
#ifdef	CX_NATIVE_GENERIC
    /*
    **	Native generic implementation
    */
    return E_CL2C2E_CX_E_NONUMA;

#else /*CX_NATIVE_GENERIC*/
#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
    /*
    **	Tru64 implementation
    */
    {
	radset_t	radset;
	radid_t		radid;
	rad_info_t      radinfo;
	struct cxaxp_radinfo {
	    radid_t	    radid;
	}		*scratchp;

	radid = cx_axp_numa_rad_to_osradset(rad, &radset);

	if ( RAD_NONE == radid )
	{
	    status = E_CL2C2F_CX_E_BADRAD;
	}
	else
	{
	    radinfo.rinfo_version = RAD_INFO_VERSION;
	    CPUSETCREATE(&radinfo.rinfo_cpuset);
	    if ( -1 == RAD_GET_INFO( radid, &radinfo ) )
	    {
		status = E_CL2C38_CX_E_OS_MISC_ERR;
	    }
	    else
	    {
		scratchp = (struct cxaxp_radinfo *)radinfobuf->cx_radi_scratch;
		scratchp->radid = radid;
		radinfobuf->cx_radi_rad_id = rad;
		radinfobuf->cx_radi_cpus = CPUCOUNTSET(radinfo.rinfo_cpuset);
		radinfobuf->cx_radi_physmem = radinfo.rinfo_physmem *
		 getpagesize() / 1024;
		radinfobuf->cx_radi_numdevs = 0; /* Sadly not implemented yet.*/
		radinfobuf->cx_radi_os_id = (PTR)&scratchp->radid;
		radinfobuf->cx_radi_os_id_decoder = cx_axp_rad_id_decoder;
		radinfobuf->cx_radi_devs = NULL;
	    }
	    CPUSETDESTROY(&radinfo.rinfo_cpuset);
	}
		
	RADSETDESTROY(&radset); 
    }
 
#else /*axp_osf (Tru64)*/
    /*
    **	All other platforms
    */
    return E_CL2C2E_CX_E_NONUMA;
#endif /*axp_osf (Tru64)*/
#endif /*CX_NATIVE_GENERIC*/
    /*
    **	End OS dependent declarations.
    */
    return status;
}


/*{
** Name: CXnuma_free_info - Free resources grabbed by CXnuma_get_info.
**
** Description:
**
**	This routine frees memory allocated by a call to CXnuma_get_info.
**
** Inputs:
**	CX_RAD_INFO *radinfobuf - Buffer filled in by CXnuma_get_info.
**
** Outputs:
**	none.
**
**	Returns:
**		OK			- All is well.
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-sep-2002 (devjo01)
**	    Created.
*/
STATUS
CXnuma_free_info( CX_RAD_INFO *radinfobuf )
{
    /* Currently no version of CXnuma_get_info allocates memory */
    return OK;
}


#if defined(axp_osf) && defined(conf_CLUSTER_BUILD)
/*
** Map Ingres RAD ident into a 'radset' returning OS RAD ident,
** or RAD_NONE if not a valid RAD.
*/
static radid_t
cx_axp_numa_rad_to_osradset( i4 rad, radset_t *pradset )
{
    radid_t	osradid, radid;
    radset_cursor_t radcursor = SET_CURSOR_INIT;
    i4		counter;
    i4		flags;

    RADSETCREATE(pradset); 
    RADFILLSET(*pradset);

    counter = 0;
    do
    {
	flags = 0;
	if ( 0 == counter )
	    flags |= SET_CURSOR_FIRST;
	counter++;
	if ( rad != counter )
	    flags |= SET_CURSOR_CONSUME;
	osradid = RAD_FOREACH(*pradset, flags, &radcursor);
	if ( rad == counter )
	    radid = osradid;
    } while ( RAD_NONE != osradid );

    if ( RADISEMPTYSET(*pradset) )
    {
	radid = RAD_NONE;
    }
    return radid;
}

/*
** Decode OS RAD ID into buffer.
*/
static char *
cx_axp_rad_id_decoder( PTR osradid, char *buf, i4 buflen )
{
    char	lbuf[16];

    CVna( (i4)*(radid_t *)osradid, lbuf );
    STlcopy(lbuf, buf, buflen);
    return buf;
}

#endif

/* EOF: CXNUMA.C */

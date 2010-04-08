/*
** Copyright (c) 2004 Ingres Corporation
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<cv.h>
# include	<cicl.h>
# include	<er.h>
# include	<me.h>
# include	<nm.h>
# include	<st.h>
# include	<si.h>
# include	<xa.h>
# include	<iixagbl.h>
# include	<iixainit.h>
# include	<iicx.h>
# include	<iicxfe.h>
# include	<iitxgbl.h>
# include	<iitxapi.h>
# include	<iitxinit.h>
# include	<generr.h>
# include	<erlq.h>
# include       <iicxxa.h>
# include       <iitxtms.h>
# include       <cs.h>
# include       <pc.h>
# include       <me.h>
# include       <iixashm.h>

static STATUS IItux_init_shm();
static STATUS IItux_detach_shm();
static VOID IItux_shm_name(char *buf);

/*
**  Name: iitxinit.c - Initialization routines for LIBQTXXA sub-component.
**
**  Description:
**      This module contains initialization and shutdown routines dealing 
**	with LIBQTXXA (TUXEDO).
**	
**  Defines:
**
**	IItux_init()		Initializes global TUXEDO CB
**	IItux_shutdown)		Frees global TUXEDO CB
**      IItux_tms_as_global_cb  structure of TUXEDO global control info
**	IItux_open		Called by TUXEDO trying to do an XA_OPEN
**				calls LIBQXA's IIXA_OPEN, then turns on
**				tuxedo stuff.
**
**  Notes:
**
**  History:
**	10-Nov-1993 (larrym)
**	    Written.
**	11-nov-1993 (larrym)
**	    moved get_process_type from iitxmain.c to here.  Now
**	    IItux_init calls it and stores it in IItux_tms_as_global_cb.
**	12-nov-1993 (larrym)
**	    Added initialization of tuxedo test field int
**	    the IItux_tms_as_global_cb
**	02-Dec-1993 (mhughes)
**	    Added initialisation of icas status services
**	03-Dec-1993 (mhughes)
**	    Removed initialisation of tuxedo_test global field
**      30-May-1996 (stial01)
**          MEadvise(ME_TUXEDO_ALLOC)
**	24-march-1997(angusm)
**	    Add option for per-user shared memory segment creation
*	    (bug 79829)
**      11-nov-98 (stial01)
**          Set II_TUX_XN_MAX to II_TUX_AS_MAX * II_XA_SESSION_CACHE_LIMIT
**	09-may-2000 (thoda04)
**	    Removed CI check for XA TUXEDO.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-aug-2002 (stial01)
**          Do not allocate IIcx_xa_xn_main_cb->max_free_xa_xn_cbs XN_ENTRIES
**          per server. This is set from II_XA_SESSION_CACHE_LIMIT, which is
**          the maximum sesions to keep connected. If the tms server is not
**          keeping up, the number of application server sessions can be greater
**          than this, so the number of XN_ENTRIES should also be greater.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GLOBALDEF	IITUX_TMS_AS_GLOBAL_CB	*IItux_tms_as_global_cb = NULL;

/*
**   Name: IItux_init()
**
**   Description: 
**       Initializes global TUXEDO data structures.
**
**   Inputs:
**       server_group	- TUXEDO server group as spec'd in OPEN string.
**
**   Outputs:
**	Returns: 
**	XAER_RMERR	- Could not allocate memory for Global CB.
**	XA_OK		- everything is hunky dory.
**
**   History:
**      09-Nov-1993 (larrym)
**	    written.   
**	11-nov-1993 (larrym)
**	    fixed up error messages.
**	12-nov-1993 (larrym)
**	    Added initialization of tuxedo test field int
**	    the IItux_tms_as_global_cb
**	02-Dec-1993 (mhughes)
**	    Added initialisation of icas status services
**	03-Dec-1993 (mhughes)
**	    Added initialisation of tux_global userlog field by evaluating
**	    environment variable II_TUX_USERLOG_TRACE
**	15-Dec-1993 (mhughes)
**	    Initialise process_type field in fe_thread_main_cb (tuxedo only).
**	07-Jan-1994 (mhughes)
**	    Initialise tpcall flags in tuxedo global cb based upon
**	    II_TUX_DEBUG environment variable.
**	12-Jan-1994 (mhughes)
**	    Modified tpcall flags for tpacall/tpgetrply pair.
**      01-Feb-1994 (mhughes)
**	    Check to see if user has specified a directory for the 
** 	    icas log file
**	09-mar-1994 (larrym)
**	    added CI check
**	08-Apr-1994 (mhughes)
**	    Added Recovery testing hooks
**	09-may-2000 (thoda04)
**	    Removed CI check for XA TUXEDO.
*/

int
IItux_init(char	*server_group)
{
    char	*tlevel_p;
    char	*debug_p;
    char	*logdir_p;
    char	ebuf[20];
    i4     trace_level;
    STATUS      cl_status, CIcapability();
    IICX_CB                  *fe_thread_cx_cb_p;
    IICX_FE_THREAD_CB        *fe_thread_cb_p;
    DB_STATUS                db_status;
    i4                       old_ingres_state;

#ifdef TUXEDO_RECOVERY_TEST
    char	*failpt_p;
    i4		fail_point;
#endif /* TUXEDO_RECOVERY_TEST */



    if (IItux_tms_as_global_cb != NULL)
    {
	/* error, already been here! */
        IIxa_error(GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
                    ERx("Internal Error: IItux_init called twice"),
				(PTR) 0, (PTR) 0, (PTR) 0, (PTR)0);
        return( XAER_RMERR );

    }

    if ((IItux_tms_as_global_cb = (IITUX_TMS_AS_GLOBAL_CB *)MEreqmem(
              (u_i4)0, (u_i4)sizeof(IITUX_TMS_AS_GLOBAL_CB), TRUE,
              (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IIxa_error(GE_NO_RESOURCE, E_LQ00D8_XA_CB_ALLOC, 2, ebuf,
                                   ERx("IItux_tms_as_global_cb"),
				   (PTR) 0, (PTR) 0, (PTR)0);
        return( XAER_RMERR );
    }

    /*
    ** Save away server_group_id and init SVN to 0.
    */
    STcopy(server_group, IItux_tms_as_global_cb->server_group_id);
    IItux_tms_as_global_cb->SVN = 0;

    /*
    ** Set the level of tracing to be written to the application userlog
    */
    IItux_tms_as_global_cb->userlog_level = IITUX_ULOG_MIN_LEVEL;
    NMgtAt( ERx("II_TUX_USERLOG_TRACE"), &tlevel_p );

    if ( ( tlevel_p != NULL ) && 
	 ( *tlevel_p != EOS ) &&
	 ( CVal( tlevel_p, &trace_level ) == OK ) )
    {
	if ( ( trace_level >= IITUX_ULOG_MIN_LEVEL ) && 
	     ( trace_level <= IITUX_ULOG_MAX_LEVEL ) )
	    IItux_tms_as_global_cb->userlog_level = (i4)trace_level;
    }

    NMgtAt( ERx("II_TUX_DEBUG"), &debug_p );

    db_status = IICXget_lock_fe_thread_cb( NULL, &fe_thread_cx_cb_p );
    if (DB_FAILURE_MACRO(db_status))
    {
	return( XAER_RMERR );
    }

    fe_thread_cb_p =
	     fe_thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;
    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    cl_status = IItux_shm_lock();
    if (cl_status)
    {
	IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_init Error taking lock"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	return(XAER_RMERR);
    }

    cl_status = IItux_init_shm();
    if (cl_status)
    {
        IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_init Error initializing shared memory segment"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
        return(XAER_RMERR);
    }

    cl_status = IItux_shm_unlock();
    fe_thread_cb_p->ingres_state = old_ingres_state;
    IICXunlock_cb( fe_thread_cx_cb_p );

    if (cl_status)
    {
        IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_init Error releasing lock"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
        return(XAER_RMERR);
    }

#ifdef TUXEDO_RECOVERY_TEST
    /*
    ** If this library was built for recovery testing see if a failure
    ** point has been specified
    */
    IItux_tms_as_global_cb->failure_point = IITUX_NO_FAILURE;

    NMgtAt( ERx("II_TUX_RECOVERY_FAIL_POINT"), &failpt_p );

    if ( ( failpt_p != NULL ) && 
	 ( *failpt_p != EOS ) &&
	 ( CVal( failpt_p, &fail_point ) == OK ) )
    {
	    IItux_tms_as_global_cb->failure_point = fail_point;
    }
#endif /* TUXEDO_RECOVERY_TEST */

    return (XA_OK);
}

/*
**   Name: IItux_shutdown()
**
**   Description: 
**       Frees global TUXEDO data structures.
**
**   Inputs:
**       None.
**
**   Outputs:
**	Returns: 
**	XA_OK		- For now...
**
**   History:
**      09-Nov-1993 (larrym)
**	    written.   
**	30-Nov-1993 (mhughes)
**	    Stomp on the address to stop reuse of free'd memory.
*/

int
IItux_shutdown()
{
    STATUS      cl_status;
    IICX_CB                  *fe_thread_cx_cb_p;
    IICX_FE_THREAD_CB        *fe_thread_cb_p;
    DB_STATUS                db_status;
    i4                       old_ingres_state;

    /* CHECK for error */
    MEfree(IItux_tms_as_global_cb);

    IItux_tms_as_global_cb=NULL;

    db_status = IICXget_lock_fe_thread_cb( NULL, &fe_thread_cx_cb_p );
    if (DB_FAILURE_MACRO(db_status))
    {
	return( XAER_RMERR );
    }

    fe_thread_cb_p =
	     fe_thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;
    old_ingres_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    cl_status = IItux_shm_lock();
    if (cl_status)
    {
	IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_shutdown Error taking lock"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	return(XAER_RMERR);
    }

    cl_status = IItux_detach_shm();
    if (cl_status)
    {
        IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_shutdown Error detaching shared memory segment"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
        return(XAER_RMERR);
    }

    cl_status = IItux_shm_unlock();
    fe_thread_cb_p->ingres_state = old_ingres_state;
    IICXunlock_cb( fe_thread_cx_cb_p );

    if (cl_status)
    {
        IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_shutdown Error releasing lock"),
            (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
        return(XAER_RMERR);
    }
	return (XA_OK);
}

/*
**  Name: IItux_open  - Call IIxa_open, then initialize tuxedo stuff.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. Currently, we don't 
**			     support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - the RMI string was ill-formed. 
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if the RMI is in the wrong state. Enforces the 
**			     state tables in the XA spec. 
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	17-nov-1998 (larrym)
**	    written.
**      30-may-1996 (stial01)
**          MEadvise(ME_TUXEDO_ALLOC) (B75073)
*/
int
IItux_open( char      *xa_info,
	   int       rmid,
	   long      flags)
{

    int		xa_ret_value;
    DB_STATUS	db_status;
    STATUS	status;

    /*
    ** start up CX and stuff 'cause we need it before we call
    ** IIxa_open
    */

    if (!IIxa_startup_done)
    {

	/* 
	** ME_TUXEDO_ALLOC: shared memory file is created in $II_TUXEDO_LOC
	*/
	status = MEadvise(ME_TUXEDO_ALLOC);
	if (status != OK)
	{
	    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
		ERx("IItux_open Error bad advice"),
		(PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	    return(XAER_RMERR);
	}

        /*
        ** can't do tracing here because IIcx_fe_thread_main_cb->fe_main_flags
        ** might not exist yet
        */
        db_status = IIxa_startup();
        if (DB_FAILURE_MACRO(db_status))
        {
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("xa_open_call: IIxa_startup failed."));
            return( XAER_RMERR );
        }

    }

    /*
    **  Turn on TUXEDO checking for LIBQXA
    */
    IIcx_fe_thread_main_cb->fe_main_tuxedo_switch = IItux_main;

    xa_ret_value = IIxa_open( xa_info, rmid, flags);

    return (xa_ret_value);
}

#define CACHENAME_SUFFIX ".tux"
#define CACHENM_MAX         (DB_MAXNAME + 1)


/*
**   Name: IItux_init_shm()
**
**   Description: 
**        This function allocates/attaches to shared memory segment used 
**        by INGRES to coordinate transaction management.
**
**   Inputs:
**       None.
**
**   Outputs:
**       OK:      Shared memory allocated
**       FAIL:    Error allocating shared memory
**
**   History:
**       01-Jan-1996 - First written (stial01,thaju02)
**	24-march-1997(angusm)
**	    Add option for per-user shared memory segment creation
**	    (bug 79829). Increase size of CACHENM_MAX to allow for
**	    usernames of up to DB_MAXNAME length. This does contravene
**	    the 8.3 filename standard, but we currently only support TUXEDO
**	    on UNIX.
**	7-oct-2004 (thaju02)
**	    Change allocate_pages to SIZE_TYPE.
*/
static STATUS 
IItux_init_shm()
{	
    char            name_buf[CACHENM_MAX + 5];
    STATUS          ret_val = OK;
    STATUS          ret_val2 = OK;
    i4         flags_arg;
    IITUX_MAIN_CB   *tux_main_cb = NULL;
    SIZE_TYPE       allocate_pages;
    i4              retry_cnt;
    CL_SYS_ERR      sys_err;
    CL_SYS_ERR      sys_err2;
    IITUX_XN_ENTRY  *xn_array;
    IITUX_XN_ENTRY  *cur_xn_array = NULL;  /* xn array for this as */
    IITUX_XN_ENTRY  *tmp_xn_array;
    i4              i, j;
    bool            shm_create = FALSE;
    IITUX_AS_DESC   *tmp_as_desc;
    IITUX_AS_DESC   *cur_as_desc;
    i4		    shm_as_max = 32;
    char	    *tmp_shm_as_max;
    i4		    xn_per_as;

		    /* II_TUX_AS_MAX(32) * II_XA_SESSION_CACHE_LIMIT(32)*/
    i4		    shm_xn_max = 1024;

    char	    *tmp_shm_xn_max;
    i4         xn_array_offset;
    i4         xn_array_size;

   
    /* get name of shm segment */
    IItux_shm_name(name_buf);

    for (retry_cnt = 0; ; retry_cnt++)
    {	
	if (retry_cnt >= 1000)
	{
            IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                    ERx("IItux_init_shm Error attaching to shm, (retry) "),
                    (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	    return (FAIL); 
	}

	/*
	** flags:
	**     ME_MSHARED_MASK: memory shared with other processors
	**     ME_SSHARED_MASK & ~ME_CREATE_MASK: ignore pages arg (arg 2)
	*/
	flags_arg = ME_MSHARED_MASK | (ME_SSHARED_MASK & ~ME_CREATE_MASK);
	ret_val = MEget_pages(flags_arg, 0, name_buf,
		(PTR *)&tux_main_cb, &allocate_pages, &sys_err);
    
	if (ret_val == OK)
	{
	    /* Attached successfully */
	    if ((tux_main_cb->tux_initflag) && 
		(!MEcmp("ITUX", (char *)tux_main_cb->tux_eyecatch, 4)))
	    {
		/* shared memory segment is initialized */
		break; 
	    }
	    else
	    {	
		/* detach from memory and try to attach again */
		ret_val = MEfree_pages((PTR)tux_main_cb, allocate_pages, 
					&sys_err);
		if (ret_val != OK)
		{
		    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                        ERx("IItux_init_shm Error Detaching from shm"),
                        (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
		    break;
		}
		continue;  /* try again */
	    }
	}

	if (ret_val != OK)
	{	
	    i4           pages;
	    i4      memory_needed;

	    if (ret_val != ME_NO_SUCH_SEGMENT) 
	    {
		IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                        ERx("IItux_init_shm Error attaching to shm"),
                        (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
		break;
	    }

	    NMgtAt( ERx("II_TUX_AS_MAX"), &tmp_shm_as_max);
	    if (tmp_shm_as_max != NULL && *tmp_shm_as_max)
	    {
		ret_val = CVan(tmp_shm_as_max, &shm_as_max );
		if (ret_val != OK || shm_as_max <= 0)
		{
		    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
			ERx("tux_init_shm: invalid II_TUX_AS_MAX env var"),
			(PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
		    return E_DB_ERROR;
		}
	    }

            NMgtAt( ERx("II_TUX_XN_MAX"), &tmp_shm_xn_max);
            if (tmp_shm_xn_max != NULL && *tmp_shm_xn_max)
            {
                ret_val = CVan(tmp_shm_xn_max, &shm_xn_max );
                if (ret_val != OK || shm_xn_max <= 0)
                {
                    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                        ERx("tux_init_shm: invalid II_TUX_AS_MAX env var"),
                        (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
                    return E_DB_ERROR;
                }
            }

	    /* try to create the segment */
	    memory_needed = sizeof (IITUX_MAIN_CB) +
			shm_as_max * sizeof (IITUX_AS_DESC);
	    memory_needed = DB_ALIGN_MACRO(memory_needed);
	    xn_array_offset = memory_needed;

	    xn_array_size = shm_xn_max * sizeof (IITUX_XN_ENTRY);
	    xn_array_size = DB_ALIGN_MACRO(xn_array_size);

	    memory_needed += xn_array_size;
	    pages = (memory_needed + ME_MPAGESIZE - 1) / ME_MPAGESIZE;

	    flags_arg = ME_MSHARED_MASK | ME_CREATE_MASK | ME_MZERO_MASK
				| ME_NOTPERM_MASK;
	    ret_val = MEget_pages(flags_arg, pages, name_buf,
                (PTR *)&tux_main_cb, &allocate_pages, &sys_err);

	    if (ret_val != OK)
	    {	
		if (ret_val == ME_ALREADY_EXISTS)
		    /* maybe someone beat us, try to attach again */
		    continue;
		else
		{
		    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                        ERx("IItux_init_shm Error creating shm segment"),
                        (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
		    break;
		}
	    }

	    shm_create = TRUE;

	    /* init shared memory */

	    tux_main_cb->tux_as_max = shm_as_max;
	    tux_main_cb->tux_xn_max = shm_xn_max;
	    xn_per_as = shm_xn_max/shm_as_max;
	    xn_array = (IITUX_XN_ENTRY *)
              ((char *)tux_main_cb + xn_array_offset);
	    cur_as_desc = &(tux_main_cb->tux_as_desc[0]);
	    cur_xn_array = xn_array;
	    tux_main_cb->tux_xn_array_offset = xn_array_offset; 
	    tux_main_cb->tux_xn_cnt = xn_per_as;
	    tux_main_cb->tux_alloc_pages = allocate_pages;

	    for (i = 0; i < tux_main_cb->tux_xn_max; i++)
		xn_array[i].xn_inuse = 0;
	    for (i = 0; i < tux_main_cb->tux_as_max; i++)
	    {
		tux_main_cb->tux_as_desc[i].as_flags = AS_FREE;
		tux_main_cb->tux_as_desc[i].as_xn_origcnt = 0;
	    }

	    PCpid(&cur_as_desc->as_pid);
	    cur_as_desc->as_xn_firstix = 0;
	    cur_as_desc->as_xn_cnt = xn_per_as;
	    cur_as_desc->as_flags &= ~AS_FREE;

	    tux_main_cb->tux_connect_cnt = 1;

	    MEcopy("ITUX", 4, (char *)tux_main_cb->tux_eyecatch);

	    /* The last thing to do is set this flag */
	    tux_main_cb->tux_initflag = 1;
	}

	break;
    }

    if (ret_val != OK)
	return (ret_val);

    if (shm_create)
    {
	IItux_lcb.lcb_tux_main_cb = tux_main_cb;
	IItux_lcb.lcb_as_desc = cur_as_desc;
	IItux_lcb.lcb_xn_array = xn_array;
	IItux_lcb.lcb_cur_xn_array = cur_xn_array;
	return (OK);
    }

    if (ret_val != OK) 
    {
	IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                        ERx("IItux_init_shm Error requesting semaphore"),
                        (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);

	ret_val2 = MEfree_pages((PTR)tux_main_cb, allocate_pages,
                                        &sys_err);
        if (ret_val2 != OK)
        {
            IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                ERx("IItux_init_shm Error Detaching from shm"),
                (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
        }
	return (ret_val);
    }
    else
    {	
	xn_array = (IITUX_XN_ENTRY *)
              ((char *)tux_main_cb + tux_main_cb->tux_xn_array_offset);

	xn_per_as = tux_main_cb->tux_xn_max/tux_main_cb->tux_as_max;

	/* First remove any/all dead AS's */
	for (i = 0; i < tux_main_cb->tux_as_max; i++)
	{
	    tmp_as_desc = &tux_main_cb->tux_as_desc[i];
	    if (!(tmp_as_desc->as_flags & AS_FREE) &&
                 !(PCis_alive(tmp_as_desc->as_pid)))
	    {
		tmp_xn_array = &xn_array[tmp_as_desc->as_xn_firstix];
		for (j = 0; j < tmp_as_desc->as_xn_cnt; j++)
		{
		    if (!tmp_xn_array[j].xn_inuse)
			continue;
		    if (tmp_xn_array[j].xn_state & TUX_XN_COMMIT)
			continue;
		    if (tmp_xn_array[j].xn_state & TUX_XN_ROLLBACK)
			continue;
		    break;
		}
			
		if (j >= tmp_as_desc->as_xn_cnt)
		{
		    tux_main_cb->tux_connect_cnt--;
		    tmp_as_desc->as_flags |= AS_FREE;
		}
	    }
	}

	cur_as_desc = NULL;
	/* First try to reuse an as_desc */
	for (i = 0; i < tux_main_cb->tux_as_max; i++)
	{
	    if ((tux_main_cb->tux_as_desc[i].as_flags & AS_FREE)
		&& tux_main_cb->tux_as_desc[i].as_xn_origcnt
		&& (tux_main_cb->tux_as_desc[i].as_xn_origcnt >= xn_per_as))
	    {
		cur_as_desc = &tux_main_cb->tux_as_desc[i];
		break;
	    }
	}

	if (!cur_as_desc)
	{
	    /* Need to get new as_desc */
	    for (i = 0; i < tux_main_cb->tux_as_max; i++)
	    {
		if (tux_main_cb->tux_as_desc[i].as_flags & AS_FREE)
		{
		    cur_as_desc = &tux_main_cb->tux_as_desc[i];
		    break;
		}
	    }

	    if ( !cur_as_desc || (tux_main_cb->tux_xn_cnt + xn_per_as >
				    tux_main_cb->tux_xn_max))
	    {
		IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
			ERx("IItux_init_shm Need to allocate another shm seg"),
			(PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	    
		/* detach from memory  */
		ret_val2 = MEfree_pages((PTR)tux_main_cb, allocate_pages, 
					&sys_err);
		return (FAIL);
	    }
	}

	/* Turn off free bit */
	cur_as_desc->as_flags &= ~AS_FREE;

	if (!cur_as_desc->as_xn_origcnt)
	{
	    cur_as_desc->as_xn_firstix = tux_main_cb->tux_xn_cnt;
	    cur_as_desc->as_xn_origcnt = xn_per_as;
	    cur_as_desc->as_xn_cnt = xn_per_as;
	    tux_main_cb->tux_xn_cnt += xn_per_as;
	}

	PCpid(&cur_as_desc->as_pid);
	cur_xn_array = &xn_array[cur_as_desc->as_xn_firstix];
	for (j = 0; j < cur_as_desc->as_xn_cnt; j++)
        {
            cur_xn_array[j].xn_inuse = 0;
        }

	tux_main_cb->tux_connect_cnt++;

    }
   
    IItux_lcb.lcb_tux_main_cb = tux_main_cb;
    IItux_lcb.lcb_as_desc = cur_as_desc;
    IItux_lcb.lcb_xn_array = xn_array;
    IItux_lcb.lcb_cur_xn_array = cur_xn_array;

    return (ret_val);
}

/*
**   Name: IItux_detach_shm()
**
**   Description: 
**        This function detaches/frees the shared memory segment used 
**        by INGRES to coordinate transaction management.
**
**   Inputs:
**       None.
**
**   Outputs:
**       OK:      Shared memory detached/freed 
**       FAIL:    Error detaching/freeing memory
**
**   History:
**       01-Jan-1996 - First written (stial01,thaju02)
**	24-march-1997(angusm)
**	    Add option for per-user shared memory segment creation
**	    (bug 79829)
*/
static STATUS
IItux_detach_shm()
{
    char            name_buf[CACHENM_MAX + 5];
    STATUS          ret_val = OK;
    STATUS          ret_val2 = OK;
    CL_SYS_ERR      sys_err;
    CL_SYS_ERR      sys_err2;
    IITUX_LCB       *tux_lcb = &IItux_lcb;
    IITUX_MAIN_CB   *tux_main_cb = tux_lcb->lcb_tux_main_cb;
    IITUX_AS_DESC   *cur_as_desc = tux_lcb->lcb_as_desc;
    PID             mypid;

    /* reset this pointer so we don't reference the shared memory
    ** after we detach
    */
    IItux_lcb.lcb_tux_main_cb = NULL;

    /* get name os shm segment */
    IItux_shm_name(name_buf);

    cur_as_desc->as_flags |= AS_FREE; 

    if (--tux_main_cb->tux_connect_cnt == 0)
    {
	/* init flag to zero and clear out eye catcher */
	tux_main_cb->tux_initflag = 0;
	MEfill(4, '0', tux_main_cb->tux_eyecatch);

	/* Destroy shm */
	ret_val = MEsmdestroy(name_buf, &sys_err);
	if (ret_val)
	{
	    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
		ERx("IItux_detach_shm Error Destroying shm"),
                (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	    return(ret_val);
	}
    }
    else
    {
	/* Detach from shm */
	ret_val = MEfree_pages((PTR)tux_main_cb, tux_main_cb->tux_alloc_pages, 
			&sys_err);
	if (ret_val)
	{
	    IIxa_error(GE_NO_RESOURCE, E_LQ00D0_XA_INTERNAL, 1,
                ERx("IItux_detach_shm Error Detaching from shm"),
                (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
            return(ret_val);
	}
    }

    return(OK);
}
/*
**   Name: IItux_shm_name()
**
**   Description: 
**        This function creates a name for the shared memory segment used 
**        by INGRES to coordinate transaction management.
**
**   Inputs:
**       buf	-	pointer to name buffer
**
**   Outputs:
**	 Name for shared memory segment.
**	 if II_TUX_SHARED is set to "USER", this will be t<username>.tux
**	 else, it will be 't1.tux'. Logic allows for future possibility
**	 of other schemes, e.g. per tuxedo GROUP instead of per UID.
**
**   History:
** 	23-march-1997 (angusm)
**	bug 79829: allow per-user creation of shared memory segments
**	for TUXEDO configurations.
**	- factor this code out of IItux_shm_init(), IItux_detach_shm()
**	- adapt scheme to allow per-user segments.
*/
static VOID
IItux_shm_name(char *buf)
{
    i4              seg_ix = 1;
    i4              name_index;
    char	    tuid[DB_MAXNAME];
    char	    *ptuid=tuid;
    char	    *ttmp;

   NMgtAt( ERx("II_TUX_SHARED"), &ttmp);
   if (ttmp != NULL && *ttmp)
   {
	if (STcompare(ttmp, ERx("USER")) == 0)
	{
	    IDname(&ptuid);
  	    STcopy(tuid, buf); 
	}
	else
	{
    	    seg_ix = 1; 
    	    buf[0] = 't';
    	    CVna(seg_ix, &buf[1]);
	}
   }
   else
   {
	/* future: support multiple shm segments */
    	seg_ix = 1; 

    	buf[0] = 't';
    	CVna(seg_ix, &buf[1]);
   }
   /* Trim off blanks and add '.tux' suffix. */
   name_index = STtrmwhite(buf);
   STcopy(CACHENAME_SUFFIX, buf + name_index);
}

/*
** Copyright (c) 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#if defined(OS_THREADS_USED)
#include    <clconfig.h>
#include    <systypes.h>
#include    <errno.h>
#include    <clsigs.h>
#include    <gv.h>
#include    <st.h>
#include    <me.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <ex.h>
#include    <cs.h>
#include    <cx.h>
#include    <csev.h>
#include    <csinternal.h>
#include    <cssminfo.h>
#include    <exinternal.h>

/*
**  Name: CSSHMEM.C - shared memory utility routines for cs
**
**  Description:
**	Contains routines to create, map and manage shared memory segments used
**	by the dbms.  These shared memory segments include a global control
**	segment and a server specific segment.  All routines
**	are internal to the CL and are proposed to be used by the UNIX 
**	implementation of CS.
**
**	Only routines in this module should manipulate any of the data in
**	the system control segment.
**
**      Routines in this module:
**
**          CS_create_sys_segment()     - create system control block segment
**          CS_map_sys_segment()        - map system control block segment
**          CS_alloc_serv_segment()     - create server shared mem segment
**          CS_init_pseudo_server()     - init a pseudo server.
**          CS_map_serv_segment()       - map server shared mem segment.
**          CS_destroy_serv_segment()   - destroy server shared mem segment.
**          CS_des_installation()       - destroy shared mem and semaphores
**                                        associated with an installation.
**          CS_find_servernum()         - find servernum for this PID.
**
**  History:
**      04-dec-2008 (joea)
**	    Created.
**      22-dec-2008 (stegr01)
**          Using UNIX version.
**	16-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
*/


/*
**  Forward and/or External function references.
*/

static i4 make_css_version(void);       /* Construct a control seg version */


/*
**  Definition of global; variables.
*/

GLOBALDEF       CS_SMCNTRL  *Cs_sm_cb ZERO_FILL; /* system control segment */
GLOBALDEF       CS_SERV_INFO    *Cs_svinfo ZERO_FILL;

i4              cs_servernum = -1;


/*{
** Name: CS_create_sys_segment()        - Create system shared mem segment.
**
** Description:
**      Create system control block.  Only caller should be the system
**      initialization program.  Other users of the system shared memory
**      segment should only map an already created shared memory segment.
**
**      This procedure creates a shared memory segment.  It then maps it into
**      the current process and initializes it.  It then unmaps it from the
**      current process (so that the CS_map_sys_segment() can be used without
**      any special casing).
**
**      Currently the already created shared memory segment is destroyed
**      before the new one is created.  We may add a destroy procedure
**      once the system installation program is fleshed out.
**
** Inputs:
**      num_of_servers                  number of servers this installation
**                                      should support.
**      num_of_wakeups                  number of wakeupblocks this installation
**                                      should support.
** Outputs:
**      err_code                        system dependent error.
**
**      Returns:
**          OK                          successfully created the segment.
**          FAIL                        failure to create shared mem segment.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      01-jan-88 (mmm)
**          Created.
**      21-jan-89 (mikem)
**          set correct value in the CS_SMCNTRL for the LG/LK shared memory id.
**          also add support for dynamic LGLK segment.
**      12-jun-89 (rogerk)
**          Integrated Terminator changes.  Changed args to MEget_pages calls.
**          Changed shared memory key to character string, not LOCATION pointer.
**          Changed ME_loctokey call to ME_getkey.
**      21-Jul-89 (anton)
**          Don't need a sys V semaphore per server any more.
**      26-aug-89 (rexl)
**          Added calls to protect page allocator.
**      28-Dec-89 (fredp)
**          Add TRdisplay error message on failure of semget.
**      16-feb-90 (greg)
**          NMgtAt is a VOID.
**      16-mar-90 (fredp)
**          Added support for system-wide II_LG_MAP_ADDR variable. If it
**          is set, it contains the hex address of where the lock segment
**          should be mapped into memory (instead of the default LG_MAP_ADDR).
**          If the system segment attach fails on Sun3, retry with a 1MB
**          lower attach address since the start of the user stack dropped
**          1MB from that of SunOS 4.0.x.
**      3-jul-1992 (bryanp)
**          We don't need to call LG/LK initialize anymore.
**          Added num_of_wakeups argument and used it to set css_wakeups_off.
**      26-jul-1993 (bryanp)
**          Removed the 5 extra semaphores that no-one needed nor used. This
**              leaves a few unused variables in various control blocks, which
**              eventually I'll get around to removing, as well. For now,
**              removing the unneeded semaphores reduces the system resource
**              usage of Ingres, which is a Good Thing.
**      31-jan-94 (mikem)
**          bug #58298
**          Changed CS_handle_wakeup_events() to maintain a high-water mark
**          for events in use to limit the scanning necessary to find
**          cross process events outstanding.  Previous to this each call to
**          the routine would scan the entire array, which in the default
**          configuration was 4k elements long.  Changed CS_create_sys_segment()
**          to initalize the new fields.
**      20-apr-94 (mikem)
**          Bug #57043
**          Added a call to CS_clockinit() to CS_create_sys_segment().  This
**          call is responsible for initializing new control information stored
**          in the shared memory system segment which is used to manipulate
**          the pseudo-quantum clock.  This clock is used to maintain both
**          quantum and timeout queues in CS.  See csclock.c for more
**          information.
**      19-sep-2002 (devjo01)
**          Allocate shared memory from local RAD if Running NUMA.
**      5-Apr-2006 (kschendel)
**          Use generated css version instead of constant.
*/
STATUS
CS_create_sys_segment(i4 num_of_servers, i4 num_of_wakeups, CL_ERR_DESC *err_code)
{
    SIZE_TYPE           size;
    STATUS              status;
    PTR                 address;
    SIZE_TYPE           alloc_pages;
    char                *string;
    char                *addr_string;
    STATUS              cv_status = OK;
    i4                  meflags;

    /* the system shared memory control block looks as follows:
    **
    **  ------------------------------
    **  |     control block          |
    **  |-----------------------------
    **  |     array of server info   |
    **  |            ...             |
    **  |            ...             |
    **  |-----------------------------
    **  |     array of wakeup blocks |
    **  |            ...             |
    **  |            ...             |
    **  ------------------------------
    */
    size = sizeof(CS_SMCNTRL) + ((num_of_servers + 1) * sizeof(CS_SERV_INFO));
    size += (num_of_wakeups * sizeof(CS_SM_WAKEUP_CB));
    /* round to ME_MPAGESIZE */
    size = (size + ME_MPAGESIZE - 1) & ~(ME_MPAGESIZE - 1);

    /*
    ** Create shared segment for sysseg.
    ** Use SYSSEG.MEM as shared memory key.
    */
    meflags = ME_MZERO_MASK|ME_SSHARED_MASK|ME_CREATE_MASK|ME_NOTPERM_MASK;
    address = (PTR) 0;

    if (CXnuma_user_rad())
        meflags |= ME_LOCAL_RAD;

    status = MEget_pages(meflags, size/ME_MPAGESIZE, "sysseg.mem", &address,
                &alloc_pages, err_code);

    /* FIX ME - initialize all the server control blocks */

    if (!status)
    {

        /* initialize the shared memory data base */
        Cs_sm_cb = (CS_SMCNTRL *) address;

        Cs_sm_cb->css_css_desc.cssm_addr = (PTR) 0;
        Cs_sm_cb->css_css_desc.cssm_size = size;
#ifdef __vms
        /* there's no such thing as a shared mem id in VMS */ 
        Cs_sm_cb->css_css_desc.cssm_id = 0;
#else
        Cs_sm_cb->css_css_desc.cssm_id = ME_getkey("sysseg.mem");
#endif
        Cs_sm_cb->css_numservers = num_of_servers;
        Cs_sm_cb->css_wakeup.css_numwakeups = num_of_wakeups;
        Cs_sm_cb->css_wakeup.css_minfree = 0;
        Cs_sm_cb->css_wakeup.css_maxused = 0;
        CS_clockinit(Cs_sm_cb);
        Cs_sm_cb->css_version = make_css_version();

        CS_SPININIT(&Cs_sm_cb->css_spinlock);

        /* now allocate cross process semaphores */
# ifdef xCL_075_SYS_V_IPC_EXISTS
        Cs_sm_cb->css_semid = semget(Cs_sm_cb->css_css_desc.cssm_id,
                                     CSCP_NUM, 0600|IPC_CREAT);
        if (Cs_sm_cb->css_semid < 0)
        {
            TRdisplay("semget of %d semaphores failed\n", CSCP_NUM);
            TRdisplay("errno = %d\n",errno);
            SETCLERR(err_code, 0, 0);
            status = FAIL;
        }
# endif
    }

    if (!status)
    {
        Cs_sm_cb->css_wakeups_off =
            (char *)&Cs_sm_cb->css_servinfo[num_of_servers] - (char *)Cs_sm_cb;

        /* initialize the server-server event structures */
        Cs_sm_cb->css_events_off = Cs_sm_cb->css_wakeups_off +
                                    (num_of_wakeups * sizeof(CS_SM_WAKEUP_CB));

        Cs_sm_cb->css_events_end = Cs_sm_cb->css_css_desc.cssm_size;
        Cs_sm_cb->css_nsem = 0;
        Cs_sm_cb->css_usem = 0;


    }

        /* FIX ME - initialize all the server control blocks */

#ifdef xCL_NEED_SEM_CLEANUP
        /* Init Shared Memory mutex chain mutex */
    if (!status)
    {
        CS_cp_synch_init(&Cs_sm_cb->css_shm_mutex, &status);
        }
#endif

    return(status);
}
/*{
** Name: CS_map_sys_segment()   - Map the system control segment to this process
**
** Description:
**      Maps the system control block to this process.  This must happen before
**      any other action can be taken on shared memory (with the exception of
**      CS_create_sys_segment()).
**
**      Upon successful exectution of this routine CS may manipulate the system
**      control data structure CS_SMCNTRL (taking care to use proper semaphore
**      techniques which take into account that the data structure is shared
**      across processes).
**
**      This call is internal to CS is meant only to be called by CS, and may
**      only exist on unix systems supporting shared memory.
**
** Inputs:
**      none.
**
** Outputs:
**      cssm_segment                    ptr to the CS_SMCNTRL data structure.
**      err_code                        system dependent error information.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-Sep-88 (mmm)
**          First Version
**      12-jun-89 (rogerk)
**          Added allocated_pages argument to MEget_pages calls.
**          Changed shared memory key from a LOCATIONN pointer to a character
**          string.
**      26-aug-89 (rexl)
**          Added calls to protect page allocator.
**      18-oct-1993 (bryanp)
**          Issue trace messages in failure cases. Simply returning FAIL doesnt
**              help the diagnosis process very much, while the trace messages
**              can help (if the caller has done a TRset_file by this point).
**      14-Oct-1998 (jenjo02)
**          CS_map_sys_segment(): Don't initialize css_spinlock. That was done
**          when the shared segment was created. Reinitializing it may
**          effectively destroy a holding process's lock!
**      21-May-2004 (wanfr01)
**          INGSRV2835, Bug 112371
**          As per the Jon Jensen's last update, removed the invalid initialize
**          of css_spinlock.
**      5-Apr-2006 (kschendel)
**          Use generated css version instead of constant.
*/
STATUS
CS_map_sys_segment(CL_ERR_DESC *err_code)
{
    STATUS      status = OK;
    PTR         address;
    SIZE_TYPE   alloc_pages;

    /* map segment into first available address and then initialize it */
    /* Use "sysseg.mem" as shared memory key. */
    address = 0;
    status = MEget_pages(ME_SSHARED_MASK, 0, "sysseg.mem", &address,
                         &alloc_pages, err_code);

    if (status)
    {
#ifdef xDEBUG
        TRdisplay("CS_map_sys_segment: unable to attach to sysseg.mem (%x)\n",
                        status);
#endif
        /* Unable to attach allocated shared memory segment. */
        /* status = FAIL; This isn't a useful error return code! */
    }
    else
    {
        i4 id = make_css_version();

        Cs_sm_cb = (CS_SMCNTRL *) address;

        if (Cs_sm_cb->css_version != id)
        {
            TRdisplay(
"CS_map_sys_segment: sysseg.mem is version %x (%d). We need version %x (%d)\n",
                Cs_sm_cb->css_version, Cs_sm_cb->css_version,
                id, id);
            SETCLERR(err_code, 0, 0);
            status = FAIL;
        }
    }

    return(status);
}

/*{
** Name: CS_get_cb_dbg()        - Debug routine to get ptr to global cntrl block
**
** Description:
**      Should only be used in testing.
**
** Inputs:
**      none.
**
** Outputs:
**      address                         set to point to CS_CNTRL global memory
**                                      segment.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-sep-88 (mmm)
**          First Version
*/
VOID
CS_get_cb_dbg(PTR *address)
{
    *address = (PTR) Cs_sm_cb;
}

/*{
** Name: CS_alloc_serv_segment()        - Create shared memory for a server
**
** Description:
**      Allocate a shared memory segment of "size" bytes.  All data placed
**      in this segment should be position independent.
**
**      Upon success this function will create the shared memory segment with
**      the given id and size.  To actually access the shared memory segment
**      the user must make a CS_map_server_segment() call.  It is expected
**      that this single request will fulfill all shared memory needs particular
**      to a single server.  It may be difficult or impossible to extend the
**      shared memory resources of some machines to provide for more than 3
**      different shared memory segments mapped to a single process (the
**      control segment, the locking/logging segment, and the server segment).
**
**      It is expected that CS will call CS_alloc_serv_segment() and
**      CS_map_serv_segment() once at server initialization.
**      Slaves will only call the map function.
**
**      This call is internal to CS is meant only to be called by CS, and may
**      only exist on unix systems supporting shared memory.
**
** Inputs:
**      size                            size in bytes of shared mem segment.
**
** Outputs:
**      id                              on success set to id of segment.
**      err_code                        system dependent error code.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-sep-88 (mmm)
**          First Version
**      12-jun-89 (rogerk)
**          Changed arguments to MEget_pages, MEdetach calls.  They now use
**          character name keys rather than a LOCATION pointer.  Changed
**          ME_loctokey call to ME_getkey.
**      26-aug-89 (rexl)
**          Added calls to protect page allocator.
**      3-jul-1992 (bryanp)
**          Explicitly ACLR the TAS objects when allocating server segment.
**      07-nov-1996 (canor01)
**          Explicitly initialize the csi_spinlock object.
*/
STATUS
CS_alloc_serv_segment(SIZE_TYPE size, u_i4 *server_seg_num,
	PTR *address, CL_ERR_DESC *err_code)
{
    STATUS              status = OK;
    CS_SERV_INFO        *serv_info;
    PTR                 dummy;
    i4                  i;
    SIZE_TYPE           alloc_pages;
    char                segname[48];

    status = CS_find_server_slot(&i);

    serv_info = Cs_sm_cb->css_servinfo;

    if (!address)
    {
        address = &dummy;
    }

    if (status)
    {
        SETCLERR(err_code, 0, 0);    /* since find_server_slot doesn't set */
    }
    else
    {
        /* now allocate and initialize the server segment */

        STcopy("server.", segname);
        CVna(i, segname+STlength(segname));

        size = (size + ME_MPAGESIZE - 1) & ~ (ME_MPAGESIZE - 1);
# ifdef SERV_MAP_ADDR
        *address = (PTR) SERV_MAP_ADDR;
        status = MEget_pages(ME_MZERO_MASK|ME_SSHARED_MASK|ME_CREATE_MASK|
                        ME_ADDR_SPEC, size/ME_MPAGESIZE, segname, address,
                        &alloc_pages, err_code);
# else
        status = MEget_pages(ME_MZERO_MASK|ME_SSHARED_MASK|ME_CREATE_MASK,
                             size/ME_MPAGESIZE, segname, address,
                             &alloc_pages, err_code);
# endif

        if (status)
        {
            /* Unable to create lg/lk shared memory segment */
            status = FAIL;
        }
        else
        {
            /* initialize it's description in the master control block */

            Cs_svinfo->csi_serv_desc.cssm_size = size;
            Cs_svinfo->csi_serv_desc.cssm_id = ME_getkey(segname);

            *server_seg_num = i;

            /*
            ** Explicitly clear these TAS objects, for machines such as the HP
            ** where MEfill'ing the memory with zeros does not initialize the
            ** TAS objects to "clear" state.
            */
            CS_ACLR(&Cs_svinfo->csi_nullev);
            CS_ACLR(&Cs_svinfo->csi_events_outstanding);
            CS_ACLR(&Cs_svinfo->csi_wakeme);
            CS_SPININIT(&Cs_svinfo->csi_spinlock);
            for (i = 0;
                i < (sizeof(Cs_svinfo->csi_subwake) /
                        sizeof(Cs_svinfo->csi_subwake[0]));
                i++)
            {
                CS_ACLR(&Cs_svinfo->csi_subwake[i]);
            }

            if (address == &dummy)
                MEdetach(segname, address, err_code); /* and free pages */

            /* FIX ME - bunch of other stuff */
        }
    }

    return(status);
}

STATUS
CS_set_server_connect(char *listen_id)
{
    i4          i;

    CS_find_server_slot(&i);

    STncpy( Cs_svinfo->csi_connect_id, listen_id,
        (sizeof(Cs_svinfo->csi_connect_id) - 1) );
    Cs_svinfo->csi_connect_id[ sizeof(Cs_svinfo->csi_connect_id) - 1 ] = EOS;
    return(OK);
}

STATUS
CS_find_server_slot(i4 *slotno)
{
    STATUS              status = OK;
    CS_SERV_INFO        *serv_info;
    i4                  i;

    if (cs_servernum >= 0)
    {
        *slotno = cs_servernum;
        return(OK);
    }

    CS_getspin(&Cs_sm_cb->css_spinlock);

    serv_info = Cs_sm_cb->css_servinfo;

    /* find a free slot */
    status = FAIL;
    for (i = 0; i < Cs_sm_cb->css_numservers; i++)
    {
        if (!serv_info[i].csi_in_use)
        {
            cs_servernum = *slotno = i;
            Cs_svinfo = &serv_info[i];
            MEfill(sizeof (CS_SERV_INFO), '\0', (char *)Cs_svinfo);
            Cs_svinfo->csi_in_use = TRUE;
            Cs_svinfo->csi_id_number = i;
            /* Set this so we don't get any wakeup signals until cleared */
            CS_TAS(&Cs_svinfo->csi_wakeme);
            i_EXsetothersig(SIGUSR2, SIG_IGN);
            Cs_svinfo->csi_pid = getpid();
            Cs_svinfo->csi_signal_pid = Cs_svinfo->csi_pid;
            CS_SPININIT(&Cs_svinfo->csi_spinlock);

            status = OK;
            break;
        }
    }

    CS_relspin(&Cs_sm_cb->css_spinlock);
    return(status);
}

VOID
CS_get_critical_sems(void)
{
    CS_getspin(&Cs_sm_cb->css_spinlock);
}
VOID
CS_rel_critical_sems(void)
{
    CS_relspin(&Cs_sm_cb->css_spinlock);
}

/*{
** Name: CS_init_pseudo_server() - init CS for a peudo-server
**
** Description:
**      Allocate resources for server-server processing by a process
**      which exists at server functional level without full server
**      task switching and asycronous i/o.  Such processes do not
**      need a shared memory segement but do need cross process semaphores
**      and other resources of a server that are keep in the system
**      shared memory segment.
**
**      It is expected that CS will call CS_init_pseudo_server() and
**      CS_map_serv_segment() once at pseudo-server initialization.
**
**      This call is internal to CS is meant only to be called by CL, and may
**      only exist on unix systems supporting shared memory.
**
** Inputs:
**      size                            size in bytes of shared mem segment.
**
** Outputs:
**      id                              on success set to id of segment.
**      err_code                        system dependent error code.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      28-mar-88 (anton)
**          Created.
**      3-jul-1992 (bryanp)
**          Install the server event handler for pseudo-servers here.
*/
STATUS
CS_init_pseudo_server(u_i4 *server_seg_num)
{
    STATUS              status = OK;
    CS_SERV_INFO        *serv_info;
    i4                  i;
    CSSL_CB             *slcb;

    status = CS_find_server_slot(&i);

    serv_info = Cs_sm_cb->css_servinfo;

    if (!status)
    {
        /* initialize it's description in the master control block */

        Cs_svinfo->csi_serv_desc.cssm_size = 0;
        Cs_svinfo->csi_serv_desc.cssm_id = -1;

        *server_seg_num = i;

        /*
        ** Explicitly clear these TAS objects, for machines such as the HP
        ** where MEfill'ing the memory with zeros does not initialize the
        ** TAS objects to "clear" state.
        */
        CS_ACLR(&Cs_svinfo->csi_nullev);
        CS_ACLR(&Cs_svinfo->csi_events_outstanding);
        CS_ACLR(&Cs_svinfo->csi_wakeme);
        CS_SPININIT(&Cs_svinfo->csi_spinlock);
        for (i = 0;
                i < (sizeof(Cs_svinfo->csi_subwake) /
                sizeof(Cs_svinfo->csi_subwake[0]));
                i++)
        {
            CS_ACLR(&Cs_svinfo->csi_subwake[i]);
        }

        status = CSinstall_server_event(&slcb, 0, 0, CSev_resume);

        /* FIX ME - bunch of other stuff */
    }

    return(status);
}

bool
CS_is_server(i4 num)
{
    return(num >= 0 && num < Cs_sm_cb->css_numservers &&
           Cs_sm_cb->css_servinfo[num].csi_in_use &&
           Cs_sm_cb->css_servinfo[num].csi_serv_desc.cssm_id != -1);
}

/*{
** Name: CS_map_serv_segment() - Map the server segment.
**
** Description:
**      Maps the server control block to this process.
**
**      Upon successful exectution of this routine the process may access
**      memory shared among server and slave processes.
**
**      This call is internal to CS is meant only to be called by CS, and may
**      only exist on unix systems supporting shared memory.
**
** Inputs:
**      serv_seg_num                    id of server segment.
**
** Outputs:
**      address                         on success, set to point to segment
**      err_code                        system dependent error information.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-sep-88 (mmm)
**          First Version
**      12-jun-89 (rogerk)
**          Added allocated_pages argument to MEget_pages calls.
**          Changed shared memory key to a character string rather than a
**          LOCATION pointer.
**      26-aug-89 (rexl)
**          Added calls to protect page allocator.
*/
STATUS
CS_map_serv_segment(u_i4 serv_seg_num, PTR *address, CL_ERR_DESC *err_code)
{
    STATUS      status = OK;
    char        segname[48];
    SIZE_TYPE   alloc_pages;

    if (!address)
    {
        SETCLERR(err_code, 0, 0);
        status = FAIL;
    }
    else
    {
# ifdef SERV_MAP_ADDR
        *address = (PTR) SERV_MAP_ADDR;
# else
        *address = (PTR) 0;
# endif /* SERV_MAP_ADDR */

        Cs_svinfo = &Cs_sm_cb->css_servinfo[serv_seg_num];
        STcopy("server.", segname);
        CVna((i4)serv_seg_num, segname+STlength(segname));

# ifdef SERV_MAP_ADDR
        status = MEget_pages(ME_SSHARED_MASK|ME_ADDR_SPEC, 0, segname,
                             address, &alloc_pages, err_code);
# else
        status = MEget_pages(ME_SSHARED_MASK, 0, segname, address,
                             &alloc_pages, err_code);
# endif /* SERV_MAP_ADDR */

        if (status)
        {
            /* Unable to attach allocated shared memory segment. */
            status = FAIL;
        }
    }

    return(status);
}

/*{
** Name: CS_destroy_serv_segment() - Destroy the server segment.
**
** Description:
**      Destroy the server shared memory segment.  This should be called when
**      server is shut down (ie. should be put into the last chance exception
**      handler of CS).
**
**      Eventually the abnormal exit code must take care of cleaning up this
**      shared memory segment.
**
**      This call is internal to CS is meant only to be called by CS, and may
**      only exist on unix systems supporting shared memory.
**
** Inputs:
**      id                              id of server segment.
**
** Outputs:
**      address                         on success, set to point to segment
**      err_code                        system dependent error information.
**
**      Returns:
**          E_DB_OK
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      08-sep-88 (mmm)
**          First Version
**      12-jun-89 (rogerk)
**          Changed MEsmdestroy to take character memory key, not LOCATION ptr.
*/
STATUS
CS_destroy_serv_segment(u_i4 serv_seg_num, CL_ERR_DESC *err_code)
{
    STATUS      status = OK;
    char        segname[48];

    STcopy("server.", segname);
    CVna((i4)serv_seg_num, segname+STlength(segname));

#ifdef xCL_NEED_SEM_CLEANUP
        CS_cp_sem_cleanup(segname,err_code);
#endif
    status = MEsmdestroy(segname, err_code);
    if (status)
    {
        /* Unable to attach allocated shared memory segment. */
        status = FAIL;
    }

    Cs_sm_cb->css_servinfo[serv_seg_num].csi_in_use = FALSE;

    /* FIX ME - probably add code to update system control stuctures to
    ** keep track of when a shared memory segment is mapped.
    */

    return(status);
}

/*{
** Name: CS_des_installation()  - destroy shared resources of an installation.
**
** Description:
**      Destroy the system shared memory segment, the logging/locking
**      shared memory segment, and the system semaphores associated with the
**      current installtion.  This is called from the logging code as a
**      result of "rcpconfig /shutdown".
**
**      It assumes that all the shared memory segments have already been
**      initialized by the appropriate routines.  The order of destruction
**      is important as a reverse may cause access violations on some
**      implementation of shared memory (where the segment disappears as
**      soon as it destroyed).
**
**      Any subsequent logging/locking or event routines called after this
**      routine will likely fail, so the caller should exit soon after this
**      call.
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**
** History:
**      08-sep-88 (anton)
**          use LOcations for segments and destroy server segments
**      09-jun-88 (mmm)
**          written.
**      12-jun-89 (rogerk)
**          Change MEsmdestroy calls to take character string key instead
**          of LOCATION pointer.
**      22-dec-92 (mikem)
**          Changed a for(;;) to a do..while(FALSE) to shut up stupid acc
**          warning.
**      02-feb-92 (sweeney)
**          remove orphaned call to MEsmdestroy() on lockseg.
**      26-jul-1993 (bryanp)
**          Remove no-longer-needed system semaphores (css_semid sems).
**      03-jun-1996 (canor01)
**          Clean up semaphores for operating system threads.
**      16-sep-2002 (somsa01)
**          Make sure we run the appropriate "cscleanup" in the case of
**          ADD_ON64.
**	22-Jun-2009 (kschendel) SIR 122138
**	    VMS doesn't do hybrids, but update the conditional anyway.
*/
STATUS
CS_des_installation(void)
{
    STATUS      status;
    LOCATION    loc;
    char        *string;
    PID         pid;
    char        *argv[1];
    CL_ERR_DESC err_code;
    i4          i;
    char        segname[48];

    /* stop all servers that are still active, forcing them to exit if
    ** they have not already exited.
    */

    /* call the cscleanup code to clean up all the server slots still out
    ** there.
    */

    if ((NMloc(SUBDIR, PATH, "utility", &loc) == OK)    &&
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
        (LOfaddpath(&loc, "lp64", &loc) == OK) &&
#endif
        (LOfstfile("cscleanup", &loc) == OK))
    {
        /* if everything is successful try and run the cleanup program,
        ** else just do the rest and make user run "cscleanup" by themselves.
        */

        LOtos(&loc, &string);

        argv[0] = string;
        PCspawn(1, argv, TRUE, (LOCATION *) NULL, (LOCATION *) NULL, &pid);
    }

    do
    {
        /* destroy any left over server segements */

        for (i = 0; i < MAXSERVERS; i++)
        {

            STcopy("server.", segname);
            CVna(i, segname+STlength(segname));
#ifdef xCL_NEED_SEM_CLEANUP
                CS_cp_sem_cleanup(segname, &err_code);
#endif
            (VOID) MEsmdestroy(segname, &err_code);
        }
        /*                      FIX ME                        */
        /* kill off slave processes here or in cscleanup code */

        /* destroy the system shared memory segment */

#ifdef xCL_NEED_SEM_CLEANUP
        CS_cp_sem_cleanup("sysseg.mem", &err_code);
#endif
        status = MEsmdestroy("sysseg.mem", &err_code);
        if (status)
           break;

    } while (FALSE);

    return(status);
}

/*{
** Name: CS_check_dead()        - Cleanup after exited processes.
**
** Description:
**      Clean up server slots left by exited processes.  These are left
**      around whenever processes "owning" cs server slots (allocated by
**      calls to CSinitiate()) exit without going through normal exit
**      procedure.  This can happen with AV's, kill -9, ...
**
**      This routine goes though the list of slots processes, checks for their
**      existence and cleans up after any process that has exited.
**
** Inputs:
**      none.
**
** Outputs:
**      none.
**
**
** History:
**      16-aug-90 (mikem)
**          Created.
**      26-jul-1993 (bryanp)
**          Removed unused "semid" variable and usage of no-longer-used
**              css_semid field.
**      23-aug-1993 (andys)
**          In CS_check_dead call CS_cleanup_wakeup_events to clean up
**          any waiting events if we find a dead process.
**      18-oct-1993 (bryanp)
**          Take the spin lock in CS_check_dead.
**          Pass new argument to CS_cleanup_wakeup_events to indicate that we
**              already hold the system segment spinlock.
*/
void
CS_check_dead(void)
{
    CS_SERV_INFO        *svinfo;
    i4             nservs;
    CS_SMCNTRL          *sysseg;
    i4                  i;
    CL_ERR_DESC         err_code;
    CSEV_SVCB           *addr;
#ifdef xCL_075_SYS_V_IPC_EXISTS
    union semun         arg;
#endif
    CS_get_cb_dbg(&sysseg);

    CS_getspin(&Cs_sm_cb->css_spinlock);

    svinfo = sysseg->css_servinfo;
    nservs = sysseg->css_numservers;

    for (i = 0; i < nservs; i++)
    {
        if (svinfo->csi_in_use && svinfo->csi_pid &&
                kill(svinfo->csi_pid, 0) < 0)
        {
            TRdisplay("CS_check_dead %@ ->csi_in_use %d  ->csi_pid %d \n",
                svinfo->csi_in_use, svinfo->csi_pid );
#ifdef xCL_075_SYS_V_IPC_EXISTS
            if (svinfo->csi_semid > 0)
            {
                semctl(svinfo->csi_semid, 0, IPC_RMID, arg);
            }
#endif
            CS_destroy_serv_segment(svinfo->csi_id_number, &err_code);

            /*
            ** Cleanup any wakeup events hanging around from a previous server
            */
            CS_cleanup_wakeup_events(svinfo->csi_pid, (i4)1);

            svinfo->csi_in_use = svinfo->csi_pid = 0;
        }
        svinfo++;
    }

    CS_relspin(&Cs_sm_cb->css_spinlock);
}

/*
** Name: CS_find_servernum      - find the servernum for this PID
**
** Description:
**      This routine returns the servernum for a process, given the process's
**      PID.
**
**      It is used by the cross-process resume support.
**
** Inputs:
**      pid                     - process ID to find the servernum for
**
** Outputs:
**      servernum               - the server number for this PID
**
** Returns:
**      OK                      - found it and set server number
**      !OK                     - can't find a server number for that PID
**
** History:
**      3-jul-1992 (bryanp)
**          Created for the new Logging and Locking system.
*/
STATUS
CS_find_servernum(PID pid, i4  *servernum)
{
    i4                  i;
    CS_SERV_INFO        *serv_info;

    *servernum = -1;

    CS_getspin(&Cs_sm_cb->css_spinlock);

    serv_info = Cs_sm_cb->css_servinfo;

    /* find the slot for this server */
    for (i = 0; i < Cs_sm_cb->css_numservers; i++)
    {
        if (serv_info[i].csi_in_use)
        {
            if (serv_info[i].csi_pid == pid)
            {
                *servernum = i;
                break;
            }
        }
    }

    CS_relspin(&Cs_sm_cb->css_spinlock);
    return((*servernum == -1) ? FAIL : OK);
}

/*
** Name: make_css_version -- Construct a control segment version number
**
** Description:
**
**      In order to ensure that we connect to a valid control segment,
**      a "version" number is stored in the header.  This version used
**      to be a hard coded version, but a) it wasn't updated often
**      enough, and b) that doesn't protect against multiple installations
**      on the same machine.
**
**      This routine will construct an i4-sized version number from the
**      build versions and the installation ID.
**
**      Note that old versions were mostly 6 decimal digits, and all new
**      versions (starting at GV_MAJOR=9) are much larger than that.
**
** Inputs:
**      none.
**
** Outputs:
**      Returns a version/id number.
**
** History:
**      5-Apr-2006 (kschendel)
**          Written to address bug 115940, cross-installation interference.
**
*/

static i4
make_css_version(void)
{

    char *installation_id;      /* Our II_INSTALLATION */
    i4 id;                      /* Constructed version / id number */

    id = ((((GV_MAJOR-8)*4 + GV_MINOR)*10 + GV_GENLEVEL)<<7) + (GV_BLDLEVEL-100);
    id = id << 16;

    /* Include the installation ID.  Don't look at the environment,
    ** only look at the symbol.tbl.
    */
    NMgtIngAt("II_INSTALLATION", &installation_id);
    if (installation_id != NULL && *installation_id != '\0')
    {
        id |= (*installation_id++) << 8;
        id |= *installation_id;
    }

    return (id);
} /* make_css_version */

#else /* OS_THREADS_USED */

   static i4 dummy = 1; /* Declared to prevent compiler warning on non-OS_THREADS platforms */

#endif

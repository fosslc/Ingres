/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <clipc.h>
#include    <tr.h>
#include    <cs.h>
#include    <fdset.h>
#include    <csev.h>
#include    <cssminfo.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <cx.h>

#ifdef xCL_NEED_SEM_CLEANUP
#include	<st.h>
VOID shmclean( CS_SMCNTRL	*sysseg);
#endif

/**
**
**  Name: CSCLEANUP.C - report and cleanup an installation
**
**  Description:
**	This program will cleanup shared memory and semaphores from an
**	installtion which may have had servers crash.  This program is
**	also run from DMFRCP as part of clean up after a server crash.
**
**          main() - main funtion of cscleanup
**
**
**  History:
**      08-sep-88 (anton)
**	    First Version
**	01-Mar-1989 (fredv)
**	   Use <systypes.h> and <clipc.h> instead of <systypes.h>, <sys/ipc.h>
**	   	and <sys/sem.h>.
**	   Added ifdef xCL_002 around struct semun.
**	27-mar-89 (anton)
**	    Changed ming hint from USELIBS to NEEDLIBS
**      15-apr-89 (arana)
**          Removed ifdef xCL_002 around struct semun.  Defined struct semun
**          in clipc.h ifndef xCL_002.
**	4-june-90 (blaise)
**	    Integrated changes from ingresug:
**		Increase the buffer (vals) size from 64 to 128 so that RT
**		won't dump core;
**		ifdef semaphore operations on symbol xCL_075_SYS_V_IPC_EXISTS
**		since not all systems have semaphores.
**	29-sep-1992 (bryanp)
**	    Remove reference to css_lglk_desc -- LG/LK in mainline.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-jul-1993 (bryanp)
**	    Remove uses of css_semid -- it no longer contains information.
**	    As a result, removed reportSVsems, since it's no longer needed,
**		and prototyped checkServs, since I removed an arg from it.
**	17-aug-1993 (tomm)
**		Must include fdset.h since protypes being included reference
**		fdset.
**	06-sep-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p in reportSM().
**	31-jan-1994 (bryanp)
**	    B58406: display csi_connect_id as a string, not as a number.
**  12-Dec-97 (bonro01)
**      Added cleanup code for cross-process semaphores during
**      MEsmdestroy().  This code is currently only required for DGUX
**      so it is enabled by xCL_NEED_SEM_CLEANUP.
**  11-aug-1998 (hweho01) 
**      a) Changed to call CSMT_cp_sem_cleanup() instead of CS_cp_sem_cleanup() 
**         in shmclean(), since the runtime switch (CS_srv_block.cs_mt)    
**         is not set in this executable. 
**      b) Destroy the mutex that is for the shared memory mutex chain.   
**      c) Cleanup the semaphores in sysseg.mem after checkServs() is called.  
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	19-sep-2002 (devjo01)
**	    Provide handling for "-rad" command line option used to 
**	    specify target RAD in NUMA environments.
**	16-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/


static VOID checkServs( CS_SERV_INFO	*svinfo, i4 nservs);
static void reportSM(CS_SM_DESC *shm);

/*{
** Name: main() - report on an installation
**
** Description:
**	cleanup and report on shared memory and semaphores of an installation
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    PCexit()
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-sep-88 (anton)
**          First version
**      15-apr-89 (arana)
**          Removed ifdef xCL_002 around struct semun.  Defined struct semun
**          in clipc.h ifndef xCL_002.
**	29-sep-1992 (bryanp)
**	    Remove reference to css_lglk_desc -- LG/LK in mainline.
**	31-jan-1994 (bryanp)
**	    B58406: display csi_connect_id as a string, not as a number.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

main(argc, argv)
int	argc;
char	*argv[];
{
    CL_ERR_DESC	err_code;
    CS_SMCNTRL	*sysseg;

    MEadvise(ME_INGRES_ALLOC);

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    {
	char	*lp64enabled;

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
    }
#endif  /* hybrid */

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_NUMA, NULL, 0 ) )
    {
	PCexit(FAIL);
    }

    TRset_file(TR_F_OPEN, "stdio", 5, &err_code);
    if (CS_map_sys_segment(&err_code))
    {
	TRdisplay("Can't map system segement\n");
	PCexit(1);
    }
    CS_get_cb_dbg((PTR *)&sysseg);
    TRdisplay("Installation version %d\n", sysseg->css_version);
    TRdisplay("Max number of servers %d\n", sysseg->css_numservers);
    TRdisplay("Description of shared memory for control system:\n");
    reportSM(&sysseg->css_css_desc);
    TRdisplay("Description of shared memory for logging & locking system:\n");
    TRdisplay("Event system: used space %d, length space %d\n",
	      sysseg->css_events_off, sysseg->css_events_end);

#ifdef xCL_NEED_SEM_CLEANUP
	shmclean(sysseg);
#endif

    TRdisplay("Server info:\n");
    checkServs(sysseg->css_servinfo, sysseg->css_numservers);

# ifdef OS_THREADS_USED
# ifdef xCL_NEED_SEM_CLEANUP
    CS_synch_destroy( &sysseg->css_shm_mutex );
    CSMT_cp_sem_cleanup( "sysseg.mem", &err_code );
# endif /* xCL_NEED_SEM_CLEANUP */
# endif /* OS_THREADS_USED */

    PCexit(0);
}

# ifdef xCL_075_SYS_V_IPC_EXISTS
static void
reportSM(CS_SM_DESC *shm)
{
    TRdisplay("key 0x%x: size %d attach %p\n", shm->cssm_id, shm->cssm_size,
	      shm->cssm_addr);
}
# else
static void
reportSM(CS_SM_DESC *shm)
{
    TRdisplay("size %d attach %p\n", shm->cssm_size, shm->cssm_addr);
}
# endif	/* xCL_075_SYS_V_IPC_EXISTS */
static VOID
checkServs(
CS_SERV_INFO	*svinfo,
i4		nservs)
{
    i4		i;
    CL_ERR_DESC	err_code;
    union semun arg;

    for (i = 0; i < nservs; i++)
    {
	TRdisplay("server %d:\n", i);
	TRdisplay("inuse %d, pid %d, connect id %s, id_number %d, semid %d\n",
		  svinfo->csi_in_use, svinfo->csi_pid, svinfo->csi_connect_id,
		  svinfo->csi_id_number, svinfo->csi_semid);
	TRdisplay("shared memory:\n");
	reportSM(&svinfo->csi_serv_desc);
	TRdisplay("\n");
	if (svinfo->csi_in_use && svinfo->csi_pid &&
		kill(svinfo->csi_pid, 0) < 0)
	{
	    TRdisplay("*** server is dead, cleaning up\n");
	    if (svinfo->csi_semid > 0)
	    {
		TRdisplay("*** removeing semaphores, slaves should die.\n");
		semctl(svinfo->csi_semid, 0, IPC_RMID, arg);
	    }
	    CS_destroy_serv_segment(svinfo->csi_id_number, &err_code);
	    svinfo->csi_in_use = svinfo->csi_pid = 0;
	}
	svinfo++;
    }
}

# ifdef OS_THREADS_USED
#ifdef xCL_NEED_SEM_CLEANUP
VOID
shmclean(
CS_SMCNTRL	*sysseg)
{
    CL_ERR_DESC err_code;
    int i;
    PTR memory;
    i4  pages;
    STATUS status;

    for(i = 0; i < CS_SHMSEG; ++i)
    {
        if ( sysseg->css_shm_ids[i].offset.q_next )
        {
            if ( STequal( "sysseg.mem", sysseg->css_shm_ids[i].key) )
                continue; /* Must Save sysseg.mem for last */
            CSMT_cp_sem_cleanup( sysseg->css_shm_ids[i].key, &err_code );
        }
    }
}
#endif /* xCL_NEED_SEM_CLEANUP */
# endif /* OS_THREADS_USED */

/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <id.h>

#ifndef	INCLUDED_LGKDEF_H
#define INCLUDED_LGKDEF_H

/**
** Name: LGKDEF.H - Data structures shared by LG and LK
**
** Description:
**	These structures are common to both the LG and LK operations.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	19-oct-1992 (bryanp)
**	    Add a version number field to the LGK shared memory.
**	16-feb-1993 (bryanp)
**	    Bump version number for 6.5 Cluster Project changes.
**	24-may-1993 (bryanp)
**	    Bump version number again for 24-may bugfix integration changes.
**	    Add new LGK parameter-checking error message numbers.
**	21-jun-1993 (andys)
**	    Bump version number again.
**	26-jul-1993 (bryanp)
**	    Bump version number due to changing some "shorts" in lgdef/lkdef.
**	26-jul-1993 (jnash)
**	    Add LOCK_LGK_MEMORY. 
**	    Add 'flag' param to LGK_initialize FUNC_EXTERN.
**	    Add E_DMA809_LGK_NO_ON_OFF_PARM.
**	20-sep-1993 (mikem)
**	    Bump version number due to changing size of a CS_SEMAPHORE.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Mar-2000 (jenjo02)
**	    Add LGK_OBJD_LG_LLB to return sizeof(LBB),
**	    LGK_OBJE_LG_MISC, LGK_OBJF_LK_MISC, to return
**	    memory requirements not otherwise accounted for.
**	    Add E_DMA811_LGK_MT_MISMATCH, mem_status,
**	    LGK_IS_MT, bumped LGK_MEM_VERSION.
**	18-apr-2001 (devjo01)
**	    Added LGK_IS_CSP, LGK_my_pid. (s103715).
**	30-Oct-2002 (hanch04)
**	    Added prototype for lgk_calculate_size
**	06-Oct-2003 (devjo01)
**	    lgk_calculate_size prototype is now in lgkparms.h.
**	17-aug-2004 (thaju02)
**	    Changed memleft from i4 to SIZE_TYPE in lgkm_initialize_mem 
**	    prototype.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Aug-2009 (wanfr01)
**	    Bug 122418 - Add E_DMA812_LGK_NO_SEGMENT, LOCK_LGK_MUST_ATTACH
**      20-Nov-2009 (maspa05) bug 122642
**          Added id_uuid_sem and uuid_last_time. These help ensure UUIDs are
**          genuinely unique across all servers. Added here as LGK is shared
**          across all servers
**/

#define LGK_OBJ0_LK_LLB		    0
#define LGK_OBJ1_LK_LLB_TAB	    1
#define LGK_OBJ2_LK_BLK		    2
#define LGK_OBJ3_LK_BLK_TAB	    3
#define LGK_OBJ4_LK_LKH		    4
#define LGK_OBJ5_LK_RSH		    5
#define LGK_OBJ7_LG_LXB		    7
#define LGK_OBJ8_LG_LXB_TAB	    8
#define LGK_OBJ9_LG_LDB		    9
#define LGK_OBJA_LG_LDB_TAB	    10
#define LGK_OBJB_LK_RSB		    11
#define LGK_OBJC_LK_RSB_TAB	    12
#define LGK_OBJD_LG_LBB	    	    13
#define LGK_OBJE_LG_MISC    	    14
#define LGK_OBJF_LK_MISC    	    15

/*
** Values of the 'flag' parameter to LGK_initialize.
*/
#define LOCK_LGK_MEMORY		0x1
#define LOCK_LGK_MUST_ATTACH	0x2
#define LGK_IS_CSP		0x4	/* Must match CX_F_IS_CSP */


/*
** Name: LGK_BASE - process handle to the LG/LK shared memory segment.
**
** Description:
**	This block contains the process-specific local handle to the LG/LK
**	shared memory segment. Each process fills this block in when it calls
**	LGK_initialize.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	30-Apr-2003 (jenjo02)
**	    Added lgk_pid_slot for BUG 110121.
*/
typedef struct
{
    PTR		lgk_mem_ptr;	    /* base address of the LG/LK segment */
    PTR		lgk_lgd_ptr;	    /* process-specific address of the LGD */
    PTR		lgk_lkd_ptr;	    /* process-specific address of the LKD */
    i4		lgk_pid_slot;	    /* process's PID slot in mem_pid */
} LGK_BASE;

GLOBALREF LGK_BASE LGK_base;

GLOBALREF PID	   LGK_my_pid;


/*
** Name: LGK_OFFSET_FROM_PTR	- macro which turns a pointer into an offset
**
** Description:
**	This macro assists with the coding of position independent linked lists
**	in the LG/LK shared memory pool. It takes a pointer to a location in
**	the LG/LK shared memory and returns the offset from the base of the
**	shared memory to the indicated location.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
*/
#define	LGK_OFFSET_FROM_PTR(ptr) ((SIZE_TYPE)\
	    ((char *)ptr - (char *)LGK_base.lgk_mem_ptr))

/*
** Name: LGK_PTR_FROM_OFFSET	- macro which turns an offset into a pointer
**
** Description:
**	This macro assists with the coding of position independent linked lists
**	in the LG/LK shared memory pool. It takes an offset from the base of the
**	shared memory and returns a pointer to the indicated location.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	28-Apr-2003 (jenjo02)
**	    If offset is zero, return NULL to induce
**	    SEGV rather than corrupt LGK_MEM.
*/
#define LGK_PTR_FROM_OFFSET(offset) \
((offset) ? ((PTR) ((char *)LGK_base.lgk_mem_ptr + offset)) \
          : (NULL))

/*
** Name: LGK_VERIFY_ADDR	- conditional debug address checking macro
**
** Description:
**	If LGK_PARANOIA is defined, this macro will call an lgkm routine
**	which will verify that memory range from addr to addr + sz - 1 is
**	in allocated LGK memory.
**
** History:
**	25-may-2002 (devjo01)
**	    Created to help shoot an allocate/deallocate bug.
*/
#ifdef LGK_PARANOIA 
# define LGK_VERIFY_ADDR(addr,sz) \
	  lgkm_valid(__FILE__,__LINE__,(PTR)(addr),(sz))
#else
# define LGK_VERIFY_ADDR(addr,sz)
#endif

/*}
** Name: LGK_EXT - Extent block.
**
** Description:
**      This block is contained at the start on all extents allocated from the
**	LG/LK shared memory pool.
**
**	Free extents are linked together on a list chained from the LGK_MEM
**	structure, and the list is kept in increasing offset order.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	17-aug-2004 (thaju02)
**	    Changed ext_size from i4 to SIZE_TYPE.
*/
typedef struct
{
    i4          ext_next;		/* Offset to next LGK_EXT block. */
    SIZE_TYPE   ext_size;		/* Size in bytes of extent. */
} LGK_EXT;

typedef struct
{
#define			LGK_INFO_SIZE	64
    char	    info_txt[LGK_INFO_SIZE];
} LGK_INFO;
/*}
** Name: LGK_MEM - Overall header for the LG/LK memory pool
**
** Description:
**	Contains information about memory allocation from the shared memory
**	segment.  This memory allocation is used by LG and LK.  The shared
**	memory segment is created at installation (boot) time.  Memory is
**	allocated and deallocated by LG and LK through the lgkm_allocate_ext()
**	and lgkm_deallocate_ext() routines.
**
**	Please note that the LG/LK memory pool is position independent.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable LG and LK.
**	19-oct-1992 (bryanp)
**	    Add a version number field to the LGK shared memory.
**	16-feb-1993 (bryanp)
**	    Bump version number for 6.5 Cluster Project changes. (V3 now)
**	24-may-1993 (bryanp)
**	    Bump version number again for 24-may bugfix integration changes.
**	21-jun-1993 (andys)
**	    Bump version number again.
**	26-jul-1993 (bryanp)
**	    Bump version number due to changing some "shorts" in lgdef/lkdef.
**	20-sep-1993 (mikem)
**	    Bump version number due to changing size of a CS_SEMAPHORE.
**      20-Aug-1999 (hanal04) Bug 97316 INGSRV921
**          Bump version number due to change in LLB structure.
**	27-Mar-2000 (jenjo02)
**	    Added mem_status, LGK_IS_MT, to prevent mixed connections
**	    of MT/INGRES threads to logging/locking, bumped version
**	    to account for change.
**	30-Apr-2003 (jenjo02)
**	    Added LGK_MAX_PIDS array of processes attached to
**	    memory segment, E_DMA80A_LGK_ATTACH_LIMIT,
**	    bumped version. BUG 110121
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      20-Nov-2009 (maspa05) bug 122642
**          Added id_uuid_sem and uuid_last_time. These help ensure UUIDs are
**          genuinely unique across all servers. Added here as LGK is shared
**          across all servers
*/
typedef struct
{
    i4	    	    mem_version_no;	/* Current version of the LG/LK shared
					** memory. Increment this each time
					** that either LG shared memory or LK
					** shared memory formats change.
					*/
#define			LGK_MEM_VERSION_1	    0x00120001L
#define			LGK_MEM_VERSION_2	    0x00120002L
#define			LGK_MEM_VERSION_3	    0x00120003L
#define			LGK_MEM_VERSION_4	    0x00120004L
#define			LGK_MEM_VERSION_5	    0x00120005L
#define			LGK_MEM_VERSION_6	    0x00120006L
#define			LGK_MEM_VERSION_7	    0x00120007L
#define			LGK_MEM_VERSION_8	    0x00120008L
#define			LGK_MEM_VERSION_9	    0x00120009L
#define			LGK_MEM_VERSION_A	    0x0012000AL
#define			LGK_MEM_VERSION_CURRENT	    LGK_MEM_VERSION_A
    LGK_EXT	    mem_ext;		/* free list of extents - maintained in
					** sorted order from least address to
					** greatest address.
					*/
    i4	    	    mem_lkd;		/* offset to the LKD database */
    i4	    	    mem_lgd;		/* offset to the LGD database */
    i4	    	    mem_status;		/* Status bits: */
#define			LGK_IS_MT	0x00000001
    CS_SEMAPHORE    mem_ext_sem;	/* semaphore protecting the extent
					** freelist (LGKM.C)
					*/
    /* UUID related elements are here because all servers need access to
     * them */
    ID_UUID_SEM_TYPE   id_uuid_sem;	/* semaphore protecting creation of
					  UUIDs */
    HRSYSTIME	    uuid_last_time;     /* last time we generated a UUID */
    u_i2	    uuid_last_cnt;      /* last counter value used for a UUID*/
    i4	    	    mem_allocs;		/* # of allocs requested */
    i4	    	    mem_deallocs;	/* # of deallocs requested */
    i4	    	    mem_fragments;	/* # of memory fragments performed */
    i4	    	    mem_coalesces;	/* # of coalesces performed */
#define			LGK_MAX_PIDS	128
#define			LGK_INFO_SIZE	64
    PID		    mem_pid[LGK_MAX_PIDS]; /* Attached processes */
    LGK_INFO	    mem_info[LGK_MAX_PIDS]; /* Process identifying info */
#define		    mem_creator_pid	mem_pid[0]
} LGK_MEM;


typedef struct _LGLK_INFO LGLK_INFO;


/*}
** Name: LGLK_INFO - a structure to keep track of the lglk resource counts.
**
** Description:
**      A structure to keep track of the lglk resource counts.  Filled in by
**	reading "rcp.par".
**
** History:
**      17-feb-90 (mikem)
**          Created.
**	13-feb-1993 (keving)
**	    Moved from lgkmemusg.c to lgkdef.h
**	24-may-1993 (bryanp)
**	    Added lgk_max_resources field, since resources and lock blocks
*8		are now separate. Added field comments.
*/
struct _LGLK_INFO
{
    i4         lgk_log_bufs;       /* number of log page buffers */
    i4         lgk_max_xacts;      /* max transactions in logging system */
    i4         lgk_max_dbs;	/* max databases in logging system */
    i4         lgk_block_size;     /* log file page size */
    i4         lgk_lock_hash;      /* lock hash table size */
    i4         lgk_res_hash;       /* resource hash table size */
    i4         lgk_max_locks;      /* max locks in locking system */
    i4	    lgk_max_resources;	/* specified number of RSB's. */
    i4         lgk_max_list;       /* max lock lists in locking system */
};

/*
** Function prototypes for LGK functions
*/
FUNC_EXTERN LGK_EXT *lgkm_allocate_ext(SIZE_TYPE size);
FUNC_EXTERN STATUS  lgkm_initialize_mem(SIZE_TYPE memleft, PTR first_free_byte);
FUNC_EXTERN VOID    lgkm_deallocate_ext(LGK_EXT *input_ext);

FUNC_EXTERN STATUS  LGK_initialize(i4 flag, CL_ERR_DESC *sys_err, char *info);
FUNC_EXTERN VOID    LGK_rundown(i4 status, i4  arg);
FUNC_EXTERN VOID    LGK_deadinfo(PID dead_pid);



/*
** Error message definitions for error messages used by the LGK modules.
*/
#define	    E_DMA800_LGKINIT_GETMEM		    (E_DM_MASK | 0xA800L)
#define	    E_DMA801_LGKINIT_MEMSIZE		    (E_DM_MASK | 0xA801L)
#define	    E_DMA802_LGKINIT_ERROR		    (E_DM_MASK | 0xA802L)
#define	    E_DMA803_LGKDEST_ERROR		    (E_DM_MASK | 0xA803L)
#define	    E_DMA804_LGK_NEGATIVE_PARM		    (E_DM_MASK | 0xA804L)
#define	    E_DMA805_LGK_CONFIG_ERROR		    (E_DM_MASK | 0xA805L)
#define	    E_DMA806_LGK_PARM_RANGE_ERR		    (E_DM_MASK | 0xA806L)
#define	    E_DMA807_LGK_BLKSIZE_ERR		    (E_DM_MASK | 0xA807L)
#define	    E_DMA808_LGK_NONNUM_PARM		    (E_DM_MASK | 0xA808L)
#define	    E_DMA809_LGK_NO_ON_OFF_PARM		    (E_DM_MASK | 0xA809L)
#define	    E_DMA80A_LGK_ATTACH_LIMIT		    (E_DM_MASK | 0xA80AL)
#define	    E_DMA811_LGK_MT_MISMATCH		    (E_DM_MASK | 0xA811L)
#define	    E_DMA812_LGK_NO_SEGMENT		    (E_DM_MASK | 0xA812L)

#endif /* INCLUDED_LGKDEF_H */

/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name: SC0M.H - Information for the SCF memory manager
**
** Description:
**      This file contains the interesting typedef's and # defines
**      for the SCF internal memory manager (sc0m_ calls).
**
** History: $Log-for RCS$
**      14-mar-86 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    Add prototyped func externs.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          This results in owner within sc0m_allocate() proto becoming PTR.
**	06-mar-1996 (bryanp)
**	    Add SC0M_MAXALLOC to represent the maximum size that one can
**		allocate through sc0m_allocate().
**      22-mar-1996 (stial01)
**          Changed SC0M_EXPAND_SIZE and SC0M_MAXALLOC for 64k tuple support
**          Changed prototype for sc0m_allocate, i4  length -> i4 length
**          Changed sco_size_allocated to sco_xtra_bytes
**          Changed scp_size_allocated to scp_xtra_bytes
**	25-Jun-1997 (jenjo02)
**	    Renamed sc0m_pool_list to sc0m_pool_next, added sc0m_pool_prev.
**	    Added SC0M_ADD_POOL mask bit, scp_flags, scp_semaphore.
**	14-jan-1999 (nanpr01)
**	    Change the function prototypes to pass the pointer to a pointer.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	6-dec-02 (inkdo01)
**	    Increased SC0M_MAXALLOC by 60K for PSF control block increase
**	    due to range table expansion.
**      18-feb-2004 (stial01)
**          Increased SC0M_MAXALLOC for 256k row support.
**          Cleaned up definition of SC0M_OBJECT.
**      12-Apr-2004 (stial01)
**          Define scp_length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Define scp_psize as SIZE_TYPE.
**	24-Aug-2009 (thaju02) (B122354)
**	    Increase SC0M_MAXALLOC/SC0M_EXPAND_SIZE for cursor_limit.
**      29-jan-2009 (smeke01) b121565
**          Improve SC0M main semaphore handling to reduce likelihood 
**          of contention.
**/

/*
**  Forward type declarations for types defined in this file
*/

typedef struct _SC0M_OBJECT   SC0M_OBJECT;
typedef struct _SC0M_POOL     SC0M_POOL;

/*
**  Defines of other constants.
*/

#define			SC0M_MAXALLOC 840000		/* Maximum allocation */
#define                 SC0M_EXPAND_SIZE ( 843776 / SCU_MPAGESIZE)
							/* to get page count */
#define			SC0M_BLOCK_PRINT_MASK	0x1	/* have sc0m_check print the
							    blocks as encountered */
#define			SC0M_READONLY_MASK	0x2	/* make memory readable */
#define			SC0M_WRITEABLE_MASK	0x4	/* make memory writeable */
#define			SC0M_NOSMPH_MASK	0x8
			/*
			** Do not protect call with semaphore.  It is
			** already protected.
			*/
#define                 SC0M_GUARD_MASK		0x10	/* Guard pages with
							** mapped out pages */

/*}
** Name: SC_OBJECT - Generic object header
**
** Description:
**      This is an object header which is used throughout SCF.
**      It exists as a separate type because it is used by memory
**      management as well as being used as a list header in various
**      places.
**
** History:
**     12-Mar-86 (fred)
**          Created on Jupiter
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/
struct _SC0M_OBJECT
{
    SC0M_OBJECT     *sco_next;
    SC0M_OBJECT     *sco_prev;
    SIZE_TYPE	    sco_size;		/* size of object */
    u_i2	    sco_type;		/* object type */
#define			SC0M_FTYPE	0x1
    u_i2	    sco_s_reserved;	/* Reserved for future use. */
    SC0M_POOL	    *sco_pool_hdr;	/* "parent" memory pool (_l_reserved) */
    PTR    	    sco_owner;		/* id of allocator */
#define			SCF_MEM		0x0	    /* free memory */
#define                 SCU_MEM         0x1
#define                 SCC_MEM         0x2
#define                 SCS_MEM         0x3
#define                 SCD_MEM         0x4
    i4	    sco_tag;		/* tag specified at declaration */
#define                 SC_FREE_TAG     CV_C_CONST_MACRO('f', 'r', 'e', 'e')
    i4         sco_extra;
};

/*}
** Name: SC0M_POOL - definition of a memory pool header
**
** Description:
**      This structure describes a pool header.  This is used
**      to describe an entire memory pool, consisting of the start
**      and end addresses, number of bytes, pool status, and the free
**      list
**
** History:
**     13-mar-86 (fred)
**          Created on Jupiter
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	25-Jun-1997 (jenjo02)
**	    Added scp_flags, scp_semaphore.

*/
struct _SC0M_POOL
{
    SC0M_POOL	    *scp_next;
    SC0M_POOL	    *scp_prev;
    SIZE_TYPE	    scp_length;		/* length of object as declared */
    u_i2	    scp_type;		/* object type */
#define			SC0M_PTYPE	    0x0
    u_i2	    scp_s_reserved;	/* Reserved for future use */
    SC0M_POOL	    *scp_pool_hdr;	/* self_pointer   (_l_reserved)       */
    PTR    	    scp_owner;		/* id of allocator */
    i4	    scp_tag;		/* tag specified at declaration */
#define                 SC_POOL_TAG     CV_C_CONST_MACRO('s', 'c', 'm', 'p')
    i4	    scp_status; /* was extra */
    SIZE_TYPE	    scp_psize;		/* number of bytes in pool */
    PTR		    scp_start;
    PTR		    scp_end;
    SC0M_OBJECT	    *scp_free_list;
    SC0M_OBJECT	    *scp_nfree;
    SC0M_OBJECT	    *scp_pfree;
    i4		    scp_flags;		/* current state of pool: */
#define			SCP_BUSY 0x0001	/* A thread is allocating or
					** deallocating in this pool */
    CS_SEMAPHORE    scp_semaphore;	/* A mutex per pool */
};

/*}
** Name: SC0M_CB - main control of SCF internal memory 
**
** Description:
**      This block is a part of sc_main_cb, the central control block for all
**      of SCF.  It contains the information necessary to control the allocation
**      of memory within SCF.
**
** History:
**     14-mar-86 (fred)
**          Created on Jupiter.
**	25-Jun-1997 (jenjo02)
**	    Renamed sc0m_pool_list to sc0m_pool_next, added sc0m_pool_prev.
**	    Added SC0M_ADD_POOL mask bit.
*/
typedef struct _SC0M_CB
{
    i4	    sc0m_flag;		/* defines which of the debugging flags
					** are available for use.  Is anded with
					** the flag passed in to determine
					** what is actually printed
					*/
    i4              sc0m_status;        /* Current status of the system */
#define                 SC0M_UNINIT     0x00 /* Hasn't been initialized   */
#define                 SC0M_INIT       0x01 /* Has been initialized      */
#define                 SC0M_CORRUPT    0x02 /* Corrupted                 */
#define                 SC0M_ADD_POOL   0x04 /* A new pool is being added */
    SC0M_POOL       *sc0m_pool_next;     /* The list of memory pools */
    SC0M_POOL       *sc0m_pool_prev;
    i4	    sc0m_pools;		/* Number of pools allocate */	
    CS_SEMAPHORE    sc0m_semaphore;	/* to protect SC0M_CB structure */
    CS_SEMAPHORE    sc0m_addpool_semaphore;	/* to ensure pools are added one at a time */
}   SC0M_CB;
/*
[@type_definitions@]
*/

/* Func externs */

FUNC_EXTERN DB_STATUS sc0m_gcalloc( i4 num, i4 size, PTR *place );
FUNC_EXTERN DB_STATUS sc0m_gcdealloc( PTR block );

FUNC_EXTERN i4 sc0m_allocate(i4 flag,
				  SIZE_TYPE size,
				  u_i2 type,
				  PTR owner,
				  i4  tag,
				  PTR *block );

FUNC_EXTERN i4 sc0m_check( i4 flag, char *tag );
FUNC_EXTERN i4 sc0m_deallocate( i4  flag, PTR *returned_block);
FUNC_EXTERN i4 sc0m_free_pages(SIZE_TYPE page_count, PTR *addr);

FUNC_EXTERN i4 sc0m_get_pages(i4 flag,
				   SIZE_TYPE  in_pages,
				   SIZE_TYPE  *out_pages,
				   PTR *addr );

FUNC_EXTERN i4 sc0m_initialize(void);

/*
** Copyright (c) 2004 Ingres Corporation
*/
#ifndef DISLAVE_H_INCLUDED
#define DISLAVE_H_INCLUDED

#include <meprivate.h>		/* get ME_SEGID definition */
#include <fdset.h>


/**
** Name: DISLAVE.H - Definition of structures for DI slave processing
**
** Description:
**	Structures and configuration of DI slave processes.
**
**	9-mar-88 (anton)
**	    Created.
**	6-Dec-1989 (anton)
**	    Increased DI_MAX_SLAVES to 10
**	    Cleaned up comments from long past.
**	14-may-90 (blaise)
**	    Integrated changes from 61 and ug into 63p library:
**		Added io_open_flags to store DI_SYNC_MASK
**	10-jan-91 (jkb)
**	    Increase DI_MAX_SLAVES from 10 to 30
**	12-nov-1991 (bryanp)
**	    Add segid and seg_offset to DI_SLAVE_CB, and added opcode 
**	    DI_SL_MAPMEM for slave access to server shared memory.
**	21-feb-92 (jnash)
**	    Remove segid from DI_SLAVE_CB, replace with seg_key for
**	    use with MMAP as well as Sys V shared mem.  (segid was
**	    not the same in the server/rcp and in the slave, seg_key is.)
**	30-October-1992 (rmuth)
**	    Add the SL_GUARANTEE operation and remove the ZPAD operation.
**	30-November-1992 (rmuth)
**	    Change DI_SLAVE_CB.length to be of type i4 from a nat.
**	26-jul-93 (mikem)
**	    Added DI_COLLECT_STATS slave operation to allow server to query
**	    slave as to slave's performance statistics.  These stats are
**	    eventually exported by the dimo.c module through a MO table.
**      03-Nov-1997 (hanch04)
**          Change i4 to OFFSET_TYPE
**	10-Nov-2010 (kschendel) SIR 124685
**	    Add slave prototypes here (not needed outside of the CL).
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _DI_SLAVE_CB	DI_SLAVE_CB;


/*
**  Defines of other constants.
*/

# define	DI_FILE_BUF_SIZE	(8*1024) /* max I/O op size */
# define	DI_MAX_SLAVES		30	 /* max num of slaves */


/*}
** Name: DI_SLAVE_CB	- Definition of DI's use of event control blocks
**
** Description:
**	This structure caries all information needed to pass between the
**	master and the slave processes for DI.
**
**	READ and WRITE operations transmit the effective buffer address of
**	the I/O buffer between the master and the slave as a (segment ID,
**	offset) pair. This is translated back into an address by the slave.
**	This allows the I/O memory to be mapped in a position independent
**	fashion in the various processes: it may reside at a different address
**	in each process.
**
** History:
**      09-mar-88 (anton)
**          Created.
**	24-jul-88 (mmm)
**	    Added DI_SL_OSYNC_OPEN op code.
**	06-feb-89 (mikem)
**	    added "reserved" and "unix_open_flags".
**	12-nov-1991 (bryanp)
**	    Add segid and seg_offset to DI_SLAVE_CB, and added opcode 
**	    DI_SL_MAPMEM for slave access to server shared memory.
**	    Add lg_data for LG's use of DI slaves to write logfile blocks.
**	30-October-1992 (rmuth)
**	    Add the SL_GUARANTEE operation and remove the ZPAD operation.
**	26-jul-93 (mikem)
**	    Added DI_COLLECT_STATS slave operation to allow server to query
**	    slave as to slave's performance statistics.  These stats are
**	    eventually exported by the dimo.c module through a MO table.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Oct-2005 (jenjo02)
**	    Replaced "reserved" with "io_fprop", file properties passed
**	    to and from DI_IO to slave.
*/
struct _DI_SLAVE_CB
{
    i4		dest_slave_no;		/* slave assigned to do the work */
    DI_UNIQUE_ID file_id;		/* unique file id */
    OFFSET_TYPE	pre_seek;		/* seek to perform before I/O */
    i4		file_op;		/* operation for perform */
# define	DI_SL_READ		1	/* slave read op */
# define	DI_SL_WRITE		2	/* slave write op */
# define	DI_SL_OPEN		3	/* slave open op */
# define	DI_SL_CLOSE		4	/* slave close op */
# define	DI_SL_SENSE		5	/* slave sense length op */
# define	DI_SL_SYNC		6	/* slave fsync op */
# define	DI_SL_DELETE		7	/* slave delete file op */
# define	DI_SL_MKDIR		8	/* slave make dir op */
# define	DI_SL_RMDIR		9	/* slave remove dir op */
# define	DI_SL_LSDIR		10	/* slave list dir op (not written) */
# define	DI_SL_OSYNC_OPEN	11	/* slave open file with the O_SYNC option */
# define	DI_SL_ZALLOC		12	/* Allocate space by writing zeros. */
# define	DI_SL_GUARANTEE		13	/* Guarantee that disc space
						** disc space for a range of
						** pages will exist when 
						** needed
						*/
# define	DI_SL_MAPMEM		14	/* slave map server memory */
# define	DI_COLLECT_STATS	15	/* collect slave statistics */

# define	DI_NOSUSPEND	0x80	/* no suspend after cause event
					   this structure is freed instead */
    i4		lru_close;		/* close this descriptor whatever */
    i4		unix_fd;		/* operate on this file in slave */
    i4		io_open_flags;		/* DI_SYNC_MASK or nothing */
    i4		unix_open_flags;	/* flags to use when opening a file */
    u_i4	io_fprop;		/* file properties, from open */
    char	seg_key[MAX_LOC];	/* segment key, id of I/O buffer */
    i4	seg_offset;		/* offset into segid of buffer */
    PTR		lg_data;		/* This field is reserved for LG to
					** use for storing its own private data
					** about logfile blocks written with
					** DI slaves. See lgintern.c.
					*/
    CL_ERR_DESC	errcode;		/* OS err code */
    STATUS	status;			/* other err information */
    OFFSET_TYPE length;			/* length of operation */
    char	buf[sizeof(ALIGN_RESTRICT)];	/* buffer for data */
					/* Allocated to size_io_buf */
};

/* Function prototypes */

FUNC_EXTERN STATUS DI_init_slave(fd_set fd_msk, i4 num_fds);
FUNC_EXTERN STATUS DI_slave(CSEV_CB *evcb);

#endif /* DISLAVE_H_INCLUDED */

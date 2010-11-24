/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <pc.h>
#include <csinternal.h>

/**CL_SPEC
** Name: DILOCAL.H - This file contains the internal defines for di routines
**
** Description:
**
**	All defines and typedef's needed by the di implementation are defined
**	here.
**      
**  History:
**      26-mar-87 (mmm)
**          Created new for 6.0.
**	30-November-1992 (rmuth)
**	    Various
**	     - Add some prototypes.
**	     - Change DI_OP.di_fd to i4 from int.
**	10-mar-1993 (mikem)
**	    Changed the type of the first parameter to DI_send_ev_to_slave() and
**	    the 2nd parameter to DI_slave_send(), so that DI_send_ev_to_slave() 
**	    could access the slave control block's status.
**	    This routine will now initialize the status to DI_INPROGRESS, before
**	    making the request and the slave will change the status once the
**	    operation is complete.
**	30-feb-1993 (rmuth)
**	    Add GLOBALREF for DIzero_buffer and DI_ZERO_BUFFER_SIZE;
**      26-jul-1993 (mikem)
**          Moved GLOBALREF's of the following into one file rather than
**          repeated in each file: Di_slave, Di_slave_cnt, DI_misc_sem,
**          DI_sc_sem.
**	12-jun-1995 (amo ICL)
**	    Added DI_OP_ASYNC for async io
**	18-Dec-1997 (jenjo02)
**	    DI cleanup. Moved all GLOBALREFs here.
**	    Added di_file to DI_OP.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Jan-2005 (jenjo02)
**	    Upped DI_ZERO_BUFFER_SIZE from 64k
**	    to 1024k.
**	06-Oct-2005 (jenjo02)
**	    DI_FD_PER_THREAD now config param
**	    ii.$.$.$.fd_affinity:THREAD, Di_thread_affinity = TRUE,
**	    still dependent on OS_THREADS_USED.
**	14-Oct-2005 (jenjo02)
**	    Added GLOBALREF for Di_direct_io.
**	28-Nov-2005 (kschendel)
**	    Remove old zero buffer definition.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Remove DI-global direct-io switch.
**/



/*
** Definition of all global references owned by this file.
*/
GLOBALREF CS_SYSTEM	Cs_srv_block;
GLOBALREF CSSL_CB       *Di_slave;      /* slave control block */

/* measure of outstanding ops per slave */

GLOBALREF i4            Di_slave_cnt[];

GLOBALREF CS_SEMAPHORE  DI_misc_sem;

GLOBALREF CS_SEMAPHORE  DI_sc_sem;

GLOBALREF i4		Di_async_io;
GLOBALREF i4		Di_gather_io;
GLOBALREF i4		Di_thread_affinity;
GLOBALREF i4		Di_backend;
GLOBALREF i4		Di_zero_bufsize;
GLOBALREF PTR		Di_zero_buffer;

#if defined(xCL_081_SUN_FAST_OSYNC_EXISTS)
GLOBALREF bool		Di_no_sticky_check;	/* Do we check for O_SYNC
						** when setting the stick bit?
						*/
#endif

GLOBALREF i4		Di_fatal_err_occurred;


/* This size is the default if the di_zero_bufsize config param is not
** specified, or if we're using slaves.
*/

#define      DI_ZERO_BUFFER_SIZE   (1024*1024)

/*
**  Forward and/or External typedef/struct references.
*/

typedef	struct	_DI_OP	DI_OP;

/*
**  Local Definitions.
*/

#define	DI_FULL_PATH_MAX 	(DI_PATH_MAX + DI_FILENAME_MAX + 2)
					/* maximum number of characters in a
					** null terminated full path 
					** specification of a file.  Path +
					** filename + '/' + '\0'.
					*/

/*}
** Name: DI_OP	- DI slave/server control block.
**
** Description:
**	Used by server/di slave subsystem to queue di operations from the
**	server to the slave and for the slave to communicate the results to
**	the server.
**
** History:
**      01-jan-88 (login)
**	    created.
**	19-Feb-1998 (jenjo02)
**	    Added di_file.
*/
struct _DI_OP
{
    i4		di_flags;
# define   DI_OP_RESV	0x01
# define   DI_OP_PIN	0x02
# define   DI_OP_ASYNC  0x04
    i4	di_fd;
    i4  	*di_pin;
    PTR		 di_file;
    CSEV_CB	*di_csevcb;
    DI_SLAVE_CB	*di_evcb;
};




FUNC_EXTERN STATUS 	DI_sense(
				DI_IO	*f,
				DI_OP	*diop,
				i4	*page,
				CL_ERR_DESC	*err_code);

FUNC_EXTERN VOID	DI_slave_send(
				i4         slave_number,
				DI_OP       *di_op,
				STATUS      *big_status,
				STATUS      *small_status,
				STATUS      *intern_status);

FUNC_EXTERN STATUS 	DI_inc_Di_slave_cnt(
				i4         number,
				STATUS      *intern_status);

FUNC_EXTERN STATUS 	DI_dec_Di_slave_cnt(
				i4         number,
				STATUS      *intern_status);

FUNC_EXTERN STATUS 	DI_send_ev_to_slave(
				DI_OP       *di_op,
				i4         slave_number,
				STATUS      *intern_status );

FUNC_EXTERN STATUS	DI_sense(
				DI_IO       *f,
				DI_OP       *diop,
				i4     *end_of_file,
				CL_ERR_DESC *err_code);



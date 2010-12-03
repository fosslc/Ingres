/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: bsshmio.h - common definitions for BS interface to shared memory
**
** History:
**	05-jun-1995 (canor01)
**	    Changed GC_BATCH_SHM structure to have only single 
**	    occurrences of flags, since the entire structure will
**	    be allocated as an array.
**	04-Dec-1995 (walro03/mosjo01)
**	    "volatile" causes compile errors on DG/UX.  Changed to "VOLATILE".
**      28-jun-00 (loera01) Bug 101800:
**          Cross-integrated from change 276901 in ingres63p:
**          Move BCB struct to bsi.h, so that the protocol driver can see the
**          contents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototypes.
*/

# include <sl.h>

#define  SOCK_TRACE(n)	if(sock_trace >= n)(void)TRdisplay
/*
** Name: LBCB - listen control block
*/

typedef struct _LBCB
{
	i4	fd;			/* listen fd */
	char	port[ 108 ];		/* port we're listening on */
} LBCB;

/*
** Name: GC_BATCH_SHM
**
** Description:
**      Header for the batchmode shared memory control block.
**
** History:
**      05-apr-95 (canor01)
**         Created
*/
typedef struct _GC_BATCH_SHM
{
    VOLATILE i4          bat_inuse;
    VOLATILE PID         process_id;   /* [0] = server ID  [1] = client id */
    VOLATILE i4          bat_signal;   /* [0] = server ID  [1] = client id */
    VOLATILE i4          bat_dirty;    /* [0] = server ID  [1] = client id */
    VOLATILE char        bat_id[ 104 ];
    VOLATILE i4          bat_sendlen;     /* number of bytes send             */
    VOLATILE i4          bat_pages;
    VOLATILE i4          bat_offset;
} GC_BATCH_SHM;
 
/* prototypes */

FUNC_EXTERN PTR shm_addr(
	BS_PARMS *bsp,
	i4 op);
FUNC_EXTERN i4 shm_getcliservidx(void);
FUNC_EXTERN bool shm_regfd(
	BS_PARMS *bsp);
FUNC_EXTERN VOID shm_ok(
	BS_PARMS *bsp);

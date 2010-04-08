/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	clstatus.h	- status codes returned by any CL routine
**				  that returns type STATUS
**
** Description:
**	Any CL routine that returns something of type STATUS needs to include
**	this file, as well as anyone who calls such a routine.  There is
**	a companion file, 'status.h', that is used by non-CL routines that
**	return type STATUS.
**
** History:
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**  06-Jul-2005 (fanra01)
**      Bug 114804
**      Consolidate LO status codes into lo.h.
**/

/* BI (block I/O) routines */
#define		BI_DISKISFULL	1
#define		BI_NOTEXIST	2
#define		BI_PRIVILEGE	3
#define		BI_SHORTPAGE	4
#define		BI_DIFFPAGE	5
#define		BI_PARTWRTPG	6
#define		BI_WRONGTYP	7

/* CV routine errors */
#define	CV_BAD_SYNTAX	1300
#define	CV_OVERFLOW	1305
#define	CV_UNDERFLOW	1310
#define	CV_TOO_WIDE	1315

/* IN routine errors */
#define IN_BACKEND_NAME	2100	/* INingres: Unable to start ingres -- unable
				   to find name for backend */
#define IN_NETPROC_NAME	2101	/* INingres: Unable to start ingres -- unable
				   to find name for ntproc */
#define IN_INTR_NAME	2105	/* INingres: Unable to start ingres -- unable
				   to find name for interrupt process */
#define IN_NO_DB_NAME	2110	/* INingres: no database name specified */
#define IN_RD_UNDEF	2115	/*  INread: receive pipe was notdefined
				   INingres did not complete normally */
#define IN_RD_SYSERR	2120	/* INread: unable to get complete PIPEBLK --
				   system error during read */
#define IN_RD_INTR	2125	/* INread: read call was interrupted */
#define IN_RD_PIPE_GONE	2130	/* INread: read pipe is no longer accessible */
#define IN_RD_NOTWHOLE	2135	/* INread: system read function did not write
				   an entire pipe block */
#define IN_RD_CTLC	2140	/* INread: got control-C */
#define IN_WR_UNDEF	2145	/* INwrite: send pipe was notdefined INingres
				   did not complete normally */
#define IN_WR_SYSERR	2150	/* INwrite: unable to send complete PIPEBLK -- 
				   system error during write */
#define IN_WR_INTR	2155	/* INwrite: write call was interrupted */
#define IN_WR_PIPE_GONE	2160	/* INwrite: write pipe is no longer accessible*/
#define IN_WR_NOTWHOLE	2165	/* INwrite: system write function did not write
				   an entire pipe block */
#define IN_WR_CTLC	2170	/* INwrite: got CNTL C */
#define IN_NO_BACKEND	2175	/* INintrpt: there is no ingres backend
				   process to interrupt */
#define IN_FE_KILL	2176	/* Frontend is telling us to cleanup and die */

/* ME routine erros */
#define ME_GOOD	2400	/* ME routine: The Status returned was good  */
#define ME_BD_CHAIN	2410	/* MEdump: correct parameter value must be one
				   of ME_D_ALLOC, ME_D_FREE, ME_D_BOTH  */
#define ME_BD_CMP	2412	/* MEcmp: number of bytes to compare must
				   be > 0  */
#define ME_BD_COPY	2414	/* MEcopy: number of bytes to copy must
				   be > 0  */
#define ME_BD_FILL	2416	/* MEfill: number of bytes to fill must
				   be > 0  */
#define ME_BD_TAG	2418	/* MEt[alloc, free]: tags must be > 0  */
#define ME_ERR_PROGRAMMER 2420	/* MEfree: There is something wrong with the
				   way this routine was programmed.  Sorry.  */
#define ME_FREE_FIRST	2424	/* ME[t]free: can't free a block before any
				   blocks have been allocated  */
#define ME_GONE	2428	/* ME[t]alloc: system can't allocate any more
				   memory for this process  */
#define ME_NO_ALLOC	2430	/* ME[t]alloc: request to allocate a block
				   with zero (or less) bytes was ignored  */
#define ME_NO_FREE	2432	/* MEfree: can't find a block with this
				   address in the allocation list  */
#define ME_NO_TFREE	2434	/* MEtfree: process hasn't allocated any
				   memory with this tag  */
#define ME_00_PTR	2440	/* ME[t]alloc: passed a null pointer  */
#define ME_00_CMP	2442
#define ME_00_COPY	2444	/* MEcopy: passed a null pointer  */
#define ME_00_DUMP	2446	/* MEdump: passed a null pointer  */
#define ME_00_FILL	2448	/* MEfill: passed a null pointer  */
#define ME_00_FREE	2450	/* MEfree: passed a null pointer  */
#define ME_TR_FREE	2460	/* MEfree: the memory has been corrupted  */
#define ME_TR_SIZE	2462	/* MEsize: the memory has been corrupted  */
#define ME_TR_TFREE	2464	/* MEtfree: the memory has been corrupted  */
#define ME_OUT_OF_RANGE 2470	/* ME routine: address is out of process's
				   data space, referencing will cause bus
				   error  */
#define ME_BF_OUT	2480	/* MEneed: 'buf' doesn't have 'nbytes' left
				   to allocate */
#define ME_BF_ALIGN	2482	/* MEinitbuf: 'buf' not aligned */
#define ME_BF_FALIGN	2484	/* MEfbuf: 'buf' not aligned */
#define ME_BF_PARAM	2486	/* MEfbuf: 'bytes' argument must come from
				   call to MEfbuf() */

/* SI routines */
#define	SI_EMFILE	3005	/* Can't open another file -- maximum
					** number of files already opened. */

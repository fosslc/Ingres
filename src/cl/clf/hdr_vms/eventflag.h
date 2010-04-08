/*
**	EVENTFLAG.H --
**
**	This file contains definitions for event flags used throughout ingres.
**	If others are needed, the should be added so as not to conflict with
**	those already present, ESPECIALLY where asynchronous activity is concerned.
**
**	History:
**		2/9/83 (fhc) -- created
**		8/20/84 (kooi) -- added INCONSIST_EF and INTR_SIG_EF.
**			deleted JNL_MBX_EF cause not used anymore.
**			Added KERNEL_LOCK_EF which is used in ingkernel.mar
**			and iisysserv.mar.
*/

# define	SYNC_EF		0		/* normally used for synchronous qiow */
# define	INTR_EF		1		/* used to perform sync io on the interrupt channel */
# define	CACHE_IO_EF	2		/* used to sync io into/outof the cache */
# define	INCONSIST_EF	3		/* set when db goes inconsistent */
# define	INTR_SIG_EF	4		/* set when interrupt arrives from front end */
# define	LOCK_EF		5		/* used to obtain locks */
# define	KERNEL_LOCK_EF	6		/* used in ingkernel.mar for consistency locking */
# define	TIMEOUT_EF	7		/* set when timer expires - timer may be set when waiting for a lock */
# define	CACHE_LOCK_EF	8		/* used by the cache code to obtain nested locks */
# define	ER_EVENT_FLAG	9		/* Error logger. */
# define	TTY_IO_EF	12		/* used to perform io in stdio.c */

/* these next two perform asynchronous io -- therefore be very careful */

# define	NET_WRITE_EF	13		/* used for writing over the network */
# define	NET_INTR_EF	14		/* used to await io over the network interrupt channel */ 
#define		FE_SYNC_EF	15		/* FE comm with local backend */

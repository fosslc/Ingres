/*
**	Internet I/O Definitions!
**    19-Jul-2000 (kinte01)
**       Removed spurious token on preprocesor directive line picked
**       up by 6.2 compiler
*/
#ifndef	IO$S_FCODE
#define	IO$S_FCODE	6
#endif	/*IO$S_FCODE*/

#define	IO$_SEND	(IO$_WRITEVBLK)
#define	IO$_RECEIVE	(IO$_READVBLK)
#define	IO$_SOCKET	(IO$_ACCESS | (0 << IO$S_FCODE))
#define	IO$_BIND	(IO$_ACCESS | (1 << IO$S_FCODE))
#define	IO$_LISTEN	(IO$_ACCESS | (2 << IO$S_FCODE))
#define	IO$_ACCEPT	(IO$_ACCESS | (3 << IO$S_FCODE))
#define	IO$_CONNECT	(IO$_ACCESS | (4 << IO$S_FCODE))
#define	IO$_SETSOCKOPT	(IO$_ACCESS | (5 << IO$S_FCODE))
#define	IO$_GETSOCKOPT	(IO$_ACCESS | (6 << IO$S_FCODE))
#define	IO$_SOCKETADDR	(IO$_ACCESS | (7 << IO$S_FCODE)) /* OBSOLETE */
#define	IO$_IOCTL	(IO$_ACCESS | (8 << IO$S_FCODE))
#define	IO$_PIPE	(IO$_ACCESS | (9 << IO$S_FCODE))
#define	IO$_ACCEPT_WAIT	(IO$_ACCESS | (10<< IO$S_FCODE))
#define	IO$_NETWORK_PTY	(IO$_ACCESS | (11<< IO$S_FCODE))
#define	IO$_SHUTDOWN	(IO$_ACCESS | (12<< IO$S_FCODE))
#define	IO$_GETSOCKNAME	(IO$_ACCESS | (13 << IO$S_FCODE))
#define	IO$_SOCKETPAIR	(IO$_ACCESS | (14 << IO$S_FCODE))
#define	IO$_GETPEERNAME	(IO$_ACCESS | (15 << IO$S_FCODE))

/*
**Copyright (c) 2004 Ingres Corporation
*/


/*
** Name: bstliio.h - common definitions for BS interface to TLI
**
** History:
**	6-Dec-91 (seiwald)
**	    Extracted from bstlitcp.c.
**	8-Sep-92 (gautam)
**          o added a struct t_bind and necessary t_info information
**            into the LBCB for more generic tli_xx routines
**          o MAXPORTNAME is now 64
**          o Added in tli_xxx function definitions into this file
**	17-Sep-92 (gautam)
**          Added in flags field to LBCB and NO_ANON_TBIND define
**	03-Mar-93 (brucek)
**          Added tli_release.
**	18-Mar-94 (rcs)
**	    Bug #59572
**	    _LBCB struct updated to allow the allocation  of
**	    opt and connect buffers for t_listen() - see bstliio.c
**	15-apr-1999 (popri01)
**	    For Unixware 7 (usl_us5), the C run-time variables
**	    sys_errlist and t_errlist are not available in a dynamic 
**	    load environment. Use the strerror and t_strerror 
**	    functions instead.
**      28-jun-00 (loera01) Bug 101800:
**          Cross-integrated from change 276901 in ingres63p:
**          Move BCB struct to bsi.h, so that the protocol driver can see the
**          contents.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

VOID	tli_addr();
VOID	tli_listen();
VOID	tli_unlisten();
VOID	tli_accept();
VOID	tli_request();
VOID	tli_connect();
VOID	tli_send();
VOID	tli_receive();
VOID	tli_release();
VOID	tli_close();
VOID	tli_detect();
bool	tli_regfd();
VOID	tli_sync();

extern int	errno;
extern int	t_errno;
#ifndef usl_us5
extern char *	t_errlist[];
extern char *	sys_errlist[];
#endif /* !usl_us5 */

#define TLIDEBUG
#ifdef	TLIDEBUG
extern int tli_debug;
#define  TLITRACE(n)	if(tli_debug >= n)(void)TRdisplay
#endif


/*
** Name: LBCB - listen control block
*/

#ifndef	MAXPORTNAME
#define	MAXPORTNAME	64
#endif

#define MAXDEVNAME	64

typedef struct _LBCB
{
	i4	fd;			   /* listen fd */
	char	port[ MAXPORTNAME ];	   /* port we're listening on */
	char	device[ MAXDEVNAME ];	   /* listen device, if given */
	i4	q_max;         	   	   /* how many listens do we Q up */
	i4	q_count;                   /* indexes  t_call array */
	char	*addrbuf;   		   /* bound t_bind transport address */
	int	addrlen;   		   /* length of transport address */
	long	maxaddr;   		   /* max transport address size */ 
	long	maxopt;   		   /* max protocol specific opts size */
#define MAX_OPT_LEN      512
	long	maxconnect;	           /* max connect data size */
#define MAX_CONNECT_LEN  128
	long	tsdu;			   /* max tsdu size, -1 if unknown */
	long	servtype;   		   /* service type provided */
	long	flags;   		   /* store miscellaneous flags */
	struct  t_call	**q_array; 	   /* t_call pointer array */
	struct	t_bind bind;		   /* qlen and listen struct netbuf */
} LBCB;

/* 
** Miscellaneous flags set in the LBCB's flags field
*/

/* defined if you cannot do anonymous t_bind - as in HP's OSI */
# define NO_ANON_TBIND    	0x1   

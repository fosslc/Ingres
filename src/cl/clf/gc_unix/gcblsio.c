/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: gcblsio.c - unix BLAST Serial I/O driver for GCC ASYNC Protocol
**
** Description:
**	Perform serial I/O functions for unix-based BLAST asynchronous 
**	protocol driver.
**
** History:
**	29-Jan-92 (alan)
**	    Added header and renamed.
**	28-Apr-92 (smc)
**	    Moved include of systypes.h ahead of the header files that
**          require it.		
**	29-Apr-92 (johnst)
**	    Changed the unconditional calls to gettimeofday() to be conditional
**	    on the xCL_005_GETTIMEOFDAY_EXISTS capability. On machines for
**	    which gettimeofday is unusable (ICL DRS6000 for example), use the
**	    TMet() time function which is already conditional on the
**	    xCL_005_GETTIMEOFDAY_EXISTS capability and provides the required
**	    functionality.
**	28-oct-92 (peterk)
**	    rename variable stime to etime; stime conflicts with a library
**	    function in Sun Solaris 2.0.
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in setto() and sioread() to describe 
**	    2 versions of gettimeofday() available.
**      29-Nov-94 (nive)
**              For dg8_us5, chnaged the prototype definition for
**              gettimeofday to gettimeofday ( struct timeval *, struct
**              timezone * )
**	12-Apr-95 (georgeb)
**	    gettimeofday() requires two args on Unixware 2.0, the second of
**	    which should be NULL. Added if defined(usl_us5).
**      19-may-1995 (canor01)
**              changed errno to errnum for reentrancy
**	23-May-95 (walro03)
**              Reversed previous change (29-Nov-94).  A better way is to use
**              the correct prototype based on xCL_GETTIMEOFDAY_TIMEONLY vs.
**              xCL_GETTIMEOFDAY_TIME_AND_TZ.
**      15-jun-95 (popri01)
**          Add support for additional, variable parm list, GETTIME prototype
**	19-sep-96 (nick)
**	    usl_us5 now defines xCL_GETTIMEOFDAY_TIME_AND_TZ
**  20-feb-97 (muhpa01)
**      Removed local declaration for gettimeofday() for hp8_us5 to fix
**      "Inconsistent parameter list declaration" error.
**  29-may-97 (muhpa01)
**      For hp8_us5 rename local function ltoa to ii_ltoa to quiet HP-UX
**	compiler complaint about inconsistency with system function ltoa
**      23-Sep-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
**  14-Feb-2000 (linke01)
**          Remove local declaration of gettimeofday() for usl_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	09-Jan-2001 (hanje04)
**	    Changed VMIN to TE_VMIN and VTIME to TE_VTIM to impliment fix
**	    for bug 103708
**      18-Apr-2005 (hanje04)
**          Add support for Max OS X (mac_osx)).
**          Use calls to tcflush() and tcflow() calls where outdated ioctl()
**	    calls have been depricated.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	15-nov-2010 (stephenb)
**	    prototype all functions fully.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Minor added prototype cleanup.
*/

/********************************************************************
BLAST Version 8   		(c)1986 Communications Research Group
---------------------------------------------------------------------
SIODVR.C			- UNIX Serial i/o driver
---------------------------------------------------------------------
SIODVR contains the following callable entry points:

sioinit()	initialize the serial io driver.
		set any local variables, allocate any pipes or buffers
		does NOT init comm port or console (read further).

siodone()	shutdown the serial io driver.

cominit(dev)	initialize the communications port.  This is called
char *dev;	to allocate/open the communication port.  It may be
		called more than once in a communications session.

comdone()	release the communications port.

setto(t)	set a timeout value of "t" hundredths of seconds.
int t;		timeout will be indicated be a SIO_TIMO return value
		from siopn().

getcm(p)	Get communications port byte and place it at "p".
char *p;	The arrival of a byte from the comm port will be indicated
		by a SIO_COMRD value from siopn().

putcm(p,n)	Write out bytes to the communications port. "N" bytes
char *p;	at location "*p" will be written to the communications 
int n;		port.  The completion of the write will be indicated by
		a SIO_COMWR value from siopn().

siopn()		Pend for input/output or timer completion.
		Siopn() will return a unique code indicating which event
		occurred.

comst(p,br,py)	Set communications port operating characteristics.
char *p;	"P" is the device name of the port.
int br;		"Br" is an integer representing the baud rate. (see sio.h)
int py;		"Py" is an integer representing the parity setting.

commd(m,x)	Set comm port operating mode.
int m;		"M" sets either byte at a time, bunch at a time, or line
int x;		at a time comm port input. "X" is non-zero if flow/control
		may be used.

comxon()	Force X'off'd output back on.  Comxon() 
		should restart output that might be suspended 
		due to the receipt of an XOFF.
*/

/* 
** NOTE: Code designated by #else xCL_018_TERMIO_EXISTS has not been
** tested and is not verified as being correct.
*/

#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <clpoll.h>
#include <clsigs.h>
#include <ernames.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <tr.h>
#include <tecl.h>
#include <gcccl.h>
#if !defined (xCL_005_GETTIMEOFDAY_EXISTS)
#include <tm.h>
#endif

# if defined(any_hpux)
#define ltoa ii_ltoa
# endif

#include <gcblerrs.h>
#include <fcntl.h>
#include <sys/time.h>

#ifdef	xCL_018_TERMIO_EXISTS			/* include the terminal control headers files */
#include <termio.h>
#define TTYSTRUCT termio
#elif defined(xCL_TERMIOS_EXISTS)
#include <termios.h>
#include <sys/ioctl.h>
#define TTYSTRUCT termios
#define TCGETA TIOCGETA
#define TCSETAW TIOCSETAW 
#define TCSETA TIOCSETA
#define TCFLSH TIOCFLUSH
#else /* xCL_TERMIOS_EXISTS */
#include <sgtty.h>
#define TTYSTRUCT sgttyb
#endif	/* xCL_018_TERMIO_EXISTS */

#include <gcblsio.h>
#include <stdio.h>
#include <errno.h>

#define HDLCKDIR	"/usr/spool/locks"	/* Honey-Danbar uucp */
#define LCKDIR		"/usr/spool/uucp"	/* UUCP Lock Directory */
#define LCKPFX		"/LCK.."		/* UUCP Lock prefix */

static i4  SIO_trace = 0;

#define SIOTRACE(n) if( SIO_trace >= n) (void) TRdisplay

/*
**	Local variables
*/
static int comfd;	    	    	/* comm port file descriptor */

/* tty control structs */
static struct TTYSTRUCT comctl;	    	/* current commport tty settings */
static struct TTYSTRUCT ocomctl;    	/* saved previous commport settings */

static bool comflg; 			/* TRUE if comm port was initialized */
static int comhost;	    		/* host mode on comm port */
static char lockfn[64];		    	/* uucp lock file name */
static int locked = FALSE;	    	/* true if lock file was created */

/* comm port read variables */
#define COMBUFSIZE	512
static char cmrbf[COMBUFSIZE];      	/* read buffer */
static char *cmrbp; 	    	    	/* read buffer pointer */
static int  cmrct;  	    	    	/* comm port read count */
static char *cmrpr;	    	    	/* comm port receive pointer */
static int cmrerr;  	    	    	/* comm port read error */
static int cmrfdreg;	    	    	/* comm port read fd registered */

/* comm port send variables */
static char *cmspr;	    	    	/* comm port send pointer */
static int cmsct;	    	    	/* comm port send count */
static int cmserr;  	    	    	/* comm port send error */
static int cmsfdreg;	    	    	/* comm port send fd registered */
static int cmsoptim;			/* optimize send */

/* timer variables */
static i4 timct;	    	    	/* timeout interval count */
static int timon;	    	    	/* timer on flag */
static int timout;	    	    	/* timer interval elapsed */

# ifdef xCL_005_GETTIMEOFDAY_EXISTS
static struct timeval timtod;	    	/* time of day */
static struct timezone timtz;  	    	/* timezone */
# else
SYSTIME etime;				/* time struct returned by TMet() */
# endif

/* miscellaneous */
static int ppid;    	    		/* my process id */

/* Callback functions */
static VOID (*cmread) (void *, i4);	/* called when read completes */
static VOID (*cmwrite) (void *, i4);	/* called when write completes */
static void *cmclosure;	    	    	/* closure parm for read/write */

/* Forward References */
FUNC_EXTERN VOID    comdone(void);
FUNC_EXTERN VOID    comxon(void);
static	VOID	    ltoa(char *, long, int, int);
FUNC_EXTERN VOID    siowrite(int);
int 		    cominit( char * );
VOID		    commd( int, int );
int		    comst( char *, int, int );
VOID		    getcm( char * );
int		    iopnd( SIO_CB * );
VOID		    putcm( char *,int );
VOID		    setto( int );
VOID		    siodone(void );
int		    sioinit(void );
VOID		    sioread(int );
VOID		    sioreg( void (*)(), void (*)(), void *);

/* Global variable references */
GLOBALREF   int	    errno;


/*{
** Name: sioinit
**
** Description: 
**      initialize serial i/o driver
**
** Inputs:
**  	none
**
** Outputs:
**  	none
**	
** Returns:
**	OK -	    serial i/o driver initialized
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**  	12-Feb-91 (cmorris)
**  	    	set up handler for timer expirations
**		
*/
int
sioinit(void )
{
    char *trace;

    NMgtAt( "II_SIO_TRACE", &trace );

    if( trace && *trace )
	SIO_trace  = atoi( trace );

    /* init local variables */
    ppid = getpid( );
    comflg = FALSE;

    timct = -1;
    timon = FALSE;
    timout = FALSE;

    /* all done */
    return( OK );
}

/*{
** Name: siodone
**
** Description: 
**      shutdown the serial i/o driver
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	OK  	- driver has been shut down
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	        added calls to unregister file descriptors.
**		
*/
VOID
siodone(void )
{

    /* unregister file descriptors */
    (VOID)iiCLfdreg(comfd, FD_READ, NULL, NULL, -1);
    (VOID)iiCLfdreg(comfd, FD_WRITE, NULL, NULL, -1);
    return;
}

/*{
** Name: cominit
**
** Description: 
**      initialize the communications port
**
** Inputs:
**  	dev -	    device to initialize. If null, device to
**  	    	    use is stdin
**
** Outputs:
**	none
**	
** Returns:
**	OK  	-   device has been initialized
**     !OK  	-   error code indicating reason for failure
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**		
*/
int
cominit( dev )
char *dev;
{
    int i, honey;
    char *pf, buf[11];

    cmspr = 0;	    	    	    	/* no send data */
    cmsct = 0;
    cmserr = 0;
    cmsfdreg = FALSE;
    cmsoptim = TRUE;			/* first send can be optimized */
    cmrpr = 0;	    	    	    	/* no receive data */
    cmrct = 0;
    cmrerr = 0;
    cmrfdreg = FALSE;

    /* passed this way before? */
    if( comflg )
    	comdone( );

    if( !*dev ) 
    	comhost = TRUE;
    else
        comhost = FALSE;

    if( !comhost )  	    		/* not going through the console */
    {
	/* if lock directory is accessible
	** make a lock file */
	if( access( HDLCKDIR, 07 )==0 ) /* honey-danbar locking */
	{
	    strcpy( lockfn, HDLCKDIR );
	    honey = TRUE;
	}
	else
    	    if( access( LCKDIR, 07 )==0)
    	    	    	    	    	/* old uucp */
	    {
		strcpy( lockfn, LCKDIR );
		honey = FALSE;
	    }
	    else
    	 	lockfn[0] = 0;

	if( lockfn[0] )     		/* have a lock directory */
	{
    	    (VOID) STcat( lockfn, LCKPFX );
	    pf = dev + STlength( dev );

	    /* use only the end of the device name */
	    while(( pf > dev ) && *pf != '/' ) 
    	    	--pf;
	    if( *pf == '/' ) 
    	    	++pf;
	    (VOID) STcat( lockfn, pf );

	    /* the lock pathname is built,
	    ** does a lock file exist?
	    */
	    if( access( lockfn, 04 ) == 0 )
		return( ERINCMLK );

    	    /* well then create one */
	    i = creat( lockfn, 0666 );
	    if( honey ) 
    	    {
		ltoa( buf, ppid, 10, TRUE );
		buf[10] = '\n';
		write( i, buf, 11 );
	    }
    	    else 
    	    {
		write( i, &ppid, 4 );
	    }
	    locked = TRUE;
	    close( i );
	    chmod( lockfn, 0666 );
	}

    	/* open port */
	if(( comfd = open( dev,O_RDWR+O_NDELAY )) < 0 ) 
    	    return( ERINCMOP );
    }
    else	
    {
    	comfd = dup( 1 );	    	/* using console, stdout */

    	/* use no delay so we don't block */
	if(( i = fcntl( comfd, F_GETFL, 0 )) < 0 ) 
    	    return( ERINCMCN );

	if( i = fcntl( comfd, F_SETFL, i&O_NDELAY ) < 0 ) 
    	    return( ERINCMCN );
    }

    /*
    **  save current comm-port characteristics
    */
#if defined(xCL_018_TERMIO_EXISTS) || defined(xCL_TERMIOS_EXISTS)
    if( i = ioctl( comfd, TCGETA, &ocomctl ) != 0 ) 
        return( ERINCMCN );
#else	/* xCL_018_TERMIO_EXISTS */
    if( i = ioctl( comfd, TIOCGETP, &ocomctl ) != 0 ) 
        return( ERINCMCN );
#endif	/* xCL_018_TERMIO_EXISTS */

    comflg = TRUE;		/* set flag to show we've been here */
    return( OK );
}

/*{
** Name: comdone
**
** Description: 
**      free communications port
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**		
*/
VOID
comdone(void )
{

    /* ---- restore and release the comm port ---- */
    if( comfd )
    {
#ifdef	xCL_018_TERMIO_EXISTS
	ioctl( comfd, TCXONC, 1 );
	ioctl( comfd, TCFLSH, 2 );
	ioctl( comfd, TCSETA, &ocomctl );
#elif defined(xCL_TERMIOS_EXISTS)
	tcflow( comfd, TCOON );
	tcflush( comfd, TCOFLUSH );
	ioctl( comfd, TCSETA, &ocomctl );
#else	/* xCL_018_TERMIO_EXISTS */
	ioctl( comfd, TIOCSETP, &ocomctl );
#endif	/* xCL_018_TERMIO_EXISTS */
	close( comfd );
	comfd = 0;
    }

    /* ---- clear the lock file ---- */
    if( locked )
    {
	locked = FALSE;
	unlink( lockfn );
    }

    comflg = FALSE;
}

/*{
** Name: setto
**
** Description: 
**      set a timeout value
**
** Inputs:
**  	to  	-   timeout value in 1/100ths of a second
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	03-Mar-91 (cmorris) 
**  	    	Added call to iiCLfdreg
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in setto() to describe 2
**          versions of gettimeofday() available.
**	19-sep-96 (nick)
**	    usl_us5 now defines xCL_GETTIMEOFDAY_TIME_AND_TZ
**      26-mar-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
*/
VOID
setto( to )
int to;
{
    if (to >= 0)
    {
    	timct = (i4) to * 10;
    	timon = TRUE;

    	/* Take a snapshot of the time */

# ifdef xCL_005_GETTIMEOFDAY_EXISTS

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
    	gettimeofday(&timtod);
	SIOTRACE(2) ("setto: %d seconds\n", timtod.tv_sec);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    	gettimeofday(&timtod, &timtz);
	SIOTRACE(2) ("setto: %d seconds\n", timtod.tv_sec);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
    	gettimeofday(&timtod, &timtz);
	SIOTRACE(2) ("setto: %d seconds\n", timtod.tv_sec);
# endif

# else
	TMet(&etime);
	SIOTRACE(2) ("setto: %d seconds\n", etime.TM_secs);
# endif /* xCL_005_GETTIMEOFDAY_EXISTS */

    }
    else
    {
    	timct = -1;
    	timon = FALSE;
    }
    timout = FALSE;

    /* 
    ** If the read file descriptor is registered for i/o completion,
    ** reregister it with timeout.
    */
    if (cmrfdreg)
    {
    	(VOID)iiCLfdreg(comfd, FD_READ, cmread, cmclosure, timct);
    }
}

/*{
** Name: putcm
**
** Description: 
**      send a string of bytes to the comm port
**
** Inputs:
**  	ptr 	-   address of string
**  	cnt 	-   number of bytes in string
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	Added call to iiCLfdreg
**      24-Jul-91 (cmorris)
**		Added support for send optimization
**		
*/
VOID
putcm( ptr,cnt )
char *ptr; 
int cnt;
{
    cmspr = ptr;
    cmsct = cnt;
    SIOTRACE(1)("putcm: request to write %d bytes to fd %d\n", cnt, comfd);

    if (cmsoptim)
    {
        /* try send right away */

	siowrite(0);
    }
    else
    {
        /* Register for i/o completion */

        (VOID)iiCLfdreg(comfd, FD_WRITE, cmwrite, cmclosure, -1);
        cmsfdreg = TRUE;
    }
}

/*{
** Name: getcm
**
** Description: 
**      get a single byte from the comm port
**
** Inputs:
**  	ptr 	-   address to place byte
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	Added call to iiCLfdreg
**		
*/
VOID
getcm( ptr )
char *ptr;
{
    cmrpr = ptr;

    /* See whether we need to register for i/o completion */
    if (!cmrct)
    {
        SIOTRACE(1)("getcm: request to read from fd %d, timeout %d milliseconds\n", comfd, timct);

    	/* no data buffered down here */
   	(VOID)iiCLfdreg(comfd, FD_READ, cmread, cmclosure, timct);
    	cmrfdreg = TRUE;
    }
}

/*{
** Name: iopnd
**
** Description: 
**      pend for serial i/o function completion
**
** Inputs:
**    	none
**
** Outputs:
**	none
**	
** Returns:
**  	0 - Unrecoverable Error
**      1 - Timer interval elapsed.
**      2 - Console byte receive is done.
**      3 - Comm port receive is done.
**      4 - Comm port send is done.
**      5 - Console interrupt char detected.
**   	6 - Blocked for i/o.
**
** Exceptions:
**	none
**
** History:
**	11-Feb-91 (cmorris) 
**  	    	rewritten for CLpoll event handling
*/
int
iopnd( siocb )
SIO_CB *siocb;
{
    static int i;
	
    if( timon && timout ) 		/* timed out? */
    {
	timon = FALSE;
	timout = FALSE;
	return( SIO_TIMO );
    }

    if( cmserr)	    	    	    	/* send error? */
    {
    	siocb->status = GC_SEND_FAIL;
    	siocb->syserr->callid = ER_write;
    	siocb->syserr->errnum = cmserr;
    	cmserr = 0;
    	return (SIO_ERROR);
    }

    if( !cmsct && cmspr ) 		/* send done? */
    {
    	cmspr = 0;
	return( SIO_COMWR );
    }

    if ( cmrerr)    	    	    	/* receive error? */
    {
    	siocb->status = GC_RECEIVE_FAIL;
    	siocb->syserr->callid = ER_read;
    	siocb->syserr->errnum = cmrerr;
    	cmrerr = 0;
    	return(SIO_ERROR);
    }

    if( cmrct && cmrpr ) 		/* receive data ready? */
    {
	cmrct--;
	*cmrpr = *cmrbp++;
	cmrpr = 0;
	return( SIO_COMRD );
    }

    return( SIO_BLOCKED );
}	/* end of iopnd */

/*{
** Name: comst
**
** Description: 
**      set comm port operating characteristics
**
** Inputs:
**  	portid	-   	not used
**  	bdrt	-   	baud rate
**  	par 	-   	parity
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**      12-Dec-91 (cmorris)
**  	    	If comhost, use all current control settings
**		
*/
int
comst( portid, bdrt, par )
char *portid;   
int bdrt, par;
{
    int i;
#if defined(xCL_018_TERMIO_EXISTS) || defined(xCL_TERMIOS_EXISTS)
    static int bdtab[8] = 
	{ B300, B600, B1200, B2400, B4800, B9600, B19200, B38400 };
    static int patab[5] = { 0, PARENB|PARODD, PARENB, 0, 0 };
    static int dbtab[5] = { CS8, CS7, CS7, CS8, CS8 };

    comctl.c_iflag = IGNBRK | IGNPAR | ISTRIP;
    comctl.c_oflag = 0;
    if( comhost )		/* use current settings */
    	comctl.c_cflag = ocomctl.c_cflag;
    else
    {
        comctl.c_cflag = bdtab[bdrt]|patab[par]|dbtab[par];
        comctl.c_cflag |= CREAD;
        comctl.c_cflag |= ocomctl.c_cflag & CLOCAL; 
    }

    comctl.c_lflag = 0;

    /* Our event handling mechanism requires us not to wait */
    comctl.c_cc[TE_VMIN] = 0;
    comctl.c_cc[TE_VTIME] = 0;
    i = ioctl( comfd, TCSETAW, &comctl );
# ifdef TCXONC
    i = ioctl( comfd, TCXONC, 1 );
# else
    i =	tcflow( comfd, TCOON );
# endif

#else	/* xCL_018_TERMIO_EXISTS */
    static int bdtab[8] = 
	{ B300, B600, B1200, B2400, B4800, B9600, B19200, B38400 }; 
    static int patab[5] = { ANYP, ODDP, EVENP, ANYP, ANYP };
    static int dbtab[5] = { 0, ISTRIP, ISTRIP, ISTRIP, ISTRIP };

    if( comhost )
	i = ocomctl.sg_ospeed & CBAUD;
    else
	i = bdtab[ bdrt ];
    comctl.sg_ispeed = i | dbtab[ par ];
    comctl.sg_ospeed = i;
/* ??? c_cflag is xCL_018_TERMIO_EXISTS stuff
    comctl.c_cflag |= ocomctl.c_cflag & CLOCAL;
*/
    comctl.sg_flags = RAW | patab[ par ];
    ioctl( comfd, TIOCSETP, &comctl );
#endif	/* xCL_018_TERMIO_EXISTS */
    return( OK );
}

/*{
** Name: commd
**
** Description: 
**      set comm port mode
**
** Inputs:
**  	mode	-   	(read mode)
**  	    	    	-1 line reads
**  	    	    	 0 byte reads
**  	    	    	 1 multi-byte reads
**  	xon 	-   	(flow control)
**  	    	    	 0 no flow control
**  	    	    	 1 xon/xoff flow control
**  	    	    	 2 dtr/cts flow control
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**		
*/
VOID
commd( mode, xon )
int mode;
int xon;
{
    int i;

#if defined( xCL_018_TERMIO_EXISTS ) || defined( xCL_TERMIOS_EXISTS )
    ioctl( comfd, TCGETA, &comctl );

/* NOTE: because of our buffer sizes and event handling mechanism, we always
   do multiple byte reads */

    if( xon ) 
	comctl.c_iflag |= ( IXON | IXOFF );
    else 
	comctl.c_iflag &= ~( IXON | IXOFF );
    ioctl( comfd, TCSETAW, &comctl );  
#else	/* xCL_018_TERMIO_EXISTS */
    ioctl( comfd, TIOCGETP, &comctl );
    if( xon ) 
	comctl.sg_flags |= TANDEM;
    else 
	comctl.sg_flags &= ~TANDEM;
    ioctl( comfd, TIOCSETP, &comctl );
#endif	/* xCL_018_TERMIO_EXISTS */
    comxon();
}

/*{
** Name: comxon
**
** Description: 
**      force xoff'd output back on
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**  	nothing
**
** Exceptions:
**	none
**
** History:
**	08-Feb-91 (cmorris) 
**  	    	cleaned up
**		
*/
VOID
comxon(void)
{
    char contQ = '\021';

#ifdef	xCL_018_TERMIO_EXISTS
    ioctl( comfd, TCXONC, 1 );
#elif defined(xCL_TERMIOS_EXISTS)
    tcflow( comfd, TCOON );
#endif	/* xCL_018_TERMIO_EXISTS */
    write( comfd, &contQ, 1 );
}

/*{
** Name: sioreg()
**
** Description: 
**  	Register callbacks. These are then passed on to iiCLfdreg
**  	as I/O completion callbacks.
**	
**
** Inputs:
**      cmread			Function to call on completion of read
**	cmwrite			Function to call on completion of write
**  	cmtimer	    	    	Function to call on timer expiration
**  	cmclosure   	    	Closure parameter
**
**
** Outputs:
**	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	01-May-90 (cmorris)
**		Initial coding.
*/
VOID
sioreg( void (*cmrdfunc)(void *, i4), void (*cmwtfunc)(void *, i4),
	void * cmclosparm)
{
    /* register functions */
    cmread = cmrdfunc;
    cmwrite = cmwtfunc;
    cmclosure = cmclosparm;
}

/*{
** Name: sioread()
**
** Description: Complete serial I/O read
**	
**
** Inputs:
**  	error - indicates whether read timed out
**
** Outputs:
**	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	01-May-90 (cmorris)
**		Initial coding.
**      20-apr-94 (mikem)
**          Added use of 2 new #defines (xCL_GETTIMEOFDAY_TIMEONLY and
**          xCL_GETTIMEOFDAY_TIME_AND_TZ) in sioread() to describe 2
**          versions of gettimeofday() available.
**      26-mar-1997 (kosma01)
**          Added xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR for rs4_us5
**          to agree with arguments of gettimeofday() in <sys/time.h>.
**	04-Apr-2001 (bonro01)
**	    Corrected Use of TMet() routine.  The returned time is in
**	    milliseconds, not microseconds.
**	15-Feb-2008 (hanje04)
**	    SIR
**	    Shouldn't be redefining system functions, causes compilation 
**	    errors.
*/
VOID
sioread(error )
int error;
{
    int n;
# ifdef xCL_005_GETTIMEOFDAY_EXISTS
    struct timeval newtod;	    	/* time of day */
# else
    SYSTIME newetime;
# endif

    cmrerr = 0;
    cmrfdreg = FALSE;

    /* See whether we timed out or completed i/o */

    if (error == OK)
    {

        /* do the read */
    	if((n = read( comfd, cmrbf, COMBUFSIZE)) <= 0)
    	{
    	    if (n < 0)
    	    {
    	    	/* see what error occured */
    	    	if (errno != EWOULDBLOCK && errno != EINTR)

    	    	    /* squirrel away error */
    	    	    cmrerr = errno;
	    	n = 0;
            }

	}

    	SIOTRACE(1)("sioread: read %d bytes from fd %d\n", n, comfd);
    	cmrbp = cmrbf;
    	cmrct = n;

    	/* 
    	** If a timer is running, take a snapshot of the time and
    	** decrement our timer by the time that has elapsed since
    	** the timer was set.
    	*/
    	if (timon)
    	{
# ifdef xCL_005_GETTIMEOFDAY_EXISTS

# if defined(xCL_GETTIMEOFDAY_TIMEONLY)
    	    gettimeofday(&newtod);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_TZ)
    	    gettimeofday(&newtod, &timtz);
# elif defined(xCL_GETTIMEOFDAY_TIME_AND_VOIDPTR)
    	    gettimeofday(&newtod, &timtz);
# endif
    	    timct -= (newtod.tv_sec - timtod.tv_sec) * 1000;
    	    timct -= ((newtod.tv_usec - timtod.tv_usec)/1000);
# else
	    TMet(&newetime);
    	    timct -= (newetime.TM_secs - etime.TM_secs) * 1000;
    	    timct -= ((newetime.TM_msecs - etime.TM_msecs));
# endif /* xCL_005_GETTIMEOFDAY_EXISTS */
       	    SIOTRACE(2)("sioread: %d milliseconds to expiration\n", timct);
    	    if (timct <= 0)
    	    {
    	    	/* timer has expired ! */
    	    	timout = TRUE;
    	    	timct = -1;
	    }
    	    else
	    {
    	    	/* store new wallclock time */

# ifdef xCL_005_GETTIMEOFDAY_EXISTS
                timtod.tv_sec = newtod.tv_sec;
    	        timtod.tv_usec = newtod.tv_usec;
# else
    	        etime.TM_secs = newetime.TM_secs;
    	        etime.TM_msecs = newetime.TM_msecs;
# endif /* xCL_005_GETTIMEOFDAY_EXISTS */
	    }
	}

	/* If we didn't read anything, repost read */
	if (n == 0)
	{
	    (VOID)iiCLfdreg(comfd, FD_READ, cmread, cmclosure, timct);
    	    cmrfdreg = TRUE;
	}

    }
    else
    {
    	/* timed out:- mark this fact for when we reenter iopnd */
    	timout = TRUE;
    	timct = -1;
    	SIOTRACE(1)("sioread: timed out\n");
    	    	
    	/*
    	** As we timed out, the read did not complete so we must
    	** reregister for i/o completion. As the timer timed
    	** out we reregister with an infinite timeout period
    	** as no timer is now running.
    	*/
    	(VOID)iiCLfdreg(comfd, FD_READ, cmread, cmclosure, timct);
    	cmrfdreg = TRUE;
    }
}

/*{
** Name: siowrite()
**
** Description: Complete serial I/O write
**	
**
** Inputs:
**  	error - indicates whether write timed out (unused)
**
** Outputs:
**	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	01-May-90 (cmorris)
**		Initial coding.
*/
VOID
siowrite(error )
int error;
{
    int n;

    cmserr = 0;
    cmsfdreg = FALSE;
    cmsoptim = TRUE;

    /* write out anything to send */
    if ((n = write( comfd, cmspr, cmsct )) < 0)
    {
    	if (errno != EWOULDBLOCK)

    	    /* squirrel away error */
    	    cmserr = errno;

    	n = 0;
    }

    SIOTRACE(1)("siowrite: written %d bytes to fd %d\n", n, comfd);
    cmsct -= n;
    if (cmsct)
    {
    	/* Still more to send; reregister */
        (VOID)iiCLfdreg(comfd, FD_WRITE, cmwrite, cmclosure, -1);
    	cmsfdreg = TRUE;
	cmsoptim = FALSE;
    }
}

/*{
** Name: ltoa
**
** Description: Convert long into ascii string
**	
**
** Inputs:
**      ptr -	    	address of ascii output
**  	lvalue -	number to convert
**  	siz -	        number of digits
**  	z -	    	leading zeroes are suppressed if TRUE
**
** Outputs:
**	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	12-Feb-91 (cmorris)
**		Moved in from utility.c
*/
static VOID
ltoa( ptr, lvalue, siz, z )	/* convert long int to ascii string */
char *ptr;	
long lvalue;	
int siz;	
int z;		
{
    static long power[10] = 
    	{ 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 
	  10, 1 };

    int i;
    char *temp;

    temp = ptr;
    for( i=0; i<10; i++ ) 
    {
	*temp = '0';
	while( lvalue >= power[i] )
	{
	    if( i >= ( 10 - siz )) 
    	    	(*temp)++;
	    lvalue -= power[i];
	}
	if( i >= ( 10 - siz )) temp++;
    }

    temp = ptr;
    if( z )
	while(( *temp == '0' )&&( --siz )) *temp++ = ' '; 
}

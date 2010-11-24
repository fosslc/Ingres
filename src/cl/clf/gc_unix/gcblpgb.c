/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: gcblpgb.c - BLAST pack/unpack data for use with GCC ASYNC Protocol
**
** Description:
**	This source file was provided as part of 'BLAST' protocol
**	support.  It is included as support routines for the GCC 'ASYNC'
**	protocol driver.
**
** History:
**	29-Jan-92 (alan)
**	    Added header and renamed.
**	31-Jan-92 (alan)
**	    Changed name of GCasync_trace global to GCblast_trace
**  	10-Dec-92 (cmorris)
**  	    Removed include of ch.h.
**      16-mar-93 (smc)
**          Added forward declarations of inc_seqn & inc_wdwn.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	03-jun-1996 (canor01)
**	    New ME for operating-system threads does not need external
**	    semaphores. Removed ME_stream_sem.
**	    Made bitmaps unsigned to prevent sign propagation on shifts.
**	10-may-1999 (popri01)
**	    Add mh header file, especially for MHrand() prototype, which
**	    returns an f8.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**  	15-nov-2010 (stephenb)
**  	    correct proto all forward functions.
*/


/*


      Module:   BLAST B Protocol Generator
Source Files:   PGB.C - BLAST 'B' Protocol
  Originator:   Chris Roussel		(c)1984 CRG, inc.
        Date:   9/27/85

Summary:
--------

This module contains the routines necessary to provide 
an error-free data-link to a remote system using the BLAST B
protocol.  The calls themselves are structured to provided 
clean separation of duties from this, the data-link layer, 
and other layers.


Entry Points:
-------------

pgini( )

Initializes all the protocol generator data structures.


pglgn( )

Attempts BLAST logon with the remote system.  Returns a 0 if
timed out, or 1 if logon accomplished.


pgcnt()

Attempts connect with remoter system using a connection string.

pglgo( )

Perform an orderly shutdown of the protocol generator
with the remote system.

pgsnd( ptr, cnt )  		char *ptr; int cnt;

Informs the protocol generator of a block of data
to be transmitted error-free to the remote system.  The values
of 'ptr' and 'cnt' are copied to local variables and return
is immediate.  The block will be packetized and sent when
pgpnd is called.


pgrd( ptr )				char *ptr; 

Informs the protocol generator of an area of memory
where data received error-free from the remote system may be
placed. Execution is similar to 'pgwrt'.  The value of 'cnt'
indicates the maximum number of bytes that may be transferred.
The actual number of bytes transferred to the caller's buffer is 
returned by the '*val' argument to pgpnd.


pguisnd( ptr, cnt )		char *ptr;  int cnt;

The block of data specified by ptr and cnt is packaged as a
"un-numbered information" block and sent to the remote the
next time pgpnd is called.  Normal "numbered information"
blk transmission is suspended until the UI blk has been
sent and acknowledged.  The acknowledgement is indicated 
be a unique return code from pgpnd.


pguird( ptr )			char *ptr;

"ptr" points to an area of memory where a received UI blk
will be placed.  When a UI blk is received, it will be
indicated by a unique return code from pgpnd.

pgpnd( val )  			int *val;

Passes control to the protocol generator in order to perform
the previously specified read and/or write.  The global int
'timo' specifies a timeout interval in seconds after which
pgpnd will abort if no good blks have been received ( 0 = 
infinite ). 'val' is the address of an int where an additional
value can be returned if required.

The pgpnd return codes are defined in the header file "gcblpgb.h".

*/


#include <compat.h>
#include <gl.h>
#include <st.h>
#include <gcccl.h>
#include <me.h>
#include <gcblsio.h>
#include <gcblpgb.h>
#include <mh.h>

static i4  PGB_trace = 0;

#define PGBTRACE(n) if( PGB_trace >= n) (void) TRdisplay

/* External functions */

FUNC_EXTERN 	int 	pk34();
FUNC_EXTERN 	int 	upkf34();
FUNC_EXTERN 	int 	pk78();
FUNC_EXTERN 	int 	upkf78();

FUNC_EXTERN 	VOID	commd();
FUNC_EXTERN 	VOID	setto();
FUNC_EXTERN 	VOID	putcm();
FUNC_EXTERN 	VOID	getcm();
FUNC_EXTERN 	int 	iopnd();
FUNC_EXTERN 	VOID	comxon();

/*
**  Forward functions 
*/
static	VOID	sndsasm(void);
static	VOID	sndsasma(void);
static	VOID	addsw(char *, char *, long, char *);
static	bool	chksw(char *, char *, long *, char *);
static	int 	upsasm(char *);
static	int 	upsasma(char *);
static 	VOID	upsw(char *);
static	VOID	buf_free(void);
static	unsigned long	ctol(char *);
static	VOID	ltoc(unsigned long, char *);
static	int 	ltos(char *, unsigned long);
static	bool	isnum(char);
static  VOID    snddisc(void);
static	VOID	sndack(void);
static  int 	addack(char *);
static	int 	getrblk(char *);
static	VOID	add_sblk(char *, int);
static	VOID	snduiblk(char *, int);
static	bool	sndnxdblk(void);
static	bool	blk2snd(void);
static	int 	udswdw(void);
static	VOID	uprak(char *);
static	VOID	erlog(int);
static	int 	pgpby(char);
static	VOID	xmtblk(char *, int, int);
static	long	blrand(void);
static  int     inc_seqn(int);
static  int     inc_wdwn(int);
VOID 		pgcnt(PG_CB *);
VOID 		pglgn(PG_CB *);
VOID 		pglgo( PG_CB * );
VOID 		pgsnd( PG_CB * );
VOID 		pgrd( PG_CB * );
VOID 		pguisnd( PG_CB * );
VOID 		pguird( PG_CB * );
int 		pgpnd( PG_CB * );
static VOID	sndsw( char * );
/* --- Rev 7/T Tunable Global Parameters --- */
static bool xonxoff = TRUE; 	    	/* enable software flow control */
static int timo = 120;	    	    	/* block reception timeout */
static int log_timo = 30;   	    	/* logon timeout */
static int snd_window = 16;    	    	/* send window size */
#ifdef OK2LOOP
static bool ok2loop = FALSE;	    	/* don't allow loopback */
#endif

/* --- Tunable protocol paramaters --- */
static int t1 = 10;		    	/* retransmit delay (in secs.) */
static int n3 = 4;  			/* frequency of ack requests during i-blk xfers */
static bool n7bit = TRUE;   		/* 7 bit mode */
static int pg_isz = 2048;    		/* size of data blks */

static bool pg7bit;	    	    	/* local copy of n7bit */
static int pgt1;	    	    	/* local copy of t1 */
static int t2; 	    			/* local connect timeout */
static int pgn3;    	    		/* local copy of n3 */
static int pg_delay = 0;		/* delay for uploads in hdx */
static int dblkisz;		    	/* no of information bytes in a block */

/* --- Data Buffers ---- */
static char *pgsbuf;	    	    	/* address of send buffer area */
static char *pgrbuf;	    	    	/* address of receive buffer area */
static char rtmpbuf[DBLK_BSZ];		/* scratch buffer for recv */
static char stmpbuf[DBLK_BSZ];		/* scratch buffer for send */
static char pgcbuf[128];	    	/* control block buffer */
static int loc_address;		    	/* address of local */
static int rem_address;		    	/* address of remote */
static long randno;		    	/* random number used in logon */
static bool pghdx;		    	/* remote is half-duplex */

/* --- send variables --- */
static char *sbufa[SWDWSZ]; 	    	/* send buffer addresses */
static int sbksiz[SWDWSZ]; 	    	/* block sizes */
static int sbksts[SWDWSZ]; 	    	/* block status */
static int sbkq[SWDWSZ];	    	/* queue of blks sent */
static int sbkqhd;		    	/* head of the queue */
static int sbkqtl;		    	/* tail of the queue */
static int nsbks = 0;		    	/* number of send blocks in sbuf */
static int swdwb = 0;		    	/* beginning of present send window */
static int swdwsz = SWDWSZ;	    	/* send window size */
static int sfbkn;		    	/* send block number */

/* --- send ack block variables --- */
static bool snakf;  			/* send an ack flag */
static int suiff;	    	    	/* send unnumbered information flip/flop */
static int suiffn;	    	    	/* new uiff from ack blk */

/* --- receive data variables --- */
static char *rbufa[SWDWSZ];	       	/* receive buffer addresses */
static int rbksiz[RWDWSZ];    	    	/* block sizes */
static int rwdwb;		       	/* receive window beginning */
static int rwdwsz = RWDWSZ;	    	/* working recv window size */
static int rfbkn;		     	/* last recv blk no used */
static int nrbks;	    		/* number of recv blks */


/* ---- unum info blk variables ---- */
static char *ruipr; 			/* ptr to UNUM_INFO buf */
static int ruicnt;	    	    	/* size of recv UI blk */
static int ruiff;	    	    	/* recvd UNUM_INFO flip/flop */
static bool ruidon; 	    	    	/* recvd unnumbered read */
 
/* ---- bit map variables ---- */
static unsigned long rbmap = 0xFFFF0000;	    	/* receive bitmap */
static unsigned long sbmap = 0xFFFF0000;	    	/* send bitmap */
static unsigned long bmap[32] = {
	0x80000000, 0x40000000, 0x20000000, 0x10000000,
	0x08000000, 0x04000000, 0x02000000, 0x01000000,
	0x00800000, 0x00400000, 0x00200000, 0x00100000,
	0x00080000, 0x00040000, 0x00020000, 0x00010000,
	0x00008000, 0x00004000, 0x00002000, 0x00001000,
	0x00000800, 0x00000400, 0x00000200, 0x00000100,
	0x00000080, 0x00000040, 0x00000020, 0x00000010,
	0x00000008, 0x00000004, 0x00000002, 0x00000001
};

/* ---- line quality monitoring variables ----- */
static int rerrn;   			/* receive error count */
static int rerro;	    	    	/* old value */
static int serrn;   			/* send error count */
static int serro;   			/* old value */
static int errps;    	    		/* error possibilities */
static int errct;   			/* error count */

/* ---- parameters from upper layer ---- */
static bool pglrq;  	 	    	/* logon request flag */
static bool pgsrq;  			/* send request flag */
static char *pgspr; 			/* send data to send */
static int pgsct;   	    	    	/* send data byte count */
static bool pgrrq;  			/* read request flag */
static char *pgrpr; 			/* read data buffer */
static bool pguisrq;	    	    	/* send unum_info req */
static char *pguispr;	    	    	/* send data buffer */
static int pguisct; 	    	    	/* send unum_info byte count */
static int pguirrq;	    		/* ui read ptr */
static bool sasmok;	    	    	/* ok to recv sasm's */
static bool pgdrq;	    	    	/* disconnect request */
static int pgdi;	    	    	/* disconnect interrupt */

/* ---- pgpnd() variables ---- */
static bool pglogmsg;	        	/* pg in logon message state */
static bool pglogon;	    		/* pg in logon state */
static bool sbksfull;	    		/* send blk buffers are full */
static bool pcmbsy;	    		/* comm port snd busy */
static bool gcmbsy;	    		/* comm port recv busy */
static int pgtimo;	    		/* timeout counter */
static int xofftimo;	    		/* xon/xoff timer */
static int akrqct;	    	    	/* ack request counter */
static int sbktimo;	    		/* send blk timeout */
static int wutimo;	    	    	/* wake up (hdx) timeout */
static int lbkwu;	    	    	/* last blk was a wake-up blk */
static int sdccnt;	    	    	/* send disc count */
static bool rakf;		    	/* ack recvd */
static bool ssasm;	    	    	/* snd a sasm (logon blk) */
static bool ssasma;	    	    	/* snd remote a sasma */
static bool pgdisc; 	    		/* disconnect in progress */
static char inbyt;  	    	    	/* next input byte */
static char last_inbyt;	    	    	/* previous input byte */
static char *lgnmsg = ";starting BLAST protocol.\r\n";

/* ---- pgpby() variables ---- */
static int vect;	    		/* pgpby vector */
static bool lerflg; 			/* log an error on a FLAG */
static bool lertal; 	  		/* log an error on a RETURN */
static bool ack_pres;    	    	/* ack info present in this blk */
static bool snakh;	    	    	/* hold snakf flag till end of blk */
static bool bk8bit;	    	    	/* blk is packed in 8-bit format */
static int address;	    	    	/* address of blk being recvd */
static char hibits;	    	    	/* hi-order bits from control blks */
static unsigned crc;	    		/* received crc */
static char *inptr;	    	    	/* input ptr and cnt */
static int incnt;  	     	    	/* input byte count */
static int max_incnt;	    	    	/* maximum count */
static int rbni;		    	/* save for blk index */
static int rbkn;		    	/* blk seq num being recvd */
static int blk_id;	    	    	/* control blk id */
static int ubyte;	    	    	/* unpacked byte */
static int dblkbsz = DBLK_BSZ;	    	/* size of datablock */


/* semaphores */

/* tracing */

GLOBALREF i4  GCblast_trace;

# define GCTRACE(n) if( GCblast_trace >= n )(void)TRdisplay


/*{
** Name: pgini()
**
** Description: 
**  	Initialize protocol generator
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	TRUE - initialization done
**  	FALSE - initialization failed
**
** Exceptions:
**	none
**
** History:
**	29-Jan-91 (cmorris)
**		Modified to use MEreqmem and MEfree.
**	09-Jan-92 (alan)
**		Changed atoi to CVan.
**	24-Aug-92 (alan)
**	 	Fixed check on send buffer allocation.
**	06-jun-95 (canor01)
**	 	semaphore protect memory allocation routines
*/
bool
pgini(void)
{
    int i;
    long li;
    char *p;
    char *trace;

    NMgtAt( "II_PGB_TRACE", &trace );

    if( trace && *trace )
        CVan( trace, &PGB_trace );

    /* use global timeouts */
    t2 = timo;
    pgt1 = t1;  

    /* Set up max bytes per packet */
    if( pg_isz > DBLK_IMAX )	        /* no bigger than max */
	dblkisz = DBLK_IMAX;
    else
	dblkisz = pg_isz;	    	/* else use fig setting */

    /* 7 bit transfers? */
    pg7bit = n7bit; 			/* use fig setting */

    /* xon/xoff timer */
    xofftimo = 0;

    /* Set comm port operating mode */
    commd( 0, FALSE );

    /* validate the send window size */
    if( snd_window > 0 )
    {
	if( snd_window > SWDWSZ ) snd_window = SWDWSZ;
	swdwsz = snd_window;
    }
    else 
    	swdwsz = SWDWSZ;

    pgn3 = n3;      	    	        /* Ack frequency */
    rwdwsz = RWDWSZ;	    	    	/* Receive window size */

    /* init the addresses */
    loc_address = DTE_ADD;		/* start by assuming we're DTE */
    rem_address = DCE_ADD;
    randno = blrand();

    pghdx = FALSE;

    /* figure the size needed for buffers */
    dblkbsz = 2 + dblkisz + 5 + 2;


    /* Allocate space for send and receive buffers */
    pgrbuf = (char *) MEreqmem( 0, dblkbsz*RWDWSZ, FALSE, (STATUS *) 0);
    if (pgrbuf == (char *) NULL)
    	/* Allocation fails */
    	return FALSE;

    pgsbuf = (char *) MEreqmem(0, dblkbsz*SWDWSZ, FALSE, (STATUS *) 0 );
    if (pgsbuf == (char *) NULL)
    {
    	/* Allocation fails - clean up */
    	buf_free();
    	return FALSE;
    }

    /* init sbuf blk addresses and send blk q */
    p = pgsbuf;
    for( i=0; i<SWDWSZ; i++ ) 
    {
	sbufa[i] = p;	    	    	/* block address */
	p += dblkbsz;	    	    	/* step buffer pointer */
	sbkq[i] = -1;	    	    	/* queue empty */
	sbksts[i] = STS_UNKNOWN;    	
    }

    sbkqhd = sbkqtl = 0;	    	/* Initialize queue head/tail */

    /* init rbuf blk addresses and sizes */
    p = pgrbuf;
    for( i=0; i<RWDWSZ; i++ ) 
    {
	rbufa[i] = p;	    	    	/* block address */
	p += dblkbsz;	    	    	/* step buffer pointer */
	rbksiz[i] = 0;	    	    	/* block is empty */
    }

    /* start with packet sequence number 0 */
    sfbkn = 0;
    rfbkn = 0;

    /* init the foreground variables */
    pgsrq = FALSE;
    pgrrq = FALSE;
    pguisrq = FALSE;
    sasmok = FALSE;
    pgdrq = FALSE;
    pgdi = 0;

    pglogmsg = FALSE;	    	    	
    pglogon = FALSE;	    	    	/* show not logged */

    /* init some stuff for pgpend */
    pcmbsy = FALSE;
    gcmbsy = FALSE;
    akrqct = sbktimo = wutimo = sdccnt = 0;
    rakf = FALSE;
    ssasm = FALSE;
    ssasma = FALSE;
    pgdisc = FALSE;

    /* init some stuff for pgpby */
    vect = 0;
    lerflg = FALSE;
    lertal = FALSE;
    ack_pres = FALSE;
    snakh = FALSE;
    bk8bit = FALSE;

    /* reset the ack ff */
    suiff = suiffn = 0;
    snakf = FALSE;
    ruiff = 0;
    ruidon = FALSE;

    /* reset error counts */
    serrn = 31;
    serro = 31;
    rerro = 31;
    rerrn = 31;
    errps = 0;
    errct = 0;

    sbksfull = FALSE;
    nsbks = 0;
    swdwb = 0;
    ruipr = rtmpbuf;		    	/* init ptr for ui data */

    nrbks = 0;			    	/* no blks in buffer */
    rwdwb = 0;	    			/* start at next blk number */

    return( TRUE );
}

/*{
** Name: pgcnt
**
** Description: 
**	Connect to peer
**
** Inputs:
**      pgcb			- Pointer a PG_CB structure
**	pgcb->buf		- Connection string
**
** Outputs:
**  	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	16-Jul-91 (cmorris)
**	    Created shell
*/
VOID
pgcnt(pgcb)	
PG_CB	*pgcb;
{		
}

/*{
** Name: pglgn
**
** Description: 
**	Send Logon message to peer
**
** Inputs:
**      pgcb			- Pointer a PG_CB structure
**
** Outputs:
**  	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	30-Jan-91 (cmorris)
**		Modified to just record fact that logon is to be
**  	    	attempted. Rest of function has been gutted and logic
**  	    	incorporated into pgpnd.
*/
VOID
pglgn(pgcb)	
PG_CB	*pgcb;
{		
    /* set total time we'll try logon for */
    pgtimo = log_timo;
    pglrq = TRUE;

    /* set timer */
    setto(400);
}

/*{
** Name: pglgo( )
**
** Description: 
**	 break protocol with remote.
**
** Inputs:
**      pgcb			- Pointer to a PGCB structure
**	pgcb->intr  	        - if FALSE, allow any pending data to drain;
**				  if TRUE, interrupt protocol session, and
**				  disconnect.
**
** Outputs:
**	none
**
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	09-May-90 (cmorris)
**		Modified BLAST i/face to support control block passing
**		and callback. Rest of function has been gutted and logic
**  	    	incorporated into pgpnd.
*/
VOID
pglgo( pgcb )
PG_CB *pgcb;
{
    int val, i, loop;

    pgdrq = TRUE;
    t2 = pgtimo = 10;
    if( pgcb->intr )
    	pgdi = DISCI;
    else
    	pgdi = 0;
}

/*{
** Name: pgsnd()
**
** Description: 
**	Initiate a write request.
**
** Inputs:
**      pgcb			- Pointer a PGCB structure
**	pgcb->buf		- outgoing data buffer
**	pgcb->len		- length of pgcb->buf
**
** Outputs:
**  	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	01-May-90 (cmorris)
**		Modified BLAST i/face to support control block passing.
*/
VOID
pgsnd( pgcb )
PG_CB	*pgcb;
{
    pgsrq = TRUE;		    	/* set request flag */
    pgspr = pgcb->buf;	        	/* copy ptr to pg variable */
    pgsct = pgcb->len;		    	/* also count */
}

/*{
** Name: pgrd()
**
** Description: 
**	Initiate a read request.
**
** Inputs:
**      pgcb			- Pointer a PGCB structure
**	pgcb->buf		- input data buffer pointer
**	pgcb->len		- length of pgcb->recv.buf
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
**		Modified BLAST i/face to support control block passing.
*/
VOID
pgrd( pgcb )
PG_CB	*pgcb;
{
    pgrrq = TRUE;		    	/* set request flag */
    pgrpr = pgcb->buf;		    	/* copy pointer to pg variable */
}

/*{
** Name: pguisnd()
**
** Description: 
**	Initiate an unnumbered information write request.
**
** Inputs:
**      pgcb			- Pointer a PGCB structure
**	pgcb->buf	- outgoing data buffer
**	pgcb->len	- length of pgcb->buf
**
** Outputs:
**  	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	01-May-90 (cmorris)
**		Modified BLAST i/face to support control block passing.
*/
VOID
pguisnd( pgcb )
PG_CB	*pgcb;
{
    pguisrq = TRUE;		    	/* set request flag */
    pguispr = pgcb->buf;    		/* save address and count */
    pguisct = pgcb->len;

    suiff = ~suiff & UI_FF;		/* toggle send ui flip/flop */
}

/*{
** Name: pguird()
**
** Description: 
**	Initiate an unnumbered information read request.
**
** Inputs:
**      pgcb			- Pointer a PGCB structure
**	pgcb->buf	- input data buffer pointer
**	pgcb->len	- length of pgcb->recv.buf
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
**		Modified BLAST i/face to support control block passing.
*/
VOID
pguird( pgcb )
PG_CB *pgcb;
{
    ruipr = pgcb->buf;	    	/* save ptr for data */
}

/*{
** Name: pgpnd()
**
** Description: 
**	Pend for pg event
**
** Inputs:
**      pgcb			- Pointer a PGCB structure
**
** Outputs:
**	depends on event occuring
**
**  	If event is PG_BLOCKED, PG_SND, PG_UISND, PG_TIMO,
**  	    	    PG_DISC:
**          none
**
**      If event is PG_LOGON:
**  	    pgcb.len	    	- blocksize for connection
**
**      If event is PG_RD, PG_UIRD:
**  	    pgcb.len            - length of data read
**
**  	If event is PG_ERROR:
**  	    pgcb.generic_status - generic error status
**  	    pgcb.syserr         - os specific error status
**      
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	29-Jan-91 (cmorris)
**		Modified BLAST i/face to support control block passing.
**  	25-Feb-91 (cmorris)
**  	    	PG_LOGON now returns block size to be used on connection.
**	09-Jul-91 (cmorris)
**		Set up pointer in siocb to syserr before calling iopnd.
**  	21-Jul-92 (cmorris)
**  	    	If a disconnect has been issued, purge any received data
**  	    	as opposed to driving receive completion.
**  	22-Jul-92 (cmorris)
**  	    	Upon receipt of a disconnect (interrupt), reset send block
**  	    	timeout as no acks occur after that point.
*/
int
pgpnd( pgcb )
PG_CB *pgcb;
{
    register int i;

    /* process send data */
    if( pgsrq && ( nsbks < swdwsz )) 
    {
	pgsrq = FALSE;
	add_sblk( pgspr, pgsct );	/* add this send blk */
	sbktimo = 0;			/* new blk to send */
	if( ++nsbks < swdwsz )  	/* if room for more blks */
    	    return(PG_SND);
	else
    	    sbksfull = TRUE;	    	/* else all send blks are full */
    }

    if(pglrq)	    	    	    	/* logon request? */
    {
    	pglrq = FALSE;

    	/* Send out logon message */
        PGBTRACE(2)("pgpnd: sending logon message\n");
    	putcm(lgnmsg, (int) STlength(lgnmsg));
    	pcmbsy = TRUE;
    	pglogmsg = TRUE;    	    	/* in login message state */
    }

    if( !gcmbsy ) 
    {
	getcm( &inbyt );    	    	/* post read to sio */
	gcmbsy = TRUE;
    }

    while( 1 ) 	    			/* --- protocol processing loop --- */
    {
    	if( pgdrq )
    	{
	    /* is it an interrupt? */
	    if( pgdi )
    	    {
	        /* forget about any send blks */
		nrbks = nsbks = 0;
		pgsrq = pgrrq = pguisrq = FALSE;
		ruipr = 0;
		rakf = snakf = FALSE;
	    }

            /* if a disc req and nothing to send */
	    if( pgdrq && !nsbks && !pguisrq ) 
    	    {
		pgdrq = FALSE;	    	/* clear req flag */
		pgdisc = TRUE;
    	    }
	}


	/* --- if a new ack has been received process the results --- */
	if( rakf )
    	{
	    /* --- chk to see if UI snd complete --- */
	    if(( pguisrq )&&( suiffn == suiff ))
    	    {
	    	sbktimo = 0;	    	/* clear t1 timeout delay */
		pguisrq = FALSE;    	/* clear request */
    	    	return (PG_UISND);
	    }

	    rakf = FALSE;   		/* ok to clr flag, chk both results */

	    /* --- chk to see if send window advanced --- */
	    if( i = udswdw() )
    	    {
		if( blk2snd() || !nsbks )
    	    	    /* if there is a blk to send */
		    sbktimo = 0;  	/* clear t1 timeout delay */
		i &= ~0200;	    	/* clear flag bit if set */
		if( i ) 	    	/* if blks were freed */
		{
		    if( sbksfull )      /* send pending? */
    	    	    {
		       	sbksfull = FALSE;
    	    	    	return (PG_SND);
		    }
		}
	    }
	}

	if( ruidon )	    	    	    /* unnunbered read */
    	{
	    ruidon = FALSE;
	    if( ruipr )
    	    {
		ruipr = 0;
		pgcb->len = ruicnt;  /* show size */
    	    	return (PG_UIRD);
    	    }
	}

	if( pgrrq && nrbks )		    /* any recv data blks? */
    	{
	    pgcb->len = getrblk( pgrpr );
    	        	    	    	    /* unpack recv blk -- show size */
	    pgrrq = FALSE;  	    	    /* clear request */
	    --nrbks;		    	    /* one less blk down here */
    	    return (PG_RD);
	}

	if( !pcmbsy && !sbktimo && !wutimo )
    	{
	    if( pgdisc &&( pgdi || !snakf ))
    	    {
		if( sdccnt && !--sdccnt )
		    return( PG_DISC );
		i = 1;
		if( pghdx ) i = 3;
		sbktimo = pgt1 = i;
                PGBTRACE(2)("pgpnd: sending disconnect\n");
		snddisc();
	    }
	    else 
    	    	if( ssasma )   	    	/* respond to logon requests */
    	    	{	
		    ssasma = FALSE; 	/* we`re sending it... */
                    PGBTRACE(2)("pgpnd: sending sasma\n");
		    sndsasma();	    	/* send sasm acknowledgement */
	    	}
		else
    	    	    if( ssasm )         /* need to send logon blk? */
    	    	    {	
                        PGBTRACE(2)("pgpnd: sending sasm\n");
		    	sndsasm();  	/* send it */
			sbktimo = 4;	/* send it again if no response... */
		    }
    	    	    else
		        if( pguisrq )   /* do we need to send a ui blk? */
    	    	    	{
			    sbktimo = pgt1;
    	    	    	    	    	/* set timeout */
			    snduiblk( pguispr, pguisct );
    	    	    	    	    	/* send it */
			}
    	    	    	else
                            if( nsbks ) 
    			               /* do we need to send data blks? */
    	    	    	    {
				if( sndnxdblk( ))
				    sbktimo = pgt1;
    	    	    	    	       /* last one flagged SEND? */
			    }
	}

	if( !pcmbsy && !wutimo )
    	{
	    if( snakf && !pgdi )
    	    {
		snakf = FALSE;
		sndack();
	    }
	}

	/* 
    	** if in half-duplex and nothing to send ...
	** do i need to send a wake-up blk? 
    	*/
	if( !pcmbsy && pghdx && !sbktimo && !wutimo && !lbkwu )
    	{
	    sndack();		    	/* send a new ack blk */
	    wutimo = pgt1;
	    lbkwu = pgt1;   	    	/* reset wakeup timeout */
	}

    	/* Enter serial i/o driver */
	pgcb->siocb.syserr = &pgcb->syserr;
	switch( iopnd(&pgcb->siocb) ) 
    	{
    	case SIO_BLOCKED:

    	    /* We are blocked for i/o completion */
    	    return( PG_BLOCKED );

	case SIO_TIMO:

    	    /* See if we are sending logon message */
    	    if (pglogmsg)
	    {
    	    	pgtimo -= 4;
    	    	if(pgtimo <= 0)
    	    	{

    	    	    /* we ran out of time.... */
	    	    pgcb->status = GC_LISTEN_FAIL;
    	    	    return ( PG_TIMO );
    	    	}    	    

    	    	/* see if time to retry sending logon message */
    	    	if (!pcmbsy)
    	    	{
    	            /* Send out logon message */
                    PGBTRACE(2)("pgpnd: sending logon message\n");
    	    	    putcm(lgnmsg, (int) STlength(lgnmsg));
    	    	    pcmbsy = TRUE;
    	    	}

        	/* set timer running again */
    	    	setto(400); 	    	/* four second timeout */
	    }
    	    else
    	    {
       	    	if (pgtimo)
    	  	    --pgtimo;	    	/* decrement timer */
		setto( 100 );		/* restart timeout */
    	    	if (pgtimo == 0)
		{
    	    	    /* timed out */
    	    	    pgtimo = t2;
    	    	    pgcb->status = GC_NTWK_STATUS; /* hack... */
		    return( PG_TIMO );
		}
			
    	    	if( ++xofftimo == 30 )  /* 30 seconds idle? */
		{
		    xofftimo = 0;
		    comxon();	/* kick it back on */
    	    	}

		if( sbktimo ) --sbktimo; /* decrement pgt1 counters */
		if( wutimo ) --wutimo;
		if( lbkwu ) --lbkwu;
	    }

	    break;

	case SIO_COMWR:
	    pcmbsy = FALSE;
	    break;

	case SIO_COMRD:
    	    /* See if we're still waiting for a logon flag */
    	    if (pglogmsg)
	    {
		/* is this the flag? */
    	    	if ((inbyt & 0177) == FLAG)
    	    	{
    	    	    /* yes, it is! */
    	    	    pglogmsg = FALSE;
    	    	    ssasm = TRUE;   	/* ok to send sasm's */
    	    	    sasmok = TRUE;  	/* ok to get sasm's */
    	    	    pgtimo = log_timo;  /* use logon timeout */
    	    	    commd(-1, xonxoff);	/* line reads, xon-xoff ok */
		    setto( 100 );	/* reset timeout period */
		}

    	    	getcm(&inbyt);	    	/* post read to sio */
	    }
    	    else    	    	    	/* normal processing */
	    {
		i = pgpby( inbyt );
		getcm( &inbyt );    	/* post read to sio */
		if( i )     	    	/* anything to process? */
		{
		    /* reset timeout on good blk reception */
		    if( pglogon )
    	    	    	pgtimo = t2;
		    xofftimo = 0;

		    /* process control blks */
		    if( i & 0200 )
    	    	    {
		        if( pghdx )
			    sbktimo = wutimo = pg_delay;

			switch( i &= BLK_ID )
    	    	    	{
			case SASM:
			    if( !sasmok )
    	    	    	    {
    	    	    	    	pgcb->status = GC_CONNECT_FAIL;
    	    	    	    	return( PG_ERROR );
    	    	    	    }
			case SASMA:
			    break;
			default:
			    sasmok = FALSE;
    	    	    	    		/* no more sasm */
			}

    	    	    	switch( i )
    	    	    	{
			case SASM:

    	    	    	    /* sasm received */
			    ssasma = TRUE;
    	    	    	    	    	/* mark to send ack for ssasm */
			    sbktimo = 0;
    	    	    	    	    	/* clear timeout */
			    break;
			case SASMA:
			    ssasm = FALSE;
    	    	    	    	    	/* don't send another sasm */
			    sbktimo = 0;
			    if( !pglogon )
			    {
    	    	    	    	pglogon = TRUE;
    	    	    	    	pgtimo = t2;

    	    	    	    	/* return connection block size */
    	    	    	    	pgcb->len = dblkisz;
    	    	    	    	return( PG_LOGON );
			    }

			    break;
			case DISCI:	/* interrupt disc */
			    pgdi = DISCI;
			    if( !pgdisc )
			    {
			      	pgdrq = TRUE;
    	    	    	    	sbktimo = 0;
			    }
			    /* no brakes! */
			case DISC:
                            PGBTRACE(2)("pgpnd: incoming disconnect\n");
			    if( !sdccnt )
    	    	    	    {
				sdccnt = 3;
    	    	    	    	    	/* send 3 more */
				t2 = pgtimo = 100;
    	    	    	    		/* but don't wait forever */
			    }
			    break;
			case ACK:
			    break;	/* uprak sets rakf */		
			case UNUM_INFO:
			    ruidon = TRUE;
			    break;
			}
		    }
    	    	    else
		    {
    	    	    	/* must be data blks recvd */
			nrbks = i & ~0100;
    	    	    	PGBTRACE(4)("pgpnd: %d block(s)\n", nrbks);
    	    		if( pgdi ) 
    	    	    	{ 
	            	    /* forget about any received blks */
                            PGBTRACE(2)("pgpnd: purging %d block(s)\n", 
    	    	    	    	    	nrbks); 
		    	    nrbks = 0; 
			} 

			if( pghdx )
			    if( snakf )	/* if just data blks */
				sbktimo = wutimo = pg_delay;
			    else
			        wutimo = pgt1;
		    }
		}
	    } 	    	    	    	/* normal processing */
	    break;

    	case SIO_ERROR:
    	    
    	    /* copy in status returned by sio */
    	    pgcb->status = pgcb->siocb.status;

    	    /* fall thru here... */

	default:
	    return(PG_ERROR);

    	} /* switch iopnd() */

    }  /* while ( 1 ) */

} /* end of pgpnd */

static char *sw7bit = "/7";
static char *hdxsw =  "/H";
static char *rndsw =  "/R=%d";
static char *bszsw =  "/B=%d";
static char *wszsw =  "/W=%d";

#define CNIL 	(char *) 0
#define LNIL	(long *) 0
#define LNULL	(long) 0

/*{
** Name: sndsasm
**
** Description: 
**	Send a sasm
**
** Inputs:
**      none
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	04-Feb-91 (cmorris)
**		Made function return VOID
*/
static VOID
sndsasm(void )
{
    register char *pt = pgcbuf;

    *pt++ = loc_address;		/* the address field */
    *pt++ = SASM;			/* add the id */
    *pt = 0;				/* string termination */
    addsw( pt, rndsw, randno, CNIL );   /* add common switches */
    sndsw( pt );    	    	    	/* add other switches */
    xmtblk( pgcbuf, 2+(int)STlength( pt ), FALSE );
    	    	    	    		/* send it out */
}

/*{
** Name: sndsasma
**
** Description: 
**	Send a sasma
**
** Inputs:
**      none
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	04-Feb-91 (cmorris)
**		Made function return VOID
*/
static VOID
sndsasma(void )
{
    register char *pt = pgcbuf;

    *pt++ = rem_address;		/* the address field */
    *pt++ = SASMA;			/* add the id */
    *pt = 0;	    	    	    	/* string termination */
    sndsw( pt );    	    	    	/* add common switches */
    xmtblk( pgcbuf, 2+(int) STlength( pt ), FALSE );
    	    	    	    		/* send it out */
}

/*{
** Name: sndsw
**
** Description: 
**	Add switches common to both sasm and sasma
**
** Inputs:
**      pt - 	    address to add switches at
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	04-Feb-91 (cmorris)
**		Made function return VOID
*/
static VOID
sndsw( pt )
char *pt;
{
    if( pg7bit )
	addsw( pt, sw7bit, LNULL, CNIL );
    	    	    	    	    	/* 7 bit switch */
    if( pghdx )
	addsw( pt, hdxsw, LNULL, CNIL );
    	    	    	    	    	/* half-duplex switch */
    addsw( pt, bszsw, (long) dblkisz, CNIL );
    	    	    	    	    	/* Block size switch */
    addsw( pt, wszsw, (long) swdwsz, CNIL );
    	    	    	    	    	/* Send window size switch */
}

/*{
** Name: addsw
**
** Description: 
**	Add switch to sasm or sasma.
**	if "sw" contains a "%d" then "lnum" will be parsed
**	into the line in ascii at that position.
**	if "sw" contains a "%s" then "str" will be copied 
**	into the line at that position.
**
** Inputs:
**      fn - 	    null terminated "filename"
**  	sw -	    switch to append
**  	lnum -	    number value (used with %d)
**  	str -	    string value (used with %s)
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Rehashed and made function return VOID
*/
static VOID
addsw( fn, sw, lnum, str )
char *fn;
char *sw;
long lnum;
char *str;
{

    /* -- find the end of the swing -- */
    fn += (int) STlength( fn );

    /* -- add the switch -- */
    while( *sw )
    {
	if( *sw == '%' )		/* percent found */
    	{
    	    ++sw;
	    switch( *sw )
    	    {
	    case 'd':		        /* number called for? */
	    	fn += ltos( fn, lnum );	/* convert to ascii */
		    break;
	    case 's':
		while( *str ) *fn++ = *str++;
    	    	    	    	    	/* just copy string */
		    break;
	    }
	}
	else
	    *fn++ = *sw;    	    	/* just copy character */

	++sw;	    	    	    	/* move on to next byte */
    }
    *fn = 0;	    			/* null terminate the filename */
}

/*{
** Name: chksw
**
** Description: 
**      check for a switch
**
**	if a "%d" is found in "sw", the number represented
**	by the ascii digits in the line at that position will
**	be returned on lnum.
**
**	if a "%s" is found in "sw", the string at the position
**	will be returned to "str".
**
** Inputs:
**      ptr -	    null terminated string to parse
**  	sw -	    switch to look for
**
** Outputs:
**  	lnum -	    number value (used with %d)
**  	str -	    string value (used with %s)
**	
** Returns:
**	TRUE - 	    if switch found
**  	FALSE -	    if switch not found
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Rehashed and made function return bool
**	22-Jan-92 (alan)
**		Use CHtoupper, not toupper.
**  	22-Jul-92 (cmorris)
**  	    	CHtoupper is deprecated, CMtoupper can potentially
**  	    	be compiled to do a two-byte comparison which will
**  	    	break Blast, so we're back to toupper.
**  	22-Jul-92 (cmorris)
**  	    	Better still, why use any sort of toupper when there's
**  	    	no requirement to do so?
*/
static bool
chksw( ptr, sw, lnum, str )
char *ptr;	
char *sw;	
long *lnum;	
char *str;	
{
    int match;
    char *s, *p;
    char uc;

    while( *ptr ) 
    {
	if( *ptr == *sw ) 	        /* match first char */
    	{
	    p = ptr;
    	    s = sw;	    	    	/* match switch? */
	    match = TRUE;		/* assume match */
	    while( *s )
    	    {
		if( *s == '%' )         /* percent sign? */
    	    	{
		    ++s;
		    switch( *s )
    	    	    {
		    case 'D':
		    case 'd':   	/* decimal int */
			*lnum = 0;
			while( isnum( *p ) )
			    *lnum = ( *lnum * 10 ) + *p++ - '0';
			break;
		    case 'S':
		    case 's':	    	/* string */
		      	while( *p &&( *p != '/' ))
			    *str++ = *p++;
			*str = 0;
			break;
		    }

		    break;
		}
    	    	else 
    	    	    if(*s != *p++) 
    	    	    {
			match = FALSE;
			break;
		    }
	    	++s;
	    }

	    if( match )
    	    {   	    			/* match found */
	    	while( *p )
		    *ptr++ = *p++;	    	/* copy rest of source */
    	    	*ptr = 0;
	    	return( TRUE );
	    }
	}

	++ptr; 
    }

    return( FALSE );	    	    		/* no match found */
}

/*{
** Name: upsasm
**
** Description: 
**      unpack incoming sasm
**
** Inputs:
**      p  - 	    address of sasm
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Rehashed and made function return int
*/
static int
upsasm( p )
char *p;
{
    long li;
#ifdef OK2LOOP
    if( ok2loop )
    	 return( 0200 | SASM );
#endif
    if( chksw( p, rndsw, &li, CNIL ))
    {
	if( randno == li )	    	/* if equal, ignore */
    	    return( 0 );

	/* if my random num > his, switch */
	if( randno > li ) 
    	{	
	    loc_address = DCE_ADD;
	    rem_address = DTE_ADD;
	}
    }
    
    upsw( p );	    	    	    	/* unpack switches */
    return( 0200 | SASM );
}

/*{
** Name: upsasma
**
** Description: 
**      unpack incoming sasma
**
** Inputs:
**      p  - 	    address of sasma
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Rehashed and made function return int
*/
static int
upsasma( p )
char *p;
{
    upsw( p );
    return( 0200 | SASMA );
}

/*{
** Name: upsw
**
** Description: 
**      unpack switches common to both sasm and sasma
**
** Inputs:
**      p  - 	    address of sasm or sasma
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Rehashed and made function return VOID
*/
static VOID
upsw( p )
char *p;
{
    long li;

    if( chksw( p, sw7bit, LNIL, CNIL )) /* 7 bit operation? */
	pg7bit = TRUE;

    if( chksw( p, hdxsw, LNIL, CNIL ))  /* half duplex */
    {
	pghdx = TRUE;
	pgt1 = HDX_T1;	    	/* retransmit delay */
	pgn3 = 2;		/* never used */
    }

    if( chksw( p, bszsw, &li, CNIL )) 
    {
        PGBTRACE(3)("upsw: incoming block size %d\n", li);
	if( li < dblkisz ) 
    	    dblkisz = li;       /* use peer's block size */
    }
    else
    {
        PGBTRACE(3)("upsw: no incoming block size\n");
    	dblkisz = 84; 	    	/* not specified; use default */
    }

    if( chksw( p, wszsw, &li, CNIL )) 
    {
	if( li < swdwsz )
    	    swdwsz = li;    	/* use peer's window size */
    } 
}

/*{
** Name: buf_free()
**
** Description: 
**	Free allocated memory.
**
** Inputs:
**      none.
**
** Outputs:
**  	none
**	
** Returns:
**	none
**
** Exceptions:
**	none
**
** History:
**	30-Jan-91 (cmorris)
**		Modified to use MEfree.
*/
static VOID
buf_free( )
{

    /* Free send buffers */
    if( pgsbuf )
    {
    	MEfree((PTR) pgsbuf);
	pgsbuf = (char *) 0;
    }

    /* Free receive buffers */
    if( pgrbuf )
    {
    	MEfree((PTR) pgrbuf);
	pgrbuf = (char *) 0;
    }
}

/*{
** Name: ctol
**
** Description: 
**	Convert a string of chars to a long int
**
** Inputs:
**      p - 	address of string
**
** Outputs:
**  	none
**	
** Returns:
**	long int representation of input string
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Moved into pgb.c
*/
static unsigned long ctol( p )
char *p;
{
    unsigned long li;
    li  = (long)(*p++ & 0377) << 24;
    li |= (long)(*p++ & 0377) << 16;
    li |= (long)(*p++ & 0377) << 8;
    li |= (long)(*p++ & 0377);
    return( li );
}

/*{
** Name: ltoc
**
** Description: 
**	Convert a long  int into a string of characters
**
** Inputs:
**  	li -	long int to convert
**      p - 	address of string
**
** Outputs:
**  	p - 	string representing long int
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Moved into pgb.c
*/
static VOID
ltoc( li,p )
unsigned long li;
char *p;
{
    *p++ = (li >> 24) & 0377;
    *p++ = (li >> 16) & 0377;
    *p++ = (li >>  8) & 0377;
    *p++ = li & 0377;
}

/*{
** Name: ltos
**
** Description: 
**	Convert a long int into an ascii string
**
** Inputs:
**      lnum - 	long int to be converted
**
** Outputs:
**  	ptr - 	address of buffer to place string
**	
** Returns:
**	length of string
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Moved into pgb.c
*/
static
ltos( ptr, lnum )
char *ptr;
unsigned long lnum;
{
    int i;
    static char num[11]; 
    char *p;

    i = 10;		    		/* max size for 32-bit number */
    p = num + 9;		    	/* lowest digit in string */
    num[10] = 0;

    while( i-- > 0 ) 	    		/* loop thru digits */
    {
	*p-- = ( lnum % 10 ) + '0';
	lnum /= 10;
    }

    /* now supress leading zeros and spaces */
    p = num;
    while((*p == '0')&&(*(p+1) != '\0')) 
	++p;

    /* then copy to destination */
    i = 0;
    while( *p )
    {
	*ptr++ = *p++;
	++i;
    }
    
    *ptr = 0;	    			/* null terminate the output */
    return( i );
}

/*{
** Name: isnum
**
** Description: 
**	Check character is a digit
**
** Inputs:
**  	ch - 	    character to check
**
** Outputs:
**  	none
**	
** Returns:
**	TRUE - 	    character is a digit
**  	FALSE -     character is not a digit
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Moved into pgb.c; returns bool
*/
static bool
isnum( ch )
char ch;
{
	return(( ch >= '0' )&&( ch <= '9' ));
}

/*{
** Name: snddisc
**
** Description: 
**	send a disconnect
**
** Inputs:
**      none
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	31-Jan-91 (cmorris)
**		Made function VOID
*/
static VOID
snddisc(void )
{
    register char *pt = pgcbuf;

    *pt++ = loc_address;		/* address the block */
    *pt = DISC | pgdi;		    	/* identify it */
    xmtblk( pgcbuf, CBLK_SZ, FALSE );	/* and send it */
}

/*{
** Name: sndack
**
** Description: 
**	send a new ack block
**
** Inputs:
**      none
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	31-Jan-91 (cmorris)
**		Made function VOID
*/
static VOID
sndack(void)
{
    register char *pt = pgcbuf;

    PGBTRACE(2)("sndack: sending ack block\n");
    *pt++ = rem_address;		/* destination address */
    *pt++ = ACK;			/* block id */

    /* add the ack information and send it */
    xmtblk( pgcbuf, 2+addack( pt ), FALSE );

} /* end of sndack */

/*{
** Name: addack
**
** Description: 
**	add a new ack block
**
** Inputs:
**      pt  	    	- address to add ack block
**
** Outputs:
**	none
**	
** Returns:
**	number of bytes added
**
** Exceptions:
**	none
**
** History:
**	31-Jan-91 (cmorris)
**		Made function type static int
*/
static int 
addack( pt )
char *pt;
{
    *pt = rerrn & LINE_QUAL;	/* line quality info */
    if( ruiff ) 
    	*pt |= UI_FF;		/* and UI flip/flop bit  */
    ++pt;
    ltoc( rbmap, pt );
    return( 5 );    	    	/* ... length of ack */
} /* end of addack */

/*{
** Name: getrblk
**
** Description: 
**	get the next receive block
**
** Inputs:
**      ptr 	    	- Pointer to buffer
**
** Outputs:
**	none
**	
** Returns:
**	i   	    	- Size of receive block
**
** Exceptions:
**	none
**
** History:
**	29-Jan-91 (cmorris)
**		Made function VOID; use MEcopy.
*/
static int
getrblk( ptr )
char *ptr;
{
    register int i;

    MEcopy( rbufa[ rfbkn ], rbksiz[ rfbkn ], ptr );
    	    	    	    	    	/* copy into caller's buffer */
    i = rbksiz[ rfbkn ];		/* show how much */
    rbksiz[ rfbkn ] = 0;		/* blk now mt */
    rfbkn = inc_wdwn( rfbkn );	    	/* next blk to get */
    return( i );    	    	    	/* ...length of block */
}

/*{
** Name: add_sblk
**
** Description: 
**	add a send block buffer to send window
**
** Inputs:
**      ptr 	    	- Pointer to buffer
**  	cnt             - length of buffer in bytes
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	29-Jan-91 (cmorris)
**		Made function return VOID
*/

static VOID
add_sblk( ptr, cnt )
char *ptr;
int cnt;
{
    register int i;
    char *p;

    i = sfbkn & SWDWMK;		        /* use low order bits of blk seqn */
					/* to figure out which block to use */
    p = sbufa[i];			/* start address of blk */
	
    *p++ = loc_address;			/* first byte is address of blk */
    *p++ = NUM_INFO | sfbkn;		/* second is id byte */

    MEcopy((PTR) ptr, (u_i4) cnt, (PTR) p );
    	    	    		    	/* copy data into blk buf */
    cnt += 2;	    	    	    	/* increment by header size */
    sbksts[i] = STS_SEND;		/* mark status as ready to send */
    sbksiz[i] = cnt;			/* show size of blk */
    sfbkn = inc_seqn( sfbkn );	    	/* next sequence number */
}

/*{
** Name: snduiblk
**
** Description: 
**	start an unnumbered information xmit
**
** Inputs:
**      pf 	    	- Pointer to buffer
**  	cnt             - length of buffer in bytes
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	29-Jan-91 (cmorris)
**		Made function return VOID; use MEcopy.
*/
static VOID
snduiblk( pf, cnt )
char *pf;
int cnt;
{
    char *pt = stmpbuf;

    *pt++ = loc_address;		/* first byte is address */
    *pt++ = UNUM_INFO | suiff;	    	/* second is ID */
    MEcopy( pf, cnt, pt );	    	/* copy the info */
    cnt += 2;			    	/* adjust cnt to total */

    if( snakf )			    	/* need to add ack? */
    {
	snakf = FALSE;
	cnt += addack( stmpbuf+cnt );	/* add ack info */
	*(stmpbuf+1) |= ACK_PRES;	/* show ack present */
    }
    else
    	*(stmpbuf+1) &= ~ACK_PRES;  	/* show ack not present */

    xmtblk( stmpbuf, cnt, FALSE );	/* start the xmit */
}

/*{
** Name: sndnxdblk
**
** Description: 
**	start data block xmit
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	TRUE -	    delay required
**  	FALSE -	    no delay required
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Made function return bool
*/
static bool
sndnxdblk( void)		/* return true if n3 delay required after */
{
    static int last_sb;		        /* last blk sent */
    char *pf, *pfp1;
    register int i,j;
    int swdwe;
    int sb;
    bool delay;

    /*
    ** First, figure out if there is blk that needs sending
    ** if there's no blk to send in the first wdwsz -1 of the window,
    ** then send the last one with ack requested and show delay needed
    */
    i = swdwb;
    swdwe = i + swdwsz;

    sb = -1;			    	/* init sb to funny value */

    delay = TRUE;
    while( i < swdwe ) 		    	/* scan thru send window */
    {
    	j = i & SWDWMK;			/* wrap window around */
	if( sbksts[j] == STS_SEND ) 	/* if block needs sending */
	{
	    if( sb < 0 )
		sb = j;		    	/* save index */
	    else 
    	    {
		delay = FALSE;	    	/* another blk needs sending */
		break;			/* no delay, exit loop */
	    }
	 }
	 i++;
    }

    /*
    ** if no blk was found that needed sending,
    ** send the last one again
    */
    if( sb < 0 )			/* no send blk found */
       	sb = last_sb;			/* use the last one */

    /*
    ** if delay we're going to delay,
    ** then we might as well ask for an ack
    */
    if( delay )
	akrqct = 0;

    last_sb = sb;			/* save for next time */

    /*
    ** Ready to start building the block
    */
    pf = sbufa[sb];
    pfp1 = pf + 1;
    if( !akrqct )
    {
    	PGBTRACE(2)("sndnxtblk: requesting ack\n");
	*pfp1 |= ACK_REQ;		/* add ack req bit to header */
	akrqct = pgn3 - 1;  		/* reset counter */
    }
    else 
    {
	*pfp1 &= ~ACK_REQ;  		/* clear it if it was set */
	if( !pghdx )
    	    akrqct--;	    		/* decrement counter */
    }

    i = sbksiz[sb];			/* get blk size */

    if( snakf ) 			/* need to add ack? */
    {
    	PGBTRACE(2)("sndnxtblk: sending ack in num_info\n");
	snakf = FALSE;
	i += addack( pf+i );		/* add ack info */
	*pfp1 |= ACK_PRES;	    	/* show ack present */
    }
    else 
    	*pfp1 &= ~ACK_PRES;	    	/* else clr if it was set */

    sbksts[sb] = STS_UNKNOWN;		/* status update */

    xmtblk( pf, i, TRUE );	    	/* start the xmit */

    /*
    ** If this blk was not the last one on the blk sent queue,
    ** add it to the end
    */
    i = *pfp1 & SEQ_NUM;
    if( !sbkqhd ||( sbkq[ sbkqhd-1 ] != i )) 
    {
	if( sbkqhd < SWDWSZ )
    	{
	    sbkq[ sbkqhd ] = i;
	    sbkqhd++;
	}
	else
	{
	    for( j=0; j<SWDWSZ-1; j++ )
		sbkq[j] = sbkq[j+1];
	    sbkq[ SWDWSZ-1 ] = i;
	    --sbkqtl;	    	    	/* adjust tail also */
	}
    }
    return( delay );	    		/* return delay flag */
}

/*{
** Name: blk2snd
**
** Description: 
**	check to see whether there's a block to send
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	TRUE -	    there is a block to send
**  	FALSE -	    no block to send
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Made function return bool
*/
static bool
blk2snd(void)
{
    int i;

    for( i=0; i<SWDWSZ; i++ )
	if( sbksts[i] == STS_SEND )
    	    return( TRUE );
    return( FALSE );
}

/*{
** Name: udswdw
**
** Description: 
**	update the send window
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	number of free blocks
**
** Exceptions:
**	none
**
** History:
**	05-Feb-91 (cmorris)
**		Made function return int
*/
static int
udswdw(void)
{
    static bool sbkqak[SWDWSZ];
    int last_ack, rc;
    register int i,j,k;

    /*
    ** Scan thru the outstanding blks ...
    ** ..( between sbkqhd and sbktl in the sbkq ) ...
    ** and see if any we're acked.  At the same time,
    ** build the sbkqak array so we won't have to fool
    ** with bit maps in later analysis
    */

    last_ack = -1;
    i = sbkqtl;
    while( i < sbkqhd )
    {
	if(!( sbmap & bmap[ j=sbkq[i] ] ))
    	    	    	    	    	/* if the bit is set in sbmap */
	{
	    sbksts[j&SWDWMK] = STS_ACKED;
    	    	    	    		/* show it as ACKED */
	    sbkqak[i] = TRUE;
	    last_ack = i;		/* remember last blk acked */
	}
	else 
    	    sbkqak[i] = FALSE;
	i++;
    }

    /*
    ** Check for known bad blks, that is, blks who've been sent
    ** before an acked blk, but have not been acked themselves
    */

    rc = 0;
    if( last_ack >= 0 )  		/* if any were acked, */
    {
	while( sbkqtl <= last_ack ) 
    	{
	    if( !sbkqak[sbkqtl] 
		&&( sbksts[ j=(sbkq[sbkqtl]&SWDWMK) ] == STS_UNKNOWN ))
    	    {
		sbksts[j] = STS_SEND;
		rc = 0200;	        /* return bit flag for send */
	    }
	    sbkqtl++;
	}

	/* Now advance the real window pointer if possible */
	i = swdwb;
	j = i + SWDWSZ;
	while( i < j )
	{
	    k = i & SWDWMK;
	    if( sbksts[k] == STS_ACKED )
    	    	    	 	    	/* if blk acked */
	    {
		swdwb = inc_wdwn( swdwb );
    	    	    	    		/* bump sdwdb */
		if( !--nsbks )
    	            break;  		/* less blks in buff */
		i++;
	    }
	    else 
    	    	break;
	}
    }
    return((swdwsz-nsbks)|rc);	    	/* return number of free blks */
}

/*{
** Name: uprak
**
** Description: 
**	unpack received ack information 
**
** Inputs:
**  	pf -	    address of ack. information 
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
**	05-Feb-91 (cmorris)
**		Made function return VOID
*/
static VOID
uprak( pf )
char *pf;
{
    register int i;

    serrn = *pf & LINE_QUAL;	    	/* extract line quality */
    i = serrn - serro;
    if( i < 0 )
    	i = -i;
    if( i > 2 )		                /* if new differs from old by 15% */
    {
	serro = serrn;
        PGBTRACE(1)("uprak: pgb line quality: %d send errors\n", serrn);
    }

    suiffn = *pf++ & UI_FF;

    sbmap = ctol( pf );	    	    	/* get bit map */
    rakf = TRUE;
}

/*{
** Name: erlog
**
** Description: 
**	log an error
**
** Inputs:
**  	x - 	TRUE if ok to log error; FALSE
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
**	05-Feb-91 (cmorris)
**		Made function return VOID; got rid of errok nonsense
*/
static VOID
erlog( x )
int x;
{

    if( x )
    {
    	++errct;
    	PGBTRACE(3)("erlog: logging error\n");
    }

    if( ++errps == 20 )			/* time to record it ? */
    {
	if( rerrn != errct ) 		/* different from current ? */
	{
	    rerrn = errct;
	    x = rerrn - rerro;
	    if( x < 0 )	        	/* different by 15% or more? */
    	    	x = -x;	
	    if( x > 2 )
    	    {
		rerro = rerrn;	    	/* yes replace old */
    	        PGBTRACE(1)("pgb line quality: %d receive errors\n", rerrn);
	    }
	}
	
	errct = 0;		    	/* start fresh */
	errps = 0;
    }
} /* end of erlog( ) */

/*{
** Name: pgpby
**
** Description: 
**	process incoming protocol bytes
**
** Inputs:
**  	byte -	    byte to be processed
**
** Outputs:
**	none
**	
** Returns:
**	0 - nothing happened
**   	1-16 - the number of new recv blks that can be removed
**  	0200|x - x is the id of a control block recvd.
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	rejigged
**		
*/

#define IGNORE -1	/* blk id for unneeded data blks */

static int
pgpby( byte )
char byte;
{
    GLOBALREF unsigned good_crc;	/* defined in crc.c */
    unsigned crca();
    int i;

    i = byte & 0177;	    	    	/* if 7-bit channel, mask parity */

    /* is this the start of the block? */
    if(( i == FLAG )||( i == FLAG8 ))
    {
	if( lerflg ) 
	{
    	    PGBTRACE(1) ("pgpby: error, flag inside block\n");
	    erlog( TRUE );		/* log an err if unexpected */
	}
	else 
	    erlog( FALSE );		/* else log a non-err */
        PGBTRACE(4)("pgpby: start of block\n");
	lerflg = TRUE;			/* we are in a blk now */
	lertal = FALSE;
	crc = 0xffff;			/* init crc to all ones */
	if( i == FLAG8 )
    	{
	    bk8bit = TRUE;    	    	/* 8 bit characters */
	    upkf78( 0 );
	}
    	else	    	    	    
    	{
	    bk8bit = FALSE;        	/* 7 bit characters */
	    upkf34( 0 );		/* init unpack routine */
	}

	vect = 1;   			/* next time expect first byte of blk */
	return( FALSE );
    }

    if( i == TAIL ) 			/* end of a blk? */
    {
        PGBTRACE(4)("pgpby: end of block\n");
	if( lertal ) 			/* is TAIL expected? */
	{
    	    PGBTRACE(1) ("pgpby: error, unexpected end of block\n");
	    erlog( TRUE );	    	/* no */
	    vect = 0;	    	    	/* clear vector */
	    return( FALSE );
	}
	else
	    erlog( FALSE );
	lerflg = FALSE;			/* flag ok now */
	lertal = TRUE;	    		/* but no return */
	ubyte = -1;
    }
    else 				/* any old char */
    {
	if( !vect ) 
    	    return( FALSE );	    	/* not in a blk */
	if( bk8bit )	 		/* unpack this byte */
	    ubyte = upkf78( byte );
	else
	    ubyte = upkf34( byte );
	if( ubyte < 0 ) 
    	    return( FALSE ); 	    	/* not a real byte, exit */
	crc = crca( crc, ubyte & 0377 );
    	    	    	    	    	/* add this byte to crc */
    }

    switch ( vect ) 			/* branch on vector */
    {
    case 0:		    	    	/* ignore all bytes */
	return( FALSE );

    case 1:		    	    	/* first byte after flag */
    	/* check to see if the user is entering ;disc. */
	if( ubyte == 0x35 ) 
    	{
	    vect = 4;
	    break;
	}
	address = ubyte;    		/* save blk address */
	++vect;
	break;

    case 2:	    	    	    	/* second byte after flag */
	incnt = 0;
	inptr = 0;
	hibits = ubyte;			/* save ubyte */
	++vect;
	ack_pres = FALSE;

	/* prepare for numbered information block */
	if( ubyte & NUM_INFO )
	{
	    /* check the address to see if its ours */
#ifdef OK2LOOP
	    if( !ok2loop )
    	    {
#endif
		if( address != rem_address ) 
		{
		    vect = 0;
		    break;
		}
#ifdef OK2LOOP
	    }
#endif

	    /* check for the presence of an ack block */
	    if(( ubyte & ACK_PRES )&& pglogon )
		ack_pres = TRUE;

	    max_incnt = dblkbsz;
	    if( pglogon )   		/* must be logged on */
	    {
	    	if( ubyte & ACK_REQ )
		{ 
	    	    PGBTRACE(2)("pgpby: ack requested\n");	
		    snakh = TRUE;	/* send ack if req */
		}

		/*  chk recv window for this seq number
		**  if empty, setup to insert data into buff directly 
		**  else (blk already recvd) throw away blk, but
		**  chk crc anyway
		*/

		rbkn = ubyte & SEQ_NUM;	/* isolate seq num */

		/* if we need this blk and the buffer is mt ... */
		if(( rbmap & bmap[ rbkn ] ) &&	
		    !rbksiz[ rbni=rbkn & RWDWMK] ) 
		{
		    inptr = rbufa[rbni];
		    	    	    	/* set address */
		    blk_id = NUM_INFO;
		}
	    }

    	    if( !inptr ) 		/* ignore this block */
	    {
		PGBTRACE(3)("pgpby: ignoring block\n");
		inptr = rtmpbuf;	/* null ptr is a flag */
		blk_id = IGNORE;    	/* ignore this blk */
	    }
	}
    	else
	{
	    blk_id = ubyte & BLK_ID;
#ifdef OK2LOOP
	    if( !ok2loop ) 
    	    {
#endif
		switch( blk_id )    	/* what kind? */
		{
		case SASM:
		    break;
		case DISC:
		case DISCI:
		case UNUM_INFO:
		    i = rem_address;	/* what should it be? */
		    goto PGPBY2;
		case ACK:
		case SASMA:
		    i = loc_address;
PGPBY2:
		    if( address != i )  /* is that what it is? */
		    {
			vect = 0;
			return( 0 );
		    }
		    break;
		default:
		    vect = 0;
		    return( 0 );
		}
#ifdef OK2LOOP
	    }
#endif
	
    	    /* At this point, we know it's our block */
	    switch( blk_id )
	    {
	    case SASM:
	    case SASMA:
		max_incnt = DBLK_BSZ;
		inptr = rtmpbuf;
		break;
	    case DISC:
	    case DISCI:
		max_incnt = CBLK_CSZ-2;	/* max bytes expected */
		break;
	    case ACK:
		if( pglogon )
		{
		    inptr = rtmpbuf;
		    max_incnt = ACK_CSZ-2;	
    	    	    	    	    	/* max bytes expected */
		}
		break;
	    case UNUM_INFO:		/* unnumbered information */
		if( pglogon ) 
    	    	{
		    if( ubyte & ACK_PRES )
			ack_pres = TRUE;
		    snakh = TRUE;
		    max_incnt = DBLK_BSZ;
		    if(( ubyte & UI_FF )!= ruiff )
    	    	    	    	    	/* new UI blk? */
		    	inptr = ruipr;
		}
			
    		/* didn't need it, so ignore it */
		if( !inptr )
		{
		    blk_id = IGNORE;
		    inptr = rtmpbuf;	/*-- save it for ack --*/
		}
		break;
	    }
	}
	break;

    case 3:
	if( ubyte < 0 ) 		/* end of block */
	{
	    vect = 0;	    		/* finished with this blk */
    	    if( crc == good_crc ) 	/* crc good? */
	    {
		erlog( FALSE );	    	/* show no error */
		switch( blk_id )
		{
		case SASM:  		/* got a SASM blk */
		    PGBTRACE(2)("pgpby: sasm received\n");
		    rtmpbuf[ incnt - 2 ] = 0;
    	    	    	    	    	/* chop off crc */
		    return( upsasm( rtmpbuf ));
    	    	    	    	    	/* unpack it */

		case SASMA:
		    PGBTRACE(2)("pgpby: sasma received\n");
		    rtmpbuf[ incnt - 2 ] = 0;
		    return( upsasma( rtmpbuf ));

		case DISC:
		case DISCI:
PGPBY1:
		    i = 0200 | blk_id; 
		    break;

		case UNUM_INFO:
		    i = incnt - 2;	/* chop off crc */
		    if( ack_pres ) 
    	    	    {	    	    	/* ack on this blk */
			i -= ACK_ISZ;
			uprak( inptr-(ACK_ISZ+2) );
		    }
		    ruicnt = i;
		    ruiff = hibits & UI_FF;
    	    	    	    		/* toggle ui ff */
		    goto PGPBY1;

		case ACK:
		    PGBTRACE(2)("pgpby: ack received\n");
    	    	    uprak( rtmpbuf );
		    goto PGPBY1;

    	    	case IGNORE:
		case NUM_INFO:
		    i = incnt - 2;	/* get size of blk */
		    if( ack_pres )  	/* ack in this blk? */
		    {
		        PGBTRACE(2)("pgpby: ack received in num_info\n");
			i -= ACK_ISZ;
			uprak( inptr-(ACK_ISZ+2) );
		    }
		    if( blk_id != IGNORE ) 
		    {
			rbksiz[rbni] = i;
    	    	    	    	    	/* save count */
			rbmap &= ~bmap[rbkn];
    	    	    	    		/* clr bit */
			/* try to advance window */
			i = 0;
			while((rbmap & bmap[rwdwb] )== 0L)
			{
			    rbmap |= bmap[(rwdwb+rwdwsz)&SEQ_NUM];
			    rwdwb = inc_seqn( rwdwb );
			    ++i;
			}
		    }
		    else
    	    	    	i = 0;	    	/* ignored blk, no new blks */

		    i |= 0100;	    	/* show data blk recvd */

		}   	    	    	/* end of switch( blk_id ) */
	    }	    	    	    	/* end of if( crc == good ) */
    	    else 
    	    {				/* bad crc */
    	        PGBTRACE(1) ("pgpby: error, bad crc\n");
		erlog( TRUE );
		i = 0;
	    }

    	    if( snakh ) 
	    {        	    	    	/* if ack was requested, send it */
		snakh = FALSE;	    	/* even if crc was bad */
    	    	snakf = TRUE;
	    }

    	    return( i );	    	/* show data blk recvd */
	}
	else 
    	    if( ++incnt > max_incnt  ) 	/* overflow */
	    {
		vect = 0;
    	        PGBTRACE(1) ("pgpby: error, buffer overflow\n");
		erlog( TRUE );		/* log an error */
	    }
	    else 
    	    	if( inptr ) 		/* if we need this blk */
		    *inptr++ = ubyte;	/* add byte to buffer */
	break;

    case 4: 	    	    	 	/* check for ";disc.", user abort */
	if( ubyte == 0x27 ) 
    	    ++vect;
	else 
	    vect = 0;
	break;
    case 5:
	if( ubyte == 0x0C )
	    ++vect;
	else
	    vect = 0;
	break;
    case 6:
	vect = 0;
	if( ubyte < 0 )
	    return( 0200 | DISCI );
	break;
    }	                                /* end of switch( vect ) */

    return( FALSE );

}	/* end of pgpby() */

/*{
** Name: inc_seqn
**
** Description: 
**	increment sequence number
**
** Inputs:
**  	seqn -	    sequence number to be incremented
**
** Outputs:
**	none
**	
** Returns:
**	incremented sequence number
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	rejigged
**		
*/
static int
inc_seqn( seqn )
int seqn;
{
    return((++seqn) & SEQ_NUM);
}

/*{
** Name: inc_wdwn
**
** Description: 
**	increment window number
**
** Inputs:
**  	wdwn -	    window number to be incremented
**
** Outputs:
**	none
**	
** Returns:
**	incremented window number
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	rejigged
**		
*/
static int
inc_wdwn( wdwn )
int wdwn;
{
    return((++wdwn) & RWDWMK);
}

/*{
** Name: xmtblk
**
** Description: 
**      transmit a blk: adds the crc (must be room in
**  	caller's buffer; packs in 3/4th's or 7/8th's format;
**	calls 'putcm' of siodvr to send it out
**
** Inputs:
**  	pf -	    address of block to be transmitted
**  	cnt -	    length of block
**  	flag -	    TRUE if force 8 bit encodings.
**
** Outputs:
**	none
**	
** Returns:
**	nothing
**
** Exceptions:
**	none
**
** History:
**	06-Feb-91 (cmorris) 
**  	    	Returns type VOID
**		
*/
static VOID
xmtblk( pf, cnt, flag )
char *pf;
int cnt, flag;
{
    static char pbuf[DBLK_PSZ];	    	/* packed blk buffer */
    char *pt = pbuf;
    register int i;

    if( !pg7bit && flag )		/* init the FLAG byte */
	*pt++ = FLAG8;
    else
	*pt++ = FLAG;			/* packed buff init */

    i = add_crc( pf, cnt );		/* add the crc */

    if( !pg7bit && flag )		/* pack in printable ASCII */
	i = pk78( pf, i, pt );
    else
	i = pk34( pf, i, pt );	    	/* pack in 3/4ths */

    pbuf[ ++i ] = TAIL;		    	/* add the TAIL */
    pbuf[ ++i ] = RETURN;		/* and the RETURN */

    /********* TESTING CODE *********/
/*
    pbuf[ ++i ] = '\n';
*/
    putcm( pbuf, ++i );			/* start transmit */

    pcmbsy = TRUE;

    /* if we're in half duplex ... */
    if( pghdx ) 
    {
	lbkwu = FALSE;		    	/* assume not a wake-up blk */

	/* if this is a data blk w/o an ack req ... */
	if(( *pf & NUM_INFO )&&!( *pf & ACK_REQ )) 
    	    wutimo = pg_delay;	    	/* use inter-line delay */
	else
	    wutimo = pgt1;		/* use turn-around delay */
    }
}

/*{
** Name: blrand
**
** Description: 
**      generate a 32-bit random number
**
** Inputs:
**  	none
**
** Outputs:
**	none
**	
** Returns:
**	random number
**
** Exceptions:
**	none
**
** History:
**	12-Feb-91 (cmorris) 
**  	    	moved in from siodvr.c
**	09-Jan-92 (alan)
**		Use the CL.
**		
*/
static long
blrand(void) 
{
    /* set seed for random numbers */
    MHsrand(TMcpu());

    /* get an f8 random number and send back the low-order signed long */
    return(((long)MHrand())&0x7fffffff);
}

/*
** Name: gcblsio.c - VMS BLAST Serial I/O driver for GCC ASYNC Protocol
**
** Description:
**	Perform serial I/O functions for BLAST asynchronous protocol
**	driver.
**
** History:
**	29-Jan-92 (alan)
**	    Added header, renamed, and extensively modified as per
**	    cmorris' associated unix BLAST changes.
**	27-Aug-92 (alan)
**	    Massive fixes and improvements.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	26-oct-01 (kinte01)
**	    cleaned up compiler warnings
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
**      22-dec-2008 (stegr01)
**          Itanium VMS port.
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
int br;		"Br" is an integer representing the baud rate. (see gcblsio.h)
int py;		"Py" is an integer representing the parity setting.

commd(m,x)	Set comm port operating mode.
int m;		"M" sets either byte at a time, bunch at a time, or line
int x;		at a time comm port input. "X" is non-zero if flow/control
		may be used.

comxon()	Force X'off'd output back on.  Comxon() 
		should restart output that might be suspended 
		due to the receipt of an XOFF.
*/

#include <compat.h>
#include <gl.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <cv.h>
#include <tr.h>
#include <gcccl.h>

#include <dcdef.h>
#include <descrip.h>
#include <efndef.h>
#include <iledef.h>
#include <iodef.h>
#include <iosbdef.h>
#include <ssdef.h>
#include <syidef.h>
#include <ttdef.h>

#include <starlet.h>

#include <gcblerrs.h>
#include <gcblsio.h>
#include <astjacket.h>

/*
** Additional definitions
*/
# define TT2$M_ALTYPEAHD 128             /* use ALTYPEAHEAD (from TT2DEF.H)  */
# define VMS_STATUS  long
# define VMS_OK(x)   (((x) & 1) != 0)
# define VMS_FAIL(x) (((x) & 1) == 0)

/*
** Forward functions
*/
FUNC_EXTERN int		sioinit();
FUNC_EXTERN VOID	sioreg();
FUNC_EXTERN VOID	siodone();
FUNC_EXTERN VOID	siowrite();
FUNC_EXTERN VOID	sioread();
FUNC_EXTERN int		cominit();
FUNC_EXTERN int		comst();
FUNC_EXTERN VOID	comdone();
FUNC_EXTERN VOID	commd();
FUNC_EXTERN VOID	setto();
FUNC_EXTERN VOID	putcm();
FUNC_EXTERN VOID	getcm();
FUNC_EXTERN int		iopnd();
FUNC_EXTERN VOID	comxon();
static VOID		timeout_ast();
static VOID		sioread_ast();
static VOID		siowrite_ast();
static VOID		ltoa();

/* 
** Callback functions 
*/
static VOID (*cmread) ();   	    	/* called when read completes */
static VOID (*cmwrite) ();  	    	/* called when write completes */
static PTR  cmclosure;	    	    	/* closure parm for read/write */

/*
** Quadword structure for VMS System Services 
*/
typedef struct _INT8
{
	int	high;
	int	low;
} INT8;

/*
** Terminal Characteristics buffer for IO$_SETMODE
*/
typedef struct _TT_MODE
{
	unsigned char	class;
	unsigned char	type;
	unsigned short	page_width;
	unsigned int 	basic_chars;
	unsigned int 	extended_chars;
} TT_MODE;
	
static i4 SIO_trace = 0;
# define SIOTRACE(n) if( SIO_trace >= n )(void)TRdisplay

/*
**	Local variables
*/

#define COMBUFMAX    	800		/* max read buffer size */
#define MIN_TYPEAHEAD  2048		/* min size of terminal typeahead */

static int comfd;	    	    	/* comm port channel number */
static TT_MODE ttmode_initial;		/* comm port initial mode */
static TT_MODE ttmode_blast;		/* comm port blast mode */
static int sysgen_altypahd;		/* SYSGEN TTY_ALTYPAHD value */
static int sysgen_typahdsz;		/* SYSGEN TTY_TYPAHDSZ value */
static int maxrbuf;			/* maximum bytes for comm read */

IOSB	riosb;          		/* read i/o status block */
IOSB	wiosb; 	  			/* write i/o status block */

/* comm port read variables */
static char cmrbf[COMBUFMAX];      	/* read buffer */
static char *cmrbp; 	    		/* read buffer pointer */
static int cmrct;			/* comm read count */
static char *cmrpr;			/* comm read pointer */
static int cmrerr;  	    	    	/* comm port read error */
static int frame = 1;			/* current frame size */

/* comm port send variables */
static int maxwbuf;			/* maximum bytes for comm write */
static char *cmspr;			/* comm send pointer */
static int cmsct;			/* comm send count */
static int cmserr;  	    		/* comm port send error */
static int cmslm;			/* comm send length */

/* timer variables */
static int timon;	    	    	/* timer on flag */
static int timout;			/* timer expired flag */

/*
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
**	25-june-91 (hasty)
**		converted to VMS
**		
**      16-jul-93 (ed)
**	    added gl.h
**	26-oct-01 (kinte01)
**	    cleaned up compiler warnings
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
*/
int
sioinit( )
{
    char *trace;
    ILE3		syi_list[4] = {  { 4, SYI$_MAXBUF, (PTR)&maxwbuf, 0 },
			         { 4, SYI$_TTY_ALTYPAHD, (PTR)&sysgen_altypahd, 0 },
			         { 4, SYI$_TTY_TYPAHDSZ, (PTR)&sysgen_typahdsz, 0 },
				 { 0, 0 }  };
    IOSB iosb;

    NMgtAt( "II_SIO_TRACE", &trace );

    if( trace && *trace )
        CVan( trace, &SIO_trace );

    timon = FALSE;
    timout = FALSE;

    /* Find out the system's MAXBUF and ALTYPAHD values. */
    sys$getsyiw(EFN$C_ENF, 0, 0, syi_list, &iosb, 0, 0);

    /*  
    ** MAXBUF less 64 is the maximum buffer the terminal driver can write,
    */
    maxwbuf -= 64;

    SIOTRACE(2)("sioinit: completed with $qio write bufsize = %d\n", maxwbuf);

    /* all done */
    return (OK);
}

/*
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
**		
*/
VOID
siodone( )
{
}

/*
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
    $DESCRIPTOR(comm_dev, "TT:");
    VMS_STATUS	status = 0;
    IOSB iosb;

    cmspr = 0;	    	    	    	/* no send data */
    cmsct = 0;
    cmserr = 0;
    cmrpr = 0;	    	    	    	/* no receive data */
    cmrct = 0;
    cmrerr = FALSE;
    cmslm = 0;

    /* if dev is not null, use it */
    if ( *dev )
    {
	comm_dev.dsc$a_pointer = dev;
        comm_dev.dsc$w_length = STlength(dev);
    }

    /* assign comm port  */
    status = sys$assign(&comm_dev, &comfd, 0, 0);
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("cominit:  terminal sys$assign error (%x)\n", status);
	return( FAIL );
    }

    /* save current mode */
    status =  sys$qiow(EFN$C_ENF, comfd, IO$_SENSEMODE, &iosb, 0, 0, 
		       &ttmode_initial, 12, 0, 0, 0, 0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("cominit:  terminal IO$_SENSEMODE error (%x)\n", status);
	return( FAIL );
    }

    /* must be a terminal device */
    if ( ttmode_initial.class != DC$_TERM )
    {
        SIOTRACE(1)("cominit:  TT device is not of device type DC$_TERM\n");
        return( FAIL );
    }

    /* 
    ** The terminal's typeahead buffer must be at least MIN_TYPEAHEAD large.
    ** If the TT2$M_ALTYPEAHD is set in the extended characteristics buffer,
    ** then the alternate typeahead buffer is in use; otherwise the default
    ** typeahead buffer is checked.
    */
    if ( ttmode_initial.extended_chars & TT2$M_ALTYPEAHD )
    {
	if ( sysgen_altypahd < MIN_TYPEAHEAD )
        {
	    SIOTRACE(1)("cominit:  TT: altypahd less than 2048\n");
	    return( FAIL );
        }
    }
    else
    {
	if ( sysgen_typahdsz < MIN_TYPEAHEAD )
	{
	    SIOTRACE(1)("cominit:  TT: typeahead size less than 2048\n");
	    return( FAIL );
        }
    }

    /* 
    ** Define BLAST terminal characteristics based on current settings.
    ** Maintain: TT$M_REMOTE, TT$M_MODEM, TT$M_OPER 
    ** Set: TT$M_MBXDSABL, TT$M_NOBRDCST, TT$M_NOECHO , TT$M_HOSTSYNC, 
    **      TT$M_LOWER, TT$M_TTSYNC
    ** Clear: All others, including:  TT$M_EIGHTBIT (i.e. Blast is 7 bit),
    **      TT$M_ESCAPE, TT$M_HALFDUP, TT$M_NOTYPEAHD, TT$M_PASSALL,
    **      and TT$M_READSYNC.
    */
    ttmode_blast.class = ttmode_initial.class;
    ttmode_blast.type = ttmode_initial.type;
    ttmode_blast.page_width = ttmode_initial.page_width;
    ttmode_blast.basic_chars = ( ( ttmode_initial.basic_chars
	& ( TT$M_REMOTE | TT$M_MODEM | TT$M_OPER | TT$M_PAGE ) )
        | ( TT$M_MBXDSABL | TT$M_NOBRDCST | TT$M_NOECHO  | 
	    TT$M_HOSTSYNC | TT$M_LOWER | TT$M_TTSYNC ) );
    ttmode_blast.extended_chars = ttmode_initial.extended_chars;

    return( OK );
}

/*
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
**		
*/
VOID
comdone( )
{
    IOSB iosb;

    SIOTRACE(2)("comdone: resetting port to original mode\n");

    sys$cancel(comfd);    
 
    /* reset comm port to original mode */
    sys$qiow(EFN$C_ENF, comfd, IO$_SETMODE, &iosb, 0, 0, &ttmode_initial, 12,
             0, 0, 0, 0);
}

/*
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
**	31-aug-92 (alan)
**	    Cleaned up.
**		
*/
VOID
setto( to )
int to;
{
    INT8	delta_time;

    sys$cantim(0,0);					/* cancel all timers */

    if (to > 0)
    {
        delta_time.high = -100000 * to;
        delta_time.low = -1;
        timon = TRUE;
        sys$setimr(0, &delta_time, timeout_ast, cmclosure, 0); /* start timer */
	SIOTRACE(3)("setto: timeout of %d centiseconds\n", to);
    }
    else
    {
        timon = FALSE;
    }
    timout = FALSE;
}

/*
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
**	25-june-91 (hasty)
**		coded for ast support
**		
*/
VOID
putcm( ptr,cnt )
char *ptr; 
int cnt;
{
    cmspr = ptr;
    cmsct = cnt;
    SIOTRACE(2)("putcm: request to write %d bytes to chan %d\n", cnt, comfd);

    siowrite(0);
}

/*
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
**	25-june-91 (hasty) 
**		added ast support
**  	    	
**		
*/
VOID
getcm( ptr )
char *ptr;
{
    cmrpr = ptr;
    if (!cmrct) 
    {
        SIOTRACE(2)("getcm: request to read from chan %d\n", comfd);
    	sioread();
    }
}

/*
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
**	25-june-91 (hasty) 
**  	    	rewritten for AST event handling
*/
int
iopnd( siocb )
SIO_CB *siocb;
{
    if( timon && timout ) 		/* timed out? */
    {
	SIOTRACE(4)("iopnd: timeout (SIO_TIMO)\n");
	timon = FALSE;
	timout = FALSE;
	return( SIO_TIMO );
    }

    if( cmserr)	    	    	    	/* send error? */
    {
	SIOTRACE(1)("iopnd: send error = %x\n", wiosb.iosb$w_status);
    	siocb->status = GC_SEND_FAIL;
   	siocb->syserr->error = wiosb.iosb$w_status;
    	cmserr = 0;
    	return( SIO_ERROR );
    }

    if( !cmsct && cmspr ) 		/* send done? */
    {
	SIOTRACE(4)("iopnd: send done (SIO_COMWR)\n");
    	cmspr = 0;
	return( SIO_COMWR );
    }

    if ( cmrerr )    	    	    	/* receive error? */
    {
	SIOTRACE(1)("iopnd: receive error = %x\n",riosb.iosb$w_status);
    	siocb->status = GC_RECEIVE_FAIL;
   	siocb->syserr->error = riosb.iosb$w_status;
    	cmrerr = FALSE;
    	return( SIO_ERROR );
    }

    if( cmrct && cmrpr ) 		/* receive data ready? */
    {
	SIOTRACE(4)("iopnd: receive data ready (SIO_COMRD)\n");
	cmrct--;
	*cmrpr = *cmrbp++;
	cmrpr = 0;
	return( SIO_COMRD );
    }

    SIOTRACE(4)("iopnd: (SIO_BLOCKED)\n");
    return( SIO_BLOCKED );
}	/* end of iopnd */

/*
** Name: timeout_ast
**
** Description: 
**      When timer expires set time out flag.
**
** Inputs:
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
**	25-jun-91 (hasty) 
**  	    	
*/
static VOID
timeout_ast( parm )
int *parm;
{
    timout = TRUE;
    SIOTRACE(2)("timeout_ast:  timer expired\n");
    (*cmwrite)(parm);
}
/*
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
**	25-june-91 (hasty) 
**  	    	
**		
*/
int
comst( portid, bdrt, par )
char *portid;   
int bdrt, par;
{
    VMS_STATUS	status = 0;
    static int bdtab[9] =                      /* baud rate table */
    {
	TT$C_BAUD_300, 
	TT$C_BAUD_600, 
	TT$C_BAUD_1200,
	TT$C_BAUD_2400, 
	TT$C_BAUD_4800, 
	TT$C_BAUD_9600, 
	TT$C_BAUD_19200,
	0,                        		/* no 38.4 in VMS */
	0                                   	/* for host mode */
    };   
    static int patab[5] = 
    {
	TT$M_ALTRPAR, 				/* no parity */
	TT$M_ALTRPAR | TT$M_PARITY | TT$M_ODD |
	TT$M_ALTDISPAR | TT$M_DISPARERR,        /* odd parity */
	TT$M_ALTRPAR | TT$M_PARITY |
	TT$M_ALTDISPAR | TT$M_DISPARERR,        /* even parity */
	TT$M_ALTRPAR, 				/* no parity */
	TT$M_ALTRPAR 				/* no parity */
    };
    static int dbtab[5] =          
    {
	TT$M_EIGHTBIT,                          /* 8 data bits */
	0,                                      /* 7 data bits */
	0,					/* 7 data bits */
	TT$M_EIGHTBIT,                          /* 8 data bits */
	TT$M_EIGHTBIT,                          /* 8 data bits */
    };
    IOSB iosb;

    SIOTRACE(2)("comst:  setting BLAST mode on channel %d, baud %d, parity %d\n",
	comfd, bdrt, par);

    ttmode_blast.basic_chars = 
	ttmode_blast.basic_chars & (dbtab[par] ^ -1);	/* 7/8 bit */

    sys$cancel( comfd );
    status = sys$qiow(EFN$C_ENF,	/* set mode event flag  */
                       comfd,           /* comm port channel */
                       IO$_SETMODE,     /* set mode function */
                       &iosb,
                       0,
                       0,
                       &ttmode_blast,   /* addr of blast char buffer */
                       12,              /* 12 bytes long */
                       bdtab[bdrt],	/* baud rate */
                       0,
                       patab[par],      /* parity */
                       0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("comst:  IO$_SETMODE error (%x)\n", status);
	return( FAIL );
    }
    return( OK );
}

/*
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
    VMS_STATUS	status = 0;
    IOSB	iosb;

    switch(mode)
    {
	case -1:		/* line reads */
	    frame = COMBUFMAX;
	    break;

        case  1:		/* 8 byte reads */
            frame = 8;
            break;

         default:       	/* 1 byte reads */
	    frame = 1;	
	    break;
    }

    SIOTRACE(2)("commd: setting port mode, frame = %d, xon = %d\n",
	frame, xon);

    /* enable/disable flow control */ 
    if (xon)
        ttmode_blast.basic_chars &= (TT$M_PASSALL ^ -1);
    else 
        ttmode_blast.basic_chars |= TT$M_PASSALL;

    sys$cancel(comfd);

    status = sys$qiow(EFN$C_ENF, comfd, IO$_SETMODE, &iosb, 0, 0, 
		       &ttmode_blast, 12, 0, 0, 0, 0);
    if (status & 1)
	status = iosb.iosb$w_status;
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("commd:  IO$_SETMODE error (%x)\n", status);
    }
    else
    {
        comxon();			/* force xoff'd output back on */
    }
}

/*
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
**  	    Cleaned up.
**		
*/
VOID
comxon()
{
    char *ctrlq = "\021";
    IOSB iosb;

    /* send ^Q */
    SIOTRACE(2)("comxon:  sending <ctrl-q>\n");
    sys$qiow(EFN$C_ENF, comfd, IO$_WRITELBLK|IO$M_CANCTRLO|IO$M_NOFORMAT,
	      &iosb, 0, 0, ctrlq, 1, 0, 0, 0, 0);
}

/*
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
sioreg( cmrdfunc, cmwtfunc, cmclosparm)
VOID		(*cmrdfunc)();
VOID		(*cmwtfunc)();
PTR 	    	cmclosparm;
{
    /* register functions */
    cmread = cmrdfunc;
    cmwrite = cmwtfunc;
    cmclosure = cmclosparm;
}

/*
** Name: sioread()
**
** Description: Post serial I/O read
**      when read completes, sioread_ast handles it.
**	
** Inputs:
**  	error - indicates whether read could not be successfully posted.
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
**	  25-june-91 (hasty)
**		Initial coding.
*/
VOID 
sioread( )
{
    VMS_STATUS	status = 0;
    int		cmbct;			/* local buffer byte count */
    INT8	comm_tac;		/* read typeahead count */

    cmrerr = FALSE;

    status = sys$qiow(EFN$C_ENF, comfd, IO$_SENSEMODE | IO$M_TYPEAHDCNT,
 		      &riosb, 0, 0, &comm_tac, 0, 0, 0, 0, 0);
    if( VMS_FAIL( status ) || ( VMS_FAIL( riosb.iosb$w_status ) ) )
    {
        SIOTRACE(1)("sioread:  IO$_SENSEMODE error (%x, %x)\n", 
	    status, riosb.iosb$w_status);
	cmrerr = TRUE;
	return;
    }

    cmbct = comm_tac.high & 0xFFFF;        /* read all bytes waiting */
    SIOTRACE(2)("sioread: %d bytes waiting\n", cmbct);

    if (cmbct > COMBUFMAX) 
	cmbct = COMBUFMAX;
    if (cmbct == 0) 
	cmbct = frame;      		   /* nothing waiting - read min */

    SIOTRACE(2)("sioread: posting $qio for %d bytes\n", cmbct);
    riosb.iosb$w_status = SS$_NORMAL;
    status = sys$qio(0, 
                     comfd, 
                     IO$_READLBLK|IO$M_NOECHO|IO$M_DSABLMBX,
                     &riosb,
	             sioread_ast, 
                     cmclosure, 
                     cmrbf, 
                     cmbct,
		     0,
                     0,
                     0, 
                     0);
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("sioread:  error posting IO$_READLBLK (%x)\n", status);
	cmrerr = TRUE;
    }
}

/*
** Name: sioread_ast()
**
** Description: Complete serial I/O read
**
** Inputs:
**  	parm - parameter passed to cmread callback completion routine
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
**	  25-june-91 (hasty)
**		Initial coding.
*/
static VOID
sioread_ast(parm)
int *parm;
{
    if ( riosb.iosb$w_status != SS$_NORMAL) 
    {
	if ( (riosb.iosb$w_status == SS$_ABORT) ||
	     (riosb.iosb$w_status == SS$_CANCEL) )
        {
            SIOTRACE(1)("sioread_ast: $QIO read cancelled (%x)\n", 
		riosb.iosb$w_status);
        }
	else
	{
            SIOTRACE(1)("sioread_ast: $QIO read error (%x)\n", 
		riosb.iosb$w_status);
	}
  	cmrerr = TRUE;
	return;
    }

    cmrbp = cmrbf;
    cmrct = riosb.iosb$w_bcnt;
    SIOTRACE(2)("sioread_ast: read %d bytes from fd %d\n", cmrct, comfd);

    /* sometimes we get a null read so re-issue the read */
    if (cmrct == 0) 
    {
	sioread();
	return;
    }

    /* before returning call user's call back routine */
    (*cmread)(parm);
}

/*
** Name: siowrite()
**
** Description: Post serial I/O write
**	
** Inputs:
**  	error - indicates whether write could not be successfully posted.
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
**	25-june-91 (hasty)
**		Initial coding.
*/
VOID 
siowrite( error )
int error;
{
    int n;
    VMS_STATUS	status = 0;

    cmserr = FALSE;
    cmslm = cmsct < maxwbuf ? cmsct : maxwbuf;
    SIOTRACE(2)("siowrite: writing %d bytes to fd %d\n", cmslm, comfd);

    /* write out anything to send */
    wiosb.iosb$w_status = SS$_NORMAL;
    status = sys$qio( 0,
                      comfd,
                      IO$_WRITELBLK|IO$M_CANCTRLO|IO$M_NOFORMAT,
                      &wiosb,
                      siowrite_ast,
                      cmclosure,
		      cmspr,
                      cmslm,
                      0,
                      0,
                      0,
                      0 );
    if( VMS_FAIL( status ) )
    {
        SIOTRACE(1)("siowrite:  error posting IO$_WRITELBLK (%x)\n", status);
	cmserr = TRUE;
    }
}

/*
** Name: siowrite_ast()
**
** Description: Complete serial I/O write
**
** Inputs:
**  	parm - parameter passed to cmread callback completion routine
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
**	25-june-91 (hasty)
**		Initial coding.
*/
static VOID
siowrite_ast(parm)
int *parm;
{
    if ( wiosb.iosb$w_status == 0 ) 
	wiosb.iosb$w_status = SS$_NORMAL;
    if ( wiosb.iosb$w_status != SS$_NORMAL) 
    {
	if ( (wiosb.iosb$w_status == SS$_ABORT) ||
	     (wiosb.iosb$w_status == SS$_CANCEL) )
        {
            SIOTRACE(1)("siowrite_ast: $qio write cancelled (%x)\n", 
	        wiosb.iosb$w_status);
        }
	else
	{
            SIOTRACE(1)("siowrite_ast: $qio write error (%x)\n", 
	        wiosb.iosb$w_status);
	    cmserr = TRUE;
	}
	return;
    }

    cmsct -= cmslm;
    cmspr += cmslm;
    SIOTRACE(2)("siowrite_ast: wrote %d bytes, %d remaining\n",
	cmslm, cmsct);

    if (cmsct > 0) 
    {
	/* we are not done writing out the user's buffer */
	siowrite(0);
	return;
    }

    /* let user know, that we are done writing */
    (*cmwrite)(parm);
}

/*
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

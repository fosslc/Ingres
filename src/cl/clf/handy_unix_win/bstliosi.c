/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<clpoll.h>
# include	<st.h>
# include	<me.h>
# include	<er.h>

# include	<errno.h>
# include	<bsi.h>

# if defined(xCL_TLI_OSLAN_EXISTS) || defined(xCL_TLI_X25_EXISTS)

# include	<systypes.h>

# if   defined(any_aix) || defined(any_hpux)
# include	<xti.h>
# else
# include	<tiuser.h>
# endif

# ifdef xCL_006_FCNTL_H_EXISTS
# include	<fcntl.h>

# if !defined(OSI_NONBLOCK) && defined(O_NONBLOCK)
# define        OSI_NONBLOCK     O_NONBLOCK
# endif

# if !defined(OSI_NONBLOCK) && defined(O_NDELAY)
# define        OSI_NONBLOCK     O_NDELAY
# endif

# endif /* xCL_006_FCNTL_H_EXISTS */

# ifdef xCL_007_FILE_H_EXISTS
# include	<sys/file.h>
# if !defined(OSI_NONBLOCK) && defined(FNDELAY)
# define        OSI_NONBLOCK     FNDELAY
# endif
# endif /* xCL_007_FILE_H_EXISTS */

# include	"bstliio.h"
# include	"bsosi.h"

/* External variables */


/*
** Name: bstliosi.c - BS interface to ISO OSI networks via XTI/TLI.
**
** Description:
**	This file provides access to ISO OSI network IPC via X/Open XTI, 
**	using the generic BS interface defined in bsi.h.  See that file for a 
**	description of the interface.
**
**	The following functions are internal to this file:
**
**	addr_to_tsap - convert address in node::port form to OSI tsap
**	tsap_to_addr - convert OSI tsap to printable representation
**
**	The following functions are exported as part of the BS interface:
**
**		osi_listen - establish an endpoint for accepting connections
**		osi_unlisten - stop accepting incoming connections
**		osi_accept - accept a single incoming connection
**		osi_request - make an outgoing connection
**		osi_connect - complete outgoing connection
**		osi_send - send data on established connection
**		osi_receive - receive data on established connection
**		osi_close - close a connection
**		osi_regfd - register operation with poll/select
**		osi_portmap - map installation id to a 'port'
**
** History:
**	20-Jan-92 (sweeney)
**	    tiuser.h should always be in /usr/include.
**	24-Jan-92 (sweeney)
**	    plagiarized from bstlitcp.c, removed redundant history.
**	26-Feb-92 (sweeney)
**	    [ swm history comment for sweeney changes ]
**	    Several cleanup changes:
**	    o corrected xCL ifdefs to xCL_TLI_OSLAN_EXISTS and
**	      xCL_TLI_X25_EXISTS;
**	    o conditionally compiled the name of the OSI transport provider
**	      special file, it is platform specific;
**	    o corrected addr argument to tli_addr to be a pointer;
**	    o renamed tli_listen() addr argument to tsap;
**	    o mapped symbolic address of the form node::port to the
**	      transport level path expected on ICL systems (this code will be
**	      made generic in a subsequent code change);
**	    o pass sizeof an OSI address in the (new) parameter to tli_request
**	      in addition to the actual address length because the transport
**	      provider needs to `know' it.
**	11-Mar-92 (sweeney)
**	    localised the ICL dependencies
**	27-Mar-92 (sweeney)
**	    At least one implementation of XTI (IBM RS6000) does not
**	    implement t_alloc, t_free, t_getinfo. Decouple from generic
**	    tli routines. n.b. include new definitions of bcb, lbcb.
**	1-Apr-92 (sweeney)
**	    use generic OSI_NONBLOCK
**	6-Apr-92 (sweeney)
**	    Fix up #error as per daveb recommendation.
**	10-Sep-92 (gautam)
**	    Revamped 
**	    o Use all tli calls - avoid code duplication and errors
**	    o changed the addr_to_tsap() and tsap_to_addr() interface 
**	      to be more generic
**	    o Added support routines for hex_char-binary conversion
**	    o Porting changes for RS/6000 OSIMF
**	12-Sep-92 (gautam)
**	    Added in HP-UX support
**	31-Dec-92 (edg)
**	    Removed support for II_OSI_QLEN.  Support for this knob will
**	    be generally folded into TLI.
**	11-Jan-93 (edg)
**	    Use gcu_get_tracesym() to get trace level.
**	18-Jan-93 (edg)
**	    Replace gcu_get_tracesym() with inline code.
**      02-Mar-93 (brucek)
**          Added tli_release to BS_DRIVER.
**	04-Mar-93 (edg)
**	    BS_DRIVER no longer has detect func ptr in structure.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-Jul-93 (brucek)
**	    Added tcp_transport arg on call to tli_listen.
**	28-oct-93 (swm)
**	    Bug #56030
**	    Integrated pauland changes from ingres63p including bug
**	    fix #56030.
**          18-aug-93 (pauland)
**      	Added usl_us5 support as per dra_us5.
**	    21-Oct-93 (pauland)
**		Removed tli_release from protocol table for dr6_us5 and usl_us5
**		since ICL's OSLAN doesn't have an orderly release.
**		WARNING: if the chosen protocol doesn't have on orderly release
**		leaving tli_release will cause bug #56030 - the iigcc process
**		consumes file descriptors until the protocol's device driver
**		can't cope.
**	20-Dec-94 (GordonW)
**		comment after a endif.
**	09-feb-95 (chech02)
**		Added rs4_us5 for AIX 4.1.
**	20-Mar-95 (smiba01)
**		Added support for dr6_ues
**	06-jun-1995 (canor01)
**		semaphore protect memory allocation routines
**	08-feb-1996 (sweeney)
**		Comment OSIDEBUG token after # endif.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores. Removed ME_stream_sem.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	10-may-1999 (walro03)
**	    Remove obsolete version string dr6_ues, dra_us5.
**	16-jul-1999 (hweho01)
**	    Added support for AIX 64-bit (ris_u64) platform.
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      19-nov-2002 (loera01)
**          Re-introduced osi_accept() as a wrapper for tli_accept().
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/


#define OSIDEBUG
#ifdef	OSIDEBUG
static int osi_debug ;
#define  OSITRACE(n)	if(osi_debug >= n)(void)TRdisplay
#endif

/*
** Per-platform definition of OSI transport provider device/name - 
** needs to be shorter than MAXDEVNAME from bsosi.h (currently 64)
*/

#if defined (dr6_us5)
#define OSI_DEVNAME "/dev/isol4"
#endif

#if defined (usl_us5)
#define OSI_DEVNAME "/dev/osits"
#endif

#if defined (any_aix)
#define OSI_DEVNAME "TOSITP4"
#endif

#if defined (any_hpux)
#define OSI_DEVNAME "/dev/ositpi"
#endif

#ifndef OSI_DEVNAME
	#error "OSI_DEVNAME not defined for this platform!"
#endif



/*
** Convert ASCII hex digit to binary
*/
static int
HexDigitToBinary(hexdigit)
char      hexdigit ;
{
	int bindigit ;

	if (hexdigit >= '0' && hexdigit <= '9')
	{
	     bindigit = hexdigit - '0';
	} else if (hexdigit >= 'A' && hexdigit <= 'F')
	{
	     bindigit = (hexdigit - 'A') + 10;
	} else if (hexdigit >= 'a' && hexdigit <= 'f')
	{
	     bindigit = (hexdigit - 'a') + 10;
	} else
	{
	     bindigit = -1 ;     /* illegal hex digit */
	}
	return bindigit ;
}

/*
** convert ASCII strings into hex octets 
*/
static	STATUS
HexToBinary(hexstring, binstring, length)
char	      *hexstring;
unsigned char *binstring ;
int	      length ;        
{
	int   i ; 

	/*  length must be divisible by 2 */
	if ( (length & 1) || (length > MAX_NSAP_SIZE) )
		return FAIL ;

	for (i = 0; i < length; i++) 
	{
	    int  binequiv ;

	    if ( (binequiv = HexDigitToBinary(hexstring[2*i])) == -1)
		return FAIL ;

	    /* put in the first octet */	
	    binstring[i] = (unsigned char) (binequiv << 4) ;

	    if ( (binequiv = HexDigitToBinary(hexstring[2*i +1])) == -1) 
		return FAIL ;

	    /* put in the second octet */
	    binstring[i] =  (unsigned char )((binstring[i] | binequiv));
	}
	return OK ;
}

/*
** convert hex octects into ASCII string
*/
static	STATUS
BinaryToHex(binstring, length, hexstring)
unsigned char	*binstring ;
int	length ;
char	*hexstring ;
{
	int i ;

	for (i = 0; i < length ; i++)
	{
	    int   bin1, bin2 ;

	    bin1 = binstring[i] & 0xf0 ;
	    bin1 = bin1 >> 4;
	    bin2 = binstring[i] & 0x0f ;

	    if (bin1 >= 0 && bin1 <= 9)
	     hexstring[2 * i] = (char)(bin1 + '0');
	    else
	     hexstring[2 * i] = (char)(bin1 + 'a' - 10);

	    if (bin2 >= 0 && bin2 <= 9)
	     hexstring[2*i +1] = (char)(bin2 + '0');
	    else
	     hexstring[2*i +1] = (char)(bin2 + 'a' - 10);
	}
	hexstring[2*length] = 0; 
	return OK ;
}
/*
** OSI "portmapper" for listen - allow for programmatic perturbation of 
** endpoint name. Not visible externally (except through driver struct).
**
** Inputs:
**	pin - string of form "XY", "XYn", or "nnnnnn"
**	sub - 0 or 1-7
** Outputs:
**	pout - string representation of port.
** Restrictions:
**	Output string must be less that 
*/
static STATUS
osi_portmap( pin, sub, pout )
char	*pin;
i4	sub;
char	*pout;
{
#if defined(dr6_us5) || defined(any_hpux) || \
    defined(usl_us5) || defined(any_aix)
if(sub==0)
	{
	STcopy(pin, pout);
	return OK;
	}
if(sub < 8)
	{
	STprintf(pout, "%s%d", pin, sub);
	return OK;
	}
#endif
}

/*
** Name: 
**	addr_to_tsap - convert address in node::port form to 
**	"A representation of a tsap" (X/Open Transport Interface
**	guide) suitable for passing to t_bind()
**
**	Currently, XTI doesn't guarantee programmatic generation
**	of addresses - if a name service is provided, we can use that,
**	else we have to do lookup through some file based mechanism.
**
** Inputs:
**      buf             string form of address
**	tsap_addr	a  preallocated buffer of maxlen
**	maxlen		length of tsap_addr
**
** Outputs:
**	tsap_addr_len	a pointer to the length of the TSAP
**
** Returns:
**	OK if address lookup succeeded, FAIL otherwise.
**
** Side effects:
**	tsap_addr overwritten by osi address
**	*addr_len  set to actual length of TSAP
*/

static STATUS
addr_to_tsap( addr, tsap_addr, tsap_addr_len, maxlen )
char 	*addr, *tsap_addr;
int	*tsap_addr_len ;
long	maxlen ; 
{
#if defined(dr6_us5) || defined (usl_us5)
#define GOT_ONE
	char	*colon;
	if (colon = STchr(addr, ':'))
		*colon = EOS ;

	STncpy( tsap_addr, addr, maxlen );
	tsap_addr[ maxlen ] = EOS;
	*tsap_addr_len = STlength(addr);
	return OK;
#endif /* dr6_us5, usl_us5 */

/*
** It appears that for the current release for HP and IBM RS6000
** we will have to do lookup through a flat file. HP internally 
** recommends that we wait till HP/UX 9.0 (beta in April)
*/
#if defined (any_aix)
#define GOT_ONE
	struct rs_addr_buf *addr_buf = (struct rs_addr_buf *)tsap_addr ;
	char	*nsap_addr, *tsel ;
	int	nsap_len, tsel_len ;
	
	/*
	** Gautam: @@@fixme - this is where the lookup in a flat file
	** may take place - (lookup for NSAP addresses from hostnames)
	*/
	nsap_addr = addr ;
	if ( (tsel = STchr(addr, ':')) == NULL)
	{
# ifdef OSIDEBUG
		OSITRACE(1)("Invalid TSAP addr %s no ':'\n", addr);
# endif
		return FAIL ;
	}
	tsel++ ; /* skip over the '::' */
	tsel++ ;
	
	/* sanity check on tsel size */
	if (STlength(tsel) > MAX_TSEL_LEN)
	    return FAIL ;
	addr_buf->tsel_len =  (short)STlength(tsel);
	STcopy( tsel, addr_buf->tsel);

	if ((tsel - nsap_addr - 2) % 2 )
	{
# ifdef OSIDEBUG
		OSITRACE(1)("Invalid length for NSAP addr\n" );
# endif
		return FAIL ;
	}

	addr_buf->nsel_len = (tsel - nsap_addr - 2)/2;
	/*
	** Use utility functions for ASCII - binary conversion
	*/
	if (HexToBinary(nsap_addr, addr_buf->nsel, (int)addr_buf->nsel_len) != OK)
	{
# ifdef OSIDEBUG
		OSITRACE(1)("Unable to get NSAP  address\n");
# endif
		return FAIL ;
	}
	*tsap_addr_len = TSAP_SIZE ; 

	return OK;
#endif

#if defined (any_hpux)
#define GOT_ONE
	unsigned char    *addr_buf = (unsigned char *)tsap_addr ;
	char	*nsap_addr, *tsel ;
	unsigned char 	nsap_len, tsel_len ;
	
	/*
	** Gautam: @@@fixme - this is where the lookup in a flat file
	** may take place - (lookup for NSAP addresses from hostnames)
	*/

	/*
	** HP can t_bind() to a tsel and does'nt need the NSAP
	** See if hostname is passed to us.
	*/
	if ( (tsel = STchr(addr, ':')) == NULL)
	{
	    /* sanity check on tsel size */
	    if (STlength(addr) > MAX_TSEL_LEN)
		return FAIL ;

	    tsel_len = STlength(addr);	
	    addr_buf[0] = tsel_len ;
	    STncpy( addr_buf+1, addr, tsel_len);
	    addr_buf[ tsel_len + 1 ] = EOS;

	    /* one byte for tsel_len + one byte for pad */
	    *tsap_addr_len = tsel_len + 2 ; 
	    return OK ;
	}
	else
	{
	    nsap_addr = addr ;

	    /* both the NSAP and the tsel are passed in */
	    tsel++ ; /* skip over the ':' */
	    tsel++ ; 
	
	    /* sanity check on tsel size */
	    if ((tsel_len = (unsigned char)STlength(tsel)) > MAX_TSEL_LEN)
		return FAIL ;

	    addr_buf[0]  =  tsel_len ;
	    STncpy( addr_buf + 1, tsel, tsel_len);
	    addr_buf[ tsel_len + 1 ] = EOS;

	    if ((tsel - nsap_addr - 2) % 2 )
	    {
# ifdef OSIDEBUG
		OSITRACE(1)("Invalid length for NSAP addr\n" );
# endif
		return FAIL ;
	    }

	    nsap_len = (tsel - nsap_addr - 2)/2;
	    addr_buf[tsel_len + 1]  =  nsap_len ;

	    /*
	    ** Use utility functions for ASCII - binary conversion
	    */
	    if (HexToBinary(nsap_addr, addr_buf + tsel_len + 2,
			nsap_len) != OK)
	    {
# ifdef OSIDEBUG
		OSITRACE(1)("Unable to get NSAP  address\n");
# endif
		return FAIL ;
	    }
	    addr_buf[tsel_len + nsap_len + 2 ]  = 0; 
	    *tsap_addr_len = tsel_len + nsap_len + 3 ;
	    return OK;
	}
#endif /* hp8_us5 */

#if defined(GOT_ONE)
#undef GOT_ONE
#else
return FAIL;
#endif
}

/*
** Name: 
**	tsap_to_addr - convert OSI tsap to printable representation
**
** Inputs:
**	tsap		a pointer to a tsap
**	tsap_len        length of the tsap
**	addr_len        length of the output buffer - addr
**
** Outputs:
**      addr            buffer which contains printable form of osi addres
**
** Returns:
**	OK if conversion succeeded, FAIL otherwise.
**
*/

static STATUS
tsap_to_addr( tsap, tsap_len, addr, addrlen)
char 	*tsap;
int 	tsap_len ;
char 	*addr;
int 	addrlen ;
{
#if defined(dr6_us5) || defined (usl_us5)
#define GOT_ONE
	STncpy( addr, tsap,
	    tsap_len < addrlen ? tsap_len : addrlen-1 ); 
	addr[ addrlen-1 ] = EOS;
	return OK;
#endif     /* dr6_us5 || usl_us5 */

#if defined(any_aix)
#define GOT_ONE
	struct rs_addr_buf *addr_buf = (struct rs_addr_buf *)tsap ;
	char	nsap_ascii[64];
	char	*nsap_addr, *tsel ;
	int	nsap_len, tsel_len ;
	
	nsap_addr = addr_buf->nsel ;
	nsap_len = addr_buf->nsel_len ;

	if (BinaryToHex(nsap_addr,  nsap_len, nsap_ascii) != OK)
	{
# ifdef OSIDEBUG
	    OSITRACE(1)("tsap_to_addr failure\n") ;
# endif
	    return FAIL ;
	}
	if (addrlen < (STlength(nsap_ascii) + STlength(addr_buf->tsel) + 3))
	{
# ifdef OSIDEBUG
	    OSITRACE(1)("tsap_to_addr: addrbuf not big enough\n") ;
# endif
	    return FAIL ;
	}
	STprintf(addr,"%s:%s\n", nsap_ascii, addr_buf->tsel);
# ifdef OSIDEBUG
	OSITRACE(1)("tsap_to_addr returns %s\n", addr);
# endif
	return OK;

#endif  /* ris_us5 rs4_us5 */

#if defined(any_hpux)
#define GOT_ONE
	unsigned char  *addr_buf = (unsigned char *)tsap ;
	char	nsap_ascii[64] , tsel_ascii[64] ;
	char	*nsap_addr, *tsel ;
	unsigned char  nsap_len, tsel_len ;
	
	tsel_len = addr_buf[0] ;
	nsap_len = addr_buf[tsel_len + 1];

	if (nsap_len == 0)
	{
	    /* only the tsel is passed in - just copy it into addr */
	    STncpy( addr, addr_buf + 1, tsel_len); 
	    addr[ tsel_len - 1 ] = EOS;
	    return OK ;
	}
	STncpy( tsel_ascii, addr_buf + 1, tsel_len); 
	tsel_ascii[ tsel_len ] = EOS;

	nsap_addr = addr_buf + tsel_len + 2 ;

	if (BinaryToHex(nsap_addr, nsap_len, nsap_ascii) != OK)
	{
# ifdef OSIDEBUG
	    OSITRACE(1)("tsap_to_addr failure\n") ;
# endif
	    return FAIL ;
	}

	STprintf(addr,"%s:%s\n", nsap_ascii, tsel_ascii);
# ifdef OSIDEBUG
	OSITRACE(1)("tsap_to_addr returns %s\n", addr);
# endif
	return OK ;

#endif  /* hp8_us5 */

#if defined(GOT_ONE)
#undef GOT_ONE
#else
return FAIL;
#endif
}


/*
** BS entry points
*/

/*
** Name: osi_listen - establish an endpoint for accepting connections
*/

static VOID
osi_listen( bsp )
BS_PARMS	*bsp;
{
	LBCB	*lbcb = (LBCB *)bsp->lbcb;
	char	*addr ;          /* holds the TSAP */
	int	addr_len ;       /* length of TSAP */
	i4	i;
	char	*fn = "osi_listen";

	/* pick up tracing */

# ifdef OSIDEBUG
	{
	char    *trace;
	NMgtAt( "II_OSI_TRACE", &trace );
	if ( !( trace && *trace ) && PMget("!.osi_trace_level", &trace) != OK )
		osi_debug = 0;
	else
		osi_debug = atoi(trace);
	}
# endif /* OSIDEBUG */

	/* clear fd */

	lbcb->fd = -1;

	STcopy( OSI_DEVNAME, lbcb->device );

	/*
	** Initialize Listen Q length to 0.  Let tli_listen() figure it
	** out.
	*/
	lbcb->q_max = 0;

	/* setup known flags */
# if  defined(any_hpux)
	lbcb->flags |= NO_ANON_TBIND ;
# endif

	/* malloc up a buffer for our tsap */
	lbcb->maxaddr = MAX_TSAP_SIZE ;
	addr = (char *)MEreqmem( 0, lbcb->maxaddr, TRUE, (STATUS *) 0 );

	if( !addr)
	{
	    tli_error( bsp, BS_INTERNAL_ERR, -1, ER_t_alloc, fn );
	    return;
	}

	/* convert the symbolic address to a tsap */
	if (addr_to_tsap( bsp->buf, addr, &addr_len, lbcb->maxaddr ) < 0)
	{
		tli_error( bsp, BS_BADADDR_ERR, -1, 0, fn);
		return;
	}
# ifdef OSIDEBUG
	OSITRACE(1)("osi_listen: addr_to_tsap: addr_len = %d\n", addr_len);
# endif

	/* do tli listen */

	tli_listen( bsp, addr, addr_len, lbcb->maxaddr, FALSE );
	MEfree( (PTR)addr );

	if( bsp->status != OK )
	    return;

	if (tsap_to_addr( lbcb->addrbuf, lbcb->addrlen, 
		lbcb->port, MAXPORTNAME) != OK)
	{
#ifdef OSIDEBUG
	    OSITRACE(1)("osi_listen: tsap_to_addr failure\n") ;
#endif
	    bsp->status = BS_LISTEN_ERR ;
	    return ; 
	}
	bsp->buf = lbcb->port;

	/* return results OK */
}
	

/*
** Name: osi_request - make an outgoing connection
*/

static VOID
osi_request( bsp )
BS_PARMS	*bsp;
{
	BCB	*bcb = (BCB *)bsp->bcb;
	LBCB    *lbcb = (LBCB *)bsp->lbcb;
	char	*serv_addr, *local_addr ;
	int	serv_addrlen, local_addrlen;  
	i4	fd;
	char    *fn = "osi_request";

	if (lbcb->device[0] == '\0')
	{
	    /* 
	    ** not initialized - called from gcctest 
	    */
	    STcopy( OSI_DEVNAME, lbcb->device );
	    lbcb->maxaddr = MAX_TSAP_SIZE ;
	    lbcb->addrlen = 0 ;
	}
	
	serv_addr = local_addr = NULL;
	serv_addrlen = local_addrlen = 0;  

	/* clear fd in bcb */

	bcb->fd = -1;

	serv_addr = (char *)MEreqmem( 0, lbcb->maxaddr, TRUE, (STATUS *) 0 );
	local_addr = (char *)MEreqmem( 0, lbcb->maxaddr, TRUE, (STATUS *) 0 );

	if( !serv_addr || !local_addr )
	{
	    tli_error( bsp, BS_INTERNAL_ERR, -1, ER_t_alloc, fn );
	    return;
	}

	/* 
	** Convert local and server addresses into TSAP's
	*/
	if (addr_to_tsap( bsp->buf, serv_addr, &serv_addrlen, lbcb->maxaddr))
	{
		tli_error( bsp, BS_BADADDR_ERR, -1, 0, fn );
		return;
	}
# ifdef OSIDEBUG
	OSITRACE(1)("osi_request: addr_to_tsap:%s  serv_addr_len = %d\n",
		bsp->buf, serv_addrlen);
# endif

#if defined(dr6_us5) || defined (usl_us5)
	/*
	** ICL boxes can evidently bind to the remote address with
	** automatic address generation.
	*/
	if (addr_to_tsap( bsp->buf, local_addr, &local_addrlen, lbcb->maxaddr))
	{
		tli_error( bsp, BS_BADADDR_ERR, -1, 0, fn );
		return;
	}
# endif

# if defined(any_aix)
	/*
	** On the RS/6000, automatic generation of addresses does'nt
	** take place. Do not bind to connecting port ( which is bsp->buf). 
	** For now, anonymous bind seems to succeed.
	** If it does'nt, we need a way to figure out our NSAP somehow
	** (probably environmental variable )
	*/
	{
	   local_addrlen = 0 ; /* ensures anonymous bind in tli_request */
	}
# endif 
# if defined(any_hpux) 
	{
	    /*
	    ** On HP-UX, you don't need a NSAP to bind to. Just pass in
	    ** our pid and bind to it.
	    */
	    char addr_buf[128], *s ;
	    int  addrlen  ;

	    STprintf(addr_buf,"%d", getpid());
	    addrlen = STlength(addr_buf);

	    if (addr_to_tsap( addr_buf, local_addr, &local_addrlen, sizeof(addr_buf)) != OK)
	    {
		tli_error( bsp, BS_BADADDR_ERR, -1, 0, fn );
		return;
	    }
	}
# endif
					  
	tli_request( bsp, lbcb->device, serv_addr, serv_addrlen, 
			local_addr, local_addrlen, lbcb->maxaddr);

	/* free up the t_call and the addr.buf */

	MEfree((PTR)serv_addr);
	MEfree((PTR)local_addr);

	/* return status in bsp->status */
}

/*
** Name: osi_accept - accept a single connect request.
**
** Inputs:
**      bsp             parameters for BS interface. 
**
** Outputs:
**	bsp->is_remote gets set to TRUE.
**
** Returns:
**	void.
**
** Side effects:
**	bsp gets modified by tli_accept().
**
** History:
**      19-nov-2002 (loera01)
**          Created.
*/
static VOID
osi_accept( bsp )
BS_PARMS        *bsp;
{
   bsp->is_remote = TRUE;
   tli_accept (bsp);
}

/*
** Exported driver specification.
*/

BS_DRIVER BS_tliosi = {
	sizeof( BCB ),
	sizeof( LBCB ),
	osi_listen,
	tli_unlisten,
	osi_accept,
	osi_request,
	tli_connect,
	tli_send,
	tli_receive,
#if defined(usl_us5) || defined(dr6_us5)
	0,		/* don't have orderly release, don't use tli_release */
#else
	tli_release,
#endif /* usl_us5, dr6_us5 */
	tli_close,
	tli_regfd,
	0,
	0,
	osi_portmap,	
} ;

# endif /* xCL_TLI_{OSLAN,X25}_EXISTS */

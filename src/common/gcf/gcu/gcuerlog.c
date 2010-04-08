/*
** Copyright (c) 1989, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <cm.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcu.h>

/*
** Name: gcuerlog.c - GCN,GCC interface to ERlookup, ERlog
**
** Description:
**	gcu_erinit()	- set hostname/server id for logging
**	gcu_erlog() 	- log an error to errlog.log
**	gcu_msglog()	- log a message string to errlog.log
**	gcu_erfmt()	- format an error message in a buffer
**
** 	All alot like ule_format(), but doesn't require SCF for the
**	language code.
**
** History:
**	17-Jul-89 (seiwald)
**	     Written.
**	16-Oct-89 (seiwald)
**	     Ifdef'ed GCF63 call to ERsend.  -DGCF63 to get 63 behavior.
**	     Ifdef'ed GCF62 call to ERlookup.  -DGCF62 -DGCF63 to get 62 
**	     behavior.
**	06-Aug-90 (anton)
**	    Changed GCF63 into xORANGE
**	12-Sep-90 (jorge)
**	    Changed ERsend() xORANGE #define to GCF63/GCF62, new ERsend()
**	    is part of GCF65
**	18-Sep-90 (jorge)
**	    Changed again to use GCF64 rather than GCF63/GCF62 defines.
**	    Required since ERsend() change was in 6.4, as compared to
**	    6.5 (the current development release). Set -DGCF64 or earlier
**	    to get 64 equivalent ERsend() usage.
**	5-Dec-90 (seiwald)
**	    Separate generic error and system error messages with a 
**	    space, not a newline.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	2-Sep-92 (seiwald)
**	    Pulled and renamed from gcnerlog.c.
**	15-Oct-92 (seiwald)
**	    Regrooved GCF64 ifdef's.
**	    Send mainline status to TRdisplay.
**      23-Mar-93 (smc)
**          Fixed forward declaration of gcu_erlook() to be static.
**	09-Nov-93 (seiwald)
**	   TRdisplay() the CL error status.
**	30-Nov-95 (gordy)
**	   Add initialization flag and only log errors if initialized.
**	24-par-96 (chech02, lawst01)
**	    cast %d arguments to i4  in TRdisplay for windows 3.1 port.
**	13-May-96 (gordy)
**	    Change ER_ARGUMENT parameter from array to pointer to quiet
**	    errors/warnings do to prototyping.
**	11-Mar-97 (gordy)
**	    Added gcu.h for utility function declarations.  Simplified
**	    the parameters types to reduce dependency on other header
**	    files.
**	 5-Aug-97 (gordy)
**	    Added format to gcu_erinit() when no address provided.
**	20-Aug-97 (gordy)
**	    Added gcu_msglog().  Use ERlog() rather than ERsend().
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	09-jun-1999 (somsa01)
**	    Since we are using STlcopy() to copy in the hostname, add an
**	    extra place for the null indicator to gca_er_host.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-oct-2000 (somsa01)
**	    The max size of the server_id has been increased to 18.
**	15-feb-2001 (somsa01)
**	    Changed printout of message information for LP64 machines.
**	16-mar-2005 (mutma03)
**	    Translate nodename to nickname
**	07-jan-2008 (joea)
**	    Reverse 16-mar-2005 nickname change.
*/

static VOID gcu_erlook( char **, char *, i4, bool, 
			STATUS, CL_ERR_DESC *, i4, PTR );

static char gca_er_initialized = FALSE;
static char gca_er_host[ 18 + 1 ];
static char gca_er_servid[ 8 + 9 + 1 + 1 ];  /* see STprintf, 20 lines down */
static char gca_er_pid[ 10 ]; /* Process ID */


/*
** Name: gcu_erinit() - set host name and server id for logging
**
** Inputs:
**	host	- host name for logging, copied in
**	server	- server type for logging, copied in
**	addr	- server addr for logging, copied in
**
** History:
**	17-Jul-89 (seiwald)
**	    Written.
**	30-Nov-95 (gordy)
**	    Flag initialization.
**	 5-Aug-97 (gordy)
**	    New server ID format when server address not provided.
**      12-Sep-2000 (hanal04)
**          STlcopy will tag a '\0' onto the gca_er_host. Pass size
**          of gca_er_host minus 1 to prevent memory corruption.
**	06-oct-2000 (somsa01)
**	    The max size of the server_id has been increased to 18.
**      16-mar-2005 (mutma03)
**	    Translate hostname to nickname for linux clusters
**       2-May-2007 (hanal04) SIR 118196
**          Fix compile problems on Windows. Previous change missed
**          variable declaration that was grouped with code instead of other
**          declarations.
**	07-jan-2008 (joea)
**	    Reverse 16-mar-2005 nickname change.
*/

VOID
gcu_erinit( char *host, char *server, char *addr )
{
    PID proc_id;
    char fmt[20];
    
    PCpid(&proc_id);

    if ( ! addr  ||  ! *addr )
	STprintf( gca_er_servid, "%-18s", server );
    else  if( CMdigit( addr ) )
	STprintf( gca_er_servid, "%-9s %8s", addr, server );
    else
	STprintf( gca_er_servid, "%-18s", addr );

    STncpy( gca_er_host, host, (sizeof( gca_er_host ) -1) );

    STprintf( fmt, "%s", PIDFMT );
    STprintf( gca_er_pid, fmt, proc_id );

    gca_er_initialized = TRUE;

    return;
}


/*
** Name: gcu_erlog() - log an error to errlog.log
**
** Inputs:
**	sessid	- session id, can be anything
**	lang	- language code of client
**	status	- error status to log
**	clerror	- CL_ERR_DESC for CL errors
**	l_eargs	- number of elements in eargs
**	eargs	- array of ER_ARGUMENT parameters
**
** History:
**	17-Jul-89 (seiwald)
**	    Written.
**	06-Aug-90 (anton)
**	    Changed GCF63 into xORANGE
**	12-Sep-90 (jorge)
**	    Changed ERsend() xORANGE #define to GCF63/GCF62, new ERsend()
**	    is part of GCF65
**	18-Sep-90 (jorge)
**	    Changed again to use GCF64 rather than GCF63/GCF62 defines.
**	    Required since ERsend() change was in 6.4, as compared to
**	    6.5 (the current development release).
**	08-sep-93 (swm)
**	    Changed sessid type from i4  to CS_SID to match recent CL
**	    interface revision. Include cs.h to pickup CS_SID definition.
**	19-sep-1995 (canor01)
**	    Print 8 characters of machine name instead of 6, to match
**	    error logging from dbms server.
**	30-Nov-95 (gordy)
**	    Log error only if initialized.
**	06-oct-2000 (somsa01)
**	    The max size of the server_id has been increased to 18.
**      26-Sep-2007 (wonca01) Bug 65038
**          Allow more characters of the hostname to be printed.
*/

VOID
gcu_erlog
( 
    SIZE_TYPE	sessid, 
    i4		lang, 
    STATUS	status, 
    CL_ERR_DESC	*clerror, 
    i4		l_eargs, 
    PTR		eargs 
)
{
    char		buf[ ER_MAX_LEN ];
    char		*bp0, *bp1, *bp2;
    CL_ERR_DESC	junk;
    if ( gca_er_initialized )
    {
#ifdef LP64
	STprintf( buf, "%-18.18s::[%-18.18s, %-10s, %016.16x]: ",
		gca_er_host, gca_er_servid, gca_er_pid, sessid );
#else
	STprintf( buf, "%-18.18s::[%-18.18s, %-10s, %08.8x]: ",
		gca_er_host, gca_er_servid, gca_er_pid, sessid );
#endif

	bp0 = bp1 = bp2 = buf + STlength( buf );

	if( !CLERROR( status ) || !clerror )
	{
	    gcu_erlook( &bp1, buf + sizeof( buf ), lang, TRUE,
			status, NULL, l_eargs, eargs );

	    TRdisplay("%t\n", (i4)(bp1 - buf), buf );

	    ERlog( buf, (i4)(bp1 - buf), &junk );
	} 
	else 
	{
	    gcu_erlook( &bp1, buf + sizeof( buf ), lang, TRUE,
			status, clerror, 0, NULL );

	    TRdisplay("%t\n", (i4)(bp1 - buf), buf );

	    ERlog( buf, (i4)(bp1 - buf), &junk );

	    gcu_erlook( &bp2, buf + sizeof( buf ), lang, FALSE,
			0, clerror, 0, NULL );

	    if( bp2 != bp0 )
	    {
		TRdisplay("%t\n", (i4)(bp2 - buf), buf );
		ERlog( buf, (i4)(bp2 - buf), &junk );
	    }
	}
    }

    return;
}


/*
** Name: gcu_msglog
**
** Description:
**	Send a message string to the error log.
**
** Input:
**	sessid		Session id.
**	message		Message string.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	12-Aug-97 (gordy)
**	    Created.
**	06-oct-2000 (somsa01)
**	    The max size of the server_id has been increased to 18.
*/

VOID
gcu_msglog( SIZE_TYPE  sessid, char *message )
{
    CL_ERR_DESC	junk;
    char	buf[ ER_MAX_LEN ];

    if ( gca_er_initialized )
    {
#ifdef LP64
	STprintf( buf, "%-8.8s::[%-18.18s, %-10s, %016.16x]: %s",
		gca_er_host, gca_er_servid, gca_er_pid, sessid, message );
#else
	STprintf( buf, "%-8.8s::[%-18.18s, %-10s, %08.8x]: %s",
		gca_er_host, gca_er_servid, gca_er_pid, sessid, message );
#endif

	TRdisplay("%s\n", buf );
	ERlog( buf, (i4)STlength( buf ), &junk );
    }

    return;
}


/*
** Name: gcu_erfmt() - format an error into a buffer
**
** Inputs:
**	buf	- buffer into which message is placed
**	ebuf	- pointer to end of buffer + 1 (= buf + length of buf)
**	lang	- language code of client
**	status	- error status to log
**	clerror	- CL_ERR_DESC for CL errors
**	l_eargs	- number of elements in eargs
**	eargs	- array of ER_ARGUMENT's for parameters within the 
**		  error message
**
** History:
**	17-Jul-89 (seiwald)
**	    Written.
*/

VOID
gcu_erfmt
( 
    char	**buf, 
    char	*ebuf, 
    i4		lang, 
    STATUS	status, 
    CL_ERR_DESC	*clerror, 
    i4		l_eargs, 
    PTR		eargs 
)
{
    if ( ! CLERROR( status ) || ! clerror )
    {
	gcu_erlook( buf, ebuf, lang, FALSE, status, NULL, l_eargs, eargs );
	if ( *buf == ebuf )  return;
	*(*buf)++ = '\n';
    } 
    else 
    {
	gcu_erlook( buf, ebuf, lang, FALSE, status, clerror, 0, NULL );
	if ( *buf == ebuf )  return;
	*(*buf)++ = ' ';

	gcu_erlook( buf, ebuf, lang, FALSE, 0, clerror, 0, NULL );
	if ( *buf == ebuf )  return;
	*(*buf)++ = '\n';
    }

    return;
}


/*
** Name: gcu_erlook() - Lookup an error with erlook
**
** Inputs:
**	buf	- buffer into which message is placed
**	ebuf	- pointer to end of buffer + 1 (= buf + length of buf)
**	lang	- language code of client
**	stamp	- TRUE means tell ERlookup to bury a timestamp in the buffer
**	status	- error status to log
**	clerror	- CL_ERR_DESC for CL errors
**	l_eargs	- number of elements in eargs
**	eargs	- array of ER_ARGUMENT's for parameters within the 
**		  error message
**
** History:
**	17-Jul-89 (seiwald)
**	    Written.
**	13-May-96 (gordy)
**	    Change ER_ARGUMENT parameter from array to pointer to quiet
**	    errors/warnings do to prototyping.
*/

static VOID
gcu_erlook
( 
    char	**buf, 
    char	*ebuf, 
    i4		lang, 
    bool	stamp, 
    STATUS	status, 
    CL_ERR_DESC	*clerror, 
    i4		l_eargs, 
    PTR		eargs 
)
{
    STATUS	erstat;
    i4	used;
    CL_ERR_DESC	erclerror;

    /* 
    ** Try to look up message. 
    ** If look up of message fails, look up failure code. 
    ** If that fails, just dump both failure codes.
    */

    if( ( erstat = ERlookup (
		status, clerror, 
		stamp ? ER_TIMESTAMP : 0, 
# ifndef GCF62
		(i4 *)0,
# endif /* GCF62 */
		*buf, (i4)(ebuf - *buf), 
		lang, &used, &erclerror, 
		l_eargs, (ER_ARGUMENT *)eargs ) ) == OK )
    {
	*buf += used;
	**buf = 0;
    } 
    else if( ERlookup (
		erstat, &erclerror, 
		stamp ? ER_TIMESTAMP : 0,
# ifndef GCF62
		(i4 *)0,
# endif /* GCF62 */
		*buf, (i4)(ebuf - *buf), 
		lang, &used, &erclerror, 
		0, (ER_ARGUMENT *)eargs ) == OK )
    {
        *buf += used;
        STprintf( *buf, " - couldn't look up message %x\n", status );
	*buf += STlength( *buf );
    }
    else
    {
	STprintf( *buf, "Couldn't look up message %x (ER error %x)\n",
		status, erstat );
	*buf += STlength( *buf );
    } 
}

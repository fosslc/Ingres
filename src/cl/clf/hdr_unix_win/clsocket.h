/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** CLSOCKET.H -- include all socket related files.  
**
**
** History:
**	    Created 18-Apr-89 (markd)
**	    Moved socket.h inclusion to this file.
**	25-Jan-90 (seiwald)
**	    Include tcp.h.
**	17-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Add check for xCL_066_UNIX_DOMAIN_SOCKETS to determine
**		whether to include un.h;
**		Pull socket headers from ucb universe for pyr_us5;
**		Add xCL_042_TLI_EXISTS so this stuff gets included for
**		machines which support TLI;
**		Don't include tcp.h for dr3_us5;
**		Add cases for 3bx_us5 and arx_us5.
**	25-sep-1991 (mikem) integrated following change: 18-aug-91 (ketan)
**	    Added socket headers for nc5_us5.
**	06-apr-1993 (mikem)
**	    Changed xCL_042_TLI_EXISTS to xCL_TLI_TCP_EXISTS, as the code no
**	    longer uses the former define.  The code did not compile correctly
**	    on clients with xCL_019_TCPSOCKETS_EXIST not defined and 
**	    xCL_TLI_TCP_EXISTS defined previously.
**  30-apr-1997 (toumi01)
**      Redefine STATUS for axp_osf.  This was getting incorrectly
**      set to 0x2 due to a prior definition in                  
**      /usr/include/arpa/nameser.h which gets picked up with the
**      include of netdb.h.  This STATUS conflicted with the STATUS
**      value used internally by Ingres. 
**	10-may-1999 (walro03)
**	    Remove obsolete version string arx_us5, dr3_us5, nc5_us5, sqs_us5,
**	    x3bx_us5.
**	16-jul-1999 (hweho01)
**	    Included inet.h header file for ris_u64 (AIX 64-bit platform). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-mar-1999 (hanch04)
**	    added arpa/inet.h
**	23-jul-2001 (stephenb)
**	    Add support for i64_aix
**      23-Sep-2002 (hweho01)
**          xCL_019_TCPSOCKETS_EXIST is defined in clsecret.h, so no need  
**          to specify include arpa/inet.h file for ris_u64 platform.
*/

# ifndef CLCONFIG_H_INCLUDED
error "You didn't include clconfig.h before including clsocket.h"
# endif
#if defined(xCL_019_TCPSOCKETS_EXIST) || defined(xCL_TLI_TCP_EXISTS)

# ifdef	xCL_040_EXOS_EXISTS
# include	<exos/sys/socket.h>
# include	<exos/netinet/in.h>
# include	<exos/ex_errno.h>
# define EXOS_EXISTS
# define NO_UNIX_DOMAIN
# define GOT_SOCKET_HDRS
# endif /* EXOS_EXISTS */

# ifdef pyr_us5
# include       "/usr/.ucbinclude/sys/socket.h"
# include       "/usr/.ucbinclude/sys/un.h"
# include       "/usr/.ucbinclude/netinet/in.h"
# include       "/usr/.ucbinclude/netdb.h"
# include       "/usr/.ucbinclude/netinet/tcp.h"
# define GOT_SOCKET_HDRS
# endif	/* pyr_us5 */

/* Default */
# ifndef GOT_SOCKET_HDRS
# include	<sys/socket.h>
# ifdef xCL_066_UNIX_DOMAIN_SOCKETS
# include	<sys/un.h>
# endif
# include	<netinet/in.h>
# include	<arpa/inet.h>


/*
** if axp_osf and threads build, system header arpa/nameser.h will define
** STATUS to the nameserver status query value 0x2, which conflicts with
** the Ingres definition of STATUS as a return value of type nat.  Undef
** here to avoid compiler complaints, and redefine below.
*/
# ifdef axp_osf
# ifdef OS_THREADS_USED 
# undef STATUS
# endif /* OS_THREADS_USED */
# endif /* axp_osf */

# include	<netdb.h>

# ifdef axp_osf
# ifdef OS_THREADS_USED 
# undef STATUS
# define STATUS i4
# endif /* OS_THREADS_USED */
# endif /* axp_osf */

# include	<netinet/tcp.h>
# endif /* GOT_SOCKET_HDRS */

# if defined(i64_aix)
# include <arpa/inet.h>
# endif
#endif /* TCPSOCKETS_EXIST TLI_EXISTS */

/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <clconfig.h>
#include    <gl.h>
#include    <qu.h>
#include    <gc.h>
#include    <clpoll.h>
#include    <cs.h>
#include    <ex.h>
#include    <me.h>
#include    <lo.h>
#include    <er.h>
#include    <pm.h>
#include    <nm.h>
#include    <tr.h>
#include    <st.h>
#include    <errno.h>
#include    <diracc.h>
#include    <bsi.h>
#include    "gcarw.h"
#include    "gcacli.h"

# define    MAXLINE 256

FUNC_EXTERN     BS_DRIVER       *GC_get_driver();

/*
** Name: GCpportaddr
**
** Description:
**      Retrieve the port address by using portmap function in BS driver   
**      for the specified protocol.
**
** Inputs:
**      protname        Name of the protocol for which the port is requested.
**      addrlen         Length available in the target portaddr including the
**                      EOS terminator.
**
** Outputs:
**      portaddr        Returned port address.
**
** Returns:
**      OK              Command completed successfully.
**      FAIL
**
** History:
**      27-Jan-2004 (fanra01)
**          Created.
**      09-Mar-2004 (fanra01)
**          Updated to use xx_INSTALATION as pce_port does not have an
**          upated identifier at this point.
**      09-Mar-2004 (hweho01)
**          Port GCpportaddr() to Unix.  
**      06-Oct-09 (rajus01) Bug: 120552, SD issue:129268
**	    When starting more than one GCC server the symbolic ports are not
**	    incremented (Example: II1, II2) in the startup messages, while the
**	    actual portnumber is increasing and printed correctly. So, the 
**	    portmapper routine defined by the network protocol driver takes 
**	    additional parameter for returning the actual symbolic port.
*/
STATUS
GCpportaddr( char* protname, i4 addrlen, char* portaddr, 
				char * port_out_symbolic )
{
    STATUS      status = FAIL;
    BS_DRIVER   *GCdriver; 
    char*       valuestr = NULL;
    char        envstr[MAXLINE], addr[MAXLINE] ;

    
    if (((protname) && (*protname)) && ((portaddr) && (addrlen > 0)))
    {
       GCdriver = GC_get_driver( NULL ); 
       if ( GCdriver != NULL )
        {
          MEfill( addrlen, '\0', portaddr );
          STprintf( envstr, "%s_INSTALLATION", SystemVarPrefix );
          NMgtAt( envstr, &valuestr );
          if ((valuestr != NULL) && (*valuestr))
            {
              /*
              ** Remove subport if defined
              */
              STlcopy( valuestr, addr, 2 );
              if ( (*GCdriver->portmap)( addr, 0, portaddr, port_out_symbolic ) == OK )
                 status = OK; 
            }
       }
    }
       
    return(status);
}


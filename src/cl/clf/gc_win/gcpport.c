/*
** Copyright (c) 1987, 2004 Ingres Corporation
*/

/*
** Name: gcpport
**
** Description:
**      Module returns the likely port address of the named
**      protocol driver.
**
**  History:
**      08-Mar-2004 (fanra01)
**          SIR 111718
**          Created.
**          Moved from gcpinit.c
**      17-Dec-09 (rajus01) Bug: 120552, SD issue:129268
**          When starting more than one GCC server the symbolic ports are not
**          incremented (Example: II1, II2) in the startup messages, while the
**          actual portnumber is increasing and printed correctly. So, the
**          portmapper routine defined by the network protocol driver takes
**          additional parameter for returning the actual symbolic port.
**          The SD issue is specific to Unix platforms. VMS, and Windows
**          display the actual symbolic ports correctly. Added this additional
**          parameter to avoid compilation errors on Windows.

*/
#include <compat.h>
#include <er.h>
#include <cs.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <qu.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <gcccl.h>
#include <gc.h>
#include "gclocal.h"
#include "gcclocal.h"

# define    MAXLINE 256

GLOBALREF 	GCC_PCT		IIGCc_gcc_pct;

/*
** Name: GCpportaddr
**
** Description:
**      Retrieve the port address that is part of the GCC_PCE structure
**      for the specified protocol.
**
** Inputs:
**      protname        Name of the protocol for which the port is requested.
**      addrlen         Length available in the target portaddr including the
**                      EOS terminator.
**
** Outputs:
**      portaddr        Returned port address.
**      port_out_symbolic Returned actual port address.
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
*/
STATUS
GCpportaddr( char* protname, i4 addrlen, char* portaddr, 
					char *port_out_symbolic )
{
    STATUS      status = FAIL;
    i4          i;
    GCC_PCE*    pce;
    WS_DRIVER*  drv;
    struct sockaddr ws_addr;            /* use sockaddr for now but this    */
                                        /* may change depending on protocol */
                                        /* requirements                     */
    GCC_WINSOCK_DRIVER	*wsd;
    THREAD_EVENTS	*Tptr;
    char        addr[GCC_L_PORT];
    i4          entries;
    
    if (((protname) && (*protname)) && ((portaddr) && (addrlen > 0)))
    {
        if ((entries = IIGCc_gcc_pct.pct_n_entries) > 0)
        {
            /*
            ** For each pct_pce entry in the IIGCc_gcc_pct structure,
            ** compare the requested protcol name with the name in the
            ** structure.
            */
            for (i=0, pce=&IIGCc_gcc_pct.pct_pce[0];
                i < entries; i+=1, pce+=1)
            {
                if(STcompare(protname, pce->pce_pid) == 0)
                {
                    /*
                    ** Protocol name matches
                    ** Clear the target buffer and call the driver address
                    ** translation function if there is one.
                    */
                    MEfill( addrlen, 0, portaddr );
                    drv = (WS_DRIVER *)pce->pce_driver; 
                    if ((drv != NULL) && (drv->addr_func != NULL))
                    {
                        char*   valuestr = NULL;
                        char    envstr[MAXLINE];
                        
                        STprintf( envstr, "%s_INSTALLATION", SystemVarPrefix );
                        NMgtAt( envstr, &valuestr );
                        if ((valuestr != NULL) && (*valuestr))
                        {
                            /*
                            ** Remove subport if defined
                            */
                            STlcopy( valuestr, addr, 2 );
                            status = (drv->addr_func)( NULL, addr,
                                &ws_addr, portaddr );
                        }
                    }
                    break;
                }
            }
        }
    }
    return(status);
}


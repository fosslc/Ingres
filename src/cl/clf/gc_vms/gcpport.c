/*
** Copyright (c) 2005 Ingres Corporation
**
*/
#include    <compat.h>

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
**      actual_symbolic_port  Returned actual symbolic port. 
**
** Returns:
**      OK              Command completed successfully.
**      FAIL
**
** History:
**      02-feb-2005 (abbjo03)
**          Created as a stub for VMS.
**      17-Dec-09 (rajus01) Bug: 120552, SD issue:129268 
**          When starting more than one GCC server the symbolic ports are not
**          incremented (Example: II1, II2) in the startup messages, while the
**          actual portnumber is increasing and printed correctly. So, the
**          portmapper routine defined by the network protocol driver takes
**          additional parameter for returning the actual symbolic port.
**          The SD issue is specific to Unix platforms. VMS, and Windows 
**          display the actual symbolic ports correctly. Added this additional
**          parameter to avoid compilation errors on VMS.
*/
STATUS
GCpportaddr(
char	*protname,
i4	addrlen,
char	*portaddr, 
char 	*actual_symbolic_port)
{
    *portaddr = EOS;
    *actual_symbolic_port = EOS;
    return OK;
}

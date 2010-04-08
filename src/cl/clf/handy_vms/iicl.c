/*
**  Copyright (c) 2008 Ingres Corporation
**  All rights reserved.
**
**  History:
**      10-Dec-2008   (stegr01)
**      Port from assembler to C for VMS itanium port
**
**/



#include <pwd.h>

#include <compat.h>

/*
** Name:  iiCLgetpwnam
**
** Description:
**  This wrapper function allows the caller to access the getpwnam
**  function in a reentrant fashion.
**
** Inputs:
**    char* name - username
**    struct passwd* pwd
**    char*  pwnam_buf
**    int size
**
** Outputs:
**    None
**
** Returns:
**     address of (static) password structure (from the C RTL)
**
** Exceptions:
**     None
**
** Side Effects:
**     None
**
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

struct passwd*
iiCLgetpwnam(char *name, struct passwd *pwd, char *pwnam_buf, i4 size)
{

    struct passwd* pwd_result;
    int sts = getpwnam_r(name, pwd, pwnam_buf, size, &pwd_result);
    return (pwd_result);
}

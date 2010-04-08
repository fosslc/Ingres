/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <add.h>
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lk.h>

/*
** ASCDSTUB.C
**	Stubs for functions referenced in Ingres modules that are not
**	needed for the Frontier server.
**
**	Currently the LK functions are referenced in SCE.
**
** History:
**	03-Aug-1998 (fanra01)
**	    Removed internal header files.
**      11-Sep-1998 (fanra01)
**          Update LKevent to match prototype.
**      15-Jan-1999 (fanra01)
**          Remove LK stubs as now linked with DMF on unix.
**          Add udadts stubs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-sep-2000 (mcgem01)
**	    More nat and longnat to i4 changes.
*/
#ifdef NT_GENERIC
STATUS
LKevent(
i4              flag,
LK_LLID         lock_list_id,
LK_EVENT        *event,
CL_ERR_DESC     *sys_err)
{
    return (OK);
}
STATUS
LKrequest(
i4                  flags,
LK_LLID             lock_list_id,
LK_LOCK_KEY         *lock_key,
u_i4               lock_mode,
LK_VALUE            *value,
LK_LKID             *lockid,
i4                  timeout,
CL_ERR_DESC         *sys_err)
{
    return (OK);
}
STATUS
LKrelease(
i4                  flags,
LK_LLID             lock_list_id,
LK_LKID             *lockid,
LK_LOCK_KEY         *lock_key,
LK_VALUE            *value,
CL_ERR_DESC         *sys_err)
{
    return (OK);
}
STATUS
LKcreate_list(
i4                      flags,
LK_LLID                 related_lli,
LK_UNIQUE               *unique_id,
LK_LLID                 *lock_list_id,
i4                      count,
CL_ERR_DESC             *sys_err)
{
    return (OK);
}
#endif /* NT_GENERIC */

int
IIclsadt_register( ADD_DEFINITION  **add_block, ADD_CALLBACKS *callbacks )
{
    return(0);
}

int
IIudadt_register( ADD_DEFINITION  **add_block, ADD_CALLBACKS *callbacks )
{
    return(0);
}

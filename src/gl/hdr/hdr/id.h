/*
**	Copyright (c) 2004 Ingres Corporation
*/

# ifndef ID_HDR_INCLUDED
# define ID_HDR_INCLUDED 1
#include    <idcl.h>

/**CL_SPEC
** Name:	ID.h	- Define ID function externs
**
** Specification:
**
** Description:
**	Contains ID function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of id.h.
**	27-oct-1998 (canor01)
**	    Add prototypes for UUID functions.
**	18-nov-1998 (canor01)
**	    Add IDuuid_from_string.
**	24-oct-2000 (somsa01)
**	    Added IDname_service for NT.
**	30-Mar-2005 (kodse01)
**	    Added prototype for IDsetsys.
**	13-Apr-2005 (hanje04)
**	    BUG 112987
**	    Add prototype for IDename
**      24-Nov-2009 (maspa05) bug 122642
**          A semaphore has been added to handle synch'ing of UUID creation, as
**          well as a last time and last counter value. Pointers to these 
**          are used so that we either point to a local or shared memory 
**          version depending on whether we're a server or not. 
**          Also defined ID_UUID_SEM* since we need to use different semaphore
**          types on VMS than unix
**/

#ifndef IDgroup
FUNC_EXTERN VOID    IDgroup(
	    GID		*gid
);
#endif

FUNC_EXTERN VOID    IDname(
	    char	**name
);

FUNC_EXTERN VOID    IDename(
	    char	**name
);

#ifdef NT_GENERIC
FUNC_EXTERN STATUS  IDname_service(
	    char	**name
);
#endif

#ifndef IDuser
FUNC_EXTERN VOID    IDuser(
	    MID		*uid
);
#endif

# ifndef IDuuid_create
FUNC_EXTERN STATUS  IDuuid_create(
	    UUID	*uuid
);
# endif

# ifndef IDuuid_to_string
FUNC_EXTERN STATUS  IDuuid_to_string(
	    UUID	*uuid,
	    char	*string
);
# endif

# ifndef IDuuid_from_string
FUNC_EXTERN STATUS  IDuuid_from_string(
	    char	*string,
	    UUID	*uuid
);
# endif

# ifndef IDuuid_compare
FUNC_EXTERN STATUS  IDuuid_compare(
	    UUID	*uuid1,
	    UUID	*uuid2
);
# endif

# ifndef IDsetsys
FUNC_EXTERN VOID IDsetsys(
          char *SysLocationVariable,
          char *SysLocationSubdirectory,
          char *SysProductName,
          char *SysDBMSName,
          char *SysVarPrefix,
          char *SysCfgPrefix,
          char *SysCatPrefix,
          char *SysAdminUser,
          char *SysSyscatOwner
);
# endif

#ifdef VMS
#define ID_UUID_SEM_TYPE                     CS_SEMAPHORE
#define ID_UUID_SEM_INIT(mutex,scope,name)   CSw_semaphore(mutex,scope,name)
#define ID_UUID_SEM_LOCK(exclusive,mutex)    CSp_semaphore(exclusive,mutex)
#define ID_UUID_SEM_UNLOCK(mutex)            CSv_semaphore(mutex)
#else
#define ID_UUID_SEM_TYPE                     CS_SYNCH
#define ID_UUID_SEM_INIT(mutex,scope,name)   CS_synch_init(mutex)
#define ID_UUID_SEM_LOCK(exclusive,mutex)    CS_synch_lock(mutex)
#define ID_UUID_SEM_UNLOCK(mutex)            CS_synch_unlock(mutex)
#endif


# endif /* ! ID_HDR_INCLUDED */

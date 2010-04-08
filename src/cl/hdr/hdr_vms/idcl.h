/*
** Copyright (c) 1985, 2005  Ingres Corporation
**
*/

/**CL_SPEC
** Name: ID.H - Global definitions for the ID compatibility library.
**
** Specification:
**
** Description:
**      The file contains the type used by ID and the definition of the
**      ID functions.  These are used to get operating system user
**      identifiers. 
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to codinng standard for 5.0.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	29-oct-99 (kinte01)
**	   27-oct-1998 (canor01)
**          Add definition of uuid_t for those systems that do not have it
**          already defined.
**         07-nov-1998 (canor01)
**          Add RPC_S_OK and RPC_S_UUID_LOCAL_ONLY for those systems that
**          do not have them already defined.
**	28-apr-2005 (raodi01)
**	    Following the UUID change for Bug b110668 which was a
**	    windows-specific one
**	    added changes to handle the change on unix (and now VMS).
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN VOID    IDegroup();
FUNC_EXTERN VOID    IDeuser();
FUNC_EXTERN VOID    IDuseic();
FUNC_EXTERN VOID    IDsuser();
FUNC_EXTERN VOID    IDsgroup();
FUNC_EXTERN VOID    IDsureal();
FUNC_EXTERN VOID    IDsgreal();

/* 
** Defined Constants
*/

/* ID return status codes. */

#define                 ID_OK           0

/* Group and process definitions. */

# define	MID		i4
# define	GID		i4

/*
** defines macros:
** IDegroup - set passed gid to the effective group id of the current process
*/

# define	IDegroup(gid)	
# define	IDsgroup(gid)
# define	IDsgreal()
/*
** defines macros:
** IDeuser - set passed uid to the effective user id of the current process
*/

# define	IDeuser(mid)
# define	IDsuser(mid)
# define	IDsureal()

# ifndef uuid_t /* for systems without built-in UUID support */

typedef struct _GUID {
    u_i4   Data1;
    u_i2   Data2;
    u_i2   Data3;
    u_char Data4[8];
} GUID;

typedef GUID uuid_t;

# endif /* !defined uuid_t */

# ifndef UUID
# define UUID uuid_t
# endif /* UUID */

 /* Common UUID-related error returns that may be checked for */
# ifndef RPC_S_OK
# define RPC_S_OK               OK
# endif /* RPC_S_OK */

# ifndef RPC_S_UUID_LOCAL_ONLY
# define RPC_S_UUID_LOCAL_ONLY  1824L
# endif /* RPC_S_UUID_LOCAL_ONLY */

# define IDuuid_create_seq IDuuid_create

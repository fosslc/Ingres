# include	<compat.h>

/*
 *	Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		ID.h
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		defines global symbols for ID module
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	27-oct-1998 (canor01)
**	    Add definition of uuid_t for those systems that do not have it
** 	    already defined.
**	07-nov-1998 (canor01)
**	    Add RPC_S_OK and RPC_S_UUID_LOCAL_ONLY for those systems that
**	    do not have them already defined.
**	29-Mar-2005 (hanje04)
**	   BUG 112987
**	   Add IDgeteuser(). Similar to IDname but just a wrapper for 
**	   cuserid.
**	13-Apr-2005 (hanje04)
**	   BUG 112987
**	   cuserid() doesn't return the same thing on Solaris as it does
**	   on Linux. Use IDename instead.
**	15-Apr-2005 (hanje04)
**	   BUG 112987
**	   Use IDename on Linux too, cuserid is causing problem using 
**	   usernames over 7 characters. Removed macro, IDename is called
**	   directly 
**      28-apr-2005 (raodi01)
**          Following the UUID change for Bug b110668 which was a 
**	    windows-specific one
**          added changes to handle the change on unix
**	08-Oct-2007 (hanje04)
**	   BUG 114907
**	   The system defined uuid_t bares very little resemblence to that
**	   which is needed so remove the #ifdef uuid_t section and always
**	   define UUID to be GUID.
 *
 */

/* static char	*Sccsid = "@(#)ID.h	4.1  3/5/84"; */

# define	MID		i4
# define	GID		i4

/*
	defines macros:
		 IDgroup	set passed gid to the real group id of the current process
		 IDegroup	set passed gid to the effective group id of the current process
		 IDsgroup	set group id of current process to passed in gid
		 IDsgreal	set group id of current process to its real group id
*/

# define	IDegroup(gid)	*gid = getegid() & MAXI2
# define	IDgroup(gid)	*gid = getgid()  & MAXI2
# define	IDsgroup(gid)	setgid(gid)
# define	IDsgreal()	setgid(getgid())

/*
	defines macros:
		 IDuser		set passed uid to the real user id of the current process
		 IDeuser	set passed uid to the effective user id of the current process
		 IDsuser	set user id of current process to passed in uid
		 IDsureal	set user id of current process to its real user id
		 IDgeteuser	set passed string to effective username.
*/

# define	IDeuser(mid)	*mid = geteuid() & MAXI2
# define	IDuser(mid)	*mid = getuid()  & MAXI2
# define	IDsuser(mid)	setuid(mid)
# define	IDsureal()	setuid(getuid())

typedef struct _GUID {
    u_i4   Data1;
    u_i2   Data2;
    u_i2   Data3;
    u_char Data4[8];
} GUID;

typedef GUID UUID;

/* Common UUID-related error returns that may be checked for */
# ifndef RPC_S_OK
# define RPC_S_OK 		OK
# endif /* RPC_S_OK */

# ifndef RPC_S_UUID_LOCAL_ONLY
# define RPC_S_UUID_LOCAL_ONLY	1824L
# endif /* RPC_S_UUID_LOCAL_ONLY */

# define IDuuid_create_seq IDuuid_create

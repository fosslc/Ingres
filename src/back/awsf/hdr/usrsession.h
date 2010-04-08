/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Corrected duplicate typedefs for USR_PTRANS and USR_PSESSION.
**	    Corrected incomplete last line.
**      03-Jun-1999 (fanra01)
**          Add semaphore to protect access to session variables.
**      17-Aug-1999 (fanra01)
**          Bug 98429
**          Add prototype for WSMGetCookieExpireStr.
**      25-Jan-2000 (fanra01)
**          Bug 100132
**          Add prototype for WSMCookieMaxAge.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add rowset and setsize fields to the transaction structure
**          for handling multirow results.
**          Add defines for their default sizes.
**       05-Nov-2001 (hweho01)
**          Added function prototype for WSMDestroyTransaction(), avoid
**          the compiler error in AIX platform.
*/
#ifndef USR_SESSION_INCLUDED
#define USR_SESSION_INCLUDED

#include <ddfhash.h>
#include <wsf.h>
#include <ddfsem.h>
#include <dbmethod.h>
#include <dbaccess.h>

# define MIN_ROW_SET    256         /* minimum row set number */
# define MAX_SET_SIZE   65536       /* maximum memory use processing results */
# define SET_BLOCK_SIZE 1024

typedef struct __USR_TRANS *USR_PTRANS;
typedef struct __USR_SESSION *USR_PSESSION;

typedef struct __USR_CURSOR 
{
	USR_PTRANS			transaction;
	char*				name;
	DB_QUERY			query;
	struct __USR_CURSOR *previous;
	struct __USR_CURSOR *next;

	struct __USR_CURSOR *cursors_previous;
	struct __USR_CURSOR *cursors_next;
} USR_CURSOR, *USR_PCURSOR;

typedef struct __USR_TRANS 
{
    USR_PSESSION        usr_session;
    char                *trname;
    PDB_CACHED_SESSION  session;
    USR_PCURSOR         cursors;
    i4                  rowset;
    i4                  setsize;
    struct __USR_TRANS  *previous;
    struct __USR_TRANS  *next;

    struct __USR_TRANS  *trans_previous;
    struct __USR_TRANS  *trans_next;
} USR_TRANS;

typedef struct __USR_SESSION 
{
    char*                   name;
    SEMAPHORE               usrvarsem;
    DDFHASHTABLE            vars;
    USR_PTRANS              transactions;

    u_i4                    userid;
    char                    *user;
    char                    *password;

    u_i2                    mem_tag;

    u_i4                    requested_counter;
    u_i4                    timeout_end;
    struct __USR_SESSION    *timeout_previous;
    struct __USR_SESSION    *timeout_next;
} USR_SESSION;

GSTATUS 
WSMRequestUsrSession (
	char		 *cookie,
	USR_PSESSION *session);

GSTATUS
WSMReleaseUsrSession(
	USR_PSESSION usr_session);

GSTATUS
WSMCleanUserTimeout();

GSTATUS 
WSMCreateUsrSession (
	char		*name,
	u_i4		userid, 
	char		*user, 
	char		*password,
	USR_PSESSION *session);

GSTATUS 
WSMDeleteUsrSession ( 
	USR_PSESSION *session);

GSTATUS
WSMBrowseUsers(
	PTR		*cursor,
	USR_PSESSION *session);

GSTATUS 
WSMCreateCursor(
	USR_PTRANS	transaction,
	char		*name,
	char		*statement,
	USR_PCURSOR	*cursor);

GSTATUS 
WSMGetCursor(	
	USR_PTRANS	transaction,
	char		*name,
	USR_PCURSOR	*cursor);

GSTATUS 
WSMBrowseCursors(	
	PTR		*cursor,
	USR_PCURSOR	*object);

GSTATUS
WSMDestroyCursor(
	USR_PCURSOR	cursor,
	u_i4		*count,
	bool	   checkAdd);

GSTATUS 
WSMCreateTransaction(
	USR_PSESSION usr_session, 
	u_i4 dbType, 
	char *name, 
	char *trname,
	char *user,
	char *password,
	USR_PTRANS *usr_dbsession);

GSTATUS 
WSMGetTransaction(	
	USR_PSESSION session, 
	char *name, 
	USR_PTRANS *dbsession);

GSTATUS 
WSMBrowseTransaction(	
	PTR		*cursor,
	USR_PTRANS *trans);

GSTATUS 
WSMDestroyTransaction(
	USR_PTRANS transaction);

GSTATUS 
WSMCommit(
	USR_PTRANS session,
	bool	   checkAdd);

GSTATUS
WSMRollback(
	USR_PTRANS session,
	bool	   checkAdd);

GSTATUS 
WSMRemoveUsrSession ( 
	USR_PSESSION session);

GSTATUS
WSMGetCookieExpireStr( USR_PSESSION usrsess, PTR expire );

GSTATUS
WSMGetCookieMaxAge( USR_PSESSION usrsess, PTR expire );

#endif

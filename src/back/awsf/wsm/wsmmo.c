/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** wsmmo.c -- MO module for wsm
**
** Description
**
**
**
**
** Functions:
**
**      act_define          defines the classes for wsm
**      act_sess_attach     make an instance based on the session name
**      act_sess_detach     remove a ACT_SESSION instance
**      act_session_get     returns the session id for a given wts instance
**      act_context_get     returns the context id for a given wts instance
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**      12-Aug-98 (fanra01)
**          Add required headers.
**      11-Sep-1998 (fanra01)
**          Corrected case for actsession and usrsession to match piccolo.
**      21-Oct-98 (fanra01)
**          Updated to get the query text from a database driver function.
**      04-Mar-1999 (fanra01)
**          Add type to usrSessSemaphore.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <iicommon.h>
# include <erglf.h>
# include <mo.h>
# include <ddfcom.h>
# include <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <adp.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <dudbms.h>
#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0m.h>
#include    <tm.h>
#include    <scfcontrol.h>
#include	<actsession.h>
#include	<usrsession.h>
#include	<dds.h>

STATUS act_define (void);
STATUS act_sess_attach (char * name, ACT_PSESSION  actSession);
VOID act_sess_detach (char * name);

STATUS usr_define (void);
STATUS usr_sess_attach (char * name, USR_PSESSION  usrSession);
VOID usr_sess_detach (char * name);

MO_GET_METHOD usr_trans_owner_get;
MO_GET_METHOD usr_curs_owner_get;
MO_GET_METHOD usr_name_get;
MO_GET_METHOD usr_timeout_get;

GLOBALREF MO_CLASS_DEF act_classes[];
GLOBALREF char act_index_name[];

GLOBALREF MO_CLASS_DEF usr_classes[];
GLOBALREF char usr_index_name[];
GLOBALREF MO_CLASS_DEF usr_trans_classes[];
GLOBALREF char usr_trans_index[];
GLOBALREF MO_CLASS_DEF usr_curs_classes[];
GLOBALREF char usr_curs_index[];

GLOBALREF DDFHASHTABLE usr_sessions;
GLOBALREF SEMAPHORE usrSessSemaphore;

/*
** Name:    act_define
**
** Description:
**      Define the MO class for wsm active sessions.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**
*/
STATUS
act_define (void)
{
    return (MOclassdef (MAXI2, act_classes));
}

/*
** Name: act_sess_attach - make an instance based on the session name.
**
** Description:
**
**      Wrapper for MOattach.  Create an instance of the WTS_SESSION.
**
** Inputs:
**
**      name            name of active session (unique key)
**      actSession      pointer to the active session structure
**
** Outputs:
**
**      none
**
** Returns:
**      OK              function completed successfully
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**
*/
STATUS
act_sess_attach (char * name, ACT_PSESSION  actSession)
{
    return (MOattach (MO_INSTANCE_VAR, act_index_name, name, (PTR)actSession));
}

/*
** Name: act_sess_detach - remove a ACT_SESSION instance
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for active session
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Mar-98 (fanra01)
**          Created.
**
*/
VOID
act_sess_detach (char * name)
{
    MOdetach (act_index_name, name);
}

/*
** Name:    usr_define
**
** Description:
**      Define the MO class for wsm user session.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_define (void)
{
    return (MOclassdef (MAXI2, usr_classes));
}

/*
** Name: usr_sess_attach - make an instance based on the session name.
**
** Description:
**
**      Wrapper for MOattach.  Create an instance of USR_SESSION.
**
** Inputs:
**
**      name            name of user session (unique key)
**      usrSession      pointer to the usrive session structure
**
** Outputs:
**
**      none
**
** Returns:
**      OK              function completed successfully
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_sess_attach (char * name, USR_PSESSION  usrSession)
{
    return (MOattach (MO_INSTANCE_VAR, usr_index_name, name, (PTR)usrSession));
}

/*
** Name: usr_sess_detach - remove a USR_SESSION instance
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for user session
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
VOID
usr_sess_detach (char * name)
{
    MOdetach (usr_index_name, name);
}

/*
** Name:    usr_trans_define
**
** Description:
**      Define the transaction and transaction owner MO classes for wsm.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_trans_define (void)
{
    STATUS status = MOclassdef (MAXI2, usr_trans_classes);

    return (status);
}

/*
** Name: usr_trans_attach - make an instance based on the session name.
**
** Description:
**
**      Wrapper for MOattach.
**
** Inputs:
**
**      name            name of user session (unique key)
**      usrSession      pointer to the usrive session structure
**
** Outputs:
**
**      none
**
** Returns:
**      OK              function completed successfully
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_trans_attach (USR_PTRANS  usrtrans)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)usrtrans, sizeof (buf), buf);

    if (status == OK)
    {
        MOattach (MO_INSTANCE_VAR, usr_trans_index, buf, (PTR)usrtrans);
    }
    return (status);
}

/*
** Name: usr_trans_detach - remove a USR_SESSION instance
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for user session
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
VOID
usr_trans_detach (USR_PTRANS  usrtrans)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)usrtrans, sizeof (buf), buf);

    if (status == OK)
    {
        MOdetach (usr_trans_index, buf);
    }
}

/*
** Name: usr_trans_sess_get
**
** Description:
**      Return the cursor query
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_trans_sess_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                     char * userbuf)
{
    USR_PTRANS  trans = (USR_PTRANS)obj;

    return (MOptrout (MO_VALUE_TRUNCATED, (PTR)trans->session, userbuflen,
            userbuf));
}
/*
** Name: usr_trans_owner_get
**
** Description:
**      Return the name of the owner for the transaction.
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the transaction owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_trans_owner_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                     char * userbuf)
{
    USR_PTRANS   trans= (USR_PTRANS)obj;
    USR_PSESSION sess = trans->usr_session;

    return (MOstrout (MO_VALUE_TRUNCATED, sess->name, userbuflen, userbuf));
}

/*
** Name:    usr_curs_define
**
** Description:
**      Define the MO class for wts.
**
**
** Inputs:
**      None.
**
** Outputs:
**      None.
**
** Returns:
**      OK                  successfully completed
**      MO_NO_MEMORY        MO_MEM_LIMIT exceeded
**      MO_NULL_METHOD      null get/set methods supplied when permission state
**                          they are allowed
**      !OK                 system specific error
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_curs_define (void)
{
    STATUS status = MOclassdef (MAXI2, usr_curs_classes);

    return (status);
}

/*
** Name: usr_curs_attach - make an instance based on the session name.
**
** Description:
**
**      Wrapper for MOattach.
**
** Inputs:
**
**      name            name of user session (unique key)
**      usrSession      pointer to the usrive session structure
**
** Outputs:
**
**      none
**
** Returns:
**      OK              function completed successfully
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_curs_attach (USR_PCURSOR  usrcursor)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)usrcursor, sizeof (buf), buf);

    if (status == OK)
        status = MOattach (MO_INSTANCE_VAR, usr_curs_index, buf,
                           (PTR)usrcursor);
    return (status);
}

/*
** Name: usr_curs_detach
**
** Description:
**
**      Wrapper for MOdetach.
**
** Inputs:
**
**      name            name for user session
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
VOID
usr_curs_detach (USR_PCURSOR  usrcursor)
{
    char buf[80];

    STATUS status = MOptrout (0, (PTR)usrcursor, sizeof (buf), buf);

    if (status == OK)
        MOdetach (usr_curs_index, buf);
}

/*
** Name: usr_curs_owner_get
**
** Description:
**      Return the cursor owner name
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_curs_owner_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                     char * userbuf)
{
    USR_PCURSOR  usrcursor  = (USR_PCURSOR)obj;
    USR_PTRANS   trans=usrcursor->transaction;

    return (MOptrout (MO_VALUE_TRUNCATED, (PTR)trans, userbuflen, userbuf));
}

/*
** Name: usr_curs_query_get
**
** Description:
**      Return the cursor query
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**      21-Oct-98 (fanra01)
**          Updated to get the query text from a database driver function.
**
*/
STATUS
usr_curs_query_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                     char * userbuf)
{
    STATUS      status = FAIL;
    USR_PCURSOR cursor = (USR_PCURSOR)obj;
    USR_PTRANS  trans  = cursor->transaction;
    DB_QUERY*   query  = &cursor->query;
    char*       statement = NULL;
    GSTATUS     err;

    err = DBGetStatement(trans->session->driver)(query, &statement);
    if (err == GSTAT_OK)
    {
        status = MOstrout (MO_VALUE_TRUNCATED, statement, userbuflen, userbuf);
    }
    return (status);
}

/*
** Name: usr_name_get
**
** Description:
**      Return the cursor query
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_name_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
    USR_PSESSION session = (USR_PSESSION)obj;
    
	if (session->userid == SYSTEM_ID)
	{
		status = MOstrout (MO_VALUE_TRUNCATED, session->user, userbuflen, userbuf);
	}
	else
	{
		GSTATUS err = GSTAT_OK;
		char*	user_name = NULL;
		err = DDSGetUserName(session->userid, &user_name);
		if (err == GSTAT_OK)
		{
			status = MOstrout (MO_VALUE_TRUNCATED, user_name, userbuflen, userbuf);
			MEfree((PTR)user_name);
		}
		else
		{
			status = err->number;
			DDFStatusFree(TRC_INFORMATION, &err);
		}
	}
return (status);
}

/*
** Name: usr_timeout_get
**
** Description:
**
**
** Inputs:
**
**      offset          offset to add to base object pointer.
**      objsize         size of object (ignored)
**      obj             pointer to bas object
**      userbuflen      size of the user buffer
**
** Outputs:
**
**      userbuf         updated user buffer with the cursor owner name.
**
** Returns:
**      OK                  successfully completed
**      MO_VALUE_TRUNCATED  buffer too short
**
** History:
**      02-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
usr_timeout_get (i4 offset, i4  objsize, PTR obj, i4  userbuflen,
                  char * userbuf)
{
	STATUS status = OK;
	USR_PSESSION session = (USR_PSESSION)obj;

	status = MOlongout (MO_VALUE_TRUNCATED, session->timeout_end - TMsecs(), userbuflen, userbuf);
return (status);
}

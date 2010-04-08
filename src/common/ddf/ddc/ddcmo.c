/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** ddcmo.c -- MO module for ddc
**
** Description
**
**
**
**
** Functions:
**
**
** History:
**      13-Jul-98 (fanra01)
**          Created.
**
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# include <compat.h>
# include <sp.h>
# include <gl.h>
# include <erglf.h>
# include <mo.h>
# include <ddfcom.h>

# include <dbaccess.h>

STATUS ddc_access_define (void);
STATUS ddc_sess_attach (PDB_CACHED_SESSION dbSession);
VOID ddc_sess_detach (PDB_CACHED_SESSION dbSession);
STATUS ddc_dbname_get (i4 offset, i4 objsize, PTR obj, i4 userbuflen,
                       char * userbuf);

STATUS ddc_dbtimeout_get (i4 offset, i4 objsize, PTR obj, i4 userbuflen,
                   char * userbuf);

GLOBALREF MO_CLASS_DEF db_classes[];
GLOBALREF char db_access_name[];

/*
** Name:    ddc_define
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
**      13-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
ddc_define (void)
{
return(MOclassdef (MAXI2, db_classes));
}

/*
** Name: ddc_sess_attach - make an instance based on the session name.
**
** Description:
**
**      Wrapper for MOattach.  Create an instance of the WTS_SESSION.
**
** Inputs:
**
**      name            name of active session (unique key)
**      dbSession       pointer to a cached db session structure
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
ddc_sess_attach (PDB_CACHED_SESSION  dbSession)
{
	STATUS status;
	char name[80];

	status = MOptrout(0, (PTR)dbSession, sizeof(name), name);
	if (status == OK)
		status = MOattach (MO_INSTANCE_VAR, db_access_name, name, (PTR)dbSession);
return(status);
}

/*
** Name: ddc_sess_detach - remove a DB_CACHED_SESSION instance
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
**      13-Jul-98 (fanra01)
**          Created.
**
*/
VOID
ddc_sess_detach (PDB_CACHED_SESSION dbSession)
{
	char name[80];

	if (MOptrout(0, (PTR)dbSession, sizeof(name), name) == OK)
		MOdetach (db_access_name, name);
}


/*
** Name: ddc_dbname_get - remove a DB_CACHED_SESSION instance
**
** Description:
**
**
**
** Inputs:
**
**
**
** Outputs:
**
**      None
**
** Returns:
**      None.
**
** History:
**      13-Jul-98 (fanra01)
**          Created.
**
*/
STATUS
ddc_dbname_get (i4 offset, i4 objsize, PTR obj, i4 userbuflen,
                  char * userbuf)
{
    PDB_CACHED_SESSION  session = (PDB_CACHED_SESSION) obj;
    PDB_CACHED_SESSION_SET sess_set = session->set;

    return (MOstrout(MO_VALUE_TRUNCATED, sess_set->name, userbuflen, userbuf));
}


/*
** Name: ddc_dbtimeout_get
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
ddc_dbtimeout_get (i4 offset, i4 objsize, PTR obj, i4 userbuflen,
                   char * userbuf)
{
    PDB_CACHED_SESSION  session = (PDB_CACHED_SESSION) obj;
	i4	left = (session->timeout_end == 0) ? 0 : session->timeout_end - TMsecs();

	return(MOlongout (MO_VALUE_TRUNCATED, left,
               userbuflen, userbuf));
}

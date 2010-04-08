/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <er.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <me.h>
#include    <st.h>
#include    <cv.h>
#include    <cs.h>
#include    <lk.h>
#include    <lo.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <scf.h>

/*
** Name: ascs_dbdeluser - decrement the number of users in the dbcb
**
** Description:
**      This routine grabs the Sc_main_cb semaphore and then chains down
**      the dbcb list looking for the given database, it then decrements the
**      number of users (db_ucount). If db_ucount reaches zero E_DB_WARN is
**      returned to indicate that the db should be deleted from the server.
**      The routine was implimented to keep SCF informed of database opens and
**      closes performed in DMF internal system threads.
**
** Inputs:
**      scf_cb
**            .scf_dbname       - name of db in which to decrement user count
**
** Outputs:
**      None.
**
** Returns:
**      DB_STATUS
**
** Side effects:
**      decrements db_ucount in dbcb and mat delete the db from the server.
**
** History:
*/
DB_STATUS
ascs_dbdeluser(SCF_CB *scf_cb)
{
    return (E_DB_OK);
}

/*
** Name: ascs_dbadduser - add to the number of users in the dbcb
**
** Description:
**      This routine grabs the Sc_main_cb semaphore and then chains down
**      the list of dbcb's looking for the database stipulated in
**      scf_cb->scf_dbname, and then adds to the user count (db_ucount).
**      The routine is implimented so that SCF can be made aware of database
**      opens performed in DMF internal system threads, this avoids an
**      attempt to delete a database from the server that may still be opened
**      by one of these threads.
**
** Inputs:
**      scf_cb
**            .scf_dbname       dbname to add to
**
** Outputs:
**      None.
**
** Side Effects:
**      Adds to db_ucount in a dbcb
**
** History:
*/
DB_STATUS
ascs_dbadduser(SCF_CB    *scf_cb)
{
    return (E_DB_OK);
}

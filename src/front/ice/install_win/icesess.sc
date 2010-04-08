/*
** Copyright (C) 2000 Ingres Corporation
*/
/*
** Name:    icesess.sc
**
** Description:
**      This file contains the the functions to request the number of
**      connected ice session to the ice server and return this count.
**
** Inputs:
**      None.
**
** Output:
**      Displays number of connected sessions.
**
** Returns:
**      0 - n   Number of sessions connected.
**      -n      Error
**
** History:
**      25-Jul-2000 (fanra01)
**          Bug 102187
**          Created utility to test if ice server has active sessions.
*/
EXEC SQL INCLUDE sqlca;

# include <compat.h>
# include <st.h>

i4
main( i4 argc, char** argv )
{
    EXEC SQL BEGIN DECLARE SECTION;
    i4 retcode = 0;
    EXEC SQL END DECLARE SECTION;
    char*   progname = NULL;
    char*   tmp = NULL;

    if ((progname = STrindex( argv[0], "\\", 0)) == NULL)
    {
        progname = argv[0];
    }

    EXEC SQL WHENEVER SQLERROR CALL sqlprint;

    /*
    ** Connect to imadb to get session information on the ice server.
    ** Continue on success.
    */
    EXEC SQL CONNECT 'imadb';

    if (sqlca.sqlcode == 0)
    {
        /*
        ** Expand domain to include all servers in the installation.
        */
        EXEC SQL EXECUTE PROCEDURE ima_set_vnode_domain;

        /*
        ** Get the number of connected user sessions.
        */
        EXEC SQL select count(*) into :retcode
            from ima_server_sessions
            where server like '%ICESVR%' and session_terminal='console';

        EXEC SQL commit;
        EXEC SQL disconnect;

        SIprintf( "%s : %d ICE session%sconnected.\n",
            progname, retcode, (retcode == 1) ? " ":"s " );
    }

    if ( sqlca.sqlcode != 0)
    {
        retcode = sqlca.sqlcode;
        SIprintf( "%s : Error SQL code %d [0x%08x]\n", progname, retcode, retcode );
    }
    return (retcode);
}

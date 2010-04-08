/*
** Copyright (c) 2004 Ingres Corporation
**
** Name: CAIIRW.C
**
** Description:
**      Defines GetReadOnly routine to read the DSN registry definition for
**      read-only capability for default read/write DLL. 
**
** History:
**      07-mar-2003 (loera01)
**          Created.
**      19-mar-2003 (loera01)
**          Made compatible with Unix/Linux.
**      06-may-2004 (loera01)
**          Added idmsodbc.h so that the driver version of 
**          SQLGetPrivateProfileString() is executed on Unix.
*/
#include <compat.h>
#include <cm.h>
#include <sql.h>
#include <sqlext.h>
#include <sqlca.h>
#ifdef _WIN32
#include <odbcinst.h>
#endif /* _WIN32 */
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"

BOOL GetReadOnly(char *dsn)
{
    char readOnly[2] = "\0";

    SQLGetPrivateProfileString (dsn, KEY_READONLY, "", readOnly,
        sizeof(readOnly),ODBC_INI);
    if (!CMcmpnocase(&readOnly[0],"Y"))
        return 1;
    return 0;
}

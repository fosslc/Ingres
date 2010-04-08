/*
**     testinfo.c 
**
*/

/*
** History:
**
**	21-apr-1994 (donj)
**	Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

EXEC SQL INCLUDE sqlca;
EXEC SQL INCLUDE sqlda;

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <tm.h>

#include <sepdefs.h>
#include <sepfiles.h>

#define testinfo_c

#include <fndefs.h>

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

EXEC SQL BEGIN DECLARE SECTION;

GLOBALDEF    i4            ing_not_found ;
GLOBALDEF    char          testinfo_errmsg [256] ;
GLOBALDEF    i4            ing_errno ;

EXEC SQL END DECLARE SECTION;


STATUS
testinfo_connect( char *db_name )
{
    STATUS                 ret_val = OK ;

    EXEC SQL BEGIN DECLARE SECTION;

    char                  *dbname = db_name ;   /* Holds the database name */

    EXEC SQL END DECLARE SECTION;

    EXEC SQL WHENEVER SQLERROR CALL testinfo_error;
    EXEC SQL WHENEVER SQLWARNING CALL testinfo_error;
    EXEC SQL WHENEVER NOT FOUND CALL testinfo_not_found;

    ing_not_found = ing_errno = OK;

    EXEC SQL connect :dbname;

    ret_val = (ing_not_found != OK)||(ing_errno != OK)
	    ? FAIL : OK ;

    return (ret_val);
}

STATUS
testinfo_disconnect()
{
    STATUS                 ret_val ;

    EXEC SQL WHENEVER SQLERROR CALL testinfo_error;
    EXEC SQL WHENEVER SQLWARNING CALL testinfo_error;
    EXEC SQL WHENEVER NOT FOUND CALL testinfo_not_found;

    ing_not_found = ing_errno = OK;

    EXEC SQL disconnect;

    ret_val = (ing_not_found != OK)||(ing_errno != OK)
	    ? FAIL : OK ;

    return (ret_val);
}

STATUS
testinfo_error()
{
    STATUS                 ret_val = OK ;

    /*
    ** Get text of error msg
    */

    EXEC SQL inquire_ingres (:testinfo_errmsg = ERRORTEXT,
			     :ing_errno = ERRORNO );

    if (tracing)
    {
	SIfprintf(traceptr,"ing_errno: %d\n", ing_errno);
	SIfprintf(traceptr,"testinfo_errmsg: %s\n", testinfo_errmsg);
	SIflush(traceptr);
    }
    else
    {
	SIprintf("ing_errno: %d\n", ing_errno);
	SIprintf("testinfo_errmsg: %s\n", testinfo_errmsg);
	SIflush(stdout);
    }

    return (ret_val);
}

STATUS
testinfo_not_found()
{
    STATUS                 ret_val = OK ;

    testinfo_error();

    ing_not_found = FAIL;

    return (ret_val);
}

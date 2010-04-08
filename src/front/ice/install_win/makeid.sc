/*
** Copyright (C) 1998 Ingres Corporation
*/
/*
** Name: makeid.sc
**
** Description:
**      Simple program to insert a range of integers into a column.
**      This is so blobstor can update the table and negates the need for
**      a filename column.
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      22-May-2007 (horda03) Bug 118375
**          Delete all rows in the table before inserting the new rows
**          to prevent duplicates.
**      01-Nov-2007 (horda03) Bug 118375
**         DELETE cannot use host variables.
*/

# include <compat.h>
# include <cv.h>
# include <si.h>
# include <st.h>

exec sql include sqlca;

/*
** Name: Usage
**
** Description:
**      Display a usage message
**
** Inputs:
**      progname    pointer to program name string.
**
** Outputs:
**      none
**
** Returns:
**      none
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
**      12-aug-1999 (mcgem01)
**          Change nat to i4.
*/
void
usage (char* progname)
{
    SIprintf ("Usage:\n");
    SIprintf ("      %s <start> <end> <column> <table> <database name>\n",
        progname);
}

/*
** Name: main
**
** Description:
**      Insert the range of values passed from the command line into the
**      specified column, table and database.
**
** Inputs:
**      argv[1]     start of range
**      argv[2]     end of range
**      argv[3]     column name
**      argv[4]     table name
**      argv[5]     database name
**
** Outputs:
**      none
**
** Returns:
**      OK      successfully completed
**      FAIL    on error
**
** History:
**      09-Oct-98 (fanra01)
**          Created.
*/
STATUS
main (i4 argc, char** argv)
{
    STATUS  status = FAIL;
    i4      start = 0;
    i4      end = 0;
    i4      i;
    char*   colname;
    char*   tbname;
exec sql begin declare section;
    char*   dbname;
    char    statement[80];
exec sql end declare section;

    if (argc < 6)
    {
        usage(argv[0]);
    }
    else
    {
        CVan (argv[1], &start);
        CVan (argv[2], &end);
        colname= argv[3];
        tbname = argv[4];
        dbname = argv[5];
        status = OK;
    }
    if (status == OK)
    {
        exec sql whenever sqlerror call sqlprint;

        exec sql connect :dbname;

        STprintf(statement, "delete from %s", tbname);

        exec sql execute immediate :statement;

        for (i=start; i < (end+1); i++)
        {
            STprintf (statement, "insert into %s (%s) values (%d)", tbname,
                colname, i);
            exec sql execute immediate :statement;
        }
        exec sql disconnect;
    }
    return (status);
}

/*
**  ROLL_DRIVER	    - Sync up fast commit sessions before crash.
**
**	This program is used to sync up the fast commit slaves before they
**	crash servers so that they all get to ready point before any of them
**	crash.
**
**	The fast commit servers will request an exclusive lock using the
**	DMF trace point DM1420 before crashing.  This program will hold
**	this lock until it sees that all the sessions have reached the crash
**	point.
**
**	Sessions indicate they have reached the crash point by writing copy
**	copy files out to the roll_data directory.  These copy files must
**	be cleaned out previous to each fast commit test run.
**
**	Inputs:
**	    dbname	- database to use.
**	    base_num	- test driver identifier
**	    num_slaves	- number of fast commit suites running.
**
**	Also, the logical ROLL_DATA must be defined to this process.
**
**      History:
**
**      DD-MMM-YYY     Unknown
**              Created Multi-Server Fast Commit test.
**      01-Mar-1991     Jeromef
**              Modified Multi-Server Fast Commit test to include ckpdb and
**              rollforwarddb.
**      21-Nov-1991     Jeromef
**              Add rollforward/msfc test to piccolo library
**      20-Jan-1994 (huffman)
**              Correct include files (xxcl.h) should look at the
**              common version (xx.h) for compiling.
**      17-feb-1994 (donj)
**              Get rid of "trigraph replaced warning"
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**
**
NEEDLIBS = LIBINGRES
**
*/

exec sql include sqlca;

#include        <compat.h>
#include        <tr.h>
#include        <pc.h>
#include        <st.h>
#include	<cv.h>

main(argc, argv)
int	    argc;
char	    *argv[];
{
    exec sql begin declare section;
    char	*dbname;
    int		base_num = -1;
    int		num_slaves = -1;
    int		slaves_done;
    char	copy_file[40];
    char	command_line[200];
    exec sql end declare section;
    char	table_name[32];
    int		roll_error();
    int		i;
    CL_SYS_ERR	syserr;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &syserr);
#endif
#ifdef UNIX
    TRset_file(TR_T_OPEN, "stdio", 5, &syserr);
#endif

    if (argc < 4)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    dbname = argv[1];
    CVal(argv[2], &base_num);
    CVal(argv[3], &num_slaves);

    if (base_num < 0 || num_slaves <= 0)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    /* IIseterr(roll_error); */
    exec sql whenever sqlerror call roll_error;

    /*
    ** Establish two sessions - one for holding the sync lock and
    ** the other for checking for the completion of the tests.
    */
    exec sql connect :dbname session 1;
    exec sql connect :dbname session 2;

    /*
    ** Get sync lock in session #1 - this will prevent any test slaves
    ** from hitting the crash point until the driver says its OK.
    */
    exec sql set_ingres(session = 1);
    exec sql set trace point DM1420;

    /*
    ** Switch to session #2 and create the sync table.
    ** This will start the slave processes going.
    */
    exec sql set_ingres(session = 2);
    STprintf(table_name, "roll_sync_tab_%d", base_num);
    STprintf(command_line, "create table %s (%s, %s) with duplicates",
	table_name, 
	"base i4 not null with default", 
	"slaves i4 not null with default");
    exec sql execute immediate :command_line;
    STprintf(command_line, "insert into %s (base, slaves) values (%d, %d)",
	table_name, base_num, num_slaves);
    exec sql execute immediate :command_line;
    exec sql commit;


    /*
    ** Wait for slaves to all get to point where they are ready for
    ** the servers to crash.
    **
    ** The slaves indicate they are ready by writing information to
    ** copy files.
    */

    STprintf(table_name, "roll_slave_tab_%d", base_num);
    STprintf(command_line, "create table %s (%s) with duplicates",
	table_name, 
	"a c50 not null");
    exec sql execute immediate :command_line;
    exec sql commit;

    /*
    ** Get information from SI files created by slave processes when
    ** they are ready for server crash.  When all slaves have written
    ** to file, then release sync lock to allow slaves to execute server
    ** crashes.
    */
    slaves_done = 0;
    while (slaves_done < num_slaves)
    {
	PCsleep(10000);
    	STprintf(command_line, "modify %s to truncated", table_name);
        exec sql execute immediate :command_line;

	for (i = 1; i <= num_slaves; i++)
	{
	    /* Build filename based on base and slave number */
	    STprintf(copy_file, "ROLL_DATA:roll_copy_%d_%d.in",
		base_num, i);
    	    STprintf(command_line, "copy table %s (a=c0nl) from '%s'", 
		table_name, copy_file);
            exec sql execute immediate :command_line;
	}

	STprintf(command_line, "select count(distinct a) from %s",
		table_name);
	exec sql declare c cursor for :command_line;
	exec sql open c;
	exec sql fetch c into :slaves_done;
	exec sql close c;
	exec sql commit;
    }

    /*
    ** Switch back to session 1 to commit the transaction holding
    ** the sync lock.
    */
    exec sql set_ingres(session = 1);
    exec sql commit;
    exec sql disconnect session 1;

    exec sql set_ingres(session = 2);
    exec sql disconnect session 2;

    PCexit(0);
}

int roll_error()
{
    EXEC SQL BEGIN DECLARE SECTION;
        char    error_buf[150];
    EXEC SQL END DECLARE SECTION;

    /*
    ** Don't error on COPY FILE doesn't exist - this is an expected error.
    */
    if (sqlca.sqlcode == -38100)
	return(0);

    TRdisplay("Error code %d\n", sqlca.sqlcode);
    EXEC SQL INQUIRE_INGRES(:error_buf = ERRORTEXT);
    TRdisplay("\nSQL Error:\n    %s\n", error_buf );
    return (0);
}

roll_usage()
{
    TRdisplay("USAGE: ROLL_DRIVER dbname base num_slaves\n");
    TRdisplay("    dbname - database to create driver tables in -\n");
    TRdisplay("             any of db's used in test can be used.\n");
    TRdisplay("    base   - base # for slaves of this driver.\n");
    TRdisplay("             this is to allow multiple drivers to run - each\n");
    TRdisplay("             must specify a different base number.\n");
    TRdisplay("    num_slaves - number of slave scripts\n");
    TRdisplay("\n");
}

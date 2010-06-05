/*
**  ROLL_SYNC	    - sync fast commit sessions with driver before starting
**
**  This program is executed by fast commit test sessions previous to starting
**  to make sure that the driver program is ready for them to begin.
**
**  The driver program will create a table "roll_sync_tab_NN" (where NN is
**  the driver base number) to indicate that it is ready.
**
**  Inputs:
**	dbname	    - database name to look for sync table in.
**	base_num    - fast commit test driver identifier.
**
**      History:
**
**      DD-MMM-YYYY     Unknown
**              Created Multi-Server Fast Commit test.
**      01-Mar-1991     Jeromef
**              Modified Multi-Server Fast Commit test to include ckpdb and
**              rollforwarddb.
**      21-Nov-1991     Jeromef
**              Add rollforward/msfc test to piccolo library
**      20-Jan-1994 (huffman)
**              Correct include files (xxcl.h) should look at the
**              common version (xx.h) for compiling.
**	17-feb-1994 (donj)
**		Added include of st.h so compiler could find STprintf().
**	17-feb-1994 (donj)
**		Get rid of "trigraph replaced warning"
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**
**
NEEDLIBS = LIBINGRES
**
*/

#include        <compat.h>
#include        <st.h>
#include        <tr.h>
#include        <pc.h>
#include	<cv.h>

main(argc, argv)
int	    argc;
char	    *argv[];
{
##  int		count_tups = 0;
##  char	*dbname;
##  char	table_name[32];
    int		base_num = 0;
    CL_SYS_ERR	syserr;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &syserr);
#endif
#ifdef UNIX
    TRset_file(TR_T_OPEN, "stdio", 5, &syserr);
#endif

    if (argc < 2)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    dbname = argv[1];
    CVal(argv[2], &base_num);

##  ingres dbname

    /*
    ** Loop checking for existence of roll_sync_tab.
    ** When this exists then the driver is ready for us to execute the
    ** fast commit script.
    **
    ** We must do lookup directly to iirelation, since the driver holds a
    ** lock on the sync table.
    */
    STprintf(table_name, "roll_sync_tab_%d", base_num);
##  set lockmode session where readlock=nolock
    while (count_tups == 0)
    {
##	retrieve (count_tups = count(iirelation.all
##		where iirelation.relid = table_name))
##	sleep 1
    }

##  exit

    PCexit(0);
}

roll_usage()
{
    TRdisplay("USAGE: ROLL_SYNC dbname base_num\n");
}

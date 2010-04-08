/*
**  ROLL_CLEANUP    - clean up tables to prepare for fast commit test run.
**
**	Destroys tables left around in sync database by the fast commit
**	test driver:
**
**	    roll_sync_tab_NN	    (tables have test driver number appened
**	    roll_slave_tab_NN	     to them so multiple drivers can run)
**
**	This must be run previous to starting up slave sessions sessions or
**	they may get out of sync.
**
**	Inputs:
**	    dbname	- database in which sync tables are used.
**	    base_num	- test driver identifier.
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
**      17-feb-1994 (donj)
**              Added include of st.h so compiler could find STprintf().
**      17-feb-1994 (donj)
**              Get rid of "trigraph replaced warning"
**
**
NEEDLIBS = LIBINGRES
**
*/

#include        <compat.h>
#include        <st.h>
#include        <tr.h>
#include        <pc.h>

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

    if (argc < 3)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    dbname = argv[1];
    CVal(argv[2], &base_num);

##  ingres dbname

    STprintf(table_name, "roll_sync_tab_%d", base_num);
##  destroy table_name
    STprintf(table_name, "roll_slave_tab_%d", base_num);
##  destroy table_name

##  exit

    PCexit(0);
}

roll_usage()
{
    TRdisplay("USAGE: ROLL_CLEANUP  dbname base_num\n");
}

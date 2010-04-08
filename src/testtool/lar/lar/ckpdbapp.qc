#include    <compat.h>
#include    <tr.h>

/*
**  CKPDB_APPEND - used for online checkpoint tests - performs appends.
**
**  CKPDB_APPEND dbname loops [options]
**
**  Required arguments
**	dbname	        - database name
**	loops	        - number of iterations
**
**  Optional arguments
**	none
**
**  History
**      13-aug-1991 (lauraw)
**              Filename changed to conform to standard
**              (changed from ckpdb_append.qc to ckpdbapp.qc)
**
**	19-sep-1995 (somsa01)
**		Added defined(DESKTOP) for NT, OS/2, and Netware platforms.
**  08-Sep-1997 (merja01)
**     Change longs to i4 to make compatable with 64 bit platforms
**     such as axp_osf (from schte01). 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Ming hints

NEEDLIBS = LIBINGRES

PROGRAM = ckpdb_append

**
*/

main(argc, argv)
int	argc;
char	*argv[];
{
##  char	*dbname;
##  i4 		i;
##  i4  	j;
##  i4  	run_no;
    CL_SYS_ERR	err_code;
    i4        loops;
    i4     start_time, end_time;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &err_code);
#endif
#if defined(UNIX) || defined(DESKTOP)
    TRset_file(TR_T_OPEN, "stdio", 5, &err_code);
#endif

    if (argc < 2)
    {
	TRdisplay("USAGE: CKPDB_APPEND dbname loops\n");
	TRset_file(TR_T_CLOSE, 0, 0, &err_code);
	exit(1);
    }

    loops = 1000;
    dbname = argv[1];
    if (argc > 2)
	loops = atoi(argv[2]);

##  ingres dbname

	TRdisplay("Starting Online-Checkpoint/Append Test\n");

	start_time = TMsecs();
#ifdef VMS
	lib$init_timer();
#endif

	/*
	** Find next run number to use.
	*/
##	retrieve (run_no = max(ckpdb_ap4.run))
	run_no++;

	/*
	** Loop through transactions.
	*/
	for (i = 1; i <= loops; i++)
	{
##	    begin transaction
	    for (j = 1; j <= i % 10; j++)
	    {
##		repeat append ckpdb_ap1 (run = @run_no, key = @i, sequence = @j)
##		repeat append ckpdb_ap2 (run = @run_no, key = @i, sequence = @j)
##		repeat append ckpdb_ap3 (run = @run_no, key = @i, sequence = @j)
##		repeat append ckpdb_ap4 (run = @run_no, key = @i, sequence = @j)
##              repeat append ckpdb_ap5_nojnl (run = @run_no, key = @i, sequence = @j)
	    }
##	    end transaction
	}

	end_time = TMsecs();

#ifdef VMS
	lib$show_timer();
#endif

	TRdisplay("Online-Checkpoint/Append Test completed in %d seconds\n",
	    end_time - start_time);

##  exit

    TRset_file(TR_T_CLOSE, 0, 0, &err_code);
}

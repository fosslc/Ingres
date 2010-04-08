#include    <compat.h>
#include    <tr.h>

/*
**  CKPDB_ABORT - used for online checkpoint tests - performs transaction
**	aborts.
**
**  CKPDB_ABORT dbname loops [options]
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
**		(ckpdb_abort.qc changed to ckpdbabt.qc)
**
**	19-sep-1995 (somsa01)
**		Added defined(DESKTOP) for NT, OS/2, and Netware platforms.
**  08-Sep-1997 (merja01)
**      Change long to i4 to make compatable with 64 bit platforms     
**      such as axp_osf (from schte01). 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Nov-2008 (wanfr01)	
**	    The output of this program does not match what is expected in the 
**	    Canons!  This has been corrected.
**	25-Nov-2008 (wanfr01)	
**	    Correction to above change
*/

/*
** Ming hints

NEEDLIBS = LIBINGRES

PROGRAM = ckpdb_abort

**
*/

main(argc, argv)
int	argc;
char	*argv[];
{
##  char	*dbname;
##  i4  	i;
    CL_SYS_ERR	err_code;
    i4		loops;
    i4     start_time, end_time;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &err_code);
#endif
#if defined(UNIX) || defined(DESKTOP)
    TRset_file(TR_T_OPEN, "stdio", 5, &err_code);
#endif

    if (argc < 2)
    {
	TRdisplay("USAGE: CKPDB_ABORT dbname loops\n");
	TRset_file(TR_T_CLOSE, 0, 0, &err_code);
	exit(1);
    }

    loops = 1000;
    dbname = argv[1];
    if (argc > 2)
	loops = atoi(argv[2]);

##  ingres dbname

	TRdisplay("Starting Online-Checkpoint/Abort Test\n");

	start_time = TMsecs();
#ifdef VMS
	lib$init_timer();
#endif

	/*
	** Start with empty control table.
	*/
##	delete ckpdb_ab_control

	/*
	** Loop through transactions.
	** Each transaction append rows to table1 and deletes rows from table2
	*/

	for (i = 1; i <= loops; i++)
	{
##	    append ckpdb_ab_control (cnt = i)

##	    begin transaction
##		append ckpdb_abort1 (a = ckpdb_ab_control.cnt)
##		delete ckpdb_abort2 where ckpdb_abort2.a < i
##              append ckpdb_ab1_nojnl (a = ckpdb_ab_control.cnt)
##              delete ckpdb_ab2_nojnl where ckpdb_ab2_nojnl.a < i
##	    abort

	}

	end_time = TMsecs();

#ifdef VMS
	lib$show_timer();
#endif

	TRdisplay("Online-Checkpoint/Abort Test completed\n");

##  exit

    TRset_file(TR_T_CLOSE, 0, 0, &err_code);
}

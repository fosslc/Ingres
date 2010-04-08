#include    <compat.h>
#include    <tr.h>

/*
**  CKPDB_DELAPP - used for online checkpoint tests - performs delete/appends
**	to btree table.
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
**	13-aug-1991 (lauraw)
**		Filename changed to conform to standard
**		(changed from ckpdb_delap.qc to ckpdbda.qc)
**
**	19-sep-1995 (somsa01)
**		Added defined(DESKTOP) for NT, OS/2, and Netware platforms.
**  08-Sep=1997 (merja01)
**      Changes longs to i4 to make compatable with the 64 bit
**      platforms such as axp_osf (from schte01).
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

PROGRAM = ckpdb_delapp

**
*/

main(argc, argv)
int	argc;
char	*argv[];
{
    CL_SYS_ERR	err_code;
##  char	*dbname;
##  i4  	min_value;
##  i4  	max_value;
##  i4  	count_values;
    i4  	loops;
    i4  	i;
    i4     start_time, end_time;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &err_code);
#endif
#if defined(UNIX) || defined(DESKTOP)
    TRset_file(TR_T_OPEN, "stdio", 5, &err_code);
#endif

    if (argc < 3)
    {
	TRdisplay("USAGE: UPDATE dbname loops\n");
	TRset_file(TR_T_CLOSE, 0, 0, &err_code);
	exit(1);
    }

    loops = 1000;

    dbname = argv[1];
    loops = atoi(argv[2]);

##  ingres dbname

	TRdisplay("Starting Online-Checkpoint/Delapp Btree Test\n");

	start_time = TMsecs();
#ifdef VMS
	lib$init_timer();
#endif

	/*
	** Prepare table - get min and max values.
	*/

##	retrieve (min_value = min(ckpdb_btree.value),
##		  max_value = max(ckpdb_btree.value),
##		  count_values = count(ckpdb_btree.value))


	/*
	** Start Delete/Append loop.
	*/
	for (i = 0; i < loops; i++)
	{
##	    begin transaction

##		delete ckpdb_btree where ckpdb_btree.value = min_value
##		append ckpdb_btree (value = max_value + 1)

##		delete ckpdb_hash where ckpdb_hash.value = min_value
##		append ckpdb_hash (value = max_value + 1)
##              delete ckpdb_btree_nojnl where ckpdb_btree_nojnl.value = min_value
##              append ckpdb_btree_nojnl (value = max_value + 1)
		min_value++, max_value++;

##	    end transaction

	}

	end_time = TMsecs();

#ifdef VMS
	lib$show_timer();
#endif

	TRdisplay("Online-Checkpoint/Delapp Btree Test completed in %d seconds\n",
	    end_time - start_time);

##  exit

    TRset_file(TR_T_CLOSE, 0, 0, &err_code);
}

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <pc.h>
#include    <mh.h>
#include    <tr.h>
#include    <tm.h>

/*
**  CKPDB1_RELOAD - used for online checkpoint tests - loads database for
**	checkpoint test #1.
**
**	Tests loaded for:
**
**	    CKPDB_ABORT	 - abort tests.
**	    CKPDB_APPEND - append tests.
**	    CKPDB_DELAPP - delete/append tests.
**
**  CKPDB_RELOAD dbname [options]
**
**  Required arguments
**	dbname	        - database name
**
**  Optional arguments
**	none
**
**  History
**      13-aug-1991 (lauraw)
**              Filename changed to conform to standard
**		(ckpdb1_load.qc changed to ckpdbld.qc)
**	9-Aug-1993 (fredv)
**		Include <gl.h> and <iicommon.h> so that it will be compiled.
**	29-sep-1993 (dianeh)
**		Remove #include of <lg.h>.
**
**	19-sep-1995 (somsa01)
**		Added defined(DESKTOP) for NT, OS/2, and Netware platforms.
**  08-Sep-1997 (merja01)
**      Changed longs to i4 to make compatable with 64 bit platforms
**      such as axp_osf.
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

PROGRAM = ckpdb1_load

**
*/

main(argc, argv)
int	argc;
char	*argv[];
{
##  char	*dbname;
    CL_SYS_ERR	err_code;
    i4	start_time, end_time;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &err_code);
#endif
#if defined(UNIX) || defined(DESKTOP)
    TRset_file(TR_T_OPEN, "stdio", 5, &err_code);
#endif

    if (argc < 2)
    {
	TRdisplay("USAGE: CKPDB_RELOAD dbname\n");
	TRset_file(TR_T_CLOSE, 0, 0, &err_code);
	exit(1);
    }

    dbname = argv[1];

##  ingres dbname

	start_time = TMsecs();

	load_abort();
	load_append();
	load_delapp();

	end_time = TMsecs();

##  exit

    TRset_file(TR_T_CLOSE, 0, 0, &err_code);
}

/*
** LOAD_ABORT - Load data for abort tests.
**
**
*/
load_abort()
{
##  i4  	i;

	/*
	** Load data for abort tests - CKPDB_ABORT.
	*/
##	destroy ckpdb_ab_control
##	destroy ckpdb_abort1
##	destroy ckpdb_abort2
##	destroy ckpdb_ab1_nojnl
##	destroy ckpdb_ab2_nojnl

##	create ckpdb_ab_control (cnt = i4)
##	create ckpdb_abort1 (a = i4, b = c100, c = c1500)
##	create ckpdb_abort2 (a = i4, b = c100)
##	create ckpdb_ab1_nojnl (a = i4, b = c100, c = c1500)
##	create ckpdb_ab2_nojnl (a = i4, b = c100)

	for (i = 1; i <= 100; i++)
	{
##	    repeat append ckpdb_abort2 (a = @i)
##	    repeat append ckpdb_abort2 (a = @i)
	}

##	set journaling on ckpdb_ab_control
##	set journaling on ckpdb_abort1
##	set journaling on ckpdb_abort2

##	modify ckpdb_ab_control to hash
##	modify ckpdb_abort1 to btree on a,b
##	modify ckpdb_abort2 to hash on a
##	modify ckpdb_ab1_nojnl to btree on a,b
##	modify ckpdb_ab2_nojnl to hash on a
}

/*
** LOAD_APPEND - Load data for append tests.
**
**
*/
load_append()
{
	/*
	** Load data for append tests - CKPDB_APPEND.
	*/
##	destroy ckpdb_ap1
##	destroy ckpdb_ap2
##	destroy ckpdb_ap3
##	destroy ckpdb_ap4
##	destroy ckpdb_ap5_nojnl

##	create ckpdb_ap1 (run = i4, key = i4, sequence = i4, pad=c40)
##	create ckpdb_ap2 (run = i4, key = i4, sequence = i4, pad=c40)
##	create ckpdb_ap3 (run = i4, key = i4, sequence = i4, pad=c40)
##	create ckpdb_ap4 (run = i4, key = i4, sequence = i4, pad=c40)
##	create ckpdb_ap5_nojnl (run = i4, key = i4, sequence = i4, pad=c40)

##	set journaling on ckpdb_ap1
##	set journaling on ckpdb_ap2
##	set journaling on ckpdb_ap3
##	set journaling on ckpdb_ap4

##	modify ckpdb_ap2 to hash on run, key
##	modify ckpdb_ap3 to isam on run, key
##	modify ckpdb_ap4 to btree on run, key, sequence
##	index on ckpdb_ap4 is ap4_ind (key) with structure=hash
##	modify ckpdb_ap5_nojnl to hash on run 
##	index on ckpdb_ap5_nojnl is ap5_ind (key) with structure=btree
}

/*
** LOAD_DELAPP - Load data for delete/append tests.
**
**
*/
load_delapp()
{
##  i4  	j;

	/*
	** Load data for append tests - CKPDB_APPEND.
	*/
##	destroy ckpdb_btree
##	destroy ckpdb_hash
##	destroy ckpdb_btree_nojnl

##	create ckpdb_btree (value = i4, pad1 = c16, pad2=c20)
##	create ckpdb_hash  (value = i4, pad1 = c16, pad2=c20)
##	create ckpdb_btree_nojnl (value = i4, pad1 = c16, pad2=c20)

	for (j = 1; j <= 1000; j++)
	{
##	    repeat append ckpdb_btree (value = @j)
##	    repeat append ckpdb_hash  (value = @j)

##	    repeat append ckpdb_btree_nojnl (value = @j)
	}

##	set journaling on ckpdb_btree
##	set journaling on ckpdb_hash

##	modify ckpdb_btree to btree on value, pad1
##	modify ckpdb_hash  to hash  on value
##	modify ckpdb_btree_nojnl to btree on value 
}

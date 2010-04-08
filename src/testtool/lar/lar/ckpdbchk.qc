#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <pc.h>
#include    <mh.h>
#include    <tr.h>
#include    <tm.h>

/*
**  CKPDB1_CHECK - used for online checkpoint tests - checks state of database
**	for consistency.
**
**	This program checks the specified database for consistency following
**	execution of the online checkpoint tests.
**
**	This program can be run either previous to the test, after completion
**	of the tests - or on any consistent version of the database mid-test.
**
**	The purpose of the program is not to verify the results of the test, but
**	to verify the consistency of the database.  This program is run after
**	restoring the database to a checkpoint created during execution of the
**	checkpoint tests.  Since we cannot predict the state of the database
**	at the time the checkpoint started (it is somewhat random) - we cannot
**	compare the restored database to any CANON version - all we can do is
**	verify that it is consistent.  The checkpoint tests use a different
**	method to verify the data is correct after using the journal files to
**	restore the database from the checkpoint.
**
**	Consistency checks which can be made in this program:
**
**	    If table's should always have a constant number of rows, then
**		the rowcount of those tables can be checked.  For example,
**		if a table starts with 1000 rows and each transaction which
**		acts on that table either replaces or does a delete/append in
**		the same transaction, then any consistent view of the table
**		should have 1000 rows.
**
**	    If a test writes duplicate data in the each transaction, then we
**		can check to see that the data is correctly duplicated.  For
**		example, if a test has two identical tables, and all operations
**		are performed to each table in the same transaction, then in any
**		consistent view the tables should remain identical.
**
**	    If a test keeps history information of changes made (i.e. TP1), then
**		the data can be checked against the history for consistency.
**
**	When building new tests, consider adding duplicate tables or history
**	information so checks can be added here for consistency.
**
**  CKPDB_CHECK dbname [options]
**
**  Required arguments
**	dbname	        - database name
**
**  Optional arguments
**	none
**
**  History
**	13-aug-1991 (lauraw)
**		Filename changed to conform to standard
**		(changed from ckpdb1_check.qc to ckpdbchk.qc)
**	9-Aug-1993 (fredv)
**		Include <gl.h> and <iicommon.h> so that it will be compiled.
**	29-sep-1993 (dianeh)
**		Remove #include of <lg.h>.
**
**	19-sep-1995 (somsa01)
**		Added defined(DESKTOP) for NT, OS/2, and Netware platforms.
**  08-Sep-1997 (merja01)
**      Changes longs to i4 to make compatable with 64 bit platforms
**      such as axp_osf (from schte01).
**
*/

/*
** Ming hints

NEEDLIBS = LIBINGRES

PROGRAM = ckpdb1_check

*/

main(argc, argv)
int	argc;
char	*argv[];
{
##  char	*dbname;
    CL_SYS_ERR	err_code;
    i4	start_time, end_time;
    i4	status = 0;

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

	status += check_abort();
	status += check_append();
	status += check_delapp();

	if (status == 0)
	    TRdisplay("Online Checkpoint Consistency Check Successfull\n");
	else
	    TRdisplay("Online Checkpoint Consistency Check Failure\n");

	end_time = TMsecs();

##  exit

    TRset_file(TR_T_CLOSE, 0, 0, &err_code);
    PCexit(status);
}


/*
** CHECK_ABORT - Check data from abort test.
**
*/
check_abort()
{
##	i4  	i;
##	i4  	j;
	i4  	status = 0;

	/*
	** Check CKPDB_ABORT data.
	**
	**     ckpdb_abort1 should have   0 rows.
	**     ckpdb_abort2 should have 100 rows with values 1-100.
	**     ckpdb_ab_control should have rows with sequential values.
	*/
##	retrieve (i = count(ckpdb_abort1.a))
	if (i != 0)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_ABORT1 has %d rows - should have 0\n", i);
	}
##	retrieve (i = count(ckpdb_ab1_nojnl.a))
	if (i != 0)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AB1_NOJNL has %d rows - should have 0\n", i);
	}

##	retrieve (i = count(ckpdb_abort2.a))
	if (i != 100)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_ABORT2 has %d rows - should have 100\n", i);
	}
##	retrieve (i = countu(ckpdb_abort2.a))
	if (i != 100)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_ABORT2 has %d unique rows - should have 100\n", i);
	}
##	retrieve (i = min(ckpdb_abort2.a))
	if (i != 1)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_ABORT2 min row is %d - should be 1\n", i);
	}
##	retrieve (i = max(ckpdb_abort2.a))
	if (i != 100)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_ABORT2 max row is %d - should be 100\n", i);
	}

##	retrieve (i = count(ckpdb_ab_control.cnt))
	if (i != 0)
	{
##	    retrieve (j = countu(ckpdb_ab_control.cnt))
	    if (i != j)
	    {
		status = 1;
		TRdisplay("Table CKPDB_AB_CONTROL has %d duplicate rows\n",i-j);
	    }
##	    retrieve (i = min(ckpdb_ab_control.cnt))
	    if (i != 1)
	    {
		status = 1;
		TRdisplay("Table CKPDB_AB_CONTROL min row is %d - should be 1\n", i);
	    }
##	    retrieve (i = max(ckpdb_ab_control.cnt))
	    if (i != j)
	    {
		status = 1;
		TRdisplay("Table CKPDB_AB_CONTROL max row is %d - should be %d\n", i, j);
	    }
	}

	return (status);
}

/*
** CHECK_APPEND - Check data from append test.
**
*/
check_append()
{
##	i4  	k;
##	i4  	l;
	i4  	status = 0;

	/*
	** Check CKPDB_APPEND data.
	**
	**     ckpdb_ap1, ckpdb_ap2, ckpdb_ap3, ckpdb_ap4 should all be same.
	**     Each should have sequential run #'s.
	*/
##	retrieve (k = count(ckpdb_ap1.tid))
##	retrieve (l = countu(ascii(ckpdb_ap1.run)+ascii(ckpdb_ap1.key)+ascii(ckpdb_ap1.sequence)))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP1 has %d duplicate rows\n", k - l);
	}

##	retrieve (l = count(ckpdb_ap2.all))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP2 has %d rows, should have %d\n", l, k);
	}
##	retrieve (l = count(ckpdb_ap3.all))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP3 has %d rows, should have %d\n", l, k);
	}
##	retrieve (l = count(ckpdb_ap4.tid))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP4 has %d rows, should have %d\n", l, k);
	}
##	retrieve (l = count(ckpdb_ap5_nojnl.tid))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP5_NOJNL has %d rows, should have %d\n", l, k);
	}
##	retrieve (l = count(ap4_ind.tidp))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table AP4_IND has %d rows, should have %d\n", l, k);
	}
##	retrieve (l = count(ap5_ind.tidp))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table AP5_IND has %d rows, should have %d\n", l, k);
	}

##	retrieve (k = max(ckpdb_ap1.run))
##	retrieve (l = max(ckpdb_ap2.run))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP2 max is %d, should be %d\n", l, k);
	}
##	retrieve (l = max(ckpdb_ap3.run))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP3 max is %d, should be %d\n", l, k);
	}
##	retrieve (l = max(ckpdb_ap4.run))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP4 max is %d, should be %d\n", l, k);
	}
##	retrieve (l = max(ckpdb_ap5_nojnl.run))
	if (k != l)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_AP5_NOJNL max is %d, should be %d\n", l, k);
	}

	return (status);
}

/*
** CHECK_DELAPP - Check data from delete/append test.
**
*/
check_delapp()
{
##	i4  	m;
##	i4  	n;
##	i4  	o;
	i4  	status = 0;

	/*
	** Check CKPDB_DELAPP data.
	**
	**     ckpdb_btree and ckpdb_hash should be identical tables.
	**     Both should have 1000 sequential tuples.
	*/
##	retrieve (m = count(ckpdb_btree.all))
##	retrieve (n = count(ckpdb_hash.all))
##	retrieve (o = count(ckpdb_btree_nojnl.all))
	if (m != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE has %d rows, should have 1000\n", m);
	}
	if (n != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_HASH has %d rows, should have 1000\n", n);
	}
	if (o != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE_NOJNL has %d rows, should have 1000\n", o);
	}

##	retrieve (m = countu(ckpdb_btree.all))
##	retrieve (n = countu(ckpdb_hash.all))
##	retrieve (o = countu(ckpdb_btree_nojnl.all))
	if (m != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE has %d unique rows, should have 1000\n", m);
	}
	if (n != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_HASH has %d unique rows, should have 1000\n", n);
	}
	if (o != 1000)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE_JNL has %d unique rows, should have 1000\n", o);
	}

##	retrieve (m = min(ckpdb_btree.value))
##	retrieve (n = max(ckpdb_btree.value))
	if (n-m != 999)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE max-min is %d, should be 999\n", n-m);
	}

##	retrieve (m = min(ckpdb_btree.value))
##	retrieve (n = min(ckpdb_hash.value))
##	retrieve (o = min(ckpdb_btree_nojnl.value))
	if (n != m)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_HASH min is %d, should be %d\n", n, m);
	}
	if (o != m)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE_NOJNL min is %d, should be %d\n", o, m);
	}

##	retrieve (m = max(ckpdb_btree.value))
##	retrieve (n = max(ckpdb_hash.value))
##	retrieve (o = max(ckpdb_btree_nojnl.value))
	if (n != m)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_HASH max is %d, should be %d\n", n, m);
	}
	if (o != m)
	{
	    status = 1;
	    TRdisplay("Table CKPDB_BTREE_NOJNL max is %d, should be %d\n", o, m);
	}

	return (status);
}

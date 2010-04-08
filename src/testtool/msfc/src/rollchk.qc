/*
**  ROLL_CHECK	    - print results of fast commit run.
**
**  Inputs:
**	dbname	    - database
**	base_num    - test driver identifier
**	sess_num    - test session number
**
**      History:
**
**      DD-MMM-YYYY     Unknown
**              Created Multi-Server Fast Commit test.
**      01-Mar-1991     Jeromef
**              Modified Multi-Server Fast Commit test to include ckpdb and
**		rollforwarddb.
**      21-Nov-1991     Jeromef
**              Add rollforward/msfc test to piccolo library
**      20-Jan-1994 (huffman)
**              Correct include files (xxcl.h) should look at the
**              common version (xx.h) for compiling.
**      17-feb-1994 (donj)
**              Get rid of "trigraph replaced warning"
**
**
NEEDLIBS = LIBINGRES
**
*/


#include        <compat.h>
#include        <tr.h>
#include        <pc.h>
#include        <st.h>

main(argc, argv)
int	    argc;
char	    *argv[];
##{
##  char	*dbname;
##  int		base_num = -1;
##  int		sess_num = -1;
##  char	table_name[32];
##  char	base_name[32];
##  char	col1[80];
##  char	col2[80];
##  char	col3[80];
##  char	col4[80];
##  char	col5[80];
##  char	col9[1002];
##  int		dcol1;
##  int		dcol2;
##  int		dcol3;
##  int		dcol4;
##  int		dcol5;
##  int		dcol6;
##  int		base_count;
##  int		idx_count;
##  short	nulli;
##  int		i;
##  int		file_test;
    int		row_count;
    int		tbl_cnt;
    CL_SYS_ERR	syserr;
    int		j;

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
    CVal(argv[3], &sess_num);

    if (base_num < 0 || sess_num < 0)
    {
	roll_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

##  ingres dbname

    TRdisplay(" \n");
    TRdisplay("Test results for Multi Server Fast Commit test suite\n");
#ifdef NOTDEFINED	/* leave out this so won't get diffs */
    TRdisplay("----------------------------------------------------\n");
    TRdisplay("  Database    : %s\n", dbname);
    TRdisplay("  Base Number : %d   Session Number %d\n", base_num, sess_num);
    TRdisplay("----------------------------------------------------\n");
#endif
    TRdisplay(" \n");
    TRdisplay(" \n");

    STprintf(table_name, "rtab_%d_%d", base_num, sess_num);
    if ( file_exist(table_name) ) 
    {
	row_count = 0;
	TRdisplay("-----------------------------------------------------------------\n");
	TRdisplay("| RTAB\n");
	TRdisplay("-----------------------------------------------------------------\n");
##	retrieve (col1=ascii(table_name.a), col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col4
##	{
		TRdisplay("|%15s|%15s|%15s|\n", col1, col2, col3);
		row_count++;
##	}
	TRdisplay("-----------------------------------------------------------------\n");
	TRdisplay("(%d rows)\n", row_count);
	TRdisplay(" \n");
    }

    STprintf(table_name, "rtab2_%d_%d", base_num, sess_num);
    if ( file_exist(table_name) ) 
    {
	row_count = 0;
	TRdisplay("-----------------------------------------------------------------\n");
	TRdisplay("| RTAB2\n");
	TRdisplay("-----------------------------------------------------------------\n");
##	retrieve (col1=ascii(table_name.a), col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col4
##	{
		TRdisplay("|%15s|%15s|%15s|\n", col1, col2, col3);
		row_count++;
##	}
	TRdisplay("-----------------------------------------------------------------\n");
	TRdisplay("(%d rows)\n", row_count);
	TRdisplay(" \n");
    }

    for (i = 1; i <= 14; i++)
    {
	STprintf(table_name, "stab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| STAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col4
##	    {
		TRdisplay("|%15s|%15s|%15s|\n", col1, col2, col3);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 14; i++)
    {
	STprintf(table_name, "btab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| BTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (dcol1=table_name.a, col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		order by #dcol1, #col2, #col3, #col4
##	    {
		TRdisplay("|%15d|%15s|%15s|\n", dcol1, col2, col3);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 14; i++)
    {
	STprintf(table_name, "itab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| ITAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col4
##	    {
		TRdisplay("|%15s|%15s|%15s|\n", col1, col2, col3);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 14; i++)
    {
	STprintf(table_name, "htab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| HTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=left(table_name.b,10),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col4
##	    {
		TRdisplay("|%15s|%15s|%15s|\n", col1, col2, col3);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 4; i++)
    {
	STprintf(table_name, "xtab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| XTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (dcol1=table_name.a, col2=ascii(table_name.b),
##		col3=ascii(table_name.c), col4=ascii(table_name.tid))
##		order by #dcol1, #col2, #col3, #col4
##	    {
		TRdisplay("|%15d|%15s|%15s|\n", dcol1, col2, col3);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (j = 1; j <= 4;  j++)
    {
	STprintf(base_name, "xtab%d_%d_%d", j, base_num, sess_num);
	if (file_exist(base_name))
	{
##	  retrieve (base_count = count(base_name.all))

	  for (i = 1; i <= 7; i++)
	  {
	    row_count = 0;
	    STprintf(table_name, "i%dx%d_%d_%d", j, i, base_num, sess_num);
	    if (file_exist(table_name))
	    {
	      TRdisplay("-----------------------------------------------------------------\n");
	      TRdisplay("| I%dX%d\n",j,i);
	      TRdisplay("-----------------------------------------------------------------\n");
	      switch (i)
	      {
		case 1:
		case 2:
##		retrieve (dcol1=table_name.a)
##		    order by #dcol1
##		{
		    TRdisplay("|%15d|\n", dcol1);
		    row_count++;
##		}

		idx_count = 0;
##		retrieve (dcol1 = table_name.a, dcol2 = base_name.a)
##			where table_name.tidp = base_name.tid
##		{
		    if (dcol1 == dcol2)
			idx_count++;
##		}

		break;

		case 3:
##		retrieve (dcol1=table_name.a, col2=ascii(table_name.b))
##		    order by #dcol1, #col2
##		{
		    TRdisplay("|%15d|%15s|\n", dcol1, col2);
		    row_count++;
##		}

		idx_count = 0;
##		retrieve (dcol1 = table_name.a, dcol2 = base_name.a,
##			  dcol3 = table_name.b, dcol4 = base_name.b)
##			where table_name.tidp = base_name.tid
##		{
		    if (dcol1 == dcol2 && dcol3 == dcol4)
			idx_count++;
##		}

		break;

		case 4:
		case 5:
##		retrieve (dcol1=table_name.c)
##		    order by #dcol1
##		{
		    TRdisplay("|%15d|\n", dcol1);
		    row_count++;
##		}

		idx_count = 0;
##		retrieve (dcol1 = table_name.c, dcol2 = base_name.c)
##			where base_name.tid = table_name.tidp
##		{
		    if (dcol1 == dcol2)
			idx_count++;
##		}

		break;

		case 6:
		case 7:
##		retrieve (dcol1=table_name.b)
##		    order by #dcol1
##		{
		    TRdisplay("|%15d|\n", dcol1);
		    row_count++;
##		}

		idx_count = 0;
##		retrieve (dcol1 = table_name.b, dcol2 = base_name.b)
##			where base_name.tid = table_name.tidp
##		{
		    if (dcol1 == dcol2)
			idx_count++;
##		}

		break;

	      }		/* end of switch			*/
	    }		/* end of table_name IF statement	*/

	    if (row_count == base_count)
		TRdisplay("| Row Count matches Base Count\n");
	    if (idx_count == base_count)
		TRdisplay("| Index Count matches Base Count\n");

	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	  }			/* end of inner FOR loop	*/
	}		/* end of the IF statement	*/
    }		/* end of outer FOR loop	*/

    for (i = 1; i <= 9; i++)
    {
	STprintf(table_name, "ctab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| CTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=ascii(table_name.b),
##		col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col4
##	    {
		TRdisplay("|%15s|%15s|\n", col1, col2);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 3; i++)
    {
	STprintf(table_name, "mtab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| MTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=ascii(table_name.b),
##		col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col4
##	    {
		TRdisplay("|%15s|%15s|\n", col1, col2);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 1; i++)
    {
	STprintf(table_name, "dtab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| DTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a),
##		col4=ascii(table_name.tid))
##		sort by #col1, #col4
##	    {
		TRdisplay("|%15s|\n", col1);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 1; i++)
    {
	STprintf(table_name, "ttab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| TTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=ascii(table_name.b),
##		col4=ascii(table_name.tid))
##		sort by #col1, #col2, #col4
##	    {
		TRdisplay("|%15s|%15s|\n", col1, col2);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    /*
    ** Collect results for compressed table tests.
    ** (Fast Commit tests 23-26)
    */
    for (i = 1; i <= 4; i++)
    {
	STprintf(table_name, "comptab%d_%d_%d", i, base_num, sess_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| COMPTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col2=table_name.b, 
##		col3=table_name.c, col9:nulli=table_name.d, 
##		col5=ascii(table_name.tid))
##		sort by #col1, #col2, #col3, #col9, #col5
##	    {
		if (nulli == -1)
		STcopy("NULL_VALUE", col9);
		TRdisplay("|%15s|%15s|%15s|%15s|\n", col1, col2, col3, col9);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");
	}
    }

    /*
    ** Check results of concurrency tables (Fast Commit tests 27-28).  
    ** These tables are shared by all sessions running fast commit tests 
    ** and the results in them will depend on the number of sessions which 
    ** executed the fast commit test suite.
    **
    ** Rather than print out the table, we read through the tables to make
    ** sure that the information is consistent.  The following should be true:
    **
    **     Each table should have 64 rows numbered 1-64
    **     Each table should be identical
    **     The sum of the column b's in each table should be a multiple of
    **         10,000 (+64 for the orignal values).
    */
    for (i = 1; i <= 6; i++)
    {
	STprintf(table_name, "concurtab%d_%d", i, base_num);
	if (file_exist(table_name))
	{
	    row_count = 0;
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("| CONCURTAB%d\n",i);
	    TRdisplay("-----------------------------------------------------------------\n");
##	    retrieve (col1=ascii(table_name.a), col5=ascii(table_name.tid))
##		sort by #col1, #col5
##	    {
		TRdisplay("|%15s|\n", col1);
		row_count++;
##	    }
	    TRdisplay("-----------------------------------------------------------------\n");
	    TRdisplay("(%d rows)\n", row_count);
	    TRdisplay(" \n");

##	    retrieve (dcol1 = sum(table_name.b))
	    j = dcol1 - 64;
	    if ((j % 10000) != 0)
		TRdisplay("Sum of CONCURTAB%d.B : %d is wrong.\n", i, dcol1);
	    TRdisplay(" \n");
	}
    }

    for (i = 1; i <= 64; i++)
    {
	tbl_cnt = 0;
	STprintf(table_name, "concurtab1_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol1 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}
	STprintf(table_name, "concurtab2_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol2 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}
	STprintf(table_name, "concurtab3_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol3 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}
	STprintf(table_name, "concurtab4_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol4 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}
	STprintf(table_name, "concurtab5_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol5 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}
	STprintf(table_name, "concurtab6_%d", base_num);
	if (file_exist(table_name))
	{
##	    retrieve (dcol6 = table_name.b) where table_name.a = i
	    tbl_cnt = tbl_cnt + 1;
	}

	if ( (tbl_cnt > 0) && ((dcol1 != dcol2) || (dcol1 != dcol3) || 
		(dcol1 != dcol4) || (dcol1 != dcol5) || (dcol1 != dcol6)) )
	{
	    TRdisplay("CONCURTAB table mismatch: row %d: |%d|%d|%d|%d|%d|%d|\n",
		i, dcol1, dcol2, dcol3, dcol4, dcol5, dcol6);
	}
	
    }


##  exit
    PCexit(0);
##}

roll_usage()
{
    TRdisplay("USAGE: ROLL_CHECK dbname base_number session_number\n");
}

int file_exist(table_name)
## char	table_name[32];

{
##  int		file_test;

##  repeat retrieve (file_test=count(iitables.#table_name
##	where iitables.#table_name = @table_name))
    if (file_test != 0)
	return(1);
    else
	return(0);
}

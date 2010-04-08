/*
**     testcase.c 
**
**	These functions handle recording test cases
**	as they appear in the SEP test and logging
**	the differences that occur during a specific
**	test case.
*/

/*
** History:
**
**    4-apr-1994 (donj)
**	Created.
**	20-apr-1994 (donj)
**	    Added some functions to be used by listexec, i.e. testcase_merge().
**	    Added some args to existing functions to extend their usage.
**	 4-may-1994 (donj)
**	    Added includes for si.h and sepfiles.h. VMS requires them.
**       4-may-1994 (donj)
**	    VMS cl doesn't seem to have SIsscanf defined. We'll do it here.
**	10-may-1994 (donj)
**	    More VMS specific changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <tm.h>

#include <sepdefs.h>
#include <sepfiles.h>

#define testcase_c

#include <fndefs.h>

#ifdef VMS

#define         SIsscanf        sscanf

#endif

#define      TESTCASE_NONAME  ERx("noname")
#define      TC_TABLE_HEADER1 ERx(" TestCase        Diffs  Weight  Instances  ElapsedTime\n")
#define      TC_TABLE_HEADER2 ERx(" =====================================================\n")

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;
GLOBALREF    char          tcName [] ;

	     TEST_CASE    *testcase_root = NULL ;
	     TEST_CASE    *testcase_current = NULL ;

STATUS testcase_merge( TEST_CASE **tc_root, TEST_CASE **tc_new )
{
    STATUS                 ret_val = OK ;
    TEST_CASE             *tc_tmp ;
    TEST_CASE             *tc_found ;
    TEST_CASE             *tc_tmp2 ;

    if (*tc_root == NULL)
    {
	*tc_root = *tc_new;
	*tc_new  = NULL;
    }
    else if (*tc_new != NULL)
    {
	for (tc_tmp = *tc_new; tc_tmp; tc_tmp = tc_tmp->next)
	{
	    if ((tc_found = testcase_find(*tc_root, tc_tmp->name, &ret_val)) != NULL)
	    {
		tc_found->secs      += tc_tmp->secs;
		tc_found->diffs     += tc_tmp->diffs;
		tc_found->weight     = ((tc_found->weight*tc_found->instances)
				        +(tc_tmp->weight*tc_tmp->instances))
				       /(tc_found->instances+tc_tmp->instances);
		tc_found->instances += tc_tmp->instances;
	    }
	    else
	    {
		tc_found = testcase_last(*tc_root);
		testcase_init(&tc_tmp2,tc_tmp->name);
		tc_tmp2->file      = tc_tmp->file;
		tc_tmp2->diffs     = tc_tmp->diffs;
		tc_tmp2->weight    = tc_tmp->weight;
		tc_tmp2->begin     = tc_tmp->begin;
		tc_tmp2->secs      = tc_tmp->secs;
		tc_tmp2->instances = tc_tmp->instances;
		tc_found->next     = tc_tmp2;
	    }
	}
    }

    return (ret_val);
}

STATUS testcase_writefile()
{
    STATUS                 ret_val ;
    FILE                  *tcPtr ;
    LOCATION               tcLoc ;

    LOfroms(FILENAME & PATH,tcName,&tcLoc);
    if ((ret_val = SIopen(&tcLoc,ERx("w"),&tcPtr)) == OK)
    {
	testcase_trace(tcPtr, NULL, NULL);
    }
    SIclose(tcPtr);

    return (ret_val);
}

TEST_CASE *testcase_find(TEST_CASE *tcroot, char *tc_name, STATUS *ret_status)
{
    STATUS                 ret_val = OK ;
    TEST_CASE             *tcp = NULL ;

    while ((tcroot != NULL)&&(tcp == NULL))
    {
	if (STcompare(tcroot->name,tc_name) == 0)
	{
	    tcp = tcroot;
	}
	tcroot = tcroot->next;
    }

    *ret_status = ret_val;
    return (tcp);
}

TEST_CASE *testcase_last(TEST_CASE *tcroot)
{
    TEST_CASE             *tcp ;

    if (tcp = tcroot)
    {
	while (tcp->next != NULL)
	{
	    tcp = tcp->next;
	}
    }

    return (tcp);
}

STATUS testcase_logdiff(i4 nr_of_diffs)
{
    STATUS                 ret_val = OK ;

    if (testcase_current == NULL)
    {
	ret_val = FAIL;
    }
    else
    {
	testcase_current->diffs += nr_of_diffs;
    }

    return (ret_val);
}

i4  testcase_sumdiffs()
{
    i4                   sum_of_diffs = 0 ;
    TEST_CASE           *tcroot ;

    for (tcroot = testcase_root; tcroot != NULL; tcroot = tcroot->next)
    {
	if (tcroot->weight != 0)
	{
	    sum_of_diffs += (tcroot->diffs * tcroot->weight);
	}
    }

    return (sum_of_diffs);
}

STATUS testcase_startup()
{
    STATUS                 ret_val ;

    if (testcase_root != NULL)
    {
	ret_val = FAIL;
    }
    else if ((ret_val = testcase_init( &testcase_root, TESTCASE_NONAME)) == OK)
    {
    	testcase_current            = testcase_root;
    	testcase_current->begin     = TMsecs();
	testcase_current->weight    = 1;
	testcase_current->instances = 1;
    }

    if (tracing&TRACE_TESTCASE)
    {
	testcase_trace(traceptr, testcase_root, NULL);
    }

    return (ret_val);
}

STATUS testcase_tos(TEST_CASE *tcptr, char *str_ptr)
{
    STATUS                 ret_val = OK ;

    STprintf(str_ptr, ERx("%-15s %3d    %3d       %3d  %10d\n"),
		      tcptr->name, tcptr->diffs, tcptr->weight,
		      tcptr->instances, tcptr->secs);

    return (ret_val);
}

STATUS testcase_froms(char *str_ptr, TEST_CASE *tcptr)
{
    STATUS                 ret_val = OK ;

    SIsscanf(str_ptr, ERx("%s %d %d %d %d"),
		      tcptr->name, &(tcptr->diffs), &(tcptr->weight),
		      &(tcptr->instances), &(tcptr->secs));
    return (ret_val);
}

STATUS testcase_trace(FILE *fptr, TEST_CASE *tcroot, char *suffix)
{
    STATUS                 ret_val = OK ;
    TEST_CASE             *tcp ;
    char                  *tbl_line = NULL ;
    bool                   print_suffix = FALSE ;

    if ((tracing&TRACE_TESTCASE)||(fptr != traceptr))
    {
	if (tcroot == NULL)
	{
	    tcp = testcase_root;
	}
	else
	{
	    tcp = tcroot;
	}
	tbl_line = (char *)SEP_MEalloc(SEP_ME_TAG_MISC, TEST_LINE, TRUE, &ret_val);

	if ((suffix != NULL)&&(*suffix != EOS))
	{
	    print_suffix = TRUE;
	    SIfprintf(fptr, ERx("%s"), suffix);
	}
	SIfprintf(fptr, ERx("\n"));
	if (print_suffix)
	{
	    SIfprintf(fptr, ERx("%s"), suffix);
	}
	SIfprintf(fptr, TC_TABLE_HEADER1);
	if (print_suffix)
	{
	    SIfprintf(fptr, ERx("%s"), suffix);
	}
	SIfprintf(fptr, TC_TABLE_HEADER2);

	for (; tcp != NULL; tcp = tcp->next)
	{
	    if (print_suffix)
	    {
	    	SIfprintf(fptr, ERx("%s"), suffix);
	    }

	    if (testcase_current == tcp)
	    {
		SIfprintf(fptr, ERx("*"));
	    }
	    else
	    {
		SIfprintf(fptr, ERx(" "));
	    }
	    testcase_tos(tcp,tbl_line);
	    SIfprintf(fptr, ERx("%s"),tbl_line);
	}
	if (print_suffix)
	{
	    SIfprintf(fptr, ERx("%s"), suffix);
	}
	SIfprintf(fptr, ERx("\n"));
	MEfree(tbl_line);
    }

    return (ret_val);
}

STATUS testcase_init(TEST_CASE **tc, char *tc_name)
{
    STATUS                 ret_val ;
    TEST_CASE             *tcp ;

    tcp = (TEST_CASE *)SEP_MEalloc(SEP_ME_TAG_MISC, sizeof(TEST_CASE), TRUE,
				   &ret_val);

    tcp->file      = NULL;
    tcp->name      = STtalloc(SEP_ME_TAG_MISC, tc_name);
    tcp->diffs     = 0;
    tcp->weight    = 1;
    tcp->begin     = 0;
    tcp->secs      = 0;
    tcp->instances = 0;
    tcp->next      = NULL;

    *tc = tcp;

    return (ret_val);
}

STATUS testcase_start(char **argv)
{
    STATUS                 ret_val = OK ;
    TEST_CASE             *tcp = NULL ;
    TEST_CASE             *ntcp = NULL ;
    i4                     i ;

    if (tracing&TRACE_TESTCASE)
    {
	SIfprintf(traceptr,ERx("testcase_start> "));
	for ( i=0; argv[i] != NULL; i++ )
	{
	    SIfprintf(traceptr,ERx("%s "),argv[i]);
	}
	SIfprintf(traceptr,ERx("\n"));
    }

    if (testcase_root == NULL)
    {
	if ((ret_val = testcase_startup()) == OK)
	{
	    testcase_end(TRUE);
	    if ((ret_val = testcase_init(&ntcp, argv[1])) == OK)
	    {
	        testcase_root->next = ntcp;
		testcase_current    = ntcp;
	    }
	}
    }
    else
    {
	if (testcase_current != NULL)
	{
	    testcase_end(TRUE);
	}

	testcase_current = testcase_find(testcase_root, argv[1], &ret_val);

	if ((testcase_current == NULL)&&(ret_val == OK))
	{
	    if ((ret_val = testcase_init(&ntcp, argv[1])) == OK)
	    {
		testcase_current = ntcp;
		if (tcp = testcase_last(testcase_root))
		{
	    	    tcp->next = testcase_current;
		}
	    }
	}
    }

    if (ret_val == OK)
    {
	testcase_current->begin = TMsecs();
	if (argv[2] != NULL)
	{
	    ret_val = CVan(argv[2], &testcase_current->weight);
	}
	(testcase_current->instances)++;
    }

    if (tracing&TRACE_TESTCASE)
    {
	testcase_trace(traceptr, testcase_root, NULL);
    }

    return (ret_val);
}

STATUS testcase_end(i4 noname_flag)
{
    STATUS                 ret_val = OK ;
    i4                     i ;
    bool                   noname_current ;

    if (tracing&TRACE_TESTCASE)
    {
	SIfprintf(traceptr,ERx("testcase_end> \n"));
    }

    if (testcase_current)
    {
	noname_current = (STcompare(testcase_current->name,TESTCASE_NONAME) == 0);

	if (noname_flag || noname_current == FALSE)
	{
	    testcase_current->secs = testcase_current->secs
				   + (TMsecs() - testcase_current->begin);
	}

	if (noname_current)
	{
	    testcase_current = NULL;
	}
	else
	{
	    testcase_current = testcase_root;
	    testcase_current->begin = TMsecs();
	}
    }

    if (tracing&TRACE_TESTCASE)
    {
	testcase_trace(traceptr, testcase_root, NULL);
    }

    return (ret_val);
}

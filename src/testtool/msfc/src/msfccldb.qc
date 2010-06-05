/*
**  MSFC_CLEANDB    - destroy all test tables in database used for MSFC tests.
**
**	This program cleans out a database used for fast commit tests so that
**	disk space will not be wasted following a successful run of the tests.
**
**  Inputs:
**	dbname	    - database to use
**	base_num    - test driver identifier
**	sess_num    - session #
**
**      History:
**
**      DD-MMM-YYYY     Unknown
**              Created Multi-Server Fast Commit test.
**      21-Nov-1991     Jeromef
**              Add msfc test to piccolo library
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

#include	<compat.h>
#include	<tr.h>
#include	<pc.h>
#include	<st.h>
#include	<cv.h>

main(argc, argv)
int	    argc;
char	    *argv[];
##{
##  char	*dbname;
##  int		base_num = -1;
##  int		sess_num = -1;
##  char	table_name[32];
    CL_SYS_ERR	syserr;

#ifdef VMS
    TRset_file(TR_T_OPEN, "SYS$OUTPUT", 10, &syserr);
#endif
#ifdef UNIX
    TRset_file(TR_T_OPEN, "stdio", 5, &syserr);
#endif

    if (argc < 4)
    {
	msfc_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

    dbname = argv[1];
    CVal(argv[2], &base_num);
    CVal(argv[3], &sess_num);

    if (base_num < 0 || sess_num < 0)
    {
	msfc_usage();
	TRset_file(TR_T_CLOSE, 0, 0, &syserr);
	PCexit(0);
    }

##  ingres dbname

    STprintf(table_name, "rtab_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "rtab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab9_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab10_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab9_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab10_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab9_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab10_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab9_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab10_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab5_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab6_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab7_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab8_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab5_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab6_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab7_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab8_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab5_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab6_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab7_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab8_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab5_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab6_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab7_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab8_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab11_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab12_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab13_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "btab14_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab11_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab12_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab13_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "htab14_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab11_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab12_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab13_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "itab14_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab11_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab12_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab13_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "stab14_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "dtab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "xtab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "xtab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "xtab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "xtab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab3_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab4_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab5_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab6_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab7_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab8_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ctab9_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "ttab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "mtab1_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "mtab2_%d_%d", base_num, sess_num);
##  destroy table_name
    STprintf(table_name, "mtab3_%d_%d", base_num, sess_num);
##  destroy table_name

##  exit
    PCexit(0);
##}

msfc_usage()
{
    TRdisplay("USAGE: MSFC_TEST dbname test base session\n");
    TRdisplay("    dbname - database.\n");
    TRdisplay("    test   - test number to execute.\n");
    TRdisplay("    base   - base # of driver.\n");
    TRdisplay("    session - which slave this is.\n");
    TRdisplay("\n");
}

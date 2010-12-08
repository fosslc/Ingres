/*
** Copyright (c) 2004 Ingres Corporation
**
** pmtest2.c -- Basic PM functionality tester.

PROGRAM  = pmtest2

NEEDLIBS = COMPATLIB

** This is a self-contained table driven test program intended to
** perform basic performance testing, and parameter bounds testing
** for the PM facility.
** 
** Program does have an one external dependency, in that it expects
** to run in the context of a configured Ingres installation, as
** the PMget tests assume a populated config.dat.
** 
**  History:
**	07-mar-2003 (devjo01)
**	    Initial version.
**	18-mar-2003 (devjo01)
**	    Trap SEGV/SIGBUS exceptions.  When run against an
**	    older PM, certain of the bounds tests fail.  Also
**	    add copyright, fix problem with passed scan pattern.
**	27-jan-2005 (abbjo03)
**	    Rename to pmtest2 to avoid conflict with pmtest in front!st!util.
**      03-nov-2010 (joea)
**          Declare local functions static.
*/

# include <stdarg.h>
# include <compat.h>
# include <gl.h>
# include <pc.h>
# include <me.h>
# include <si.h>
# include <st.h>
# include <er.h>
# include <pm.h>
# include <ex.h>

# define	DEFAULT_1ST_PREFIX	"ii"

/*
To enable leaktest for your platform, add a suitable definition
of LEAKTEST_PSCMD_LINE within a conditional compile section.
*/
# ifdef axp_osf
# define LEAKTEST_PSCMD_LINE		"ps -o vsz -p %d"
# endif

i4	Verbose = 1;

char *my_defaults[PM_MAX_ELEM] = { NULL, };

struct get_test {
    char	*name;
    STATUS	 expected;
}	get_tests[] = {

    "Simple Gets", -1,
    "%s.%s.cbf.syscheck_mode", OK,
    "%s.$.cbf.syscheck_mode", OK,
    "%s.*.config.syscheck", OK,

    "Gets with wrong case", -1,
    "%s.%s.CBF.syscheck_mode", OK,
    "%s.$.CBF.syscheck_mode", OK,
    "!.CBF.syscheck_mode", OK,

    "Try to get undefined parameters" , -1,
    "%s.%s", PM_NOT_FOUND,
    "$.$", PM_NOT_FOUND,
    "%s.%s.utter.nonsense", PM_NOT_FOUND,
    "%s.$.utter.nonsense", PM_NOT_FOUND,
    "!.utter.nonsense", PM_NOT_FOUND,
    "!.cbf.syscheck_mode.xyz", PM_NOT_FOUND,
    "!.cbf.syscheck_mode.*", PM_NOT_FOUND,
    "!.*.syscheck_mode", PM_NOT_FOUND,
    "$.*.*.syscheck_mode", PM_NOT_FOUND,

    "Various badly formed parameters" , -1,
    "%s.%s..syscheck_mode", PM_NOT_FOUND,
    "%s.$.cbf.syscheck_mode.", PM_NOT_FOUND,
    "!", PM_NOT_FOUND,
    "", PM_NOT_FOUND,
    NULL, 0
};

struct add_test {
    char	*name;
    char	*value;
    STATUS	 expected;
}	add_tests[] = {
    "Testing PMinsert for good inserts", NULL, -1,
    "alpha.bits.three", "three", OK,
    "alpha.bits.threed", "threed", OK,
    "alpha.bits.three.four", "four", OK,
    "alpha.$.five", "five", OK,
    "$.$.six", "six", OK,
    "!.seven", "seven", OK,
    "!.test.a.ridiculously.long.parameter.string.ababababababababababababababbababababbababbabbababababbabababababababababababababbababababbabababab.fin",
      "fin", OK,

    "Testing PMinsert for bad inserts", NULL, -1,
    "$.$.six", "six", PM_FOUND,
    "trailing.dot.", "six", PM_BAD_REQUEST,
    "consecutive..dots", "six", PM_BAD_REQUEST,
    "", "six", PM_BAD_REQUEST,
/*  "!", "six", PM_BAD_REQUEST, --- Old & New PM both don't pass this test. */
    "too.many.elements.d.e.f.g.h.i.j.k", "six", PM_BAD_REQUEST,
    NULL, NULL, 0
};

struct get_test del_tests[] = {
    "Testing PMdelete", -1,
    "alpha.bits.three", OK,
    "alpha.bits.threed", OK,
    "alpha.bits.three.four", OK,
    "alpha.$.five", OK,
    "$.$.six", OK,
    "!.seven", OK,
    "!.test.a.ridiculously.long.parameter.string.ababababababababababababababbababababbababbabbababababbabababababababababababababbababababbabababab.fin", OK,

    "Testing PMdelete with undefined / bad names", -1,
    "$.$.six", PM_NOT_FOUND,
    "trailing.dot.", PM_NOT_FOUND,
    "consecutive..dots", PM_NOT_FOUND,
    "", PM_NOT_FOUND,
/*  "!", PM_NOT_FOUND,   --- Old & New PM both don't pass this test. */
    "too.many.elements.d.e.f.g.h.i.j.k", PM_NOT_FOUND,
    NULL, 0
};

static STATUS
null_handler(exargs)
EX_ARGS     *exargs;
{
    return(EXDECLARE);
} /* null_handler */

static void
banner( char *title )
{
    SIprintf( "\n** %s **\n\n", title );
}

/*
** Translate PM status code to text.  
**
** Note: certain of these codes do not exist in 2.0
*/
static char *
xlate_code( STATUS status, char *buf )
{
    switch ( status )
    {
    case OK: return "OK";
    case PM_SYNTAX_ERROR: return "PM_SYNTAX_ERROR";
    case PM_OPEN_FAILED: return "PM_OPEN_FAILED";
    case PM_BAD_FILE: return "PM_BAD_FILE";
    case PM_BAD_INDEX: return "PM_BAD_INDEX";
    case PM_FOUND: return "PM_FOUND";
    case PM_NOT_FOUND: return "PM_NOT_FOUND";
    case PM_BAD_REGEXP: return "PM_BAD_REGEXP";
    case PM_NO_INIT: return "PM_NO_INIT";
    case PM_NO_II_SYSTEM: return "PM_NO_II_SYSTEM";
    case PM_BAD_REQUEST: return "PM_BAD_REQUEST";
    case PM_NO_MEMORY: return "PM_NO_MEMORY";
    case PM_DUP_INIT: return "PM_DUP_INIT";
    default:
	STprintf(buf, "%x", status);
	break;
    }
    return buf;
}

static void
check_res( char *title, STATUS expected, STATUS actual )
{
    char	buf1[36],buf2[36];

    if ( expected != actual )
    {
	SIprintf( ">>> %s : Expected %s, got %s.\n",
		  title, xlate_code(expected,buf1), xlate_code(actual,buf2) );
    }
    else if ( Verbose )
    {
	SIprintf( "%s : passed\n", title );
    }
}

static void
test_setdefault( int count, char *title, ... )
{
    va_list	p;
    STATUS      status;
    char	*value, buf[132];
    i4		idx;

    banner(title);
    va_start(p,title);

    for ( idx = 0; idx < count; idx++ )
    {
	value = va_arg(p,char *);
	if ( NULL != value )
	{
	    STprintf(buf, "PMsetDefault element %d to \"%s\"", idx, value);
	    my_defaults[idx] = value;
	    status = PMsetDefault(idx,value);
	    check_res( buf, OK, status );
	}
    }
    va_end(p);
}

static i4 
test_get( char *name, char *expectedvalue, STATUS expectedstatus )
{
    STATUS	status;
    char 	*value = NULL, buf[132], namebuf[256];
    EX_CONTEXT           ex_context;

    STprintf(namebuf, name, my_defaults[0], my_defaults[1]);

    if ( EXdeclare(null_handler, &ex_context) )
    {
	SIprintf( ">>> PMget: EXCEPTION with \"%-.64s\"\n", namebuf );
	status = ~expectedstatus;
    }
    else
    {
	status = PMget( namebuf, &value );

	STprintf( buf, "PMget of \"%-.64s\"", namebuf );
	check_res( buf, expectedstatus, status );

	if ( OK == status && status == expectedstatus )
	{
	    if ( NULL == value )
	    {
		SIprintf( ">>> PMget: No value gotten for \"%-.64s\"\n",
		   namebuf );
		status = ~expectedstatus;
	    }
	    else if ( expectedvalue && 0 != STcompare(expectedvalue, value) )
	    {
		SIprintf(
 ">>> PMget: Bad value gotten for \"%-.64s\".  Got \"%s\", expected \"%s\"\n",
			  namebuf, value, expectedvalue );
		status = ~expectedstatus;
	    }
	}
    }
    EXdelete();
    return ( status == expectedstatus ) ? OK : FAIL;
}

static i4
test_add( char *name, char *value, STATUS expectedstatus )
{
    STATUS	status;
    char 	buf[132], namebuf[256];
    EX_CONTEXT           ex_context;

    STprintf(namebuf, name, my_defaults[0], my_defaults[1]);

    if ( EXdeclare(null_handler, &ex_context) )
    {
	SIprintf( ">>> PMinsert: EXCEPTION with \"%-.64s\"\n", namebuf );
	status = ~expectedstatus;
    }
    else
    {
	status = PMinsert( namebuf, value );

	STprintf( buf, "PMinsert of \"%-.64s\"", namebuf );
	check_res( buf, expectedstatus, status );

	if ( OK == status && status == expectedstatus )
	{
	    return test_get( name, value, OK );
	}
    }
    EXdelete();
    return ( status == expectedstatus ) ? OK : FAIL;
}

static i4
test_del( char *name, STATUS expectedstatus )
{
    STATUS	status;
    char 	buf[132], namebuf[256];
    EX_CONTEXT           ex_context;

    STprintf(namebuf, name, my_defaults[0], my_defaults[1]);

    if ( EXdeclare(null_handler, &ex_context) )
    {
	SIprintf( ">>> PMdelete: EXCEPTION with \"%-.64s\"\n", namebuf );
	status = ~expectedstatus;
    }
    else
    {
	status = PMdelete( namebuf );

	STprintf( buf, "PMdelete of \"%-.64s\"", namebuf );
	check_res( buf, expectedstatus, status );

	if ( OK == status && status == expectedstatus )
	{
	    return test_get( name, NULL, PM_NOT_FOUND );
	}
    }
    EXdelete();
    return ( status == expectedstatus ) ? OK : FAIL;
}

static i4
test_scan( char *title, char *pattern )
{
    PM_SCAN_REC	scanstate;
    STATUS	status;
    char	*name, *value, buf[132];
    EX_CONTEXT           ex_context;

    STprintf(buf, "%s: Pattern = \"%s\"", title, pattern);
    banner(buf);
    MEfill( sizeof(scanstate), '\0', (PTR)&scanstate );
    status = OK;
    if ( EXdeclare(null_handler, &ex_context) )
    {
	SIprintf( ">>> PMscan: EXCEPTION\n" );
	status = ~PM_NOT_FOUND;
    }
    else
    {
	while (OK == status)
	{
	    status = PMscan( pattern, &scanstate, NULL, &name, &value );
	    if (OK == status)
	    {
		SIprintf( "%s = %s\n", name, value );
	    }
	    else
	    {
		check_res( buf, PM_NOT_FOUND, status );
	    }
	    pattern = NULL;
	}
    }
    EXdelete();
    return ( status == PM_NOT_FOUND ) ? OK : FAIL;
}

int
main(argc, argv)
i4	argc;
char	*argv[];
{
    STATUS	status;
    struct get_test	*gts;
    struct add_test	*ats;

    MEadvise( ME_INGRES_ALLOC );

    banner( "Initialization & Loading" );

    status = PMinit();
    check_res( ERx("PMinit"), OK, status );
    status = PMinit();
    check_res( ERx("Duplicate PMinit"), PM_DUP_INIT, status );

    PMlowerOn();

    status = PMload((LOCATION *)NULL, (PM_ERR_FUNC *)0);
    check_res( ERx("PMload"), OK, status );

    test_setdefault( 2, "Setting Element defaults",
		     DEFAULT_1ST_PREFIX, PMhost() );

    for ( gts = get_tests; gts->name != NULL; gts++ )
    {
	if ( -1 == gts->expected ) 
	    banner(gts->name);
	else
	    test_get( gts->name, NULL, gts->expected );
    }

    test_setdefault( 2, "Resetting Element defaults", "alpha", "bits"  );

    for ( ats = add_tests; ats->name != NULL; ats++ )
    {
	if ( -1 == ats->expected ) 
	    banner(ats->name);
	else
	    test_add( ats->name, ats->value, ats->expected );
    }

    test_scan("Scanning Adds", "^alpha\\.bits\\.");

    for ( gts = del_tests; gts->name != NULL; gts++ )
    {
	if ( -1 == gts->expected ) 
	    banner(gts->name);
	else
	    test_del( gts->name, gts->expected );
    }

    test_scan("Scanning Adds after delete", "^alpha\\.bits\\.");

# ifdef LEAKTEST_PSCMD_LINE
    {
	PID		pid;
	i4		i;
	char	   	*ignore;
	char		cmdbuf[256];
	CL_ERR_DESC	errcode;

	PCpid( &pid );
	STprintf( cmdbuf, LEAKTEST_PSCMD_LINE, pid );
	banner( "Checking for PMget memory leak (please wait)" );

	banner( "Virtual size before 1,000,000 PMgets" );
	PCcmdline(NULL, cmdbuf, TRUE, NULL, &errcode);
	for ( i = 1000000; i > 0; i-- )
	{
	    PMget( "ii.*.ingstart.syscheck_command", &ignore );
	}
	banner( "Virtual size after 1,000,000 PMgets" );
	PCcmdline(NULL, cmdbuf, TRUE, NULL, &errcode);
    }
# else
    banner( "PMget memory leak test NOT ENABLED for this platform" );
# endif
    PCexit( OK );
}

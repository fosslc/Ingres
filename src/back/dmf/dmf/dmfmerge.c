/*
** Copyright (c) 1990, 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <lo.h>
# include <pc.h>
# include <me.h>
# include <er.h>
# include <st.h>
# include <si.h>
# include <id.h>
# include <cv.h>
# include <sl.h>
# include <tm.h>
# include <cs.h>
# include <pm.h>
# include <st.h>
# include <gc.h>
# include <lk.h>
# include <cx.h>
# include <gcccl.h>
# include <iicommon.h>
# include <dbdbms.h>
# include <dmf.h>
# include <dmfmerge.h>

/*
**
** History:
**	14-mar-1990 (boba)
**		Copied from 6.2 to 6.3.  Added cacheutil and lartool.
**	5-jul-1990 (jonb)
**		Added GWFLIB to ming hints
**	29-aug-1990 (jonb)
**		Add spaces in front of "main" to prevent automatic creation 
**		of the executable.  This is gets turned into a partially
**		linked .o file, which is deliverable.
**	18-dec-1990 (bonobo)
**		Added the PARTIAL_LD mode, and moved "main(argc, argv)" back to
**		the beginning of the line.
**	 4-feb-1991 (rogerk)
**		Allowed LOGSTAT and LOCKSTAT to be run by non superusers.
**	 4-feb-1991 (rogerk)
**		Added capability to follow program names with suffixes
**		(e.g. iidbms.test) so development installations can run
**		multiple versions of executables.
**	01-aug-91 (kirke)
**		Bug #37490 - The usrline buffer was the wrong size - it needs
**		to be big enough to hold the entire line from the users file.
**		The buffer would overflow on long user names, resulting in
**		unpredictable behavior.
**
**	3-jul-1992 (bryanp)
**		Set usrname size to DU_USR_SIZE + 1, since du_get_usrname
**		sticks a trailing EOS at usrname[DU_USR_SIZE].
**	8-july-1992 (ananth)
**	    Prototyping DMF.
**	3-oct-1992 (bryanp)
**		The dmfrcp is now just another name for the iidbms server. We
**		still call it the dmfrcp process so that the tools can find it
**		easily in a "ps" listing.
**	1-nov-1992 (fpang)
**		Added IISTAR as another personality of iimerge.	
**		Also added RQFLIB and TPFLIB to NEEDLIBS.
**	1-dec-1992 (robf)
**		Changed check reference to SECURITY/OPERATOR.
**		Updated message to refer to privileges rather than SUPERUSER.
**	22-jan-1993 (bryanp)
***		rcpstat is now a DMF_MERGE executable.
**	29-apr-1993 (bryanp)
**		iishowres is now a DMF_MERGE executable.
**	26-July-1993 (rmuth)
**		Change to use the PM system to check if the user is authorized
**	        to start the process as opposed to the "users" file. Make the
**	        dmfacp a SU_PRIVS process instead of a OWNERONLY.
**      13-jul-1993 (ed)
**          unnest dbms.h
**	20-Oct-1993 (rmuth)
**	        The PM check above has been changed to be a privilege list
**	        hence change code accordingly. At the same time create a
**	        <dmfmerge.h>.
**	25-Oct-1993 (seiwald)
**	    This file now neither includes nor depends on DUF.  Replaced
**	    call to du_usr_name() with IDname(), which is what du_usr_name()
**	    called anyhow.  Put #includes into the proper order.
**	22-nov-1993 (bryanp) B56700
**	    Add a PMload error handler to detect/report errors in config.dat
**	    Retrained the "#else" so that only DMF_MERGE code gets this file.
**	16-dec-1993 (tyler)
**	    Replaced GChostname() with call to PMhost().
**	19-dec-97 (stephenb)
**	    Add repstat_libmain() external
**      21-apr-1999 (hanch04)
**        Replace STindex with STchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-aug-2002 (hanch04)
**          For the 32/64 bit build,
**          call PCexeclp64 to execute the 64 bit version
**          if II_LP64_ENABLED is true
**	27-aug-2002 (somsa01)
**	    Corrected ifdef change from previous changes.
**	29-Aug-2002 (hanch04)
**	    PCexeclp64 should be called from the 32-bit version
**	19-sep-2002 (devjo01)
**	    Centralized MEadvise, and processing of NUMA params for
**	    non-server dmfmerge linked utilities.
**	20-may-2004 (devjo01)
**	    Call CXalter as appropriate to suppress DLM version
**	    messages to the screen.
**	16-Nov-2004 (bonro01)
**	    Allow Ingres to be installed using a non-ingres userid.
**	    Initialize the SystemAdminUser to the install userid.
**	31-Mar-2005 (kodse01)
**	    Changed 0 to NULL in IDsetsys call.
**	17-Jan-2006 (hanje04)
**	    BUG 115563
**	    We use PGID as the server PID on Linux because there is no real
**	    difference between threads and processes under LinuxThreads. This
**	    was previously set in iirun and PCpid would return the wrong value
**	    until it is set again in CSinterface() or CSMTinterface().
**	    Make sure PGID is set as soon as we start up on Linux by calling
**	    setpgrp() here.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Don't include dudbms when not needed.
**	02-Nov-2010 (jonj) SIR 124685
**	    Relocated FUNC_EXTERNs from here to dmf.h.
*/

/*
NEEDLIBS = 	DMFLIB SCFLIB PSFLIB OPFLIB RDFLIB QEFLIB QSFLIB \
		DMFLIB ADFLIB ULFLIB GCFLIB COMPATLIB MALLOCLIB \
		GWFLIB RQFLIB TPFLIB
**
PROGRAM =	iimerge
**
UNDEFS =        pst_nddump dmf_call dmf_trace
**
MODE =          PARTIAL_LD
**
OWNER =         INGRES
*/
# ifndef II_DMF_MERGE

int
main(argc, argv)
{
    MEadvise( ME_INGRES_ALLOC );
    SIprintf( "iimerge is for UNIX only!\n");
    PCexit( FAIL );

    /* NOTREACHED */
    return(FAIL);
}

# else	/* UNIX */

typedef struct
{
	char		*name;		/* name of the "program" */
	int		(*func)();	/* function that was main */
	i4		privs;		/* should it be setuid? */

#		define USERMODE		1	/* anyone, no privs */
#		define SETUID		2	/* anyone, w/privs */
#		define OWNERONLY	3	/* ingres w/privs */
#		define SU_PRIVS		4	/* superusers w/privs */
	i4		numaopt;
#		define NUMA_NEUTRAL	0	/* Does'nt care about NUMA */
#		define NUMA_SPECIAL	1	/* Context from Inst code. */
#		define NUMA_LOCAL_RAD	2	/* If NUMA, must specify
						   one local Rad */
#		define NUMA_ANY_NODE    3	/* Happy setting context
						   to any node, local or
						   remote. Implies doesn't
						   map to sysmem, etc. */
#		define NUMA_MIXED	4	/* Needs local NUMA context,
						   but can also specify a
						   remote target with -node*/
#		define NUMA_ANY_RAD	5	/* Needs a local NUMA context,
						   but doesn't particulary
						   care which one. */
	i4		alloctype;		/* MEadvise type. */
} PROG;

PROG progs[] =
{
    "cacheutil",cacheutil_libmain, SU_PRIVS, NUMA_LOCAL_RAD, ME_INGRES_ALLOC,
    "dmfacp",	dmfacp_libmain,    SU_PRIVS, NUMA_SPECIAL,   ME_INGRES_ALLOC,
    "dmfjsp",	dmfjsp_libmain,    SETUID,   NUMA_ANY_RAD,   ME_INGRES_ALLOC,
    "dmfrcp",	iidbms_libmain,    SU_PRIVS, NUMA_SPECIAL,   ME_INGRES_ALLOC,
    "iidbms",	iidbms_libmain,    SU_PRIVS, NUMA_SPECIAL,   ME_INGRES_ALLOC,
    "iimerge",	iimerge,	   USERMODE, NUMA_NEUTRAL,   ME_INGRES_ALLOC,
    "iishowres",iishowres_libmain, SETUID,   NUMA_ANY_NODE,  ME_INGRES_ALLOC,
    "iistar",	iidbms_libmain,    SU_PRIVS, NUMA_SPECIAL,   ME_INGRES_ALLOC,
    "lartool",	lartool_libmain,   SU_PRIVS, NUMA_LOCAL_RAD, ME_INGRES_ALLOC,
    "lockstat",	lockstat_libmain,  SETUID,   NUMA_LOCAL_RAD, ME_INGRES_ALLOC,
    "logdump",	logdump_libmain,   SU_PRIVS, NUMA_LOCAL_RAD, ME_INGRES_ALLOC,
    "logstat",	logstat_libmain,   SETUID,   NUMA_LOCAL_RAD, ME_INGRES_ALLOC,
    "rcpstat",	rcpstat_libmain,   SETUID,   NUMA_MIXED,     ME_INGRES_ALLOC,
    "rcpconfig",rcpconfig_libmain, OWNERONLY,NUMA_MIXED,     ME_INGRES_ALLOC,
    "repstat",	repstat_libmain,   SETUID,   NUMA_LOCAL_RAD, ME_INGRES_ALLOC,

    NULL,		NULL,	0, 0, 0
};

static VOID pmerr_func(STATUS, i4, ER_ARGUMENT *);
static PM_ERR_FUNC pmerr_func;

# define USER_NAME_SIZE DB_MAXNAME

static DB_STATUS dmf_check_supriv( VOID );

int 
main( argc, argv )
int argc;
char **argv;
{
    register char *want;
    register PROG *p;
    char	*suffix, *local_host;
    i4		want_length = 0, rad, targ;
    CL_ERR_DESC	sys_err;
    STATUS	status;
    char        *lp64enabled;
    ER_ARGUMENT	    arg;
    char        *adminuser=NULL;

    /*
    ** Try to exec the 64-bit version
    */
    NMgtAt("II_LP64_ENABLED", &lp64enabled);
    if ( (lp64enabled && *lp64enabled) &&
       ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
         !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
    {
        PCexeclp64(argc, argv);
    }

# ifdef LNX
    /*
    ** As we use the process group for the server PID on Linux (because of
    ** LinuxThreads 1:1 threading model) we need to make sure it is set
    ** correctly as soon as we start up otherwise "%p" in II_DBMS_LOG
    ** will be miss translated. Can be removed once support for LinuxThreads
    ** is dropped.
    */
    setpgrp();
# endif

    PMinit();
    status = PMload((LOCATION *)NULL, pmerr_func);
    if ( status != OK )
    {
	if (status != FAIL)
	    pmerr_func(status, (i4)0, &arg);

	SIfprintf(stdout, "Internal error initializing PM system\n");
	SIflush(stdout);
	PCexit( FAIL );
    }

    /*
    ** Set the default node name
    */
    PMsetDefault( 1, PMhost() );

    /* Set the install user as the System Administrator */
    if( PMget("ii.$.setup.owner.user", &adminuser ) == OK && adminuser)
	IDsetsys(NULL,NULL,NULL,NULL,NULL,NULL,NULL, adminuser,NULL);

    /* strip off any leading path and get to the name part */

    if ( want = STrchr( argv[0], '/' ) )
	want++;
    else
	want = argv[0];

    /*
    ** Allow program names with format:
    **
    **     PROGNAME.my_name
    **
    ** where:
    **
    **     PROGNAME is one of the supported program names for IIMERGE
    **     my_name is an abitrary extension.
    **
    ** example: iidbms.rog
    */

    /*
    ** Check for a filename suffix - if there is one, set want_length to
    ** to exclude the suffix.
    */
    suffix = STchr(want, '.');
    if (suffix)
	want_length = (suffix - want);

    /* search for and execute the program implied in argv[0] */

    for( p = progs; p->name; p++ )
    {
	if( !STscompare( p->name, 0, want, want_length ) )
	{
	    /*
	    ** Set memory allocation strategy.
	    */
	    MEadvise( p->alloctype );

	    /* turn off setuid-ness if this is a usermode only program */
	    if( p->privs == USERMODE )
	    {
		if( setuid( getuid() ) < 0 )
		{
			/* FIXME: translatable? */
			perror("iimerge setuid");
		}
	    }
	    else if( p->privs == OWNERONLY )
	    {
		/* if not root and you are not the program owner, die. */

		if( getuid() && getuid() != geteuid() )
		{
		    /* FIXME: should be a looked up translatable message */
		    SIfprintf( stderr,
		    	"Only the INGRES user or root may run %s\n", want );
		    PCexit( FAIL );
		}

		/* if root, become the ingres user completely */
		if( !getuid() )
			(void)setuid( geteuid() );
	    }
	    else if( p->privs == SU_PRIVS )
	    {
		status = dmf_check_supriv();
		if ( status != OK )
		{
		    PCexit( FAIL );
		}

	    }
	    /* else SETUID */

	    if ( NUMA_SPECIAL != p->numaopt )
	    {
		/* Only SERVERS should enable "noisy" trace messages. */
		CXalter( CX_A_QUIET, (PTR)1 );
	    }

	    switch ( p->numaopt )
	    {
	    case NUMA_MIXED:
		if ( argc >= 2 && CXnuma_cluster_configured() )
		{
		    /*
		    ** Only jump these hoops if running NUMA clusters,
		    ** and user is looking for more than a dump of
		    ** the valid parameters.
		    */
		    if ( OK == CXget_context( &argc, argv,
			    CX_NSC_CONSUME|CX_NSC_SET_CONTEXT, NULL, 0 ) )
		    {
			/*
			** We're working on a local node, context is set,
			** and -node / -rad parameters have been eaten.
			** Utility will now operate exactly as if
			** it was directed at the local node in a
			** stand-alone or non-NUMA cluster installation.
			*/
			;
		    }
		    else if ( OK == CXget_context( &argc, argv, 
				     CX_NSC_CONSUME|CX_NSC_IGNORE_NODE|
				     CX_NSC_SET_CONTEXT, NULL, 0 ) )
		    {
			/*
			** Both a RAD and a NODE were specified, eat
			** the RAD, leave the node, and set NUMA context
			** based on RAD.
			*/
			;
		    }
		    else if ( OK == CXget_context( &argc, argv,
			      CX_NSC_NODE_GLOBAL, NULL, 0 ) )
		    {
			/*
			** Hmm, dicier.  -node parameter was provided,
			** but was directed against a remote node.
			** We can run on any local node, but none was
			** specified.
			*/
			local_host = PMhost();
			for ( rad = CXnuma_rad_count(); rad > 0; rad-- )
			{
			    if ( CXnuma_rad_configured( local_host, rad ) &&
				 OK == CXset_context(local_host, rad) &&
				 OK == CS_map_sys_segment(&sys_err) )
			    {
				/*
				** Found one.  Redundant CS_map_sys_segment
				** is harmless, since logic in ME_attach,
				** detects multiple attachments, and just
				** returns address previously assigned.
				*/
				break;
			    }
			}
			/*
			** If we did'nt find a context, we'll crash & burn
			** with the same an ugly E_CL2537_CS_MAP_SSEG_FAIL
			** fail to map shared memory error, that rcpstat,
			** and rcpconfig have always reported when
			** csinstall has not been run on the local node.
			*/
		    }
		    else
		    {
			/*
			** No context was provided at all, or bad or
			** multiple rad and/or node specs were specified.
			** Call once more, and yell at the user unless
			** -silent was specified.
			*/
			targ = argc;
			while ( --targ > 0 &&
				0 != STcompare(argv[targ], "-silent") )
			    ; /* do nothing */

			if ( targ == 0 )
			{
			    (void)CXget_context( &argc, argv,
						 CX_NSC_STD_NUMA, NULL, 0 );
			}
			PCexit(FAIL);
		    }
		}
		break;

	    case NUMA_ANY_RAD:
		if ( CXnuma_cluster_configured() )
		{
		    /*
		    ** Only jump these hoops if running NUMA clusters.
		    */
		    if ( OK == CXget_context( &argc, argv,
			    CX_NSC_CONSUME|CX_NSC_SET_CONTEXT, NULL, 0 ) )
		    {
			/*
			** We're working on a local node, context is set,
			** and -node / -rad parameters have been eaten.
			** Utility will now operate exactly as if
			** it was directed at the local node in a
			** stand-alone or non-NUMA cluster installation.
			*/
			;
		    }
		    else
		    {
			/*
			** No explicit context provided.  Search for
			** one.
			*/
			local_host = PMhost();
			for ( rad = CXnuma_rad_count(); rad > 0; rad-- )
			{
			    if ( CXnuma_rad_configured( local_host, rad ) &&
				 OK == CXset_context(local_host, rad) &&
				 OK == CS_map_sys_segment(&sys_err) )
			    {
				/*
				** Found one.  Redundant CS_map_sys_segment
				** is harmless, since logic in ME_attach,
				** detects multiple attachments, and just
				** returns address previously assigned.
				*/
				break;
			    }
			}
			if ( 0 == rad )
			{
			    /*
			    ** No context was found.
			    */
			    (void)CXget_context( &argc, argv,
						 CX_NSC_STD_NUMA, NULL, 0 );
			    PCexit(FAIL);
			}
		    }
		}
		break;

	    case NUMA_SPECIAL:
		/*
		** Check for special NUMA cluster usage of the
		** installation code.
		*/
		if ( CXnuma_cluster_configured() )
		{
		    /*
		    ** Desired RAD is passed in as part of the installation
		    ** code parameter in the form "<inst_id>.<rad>".
		    */
		    if ( p->func == dmfacp_libmain )
			targ = 1;
		    else
			targ = 3;

		    /* 
		    ** If for whatever reason, passed RAD is missing,
		    ** or invalid, we leave it to the App to detect this,
		    ** and report the error.  At the very least, it
		    ** will fail to map to shared memory.
		    */
		    if ( (argc > targ) && 
			 (STlength( argv[targ] ) >= 4) &&
			 ('.' == *(argv[targ]+2)) &&
			 (OK == CVan( argv[targ] + 3, &rad )) )
		    {
			local_host = PMhost();
			(void)CXset_context(local_host, rad);
		    }
		}
		break;

	    case NUMA_ANY_NODE:
		if ( OK != CXget_context(&argc, argv, CX_NSC_STD_CLUSTER,
					 NULL, 0) )
		{
		    PCexit( FAIL );
		}
		break;

	    case NUMA_LOCAL_RAD:
		if ( OK != CXget_context(&argc, argv, CX_NSC_STD_NUMA,
					 NULL, 0) )
		{
		    PCexit( FAIL );
		}
		break;

	    default:
		break;
	    }
	    return( (*p->func)(argc, argv) );
	}
    }

    /* not in the table.  too bad */
    MEadvise( ME_INGRES_ALLOC );

    /* FIXME: should be a looked up translatable message */
    SIfprintf( stderr, "\"%s\" shouldn't be linked to iimerge\n", want );

    return( FAIL );
}


/*
** debug entry point echos args and prints uid and euid.
*/

int
iimerge( argc, argv )
int argc;
char **argv;
{
    while( argc-- )
	SIprintf( "%s ", *argv++ );
    SIprintf("\nuid %d, euid %d\n", getuid(), geteuid() );
    return( OK );
}



/*{
** Name: dmf_check_supriv - Checks if user has required PM resource.
**
** Description:
**	If this process is SUPRIV check that the user has the 
**	required PM resource.
**
** Inputs:
**      err_code                        The error code to lookup.
**	sys_err				Pointer to associated CL error
**	p				Parameter for the error code.
**	ret_err_code			RCP error code.
** Outputs: 
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-oct-1993 (rmuth)
**	    Created
**	22-nov-1993 (bryanp) B56700
**	    Add a PMload error handler to detect/report errors in config.dat
*/
static
DB_STATUS
dmf_check_supriv( VOID )
{
    STATUS status = OK;
    char   *pm_privilege, *pm_ptr, *str_buf = 0;
    char   pm_res_name[ USER_NAME_SIZE + 1 + PM_START_PRIV_LEN];
    char   usrname[ USER_NAME_SIZE + 1 ];
    char   *usrnamep = usrname;
    int	   priv_len;   
    ER_ARGUMENT	    arg;

    do
    {
	PMinit();
	status = PMload((LOCATION *)NULL, pmerr_func);
	if ( status != OK )
	{
	    if (status != FAIL)
		pmerr_func(status, (i4)0, &arg);

	    SIfprintf(stdout, "Internal error initializing PM system\n");
	    SIflush(stdout);
	    break;
	}

	/*
	** Set the default node name
	*/
	PMsetDefault( 1, PMhost() );

	/*
	** Set the username
	*/

	IDname(&usrnamep);
	usrname[USER_NAME_SIZE] = EOS;
	(VOID) STtrmwhite(usrname);
	CVlower(usrname);

	/*
	** Generate the PM resource name
	*/
	STpolycat( 2, PM_START_PRIV_RES, usrname, pm_res_name );


	/*
	** If no privilege for user then bail
	*/
	status =  PMget(pm_res_name, &pm_privilege);
	if ( status == PM_NOT_FOUND )
	{
	    SIfprintf( stdout, 
		       "Your username: %s does not have any privilege\n",
		       pm_res_name );
	    SIflush(stdout);
	    break;
	}

	/*
	** Error encountered using PM
	*/
	if ( status != OK )
	{
	    SIfprintf( stdout, 
		       "Internal error searching for username privilege %s\n", 
		        pm_res_name);
	    SIflush(stdout);
	}

	/*
	** Scan the PM resource looking for required privilege
	*/
	priv_len = STlength( PM_START_PRIVILEGE );
	pm_ptr = pm_privilege;

	while ( (*pm_ptr != EOS) && (str_buf=STchr(pm_ptr, ',')))
	{
	    if (!STscompare(pm_ptr, priv_len, PM_START_PRIVILEGE, priv_len))
	    {
		return(OK);
	    }

	    /* 
	    ** skip blank characters after the ','
	    */ 
	    pm_ptr = STskipblank(str_buf+1, 10);

	}

        /* 
	** we're now at the last or only (or NULL) word in the string	
	*/
	if ( ( *pm_ptr != EOS ) &&
	     (!STscompare(pm_ptr, priv_len, PM_START_PRIVILEGE, priv_len)))
	{
	    return(OK);
	}
	   
	status = FAIL;

	SIprintf( "Username: %s has insufficent privilege\n",
		  pm_res_name );
	SIprintf( "Privilege required: %s\n", PM_START_PRIVILEGE);
	SIprintf( "Privilege's given : %s\n", pm_privilege);
	SIflush(stdout);

    } while ( FALSE );

    return( status );
}

/*
** Name: pmerr_func	    - handle PM errors.
**
** Description:
**	This routine is called if an error should arise while processing a
**	PM call. In particular, it is passed as the error-handling function
**	to PMload.
**
**	All that this function currently does is format the error message and
**	display it to the "screen".
**
** Inputs:
**	err_number	    - error message number.
**	num_arguments	    - number of entries in the args array.
**	args		    - arguments for use in formatting err_number
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	22-nov-1993 (bryanp)
**	    Created to respond to B56700.
*/
static VOID
pmerr_func( STATUS err_number, i4  num_arguments, ER_ARGUMENT *args)
{
    STATUS	status;
    char	message[ER_MAX_LEN];
    i4		text_length;
    CL_ERR_DESC	sys_err;

    status = ERslookup( err_number, (CL_ERR_DESC*)0,
			    0,
			    (char *)NULL,
			    message,
			    ER_MAX_LEN,
			    (i4) -1,
			    &text_length,
			    &sys_err,
			    num_arguments,
			    args
			  );

    if (status != OK)
    {
	SIprintf("IIDBMS: Unable to look up PM error %x\n", err_number);
    }
    else
    {
	SIprintf("IIDBMS: %.*s\n", text_length, message);
    }
    return;
}
# endif /* II_DMF_MERGE */

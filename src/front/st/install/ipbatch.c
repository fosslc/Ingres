/*
** Copyright (c) 1992, 2004 Ingres Corporation
**
*/
# include	<compat.h>
# include	<si.h>
# include	<st.h>
# include	<me.h>
# include	<gc.h>
# include	<ex.h>
# include	<er.h>
# include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<pc.h>
# include	<lo.h>
# include	<iplist.h>
# include	<erst.h>
# include	<uigdata.h>
# include	<stdprmpt.h>
# include	<ip.h>
# include	<ipcl.h>
# include	"ipparse.h"

/*
**  Name: ipbatch -- top level call to implement command-line mode
**
**  Entry point:
**
**    ip_batch -- see if we're in batch mode and handle it if so
**
**  History:
**      xx-xxx-92 (jonb)
**      	Created.
**      5-mar-93 (jonb)
**		Beautified and commented.
**	16-mar-1993 (jonb)
**		Brought comment style into conformance with standards.
**	13-jul-93 (tyler)
**		Changes to support the portable manifest language.
**	14-jul-93 (tyler)
**		Replaced changes lost from r65 lire codeline.
**	21-jul-93 (kellyp)
**		Changed MAX_TXT to MAX_MANIFEST_LINE
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer.
**	30-nov-93 (tyler)
**		Don't exit when II_AUTHORIZATION is undefined unless it's
**		really needed.
**	05-jan-94 (tyler)
**		Changed -packages option to -products.
**	22-feb-94 (tyler)
**		Fixed BUG 59511: Remove -output option which is useless
**		on Unix.
**      08-Apr-94 (michael)
**              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
**              appropriate.
**	17-feb-1995 (canor01)
**		changed 'GLOBALREF char *installDir' to 'GLOBALREF
**		char installDir[]' to prevent SEGV on call to IIUGerr()
**	02-jun-1995 (forky01)
**		Change check for ip_licensed to use temp var ptr to allow
**		recursion rather than moving along real distribution list.
**	11-nov-1995 (hanch04)
**		Made changes for new ip_is_visiblepkg and express mode.
**	18-march-1996 (angusm)
**		SIR 75443 - patch installation changes. Disregard '-patch'
**		flag - this is validated elsewhere.
**	16-oct-96 (mcgem01)
**		ingres_is_up becomes installation_is_up as part of the
**		Ingres/Jasmine merger.
**	09-jun-1998 (hanch04)
**		We don't use II_AUTH_STRING for Ingres II
**	12-jun-1998 (walro03)
**		Delineate comments on endif statement.
**	11-mar-199 (hanch04)
**		Added ignoreChecksum flag to ignore file mismatch errors.
**	07-jun-2000 (somsa01)
**		Modified E_ST0177 and E_ST017E for multiple products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Oct-2001 (gupsh01)
**	    Added exresponse flag for response file based installation.
**	06-Nov-2001 (gupsh01)
**	    Added support for user specified filename for mkresponse and
**	    exresponse option.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-Nov-2004 (sheco02)
**	    Add return code for ip_batch at the end.
**	31-may-2005 (abbjo03)
**	    Eliminate Licensed? column.
**	08-Jun-2005 (bonro01)
**	    Remove -licensed option
**	23-mar-2006 (bonro01)
**	    The install has been changed to display the LICENSE for all
**	    installs including interactive, express, and batch modes.
**	    Embedded installs will also be prompted for a LICENSE unless
**	    the -acceptlicense option is passed to ingbuild.
**      11-Dec-2006 (bonro01)
**          Disable the license prompt yet again. (third time).
**	    Allow -acceptlicense syntax for forward compatibility.
**       9-May-2007 (hanal04) Bug 117075
**          In exresponse mode only install the packages listed in the
**          response file unless the user has indicated otherwise.
**      20-Oct-2008 (hweho01)
**          For 9.2, the ice and ice64 packages can only be installed
**          in these two situations :
**          1)The package name is specified in option -install in command line.
**          2)In a upgrade install, the package was installed previously.
**	10-Dec-2009 (bonro01)
**	    Enable the license prompt yet again. (fourth time).
*/

#define LICENSE_PROMPT 1

GLOBALREF char	*instRelID;
GLOBALREF char	installDir[];
GLOBALREF LIST	instPkgList;
GLOBALREF LIST	distPkgList;
GLOBALREF bool	batchMode;
GLOBALREF bool	ignoreChecksum;
GLOBALREF bool	noSetup;
GLOBALREF bool	eXpress;
GLOBALREF FILE  *batchOutf;
GLOBALREF char  yesWord[], noWord[];
GLOBALREF char  distribDev[];   	   /* Name of distribution device */
GLOBALREF bool  authStringEnv;
GLOBALREF bool  pmode;
GLOBALREF bool  mkresponse;
GLOBALREF bool  exresponse;
GLOBALREF char  *respfilename; 
GLOBALREF bool  ingres_menu;

#if LICENSE_PROMPT
GLOBALREF bool	acceptLicense;
#endif

GLOBALDEF bool  batch_opALL = FALSE;
GLOBALDEF bool  Install_Ice = FALSE;


/*  Declarations of local functions... */

static char *argval, *featlist=NULL;
static bool apply_list(bool (*proc) (char *, bool));
static bool proc_install(char *, bool);
static bool proc_version(char *, bool);
static bool proc_describe(char *, bool);

#if LICENSE_PROMPT
void license_prompt();
#endif

/* Initialize the structure which defines the possible command-line options. */

typedef struct _option
{
    char *op_text;
    uchar op_id;
} OPTION;

/* Option codes; used internally. */

# define opNONE     0
# define opALL      1
# define opINSTALL  3
# define opPACKAGES 4
# define opVERSION  5
# define opOUTPUT   6
# define opDESCRIBE 7
# define opNOSETUP  8
# define opEXPRESS  9
# define opPATCH    10
# define opIGNORE   11
# define opEXRESPONSE 12
# define opRESPFILE 13
/* -acceptlicense is still accepted for forward compatibility */
# define opACCEPTLIC 14

/* Map option names onto codes.  */

OPTION option[] =
{
    ERx( "all" ),	opALL,
    ERx( "install" ),	opINSTALL,
    ERx( "products" ),	opPACKAGES,
    ERx( "version" ),	opVERSION,
    ERx( "describe" ),	opDESCRIBE,
    ERx( "nosetup" ),	opNOSETUP,
    ERx( "express" ),	opEXPRESS,
    ERx( "patch" ), 	opPATCH,
    ERx( "ignore" ), 	opIGNORE,
    ERx( "exresponse" ),opEXRESPONSE,
/* -acceptlicense is still accepted for forward compatibility */
    ERx( "acceptlicense" ),opACCEPTLIC,
    ERx( "file" ),      opRESPFILE
};

# define NUM_OPT (sizeof(option) / sizeof(option[0]))

/* 
** Parse a command-line flag, which may optionally be followed by an equal
** sign and a value.  If the value is present, set the global "argval" to
** point to it.  Returns the option code associated with the command-line
** flag, or opNONE if the flag couldn't be parsed.  Parse failure means
** that either the flag was completely bogus, or not enough was typed
** in to uniquely distinguish the flag from some other flag.
*/

static uchar
lookup(char *opname)
{
    i4  opn;
    uchar rtn = opNONE;
     
    /* 
    ** If there's an "=<value>" following the flag, point argval at the value,
    ** and then replace the "=" with an EOS... 
    */

    if (NULL != (argval = STindex(opname, ERx("="), 0)))
    {
	*argval = EOS;
	CMnext(argval);
	if (*argval == EOS)
	    return opNONE;
    }

    /* Look up the flag in the table... */

    for (opn = 0; opn < NUM_OPT; opn++)
    {
	if (!STbcompare(opname,99,option[opn].op_text,99,FALSE))
	{
	    if (rtn == opNONE)  /* First candidate we've found? */
		rtn = option[opn].op_id;
	    else
		return opNONE;  /* Not unique, apparently. */
	}
    }

    return rtn;
}

/*  Fairly lame usage message, and a macro to invoke it.  It would be nice
**  to actually give the user some notion of what went wrong.  At the moment,
**  we just blast out a long description of all the possible options for
**  any kind of error whatsoever.
*/

static VOID
usage()
{
    if(ingres_menu) {
    SIfprintf(stderr, ERget(E_ST0177_usage_1), SystemExecName);
    SIfprintf(stderr, ERget(E_ST017E_usage_2), SystemExecName);
    }
    else {
    SIfprintf(stderr, ERget(E_ST0534_usage_3),SystemExecName, SystemExecName);
    }
}

# define USAGE { usage() ; return FAIL; }


/*  ip_batch()
**
**  This function handles all operations which are controlled by options
**  on the command line.  
**
**  Note that we're using the standard argc/argv approach to parsing the
**  optiones, rather than UT routines, because, since this is an installation
**  program, we can't count on the UT control files being present.
*/

STATUS
ip_batch(i4 argc, char *argv[])
{
    i4  argvx;
    PKGBLK *pkg;
    uchar opid;
    uchar option = opNONE;
    LOCATION outloc;
    char line[ MAX_MANIFEST_LINE + 1 ];
    LISTELE *lp;

    /* no command line arguments except mkresponse 
    ** or exresponse allowed for ingbuild in form mode
    */
    if ((argc < 2) || 
	((argc ==2) && ((pmode == TRUE) || 
           		(mkresponse == TRUE) || (exresponse == TRUE))) ||
	((argc ==3) && (((mkresponse == TRUE) || (exresponse == TRUE)) && 
			 (respfilename != NULL)))) 
	/* No command-line args? */
	return OK; /* Fine, must be forms mode; return without setting flag. */

    batchMode = TRUE;   /* We're in command-line mode. */
    batchOutf = stdout; /* For now, we'll direct output to the terminal. */

    /* Top of loop through the non-positional arguments on the command line. */

    for (argvx = 1; argvx < argc; argvx++)
    {
	if (argv[argvx][0] != '-') /* Not a switch?  Time for positional arg. */
	    break;

        /* Get the option code for the current flag and deal with it... */

	switch (opid = lookup(&argv[argvx][1]))  
	{
	    /* instruct ingbuild to skip set up */

	    case opNOSETUP:  

		noSetup = TRUE;
		break;

	    case opEXPRESS:  

		eXpress = TRUE;
		break;

	    case opPATCH:
		continue;

	    /* -install=<list of features> */

	    case opINSTALL:  
		if (option != opNONE || argval == NULL)
			USAGE;
		option = opid;
		featlist = argval;
		if( !STbcompare("ice", 3, featlist, STlength(featlist), TRUE) ||  
		    !STbcompare("ice64", 5, featlist, STlength(featlist), TRUE) )
		    Install_Ice = TRUE; 
		break;

	    /* 
	    ** -version, -products, -all, -describe
	    ** (All the options that don't require values.) 
	    */

	    case opVERSION:
	    case opPACKAGES:
	    case opDESCRIBE:
	    case opALL:

		if (option == opNONE)
		{
		    featlist = argval;
		    option = opid;
		    break;
		}
	    case opIGNORE:
		ignoreChecksum = TRUE;
		break;
	
	    case opEXRESPONSE:
		exresponse = TRUE;
		break;

	    case opRESPFILE:
		break;

/* -acceptlicense is still accepted for forward compatibility */
	    case opACCEPTLIC:
#if LICENSE_PROMPT
		acceptLicense = TRUE;
#endif
		break;

	    /* Fall through */
	    default:
		USAGE;
	}
    }

    /* 
    ** When we break from the flags loop, we must have a maximum of one
    ** remaining argument... 
    */

    if (argvx < (argc - 1))  /* Too many non-flags at the end of the line. */
	USAGE;

    if (exresponse && (option == opNONE))
    {
        char        *def_file="ingrsp.rsp";
        char        *filename;
        FILE        *respfile;

        if (respfilename != NULL)
            filename = respfilename;
        else
            filename = def_file;

        if (OK == ip_open_response (ERx(""), filename, &respfile, ERx( "r" )) )
        {
            bool        found = FALSE;
            char	line[MAX_MANIFEST_LINE + 1];
            char        *pkg_set = ERx("_set=YES");
            char        *setp, *realloc;
            i4          pkglen;
            i4          buflen = MAX_MANIFEST_LINE + 1;
            i4          space = buflen;

            while (SIgetrec((char *)&line, MAX_MANIFEST_LINE, respfile) == OK )
            {
                /* Check for _set=YES */
                setp = STstrindex(line, pkg_set, 0, 1);
                if(setp != NULL) 
                {
                    if(found == FALSE)
                    {
                        featlist = ip_alloc_blk( buflen );
                    }

                    *setp = NULLCHAR;
                    pkglen = STlength(line);
                     
                    if(found == TRUE)
                    {
                        if(space < (pkglen + 1))
                        {
                            buflen += MAX_MANIFEST_LINE;
                            realloc = ip_alloc_blk( buflen );
                            STcopy(featlist, realloc);
                            MEfree(featlist);
                            featlist = realloc;
                        }
                        STcat(featlist, ",");
                        space--;
                    }
                    else
                    {
                        found = TRUE;
                    }
                    STcat(featlist, line); 
                    space = space - pkglen;
                }

            } /* end read loop */

            ip_close_response(respfile);

            if( found == TRUE)
            {
                /* Need to test this does not override any other options that
                ** may be valid
                */
                option = opINSTALL;
            }
        }
        else
        {
            PCexit ( FAIL );
        }
    }    

    /* 
    ** If we have no positionals, no distribution device was provided on
    ** the command line.  That might or might not be ok... 
    */

    if (argvx == argc) 
    {
	switch (option)
	{
	    case opPATCH:
	    case opVERSION:
	    case opPACKAGES:
	    case opDESCRIBE:
		break;       /*  ...yes.  */

	    default:
	        USAGE;    /*  ...no.   */
	}
    }
    else  /* We got a positional, so copy it into the global field. */
    {
	STcopy(argv[argvx], distribDev);
	if( !IPCLsetDevice( distribDev ) )
	    USAGE;
    }

    if (option == opNONE)    /* No option?  Default to "all". */
	option = opALL; 

    if ( option == opALL );
       batch_opALL = TRUE; 

    /*  
    **  First processing pass.  If the option doesn't involve doing actual
    **  installation, just do whatever needs to be done, and return.
    **  If it's an installation option, mark all requested packages
    ** 	as selected, and continue.  
    */

    switch (option)
    {
	case opPACKAGES:
	    SIfprintf(batchOutf,ERget(S_ST0179_pkg_list_header));

            for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
            {
                pkg = (PKGBLK *) lp->car;

		if( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) )
		{
		    SIfprintf( batchOutf, ERx("\t%-16s  %-30s\n"),
			pkg->feature,pkg->name );
		}
	    }

	    return OK;

	case opVERSION:
	    if (instRelID == NULL)
	    {
		IIUGerr(E_ST017B_not_present,0,1,SystemProductName);
		return FAIL;
	    }

	    if (featlist == NULL)
	    {
		SIfprintf(batchOutf, ERx("%s\n"), instRelID);
		return OK;
	    }

	    if (!apply_list(proc_version))
		return FAIL;

	    return OK;

	case opINSTALL:
	    SCAN_LIST(distPkgList, pkg)
	    {
		pkg->selected = NOT_SELECTED;
	    }

	    if (!apply_list(proc_install))
		return FAIL;

	    break;

	case opDESCRIBE:
	    _VOID_ apply_list(proc_describe);
	    return OK;

	default:
	    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
	    {
                pkg = (PKGBLK *) lp->car;

		switch (option)
		{
		    case opALL:
			if ( ip_is_visible(pkg) || ip_is_visiblepkg( pkg ) )
			    pkg->selected = SELECTED;
			break;
		}
	    }
    }

    /* 
    ** If we haven't left yet, that must mean we have an install-class
    ** option, i.e. one that's going to cause us to actually install 
    ** something.  First of all, let's make sure that the user has
    ** defined II_AUTH_STRING and has selected something to install... 
    */

#ifndef INGRESII
    if( !authStringEnv )
    {
	IIUGerr( E_ST0117_license_required, 0, 0 );
	return( FAIL );	
    }
#endif /* INGRESII */

    SCAN_LIST(distPkgList, pkg)
    {
	if (pkg->selected == SELECTED)
	    break;
    }

    if (pkg == NULL) /* Got all the way through the list?  Nothing selected! */
	USAGE;

    /* If Ingres is up, no go; otherwise, do the installation... */

    switch (option)
    {
	case opALL:
	case opINSTALL:
	    if (ip_sysstat() == IP_SYSUP) 
	    {
		IIUGerr(E_ST0178_installation_is_up, 0, 2, 
			SystemProductName, installDir);
		return FAIL;
	    }

#if LICENSE_PROMPT
	    if( !acceptLicense )
		license_prompt();
#endif

	    ip_install(NULL);
	    break;
    }
return OK;
}

/* apply_list -- apply a function to the list of specified packages.
**
** The processing functions are defined below.  By convention, each is
** called with a text featurename, which identifies a single package, and
** a boolean which is TRUE if this is the only package in the list, FALSE
** otherwise.  The processing functions are boolean, and return TRUE if
** the identified package exists, FALSE otherwise.
*/

static bool
apply_list( bool (*proc) (char *, bool) )
{
    char *featname, *featnext;
    bool only;
    char savechar;
    bool rtn = TRUE;

    /* 
    ** Flag whether there's only one package in the list.  Some of the
    ** processing functions care.  
    */

    only = (NULL == STindex(featlist, ERx(","), 0));

    /* Loop through each feature name in the list... */

    for (featname = featlist; featname && *featname;)
    {
	/* Find the next one, and make sure current one is EOS-terminated... */

	featnext = STindex(featname, ERx(","), 0);
	if (featnext != NULL)
	{
	    savechar = *featnext;
	    *featnext = EOS;
	}

        /* Call the processing routine... */

	if (!(*proc)(featname,only))
	    rtn = FALSE;

  	/* Onward through the fog... */

	if (featnext == NULL)
	    break;

	*(featname = featnext) = savechar;
	CMnext(featname);
    }

    return rtn;
}

/* proc_install -- process the feature list in a -install command.
**
** Select the feature identified by "featname" for installation.
*/

static bool
proc_install(char *featname, bool only)
{
    PKGBLK *pkg;

    SCAN_LIST(distPkgList,pkg)
    {
	if (!STbcompare(featname, 0, pkg->feature, 0, TRUE))
	{
	    pkg->selected = SELECTED;
	    return TRUE;
	}
    }

    IIUGerr(E_ST017A_bad_featname, 0, 1, featname);
    return FALSE;
}

/* proc_version -- process the feature list in a -version command.
**
** Display the version ID of the feature identified by "featname".
*/

static bool
proc_version(char *featname, bool only)
{
    PKGBLK *pkg;

    SCAN_LIST(instPkgList,pkg)
    {
	if (!STbcompare(featname, 0, pkg->feature, 0, TRUE))
	{
	    if (!only)  /* Don't identify the package if there's only one. */
		SIfprintf(batchOutf, ERx("%s "), pkg->feature);

	    SIfprintf(batchOutf, ERx("%s\n"), pkg->version);
	    return TRUE;
	}
    }

    IIUGerr(E_ST017B_not_present, 0, 1, featname);
    return FALSE;
}

/* proc_describe -- process the feature list in a -describe command.
**
** Display the descriptive text for the package identified by "featname".
*/

static bool
proc_describe(char *featname, bool only)
{
    PKGBLK *pkg = ip_find_feature(featname, DISTRIBUTION);

    if (pkg != NULL)
	ip_describe(pkg->name);

    return TRUE;
}

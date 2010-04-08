/*
**  Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <cm.h>
#include <cv.h>
#include <er.h>
#include <ex.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <qu.h>
#include <si.h>
#include <st.h>

#include <dbms.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcu.h>
#include <gcaimsg.h>


/**
**
**  Name: iinamu.c
**
**  Description:
**	This is the Name Server Managment Utility. It contains the following
**	module and functions:
**	gcn_u_s - Stop the Name Server.
**	gcn_u_r - Modify GCF Server REGISTRATION Information.
**
**
**  History:
**	    
**      08-Sep-87   (lin)
**          Initial creation as NSMU (Combined NAMU and NETU).
**	20-Mar-88   (lin)
**	    Formally coded the routines for each function.
**	05-Sep-88   (jorge)
**	    Converted NSMU to NAMU only functionality.
**	01-Mar-89 (seiwald)
**	    Revamped.  checkerr() moved in.
**	05-Apr-89 (jorge)
**	    Added initial call to MEadvise.
**	09-May-89 (jorge)
**	    Fixed 1989 copyright
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**      21-Feb-90 (cmorris)
**	    Added call to EXdeclare to set up GCX tracing exception handler.
**	03-Jan-91 (brucek)
**	    Enhanced ADD operation to accept the following options:
**	        1) sole flag allows user to add sole server entries
**	        2) merge flag allows user to add new entries for a previously
**	           registered server
**	    Allow abbreviations for ADD, DEL, SHOW, and HELP commands.
**      09-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	23-apr-91 (seiwald)
**	    Make stevet's change conditionally compile with GCF63.
**	24-May-91 (brucek)
**	    Fixed 1991 copyright
**	14-Aug-91 (seiwald)
**	    Flattened.
**	14-Aug-91 (seiwald)
**	    Removed nonsensical use of PTR in gcn_get_line().
**	    Added missing 'nc' (noncase) parameter to STbcompare.
**	    Included <st.h>.
**	14-Aug-91 (seiwald)
**	    Added (undocumented) flag "mine" to "show" so that I can
**	    dump out private entries.
**	13-aug-91 (stevet)
**	    Changed read-in character set support to use II_CHARSET.
**	26-Dec-91 (seiwald)
**	    Added (undocumented) "test" command for testing addresses.
**	15-Apr-92 (brucek)
**	    Added call to gcn_bedcheck to see if Name Server is up.
**	17-apr-92 (seiwald)
**	    Changed (undocumented) "mine" flag on show to be showmine,
**	    delmine, and addmine for manipulating private entries.
**	2-June-92 (seiwald)
**	    Added gcn_host to GCN_CALL_PARMS to support connecting to a remote
**	    Name Server.
**	2-July-92 (jonb)
**	    Nullify gcn_response_func to avoid new netutil functionality.
**	05-Jan-93 (brucek)
**	    Use gcn_fastselect for SHUTDOWN function.
**	29-Jan-93 (brucek)
**	    Expect E_GC0040_CS_OK on fastselect SHUTDOWN.
**      04-Oct-93 (joplin)
**          Changed (undocumented) "test" command's interface to gcn_testaddr()
**          to send a NULL for the new username parameter.
**	19-jan-94 (edg)
**	    Added standard argc and argv arguments to main() so that this
**	    will compile w/o error on Windows NT.
**	10-Feb-94 (brucek)
**	    Call gcn_checkerr when gcn_bedcheck() fails.
**	29-Nov-95 (gordy)
**	    Standardize name server client interface, add terminate call.
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	 8-Dec-95 (gordy)
**	    Change call to gcn_bedcheck() to IIgcn_check() which also
**	    checks for the embedded Name Server interface.  Initialize PM.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	29-oct-96 (hanch04)
**	    prompt needs to be upper case
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	17-Mar-98 (gordy)
**	    Use GChostname() for effective user ID.
**	14-sep-98 (mcgem01)
**	    Update the copyright notice.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	11-may-99 (hanch04)
**	    Update the copyright notice.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-jul-2002 (somsa01)
**	    Corrected fix to bug 106443, 106157.
**	26-mar-2003 (somsa01)
**	    Added check for "del", inadvertently removed when we started
**	    using STncasecmp().
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	24-Aug-09 (gordy)
**	    Remove string length restrictions.
**	26-Oct-2009 (whiro01) SIR 122796
**	    Allow a single command to be specified on the command line which
**	    does an automatic quit once it is done.
**	    Update copyright year on sign-on message.
**/

/*
**      mkming hints
NEEDLIBS =      GCFLIB COMPATLIB 

OWNER =         INGUSER

PROGRAM =       (PROG1PRFX)namu
*/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN	i4		GCX_exit_handler(EX_ARGS *);

static i4	gcn_line_get( char *prompt, char *in_line );


/*{
** Name: main - The main entry routine for the Name Server Management 
**		Utility(IINAMU).
**
** Description:
**	The NMAU currently supports two types functions:
**	Stop the Name Server.
**	Manipulate (Show, Add, Delete) GCF Server REGISTRATION Information.
**
**	The main routine is the top loop prompting the user to choose the
**	function item. It then call the corresponding routine to prompt
**	the parameters for that specified function if they are not provided.
**	All actions needed for
**	that function are implemented inside that functional routine.
**	When the user hits the <esc> key, control is returned back to the
**	top loop propmt.
**
** Inputs:
**	The desired action.
**
** Outputs:
**	The outpu messages for the desired action.
**
** Returns:
**	Zero exit completion code if sucessfull.
**
** Exceptions:
**	    None
**
** Side Effects:
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	26-jun-89 (seiwald)
**	    PCexit( OK ) as long as initialization succeeds.
**      09-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	13-aug-91 (stevet)
**	    Changed read-in character set support to use II_CHARSET.
**	27-jul-93 (tyler)
**	    Added missing call to PMinit().
**	29-Nov-95 (gordy)
**	    Standardize name server client interface, add terminate call.
**	 8-Dec-95 (gordy)
**	    Change call to gcn_bedcheck() to IIgcn_check() which also
**	    checks for the embedded Name Server interface.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**	20-oct-96 (mcgem01)
**	    Messages changed to allow for execuatble name change for Jasmine.
**	29-oct-96 (hanch04)
**	    prompt needs to be upper case
**      14-Jan-2000 (fanra01)
**          Bug 100030
**          Changed prompt name to be from compile time rather than run time
**          as prompt name is affected by inconsistencies with argv[0].
**      30-oct-2001 (ashco01) Bug 106157
**          Reduced user prompt to the name of the iinamu executable rather
**          than the full path and filename.  
**	10-jul-2002 (somsa01)
**	    Corrected fix to bug 106443, 106157.
**	17-Jun-2004 (schka24)
**	    Use canned/safe routine for setting charset.
**	06-Sep-2005 (wansh01)
**	    Added showmsg3 to SHOW [SVR_TYPE] help.
**	    bug 115153
**	24-Aug-09 (gordy)
**	    User a more appropriate size for user ID buffer.
**	26-Oct-2009 (whiro01) SIR 122796
**	    Allow a single command to be specified on the command line which
**	    does an automatic quit once it is done.
**	    Update copyright year on sign-on message.
*/

char prompt[32];	

/* Informational messages */
char showmsg1[] = 
"   SHOW [SVR_TYPE]                     - Show list of registered servers,\n";
char showmsg2[] =
"                                         %s is default type\n";
char showmsg3[] =
"                                         SERVERS lists all registered servers\n";
char addmsg1[] = 
"   ADD  SVR_TYPE OBJ_NAME GCF_ADDRESS  - Add to list of registered\n";
char addmsg2[] = 
"                                         servers (Superuser only)\n";
char delmsg1[] = 
"   DEL  SVR_TYPE OBJ_NAME GCF_ADDRESS  - Delete from list of registered\n";
char delmsg2[] = 
"                                         servers (Superuser only)\n";
char stopmsg[] = 
"   STOP                                - Stop Name Service (Superuser only)\n";
char helpmsg[] =
"   HELP                                - Displays command information\n";
char quitmsg[] =
"   QUIT                                - QUIT %s\n";

char badpmsg[] = "Incorrect parameters, try: \n";

#define		MAXTOKEN	8

int
main( int argc, char **argv )
{
    char	line[MAXLINE];
    char	uid[ GC_USERNAME_MAX + 1 ];
    i4  	ntoken;
    char 	*token[MAXTOKEN];
    char	*hostvar = NULL;
    EX_CONTEXT	context;
    STATUS      ret_status;
    CL_ERR_DESC cl_err;
    char        cmd_name[MAX_LOC + 1], cmd_prior_name[MAX_LOC + 1];
    char        null_str[MAX_LOC + 1];
    LOCATION    loc;
    int         arg;
    int         single_mode = 0;
    int         all_done;


    /* Issue initialization call to ME */
    MEadvise(ME_INGRES_ALLOC);

    /* Declare the GCA real-time tracing exception handler. */
    EXdeclare(GCX_exit_handler, &context);

    SIeqinit();

    /* setup command prompt from program name */
    STcopy(argv[0], cmd_prior_name);

    LOfroms(FILENAME & PATH, cmd_prior_name, &loc);
    LOdetail(&loc, null_str, null_str, cmd_name, null_str, null_str);

    STprintf ( prompt, ERx("%s> " ), cmd_name);
    CVupper( prompt );

    /* 
    **	Get character set logical.
    */

    ret_status = CMset_charset(&cl_err);
    if ( ret_status != OK )
    {
	SIprintf("Error while processing character set attribute file.\n");
	PCexit(FAIL);
    }
    
    PMinit();

    switch( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) )
    {
	case OK:
	    /* loaded successfully */
	    break;

	case PM_FILE_BAD:
	    gcu_erlog( 0, 1, E_GC003D_BAD_PMFILE, NULL, 0, NULL );
	    break;

	default:
	    gcu_erlog( 0, 1, E_GC003E_PMLOAD_ERR, NULL, 0, NULL );
	    break;
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );

    if ( IIGCn_call( GCN_INITIATE, NULL ) != OK )
        PCexit( FAIL );

    if ( IIgcn_check() != OK )
    {
	IIGCn_call( GCN_TERMINATE, NULL );
	PCexit( FAIL );
    }

    GCusername( uid, sizeof( uid ) );

    /* Decide if we are in "single command" mode by looking for passed in stuff on cmd line */
    if (argc > 1)
    {
	ntoken = 0;
	for (arg = 1; arg < argc; arg++)
	{
	    /* If param begins with "-v" or "/v" then the following param */
	    /* (if any) is a static or dynamic vnode designation.         */
	    char first = argv[arg][0];
	    if (first == '-' || first == '/')
	    {
		char second = argv[arg][1];
		if (second == 'v' || second == 'V')
		{
		    arg++;
		    if (arg < argc)
			hostvar = argv[arg];
		    continue;
		}
		/* Syntax errors will be caught simply by putting */
		/* the unrecognized stuff into the token list.    */
	    }

	    /* For now we will silently ignore stuff that is too long */
	    if (ntoken < MAXTOKEN)
		token[ntoken++] = argv[arg];
	    single_mode = 1;
	}
    }

    if ( !single_mode )
    {
	SIprintf("%s NAME SERVICE MANAGEMENT UTILITY --\n", SystemProductName);
	SIprintf("-- Copyright (c) 2009 Ingres Corporation\n");
    }

    for ( all_done = 0; !all_done; all_done = single_mode )	/* TOP LOOP */
    {
	i4		i;
	GCN_CALL_PARMS 	gcn_parm;
	STATUS		status;

	gcn_parm.gcn_host = hostvar;
	gcn_parm.gcn_response_func = NULL;
	gcn_parm.gcn_flag = 0;
	gcn_parm.gcn_uid = uid;
	gcn_parm.gcn_install = SystemCfgPrefix;

	if ( !single_mode )
	{
	    if(gcn_line_get(prompt, line) == EOF)
		break;	/* End of file - scan no more devices */

	    /* Parse input line for the tokens */

	    ntoken = MAXTOKEN;
	    STgetwords(line, &ntoken, token);

	    if(!ntoken)		/* if there are any commands */
		continue;
	}

	if( !STncasecmp(token[0], "help", 4))
	{
	    SIprintf("Commands are: \n");	/* HELP */
	    SIprintf(stopmsg);
	    SIprintf(showmsg1);
	    SIprintf(showmsg2, SystemDBMSName);
	    SIprintf(showmsg3);
	    SIprintf(addmsg1);
	    SIprintf(addmsg2);
	    SIprintf(delmsg1);
	    SIprintf(delmsg2);
	    SIprintf(helpmsg);
	    SIprintf(quitmsg, argv[0]);
	}
	else if( !STcasecmp(token[0], "stop" ))
	{		
	    if( gcn_line_get( "STOP NAME SERVICE: Are you sure? (y/n)> ", line )
		== EOF || *line != 'y' && *line != 'Y' )
		    break;

	    status = IIGCn_call( GCN_SHUTDOWN, NULL );

	    if ( status == E_GC0040_CS_OK )
            {
	        SIprintf("Name Service is stopped gracefully\n");
	        SIflush(stdout);
		break;
            }

	}
	else if( !STncasecmp(token[0], "show", 4 ) ||
		 !STncasecmp(token[0], "showmine", 8 ) )
	{
	    switch( ntoken )
	    {
	    case 1:
		gcn_parm.gcn_type = SystemDBMSName; break;

	    case 2:
		gcn_parm.gcn_type = token[1]; break;

	    default:
		SIprintf(badpmsg);
		SIprintf(showmsg1);
		SIprintf(showmsg2, SystemDBMSName);
		SIprintf(showmsg3);
		continue;
	    }

	    if( STcasecmp(token[0], "showmine" ) )
		gcn_parm.gcn_flag |= GCN_PUB_FLAG;

	    gcn_parm.gcn_obj = "";
	    gcn_parm.gcn_value = "";

	    status = IIGCn_call( GCN_RET, &gcn_parm );
	}
	else if( !STncasecmp(token[0], "add", 3 ) ||
		 !STcasecmp(token[0], "addmine" ))
	{
	    if( ntoken < 4 )
	    {
		SIprintf(badpmsg);
		SIprintf(addmsg1);
		SIprintf(addmsg2);
		continue;
	    }

	    if( STcasecmp(token[0], "addmine" ) )
		gcn_parm.gcn_flag |= GCN_PUB_FLAG;

	    gcn_parm.gcn_type = token[1];
	    gcn_parm.gcn_obj = token[2];
	    gcn_parm.gcn_value = token[3];

	    for( i = 4; i < ntoken; i++ )
	    {
		if (! STcasecmp(token[i], "sole" ) )
    		    gcn_parm.gcn_flag |= GCN_SOL_FLAG;

		if (! STcasecmp(token[i], "merge" ) )
    		    gcn_parm.gcn_flag |= GCN_MRG_FLAG;
	    }

	    status = IIGCn_call( GCN_ADD, &gcn_parm );
	}
	else if( !STncasecmp(token[0], "del", 3 ) ||
		 !STncasecmp(token[0], "delete", 6 ) ||
	         !STncasecmp(token[0], "delmine", 7 ))
	{
	    if( ntoken != 4 )
	    {
		SIprintf(badpmsg);
		SIprintf(delmsg1);
		SIprintf(delmsg2);
		continue;
	    }

	    if( STcasecmp(token[0], "delmine" ) )
		gcn_parm.gcn_flag |= GCN_PUB_FLAG;

	    gcn_parm.gcn_type = token[1];
	    gcn_parm.gcn_obj = token[2];
	    gcn_parm.gcn_value = token[3];

	    status = IIGCn_call( GCN_DEL, &gcn_parm );
	}
	else if( !STncasecmp(token[0], "quit", 4 ))
	{
		break;
	}
	else if(!STncasecmp( token[0], "test", 4 ))
	{
	    status = gcn_testaddr( token[1], 0, NULL );
	}
	else
	{			/* Bad command */
		SIprintf("Not a valid function, try:\n");
		SIprintf(helpmsg);
	} 

	SIflush(stdout);

    }	/* End of FOR */

    IIGCn_call( GCN_TERMINATE, NULL );

    PCexit( OK );
}

static i4
gcn_line_get( char *prompt, char *in_line )
{
    char    line[MAXLINE];
    char    *p;
    char	ret = '\n';

    SIprintf(prompt);
    SIflush(stdout);
    if (SIgetrec(line, (i4) MAXLINE,stdin) != OK)
	return(EOF);	/* End of file - scan no more devices */
    if(line[0] == '\033')
	return(EOF);
    if (p = STchr(line, ret))
	*p = '\0';
    STcopy(line,in_line);
    return(OK);
}

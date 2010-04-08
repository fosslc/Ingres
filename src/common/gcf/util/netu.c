/*
** Copyright (c) 2004, 2009 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>

#include <cm.h>
#include <er.h>
#include <id.h>
#include <gc.h>
#include <lo.h>
#include <me.h>
#include <pc.h>
#include <pm.h>
#include <qu.h>
#include <si.h>
#include <st.h>
#include <te.h>		/* for TEsave(), TErestore() */

#include <sl.h>
#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcnint.h>
#include <gcu.h>
#include <gcaimsg.h>
#include <gccer.h>

/**
**
**  Name: netu.c 
**
**  Description:
**	This is the Network Management Utility. It contains the following module
**	and functions:
**	gcn_u_a - Define Remote Login Information.
**	gcn_u_n - Define Node information.
**	gcn_u_o - Define Other Information.
**
**
**  History:    $Log-for RCS$
**	    
**      08-Sep-87   (lin)
**          Initial creation.
**	20-Mar-88   (lin)
**	    Formally coded the routines for each function.
**	20-Jan-89 (jorge)
**	    Upgraded to strip blanks from input for everything but noecho
**	9-Feb-89 (seiwald)
**	    Echo-off support for entering passwords.
**	    Ideally, we would just call a TEnoecho() before the prompt and 
**	    TEecho() after, but there is no TEnoecho() call.  We use 
**	    TErestore() instead, and must write our own char-by-char 
**	    gcn_getrec() function, because TErestore() turns off newline
**	    processing.  check_err() moved in.
**	2-Mar-89    (jorge)
**	    fixed bug 4878, and no echo of password
**	6-Mar-89 (jorge)
**	    Did additional fix for 4878, used REMOTE USERNAME instead of LOCAL
**	28-Mar-89 (jorge)
**	    Remerged with Seiwald's updates.
**	05-Apr-89 (jorge)
**	    Added initialcamm to MEadvise.
**	09-May-89 (jorge)
**	    Fixed 1989 copyright and the bad spelling of INGRES
**	22-May-89 (seiwald)
**	    Use MEreqmem().
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	18-Jul-89 (jorge)
**	    Added ifdef as quick fix to SUN bug for deafult Protocol.
**	    Will add seperate GCN CL header file in next release.
**	09-Sep-89 (seiwald)
**	    Commands to Comm Server are now routed through the gcc_doop()
**	    routine, which makes a GCA_REQUEST call with the aux_data_type 
**	    to invoke special services (like SHUTDOWN).
**	19-Sep-89 (davidg)
**	    If netu was invoked without a comm. server id, it is now possible
**	    to issue a STOP command. The usage is: s <Comm_Server_Id>.
**	15-Nov-89 (davidg)
**	    New command added: quiesce a comm. server. This command will
**	    send a GCA_CS_QUIESCE message to the comm. server. The comm.
**	    server will stop accepting any more connect requests. When
**	    the last connection is terminated, the comm. server will stop.
**	05-Dec-89 (seiwald)
**	    gcn_doop and gcn_usr_validate moved out.
**	    Now uses the new gcn_fastselect() for shutdown and quiesce
**	    commands.
**      09-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	23-apr-91 (seiwald)
**	    Make stevet's change conditionally compile with GCF63.
**	07-May-91 (brucek)
**	    Numerous minor enhancements:
**	    Implemented merge operation for node entries (this allows
**	    users to add multiple entries for same vnode).  This function
**	    was actually occurring for all adds due to previous enhancement,
**	    but we are now going back to the old behavior (delete any previous
**	    entry with same vnode name) by default and have added a new
**	    prompt for the merge operation.
**	    Also added exit prompt to return from node or auth submenus,
**	    made password prompt for user authorization insist on a value,
**	    and made abbreviations for all operations consistently usable,
**	    and corrected STOP logic so that error message is displayed if
**	    unable to shut down Comm Server.
**	24-May-91 (brucek)
**	    Fixed 1991 copyright
**	13-aug-91 (stevet)
**	    Changed read-in character set support to use II_CHARSET. 
**	14-Aug-91 (seiwald)
**	    Removed nonsensical use of PTR in gcn_get_line().
**	20-Nov-91 (seiwald)
**	    Goto labels must be unique for OS/2 compiler.
**	26-Mar-92 (seiwald)
**	    Support for installation passwords: allow a user name of *.
**	21-Apr-92 (brucek)
**	    Added call to gcn_bedcheck to see if Name Server is up.
**	29-Apr-92 (brucek)
**	    Added #ifdef OS2 for default protocol/listen address.
**	2-June-92 (seiwald)
**	    Added gcn_host to GCN_CALL_PARMS to support connecting to a remote
**	    Name Server.
**	30-Jun-92 (alan) bug #45160
**	    Move the STzapblank in gcc_line_get prior to the check 
**	    on a null input string.
**	2-July-92 (jonb)
**	    Set gcn_response_func to null to disable netutil functionality.
**	29-Oct-92 (brucek)
**	    Get rid of gcn_usr_validate calls.  (Now we check for required
**	    privileges on the server side of these connections.)
**	16-Jan-93 (mrelac)
**	    Added #ifdef hp9_mpe for default protocol/listen address.
**	29-Jan-93 (brucek)
**	    Look for E_GC0040_CS_OK instead of E_GC2002_SHUTDOWN.
**	10-Jun-93 (brucek)
**	    Add support for =<userid> flag to allow privileged users
**	    to access private entries on behalf of other users.
**	11-aug-1993 (shailaja)
**	    Fixed QAC warning in gcn_complete.
**	19-jan-94 (edg)
**	   Added ifdef WIN32 for Windows NT default protocol of wintcp and
**	   listen address of II0.
**	10-Feb-94 (brucek)
**	    Call gcn_checkerr() when gcn_bedcheck() fails.
**	24-may-95 (emmag)
**	   Changed copyright notice to Computer Associates Intl, Inc.
**	29-Nov-95 (gordy)
**	    Standardize name server client interface, add terminate call.
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	 8-Dec-95 (gordy)
**	    Change call to gcn_bedcheck() to IIgcn_check() which also
**	    checks for the embedded Name Server interface.  Initialize PM.
**	10-oct-1996 (canor01)
**	    Use SystemCfgPrefix to allow system variable override.
**      20-oct-96 (mcgem01)
**          Change messages to allow this to be called something else
**          for Jasmine.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	18-Aug-97 (rajus01)
**	    Added support for specifying security mechanism in conjunction
**	    to extending the vnode database for the name server.
**	    Added gcn_u_o(), gcn_opcode_o() routines.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for request to Name Server network database.
**	17-Mar-98 (gordy)
**	    Use GChostname() for effective user ID.
**	14-sep-98 (mcgem01)
**	    Update the copyright notice.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      11-may-99 (hanch04)
**          Update the copyright notice.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-Mar-02 (gordy)
**	    Got rid of GCN_MOD which was only used internally.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	11-Jan-2006 (bolke01) Bug 115632
**	    Back in version 1 of netu, the term 'public' was used to
**	    indicate 'private' , even though the displayed value was
**          'Private'.  As there was single letter abbrieviations
**	    this was not a problem (unless you tried to type the
**          whole word). 
**	    Corrected this, but also left in public to ensure no
**	    one who has been told this will get a failing system.
**	22-Sep-2008 (lunbr01) Sir 119985
**	    Change the default protocol from "wintcp" to "tcp_ip" for
**	    Windows (32-bit).  Also, change default Listen Address
**	    from "II0" to "II" to be consistent with "tcp_ip" on other
**	    platforms. Fix compiler warnings for unref'd variables.
**	24-Aug-09 (gordy)
**	    Remove string length restrictions.
**/

/*
**      mkming hints
NEEDLIBS =      GCFLIB COMPATLIB 
OWNER =         INGUSER
PROGRAM =       (PROG0PRFX)netu
*/

/*
**  Forward and/or External function references.
*/

static char	gcn_complete( VOID );
static i4	gcn_line_get( char *, char *, i4, char *, i4, i4  );
static STATUS	gcn_u_q( char *, char * );
static STATUS	gcn_u_s( char *, char * );
static STATUS	gcn_u_a( char * );
static STATUS	gcn_u_n( char * );
static STATUS	gcn_u_o( char * );
static i4	gcn_opcode_n( char *, i4 *, bool * );
static i4	gcn_opcode_o( char *, i4 *, bool * );
static i4	gcn_opcode_a( char *, i4 * );
static i4	gcn_qgetrec( char *, i4 );
static i4	gcn_getrec( char *, i4 );


/*{
** Name: main - The main entry routine for the Network Management 
**		Utility(NETU).
**
** Description:
**	The NETU currently supports five functions:
**	Shut down the Comm Server.
**	Define Remote Login Information.
**	Define Node information.
**	Define Other Attribute Information.
**
**	In the future it should also define link cost-factor, CPU cost-
**	factor, and perhaps even remote disk cost-factor; where the cost-factor
**	is a simple integer indicating relative performance/cost.
**
**	The main routine is the top loop prompting the user to choose the
**	function item. It then call the corresponding routine to prompt
**	the parameters for that specified function. All actions needed for
**	that function are implemented inside that functional routine.
**	When the user hits the <esc> key, the control returned back to the
**	top loop.
**
** Inputs:
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	    None
**
** Side Effects:
**
** History:
**
**      08-Oct-88 (Lin)
**          Initial function creation.
**	26-jun-89 (seiwald)
**	    PCexit( OK ) as long as initialization succeeds.
**      09-jan-91 (stevet)
**          Added CMset_attr call to initialize CM attribute table.
**      13-aug-91 (stevet)
**          Changed read-in character set support to use II_CHARSET.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	29-Nov-95 (gordy)
**	    Standardize name server client interface, add terminate call.
**	 8-Dec-95 (gordy)
**	    Change call to gcn_bedcheck() to IIgcn_check() which also
**	    checks for the embedded Name Server interface.
**	17-Jun-2004 (schka24)
**	    Replace unsafe env var handling.
*/


/* The following define should be in one of the GC CL header files */
#ifdef VMS
#define DEF_NET_WARE	"decnet"	/* default network software for
					   this product platform */
#define DEF_NET_ADDR	"II_GCC_0"	/* default network software Comm
					   Server address */
#endif

#ifdef UNIX
#define DEF_NET_WARE    "tcp_ip"        /* default network software for
                                           this product platform */
#define DEF_NET_ADDR    "II"  		/* default network software Comm
                                           Server address */
#endif

#ifdef hp9_mpe
#define DEF_NET_WARE    "tcp_ip"        /* default network software for
                                           this product platform */
#define DEF_NET_ADDR    "II"  		/* default network software Comm
                                           Server address */
#endif

#ifdef WIN32
#define DEF_NET_WARE    "tcp_ip"        /* default network software for
                                           this product platform */
#define DEF_NET_ADDR    "II"	   	/* default network software Comm
                                           Server address */
#endif

#ifdef OS2
#define DEF_NET_WARE    "lmgnbi"        /* default network software for
                                           this product platform */
#define DEF_NET_ADDR    "II0"   	/* default network software Comm
                                           Server address */
#endif

# define        U_SUPER         0100000 /* INGRES SUPER_USER */


GLOBALDEF	i4		gcn_trace = 0;

main(argc,argv)
int argc;
char *argv[];
{
    char	line[ 65 ];
    char	*p;
    STATUS	status;
    char	ret = '\n';
    char	uid[ GC_USERNAME_MAX + 1 ];
#define         MAXTOKEN        8
    i4          ntoken;
    char        *token[MAXTOKEN];
    CL_ERR_DESC cl_err;
	 
    /* Issue initialization call to ME */
    MEadvise(ME_INGRES_ALLOC);

    SIeqinit();

    /* 
    **	Get character set logical.
    */

    status = CMset_charset(&cl_err);
    if ( status != OK )
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

    SIprintf("%s/NET MANAGEMENT UTILITY --\n", SystemProductName);
    SIprintf("-- Copyright (c) 2004 Ingres Corporation\n");
    for(;;)
    {
	SIprintf("\n\n");
	SIprintf("Select one of the following: \n");
	if (argc > 1)
	{
	    SIprintf("\t  Q - Quiesce %s/NET, server id: %s\n",
				SystemProductName, argv[1]);
	    SIprintf("\t  S - Stop %s/NET, server id: %s\n", 
				SystemProductName, argv[1]);
	}
	else
	{
	    SIprintf("\t  Q <Comm_Server_Id> - Quiesce %s/NET\n",
				SystemProductName);
	    SIprintf("\t  S <Comm_Server_Id> - Stop %s/NET\n",
				SystemProductName);
	}
	SIprintf("\t  N - Modify Node Entry\n");
	SIprintf("\t  A - Modify Remote Authorization Entry\n");
	SIprintf("\t  O - Modify Other Attributes Entry\n");
	SIprintf("\t  E - Exit\n");
	SIprintf("%s> ", argv[0]);
	SIflush(stdout);
	if ( gcn_getrec( line, (i4)sizeof( line ) ) != OK)
		break;	/* End of file - scan no more devices */
	if (p = STchr(line, ret ))
		*p = '\0';
        ntoken = MAXTOKEN;
	STgetwords(line, &ntoken, token);
		   
	switch(line[0])
	{
	    case    'q':
	    case    'Q':
		    if((line[1] != EOS)
		       && STncasecmp(line, "quiesce", 6))
			   goto error;
		    status = gcn_u_q(uid,
					  (ntoken > 1) ? token[1]
					: (argc > 1) ? argv[1]
					: ""
				    );
			break;
	    case    's':
	    case    'S':
		    if((line[1] != EOS) 
			&& STncasecmp(line, "stop", 4 ))
			    goto error;
		    status = gcn_u_s(uid,
					  (ntoken > 1) ? token[1]
					: (argc > 1) ? argv[1]
					: ""
			            );
			break;
	    case    'a':
	    case    'A':
		    if((line[1] != EOS) 
			&& STncasecmp(line, "authorization",13 ))
			    goto error;
		    status = gcn_u_a(uid);
			break;
	    case    'n':
	    case    'N':
		    if((line[1] != EOS) 
			&& STncasecmp(line, "node",4))
			    goto error;
		    status = gcn_u_n(uid);
			break;
	    case    'o':
	    case    'O':
		    if( ( line[ 1 ] != EOS ) 
			&& STncasecmp( line, "attributes",10 ) )
		        goto error;
		    status = gcn_u_o( uid );
		    break;
	    case    'e':
	    case    'E':
		    if((line[1] != EOS) 
			&& STncasecmp(line, "exit",4))
			    goto error;
		    IIGCn_call( GCN_TERMINATE, NULL );
		    PCexit(OK);
	    default:
  	    error:
		    SIprintf("Not a valid function.\n");
    		    SIflush(stdout);
	}
    } 	/* End of FOR loop */

    IIGCn_call( GCN_TERMINATE, NULL );

    PCexit( OK );
}

static char
gcn_complete( VOID )
{
    return(0);
}

/*{
** Name: gcn_u_q - This routine quesces I/NET server
**
** Description:
**	This routine sends a special operation GCA_CS_QUIESCE to the
**      comm server.
**	The Comm Server will not accept any more calls and will
**	stop when all current calls terminate.
**	INGRES Superusers are the only user can run this function.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
** Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
** Exceptions:
**	    None
**
** Side Effects:
**
** History:
**	15-Nov-89 (davidg)
**		Initial creation.
**	24-Aug-09 (gordy)
**	    Limit input to line buffer length.
*/

static STATUS
gcn_u_q( char *uid, char *cs_id )
{
	char line[64];
	STATUS status;

	if(cs_id == NULL || cs_id[0] == EOS)
	{
	    SIprintf("   Quiesce %s/NET: Parameter Error!\n", 
			SystemProductName);
	    SIprintf("\tUsage: Q <Comm_Server_Id>\n");
	    return FAIL; 
	}

	SIprintf("   Comm_Server_Id = %s \n", cs_id);
	if ( gcn_line_get( "   Quiesce Communication Server: (Yes/No)", "", 
			   FALSE, line, sizeof( line ), FALSE ) == EOF )
	    return FAIL;
	STzapblank(line,line);
	if(line[0] != 'y' && line [0] != 'Y')
	    return FAIL;

	status = gcn_fastselect( GCA_CS_QUIESCE | GCA_NO_XLATE, cs_id );

	if( status != E_GC0040_CS_OK )
	{
	    SIprintf("\tError: Unable to quiesce Comm Server %s.\n", cs_id);
	    return FAIL;
	}


	SIprintf("\tCommunication Service is in a quiescent state\n");
	return OK;
}

/*{
** Name: gcn_u_s - This routine stops I/NET server
**
** Description:
**	This routine sends a special operation GCC_STOP to the comm server.
**	The Comm Server will close all its files and die.
**	INGRES Superusers are the only user can run this function.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
** Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
** Exceptions:
**	    None
**
** Side Effects:
**
** History:
**	08-Sep-89 (seiwald)
**	    On shutdown of IIGCC, call new gcc_doop() to make request
**	    of special command GCA_CS_SHUTDOWN to comm server.  No
**	    normal interaction takes place.
**	24-Aug-09 (gordy)
**	    Limit input to line buffer length.
*/

static STATUS
gcn_u_s( char *uid, char *cs_id )
{
	char line[64];
	STATUS status;

	if(cs_id == NULL || cs_id[0] == EOS)
	{
	    SIprintf("   Stop %s/NET: Parameter Error!\n", SystemProductName);
	    SIprintf("\tUsage: S <Comm_Server_Id>\n");
	    return FAIL; 
	}

	SIprintf("   Comm_Server_Id = %s \n", cs_id);
	if ( gcn_line_get( "   Stop Communication Server: (Yes/No)", "", 
			   FALSE, line, sizeof( line ), FALSE ) == EOF )
	    return FAIL;
	STzapblank(line,line);
	if(line[0] != 'y' && line [0] != 'Y')
	    return FAIL;

	status = gcn_fastselect( GCA_CS_SHUTDOWN | GCA_NO_XLATE, cs_id );

	if( status != E_GC0040_CS_OK )
	{
	    SIprintf("\tError: Unable to shut down Comm Server %s.\n", cs_id);
	    return FAIL;
	}

	SIprintf("\t%s/NET Communication Service is stopped gracefully\n",
				SystemProductName);
	return OK;
}

/*{
** Name: gcn_u_a - This routine handle the request of remote authorization
**                  information.
**
** Description:
**	The user is prompted for adding, deleting, or retrieving, 
**	information about remote login information.
**	The user should enter logical nodename, user name, and password.
**	Only INGRES can register the node as global name.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
**  Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**  Exceptions:
**	    None
**
** Side Effects:
**	    The IILOGIN database in the Name Server may be updated.
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	25-Mar-88 (lin)
**	    Formally reorganized from the original code.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for request to Name Server network database.
**	24-Aug-09 (gordy)
**	    Increase size of username and password buffers.
**	    Limit input to line buffer length.
*/
static STATUS
gcn_u_a( char *uid )
{
    char	v_node[65];
    char	user[257];
    char	encrp_pwd[512];
    char	password1[257], password2[257];
    char	value[1024];
    char	p_g[64];
    GCN_CALL_PARMS gcn_parm;
    i4		opcode;
    STATUS	status = OK;

  for(;;)
  {
    if(gcn_opcode_a("    Modify Remote Authorization Entry:\n", &opcode) == EOF)
	break;

  p_g_loop:
    if ( gcn_line_get( "\tEnter Private or Global ", "P", 
    			FALSE, p_g, sizeof( p_g ), FALSE ) == EOF )
	continue;
    if(STncasecmp(p_g, "public",STlen(p_g)) 
        && STncasecmp(p_g, "private",STlen(p_g) )
        && STncasecmp(p_g, "global",STlen(p_g) )
        && (p_g[0] != '=') ) 
    {
	SIprintf("\t  Invalid Input Value, enter <ESC> to exit.\n");
	SIflush(stdout);
	goto p_g_loop;
    }

    if ( gcn_line_get( "       Enter the remote v_node name", "",
			(opcode == GCN_ADD) ? FALSE : TRUE,
			v_node, sizeof( v_node ), FALSE ) == EOF)
	continue;

    if ( gcn_line_get( "       Enter remote User Name", "",
			TRUE, user, sizeof( user ), FALSE ) == EOF )
	continue;

    if( opcode == GCN_ADD && user[0] == EOS )
    {
  	SIprintf("\t  Defining installation password:\n");
	STcopy( "*", user );
    }

    if(opcode == GCN_ADD)
    {
      if ( gcn_line_get( "       Enter the remote Password", "",
      			TRUE, password1, sizeof( password1 ), TRUE ) == EOF )
  	continue;
      if ( gcn_line_get( "       Repeat the remote Password", "",
      			TRUE, password2, sizeof( password2 ), TRUE ) == EOF )
  	continue;
  
      if(STcmp(password1, password2))
      {
  	SIprintf("\t  Password does not match.\n");
  	continue;
      }
      if(password1[0] == EOS)
      {
  	SIprintf("\t  NULL Password defined.\n");
      }
      SIflush(stdout);
    }		/* end if IF( !GCN_RET)  */
    else
    	password1[0] = EOS;

    if ( password1[0] != EOS )
    	gcu_encode( user, password1, encrp_pwd );
    else
    	encrp_pwd[0] = EOS;

    STpolycat(3,user,",",encrp_pwd,value);

    SIprintf("\n");
    gcn_parm.gcn_flag = GCN_NET_FLAG;

    if(p_g[0] == 'G' || p_g[0] == 'g')
	gcn_parm.gcn_flag |= GCN_PUB_FLAG;

    if(p_g[0] == '=' && p_g[1])
    {
	gcn_parm.gcn_flag |= GCN_UID_FLAG;
	uid = &p_g[1];
    }

    if(opcode == GCN_RET)
    {
	if ( gcn_parm.gcn_flag & GCN_PUB_FLAG )
		SIprintf("Global:   ");
	else
		SIprintf("Private:  ");
        SIprintf("V_NODE          ");
        SIprintf("User Name       ");
        SIprintf("\n");
        SIflush(stdout);
    }

    gcn_parm.gcn_host = NULL;
    gcn_parm.gcn_response_func = NULL;
    gcn_parm.gcn_obj = v_node;
    gcn_parm.gcn_uid = uid;
    gcn_parm.gcn_type = GCN_LOGIN;
    gcn_parm.gcn_value = value;
    gcn_parm.gcn_install = SystemCfgPrefix;
    gcn_parm.async_id = gcn_complete;
    status = IIGCn_call(opcode,&gcn_parm);	
  } 	/* End of FOR loop */

  return status;
}

/*{
** Name: gcn_u_n - This routine handle the request of node information.
**
** Description:
**	The user is prompted for adding, deleting, or retrieving 
**	information about a virtual node name.
**	The user should enter GLOBAL or PRIVATE, netware, virtual nodename,
**	remote physical nodename, and port.
**	Only INGRES can register the node as global name.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    The IINODE database in the Name Server may be updated.
**
** History:
**
**      08-Sep-87 (Lin)
**          Initial function creation.
**	25-Mar-88 (lin)
**	    Formally reorganized from the original code.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for request to Name Server network database.
**	22-Mar-02 (gordy)
**	    Use merge flag rather than GCN_MOD.
**	24-Aug-09 (gordy)
**	    Use more appropriate sizes for input buffers.  Limit input 
**	    to buffer length.
*/

static STATUS
gcn_u_n( char *uid )
{
    i4		opcode;
    char    	value[ GC_HOSTNAME_MAX + 132 ];
    char    	p_node[ GC_HOSTNAME_MAX + 1 ];
    char    	v_node[65];
    char    	netware[65];
    char    	port[65];
    char	p_g[64];
    GCN_CALL_PARMS gcn_parm;
    STATUS	status = OK;
    bool	merge;

  for(;;)
  {

    if(gcn_opcode_n("   Modify Node Entry:\n", &opcode, &merge) == EOF)
	break;

  p_g_loopa:
    if ( gcn_line_get( "\tEnter Private or Global ","P", 
    			FALSE, p_g, sizeof( p_g ), FALSE ) == EOF )
	continue;
    if(STncasecmp(p_g, "public",STlen(p_g)) 
        && STncasecmp(p_g, "private",STlen(p_g) )
        && STncasecmp(p_g, "global",STlen(p_g) )
        && (p_g[0] != '=') ) 
    {
	SIprintf("\t  Invalid Input Value, enter <ESC> to exit.\n");
	SIflush(stdout);
	goto p_g_loopa;
    }

    if ( gcn_line_get( "       Enter the remote v_node name", "",
			(opcode == GCN_ADD) ? FALSE : TRUE,
			v_node, sizeof( v_node ), FALSE ) == EOF )
	continue;

    if ( gcn_line_get( "\tEnter the network software type", DEF_NET_WARE,
			(opcode == GCN_ADD) ? FALSE : TRUE,
		     	netware, sizeof( netware ), FALSE ) == EOF )
	continue;

    if ( gcn_line_get( "\tEnter the remote node address", "",
			(opcode == GCN_ADD) ? FALSE : TRUE,
			p_node, sizeof( p_node ), FALSE ) == EOF )
	continue;

    if ( gcn_line_get("\tEnter the remote Communications Server Listen address",
			(!STcasecmp(netware, DEF_NET_WARE )) ?
				DEF_NET_ADDR : "",
			(opcode == GCN_ADD) ? FALSE : TRUE,
			port, sizeof( port ), FALSE ) == EOF )
	continue;

    STpolycat(5,p_node,",",netware,",",port,value);

    SIprintf("\n");
    gcn_parm.gcn_flag = GCN_NET_FLAG;

    if(p_g[0] == 'G' || p_g[0] == 'g')
	gcn_parm.gcn_flag |= GCN_PUB_FLAG;

    if(p_g[0] == '=' && p_g[1])
    {
	gcn_parm.gcn_flag |= GCN_UID_FLAG;
	uid = &p_g[1];
    }

    if ( merge )
	gcn_parm.gcn_flag |= GCN_MRG_FLAG;

    if (opcode == GCN_RET)
    {
	if ( gcn_parm.gcn_flag & GCN_PUB_FLAG )
		SIprintf("Global:   ");
	else
		SIprintf("Private:  ");
        SIprintf("V_NODE          ");
        SIprintf("Net Software    ");
        SIprintf("Node Address    ");
        SIprintf("Listen Address  ");
        SIprintf("\n");
        SIflush(stdout);
    }

    gcn_parm.gcn_host = NULL;
    gcn_parm.gcn_response_func = NULL;
    gcn_parm.gcn_obj = v_node;
    gcn_parm.gcn_uid = uid;
    gcn_parm.gcn_type = GCN_NODE;
    gcn_parm.gcn_value = value;
    gcn_parm.gcn_install = SystemCfgPrefix;
    gcn_parm.async_id = gcn_complete;
    status = IIGCn_call(opcode,&gcn_parm);	
  } 	/* FOR loop */
  return status;
}

/*
** Name: gcn_line_get
**
** Description:
**	Read input.
**
** Input:
**	prompt		String used as input prompt.
**	def_line	Default value.
**	wild_flag	TRUE if '*' is valid input.
**	in_max		Max length of input buffer.
**	noecho		TRUE if input should not be echoed.
**
** Output:
**	in_line		Input string.
**
** Returns:
**	i4		Status.
**
** History:
**	24-Aug-09 (gordy)
**	    Added input buffer length to avoid buffer override.
*/

static i4
gcn_line_get( char *prompt, char *def_line, 
	      i4 wild_flag, char *in_line, i4 in_max, i4  noecho ) 
{
	char    line[MAXLINE];
	char	ret = '\n';
	char	*p;
	i4	status = EOF;

	for(;;)
	{
		STATUS s;

		SIprintf(prompt);
		SIprintf( *def_line ? "(%s): " : ": ", def_line );

/* This is somewhat unusuall but does force the SIflush to work */
		if( noecho ) SIprintf( "\n" );
		SIflush(stdout);

		if( noecho )
			s = gcn_qgetrec( line, (i4)sizeof( line ) );
		else
			s = gcn_getrec( line, (i4)sizeof( line ) );

		if( s != OK || line[0] == '\033' )
			break;

		if (p = STchr(line, ret ))
			*p = EOS;

		if( *def_line && !*line )
		{
			STcopy( def_line, in_line );
			SIprintf("\t\tDefault value: %s\n",in_line);
		}
		else  if ( STlength( line ) >= in_max )
		{
			SIprintf("\t\tInput length exceeds permitted size.\n");
			continue;
		}
		else
		{
			STcopy(line,in_line);
		}

		if( ! noecho )  	/* If NOT noecho, strip blanks */
			STzapblank(in_line,in_line);

		if( !*in_line )
		{
			SIprintf("\t\tValue required, enter <ESC> to exit.\n");
			continue;
		}

		if(!STcasecmp( in_line, "*" ) )
		{
			if( !wild_flag )
			{
				SIprintf("\t\tValue required, enter <ESC> to exit.\n");
				continue;
			}
			in_line[0] = EOS;
		}

		status = OK;
		break;		/* There is valid data */
	}	/* end of FOR loop */
	return status;
}

/*
** Name: gcn_opcode_n
**
** Description:
**	Input desired operation including merge.
**
** Input:
**	menu_prompt	Input prompt.
**
** Output:
**	opcode		Operation code.
**	merge		TRUE if form of add operation.
**
** Returns:
**	i4		Status.
**
** History:
**	24-Aug-09 (gordy)
**	    Provide more appropriate input buffer lengths.
**	    Limit input to line buffer length.
*/

static i4
gcn_opcode_n( char *menu_prompt, i4  *opcode, bool *merge )
{
    char    	line[65];
    char    	prompt[128];
    char	ret = '\n';

   *merge = FALSE;
   STpolycat(3,"\n\n", menu_prompt, "     Enter operation (add, merge, del, show, exit)",
		prompt);

   for(;;)
   {
    if ( gcn_line_get( prompt, "", FALSE, line, sizeof( line ), FALSE ) == EOF )
	return(EOF);	/* End of file - scan no more devices */


    STzapblank(line,line);
    if(!STncasecmp(line, "add",STlen(line) )) 
	*opcode = GCN_ADD;
    else if(!STncasecmp(line, "merge",STlen(line) )) 
    {
	*opcode = GCN_ADD;
	*merge = TRUE;
    }
    else if(!STncasecmp(line, "delete",STlen(line) )) 
	*opcode = GCN_DEL;
    else if(!STncasecmp(line, "show",STlen(line) ))
	 *opcode = GCN_RET;
    else if(!STncasecmp(line, "exit",STlen(line) ))
	return(EOF);
    else 
    {
	 SIprintf("\t\tInvalid Operation, enter <ESC> to exit.\n");
	 SIflush(stdout);
	 continue;	/* FOR loop */
    }
    return(OK);
   } /* FOR loop */
}

/*
** Name: gcn_opcode_a
**
** Description:
**	Input desired operation excluding merge.
**
** Input:
**	menu_prompt	Input prompt.
**
** Output:
**	opcode		Operations code.
**
** Returns:
**	i4		Status.
**
** History:
**	24-Aug-09 (gordy)
**	    Provide more appropriate input buffer lengths.
**	    Limit input to line buffer length.
*/

static i4
gcn_opcode_a( char *menu_prompt, i4  *opcode )
{
    char    	line[65];
    char    	prompt[128];
    char	ret = '\n';

   STpolycat(3,"\n\n", menu_prompt, "     Enter operation (add, del, show, exit)",
		prompt);

   for(;;)
   {
    if ( gcn_line_get( prompt, "", FALSE, line, sizeof( line ), FALSE ) == EOF )
	return(EOF);	/* End of file - scan no more devices */


    STzapblank(line,line);
    if(!STncasecmp(line, "add",STlen(line) )) 
	*opcode = GCN_ADD;
    else if(!STncasecmp(line, "del",STlen(line) )) 
	*opcode = GCN_DEL;
    else if(!STncasecmp(line, "show",STlen(line) ))
	 *opcode = GCN_RET;
    else if(!STncasecmp(line, "exit",STlen(line)))
	return(EOF);
    else 
    {
	 SIprintf("\t\tInvalid Operation, enter <ESC> to exit.\n");
	 SIflush(stdout);
	 continue;	/* FOR loop */
    }
    return(OK);
   } /* FOR loop */
}

static i4
gcn_qgetrec( char *buf, i4  len )
{
	i4 c;
	char *obuf = buf;
	static TEENV_INFO te;
	static i4  teopened = 0;
        static bool tefile = FALSE;

        if( !teopened )
	{
		if( TEopen( &te ) != OK || !SIterminal( stdin ) )
			tefile = TRUE;
		teopened++;
	}

	if( tefile )
	{
		return( gcn_getrec( buf, len ) );
	}

	TErestore( TE_FORMS );

	/* make sure that the buffer is allways NULL terminated */
	--len;
	if( len >= 0)
		*(buf + len) = 0;

	while( len-- > 0 )
		switch( c = TEget((i4) 0) ) 	/* No timeout */
	{
	case TE_TIMEDOUT:	/* just incase of funny TE CL */
	case EOF:
	case '\n':
	case '\r':
		*buf++ = 0;
		len = 0;
		break;
	default:
		*buf++ = c;
		break;
	}

	TErestore( TE_NORMAL );

	return buf == obuf ? EOF : OK;
}

static i4
gcn_getrec( char *buf, i4  len )
{
	return SIgetrec( buf, (i4)len, stdin );
}

/*{
** Name: gcn_u_o - This routine handle the request of attribute information.
**
** Description:
**	The user is prompted for adding, deleting, or retrieving 
**	information about attributes of the virtual node name.
**	The user should enter GLOBAL or PRIVATE,
**	attribute name and attribute value for the virtual node name.	
**	Only INGRES can register the node as global name.
**
** Inputs:
**	Local user id.
**
** Outputs:
**
**
**	Returns:
**	    DB_STATUS:
**		E_DB_OK		
**		E_DB_ERROR		
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    The IIATTR database in the Name Server may be updated.
**
** History:
**
**	25-Jul-97 (rajus01)
**          Initial function creation.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for request to Name Server network database.
**	22-Mar-02 (gordy)
**	    Use merge flag rather than GCN_MOD.
**	24-Aug-09 (gordy)
**	    Provide more appropriate size for input buffers.
**	    Limit input to line buffer length.
*/

static STATUS
gcn_u_o( char *uid )
{
    i4	    		opcode;
    char    		v_node[ 65 ];
    char    		attr_name[ 65 ];
    char    		attr_value[ 65 ];
    char    		value[ 130 ];
    char    		p_g[ 64 ];
    STATUS		status = OK;
    bool		merge;
    GCN_CALL_PARMS 	gcn_parm;

    for(;;)
    {
    	if( gcn_opcode_o( "   Modify Other Attributes Entry:\n",
						&opcode, &merge ) == EOF )
	    break;

p_g_loopa:
	if ( gcn_line_get( "\tEnter Private or Global ", "P", 
			   FALSE, p_g, sizeof( p_g ), FALSE ) == EOF )
	    continue;
        if(STncasecmp(p_g, "public",STlen(p_g)) 
        && STncasecmp(p_g, "private",STlen(p_g) )
        && STncasecmp(p_g, "global",STlen(p_g) )
        && ( p_g[0] != '=' ) ) 
	{
	    SIprintf( "\t  Invalid Input Value, enter <ESC> to exit.\n" );
	    SIflush( stdout );
	    goto p_g_loopa;
    	}
    	if ( gcn_line_get( "\tEnter the remote v_node name", "",
			   (opcode == GCN_ADD) ? FALSE : TRUE,
			   v_node, sizeof( v_node ), FALSE ) == EOF )
	    continue;

	if ( gcn_line_get( "\tEnter the Attribute Name", "",
			   (opcode == GCN_ADD) ? FALSE : TRUE,
			   attr_name, sizeof( attr_name ), FALSE ) == EOF )
	    continue;

	if ( gcn_line_get( "\tEnter the Attribute Value", "",
			   (opcode == GCN_ADD) ? FALSE : TRUE,
	     		   attr_value, sizeof( attr_value ), FALSE ) == EOF )
	    continue;

	STpolycat( 3,attr_name,",",attr_value,value );

	SIprintf("\n");
	gcn_parm.gcn_flag = GCN_NET_FLAG;

    	if( p_g[ 0 ] == 'G' || p_g[ 0 ] == 'g' )
	    gcn_parm.gcn_flag |= GCN_PUB_FLAG;

	if( p_g[ 0 ] == '=' && p_g[ 1 ] )
    	{
	    gcn_parm.gcn_flag |= GCN_UID_FLAG;
	    uid = &p_g[ 1 ];
    	}

	if ( merge )
	    gcn_parm.gcn_flag |= GCN_MRG_FLAG;

	if ( opcode == GCN_RET )
    	{
	    if ( gcn_parm.gcn_flag & GCN_PUB_FLAG )
		SIprintf( "Global:   " );
	    else
		SIprintf( "Private:  " );
            SIprintf( "V_NODE          " );
            SIprintf( "Attribute Name       " );
            SIprintf( "Attribute Value " );
            SIprintf( "\n" );
            SIflush( stdout );
    	}

	gcn_parm.gcn_host 		= NULL;
	gcn_parm.gcn_response_func 	= NULL;
	gcn_parm.gcn_obj 		= v_node;
	gcn_parm.gcn_uid 		= uid;
	gcn_parm.gcn_type 		= GCN_ATTR;
	gcn_parm.gcn_value 		= value;
	gcn_parm.gcn_install 		= SystemCfgPrefix;
	gcn_parm.async_id 		= gcn_complete;
	status 				= IIGCn_call( opcode, &gcn_parm );	

    }/* end FOR */

    return status;
}

/*
** Name: gcn_opcode_o
**
** Description:
**	Input desired operation including merge.
**
** Input:
**	menu_prompt	Input prompt.
**
** Output:
**	opcode		Operation code.
**	merge		TRUE if form of add operation.
**
** Returns:
**	i4		Status.
**
** History:
**	24-Aug-09 (gordy)
**	    Provide more appropriate input buffer lengths.
**	    Limit input to line buffer length.
*/

static i4
gcn_opcode_o( char *menu_prompt, i4  *opcode, bool *merge )
{
    char	line[ 65 ];
    char	prompt[ 128 ];
    char	ret = '\n';

    *merge = FALSE;
    STpolycat( 3,"\n\n", menu_prompt,
		"     Enter operation (add, merge, del, show, exit)",
		prompt );

    for(;;)
    {
	if ( gcn_line_get(prompt, "", FALSE, line, sizeof(line), FALSE) == EOF )
	    return(EOF);	/* End of file - scan no more devices */

    	STzapblank(line,line);
    	if( !STncasecmp( line, "add", STlen(line) ) ) 
	    *opcode = GCN_ADD;
	else if( !STncasecmp( line, "merge", STlen(line) ) ) 
	{
	    *opcode = GCN_ADD;
	    *merge = TRUE;
	}
	else if( !STncasecmp( line, "del", STlen(line) ) ) 
	    *opcode = GCN_DEL;
	else if( !STncasecmp( line, "show", STlen(line) ) )
	    *opcode = GCN_RET;
	else if( !STncasecmp( line, "exit", STlen(line) ) )
	    return(EOF);
	else 
    	{
	    SIprintf( "\t\tInvalid Operation, enter <ESC> to exit.\n" );
	    SIflush( stdout );
	    continue;	/* FOR loop */
    	}

    	return( OK );

    } /* FOR loop */
}

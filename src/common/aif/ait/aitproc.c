/*
** Copyright (c) 2004 Ingres Corporation
*/





# include	<compat.h>
# include	<cv.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<iiapi.h>
# include	"aitfunc.h"
# include	"aitdef.h"
# include	"aitsym.h"
# include	"aitmisc.h"
# include	"aitproc.h"
# include	"aitparse.h"





/*
** Name:	aitproc.c - API function processor
**
** Description:
**	This file contains functions that execute API function calls, 
**	print ouput parameters and report error message.
**
**	AITinit			Initialize the tester.
**	AITterm			Terminate the tester.
**	AITprocess		Process the input script.
**
**	Local function:
**
**	ait_exec		Execute an API function.
**	ait_wait		Check for API function completion.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Generalized the API function execution and removed
**	    API result printing to aitfunc.c.
**	31-Mar-95 (gordy)
**	    Added function pointer for messaging routine
**	    to handle output to standard output.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jul-95 (feige)
**	    Added command line options:
**		-user <username> which indicates shell variable
**				 $user and its value <username>
**		-passwd <password> which indicates shell variable
**				 $passwd and its value <password>
**		-verbose	 ge_serverInfo and various handles
**				 will be printed into output only
**				 when this flag is set at the
**				 command line.
**	20-Sep-96 (gordy)
**	    Slight adjustment to output.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/





/*
** Macro definition
*/
/*
** Command line option -- target DB
*/
# define                db_flag         "-db"

/*
** Command line option -- tracing level
*/
# define                tr_flag         "-trace"

/*
** Command line option -- tracing log
*/
# define                log_flag        "-log"

/*
** Command line option -- username
*/
# define		user_flag	"-user"

/*
** Command line option -- password
*/
# define		passwd_flag	"-passwd"

/*
** Command line option -- verbose fro output control
*/
# define		verbose_flag	"-verbose"





/*
** Global Variables.
*/

GLOBALDEF FILE		*inputf = NULL;
GLOBALDEF FILE		*outputf = NULL;
GLOBALDEF char    	*dbname = NULL;
GLOBALDEF char		*username = NULL;
GLOBALDEF char		*password = NULL;
GLOBALDEF i4      	traceLevel = 0;
GLOBALDEF i4		verboseLevel = 0;

/*
** It would be better to make the control block
** local and pass it around as needed, but this
** will require a major amount of work.  Since
** we only have 1 control block, settle for
** global access.
*/

GLOBALDEF AITCB		ait_cb;		/* AIT Control Block */





/*
** Local variables.
*/

static char	str[ 256 ];





/*
** Local Functions
*/
static VOID		ait_exec( AITCB *aitcb );
static VOID		ait_wait( AITCB *aitcb );






/*
** Name: AITinit() - tester initializing function
**
** Description:
**	This function initializes the API tester.
**
** Inputs
**	argc		Number of command line arguments.
**	argv		Array of commond line arguments.
**	msg		message displaying function
**
** Outputs:
**	None.
**
** Returns:
**	AITCB *		AIT control block, NULL if error.
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Moved command line argument processing from main().
**	19-Apr-95 (feige)
**	    Changed while(i<argc && argv[i][0]=='-') to
**	    while (i<argc-1 && argv[i]=='-') so that a command
**	    line argument with prefix '-' will not be treated
**	    as an option flag if it appears as the last argument
**	14-Jul-95 (feige)
**	    Added command line options:
**		-user <username> which indicates shell variable
**				 $user and its value <username>
**		-passwd <password> which indicates shell variable
**				 $passwd and its value <password>
**		-verbose	 ge_serverInfo and various handles
**				 will be printed into output only
**				 when this flag is set at the
**				 command line.
*/

FUNC_EXTERN AITCB *
AITinit( int argc, char** argv, void (*msg)() )
{
    STATUS	status;
    char	*inputFileName; 
    char	*outputFileName = default_output;
    char	*tracefile = NULL;
    i4		i = 1;

    ait_cb.state = AIT_DONE;
    ait_cb.msg = msg;

    if ( argc < 2)
    {
	(*ait_cb.msg)( "Usage:\napitest [-db dbname] [-trace level] [-log traceFile] [-user username] [-passwd password] [-verbose] inputfilename outputfilename\n" );
	return( &ait_cb );
    }

    while ( i < argc -1  && argv[i][0] == '-' )
    {
	if ( STequal( argv[i], db_flag ) )
	{
	    i++;
	    dbname = (char *)ait_malloc( STlength( argv[i] ) + 1 );
	    STcopy( argv[i], dbname );
	    i++;
	}
        else  if ( STequal( argv[i], log_flag ) )
	{
	    i++;
	    tracefile = argv[ i++ ];
       	}	      
	else  if ( STequal( argv[i], tr_flag ) )
	{
	    i++;
	    status = CVan( argv[i], &traceLevel );
	    if ( status != OK )
	    {
		(*ait_cb.msg)( "Command line error: trace-level is not an integer.\n" );
		return( &ait_cb );
	    }
	    i++;
	}	    
	else  if ( STequal( argv[i], user_flag ) )
	{
	    i++;
	    username = ( char * )
		       ait_malloc( STlength( argv[i] ) + 1 );
	    STcopy( argv[i], username );
	    i++;
	}
	else  if ( STequal( argv[i], passwd_flag ) )
	{
	    i++;
	    password = ( char * )
		       ait_malloc( STlength( argv[i] ) + 1 );
	    STcopy( argv[i], password );
	    i++;
	}
	else  if ( STequal( argv[i], verbose_flag ) )
	{
	    i++;
	    verboseLevel = 1;
	}
	else
	{
	    STprintf( str, "%s: unknown option.\n", argv[i] );
	    (*ait_cb.msg)( str );
	    (*ait_cb.msg)( "Usage:\napitest [-db dbname] [-trace level] [-log traceFile] inputfilename outputfilename\n" );
	    i++;
	    break;
	}
    }

    if ( i == argc )
    {
	(*ait_cb.msg)( "Command line error: inputfilename unknown.\n" );
	return( &ait_cb );
    }

    inputFileName = argv[ i++ ];

    if ( i < argc )
    {
	outputFileName = argv[ i++ ];

	if ( STequal( inputFileName, outputFileName ) )
	{
	    (*ait_cb.msg)( "input and output file name must not be the same.\n" );
	    return( &ait_cb );
	}
    }

    if ( i < argc )
    {
	for ( ; i < argc; i++ )
	{
	    STprintf( str, "%s ignored\n", argv[i] );
	    (*ait_cb.msg)( str );
	}
    }
   
    if ( tracefile  &&  *tracefile )  ait_initTrace( tracefile );
    AIT_TRACE( AIT_TR_TRACE )( "initialize the api tester...\n" );
    AIT_TRACE( AIT_TR_TRACE )( "inputfile = %s, outputfile = %s\n",
			     inputFileName, outputFileName );
    
    /*
    ** Open input/output files.  Only
    ** enter parsing state if successful.
    */
    if ( ait_openfile( inputFileName, "r", &inputf ) )
        if ( STequal( outputFileName, default_output ) )
	{
	    outputf = stdout;
	    ait_cb.state = AIT_PARSE;
        }
	else  if ( ait_openfile ( outputFileName, "w", &outputf ) )
	    ait_cb.state = AIT_PARSE;

    return( &ait_cb );
}






/*
** Name: AITterm() - API tester terminating function
**
** Description:
**	This function shut down the API tester.
**
** Inputs
**	aitcb		Tester control block.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
** 	21-Feb-95 (feige)
**	    Created.
*/

FUNC_EXTERN VOID
AITterm( AITCB *aitcb )
{
    if ( inputf )  ait_closefile( inputf );
    if ( outputf  &&  outputf != stdout )  ait_closefile( outputf );
    inputf = NULL;
    outputf = NULL;

    free_calltrackList();
    free_dscrpList( );
    free_datavalList( );
    

    AIT_TRACE( AIT_TR_TRACE )( "api tester terminates!\n" );
    ait_termTrace();

    return;
}






/*
** Name: AITprocess
**
** Description:
**	API tester main processing.  Parse the input script until
**	an API function is executed.  If the function is async,
**	return control to the caller.  Check for function completion,
**	at which point we continue parsing.  Return to the caller
**	when parser reaches end-of-file.
**
** Input:
**	aitcb		Tester control block.
**
** Output:
**	None
**
** Returns:
**	bool		TRUE while processing, FALSE when done.
**
** History:
**	27-Mar-95 (gordy)
**	    Created.
*/

FUNC_EXTERN bool
AITprocess( AITCB *aitcb )
{
    /*
    ** Process input until done or we execute
    ** an async API function.
    */
    do
    {
	switch( aitcb->state )
	{
	    case AIT_PARSE :
		/*
		** Parse input file until end-of file
		** or API function needs execution.
		*/
		AITparse( aitcb );
		break;
	
	    case AIT_EXEC :
		/*
		** Execute an API function.  Async functions
		** will need to check for completion.  Sync
		** functions will continue parsing.
		*/
		ait_exec( aitcb );
		break;
	
	    case AIT_WAIT :
		/*
		** Check for async API function completion.
		** Repeated functions will need to be re-executed,
		** otherwise we continue parsing input file.
		*/
		ait_wait( aitcb );
		break;
	}

    } while( aitcb->state != AIT_DONE  &&  aitcb->state != AIT_WAIT );

    return( aitcb->state == AIT_DONE ? FALSE : TRUE );
}






/* Name: ait_exec
**
** Description:
**	Execute an API function.  If the function is async,
**	set AIT_WAIT state.  Otherwise, display function
**	results and set AIT_PARSE state.
**
** Input:
**	aitcb		Tester control block.
**
** Output:
**	none
**
** Returns:
**	none
**
** History:
**	27-Mar-95 (gordy)
**	    Created.
*/

static VOID
ait_exec( AITCB *aitcb )
{
    API_FUNC_INFO *funInfo = aitcb->func->funInfo;

    SIfprintf( outputf, "%s( %s )\n", 
	       funInfo->funName, aitcb->func->parmBlockName );

    /*
    ** Execute the API function.
    */
    (*funInfo->api_func)( aitcb->func->parmBlockPtr );

    if ( funInfo->flags & AIT_ASYNC )
    {
	/*
	** There is a subtle interaction between
	** ait_exec() and ait_wait() at this point.
	** For async functions we need to check to
	** see if they completed immediatly.  If so,
	** we need to do normal result processing.  
	** If not, we don't want to call IIapi_wait(),
	** but we do want to enter the AIT_WAIT state.
	** This can be accomplished by calling 
	** ait_wait() with a state other than AIT_WAIT.  
	*/
	ait_wait( aitcb );
    }
    else
    {
	/*
	** Process sync function results and
	** continue parsing input file.
	*/
	if ( funInfo->results )  
	    (*funInfo->results)( aitcb, aitcb->func->parmBlockPtr );

	aitcb->state = AIT_PARSE;
    }

    return;
}






/*
** Name: ait_wait
**
** Description:
**	Check for completion of an API function.  If we are
**	in AIT_WAIT state, IIapi_wait() will be called if the
**	function has not completed.  Otherwise, we will just
**	check for immediate completion, and enter the AIT_WAIT
**	state if not completed.
**
**	Repeated functions which complete successfully will
**	return to the AIT_EXEC state.  All other completed
**	functions will continue parsing the input file.
**
** Input:
**	aitcb		Tester control block.
**
** Output:
**	none
**
** Returns:
**	none
**
** History:
**	27-Mar-95 (gordy)
**	    Created.
*/

static VOID
ait_wait( AITCB *aitcb )
{
    API_FUNC_INFO	*funInfo = aitcb->func->funInfo;
    IIAPI_GENPARM	*genParm = (IIAPI_GENPARM *)aitcb->func->parmBlockPtr;
    IIAPI_WAITPARM	wt;

    /*
    ** Only call IIapi_wait() when in the wait state.
    ** This gives us a chance to check for immediate
    ** completion of an async function without
    ** blocking before returning control to the
    ** caller.
    */
    if ( aitcb->state == AIT_WAIT  &&  ! genParm->gp_completed )
    {
	wt.wt_timeout = -1;
	IIapi_wait ( &wt );
    }

    if ( ! genParm->gp_completed )
	aitcb->state = AIT_WAIT;
    else
    {
	AITapi_results( genParm->gp_status, genParm->gp_errorHandle );

	if ( 
	     genParm->gp_status != IIAPI_ST_MESSAGE  && 
	     genParm->gp_status != IIAPI_ST_WARNING  &&
	     genParm->gp_status != IIAPI_ST_SUCCESS 
	   )
	    aitcb->state = AIT_PARSE;
	else
	{
	    if ( funInfo->results )  
		(*funInfo->results)( aitcb, aitcb->func->parmBlockPtr );

	    aitcb->state = aitcb->func->repeated ? AIT_EXEC : AIT_PARSE;
	}
    }

    return;
}

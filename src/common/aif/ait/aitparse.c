/*
** Copyright (c) 1995, 2005  Ingres Corporation
**
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
# include	"aitbind.h"
# include	"aitproc.h"
# include	"aitparse.h"

/*
** Name:	aitparse.c - AIT input file syntax parser
**
** Description:
**	This file contains functions for the input file syntax parsing
**
**	AITparse		API tester parsing function
**	input_line		input file reading function
**	Parser_Mode		parsing mode adjudgement function
**	Var_Decl_Syntax		variable declaration parsing function
**	Var_Decl_Semantics	variable declaration semantics parsing function
**	Fun_Call_Syntax		function call syntax parsing function
**	Fun_Call_Semantics	function call semantics parsing function
**	Var_Reuse_Syntax	reused variable syntax checking function
**	Var_Assgn_Syntax	variable assignment syntax parsing function
**	Field_Assgm_Parse	parameter field assignment parsing function
**	Dataval_Assgn_Parse	IIAPI_DATAVALUE value assignment parsing
**	Dscrpt_Assgn_Parse	descriptor assignment parsing function
**	Var_Assgn_Semantics	variable assignment semantics checking function
**	isParmBlockNameDeclared	reused variable searching function
**	Varname_Lex		variable name parsing function
**	Varval_Lex		variable value parsing function
**	read_line		input line reading function
**	line_filter		input line filtering function
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Added tester control block.
**	12-May-95 (feige)
**	    Changed the null datavalue format form
**	    NULL() to NULL.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jul-95 (feige)
**	    Parsed command line options:
**		-user <username>
**		-passwd	<password>
**		-verbose
**	27-Jul-95 (feige)
**	    Supported syntax ds_length = sizeof( $shell_variable );
**	21-Dec-95 (feige)
**	    Checked line_buf_ptr after skipping blanks in line_filter().
**	27-Dec-95 (feige)
**     	    Supported embedded double-quote (") in string.	
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	13-Mar-97 (gordy)
**	    Added IIapi_releaseEnv().
**	19-Aug-98 (rajus01)
**	    Added IIapi_setEnvParam(), IIapi_abort(), IIapi_formatData().
**	11-Sep-98 (rajus01)
**	    Added descriptorCount definition. Changed descritporList
**	    definition to be an array of pointers to struct _descriptorList.
**	    This was needed since IIapi_formatData() provides two descriptors
**          and Dscrpt_Assgn_Parse() core dumped while parsing the 2nd
**          descriptor.
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	11-Feb-99 (rajus01)
**	    Added support for IIapi_formatData() to use the syntax
**	    "fd_srcValue = xxxx.fd_dstValue". Removed unwanted trace
**	    statement.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Oct-04 (gordy)
**	    Fix support for 32K strings.
**	21-jan-2005 (abbjo03)
**	    Remove include of stdio.h to build on VMS.
*/





/*
** Input text after "##" in a line is comment 
** that will be copied to the output file
*/
# define	output_comment_sign		'#'

/*
** Input text after "//" in a line is comment 
** that will not be written to the output file
*/
# define	skip_comment_sign		'/'

/*
** API function followed keyword repeat is executed
** repeatedly
*/
# define	repeat_sign			"repeat"

/*
** Keyword using indicates the descriptor used by
** IIapi_getColumns
*/
# define	using_sign			"using"

# define IS_CHAR_LEGAL		(*line_buf_ptr>='a' && *line_buf_ptr<='z') || \
				(*line_buf_ptr>='A' && *line_buf_ptr<='Z') || \
				(*line_buf_ptr>='0' && *line_buf_ptr<='9') || \
				*line_buf_ptr=='_'

# define SKIP_BLANKS_IN_LINE    if ( (*line_buf_ptr == ' ')  ||		\
				     (*line_buf_ptr == '\t') ) 		\
				{					\
				    ++line_buf_ptr; 			\
				    while( (*line_buf_ptr==' ') ||	\
					   (*line_buf_ptr=='\t') )	\
				        ++line_buf_ptr;			\
				}

# define GOTO_NEXT_WORD		if ( *line_buf_ptr == EOS )  \
				    input_line()

# define BUF_ALLOC		(char *)ait_malloc  \
				    ( STlength( line_buf_ptr ) + 1 )


typedef VOID ( *INVOCATION )( i4, i4  );






/*
** Global Variables
*/

GLOBALDEF CALLTRACKLIST			*callTrackList = NULL; 
GLOBALDEF CALLTRACKLIST			*curCallPtr;
GLOBALDEF i4      		  	cur_line_num = 0;
GLOBALDEF struct _varRec          	curVarAsgn[MAX_FIELD_NUM];
GLOBALDEF struct _dataValList		*dataValList = NULL;
GLOBALDEF struct _descriptorList	*descriptorList[MAX_DESCR_LIST]={NULL};
GLOBALDEF i4				descriptorCount = 0;





/*
** Local Variables
*/
static CALLTRACKLIST	*preReusedPtr; 
static CALLTRACKLIST	*lastCallPtr = NULL;
static char    		input_line_buf[FILE_REC_LEN];
static char    		*line_buf_ptr = NULL;
static char    		*comment = NULL;
static struct _varRec   curVarBuf;
static i4		parser_mode = 0;
static bool		str_parse_on = FALSE;
static bool		repeat_on = FALSE;
static bool		isEOF = FALSE;

static INVOCATION bindfunTBL[] =
{
	var_assgn_auto,				/* 0 */
        var_assgn_cancel,                       /* 1 */
        var_assgn_catchEvent,                   /* 2 */
        var_assgn_close,                        /* 3 */
        var_assgn_commit,                       /* 4 */
        var_assgn_connect,                      /* 5 */
        var_assgn_disconnect,                   /* 6 */
        var_assgn_getColumns,                   /* 7 */
        var_assgn_getCopyMap,                   /* 8 */
        var_assgn_getDescriptor,                /* 9 */
        var_assgn_getErrorInfo,                 /* 10 */
        var_assgn_getQueryInfo,                 /* 11 */
	var_assgn_getEvent,			/* 12 */
        var_assgn_initialize,                   /* 13 */
	var_assgn_modifyConnect,		/* 14 */
        var_assgn_prepareCommit,                /* 15 */
        var_assgn_putColumns,                   /* 16 */
        var_assgn_putParms,                     /* 17 */
        var_assgn_query,                        /* 18 */
        var_assgn_registerXID,                  /* 19 */
        var_assgn_releaseEnv,                   /* 20 */
        var_assgn_releaseXID,                   /* 21 */
        var_assgn_rollback,                     /* 22 */
        var_assgn_savePoint,                    /* 23 */
        var_assgn_setConnectParam,              /* 24 */
        var_assgn_setEnvParam,              	/* 25 */
        var_assgn_setDescriptor,                /* 26 */
        var_assgn_terminate,                    /* 27 */
        var_assgn_wait,                         /* 28 */
        var_assgn_abort,                        /* 29 */
        var_assgn_frmtData                      /* 30 */
};





/* 
** Local Functions
*/

static VOID	read_line();
static VOID	line_filter();
static VOID	input_line();
static i4	Parser_Mode();
static VOID	Var_Decl_Syntax();
static VOID	Var_Decl_Semantics();
static VOID	Var_Assgn_Syntax();
static VOID	Fun_Call_Semantics();
static VOID	Var_Reuse_Syntax();
static VOID	Fun_Call_Syntax();
static VOID	Field_Assgn_Parse();
static VOID	Dscrpt_Assgn_Parse( i4  maxIndex );
static VOID	Dataval_Assgn_Parse( i4  maxIndex );
static VOID	Var_Assgn_Semantics();
static bool	isParmBlockNamDecled( struct _callTrackList *callTrackL, 
				      char *varName );

static VOID	Varname_Lex( char *varNameBuf, char deliminator1, 
			       char deliminator2, char deliminator3 );

static VOID	Varval_Lex( char *valStrBuf, char deliminator1, 
			      char deliminator2, char deliminator3 );






/*
** Name: AITparse() - AIT parsing function
**
** Description:
**	This function decides current parsing mode, proceeds 
**	if the input syntax is correct, exit otherwise.
**
** Inputs:
**	aitcb		Tester control block.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**/

FUNC_EXTERN VOID
AITparse( AITCB *aitcb )
{
    parser_mode = Parser_Mode();

    switch( parser_mode )
    {
	case Var_Decl_Mode:	
	    Var_Decl_Syntax(); 	
	    Var_Decl_Semantics();

	    break;

	case Var_Assgn_Mode:
	    Var_Assgn_Syntax();	
	    Var_Assgn_Semantics();

	    break;

	case Var_Reuse_Mode:
	    Var_Reuse_Syntax();	

	    break;

	case Fun_Call_Mode:
	    Fun_Call_Syntax();	
	    Fun_Call_Semantics();
	    aitcb->state = AIT_EXEC;
	    aitcb->func = curCallPtr;

	    break;

	default:	
	    if ( ! isEOF )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: %s: unrecognized syntax.\n",
		      cur_line_num, line_buf_ptr);

		SIprintf
		    ( "[line %d]: %s: unrecognized syntax.\n",
		      cur_line_num, line_buf_ptr);

		if ( repeat_on == TRUE )
		{
		    AIT_TRACE( AIT_TR_ERROR )
			( "[line %d]: \"repeat\" is legal only when put right before funtion call\n",
			  cur_line_num );

		    SIprintf
			( "[line %d]: \"repeat\" is legal only when right before fun\n",
			  cur_line_num );
		}
	    }

	    aitcb->state = AIT_DONE;

	    break;
    }

    return;
}






/*
** Name: input_line() - input file reading function
**
** Description:
**	This function repeatly reads in a line from the input file until 
**	a nonempty line incurred, then filters out beginning and trailing 
**	blanks, comments.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-feb-95 (feige)
**	    Created.
*/

static VOID
input_line()
{
    char	*index1, *index2;

    while ( line_buf_ptr == NULL || *line_buf_ptr == EOS )
    {
        /* 
	** if there is any comment marked by "##"
	** in current line, write it to the output
	** file before reading next line 
	*/
    	if ( comment != NULL )
    	{
    	    SIfprintf( outputf, "%s\n", comment );
	    MEfree( comment );
	    comment = NULL;
        }

	/*
	** read next line
	*/
	read_line();

	if ( isEOF == TRUE )
	    return;

	line_filter();
    }

    return;
} 






/*
** Name: Parser_Mode() - parsing mode adjudgement function
**
** Description:
**	This function figures out the current parsing mode based on 
**	the input syntax.
**
** Inputs:
**	None.
** 
** Outputs:
**	None.
**
** Returns:
**	i4	parser-mode(1-4), 0 if error	
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/
static i4
Parser_Mode()
{
    II_INT	mode = 0;
    i4		i = 0;
    char 	*ptr,	*varName;

    input_line();
	
    if ( isEOF == TRUE )
	return ( mode );

    /* 
    ** Check if the input text is
    ** IIAPI_xxxPARM varible assignment.
    ** Yes if a "=" follows IIAPI_xxxParm
    ** declaration or a reused variable. 
    */
    if ( *line_buf_ptr == '=' )
	if (
	    parser_mode == Var_Decl_Mode ||
	    parser_mode == Var_Reuse_Mode
	   )
	{	
	    line_buf_ptr++;

	    return 	( Var_Assgn_Mode );
	}


    varName = (char *)ait_malloc( STlength( line_buf_ptr ) + 1 );
    ptr = line_buf_ptr;

    while ( *ptr != EOS && *ptr != ' ' && *ptr != '\t' &&
	    *ptr != '('  && *ptr != '=' )
	varName[i++] = *ptr++;

    varName[i] = EOS;
    
    /* 
    ** Check if the input text is reused
    ** parmBlockname. 
    ** Yes if varName is in the callTrackList.
    */
    if ( isParmBlockNamDecled( callTrackList, varName ) == TRUE )
    {
	MEfree ( varName );

	return	( Var_Reuse_Mode ); 
    }

    /*
    ** Detect keyword "repeat".
    */
    if ( STequal( varName, repeat_sign ) )
    {
	line_buf_ptr = ptr;
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;
	repeat_on = TRUE;
    }

    /* 
    ** unrecognized syntax 
    */
    if ( STlength( varName ) <= STlength( "IIAPI_" ) && repeat_on == FALSE )
    {	
	MEfree ( varName );
	return	( mode );	
    }

    /* 
    ** Check if the input text is IIAPI_xxxParm
    ** variable declaration.
    */
    if ( ! STncmp( line_buf_ptr, "IIAPI_", STlength( "IIAPI_" )) &&
	 repeat_on == FALSE
       )
    {
	MEfree ( varName );

	return ( Var_Decl_Mode );
    }

    /* 
    ** Check if the input text is IIapi_xxxx
    ** function call.
    */
    if ( ! STncmp( line_buf_ptr, "IIapi_", STlength( "IIapi_" ) ) )
    {
	MEfree ( varName );

	return ( Fun_Call_Mode );
    }

    /*
    ** Illegal mode 
    */
    MEfree( varName );

    return 	( mode );
}






/*
** Name: Var_Decl_Syntax() - variable declaration parsing function
**
** Description:
**	This functions parses the variable declaration syntax.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Var_Decl_Syntax()
{
    AIT_TRACE( AIT_TR_TRACE )( "Var_Decl_Mode\n" );

    /*
    ** Variable declaration format:
    ** IIAPI_xxxParm  var_name
    ** IIAPI_xxxParm is parse into the left_side
    ** of current variable buffer curVarBuf, while
    ** var_name is parsed into right_side.
    */
    curVarBuf.left_side = BUF_ALLOC;
    Varname_Lex( curVarBuf.left_side, ' ', ' ', '\t' );
    
    SKIP_BLANKS_IN_LINE;

    GOTO_NEXT_WORD;
    
    /*
    ** If var_name is a string
    */
    if ( *line_buf_ptr == '\"' )
	curVarBuf.right_side = (char *) ait_malloc ( MAX_STR_LEN + 2 );
    else
	/*
        ** If var_name is a shell variable:
        ** $db, $user, or $passwd.
	*/
        if ( *line_buf_ptr == '$' )
   	    curVarBuf.right_side = (char *) ait_malloc ( MAX_VAR_LEN );
    	else
	    curVarBuf.right_side = BUF_ALLOC;
    Varname_Lex( curVarBuf.right_side, ' ', '\t', '=' );
    
    SKIP_BLANKS_IN_LINE;

    AIT_TRACE( AIT_TR_TRACE )
	( "%s %s---%s\n", 
	  curVarBuf.left_side, curVarBuf.right_side, line_buf_ptr);

    return;
}






/*
** Name: Var_Decl_Semantics - variable declaration semantics parsing
**
** Description:
**	This function parses the variable declaration based on the context
**     	of the input file, exits if incorrect semantics found.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-feb-95 (feige)
**	    Created.
*/

static VOID
Var_Decl_Semantics()
{
    i4				i;
    CALLTRACKLIST		*nodePtr;

    /*
    ** Check if left_side is a legal IIAPI_xxxParm
    */
    for ( i = 0; 
	  i < MAX_FUN_NUM &&
          ! STequal( funTBL[i].parmBlockName, curVarBuf.left_side );
	  i++);

    if ( i == MAX_FUN_NUM )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: unrecognized data type %s.\n",
	      cur_line_num, curVarBuf.left_side);

	SIprintf( "[line %d]: unrecognized data type %s.\n",
		  cur_line_num, curVarBuf.left_side);

    	MEfree ( curVarBuf.left_side );
    	MEfree ( curVarBuf.right_side );

	ait_exit();
    }

    /*
    ** Error if right_side has already been used
    ** in another variable declaration
    */
    if ( isParmBlockNamDecled( callTrackList, curVarBuf.right_side ) == TRUE )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: duplicated variable %s.\n",
	      cur_line_num, curVarBuf.right_side );

	SIprintf( "[line %d]: duplicated variable %s.\n",
		  cur_line_num, curVarBuf.right_side );

    	MEfree ( curVarBuf.left_side );
    	MEfree ( curVarBuf.right_side );

	ait_exit();
    }

    /*
    ** Allocate internal buffer for this
    ** variable declaration.
    */
    nodePtr = ( struct _callTrackList * ) 
	ait_malloc( sizeof( CALLTRACKLIST ) );

    if ( callTrackList == NULL )
   	callTrackList = lastCallPtr = curCallPtr = nodePtr;
    else
    {
	lastCallPtr->next = nodePtr;
	curCallPtr = lastCallPtr = lastCallPtr->next;
    }

    lastCallPtr->next = NULL;
    lastCallPtr->funNum = i;
    lastCallPtr->funInfo = &funTBL[ i ];
    lastCallPtr->parmBlockName = (char *) ait_malloc (
				 STlength( curVarBuf.right_side ) + 1 );
    STcopy( curVarBuf.right_side, lastCallPtr->parmBlockName );
    lastCallPtr->parmBlockPtr = (II_PTR) ait_malloc(
				funTBL[lastCallPtr->funNum].parmBlockSize );
    lastCallPtr->repeated = FALSE;
    lastCallPtr->descriptor = NULL;

    MEfree ( curVarBuf.left_side );
    MEfree ( curVarBuf.right_side );

    return;
}






/*
** Name: Fun_Call_Syntax - API-function call syntax parsing function
**
** Description:
**	This function parses the API function call based on the input file,
**     	exits if input syntax is incorrect.
**
** Inputs:
** 	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Fun_Call_Syntax()
{

    AIT_TRACE( AIT_TR_TRACE )
	( "Fun_Call_Mode\n" );

    /*
    ** Function Call format:
    ** IIapi_xxxx ( var_name );
    */

    /*
    ** Parse function name IIapi_xxx
    ** into left_side.
    */
    curVarBuf.left_side = BUF_ALLOC;
    Varname_Lex( curVarBuf.left_side, '(', ' ', '\t' );

    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( *line_buf_ptr != '(' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Syntax error at '%c'. '(' expected.\n",
	      cur_line_num, *line_buf_ptr );

	SIprintf
	    ( "[line %d]: Syntax error at '%c'. '(' expected.\n",
	      cur_line_num, *line_buf_ptr );

   	MEfree ( curVarBuf.left_side );

    	ait_exit();
    }

    /* 
    ** *line_buf_ptr=='(' 
    */
    line_buf_ptr++;

    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    /*
    ** Parse variable name into right_side.
    */
    if ( *line_buf_ptr == '\"' )
	curVarBuf.right_side = (char *) ait_malloc ( MAX_STR_LEN + 2 );
    else 
    	if ( *line_buf_ptr == '$' )
   	    curVarBuf.right_side = (char *) ait_malloc ( MAX_VAR_LEN );
        else
	    curVarBuf.right_side = BUF_ALLOC;

    Varname_Lex( curVarBuf.right_side, ')', ' ', '\t' );

    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( *line_buf_ptr != ')' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Syntax error at '%c'. ')' expected.\n",
	      cur_line_num, *line_buf_ptr );

	SIprintf
	    ( "[line %d]: Syntax error at '%c'. ')' expected.\n",
	      cur_line_num, *line_buf_ptr );
	
	MEfree ( curVarBuf.left_side );
	MEfree ( curVarBuf.right_side );

    	ait_exit();
    }

    /* 
    ** *line_buf_ptr == ')' 
    */
    line_buf_ptr++;
    
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( *line_buf_ptr != ';' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Syntax error at '%c'. ';' expected.\n",
	      cur_line_num, *line_buf_ptr);

	SIprintf
	    ( "[line %d]: Syntax error at '%c'. ';' expected.\n",
	      cur_line_num, *line_buf_ptr );
	
	MEfree ( curVarBuf.left_side );
	MEfree ( curVarBuf.right_side );

    	ait_exit();
    }

    /* 
    ** *line_buf_ptr == ';' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;

    AIT_TRACE( AIT_TR_TRACE )
	( "%s---%s---%s\n", 
	  curVarBuf.left_side, curVarBuf.right_side, line_buf_ptr);

    return;
}






/*
** Name: Fun_Call_Semantics() - API-function call semantics parsing function
**
** Description:
**	This function checks the API function call semantics based on 
**	the context of the input file, exits if input is incorrect in 
**	sense of semantics.
**
** Inputs:
**	None.
** 
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Fun_Call_Semantics()
{
    i4				i;

    /*
    ** Check if left_side is a legal function name.
    */
    for ( i = 0; 
	  i < MAX_FUN_NUM &&
	  ! STequal( funTBL[i].funName, curVarBuf.left_side );
	  i++ );

    if ( i == MAX_FUN_NUM )
    {
        AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: unrecognized function name %s.\n",
              cur_line_num, curVarBuf.left_side );

        SIprintf( "[line %d]: unrecognized function name %s.\n",
                  cur_line_num, curVarBuf.left_side);

	MEfree (curVarBuf.left_side );
	MEfree (curVarBuf.right_side );

        ait_exit();
    }

    /*
    ** Error if right_side variable is not declared.
    */
    if ( isParmBlockNamDecled( callTrackList, curVarBuf.right_side ) == FALSE )
    {
        AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: undeclared varrible %s.\n",
              cur_line_num, curVarBuf.right_side );

        SIprintf( "[line %d]: undeclared varrible %s.\n",
                  cur_line_num, curVarBuf.right_side );

	MEfree ( curVarBuf.left_side );
	MEfree (curVarBuf.right_side );

        ait_exit();
    }

    /*
    ** Error if right_side variable is declared of a
    ** data type not for left_side function.
    */
    if ( curCallPtr->funNum != i )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: %s is not of  data type %s\n",
	      cur_line_num, curVarBuf.right_side, funTBL[i].parmBlockName);

	SIprintf( "[line %d]: %s is not of  data type %s\n",
		  cur_line_num, curVarBuf.right_side, funTBL[i].parmBlockName);

	MEfree ( curVarBuf.left_side );
	MEfree ( curVarBuf.right_side );

	ait_exit();
    }

    /*
    ** Legal function call mode
    */
    curCallPtr->repeated = repeat_on;
    repeat_on = FALSE;

    MEfree ( curVarBuf.left_side );
    MEfree ( curVarBuf.right_side );

    return;
}






/*
** Name: Var_Reuse_Syntax() - reusad variable syntax checking function
**
** Description:
** 	This function checks the reused variable syntax.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Var_Reuse_Syntax()
{
    /*
    ** Syntax and Semantics check actually has been done
    ** in Parser_Mode().
    ** Stricter check on var_name is performed by
    ** Varname_Lex() here.
    */
    AIT_TRACE( AIT_TR_TRACE )
	( "Var_Reuse_Mode\n" );

    /*
    ** Parsed var_name into right_side.
    */
    if ( *line_buf_ptr == '\"' )
	curVarBuf.right_side = (char *) ait_malloc ( MAX_STR_LEN + 2 );
    else
    	if ( *line_buf_ptr == '$' )
   	    curVarBuf.right_side = (char *) ait_malloc ( MAX_VAR_LEN );
    	else
	    curVarBuf.right_side = BUF_ALLOC;
    
    Varname_Lex( curVarBuf.right_side, ' ', '\t', '=' );
    
    SKIP_BLANKS_IN_LINE;

    AIT_TRACE( AIT_TR_TRACE )
        ( "%s %s---%s\n", 
	  curVarBuf.left_side, curVarBuf.right_side, line_buf_ptr );

/*
    MEfree ( curVarBuf.left_side );
*/
    MEfree ( curVarBuf.right_side );

    return;
}

    
    
    
    

/*
** Name: Var_Assgn_Syntax() - variable assignment syntax parsing function
**
** Description:
**	This function checks the variable assignment syntax, exits when 
**	syntax error is found.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** return:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Var_Assgn_Syntax()
{
    AIT_TRACE( AIT_TR_TRACE )
	( "Var_Assgn_Mode\n" );

    /*
    ** Variable assignment is legal if and only if
    ** when immediatedly preceed by variable
    ** declaration or variable reusing mode.
    ** Format:
    **  = { field-assignment;
    **	    ...
    **	    field-assignment;
    **    };
    */
   
    /* 
    ** *line_buf_ptr == "=' 
    */
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( *line_buf_ptr != '{' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Syntax error at '%c'. '{' is expected.\n",
	      cur_line_num, *line_buf_ptr );
    
	SIprintf
	    ( "[line %d]: Syntax error at '%c'. '{' is expected.\n",
	      cur_line_num, *line_buf_ptr );
    
	ait_exit();
    }

    /* 
    ** *line_buf_ptr == '{' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    /*
    ** Field assignment
    */
    Field_Assgn_Parse();

    /* 
    ** *line_buf_ptr == '}' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( *line_buf_ptr != ';' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Syntax error at '%c'. ';' is expected.\n",
	      cur_line_num, *line_buf_ptr );

	SIprintf
	    ( "[line %d]: Syntax error at '%c'. ';' is expected.\n",
	      cur_line_num, *line_buf_ptr );

	ait_exit();
    }

    /* *line_buf_ptr == ';' */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;

    return;
}






/*
** Name: Field_Assgn_Parse() - IIAPI_xxxPARM field assignment parsing function
**
** Description:
**	This function checks the syntax of IIAPI_ parameter structure 
**	field assignment, exits when error found.
**
** Inputs:
**	None.
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

static VOID	
Field_Assgn_Parse()
{
    i4  	i = 0, j;
    char	*index;
    II_BOOL	is_formatSrcval = FALSE;	

    /*
    ** Each field assignment statement takes the format:
    **
    ** 	   field_name = field_value;
    **		or
    **	   field_name = descriptor-list;
    **	        or
    **	   field_name = IIAPI_DATAVALUE-value-list;
    **
    ** One variable assignment usually consists of
    ** several field assignment statements. An array
    ** curVarAsgn is used as the buffer for all the
    ** field assignments within a variable assignment.
    */
    for ( i = 0; 
          i < MAX_FIELD_NUM -1 && isEOF==FALSE && *line_buf_ptr != '}'; 
	  i++ )
    {
	/*
   	** Field_name is parsed into left_side.
 	*/
	curVarAsgn[i].left_side = BUF_ALLOC;
	Varname_Lex( curVarAsgn[i].left_side, ' ', '\t', '=' );
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	if ( STequal( curVarAsgn[i].left_side, "gc_columnData" ) ||
	     STequal( curVarAsgn[i].left_side, "fd_dstValue" ) )
	{
	    /*
	    ** gc_columnData requires a different field 
	    ** assignment format:
	    **
	    **	   gc_columnData using descriptor-name
	    **
	    ** Error if the next 5 chars are not keyword "using".
	    */
	    if ( STncmp( line_buf_ptr, "using", STlength ( "using" ) ) )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: keyword \"using\" expected after \"gc_columnData/fd_dstValue\".\n",
		      cur_line_num
		    );

		SIprintf
		    ( "[line %d]: keyword \"using\" expected after \"gc_columnData/fd_dstValue\".\n",
		      cur_line_num
		    );
		
		for ( j = 0; j < i; j++ )
		{
		    MEfree( curVarAsgn[j].left_side );
		    MEfree( curVarAsgn[j].right_side );
		}
		MEfree ( curVarAsgn[i].left_side );

		ait_exit();
	    }
	   
	    /*
	    ** line_but_ptr points to the last letter 'g'
	    ** in "using"
	    */
	    line_buf_ptr += STlength( "using" ) -1;
	}
	else	 
	    if ( *line_buf_ptr != '=' )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: Syntax error:'%c'. '=' expected.\n", 
		      cur_line_num, *line_buf_ptr );

		SIprintf
		    ( "[line %d]: Syntax error:'%c'. '=' expected.\n", 
		      cur_line_num, *line_buf_ptr);

		for ( j = 0; j < i; j++ )
		{
		    MEfree( curVarAsgn[j].left_side );
		    MEfree( curVarAsgn[j].right_side );
		}
		MEfree( curVarAsgn[i].left_side );

		ait_exit();
	    }

	/* 
	** *line_buf_ptr == '=' 
	*/
	line_buf_ptr++;
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	/*
	** Single '{' marks the beginning of an
	** IIAPI_DATAVALUE value list.
	**
	** Two continous '{' (excluding blanks or
	** newlines between the two '{'s) marks the
	** beginning of an descriptor list.
	*/
	index = STindex( line_buf_ptr, ".", 0);
	
	if ( STequal( curVarAsgn[i].left_side, "fd_srcValue" ) &&
		index != NULL && STequal( index, ".fd_dstValue;" ) )
	{
	    is_formatSrcval = TRUE;	
	    formatData_srcVal( line_buf_ptr );
	}
	else
	{
	    if ( *line_buf_ptr == '{' )
	    {
	    	line_buf_ptr++;
	    	SKIP_BLANKS_IN_LINE;
	    	GOTO_NEXT_WORD;
	    	if ( *line_buf_ptr == '{' )
 	    	{	
		    /*
		    ** right_side is a descriptor list.
		    */
		    curVarAsgn[i].right_side = (char *) ait_malloc (
					   	STlength( "_DESCRIPTOR" ) + 1 );
		    STcopy( "_DESCRIPTOR", curVarAsgn[i].right_side );
		    Dscrpt_Assgn_Parse( i );
	    	}
	    	else
	    	{
		    /*
		    ** right_side is IIAPI_DATAVALUE value list.
		    */
		    curVarAsgn[i].right_side = (char *) ait_malloc (
						STlength( "_DATAVALUE" ) + 1 );
		    STcopy( "_DATAVALUE", curVarAsgn[i].right_side );
		    Dataval_Assgn_Parse( i );
	        }
		
	        continue;
	    }
        }
	/* 
	** non-descriptor & non-datavalue assignment 
	*/

	if ( *line_buf_ptr == '\"' )
	    curVarAsgn[i].right_side = (char *) ait_malloc ( MAX_STR_LEN + 2 );
	else
	    if ( *line_buf_ptr == '$' )
	    	curVarAsgn[i].right_side = (char *) ait_malloc ( MAX_VAR_LEN );
	    else
	    	curVarAsgn[i].right_side = BUF_ALLOC;

	Varval_Lex( curVarAsgn[i].right_side, ';', ' ', '\t' );

        AIT_TRACE( AIT_TR_TRACE )
	    ( "%s %s---%s\n", 
	      curVarAsgn[i].left_side, curVarAsgn[i].right_side, 
	      line_buf_ptr);

 	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;
	
	if ( *line_buf_ptr == ';' )
	{
	    line_buf_ptr++;
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	}
	else if( ! is_formatSrcval )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' syntax err. ';' expected.\n",
		   cur_line_num, *line_buf_ptr );

	    SIprintf
		( "[line %d]: '%c' syntax err. ';' expected.\n",
		  cur_line_num, *line_buf_ptr);

	    for ( j = 0; j <= i; j++ )
	    {
		MEfree( curVarAsgn[j].left_side );
		MEfree ( curVarAsgn[j].right_side );
	    }

	    ait_exit();
	}
    }	/* for */

    /*
    ** Error if '{' and '}' are mismatched.
    */ 
    if ( isEOF == TRUE )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Incorrect EOF. '}' is expected. \n",
	      cur_line_num-1);

	SIprintf( "[line %d]: Incorrect EOF. '}' is expected. \n",
		  cur_line_num-1);

	for ( j = 0; j < i; j++ )
	{
	    MEfree( curVarAsgn[j].left_side );
	    MEfree( curVarAsgn[j].right_side );
	}

	ait_exit();
    }

    /*
    ** Number of reasonable field assignments within
    ** a variable assignment is upper-bounded by
    ** MAX_FIELD_NUM.
    ** Error if exceeded.
    */
    if ( i == MAX_FIELD_NUM - 1 )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "%s %s has too many fields.\n", 
	      curVarBuf.left_side, curVarBuf.right_side );

	SIprintf( "%s %s has too many fields.\n", 
		  curVarBuf.left_side, curVarBuf.right_side );

	for ( j = 0; j < i; j++ )
	{
	    MEfree( curVarAsgn[j].left_side );
	    MEfree( curVarAsgn[j].right_side );
	}

	ait_exit();
    }

    /*
    ** Since various IIAPI_xxxParm variable assignments
    ** take various number of field assignments, symbol
    ** "_NULL" is used to indicate the end of all the
    ** field assignments in a variable assignment.
    */
    curVarAsgn[i].left_side = ( char * ) ait_malloc
				  ( STlength( "_NULL" ) + 1 );
    STcopy( "_NULL", curVarAsgn[i].left_side );

    return;
}






/*
** Name: Dataval_Assgn_Parse() - IIAPI_DATAVALUE value assignment syntax parsing
**
** Description:
**	This function checks the IIAPI_DATAVALUE data value assignment syntax, 
**	buffers the array of valus in an internal linked list dataValList 
**	if the syntax is correct, exits otherwise.
**
** Inputs:
**	maxIndex - the number of fields that have been parsed within the
**                 scope of current IIAPI_xxxPARM variable assignment in
**                 the input file
**
** Outputs:
** 	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	12-May-95 (feige)
**	    Changed the farmat of null value form NULL() to NULL.
*/

static VOID
Dataval_Assgn_Parse
( 
    i4  maxIndex 
)
{
    struct _varRec	curDataPair;
    struct _dataValList	*nodeptr, *tail;
    i4			i;

    /*
    ** IIAPI_DATAVALUE value list format:
    **
    **	{
    **	    DATATYPE( value ),
    **      ...
    **      DATATYPE( value )
    **	}
    **
    ** If dv_value is NULL, an entry of 
    ** DATATYPE( value ) is replace by
    ** NULL.
    **
    ** A value list format will be converted
    ** to an array of type IIAPI_DATAVALUE  
    ** in aitbind.c.
    ** The check of the mismatch of DATATYPE
    ** and value will be done in aitbind.c.
    */

    while ( isEOF == FALSE && *line_buf_ptr !='}' )
    {
	curDataPair.left_side = BUF_ALLOC;

  	/*
	** NULL value processing
	*/
	if ( ! STncmp( line_buf_ptr, "NULL", STlength( "NULL" ) )
	   )
	{
	    STcopy( "NULL", curDataPair.left_side );
	    line_buf_ptr += STlength( "NULL" ) - 1;
	    curDataPair.right_side = ( char * )
		ait_malloc( STlength( "NULL" ) + 1 );
	    STcopy( "NULL", curDataPair.right_side );

	    goto _deliminator;
	}

 	/*
	** Parse the DATATYPE into left_side.
	*/
	Varname_Lex( curDataPair.left_side, ' ', '\t', '(' );
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	if ( *line_buf_ptr != '(' )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' unrecognized. '(' expected.\n",
		  cur_line_num, *line_buf_ptr );

	    SIprintf
		( "[line %d]: '%c' unrecognized. '(' expected.\n",
		  cur_line_num, *line_buf_ptr );

	    for ( i = 0; i < maxIndex; i++ )
	    {
		MEfree( curVarAsgn[i].left_side );
		MEfree( curVarAsgn[i].right_side );
	    }

	    MEfree (curVarAsgn[maxIndex].left_side );
	    MEfree (curDataPair.left_side );

	    ait_exit();
	}

	/* 
	** else *line_buf_ptr == '(' 
	*/
	line_buf_ptr++;
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

     
	/*
	** Parse value into right_side.
	*/
	if ( *line_buf_ptr == '\"' )
	    curDataPair.right_side = (char *)
		ait_malloc ( MAX_STR_LEN + 2 );
	else
	    if ( *line_buf_ptr == '$' )
		curDataPair.right_side = (char *)
		    ait_malloc ( MAX_VAR_LEN );
	else
	    curDataPair.right_side = BUF_ALLOC;

	Varval_Lex( curDataPair.right_side, ' ', '\t', ')' );

	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	if ( *line_buf_ptr != ')' )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' unrecognized. ')' expected.\n",
		  cur_line_num, *line_buf_ptr );
	    SIprintf
		( "[line %d]: '%c' unrecognized. ')' expected.\n",
		  cur_line_num, *line_buf_ptr );

	    for (i = 0; i < maxIndex; i++ )
	    {
		MEfree( curVarAsgn[i].left_side );
		MEfree( curVarAsgn[i].right_side );
	    }

	    MEfree( curVarAsgn[maxIndex].left_side );
	    MEfree( curDataPair.left_side );
	    MEfree( curDataPair.right_side );

	    ait_exit();
	}

	/*
	** else *line_buf_ptr == ')' or 'L'
	*/
_deliminator:

	line_buf_ptr++;
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	if ( *line_buf_ptr != ',' && *line_buf_ptr != '}' )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' unrecognized.',' or '}' expected.\n",
		  cur_line_num, *line_buf_ptr );

	    SIprintf
		( "[line %d]: '%c' unrecognized.',' or '}' expected.\n",
		  cur_line_num, *line_buf_ptr );

	    for ( i = 0; i < maxIndex; i++ )
	    {
		MEfree( curVarAsgn[i].left_side );
		MEfree( curVarAsgn[i].right_side );
	    }

	    MEfree( curVarAsgn[maxIndex].left_side );
	    MEfree( curDataPair.left_side );
	    MEfree( curDataPair.right_side );

	    ait_exit();
	}

	if ( *line_buf_ptr == ',' )
	{
	    line_buf_ptr++;
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	}

	AIT_TRACE( AIT_TR_TRACE )
	    ( "%s %s---%s\n", 
	      curDataPair.left_side, curDataPair.right_side, line_buf_ptr );

	/*
	** Buffer left_side and right_side on dataValList.
	*/
	nodeptr = (struct _dataValList *)
	    ait_malloc ( sizeof( struct _dataValList ) );

	if ( dataValList == NULL )
	    dataValList = tail = nodeptr;
	else
	{
	    tail->next = nodeptr;
	    tail = tail->next;
	}

	tail->valBuf.left_side = (char *) ait_malloc
	      ( STlength( curDataPair.left_side ) + 1 );
	STcopy( curDataPair.left_side, tail->valBuf.left_side );
	tail->valBuf.right_side = (char *) ait_malloc
	       ( STlength( curDataPair.right_side ) + 1 );

	STcopy( curDataPair.right_side, tail->valBuf.right_side );
	tail->next = NULL;

	MEfree( curDataPair.left_side );
	MEfree( curDataPair.right_side );

	AIT_TRACE( AIT_TR_TRACE )
	    ( "list-ele: %s %s\n", 
	      tail->valBuf.left_side, tail->valBuf.right_side );
    } /* while */


    /*
    ** Check the mismatch of '{' and '}'.
    */
    if ( isEOF == TRUE )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Incorrect EOF. '}' expected. \n",
	      cur_line_num-1);

	SIprintf( "[line %d]: Incorrect EOF. '}' expected. \n",
		  cur_line_num-1 );

	for ( i = 0; i < maxIndex; i++ )
	{
	    MEfree( curVarAsgn[i].left_side );
	    MEfree( curVarAsgn[i].right_side );
	}
	MEfree( curVarAsgn[maxIndex].left_side );

	ait_exit();
    }

    /* 
    ** else *line_buf_ptr == '}' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD; 

    if ( *line_buf_ptr != ';' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: '%c' unrecognized. ';' expected.\n",
	      cur_line_num, *line_buf_ptr );

	SIprintf( "[line %d]: '%c' unrecognized. ';' expected.\n",
		  cur_line_num, *line_buf_ptr );

	for ( i = 0; i < maxIndex; i++ )
	{
	    MEfree( curVarAsgn[i].left_side );
	    MEfree( curVarAsgn[i].right_side );
	}
	MEfree( curVarAsgn[maxIndex].left_side );

	ait_exit();
    }
   
    /* 
    ** else *line_buf_ptr == ';' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;  

    return;
}






/*
** Name: Dscrpt_Assgn_Parse() - descriptor value assignment syntax
**				parsing function
**
** Description:
**	This function checks the syntax of IIAPI_DESCRIPTOR data value 
**	assignment, buffers the array of values into an internal linked 
**	list descriptorList if the input syntax is correct, exits otherwise.
**
** Inputs:
**	maxIndex - the number of fields that have been parsed within the
**		   scope of current IIAPI_xxxPARM variable assignment in
**		   the input file
**
** Outputs:
** 	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Jul-95 (feige)
**	    Supported syntax ds_length = sizeof( $shell_variable );
**	11-Sep-98 (rajus01)
**	    Use the correct descriptor list to build it.
*/

static VOID
Dscrpt_Assgn_Parse
( 
    i4	maxIndex 
)
{
    struct _varRec		curDscrptPair;
    struct _descriptorList	*nodeptr, *tail;
    i4				i, j, descr;

    /*
    ** Descriptor list format:
    **
    **	{
    **	  {
    **		ds_dataType = value;
    **		ds_length = value;
    **		or ds_length = sizeof( $shell_variable );
    **		ds_precision = value;
    **		ds_scale = value;
    **		ds_columnType = value;
    **		ds_columnName = value;
    **	  },
    **    ...
    **    {
    **     	...
    **    }
    **  }
    **
    ** All fields of each descriptor (ds_datatype,
    ** len, prec., scale, etc.) are optional.
    ** The check of the mismatch of field type and
    ** value will be done in aitbind.c.
    */
	

    /* 
    ** *line_buf_ptr == '{' 
    */

    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;

    if ( descriptorCount >= MAX_DESCR_LIST )
    {
	AIT_TRACE( AIT_TR_ERROR )
	( "Invalid descriptorCount %d.\n", descriptorCount );
	    SIprintf( "Invalid descriptorCount %d.\n", descriptorCount );
	ait_exit();
    }

    descr = descriptorCount++;
    
    while ( isEOF == FALSE && *line_buf_ptr !='}' )
    {
	nodeptr = (struct _descriptorList *) 
           	  ait_malloc ( sizeof( struct _descriptorList ) );
	if ( nodeptr == NULL)
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "Parser err: out of memory.\n" );
	    SIprintf( "Parser err: out of memory.\n" );
	   
	    for ( j = 0; j < maxIndex; j++ )
	    {
		MEfree( curVarAsgn[j].left_side );
		MEfree( curVarAsgn[j].right_side );
	    }
	    MEfree( curVarAsgn[maxIndex].left_side );

	    ait_exit();
	}

        if ( descriptorList[ descr ] == NULL )
	    descriptorList[ descr ] = tail = nodeptr;
	else
	{
            tail->next = nodeptr;
	    tail = tail->next;
	}

	tail->next = NULL;

	for ( i = 0; i < 7 && isEOF == FALSE; i++ )
	{
	    /*
	    ** Parse field name (ds_dataType,...,
	    ** ds_columnName) into left_side.
	    */	
	    curDscrptPair.left_side = BUF_ALLOC;
	    Varname_Lex( curDscrptPair.left_side, ' ', '\t', '=' );
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;

	    if ( *line_buf_ptr != '=' )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: '%c' unrecognized. '=' expected.\n",
		      cur_line_num, *line_buf_ptr );

		SIprintf
		    ( "[line %d]: '%c' unrecognized. '=' expected.\n",
		      cur_line_num, *line_buf_ptr );

	    	for ( j = 0; j < maxIndex; j++ )
	    	{
		    MEfree( curVarAsgn[j].left_side );
		    MEfree( curVarAsgn[j].right_side );
	    	}
	    	MEfree( curVarAsgn[maxIndex].left_side );
		MEfree( curDscrptPair.left_side );

	    	ait_exit();
	    }

	    line_buf_ptr++;
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	    
	    /*
	    ** Parse value into right_side.
	    */
	    if ( *line_buf_ptr == '\"' )
		curDscrptPair.right_side = (char *)
		    ait_malloc ( MAX_STR_LEN + 2 );
	    else
	     	if ( *line_buf_ptr == '$' )
		    curDscrptPair.right_side = (char *)
			ait_malloc (  MAX_VAR_LEN );
	    	else
		    curDscrptPair.right_side = BUF_ALLOC;

	    if ( STequal( curDscrptPair.left_side,
			  "ds_length"
			) &&
		 STlength( line_buf_ptr ) >= STlength( "sizeof" ) &&
		 ! STncmp( line_buf_ptr, "sizeof", STlength( "sizeof" ) )
	       )
	    {
		line_buf_ptr += STlength( "sizeof" );
		SKIP_BLANKS_IN_LINE;
		GOTO_NEXT_WORD;

		if ( *line_buf_ptr != '(' )
		{
		    AIT_TRACE( AIT_TR_ERROR )
			( "[line %d]: ( is missing from or misplaced in %s.\n",
			  cur_line_num,
			  line_buf_ptr
			);
		    SIprintf
			( "[line %d]: ( is missing from or misplaced in %s.\n",
			  cur_line_num,
			  line_buf_ptr
			);
		
		ait_exit();
		}

		/*
		** *line_buf_ptr == '('
		*/
		line_buf_ptr++;

		SKIP_BLANKS_IN_LINE;
		GOTO_NEXT_WORD;

		Varval_Lex( curDscrptPair.right_side, ' ', '\t', ')' );
		SKIP_BLANKS_IN_LINE;
		GOTO_NEXT_WORD;

		/*
		** *line_buf_ptr == ')'
		*/
		line_buf_ptr++;
	    }
	    else
		Varval_Lex( curDscrptPair.right_side, ' ', '\t', ';' );

	    tail->descrptRec[i].left_side = (char *) ait_malloc
		( STlength( curDscrptPair.left_side ) + 1 );
	    STcopy(curDscrptPair.left_side, tail->descrptRec[i].left_side);
	    tail->descrptRec[i].right_side = (char *) ait_malloc
		( STlength( curDscrptPair.right_side ) + 1 );
	    STcopy( curDscrptPair.right_side, tail->descrptRec[i].right_side );

	    MEfree( curDscrptPair.left_side );
	    MEfree( curDscrptPair.right_side );

	    AIT_TRACE( AIT_TR_TRACE )
		( "DESCRPT: %s %s\n", tail->descrptRec[i].left_side,
		  tail->descrptRec[i].right_side );
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	    
	    if ( *line_buf_ptr != ';' )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: '%c' unrecognized. ';' expected.\n",
		      cur_line_num, *line_buf_ptr );

		SIprintf
		    ( "[line %d]: '%c' unrecognized. ';' expected.\n",
		      cur_line_num, *line_buf_ptr );

	    	for ( j = 0; j < maxIndex; j++ )
	    	{
		    MEfree( curVarAsgn[j].left_side );
		    MEfree( curVarAsgn[j].right_side );
	    	}
	    	MEfree( curVarAsgn[maxIndex].left_side );

	    	ait_exit();
	    }

	    /* 
	    ** else *line_buf_ptr==';' 
	    */
	    line_buf_ptr++;
            SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;

            if ( *line_buf_ptr == '}' )
		break;
	}   /* for */

        if ( isEOF == TRUE )
     	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: Incorrect EOF. '}' expected. \n",
		  cur_line_num-1 );

	    SIprintf
		( "[line %d]: Incorrect EOF. '}' expected. \n",
		  cur_line_num-1 );

	    for ( j = 0; j < maxIndex; j++ )
	    {
	        MEfree( curVarAsgn[j].left_side );
	        MEfree( curVarAsgn[j].right_side );
	    }
	    MEfree( curVarAsgn[maxIndex].left_side );

	    ait_exit();
    	}

	if ( i == 7 )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: too many fields for the descriptor.\n",
		  cur_line_num );

	    SIprintf
		( "[line %d]: too many fields for the descriptor.\n",
		  cur_line_num );

	    for ( j = 0; j < maxIndex; j++ )
	    {
	        MEfree( curVarAsgn[j].left_side );
	        MEfree( curVarAsgn[j].right_side );
	    }
	    MEfree( curVarAsgn[maxIndex].left_side );

	    ait_exit();
  	}

	/* 
	** *line_buf_ptr=='}'--inner '}' 
	*/
/*
	STcopy("_NULL", curDscrptPair.left_side);
*/
	tail->descrptRec[i+1].left_side = (char *) ait_malloc (
					       STlength( "_NULL" ) + 1 );
	STcopy( "_NULL", tail->descrptRec[i+1].left_side );
	line_buf_ptr++;
	SKIP_BLANKS_IN_LINE;
	GOTO_NEXT_WORD;

	/*
	** Check next descriptor.
	*/
        if ( *line_buf_ptr==',' )
        {
	    line_buf_ptr++;
	    SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	    
            if ( *line_buf_ptr!='{' ) 
	    {
	     	AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: '%c' unrecognized. '{' expected.\n",
		      cur_line_num, *line_buf_ptr );
		SIprintf
		    ( "[line %d]: '%c' unrecognized. '{' expected.\n",
		      cur_line_num, *line_buf_ptr );
	    	for ( j = 0; j < maxIndex; j++ )
	    	{
		    MEfree( curVarAsgn[j].left_side );
		    MEfree( curVarAsgn[j].right_side );
	    	}
	    	MEfree( curVarAsgn[maxIndex].left_side );
	     	ait_exit();
	    }

	    /* 
	    ** else *line_buf_ptr == inner '{' 
	    */
	    line_buf_ptr++;
            SKIP_BLANKS_IN_LINE;
	    GOTO_NEXT_WORD;
	    continue;
	}
            

        /* 
	** Error if *line_buf_ptr is not outer '}' 
	*/
        if ( *line_buf_ptr!='}' )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' unrecognized. '}' expected.\n",
		  cur_line_num );

	    SIprintf
		( "[line %d]: '%c' unrecognized. '}' expected.\n",

		  cur_line_num );
	    for ( j = 0; j < maxIndex; j++ )
	    {
		MEfree( curVarAsgn[j].left_side );
		MEfree( curVarAsgn[j].right_side );
	    }
	    MEfree( curVarAsgn[maxIndex].left_side );

	    ait_exit();
	}

    }	/* while */

    if ( isEOF == TRUE )
    {
    	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Incorrect EOF. '}' expected. \n",
	      cur_line_num-1 );

    	SIprintf( "[line %d]: Incorrect EOF. '}' expected. \n",
	          cur_line_num-1 );

	for ( j = 0; j < maxIndex; j++ )
	{
	    MEfree( curVarAsgn[j].left_side );
	    MEfree( curVarAsgn[j].right_side );
	}
	MEfree( curVarAsgn[maxIndex].left_side );

 	ait_exit();
    }

    /* 
    ** else *line_buf_ptr == outer '}' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD; 

    if ( *line_buf_ptr != ';' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: '%c' unrecognized. ';' expected.\n",
	      cur_line_num, *line_buf_ptr );

	SIprintf( "[line %d]: '%c' unrecognized. ';' expected.\n",
		  cur_line_num, *line_buf_ptr );

	for ( j = 0; j < maxIndex; j++ )
	{
	    MEfree( curVarAsgn[j].left_side );
	    MEfree( curVarAsgn[j].right_side );
	}
	MEfree( curVarAsgn[maxIndex].left_side );

	ait_exit();
    }
   
    /* 
    ** else *line_buf_ptr == ';' 
    */
    line_buf_ptr++;
    SKIP_BLANKS_IN_LINE;
    GOTO_NEXT_WORD;  

    return;
}






/*
** Name: Var_Assgn_Semantics() - variable assignment semantics checking function
**
** Desription:
**	This function checks the semantics of variable assignment
**	based on the context of the input file, calls related function
**	in module aitproc.c to bind the variable to an internal buffer
**	if the semantics is correct, exits otherwise.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	11-Sep-98 (rajus01)
**	    Free the descriptorList here.
*/

static VOID
Var_Assgn_Semantics()
{
    i4		funTBLIndex, curVarIndex = -1;
    i4		i, j;

    for ( i = 0; 
  	  i < MAX_FIELD_NUM && 
	  ! STequal( curVarAsgn[i].left_side, "_NULL" );
	  i++ )
    {
	if ( ( funTBLIndex = funTBL[curCallPtr->funNum].orderReqedParm ) >= 0 )
	    if ( STequal(
			 funTBL[curCallPtr->funNum].parmName[funTBLIndex],
			 curVarAsgn[i].left_side
			) )
	    {
		curVarIndex = i;
		continue; 
	    }

	for ( j = 0; 
	      ! STequal( funTBL[curCallPtr->funNum].parmName[j], "_NULL" );
	      j++)
	{
	    if ( STequal( funTBL[curCallPtr->funNum].parmName[j],
			  curVarAsgn[i].left_side
			) )
	    {
		(*bindfunTBL[curCallPtr->funNum])(j, i); 
		break;
	    }
	} /* inner for */

        if ( STequal( funTBL[curCallPtr->funNum].parmName[j], "_NULL" ) )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a field of %s\n", cur_line_num,
	          curVarAsgn[i].left_side, 
		  funTBL[curCallPtr->funNum].parmBlockName );

	    SIprintf
		( "[line %d]: %s is not a field of %s\n", cur_line_num,
	          curVarAsgn[i].left_side, 
		  funTBL[curCallPtr->funNum].parmBlockName );

    	    for ( j = 0;
	          j < MAX_FIELD_NUM && 
		  ! STequal( curVarAsgn[j].left_side, "_NULL" );
	          j++ )
    	    {
		MEfree( curVarAsgn[j].left_side );
		MEfree( curVarAsgn[j].right_side );
    	    }

            if ( j < MAX_FIELD_NUM )
	    MEfree( curVarAsgn[j].left_side );
            
	    ait_exit();
	}
	
    }	/* outer for */

    if ( curVarIndex >= 0 )
	(*bindfunTBL[curCallPtr->funNum])(
		funTBL[curCallPtr->funNum].orderReqedParm, curVarIndex ); 

    for ( i = 0;
	  i < MAX_FIELD_NUM && 
	  ! STequal( curVarAsgn[i].left_side, "_NULL" );
	  i++ )
    {
	MEfree( curVarAsgn[i].left_side );
	MEfree( curVarAsgn[i].right_side );
    }

    if ( i < MAX_FIELD_NUM )
	MEfree( curVarAsgn[i].left_side );

    free_dscrpList();

    return;
}






/*
** Name: isParmBlockNameDeclared - reused variable searching function
**
** Description:
**	This function looks up into the callTrackList to check if the 
**	input variable name is a reused one, points curCallPtr the the 
**	node in the callTrackList that the variable was declared the 
**	first time if found.
**
** Inputs:
**	callTrackL - callTrackList head node
**	varname - string of variable name
**
** Outputs:
**	None.
** 
** Returns:
**	TRUE	if found
**	FALSE	otherwise
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static bool
isParmBlockNamDecled
( 
    struct _callTrackList *callTrackL, 
    char 		  *varName 
)
{
    CALLTRACKLIST	*ptr;

    /*
    ** preReusedPtr is useless at this 
    ** point. However, in later version
    ** we will consider loop in the input
    ** file, then there may be more than
    ** one node on callTrackL with the same
    ** parameter block names. If this is
    ** the case, reused variable should
    ** always reuse the parameter block
    ** which is most recent declared.
    */

    preReusedPtr = curCallPtr = NULL;

    if ( STlength( varName ) == 0 )
	return ( FALSE );

    ptr = callTrackL;

    while ( ptr != NULL )
    { 
	if ( STequal( ptr->parmBlockName, varName ) )
	{	
	    preReusedPtr = curCallPtr;
	    curCallPtr = ptr;
	}
	ptr = ptr->next;
    }

    if ( curCallPtr != NULL )
	return ( TRUE );

    return ( FALSE );
}






/*
** Name: Varname_Lex() - field variable name parsing function
**
** Desription:
**	This function checks the left_hand syntax in the filed assignment,
**     	exits when error found.
**
** Inputs:
**	deliminator1 - legal deliminator 1 following the variable name
**	deliminator2 - legal diliminator 2 following the variable name
**	deliminator3 - legal deliminator 3 following the variable name
**
** Outputs:
**	varNameBuf - string buffer for the variable name
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
Varname_Lex
( 
    char *varNameBuf, 
    char deliminator1, 
    char deliminator2, 
    char deliminator3 
)
{
    i4  i = 0;

    /*
    ** Digital number can be at any place
    ** variable name except the 1st one.
    */
    if ( *line_buf_ptr >= '0' && *line_buf_ptr <= '9' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Variable name shouldn't start with a number.\n",
	      cur_line_num );

	SIprintf
	    ( "[line %d]: Variable name shouldn't start with a number.\n",
	      cur_line_num );

	ait_exit();
    }

    while ( *line_buf_ptr != EOS && 
	    *line_buf_ptr != deliminator1 &&
	    *line_buf_ptr != deliminator2 &&
	    *line_buf_ptr != deliminator3 ) 
    {
	if ( IS_CHAR_LEGAL )
	    varNameBuf[i++] = *line_buf_ptr++;
	else
        {
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: Syntax error: '%c'\n", 
		   cur_line_num, *line_buf_ptr );

	    SIprintf( "[line %d]: Syntax error: '%c'\n", 
		      cur_line_num, *line_buf_ptr );

	    ait_exit();
	}
    }
    varNameBuf[i] = EOS;

    return;
}






/*
** Name: Varval_Lex() - field variable value parsing function
**
** Desription:
**	This function checks the right_hand syntax in the filed assignment,
**    	exits when error found.
**
** Inputs:
**	deliminator1 - legal deliminator 1 following the variable name
**	deliminator2 - legal diliminator 2 following the variable name
**	deliminator3 - legal deliminator 3 following the variable name
**
** Outputs:
**	varNameBuf - string buffer for the variable value
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	14-Jul-95 (feige)
**	    Parsed two more shell variables:
**	    $user and $passwd.
**	27-Dec-95 (feige)
**     	    Supported embedded double-quote (") in string.	
*/

static VOID
Varval_Lex
( 
    char *valStrBuf, 
    char deliminator1, 
    char deliminator2, char 
    deliminator3 
)
{
    char	shellvar[MAX_VAR_LEN];
    bool 	isDotSign = FALSE;
    i4  	i = 0;

    /* 
    ** if shell variable assignment 
    */
    if ( *line_buf_ptr == '$' )
    {
	line_buf_ptr++;
	while ( *line_buf_ptr != EOS &&
	        *line_buf_ptr != deliminator1 &&
	        *line_buf_ptr != deliminator2 &&
	        *line_buf_ptr != deliminator3 )
	{
	    shellvar[i++] = *line_buf_ptr++;
	}

        shellvar[i] = EOS;

        if ( STequal( shellvar, "db" ) )
        {
	    if ( dbname == NULL )
	    {
	        AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		      cur_line_num, shellvar );

	        SIprintf
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		      cur_line_num, shellvar );

	        ait_exit();
	    }
	   
	    *valStrBuf = '"';
	    STcopy( dbname, ( char * )( valStrBuf + 1 ) );

	    return;
	}

	if ( STequal( shellvar, "passwd" ) )
        {
	    if ( password == NULL )
	    {
	        AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		     cur_line_num, shellvar );

	        SIprintf
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		      cur_line_num, shellvar );
	    
	        ait_exit();
	    }
	  
	    *valStrBuf = '"';
	    STcopy( password, ( char * )( valStrBuf + 1 ) );
	
	    return;
	} 

	if ( STequal( shellvar, "user" ) )
	{
	    if ( username == NULL )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		      cur_line_num, shellvar );

		SIprintf
		    ( "[line %d]: shell variable $%s unspecified in command line.\n",
		      cur_line_num, shellvar );

		ait_exit();
	    }

	    *valStrBuf = '"';
	    STcopy( username, ( char * )( valStrBuf + 1 ) );

	    return;
	}

       AIT_TRACE( AIT_TR_ERROR )
	   ( "[line %d]: unknown shell variable '%s'.\n",
	     cur_line_num, shellvar );

       SIprintf( "[line %d]: unknown shell variable '%s'.\n",
		 cur_line_num, shellvar );

       ait_exit();
    }

    /* 
    ** if string assignment 
    */
    if ( *line_buf_ptr == '"' )
    {
	/* 
	** put '"' as the 1st letter in valStrBuf to indicate 
	** taht the text followed '"' is a string 
	*/
	valStrBuf[i++] = *line_buf_ptr++;
	
	while( 
	       *line_buf_ptr != EOS  && 
	       *line_buf_ptr != '\"' &&
	       i < (MAX_STR_LEN + 1) 	/* Allow for leading quote */
	     )
	{
	    /*
	    ** Handle multi-line string.
            ** A '\' at the end of current
	    ** line indicated string continues
            ** in next line.
	    */
	    if ( *line_buf_ptr == '\\' && *(line_buf_ptr+1) == EOS )
	    {
		str_parse_on = TRUE;
		read_line();
		line_filter();
		continue;
	    }	    

	    if ( *line_buf_ptr == '\\' && *(line_buf_ptr+1) == '\"' )
		line_buf_ptr++;

	    valStrBuf[i++] = *line_buf_ptr++;
	}
     
	/*
	** Error if string is longer than 32000 (plus leading quote)
	*/
	if ( i > (MAX_STR_LEN + 1) )
        {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: quoated string is too long.\n",
		      cur_line_num );

		SIprintf
		    ( "[line %d]: quoated string is too long.\n",
		      cur_line_num );

		ait_exit();
	}

	/*
	** Error if neither '\' nor EOS
	** is the last characted in current
	** line.
	*/
	if ( *line_buf_ptr == EOS )
	{
		AIT_TRACE( AIT_TR_ERROR)
		    ( "[line %d]: invalid string assignment.\n",
		      cur_line_num );

		SIprintf
		    ( "[line %d]: invalid string assignment.\n",
		      cur_line_num);

		ait_exit();
	}

	/* 
	** else *line_buf_ptr=='"' 
	*/
        valStrBuf[i] = EOS;

	if ( str_parse_on == TRUE )
	    str_parse_on = FALSE;

	line_buf_ptr++;	

	if ( *line_buf_ptr!=EOS &&
	     *line_buf_ptr!=deliminator1 &&
	     *line_buf_ptr!=deliminator2 &&
	     *line_buf_ptr!=deliminator3 )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: '%c' syntax err. '%c' or '%c' expected.\n",
		   cur_line_num, 
		   *line_buf_ptr, deliminator1, deliminator2 );

	    SIprintf
		( "[line %d]: '%c' syntax err. '%c' or '%c' expected.\n",
		  cur_line_num, 
		  *line_buf_ptr, deliminator1, deliminator2 );

	    ait_exit();
	}
	
	return;
    }
	   

    /* 
    ** if numerical assignment 
    **
    ** Further check will be done in
    ** aitbind.c using CL functions.
    */
    if ( *line_buf_ptr == '+' || *line_buf_ptr == '-' )
	valStrBuf[i++] = *line_buf_ptr++;

    if ( *line_buf_ptr=='.' ||
        ( *line_buf_ptr>='0' && *line_buf_ptr<='9' ) ) 
    {
    	while ( i < MAX_VAR_LEN && *line_buf_ptr != EOS &&
	        *line_buf_ptr != deliminator1 && 
	        *line_buf_ptr != deliminator2 &&
	        *line_buf_ptr != deliminator3 )
    	{
	    if ( i > 0 && isDotSign == FALSE && *line_buf_ptr == '.' )
	    {
		/*
		** Only one '.' is allowed in number
		*/
		valStrBuf[i++] = *line_buf_ptr++;
		isDotSign = TRUE;

		continue;
	    }
	   if ( *line_buf_ptr >= '0' && *line_buf_ptr <= '9' )
	       valStrBuf[i++] = *line_buf_ptr++;
	   else
           {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: Syntax error: '%c'\n", 
		      cur_line_num, *line_buf_ptr );

		SIprintf
		    ( "[line %d]: Syntax error: '%c'\n", 
		      cur_line_num, *line_buf_ptr );

		ait_exit();
	   }
    	} /* while */
	    
	if ( i >= MAX_VAR_LEN )
        {
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: number has too many digits.\n",
		  cur_line_num );

	    SIprintf
		( "[line %d]: number has too many digits.\n",
		   cur_line_num );

	    ait_exit();
	}
			
	valStrBuf[i] = EOS;

	return;
    }
    
    /* 
    ** else variable name assignment 
    */
    while ( *line_buf_ptr != EOS && 
	    *line_buf_ptr != deliminator1 &&
	    *line_buf_ptr != deliminator2 &&
	    *line_buf_ptr != deliminator3 ) 
    {
	if ( i > 0 && isDotSign == FALSE && *line_buf_ptr == '.' )
	{
	    /*
	    ** Only one '.' is allowed in
	    ** variable name.
	    */
	    valStrBuf[i++] = *line_buf_ptr++;
	    isDotSign = TRUE;

	    continue;
	}

	if ( IS_CHAR_LEGAL )
	    valStrBuf[i++] = *line_buf_ptr++;
	else
        {
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: Syntax error: '%c'\n", 
		  cur_line_num, *line_buf_ptr );

	    SIprintf( "[line %d]: Syntax error: '%c'\n", 
		      cur_line_num, *line_buf_ptr);

	    ait_exit();
	}
    }

    if ( isDotSign == TRUE && valStrBuf[i-1] == '.' )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Incorrect value assignment.\n",
	      cur_line_num );

	SIprintf( "[line %d]: Incorrect value assignment.\n",
	    cur_line_num );

	ait_exit();
    }
    valStrBuf[i] = EOS;

    return;
}






/*
** Name: read_line() - input file reading function
**
** Description:
**	This function reads one line from input file.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

static VOID
read_line()
{
    STATUS		retcode;
	
    cur_line_num++;

    retcode = SIgetrec( input_line_buf, FILE_REC_LEN, inputf );

    if ( retcode == ENDFILE ) 
    {
    	AIT_TRACE( AIT_TR_TRACE )
	    ( "End of file\n" );

    	isEOF = TRUE;

    	return;
    }

    if ( retcode != OK )
    {
    	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: Abnormal terminate reading input file\n",
	      cur_line_num );

    	SIprintf( "[line %d]: Abnormal terminate reading input file\n",
	          cur_line_num );

	ait_exit();
    }

    return;
}






/*
** Name: line_filter() - input line filtering function
**
** Description:
**	This function trims the beginning blanks/tabs if current input 
**	line is not a string, trims the trailing blanks/tabs, and screens 
**	out commens if any.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	21-Dec-95 (feige)
**	    Checked line_buf_ptr after skipping blanks.
*/

static VOID
line_filter()
{
    char		*index1, *index2;

    line_buf_ptr = input_line_buf;

    /* 
    ** replace '\n' at the end of the input line with EOS 
    */
    *(line_buf_ptr + STlength( line_buf_ptr ) - 1 ) = EOS;

    /* 
    ** filter the beginning ' ' and '\t' in a line 
    */
    if ( str_parse_on == FALSE )
    while ( *line_buf_ptr == ' ' || *line_buf_ptr == '\t' )
    {
    	if ( *line_buf_ptr == ' ' )
	    line_buf_ptr = STskipblank( line_buf_ptr, 
					STlength( line_buf_ptr ) );

	if ( line_buf_ptr == NULL )
	    return;
	    
    	while ( *line_buf_ptr == '\t' )
    	    line_buf_ptr++;
    }	/* outer while */

    /* 
    ** filter the trailing blanks in a line 
    */
    STtrmwhite( line_buf_ptr );

    /* 
    ** filter the skipped comment starting with "//" in the line 
    */
    index1 = STchr( line_buf_ptr, skip_comment_sign );
    index2 = STrchr( line_buf_ptr, skip_comment_sign );
    if ( index1 != NULL && index2 != NULL && (index2-index1) == sizeof( char ) )
    {
    	*(line_buf_ptr+(index1-line_buf_ptr)) = EOS;	
    }

    /* 
    ** filter the skipped comment starting with "##" in the line 
    */
    index1 = STchr( line_buf_ptr, output_comment_sign );
    index2 = STrchr( line_buf_ptr, output_comment_sign );
    if ( index1 != NULL && index2 != NULL && (index2-index1) == sizeof( char ) )
    {
    	comment = (char *)ait_malloc(FILE_REC_LEN * sizeof(char));
	STcopy( index1, comment );
	*(line_buf_ptr+(index1-line_buf_ptr)) = EOS;	
    }

    AIT_TRACE( AIT_TR_TRACE )
    	( "[line %d].........................................\n",
	  cur_line_num );

    AIT_TRACE(AIT_TR_TRACE) 
        ( "%s\n", line_buf_ptr );

    return;
}

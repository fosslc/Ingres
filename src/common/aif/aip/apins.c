/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cm.h>
# include <id.h>
# include <gc.h>
# include <st.h>

# include <iicommon.h>
# include <gcn.h>

# include <iiapi.h>
# include <api.h>
# include <apishndl.h>
# include <apins.h>
# include <apimisc.h>
# include <apitrace.h>

/*
** Name: apins.c
**
** Description:
**	Name Server connection support functions.
**
**	IIapi_createGCNOper	Create GCN_NS_OPER message.
**	IIapi_parseNSQuery	Parse Name Server query text.
**	IIapi_validNSDescriptor	Validate input descriptor.
**	IIapi_validNSParams	Validate parameter values.
**	IIapi_saveNSParams	Save parameter values.
**	IIapi_getNSDescriptor	Build output descriptor.
**	IIapi_loadNSColumns	Process GCN_RESULT tuples.
**
** History:
**	18-Feb-97 (gordy)
**	    Created.
**	20-Aug-97 (rajus01)
**	    Added capability to specify security mechanisms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	22-Oct-04 (gordy)
**	    Added descriptor specific error codes.
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from MEcopy calls.
**	 1-Sep-09 (gordy)
**	    Increase supported length for user ID's to 256.  Use local
**	    max length for current user ID.
**	25-Mar-10 (gordy)
**	    Enhanced parameter memory block handling.
*/

/*
** Static string literals.
*/
static char	install_id[] = { "ii" };
static char	wild_card[] = { "*" };
static char	priv_type[] = { "Private" };
static char	glob_type[] = { "Global" };
static char	err_str[] = { "?" };
static char	empty[] = { "" };

static char	uid[ GC_USERNAME_MAX + 1 ] = { "" };
static char	*puid = uid;

/*
** Keyword, function, type, and object ID's.
*/
#define INVALID_KEYWORD		0	/* Must be 0 (MEreqmem() init) */
#define API_KW_ADD		1
#define API_KW_DEL		2
#define API_KW_GET		3
#define API_KW_NODE		4
#define API_KW_LOGIN		5
#define API_KW_SERVER		6
#define API_KW_PRIVATE		7
#define API_KW_GLOBAL		8
#define API_KW_PARAM		9
#define API_KW_ATTR		10	


/*
** Forward structure references.
*/
typedef struct _API_PARMS	API_PARMS;
typedef struct _API_PARSE	API_PARSE;
typedef struct _KEYWORD_TBL	KEYWORD_TBL;


/*
** Parameter control information.
*/
struct _API_PARMS
{

    i4		opcode;
    i4		object;
    i4		parm_min;
    i4		parm_max;
    char *	(*parms)( API_PARSE *, i4, char * );

};

static	char *	ns_value( API_PARSE *, i4, char * );
static	char *	ns_login( API_PARSE *, i4, char * );

static API_PARMS ns_parms[] =
{
    { API_KW_ADD,	API_KW_NODE,	7,	7,  ns_value },
    { API_KW_ADD,	API_KW_LOGIN,	6,	6,  ns_login },
    { API_KW_ADD,	API_KW_SERVER,	5,	5,  ns_value },
    { API_KW_ADD,	API_KW_ATTR,	6,	6,  ns_value },
    { API_KW_DEL,	API_KW_NODE,	3,	7,  ns_value },
    { API_KW_DEL,	API_KW_LOGIN,	3,	5,  ns_login },
    { API_KW_DEL,	API_KW_ATTR,	3,	6,  ns_value },
    { API_KW_DEL,	API_KW_SERVER,	2,	5,  ns_value },
    { API_KW_ADD,	API_KW_ATTR,	3,	6,  ns_value },
    { API_KW_GET,	API_KW_NODE,	3,	7,  ns_value },
    { API_KW_GET,	API_KW_LOGIN,	3,	5,  ns_login },
    { API_KW_GET,	API_KW_SERVER,	2,	5,  ns_value },
    { API_KW_GET,	API_KW_ATTR,	3,	6,  ns_value }
};


/*
** Parsing information.
*/

#define API_FIELD_FUNC	0
#define API_FIELD_TYPE	1
#define API_FIELD_OBJ 	2
#define API_FIELD_VNODE	3
#define API_FIELD_PARM	4

#define	API_FIELD_MAX	7


struct _API_PARSE
{
    i4		opcode;				/* Semantic info */
    i4		type;
    i4		object;
    API_PARMS	*parms;

    i4		field_count;			/* Number of fields in query */
    i4		parameter[ API_FIELD_MAX ];	/* Field text or parameter # */

#define API_FV_TEXT		-1

    char	fields[ API_FIELD_MAX ][ API_NS_MAX_LEN ];   /* Field values */

};

struct _KEYWORD_TBL
{

    char	*full;		/* Full text of keyword */
    char	*abbr;		/* Minimum abbreviation */
    i4		code;		/* Keyword ID */

};

static KEYWORD_TBL function_tbl[] =
{
    { "Create",		"C",	API_KW_ADD },
    { "Destroy",	"D",	API_KW_DEL },
    { "Show",		"S",	API_KW_GET },
    { NULL,		NULL,	-1 }
};

static KEYWORD_TBL type_tbl[] =
{
    { "Global",		"G",	API_KW_GLOBAL },
    { "Private",	"P",	API_KW_PRIVATE },
    { "Server",		"S",	API_KW_SERVER },
    { NULL,		NULL,	-1 }
};

static KEYWORD_TBL object_tbl[] =
{
    { "Connection",	"C",	API_KW_NODE },
    { "Login",		"L",	API_KW_LOGIN },
    { "Attribute",	"A",	API_KW_ATTR },
    { NULL,		NULL,	-1 }
};

static KEYWORD_TBL *keyword_tbl[] =
{
    function_tbl, 
    type_tbl,
    object_tbl
};


/*
** Descriptors.
*/

static IIAPI_DESCRIPTOR ns_node_desc[] =
{
    { IIAPI_CHA_TYPE, FALSE, 1, 0, 0, IIAPI_COL_TUPLE, "type" },
    { IIAPI_VCH_TYPE, FALSE, 34, 0, 0, IIAPI_COL_TUPLE, "vnode" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "network_address" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "protocol" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "listen_address" },
};

static IIAPI_DESCRIPTOR ns_login_desc[] =
{
    { IIAPI_CHA_TYPE, FALSE, 1, 0, 0, IIAPI_COL_TUPLE, "type" },
    { IIAPI_VCH_TYPE, FALSE, 34, 0, 0, IIAPI_COL_TUPLE, "vnode" },
    { IIAPI_VCH_TYPE, FALSE, 258, 0, 0, IIAPI_COL_TUPLE, "login" },
};

static IIAPI_DESCRIPTOR ns_attr_desc[] =
{
    { IIAPI_CHA_TYPE, FALSE, 1, 0, 0, IIAPI_COL_TUPLE, "type" },
    { IIAPI_VCH_TYPE, FALSE, 34, 0, 0, IIAPI_COL_TUPLE, "vnode" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "attribute_name" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "attribute_value" },
};

static IIAPI_DESCRIPTOR ns_server_desc[] =
{
    { IIAPI_VCH_TYPE, FALSE, 34, 0, 0, IIAPI_COL_TUPLE, "server_type" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "object" },
    { IIAPI_VCH_TYPE, FALSE, 66, 0, 0, IIAPI_COL_TUPLE, "address" },
};


#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof(arr[0]))


/*
** Local function forward references.
*/
static STATUS	ns_scanner( char *, API_PARSE * );
static STATUS	ns_validate( API_PARSE *, i4  * );
static i4	ns_keyword( char *, KEYWORD_TBL * );
static II_BOOL	ns_param_marker( API_PARSE *, i4, i4  * );
static II_VOID	ns_extract(IIAPI_STMTHNDL *, IIAPI_PUTPARMPARM	*, i4, char *);
static char *	ns_resolve_param( API_PARSE *, i4, bool );



/*
** Name: IIapi_createGCNOper
**
** Description:
**	This function creates a GCN_NS_OPER message.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_MSG_BUFF *	NULL if error.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
**	 9-Jan-98 (gordy)
**	    Added GCN_NET_FLAG for requests to Name Server network database.
**	25-Mar-10 (gordy)
**	    Replace formatted GCA interface.
*/

II_EXTERN IIAPI_MSG_BUFF *
IIapi_createGCNOper( IIAPI_STMTHNDL *stmtHndl )
{
    IIAPI_MSG_BUFF	*msgBuff;
    u_i1		*msg;
    char		*str, temp[ 512 ];
    i4			val;
    IIAPI_CONNHNDL	*connHndl = IIapi_getConnHndl((IIAPI_HNDL *)stmtHndl);
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_createGCNOper: creating GCN Oper message\n" );
    
    if ( ! connHndl )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_createGCNOper: can't get connHndl from stmtHnd\n" );
	return( NULL );
    }

    if ( ! (msgBuff = IIapi_allocMsgBuffer( (IIAPI_HNDL *)connHndl )) )
	return( NULL );

    msgBuff->msgType = GCN_NS_OPER;
    msg = msgBuff->data + msgBuff->length;

    /*
    ** Set the operations flags.
    **
    ** Netutil always sets the merge flag when adding
    ** nodes, so we will too.  
    **
    ** The user ID flag is set if a username was given
    ** for IIapi_connect() but no password.  The Name
    ** Server will reject the request if current user
    ** not authorized.
    ** 
    ** The public flag is set for global values.
    **
    ** The network database flag is set for non-server
    ** requests.
    **
    ** We currently don't support the sole server flag.
    ** The merge flag is only marginally supported.
    ** These flags are mostly used for adding servers,
    ** which is currently only supported as an 
    ** undocumented feature.
    */
    val = GCN_DEF_FLAG;				/* gcn_flag */

    if ( parse->opcode == API_KW_ADD  && 
	( parse->object == API_KW_NODE || parse->object == API_KW_ATTR ) )  
	val |= GCN_MRG_FLAG;

    if ( connHndl->ch_username  &&  ! connHndl->ch_password )  
	val |= GCN_UID_FLAG;

    if ( parse->type == API_KW_GLOBAL )  val |= GCN_PUB_FLAG;
    if ( parse->object != API_KW_SERVER )  val |= GCN_NET_FLAG;

    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    switch( parse->opcode )			/* gcn_opcode */
    {
	case API_KW_ADD :  val = GCN_ADD;  break;
	case API_KW_DEL :  val = GCN_DEL;  break;
	case API_KW_GET :  val = GCN_RET;  break;

	default :	/* This should not happen! */
	    IIAPI_TRACE( IIAPI_TR_FATAL )
		( "IIapi_createGCNOper: invalid operations code.\n" );
	    val = GCN_RET;
	    break;
    }

    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    /*
    ** Installation ID is currently ignored.
    */
    val = STlength( install_id ) + 1;		/* gcn_install.gcn_l_item */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( install_id, val, msg );		/* gcn_install.gcn_value */
    msg += val;
    msgBuff->length += val;

    val = 1;					/* gcn_tup_cnt */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    /*
    ** Node and connection operations have fixed types
    ** defined by GCN.  For server operations, the server
    ** class is used as provided by the application.
    */
    switch( parse->object )
    {
	case API_KW_NODE :  str = GCN_NODE;	break;
	case API_KW_LOGIN : str = GCN_LOGIN;	break;
	case API_KW_ATTR :  str = GCN_ATTR;	break;

	case API_KW_SERVER :
	    str = ns_resolve_param( parse, API_FIELD_OBJ, FALSE );
	    break;

	default :	/* Should not happen! */
	    IIAPI_TRACE( IIAPI_TR_TRACE )
		( "IIapi_createGCNOper: invalid object.\n" );
	    str = empty;
	    break;
    }

    /*
    ** Build the GCN tuple.
    */
    val = STlength( str ) + 1;			/* gcn_type.gcn_l_item */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( str, val, msg );			/* gcn_type.gcn_value */
    msg += val;
    msgBuff->length += val;

    /*
    ** Use username if specified.  This will
    ** either be the username we are connected
    ** with (if password also provided) or it
    ** will be the username for the GCN_UID_FLAG
    ** which requires special privileges for the
    ** current user.  Otherwise, we use the user
    ** ID of the current process.
    */
    if ( connHndl->ch_username )
	str = connHndl->ch_username;
    else
    {
	if ( ! uid[0] )
	{
	    IDname( &puid );
	    STzapblank( uid, uid );
	}

	str = uid;
    }

    val = STlength( str ) + 1;			/* gcn_uid.gcn_l_item */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( str, val, msg );			/* gcn_uid.gcn_value */
    msg += val;
    msgBuff->length += val;

    str = ns_resolve_param( parse, API_FIELD_VNODE, 
			    (parse->opcode != API_KW_ADD) );

    val = STlength( str ) + 1;			/* gcn_obj.gcn_l_item */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( str, val, msg );			/* gcn_obj.gcn_value */
    msg += val;
    msgBuff->length += val;

    /*
    ** The tuple value must be built up from various parameters 
    ** and can be function and object dependent.  Call appropriate
    ** function for the current request to process the parameters.
    **
    ** Pass in our temp buffer.  It will be used if large enough
    ** or a different buffer will be allocated and returned. Check
    ** for memory allocation failures and free the returned buffer
    ** if it is not the one passed in (after copying the value).
    */
    if ( ! (str = (*parse->parms->parms)( parse, sizeof( temp ), temp )) )
    {
	IIapi_freeMsgBuffer( msgBuff );
	return( NULL );
    }

    val = STlength( str ) + 1;			/* gcn_val.gcn_l_item */
    I4ASSIGN_MACRO( val, *msg );
    msg += sizeof( i4 );
    msgBuff->length += sizeof( i4 );

    MEcopy( str, val, msg );			/* gcn_val.gcn_value */
    msgBuff->length += val;
    if ( str != temp )  MEfree( (PTR)str );

    msgBuff->flags = IIAPI_MSG_EOD;
    return( msgBuff );
}


/*
** Name: IIapi_parseNSQuery
**
** Description:
**	Parses a Name Server query in NETUTIL syntax (with extensions).
**	Validates keywords and parameters and replaces the query text
**	with a parse info structure.
**
** Input:
**	stmtHndl	Statement handle.
**
** Output:
**	func_status	Error code if IIAPI_ST_FAILURE returned.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS, IIAPI_ST_OUT_OF_MEMORY
**			IIAPI_ST_FAILURE: error code in func_status.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_parseNSQuery( IIAPI_STMTHNDL *stmtHndl, STATUS *func_status )
{
    API_PARSE	*parse;
    i4		keyword[ API_FIELD_MAX ];
    i4		fld, marker_index;
    STATUS	status;

    *func_status = OK;

    /*
    ** Allocate the parse info structure.
    */
    parse = (API_PARSE *)MEreqmem( 0, sizeof( API_PARSE ), TRUE, &status );

    if ( ! parse )  
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_parseNSQuery: couldn't allocate parser info.\n" );

	return( IIAPI_ST_OUT_OF_MEMORY );
    }

    /*
    ** Tokenize the query text for individual
    ** field validation and processing.
    */
    if ( (*func_status = ns_scanner( stmtHndl->sh_queryText, parse )) != OK )
    {
	MEfree( (PTR)parse );
	return( IIAPI_ST_FAILURE );
    }

    /*
    ** Check for keywords and parameter markers in fields.
    */
    for( fld = marker_index = 0; fld < parse->field_count; fld++ )
	if ( fld >= ARRAY_SIZE( keyword_tbl ) )
	    ns_param_marker( parse, fld, &marker_index );
	else
	{
	    keyword[ fld ] = ns_keyword(parse->fields[fld], keyword_tbl[fld]);

	    if ( keyword[ fld ] == INVALID_KEYWORD  &&
		 ns_param_marker( parse, fld, &marker_index ) )
		keyword[ fld ] = API_KW_PARAM;
	}

    if ( (*func_status = ns_validate( parse, keyword )) != OK )
    {
	MEfree( (PTR)parse );
	return( IIAPI_ST_FAILURE );
    }

    /*
    ** Finally, we replace the original query
    ** text with our parsed information.
    */
    MEfree( stmtHndl->sh_queryText );
    stmtHndl->sh_queryText = (char *)parse;

    return( IIAPI_ST_SUCCESS );
}


/*
** Name: IIapi_validNSDescriptor
**
** Description:
**	Validates query parameter usage by making sure each
**	parsed query field represented by a parameter marker
**	is provided in the descriptor with the correct values.
**
** Input:
**	stmtHndl	Statement handle.
**	descCount	Number of entries in descriptor.
**	descriptor	API descriptor.
**
** Output:
**	None.
**
** Returns:
**	II_ULONG	Error code, OK if no error.
**
** History:
**	12-Mar-97 (gordy)
**	    Created.
**	22-Oct-04 (gordy)
**	    Return specific error code.
*/

II_EXTERN II_ULONG
IIapi_validNSDescriptor( IIAPI_STMTHNDL *stmtHndl,
			 II_LONG descCount, IIAPI_DESCRIPTOR *descriptor )
{
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    IIAPI_DESCRIPTOR	*desc;
    i4			fld;

    /*
    ** Must have at least 1 descriptor.
    */
    if ( descCount < 1  ||  ! descriptor )  
	return( E_AP0010_INVALID_COLUMN_COUNT);

    /*
    ** Validate the fields provided through parameters.
    */
    for( fld = 0; fld < parse->field_count; fld++ )
	if ( parse->parameter[ fld ] != API_FV_TEXT )
	{
	    /*
	    ** Make sure field contained in descriptor.
	    */
	    if ( parse->parameter[ fld ] >= descCount )  
		return( E_AP0010_INVALID_COLUMN_COUNT );

	    desc = &descriptor[ parse->parameter[ fld ] ];

	    /*
	    ** Validate descriptor info.
	    */
	    if ( desc->ds_columnType != IIAPI_COL_QPARM )
		return( E_AP0012_INVALID_DESCR_INFO );
	    else  if ( desc->ds_dataType != IIAPI_CHA_TYPE &&
		       desc->ds_dataType != IIAPI_VCH_TYPE )
		return( E_AP0025_SVC_DATA_TYPE );
	    else  if ( desc->ds_length >= API_NS_MAX_LEN )
		return( E_AP0024_INVALID_DATA_SIZE );
	}

    return( OK );
}


/* Name: IIapi_validNSParams
**
** Description:
**	Validate parameter values.  The only field which may
**	be passed in a parameter and requires validation is
**	the type field (global/private).
**
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameters.
**
** Output:
**	None.
**
** Returns:
**	II_BOOL		TRUE if parameters are valid.
**
** History:
**	12-Mar-97 (gordy)
**	    Created.
*/

II_EXTERN II_BOOL
IIapi_validNSParams( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    API_PARSE *parse = (API_PARSE *)stmtHndl->sh_queryText;

    /*
    ** Nothing to do if type is not provided through
    ** a parameter, or the current parameter set does
    ** not contain referenced parameter.
    */
    if ( 
	 parse->type != API_KW_PARAM  ||
         ( parse->parameter[ API_FIELD_TYPE ] < stmtHndl->sh_parmIndex  &&
	   parse->parameter[ API_FIELD_TYPE ] >= (stmtHndl->sh_parmIndex + 
						  stmtHndl->sh_parmSend) )
       )
	return( TRUE );

    /*
    ** We are going to have to do most of the
    ** work to save the parameter value, so we
    ** might as well do all the work here.
    ** Parameter references for the field are
    ** overwritten so this won't be done again.
    */
    ns_extract( stmtHndl, putParmParm, parse->parameter[ API_FIELD_TYPE ], 
		parse->fields[ API_FIELD_TYPE ] );

    parse->parameter[ API_FIELD_TYPE ] = API_FV_TEXT;

    /*
    ** We require the private or global keyword.
    **
    */
    if ( ! parse->fields[ API_FIELD_TYPE ][ 0 ] )
	parse->type = INVALID_KEYWORD;
    else
	parse->type = ns_keyword( parse->fields[ API_FIELD_TYPE ],
				  keyword_tbl[ API_FIELD_TYPE ] );

    return( parse->type == API_KW_GLOBAL  ||  parse->type == API_KW_PRIVATE );
}


/*
** Name: IIapi_saveNSParams
**
** Description:
**	Extract parameter values and save in parser info fields.
**
** Input:
**	stmtHndl	Statement handle.
**	putParmParm	IIapi_putParms() parameters.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS
**
** History:
**	12-Mar-97 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_saveNSParams( IIAPI_STMTHNDL *stmtHndl, IIAPI_PUTPARMPARM *putParmParm )
{
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    char		*str;
    i4			fld, len;

    /*
    ** We process all the parameters in the current
    ** set indirectly through the parsed query fields.
    */
    for( fld = 0; fld < parse->field_count; fld++ )
	if ( 
	     parse->parameter[ fld ] != API_FV_TEXT  &&
	     parse->parameter[ fld ] >= stmtHndl->sh_parmIndex  &&
	     parse->parameter[ fld ] < (stmtHndl->sh_parmIndex + 
					stmtHndl->sh_parmSend) 
	   )
	{
	    /*
	    ** Field is in the current parameter set: 
	    ** extract and save the parameter value.  
	    */
	    ns_extract( stmtHndl, putParmParm, 
			parse->parameter[ fld ], parse->fields[ fld ] );
	    parse->parameter[ fld ] = API_FV_TEXT;
	}

    /*
    ** Done with the current parameter set.
    */
    stmtHndl->sh_parmIndex += stmtHndl->sh_parmSend;
    stmtHndl->sh_parmSend = 0;

    return;
}


/*
** Name: IIapi_getNSDescriptor
**
** Description:
**	Build the tuple descriptor for the type of object
**	being accessed.
**
** Input:
**	stmtHndl	Statement handle
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS, IIAPI_ST_OUT_OF_MEMORY.
**
** History:
**	 5-Mar-97 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_getNSDescriptor( IIAPI_STMTHNDL *stmtHndl )
{
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    IIAPI_DESCRIPTOR	*desc1, *desc2;
    STATUS		status;
    i4			count;

    switch( parse->object )
    {
	case API_KW_NODE :
	    desc1 = ns_node_desc;
	    count = ARRAY_SIZE( ns_node_desc );
	    break;

	case API_KW_LOGIN :
	    desc1 = ns_login_desc;
	    count = ARRAY_SIZE( ns_login_desc );
	    break;

	case API_KW_ATTR :
	    desc1 = ns_attr_desc;
	    count = ARRAY_SIZE( ns_attr_desc );
	    break;

	case API_KW_SERVER :
	    desc1 = ns_server_desc;
	    count = ARRAY_SIZE( ns_server_desc );
	    break;

	default :
	    /*
	    ** Should not happen!
	    */
	    IIAPI_TRACE( IIAPI_TR_TRACE )
		( "IIapi_getNSDescriptor: invalid object.\n" );
	    return( IIAPI_ST_FAILURE );
	    break;
    }

    stmtHndl->sh_colDescriptor = (IIAPI_DESCRIPTOR *)
	MEreqmem( 0, sizeof(IIAPI_DESCRIPTOR) * count, FALSE, &status );

    if ( ! stmtHndl->sh_colDescriptor )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_getNSDescriptor: error allocating descriptor.\n" );

	return( IIAPI_ST_OUT_OF_MEMORY );
    }

    for( 
	 stmtHndl->sh_colCount = 0, desc2 = stmtHndl->sh_colDescriptor; 
	 stmtHndl->sh_colCount < count; 
	 stmtHndl->sh_colCount++, desc1++, desc2++
       )
    {
	STRUCT_ASSIGN_MACRO( *desc1, *desc2 );
	desc2->ds_columnName = STalloc( desc1->ds_columnName );

	if ( ! desc2->ds_columnName )
	{
	    IIAPI_TRACE( IIAPI_TR_ERROR )
		( "IIapi_getNSDescriptor: error allocating column name.\n" );

	    /*
	    ** Since all the entries are valid
	    ** upto sh_colCount, we'll leave the
	    ** descriptor to be freed when the
	    ** statement handle is freed.
	    */
	    return( IIAPI_ST_OUT_OF_MEMORY );
	}
    }

    return( IIAPI_ST_SUCCESS );
}


/* Name: IIapi_loadNSColumns
**
** Description:
**	Extracts the fields from a single GCN tuple for the
**	columns in the current request.  This function does
**	not handle spanning tuples, the column fetch count
**	must not cause the current column index to exceed
**	the descriptor count.
**
**	Since adaquate buffer space is reserved (famous last
**	words), excessive string lengths are not expected
**	and will be handled by simple truncation if required.
**
** Input:
**	stmtHndl	Statement handle.
**	getColParm	IIapi_getColumns() parameters.
**	tuple		GCN tuple data.
**
** Output:
**	None.
**
** Returns:
**	II_VOID
**
** History:
**	 6-Mar-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
*/

II_EXTERN II_VOID
IIapi_loadNSColumns
( 
    IIAPI_STMTHNDL	*stmtHndl, 
    IIAPI_GETCOLPARM	*getColParm,
    IIAPI_GCN_REQ_TUPLE	*tuple 
)
{
    IIAPI_DESCRIPTOR	*descr;
    IIAPI_DATAVALUE	*value;
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    char		buffer[ API_NS_MAX_LEN ];
    char		*str, *val[ 3 ];
    u_i2		len;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_loadNSColumns: %d columns starting at %d, %d total columns\n",
	  (II_LONG)stmtHndl->sh_colFetch,
	  (II_LONG)stmtHndl->sh_colIndex,
	  (II_LONG)stmtHndl->sh_colCount );

    /*
    ** Some columns are combined in the returned
    ** GCN value field.  Break out the combinded
    ** columns for reference below.  We don't
    ** expect to overflow the buffer, but just
    ** in case we truncate the input.
    */
    if ( tuple->gcn_val.gcn_l_item > (sizeof( buffer ) - 1) )
	tuple->gcn_val.gcn_value[ sizeof( buffer ) - 1 ] = EOS;
    gcu_words( tuple->gcn_val.gcn_value, buffer, val, ',', 3 );

    descr = &stmtHndl->sh_colDescriptor[ stmtHndl->sh_colIndex ];
    value = &getColParm->gc_columnData[ (getColParm->gc_rowsReturned *
					 getColParm->gc_columnCount) +
					 getColParm->gc_columnCount -
					 stmtHndl->sh_colFetch ];

    for( ; stmtHndl->sh_colFetch;
	 stmtHndl->sh_colFetch--, stmtHndl->sh_colIndex++, descr++, value++ )
    {
	/*
	** Figure out which piece of of the tuple
	** satisfies the current column depending
	** on which object type is being accessed.
	*/
	switch( parse->object )
	{
	    case API_KW_NODE :
		switch( stmtHndl->sh_colIndex )
		{
		    case 0 :		/* Type */
			str = (parse->type == API_KW_GLOBAL) ? glob_type 
							     : priv_type;	
			break;

		    case 1 :		/* Vnode */
			str = tuple->gcn_obj.gcn_value;	
			break;

		    case 2 :		/* Network address */
			str = val[0];
			break;

		    case 3 :		/* Protocol */
			str = val[1];
			break;

		    case 4 :		/* Listen address */
			str = val[2];
			break;
		}
		break;

	    case API_KW_LOGIN :
		switch( stmtHndl->sh_colIndex )
		{
		    case 0 :		/* Type */
			str = (parse->type == API_KW_GLOBAL) ? glob_type 
							     : priv_type;	
			break;

		    case 1 :		/* Vnode */
			str = tuple->gcn_obj.gcn_value;	
			break;

		    case 2 :		/* Login */
			str = val[0];
			break;
		}
		break;

	    case API_KW_ATTR :
		switch( stmtHndl->sh_colIndex )
		{
		    case 0 :		/* Type */
			str = (parse->type == API_KW_GLOBAL) ? glob_type 
							     : priv_type;	
			break;

		    case 1 :		/* Vnode */
			str = tuple->gcn_obj.gcn_value;	
			break;

		    case 2 :		/* Attribute Name */
			str = val[0];
			break;

		    case 3 :		/* Attribute Value */
			str = val[1];
			break;
		}
		break;

	    case API_KW_SERVER :
		switch( stmtHndl->sh_colIndex )
		{
		    case 0 :		/* Server type */
			str = tuple->gcn_type.gcn_value;	
			break;

		    case 1 :		/* Object */
			str = tuple->gcn_obj.gcn_value;	
			break;

		    case 2 :		/* Address */
			str = val[0];
			break;
		}
		break;
	}

	/*
	** We should always get a valid string,
	** but provide a default just in case.
	** Strings are silently truncated if too
	** long since we don't expect truncation
	** to occur (except for the type column,
	** in which case silent truncation is
	** desired).
	*/
	if ( ! str )  str = err_str;
	len = STlength( str );
	value->dv_null = FALSE;

	switch( descr->ds_dataType )
	{
	    case IIAPI_CHA_TYPE :
		len = min( len, descr->ds_length );
		value->dv_length = len;
		MEcopy( (PTR)str, len, value->dv_value );
		break;
	    
	    case IIAPI_VCH_TYPE :
		len = min( len, descr->ds_length - sizeof( len ) );
		value->dv_length = sizeof( len ) + len;
		MEcopy( (PTR)&len, sizeof( len ), value->dv_value );
		MEcopy( (PTR)str, len, 
			(PTR)((char *)value->dv_value + sizeof( len )) );
		break;

	    default :
		/*
		** Should not happen.
		*/
		value->dv_null = TRUE;
		value->dv_length = 0;
		break;
	}
    }

    return;
}


/*
** Name: ns_scanner
**
** Description:
**	Scan and tokenize NS query text.
**
** Input:
**	queryText	Original NS query text.
**
** Output:
**	parse		Parsing information.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
*/

static STATUS
ns_scanner( char *queryText, API_PARSE *parse )
{
    char	*in, *token;
    STATUS	status = OK;

    /*
    ** Each token in the query text, excluding
    ** comments, is extracted and saved.  One
    ** token is processed each time through loop.
    */
    for( 
	 in = queryText, parse->field_count = 0; 
         ; 
	 parse->field_count++ 
       )
    {
	/*
	** Skip white space preceding token.
	*/
	for( ; CMwhite( in )  &&  *in != EOS; CMnext( in ) );

	/*
	** Have we found the end of the query,
	** or the start of a comment?  If so,
	** we be done.  Also, check for field
	** overflow at this point.
	*/
	if ( *in == EOS  ||  *in == '#' )  break;

	if ( parse->field_count >= API_FIELD_MAX ) 
	{
	    status = E_AP0010_INVALID_COLUMN_COUNT;
	    break;
	}

	/*
	** Find end of token.
	*/
	for( 
	     token = in; 
	     ! CMwhite( in )  &&  *in != EOS  &&  *in != '#';
	     CMnext( in )
	   );

	/*
	** Save token in parser info.  All tokens marked
	** as text for now, parameter markers will be
	** identified during field validation.
	*/
	if ( (in - token) >= API_NS_MAX_LEN )
	{
	    status = E_AP0011_INVALID_PARAM_VALUE;
	    break;
	}

	MEcopy( (PTR)token, (in - token), 
		(PTR)parse->fields[ parse->field_count ] );
	parse->fields[ parse->field_count ][ in - token ] = EOS;
	parse->parameter[ parse->field_count ] = API_FV_TEXT;
    }

    return( status );
}


/*
** Name: ns_validate
**
** Description:
**	Validate syntax of parsed query.  Validate keyword and
**	parameter marker usage.  Validate number of parameters.
**	Build parser semantic information.
**
** Input:
**	parse		Parser information.
**	keyword		Array of keyword ID's for each field.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
*/

static STATUS
ns_validate( API_PARSE *parse, i4  *keyword )
{
    i4  i;

    /*
    ** Validate keywords.  This is a little bit more
    ** complicated than Netutil since we support the
    ** SERVER object and parameter markers for type.
    ** Keywords (except for type) cannot be provided
    ** through parameters.
    **
    ** Field syntax and semantics:
    **
    ** <function> <type> <object> [<value>...]
    **
    ** Function, type and object must be provided.
    ** Function cannot be provided in a parameter.  
    ** Type can be provided in a parameter.  Values 
    ** may be parameters and do not contain keywords.
    **
    ** The SERVER keyword may replace type (but not in 
    ** a parameter), and object (server class) may then 
    ** be provided in a parameter.  Otherwise, object 
    ** cannot be provided in a parameter.
    */
    if ( parse->field_count < 3 )  return( E_AP0010_INVALID_COLUMN_COUNT );

    if ( keyword[ API_FIELD_FUNC ] == INVALID_KEYWORD  ||
	 keyword[ API_FIELD_FUNC ] == API_KW_PARAM )
	return( E_AP0011_INVALID_PARAM_VALUE );
    else
	parse->opcode = keyword[ API_FIELD_FUNC ];

    if ( keyword[ API_FIELD_TYPE ] == INVALID_KEYWORD )
	return( E_AP0011_INVALID_PARAM_VALUE );

    if ( keyword[ API_FIELD_TYPE ] == API_KW_SERVER )
	if ( keyword[ API_FIELD_OBJ ] != INVALID_KEYWORD  &&
	     keyword[ API_FIELD_OBJ ] != API_KW_PARAM )
	    return( E_AP0011_INVALID_PARAM_VALUE );
	else
	{
	    parse->type = API_KW_PRIVATE;	/* Default for servers */
	    parse->object = API_KW_SERVER;
	}
    else  if ( keyword[ API_FIELD_OBJ ] == INVALID_KEYWORD  ||
	       keyword[ API_FIELD_OBJ ] == API_KW_PARAM )
	return( E_AP0011_INVALID_PARAM_VALUE );
    else
    {
	parse->type = keyword[ API_FIELD_TYPE ];
	parse->object = keyword[ API_FIELD_OBJ ];
    }

    /*
    ** Make sure enough fields have been provided
    ** (or will be provided through query parameters).
    ** Parameter count dependent on the function and
    ** object.  Optional parameters supported.
    */
    for( i = 0; i < ARRAY_SIZE( ns_parms ); i++ )
	if ( ns_parms[ i ].opcode == parse->opcode  &&
	     ns_parms[ i ].object == parse->object )
	{
	    parse->parms = &ns_parms[ i ];

	    if ( parse->field_count < ns_parms[ i ].parm_min  ||
		 parse->field_count > ns_parms[ i ].parm_max )
	    {
		return( E_AP0010_INVALID_COLUMN_COUNT );
	    }

	    break;
	}

    if ( i >= ARRAY_SIZE( ns_parms ) )  
    {
	/*
	** This shouldn't happen.
	*/
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "ns_validate: couldn't find parameter info for request\n" );

        return( E_AP0011_INVALID_PARAM_VALUE );
    }

    return( OK );
}


/*
** Name: ns_keyword
**
** Description:
**	Validates a token against a keyword table of acceptable
**	values and returns the keyword ID.  Keywords may be
**	abbreviated.
**
** Input:
**	token		Keyword to validate.
**	tbl		Keyword table.
**
** Output:
**	None.
**
** Returns:
**	i4		Keyword ID or INVALID_KEYWORD.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
*/

static i4
ns_keyword( char *token, KEYWORD_TBL *tbl )
{
    i4  keyword = INVALID_KEYWORD;

    for( ; tbl->full != NULL; tbl++ )
	if ( ! STbcompare( token, 0, tbl->abbr, STlength(tbl->abbr), TRUE )  &&
	     STlength( token ) <= STlength( tbl->full )  &&
	     ! STbcompare( token, STlength( token ), tbl->full, 0, TRUE ) )
	{
	    keyword = tbl->code;
	    break;
	}

    return( keyword );
}


/*
** Name: ns_param_marker
**
** Description:
**	Tests a parsed query field to see if it is a parameter
**	marker.  If so, the field is converted from a text field
**	to a parameter field.
**
**	A default parameter index is maintained by this routine,
**	but must be initialized by the caller to 0 prior to the
**	first call.
**
** Input:
**	parse		Parser info.
**	fld		Field to be tested.
**	index		Current parameter marker index, initially 0.
**
** Output:
**	index		Next parameter marker index.
**
** Returns:
**	II_BOOL		TRUE if field is a parameter marker.
**
** History:
**	10-Mar-97 (gordy)
**	    Created.
*/

static II_BOOL
ns_param_marker( API_PARSE *parse, i4  fld, i4  *index )
{
    II_BOOL	marker = FALSE;
    i4	value;

    if ( parse->fields[ fld ][0] == '~'  &&  parse->fields[ fld ][1] == 'V' )
    {
	/*
	** Was an explicit parameter index
	** included with the marker?
	*/
	if ( parse->fields[ fld ][2]  &&
	     CVal( parse->fields[ fld ][2], &value ) == OK )
	    *index = value;

	/*
	** Convert field from text to parameter.
	*/
	parse->parameter[ fld ] = *index;	/* Parameter index */
	parse->fields[ fld ][0] = EOS;		/* Parameter value */
	(*index)++;				/* Next default index */
	marker = TRUE;
    }

    return( marker );
}


/*
** Name: ns_value
**
** Description:
**	Generic routine for combining parameters into a single
**	GCN value based on the maximum number of parameters
**	allowed for the current request.  The passed in buffer 
**	is used if large enough.  Otherwise, a larger buffer 
**	is allocated and returned.  If the returned buffer is 
**	not the passed in buffer, it must be released using 
**	MEfree().
**
** Input:
**	parse		Parser info.
**	buflen		Length of buffer.
**
** Output:
**	buffer		Output buffer.
**
** Returns:
**	char *		Output buffer or allocated buffer.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Enhanced parameter memory block handling.
*/

static char *
ns_value( API_PARSE *parse, i4 buflen, char *buffer )
{
    char	*parms[ API_FIELD_MAX ];
    char	*value;
    i4		len, fld, fld_cnt;
    STATUS	status;

    /*
    ** Get each (possibly optional) parameter
    ** and determine the size of the final
    ** combined value.
    */
    fld_cnt = max( 0, parse->parms->parm_max - API_FIELD_PARM );

    for( fld = len = 0; fld < fld_cnt; fld++ )
    {
	parms[ fld ] = ns_resolve_param( parse, fld + API_FIELD_PARM, 
					 (parse->opcode != API_KW_ADD) );
	len += STlength( parms[ fld ] );
    }

    len += fld_cnt;		/* Allow room for EOS and separating ',' */
    if ( ! len )  len++;

    /*
    ** Use passed in buffer if large enough.
    ** Otherwise, allocate temp buffer.
    */
    value = (len <= buflen) ? buffer
			    : (char *)MEreqmem( 0, len, FALSE, &status );
    if ( ! value )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "ns_value: can't allocate value buffer\n" );
	return( NULL );
    }

    /*
    ** Build the comma separated list of parameters.
    */
    if ( fld_cnt )
	STcopy( parms[ 0 ], value );
    else
	*value = EOS;

    for( fld = 1; fld < fld_cnt; fld++ )
    {
	STcat( value, "," );
	STcat( value, parms[ fld ] );
    }

    return( value );
}


/*
** Name: ns_login
**
** Description:
**	Combines user ID and password (as needed) into GCN value
**	format required for LOGIN operation requests.  The passed
**	in buffer is used if large enough.  Otherwise, a larger
**	buffer is allocated and returned.  If the returned buffer
**	is not the passed in buffer, release it using MEfree().
**
** Input:
**	parse		Parser info.
**	buflen		Length of buffer.
**
** Output:
**	buffer		Output buffer.
**
** Returns:
**	char *		Output buffer or allocated buffer.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
**	25-Mar-10 (gordy)
**	    Enhanced parameter memory block handling.
*/

static char *
ns_login( API_PARSE *parse, i4 buflen, char *buffer )
{
    char	*user, *pwd, *value;
    i4		usrlen, pwdlen, len;
    STATUS	status;

    if ( parse->opcode == API_KW_ADD )
    {
	/*
	** Password required and must be sent encrypted.
	*/
	user = ns_resolve_param( parse, API_FIELD_PARM, FALSE );
	usrlen = STlength( user );
	pwd  = ns_resolve_param( parse, API_FIELD_PARM + 1, FALSE );
	pwdlen = (STlength( pwd ) + 8) * 2;	/* estimate encrypted length */
    }
    else
    {
	/*
	** Use GCN wild card format for password.
	*/
	user = ns_resolve_param( parse, API_FIELD_PARM, TRUE );
	usrlen = STlength( user );
	pwd = empty;
	pwdlen = 0;
    }

    /* Allow room for EOS and separating ',' */
    len = usrlen + pwdlen + 2;

    /*
    ** Allocate storage for the final value
    ** and build it from the parameters
    ** retrieved above.
    */
    value = (len <= buflen) ? buffer
    			    : (char *)MEreqmem( 0, len, FALSE, &status );
    if ( ! value )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "ns_login: can't allocate value buffer\n" );
	return( NULL );
    }

    if ( parse->opcode != API_KW_ADD )
	STpolycat( 3, user, ",", pwd, value );
    else
    {
	/*
	** Encrypt password directly into formatted output.
	*/
	STpolycat( 2, user, ",", value );
	gcu_encode( user, pwd, &value[ usrlen + 1 ] );
    }

    return( value );
}


/*
** Name: ns_extract
**
** Description:
**	Extracts a string parameter value.  The output buffer
**	must be large enough to hold API_NS_MAX_LEN bytes.
**	Longer strings are truncated.  NULL parameters are
**	returned as zero length strings.
**
** Input:
**	stmtHndl	Statement Handle.
**	putParmParm	IIapi_putParms() parameters.
**	param		Which parameter to extract.
**
** Output:
**	buff		Buffer for extracted parameter value.
**
** Returns:
**	II_VOID
**
** History:
**	12-Mar-97 (gordy)
**	    Created.
**	18-Mar-97 (gordy)
**	    Check input length of varchar before access value.
*/

static II_VOID
ns_extract
( 
    IIAPI_STMTHNDL	*stmtHndl, 
    IIAPI_PUTPARMPARM	*putParmParm, 
    i4			param,
    char		*buff
)
{
    API_PARSE		*parse = (API_PARSE *)stmtHndl->sh_queryText;
    IIAPI_DESCRIPTOR	*descr;
    IIAPI_DATAVALUE	*value;
    II_UINT2		len;
    char		*str = NULL;

    descr = &stmtHndl->sh_parmDescriptor[ param ];
    value = &putParmParm->pp_parmData[ param - stmtHndl->sh_parmIndex ];

    if ( descr->ds_nullable  &&  value->dv_null )
	len = 0;
    else
	switch( descr->ds_dataType )
	{
	    case IIAPI_CHA_TYPE :
		len = value->dv_length;
		str = (char *)value->dv_value;
		break;
		    
	    case IIAPI_VCH_TYPE :
		if ( value->dv_length < sizeof( len ) )
		    len = 0;
		else
		{
		    MEcopy( value->dv_value, sizeof( len ), (PTR)&len );
		    str = (char *)value->dv_value + sizeof( len );
		}
		break;

	    default :
		/*
		** Should not happen.
		*/
		len = 0;
		break;
	}
    
    /*
    ** Lengths have been validated so just truncate
    ** if the unexpected happens.
    */
    len = min( len, API_NS_MAX_LEN - 1 );
    if ( len )  MEcopy( (PTR)str, len, (PTR)buff );
    buff[ len ] = EOS;

    return;
}


/*
** Name: ns_resolve_param
**
** Description:
**	Returns the value for a parameter field whether the
**	value was provided in the query text or as a query
**	parameter.  Optional fields which were not specified
**	are converted to the GCN wild card value.  Fields
**	specified as wild cards may optionally be converted
**	to GCN wild card format also.
**
** Input:
**	parse		Parser info.
**	fld		Field to be resolved.
**	wild		TRUE if wild cards should be transformed.
**
** Output:
**	None.
**
** Returns:
**	char *		Parameter value.
**
** History:
**	19-Feb-97 (gordy)
**	    Created.
*/

static char *
ns_resolve_param( API_PARSE *parse, i4  fld, bool wild )
{
    char *str = empty;

    if ( fld < parse->field_count  &&  parse->fields[ fld ][ 0 ] )
	str = parse->fields[ fld ];

    if ( wild  &&  ! STcompare( str, wild_card ) )  str = empty;

    return( str );
}


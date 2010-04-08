/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<dbms.h>
# include	<iiapi.h>
# include	"aitfunc.h"
# include	"aitdef.h"
# include	"aitsym.h"
# include	"aitmisc.h"
# include	"aitbind.h"
# include	"aitproc.h"
# include	"aitparse.h"

/*
** Name:	aitbind.c - buffer binding functions
**
** Description:
**	Thsi file contains functions that do semantics check on field 
**	assignment, allocat buffers, parse the values into the 
**	corresponding buffer.	
**
**	var_assgn_auto		IIAPI_AUTOPARM variable binding function
**	var_assgn_cancel	IIAPI_CANCELPARM variable binding function
**	var_assgn_catchEvent	IIAPI_CATCHEVENTPARM variable binding function
**	var_assgn_close		IIAPI_CLOSEPARM variable binding function
**	var_assgn_commit	IIAPI_COMMITPARM variable binding function
**	var_assgn_connect	IIAPI_CONNPARM variable binding function
**	var_assgn_disconnect	IIAPI_DISCONNPARM variable binding function
**	var_assgn_getColumns	IIAPI_GETCOLPARM variable binding function
**	var_assgn_getCopyMap	IIAPI_GETCOPYMAPPARM variable binding function
**	var_assgn_getDescriptor	IIAPI_GETDESCRPARM variable binding function
**	var_assgn_getErrorInfo	IIAPI_GETEINFOPARM variable binding function
**	var_assgn_getQueryInfo	IIAPI_GETQINFOPARM variable binding function
**	var_assgn_getEvent	IIAPI_GETEVENTPARM variable binding function
**	var_assgn_initialize	IIAPI_INITPARM variable binding function
**	var_assgn_modifyConnect	IIAPI_MODCONNPARM variable binding function
**	var_assgn_prepareCommit	IIAPI_PREPCMTPARM variable binding function
**	var_assgn_putColumns	IIAPI_PUTCOLPARM variable binding function
**	var_assgn_putParms	IIAPI_PUTPARMPARM variable binding function
**	var_assgn_query		IIAPI_QUERYPARM variable binding function
**	var_assgn_registerXID	IIAPI_REGXIDPARM variable binding function
**	var_assgn_releaseEnv	IIAPI_RELENVPARM variable binding function
**	var_assgn_releaseXID	IIAPI_RELXIDPARM variable binding function
**	var_assgn_rollback	IIAPI_ROLLBACKPARM variable binding function
**	var_assgn_savePoint	IIAPI_SAVEPTPARM variable binding function
**	var_assgn_setConnectParm
**				IIAPI_SETCONNPRMPARM variable binding function
**	var_assgn_setEnvParm
**				IIAPI_SETENVPRMPARM variable binding function
**	var_assgn_setDescriptor	IIAPI_SETDESCRPARM variable binding function
**	var_assgn_terminate	IIAPI_TERMPARM variable binding function
**	var_assgn_wait		IIAPI_WAITPARM variable binding function
**	var_assgn_abort		IIAPI_ABORTPARM variable binding function
**	var_assgn_frmtData	IIAPI_FORMATPARM variable binding function
**	hndl_assgn		II_PTR value assignibg function
**	symbolLong_assgn	long value searching function
**	symbolStr_assgn		string searching function
**	str_assggn		II_CHAR * value assigning function
**	bool_assgn		II_BOOL value assigning function
**	long_assgn		II_LONG value assigning function
**	int_assgn		II_INT value assigning function
**	float_assgn		f8 value assigning function
**	descriptor_assgn	IIAPI_DESCRIPTOR field parsing function
**	datavalue_assgn		IIAPI_DATAVALUE field parsing function
**	bind_longvar		long variable referencing function
**	find_fun_num		function number (index) searching function
**	find_field_num		variable field number (index) searching function
**	funcptr_assgn		Callback function assign
**	longval_assgn		long value searching function
**	ptrval_assgn		II_PTR value searching function
**	strval_assgn		II_CHAR * value searching function
**	boolval_assgn		II_BOOL value searching function
**	datavalueval_assgn	IIAPI_DATAVALUE value searching function
**	descrval_assgn		IIAPI_DESCRIPTOR value searching function
**	print_err		error printing function
** 	formatData_srcVal	get IIAPI_DATAVALUE for IIapi_formatData().
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	27-Mar-95 (gordy)
**	    Removed special parameter block for IIapi_getColumns()
**	    and move info to curCallPtr.  Added typedef.
**	10-May-95 (feige)
**	    Updated TINYINT, SMALLINT, DECIMAL and MONEY assignment
**	    in dataval_assgn.
**	    Updated sc_paramValue assignment in var_assgn_setConnectParam.
**	    Updated symbolStr_assgn().
**	19-May-95 (gordy)
**	    Fixed include statements.
**	14-Jun-95 (gordy)
**	    Finalized Two Phase Commit interface.
**	13-Jul-95 (gordy)
**	    Add casts for type correctness.
**	27-Jul-95 (feige)
**	    Support syntax ds_length = sizeof( $shell_variable );
**      12-Feb-96 (fanra01)
**          Modifed the str_assgn function in the case where a '"' is found to
**          use MEcopy instead of STcopy.
**          The copy of certain strings seems to corrupt the heap when the null
**          is appended in windows NT.
**	26-Feb-96 (feige)
**	    Updated COPYMAP interface.
**	22-Apr-96 (gordy)
**	    Allocate memory for the pc_columnData data value array.
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	10-Feb-97 (gordy)
**	    Added IIapi_releaseEnv(), in_envHandle to IIapi_initialize(),
**	    and co_type to IIapi_connect().
**	19-Aug-98 (rajus01)
**	    Added IIapi_setEnvParam(), IIapi_abort(), IIapi_formatData().
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	03-Feb-99 (rajus01)
**	    Added support for handling callback functions in 
**	    IIAPI_SETENVPARMPARM. Added funcptr_assgn().
**	11-Feb-99 (rajus01)
**	    Added funcptr_assgn(), formatData_srcVal().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from mecopy calls.
*/





# define	TINYINT_SIZE			1
# define	SMALLINT_SIZE			2
# define	INTEGER_SIZE			4
# define	REAL_SIZE			4
# define	FLOAT_SIZE			8
# define	HANDLE_SIZE			4


/*
** Local Functions
*/

static II_PTR 		hndl_assgn( struct _varRec curVar );

static VOID 		str_assgn( struct _varRec curVar, 
			   	   char  *str );

static II_BOOL 		bool_assgn( struct _varRec curVar );

static II_LONG 		long_assgn( struct _varRec curVar );

static II_LONG 		symbolLong_assgn( struct _varRec curVar, 
				   	  struct _strlongtbl *tbl, 
				   	  i4  sizeofTbl );

static char * 		symbolStr_assgn( struct _varRec curVar, 
				  	 struct _strstrtbl *tbl, 
				  	 i4  sizeofTbl );

static f8 		float_assgn( struct _varRec curVar );

static II_INT 		int_assgn( struct _varRec curVar );

static VOID 		descriptor_assgn( struct _varRec curDscrpt, 
				  	  IIAPI_DESCRIPTOR *dscrptPtr );

static VOID 		dataval_assgn( struct _varRec curDataval, 
			       	       IIAPI_DATAVALUE *datavalPtr );

static II_LONG 		bind_longvar( char *varname );

static II_LONG 		longval_assgn( CALLTRACKLIST *ptr, i4  fieldNum );

static II_PTR 		ptrval_assgn( CALLTRACKLIST *ptr, i4  fieldNum );

static II_CHAR  	*strval_assgn( CALLTRACKLIST *ptr, i4  fieldNum );

static II_BOOL 		boolval_assgn( CALLTRACKLIST *ptr, i4  fieldNum );

static IIAPI_DESCRIPTOR *descrval_assgn( CALLTRACKLIST *ptr, i4  fieldNum );

static CALLTRACKLIST	*find_fun_num( char *varname );

static i4  		find_field_num( char *varname, 
					i4 funNum );

static VOID 		print_err( char *varname, 
				   char *fieldname, 
				   char *typename );

static II_VOID 		funcptr_assgn ( struct _varRec      curVar,
					struct _strfunctbl   *tbl,
					i4                 sizeofTbl,
					II_PTR		    **func );





/*
** Name: var_assgn_auto() - IIAPI_AUTOPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	 9-Jul-96 (gordy)
** 	    Created.
*/

/* 0 */
FUNC_EXTERN VOID
var_assgn_auto
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_AUTOPARM	*autoparm;

    autoparm = ( IIAPI_AUTOPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_ac_connHandle:
	    autoparm->ac_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

	case	_ac_tranHandle:
	    autoparm->ac_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_cancel() - IIAPI_CANCELPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind 
**		     the current number of field within the scope 
**		     of current variable assignment in the input script.  
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 1 */
FUNC_EXTERN VOID
var_assgn_cancel
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_CANCELPARM		*canc;

    canc = ( IIAPI_CANCELPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_cn_stmtHandle:
	    canc->cn_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
	    SIprintf
	        ( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                 );
            ait_exit();
   }
}






/*
** Name: var_assgn_catchEvent() - IIAPI_CATCHEVENTPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	28-Apr-95 (feige)
**	    Removed ce_eventValueCount.
**	    Updated ce_eventTime.
**	    Added ce_eventInfoAvail.
**	    
*/

/* 2 */
FUNC_EXTERN VOID
var_assgn_catchEvent
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_CATCHEVENTPARM 	*ce;

    ce = ( IIAPI_CATCHEVENTPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {	
	case	_ce_connHandle:
	    ce->ce_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

	case	_ce_selectEventName:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	ce->ce_selectEventName = ( II_CHAR * ) ait_malloc
		    ( STlength(  curVarAsgn[parmArrInd].right_side ) + 1 );	
	    else
	    	ce->ce_selectEventName = 
		    ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd],
		       ( char * )ce->ce_selectEventName );

	    return; 

	case	_ce_selectEventOwner:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	ce->ce_selectEventOwner = ( II_CHAR * ) ait_malloc(
		    STlength(  curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	ce->ce_selectEventOwner =
		    ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd],
		       ( char * )ce->ce_selectEventOwner );

	    return;

	case	_ce_eventHandle:
	    ce->ce_eventHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_ce_eventName:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	ce->ce_eventName = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	ce->ce_eventName = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )ce->ce_eventName );

	    return;

	case	_ce_eventOwner:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	ce->ce_eventOwner = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	ce->ce_eventOwner = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )ce->ce_eventOwner );

	    return;

	case	_ce_eventDB:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	ce->ce_eventDB = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	ce->ce_eventDB = ( II_CHAR * )ait_malloc ( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )ce->ce_eventDB );
	    return;

	case	_ce_eventTime:
            AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: ce_eventTime is output parameter.\n",
                  cur_line_num
                );

	    return;

	case	_ce_eventInfoAvail:
	    ce->ce_eventInfoAvail = bool_assgn( curVarAsgn[parmArrInd] );
	    return;	

        default:
            AIT_TRACE( AIT_TR_ERROR )
	     	( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
	    SIprintf
	        ( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
            ait_exit();
    }
}


/*
** Name: var_assgn_close() - IIAPI_CLOSEPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 3 */
FUNC_EXTERN VOID
var_assgn_close
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_CLOSEPARM	*clse;

    clse = ( IIAPI_CLOSEPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_cl_stmtHandle:

	    clse->cl_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_commit() - IIAPI_COMMITPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 4 */
FUNC_EXTERN VOID
var_assgn_commit
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_COMMITPARM	*cmt;

    cmt = ( IIAPI_COMMITPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_cm_tranHandle:
	    cmt->cm_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_connect() - IIAPI_CONNPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 5 */
FUNC_EXTERN VOID
var_assgn_connect
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_CONNPARM	*conn;
  
    conn = ( IIAPI_CONNPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_co_target:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	conn->co_target = ( II_CHAR * ) ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	conn->co_target = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )conn->co_target );

	    return;
	
 	case	_co_username:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	conn->co_username = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	conn->co_username = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )conn->co_username );

	    return;

	case	_co_password:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	conn->co_password = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	conn->co_password = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )conn->co_password );

	    return;

	case	_co_timeout:
	    conn->co_timeout = long_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_co_connHandle:
	    conn->co_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_co_tranHandle:
	    conn->co_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_co_sizeAdvise:
	    conn->co_sizeAdvise	= long_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_co_apiLevel:
	    conn->co_apiLevel = symbolLong_assgn( curVarAsgn[parmArrInd],
						  co_apiLevel_TBL,
						  MAX_co_apiLevel );

	    return;

	case	_co_type:
	    conn->co_type = symbolLong_assgn( curVarAsgn[parmArrInd],
				    	      co_type_TBL, 
					      MAX_co_type );

	    return;

	default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }	/* switch */
}






/*
** Name: var_assgn_disconnect() - IIAPI_DISCONNPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 6 */
FUNC_EXTERN VOID
var_assgn_disconnect
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_DISCONNPARM	*dcon;
  
    dcon = ( IIAPI_DISCONNPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_dc_connHandle:
	    dcon->dc_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_getColumns() - IIAPI_GETCOLPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 7 */
FUNC_EXTERN VOID
var_assgn_getColumns
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_GETCOLPARM		*getc;
    IIAPI_DESCRIPTOR		*dptr;
    IIAPI_DATAVALUE		*columnData;
    CALLTRACKLIST		*ptr;
    char			*index;
    i4				i, j, fieldNum;

    getc = ( IIAPI_GETCOLPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_gc_stmtHandle:
	    getc->gc_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gc_rowCount:
	    getc->gc_rowCount = int_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gc_columnCount:
	    getc->gc_columnCount = int_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gc_columnData:
	    if ( STequal( curVarAsgn[parmArrInd].right_side, "NULL" ) )
	    {
		getc->gc_columnData = NULL;

		return;
	    }

	    index = STchr( curVarAsgn[parmArrInd].right_side, '.' );

	    if ( index == NULL )
	    {
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: %s is not of type IIAPI_DESCRIPTOR.\n",
		      cur_line_num,curVarAsgn[parmArrInd].right_side );	

		SIprintf
		    ( "[line %d]: %s is not of type IIAPI_DESCRIPTOR.\n",
		      cur_line_num,curVarAsgn[parmArrInd].right_side );	

		ait_exit();
	    }

	    ptr = find_fun_num ( curVarAsgn[parmArrInd].right_side );
	    fieldNum = find_field_num( curVarAsgn[parmArrInd].right_side,
				       ptr->funNum );
	    curCallPtr->descriptor = descrval_assgn( ptr, fieldNum );

	    if ( curCallPtr->descriptor == NULL )
		getc->gc_columnData = NULL;
	    else
	    {
		getc->gc_columnData = ( IIAPI_DATAVALUE * )
		    ait_malloc ( getc->gc_rowCount *
				 getc->gc_columnCount *
				 sizeof( IIAPI_DATAVALUE ) );
	 	 columnData = getc->gc_columnData;

		 for ( i = 0; i < getc->gc_rowCount; i++ )
		 {
		     dptr = curCallPtr->descriptor;

		     for ( j = 0;
			   j < getc->gc_columnCount;
			   j++ )
		     {
			 columnData->dv_value = ( II_PTR)
			     ait_malloc( dptr->ds_length );
			 dptr++;
			 columnData++;
		     }
		}  
	    }  

	    return;
	
	case	_gc_rowsReturned:
	    getc->gc_rowsReturned = int_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: gc_rowsReturned is output parm.\n",
		  cur_line_num );

	    return;

	case	_gc_moreSegments:
	    getc->gc_moreSegments = bool_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gc_moreSegments is output parm.\n",
		  cur_line_num );

	    return;

	default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_getCopyMap() - IIAPI_GETCOPYMAPPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 8 */
FUNC_EXTERN VOID
var_assgn_getCopyMap
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_GETCOPYMAPPARM  *gcm;

    gcm = ( IIAPI_GETCOPYMAPPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_gm_stmtHandle:
	    gcm->gm_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gm_copyMap:
	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gm_copyMap is output parm.\n",
		  cur_line_num
		);
	    /* ignore the assignment in this version */

	    return;

	default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_getDescriptor() - IIAPI_GETDESCRPARM variable
**				     binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 9 */
FUNC_EXTERN VOID
var_assgn_getDescriptor
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_GETDESCRPARM	*getd;

    getd = ( IIAPI_GETDESCRPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
 	case	_gd_stmtHandle:
	    getd->gd_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gd_descriptorCount:
	    getd->gd_descriptorCount = long_assgn( curVarAsgn[parmArrInd] );

/*
	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gd_descriptorCount is output parm.\n",
		  cur_line_num );
	    assign value to getd->gd_descriptorCount anyway ? */ 

	    return;

	case	_gd_descriptor:
	    if ( STequal( curVarAsgn[parmArrInd].right_side, "NULL" ) )
	    {
	    	getd->gd_descriptor = NULL;
		return;
	    }

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: gd_descriptor is output parm.\n",
	      	  cur_line_num );
	    /* assign value to getd->gd_descriptor anyway ? */ 

	    return;
	    
        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_getErrorInfo() - IIAPI_GETEINFOPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**	  	     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 10 */
FUNC_EXTERN VOID
var_assgn_getErrorInfo
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
}






/*
** Name: var_assgn_getQueryInfo() - IIAPI_GETQINFOPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 11 */
FUNC_EXTERN VOID
var_assgn_getQueryInfo
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_GETQINFOPARM	*getq;

    getq = ( IIAPI_GETQINFOPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_gq_stmtHandle:
	    getq->gq_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_gq_readonly:
	    getq->gq_readonly = bool_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_readonly is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_flags:
	    getq->gq_flags = symbolLong_assgn( curVarAsgn[parmArrInd],
					       gq_flags_TBL,
					       MAX_gq_flags );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: gq_flags is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_mask:
	    getq->gq_mask = symbolLong_assgn( curVarAsgn[parmArrInd],
					      gq_mask_TBL,
					      MAX_gq_mask );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: gq_mask is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_rowCount:
	    getq->gq_rowCount = long_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_rowCount is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_procedureReturn:
	    getq->gq_procedureReturn = long_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_procedureReturn is output parm\n",
		  cur_line_num );

	    return;

	case	_gq_procedureHandle:
	    getq->gq_procedureHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_procedureHandle is output parm.\n",
	      	  cur_line_num );

	    return;

	case	_gq_repeatQueryHandle:
	    getq->gq_repeatQueryHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    AIT_TRACE( AIT_TR_WARN )
	        ( "[line %d]: Warning: gq_repeatQueryHandle is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_tableKey:
	    /* ignore at this point */
	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_tableKey is output parm.\n",
		  cur_line_num );

	    return;

	case	_gq_objectKey:
	    /* ignore at this point */
	    AIT_TRACE( AIT_TR_WARN )
		( "[line %d]: Warning: gq_objectKey is output parm.\n",
		  cur_line_num );

	    return;

	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		SIprintf
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		ait_exit();
    }
}






/*
** Name: var_assgn_getEvent() - IIAPI_GETEVENTPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	16-Jul-96 (gordy)
**	    Created.
*/

/* 12 */
FUNC_EXTERN VOID
var_assgn_getEvent
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_GETEVENTPARM 	*gv;

    gv = ( IIAPI_GETEVENTPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {	
	case	_gv_connHandle:
	    gv->gv_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

	case	_gv_timeout:	
	    gv->gv_timeout = long_assgn( curVarAsgn[parmArrInd] );
	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
	     	( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
	    SIprintf
	        ( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
            ait_exit();
    }
}


/*
** Name: var_assgn_initialize() - IIAPI_INITPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 13 */
FUNC_EXTERN VOID
var_assgn_initialize 
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_INITPARM	*init;

    init = ( IIAPI_INITPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_in_timeout:	
		init->in_timeout = long_assgn( curVarAsgn[parmArrInd] );

		return;

	case	_in_version:	
		init->in_version = symbolLong_assgn( curVarAsgn[parmArrInd],
				    		     in_version_TBL, 
						     MAX_in_version );

		return;

	case	_in_envHandle:
		init->in_envHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		SIprintf
			 ( "[line %d]: unrecognized field name %s.\n",
			  cur_line_num,
			  funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		ait_exit();
    }	/* switch */
}	






/*
** Name: var_assgn_modifyConnect() - IIAPI_MODCONNPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 14 */
FUNC_EXTERN VOID
var_assgn_modifyConnect 
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_MODCONNPARM	*mcon;

    mcon = ( IIAPI_MODCONNPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_mc_connHandle:
	    mcon->mc_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]

                );
            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_prepareCommit() - IIAPI_PREPCMTPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 15 */
FUNC_EXTERN VOID
var_assgn_prepareCommit
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_PREPCMTPARM	*prepc;

    prepc = ( IIAPI_PREPCMTPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_pr_tranHandle:
	    prepc->pr_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )(
		"[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_putColumns() - IIAPI_PUTCOLPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	22-Apr-96 (gordy)
**	    Allocate memory for the pc_columnData data value array.
*/

/* 16 */
FUNC_EXTERN VOID
var_assgn_putColumns
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_PUTCOLPARM	*putc;
    IIAPI_DATAVALUE	*datavalPtr;
    struct _dataValList	*nodeptr;
    i4  		i;

    putc = ( IIAPI_PUTCOLPARM	* )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_pc_stmtHandle:
	    putc->pc_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

        case	_pc_columnCount:
	    putc->pc_columnCount = long_assgn( curVarAsgn[parmArrInd] );

	    return;

 	case	_pc_columnData:
	    if (putc->pc_columnCount <= 0)
		return;

	    if ( STequal( curVarAsgn[parmArrInd].right_side, "NULL" ) )
	    {
		putc->pc_columnData = NULL;

		return;
	    }

	    putc->pc_columnData = ( IIAPI_DATAVALUE * )
		ait_malloc( putc->pc_columnCount * sizeof( IIAPI_DATAVALUE ) );

	    nodeptr = dataValList; 
	    datavalPtr = putc->pc_columnData;

	    for ( i = 0; i < putc->pc_columnCount; i++ )
	    {
		if ( nodeptr == NULL )
		    return;

		dataval_assgn( nodeptr->valBuf, datavalPtr );
		nodeptr = nodeptr->next;
		datavalPtr++;
	    }

	    free_datavalList();
	    
	    return;

	case	_pc_moreSegments:
	    putc->pc_moreSegments = bool_assgn( curVarAsgn[parmArrInd] );

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )(
		    "[line %d]: unrecognized field name %s.\n",
                     cur_line_num,
                     funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_putParms() - IIAPI_PUTPARMPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 17 */
FUNC_EXTERN VOID
var_assgn_putParms
( 
    i4	parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_PUTPARMPARM	*pupp;
    IIAPI_DATAVALUE	*datavalPtr;
    struct _dataValList	*nodeptr;
    i4	i;

    pupp = ( IIAPI_PUTPARMPARM * )curCallPtr->parmBlockPtr;

    switch 	(parmIndex)
    {
	case	_pp_stmtHandle:
	    pupp->pp_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_pp_parmCount:
	    pupp->pp_parmCount = long_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_pp_parmData:
	    if (pupp->pp_parmCount <= 0)
		return;

	    if ( STequal( curVarAsgn[parmArrInd].right_side, "NULL" ) )
	    {
		pupp->pp_parmData = NULL;

		return;
	    }

	    pupp->pp_parmData = ( IIAPI_DATAVALUE * ) ait_malloc (
			pupp->pp_parmCount * sizeof ( IIAPI_DATAVALUE ) );

	    nodeptr = dataValList;
	    datavalPtr = pupp->pp_parmData;

	    for ( i = 1; i <= pupp->pp_parmCount; i++ )
	    {
		if ( nodeptr == NULL )
		    return;

		dataval_assgn( nodeptr->valBuf, datavalPtr );

		nodeptr = nodeptr->next;
		datavalPtr++;
	    }

	    free_datavalList();
	
	    return;

	case	_pp_moreSegments:
	    pupp->pp_moreSegments = bool_assgn( curVarAsgn[parmArrInd] );

	    return;

	default:
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
		  cur_line_num,
		  funTBL[curCallPtr->funNum].parmName[parmIndex]	
		);

	    SIprintf( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );

	   ait_exit();
    }	/* switch */
}






/*
** Name: var_assgn_query() - IIAPI_QUERYPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 18 */
FUNC_EXTERN VOID
var_assgn_query
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_QUERYPARM	*qury;
    
    qury = ( IIAPI_QUERYPARM * )curCallPtr->parmBlockPtr;

    switch 	(parmIndex)
    {
	case	_qy_connHandle:
		qury->qy_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

  	case	_qy_queryType:
		qury->qy_queryType = ( IIAPI_QUERYTYPE) symbolLong_assgn( 
							curVarAsgn[parmArrInd],
							qy_queryType_TBL,
							MAX_qy_queryType );

		return;

	case	_qy_queryText:
		if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
		    qury->qy_queryText = ( II_CHAR * )
			ait_malloc
			   ( STlength( curVarAsgn[parmArrInd].right_side )
			     + 1 );
		else
		    qury->qy_queryText = ( II_CHAR * )
			ait_malloc( MAX_STR_LEN );

		str_assgn( curVarAsgn[parmArrInd],
			   ( char * )qury->qy_queryText );

		return;

	case	_qy_parameters:
		qury->qy_parameters = bool_assgn ( curVarAsgn[parmArrInd] );

		return;

	case	_qy_tranHandle:
		qury->qy_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

	case	_qy_stmtHandle:
		qury->qy_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

	default:
                AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

		SIprintf
		    ( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

                ait_exit();
    }		/* case */
}






/*
** Name: var_assgn_registerXID() - IIAPI_REGXIDPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	14-Jun-95 (gordy)
**	    Finalized Two Phase Commit interface.
*/

/* 19 */
FUNC_EXTERN VOID
var_assgn_registerXID
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_REGXIDPARM	*regXID;

    regXID = (IIAPI_REGXIDPARM *)curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_ti_type:	
		regXID->rg_tranID.ti_type = 
				symbolLong_assgn( curVarAsgn[ parmArrInd ], 
						  ti_type_TBL, MAX_ti_type );
		return;

	case	_it_highTran:	
		regXID->rg_tranID.ti_value.iiXID.ii_tranID.it_highTran = 
				long_assgn( curVarAsgn[ parmArrInd ] );
		return;

	case	_it_lowTran:	
		regXID->rg_tranID.ti_value.iiXID.ii_tranID.it_lowTran = 
				long_assgn( curVarAsgn[ parmArrInd ] );
		return;

	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		SIprintf
			 ( "[line %d]: unrecognized field name %s.\n",
			  cur_line_num,
			  funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		ait_exit();
    }	/* switch */
}






/*
** Name: var_assgn_releaseEnv() - IIAPI_RELENVPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	10-Feb-97 (gordy)
** 	    Created.
**	    
*/

/* 20 */
FUNC_EXTERN VOID
var_assgn_releaseEnv
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_RELENVPARM	*relEnv;

    relEnv = ( IIAPI_RELENVPARM	* )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_re_envHandle:	
		relEnv->re_envHandle = hndl_assgn( curVarAsgn[parmArrInd] );
		return;

	case	_re_status:
	    relEnv->re_status = symbolLong_assgn( curVarAsgn[parmArrInd],
					 	  gp_status_TBL,
					 	  MAX_gp_status );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: re_status is output parm.\n",
		  cur_line_num );

	default:
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
		  cur_line_num,
	      	  funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    SIprintf( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
	      	      funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    ait_exit();
    }
}






/*
** Name: var_assgn_releaseXID() - IIAPI_RELXIDPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	14-Jun-95 (gordy)
**	    Finalized Two Phase Commit interface.
*/

/* 21 */
FUNC_EXTERN VOID
var_assgn_releaseXID
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_RELXIDPARM	*relXID;

    relXID = (IIAPI_RELXIDPARM *)curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_rl_tranIdHandle:	
		relXID->rl_tranIdHandle = hndl_assgn( curVarAsgn[parmArrInd] );
		return;

	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		SIprintf
			 ( "[line %d]: unrecognized field name %s.\n",
			  cur_line_num,
			  funTBL[curCallPtr->funNum].parmName[parmIndex] );	

		ait_exit();
    }	/* switch */
}






/*
** Name: var_assgn_rollback() - IIAPI_ROLLBACKPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 22 */
FUNC_EXTERN VOID
var_assgn_rollback
( 
    i4	parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_ROLLBACKPARM	*roll;

    roll = ( IIAPI_ROLLBACKPARM * )curCallPtr->parmBlockPtr;

    switch 	(parmIndex)
    {
	case	_rb_tranHandle:
	    roll->rb_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

	case	_rb_savePointHandle:
	    roll->rb_savePointHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]

                );
            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_savePoint() - IIAPI_SAVEPTPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 23 */
FUNC_EXTERN VOID
var_assgn_savePoint
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_SAVEPTPARM	*savept;

    savept = ( IIAPI_SAVEPTPARM	* )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_sp_tranHandle:
	    savept->sp_tranHandle = hndl_assgn( curVarAsgn[parmArrInd] );

	    return;

 	case	_sp_savePoint:
	    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
	    	savept->sp_savePoint = ( II_CHAR * )ait_malloc
		    ( STlength( curVarAsgn[parmArrInd].right_side ) + 1 );
	    else
	    	savept->sp_savePoint = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	    str_assgn( curVarAsgn[parmArrInd], ( char * )savept->sp_savePoint);

	    return;

	case	_sp_savePointHandle:
	    savept->sp_savePointHandle = hndl_assgn( curVarAsgn[parmArrInd] );
/*
	    AIT_TRACE( AIT_TR_WARN )
		    ( "[line %d]: Warning: sp_savePointHandle is output parm.\n",
		      cur_line_num );
	    assign value to getd->gd_descriptorCount anyway ? */ 

	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );

            SIprintf( "[line %d]: unrecognized field name %s.\n",
                      cur_line_num,
                      funTBL[curCallPtr->funNum].parmName[parmIndex]
                    );

            ait_exit();
    }
}






/*
** Name: var_assgn_setConnectParam() - IIAPI_SETCONPRMPARM variable
**				       binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	10-May-95 (feige)
**	    Updated sc_paramValue assignment.
**	    
*/

/* 24 */
FUNC_EXTERN VOID
var_assgn_setConnectParam
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
   IIAPI_SETCONPRMPARM	*setc;

   setc = ( IIAPI_SETCONPRMPARM	* )curCallPtr->parmBlockPtr;

   switch( parmIndex )
   {
	case	_sc_connHandle:			/* 0 */
		setc->sc_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

	case	_sc_paramID:			/* 1 */
		setc->sc_paramID = symbolLong_assgn( curVarAsgn[parmArrInd],
				    		     sc_paramID_TBL, 
						     MAX_sc_paramID );

		return;

	case	_sc_paramValue:			/* 2 */
		/*
		** sc_paramID == 1..10, 25, 26
		*/
		if ( IS_PRMID_LONG )
		{
		    setc->sc_paramValue = ( II_LONG * )
			ait_malloc( sizeof ( II_LONG ) );

		    *( ( II_LONG * )setc->sc_paramValue )
			= symbolLong_assgn( curVarAsgn[parmArrInd],
					    CP_SYMINT_TBL,
					    MAX_CP_SYMINT
					  );

		    return;
	   	}

		/*
		** sc_paramID == 12, 14, 16, 18, 20, 22, 27, 28, 33, 34
		*/
		if ( IS_PRMID_CHAR )
		{
		    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
		    	setc->sc_paramValue = ( II_CHAR * )
			    ait_malloc (
				STlength( curVarAsgn[parmArrInd].right_side )
				+ 1 );
		    else
		    	setc->sc_paramValue =
			    ( II_CHAR * ) ait_malloc ( MAX_STR_LEN );

		    STcopy(
			    symbolStr_assgn( curVarAsgn[parmArrInd],
					     CP_SYMSTR_TBL,
					     MAX_CP_SYMSTR
					   ),
			    ( char * )setc->sc_paramValue 
			  );

		    return;
		}

		/*
		** sc_paramID == 29..32 
		*/
		if ( IS_PRMID_BOOL )
		{
		    setc->sc_paramValue = ( II_BOOL * )
		        ait_malloc( sizeof ( II_BOOL ) );
		    *( ( II_BOOL * )setc->sc_paramValue )
			= bool_assgn( curVarAsgn[parmArrInd] );

		    return;
		}

		/*
		** sc_paramID == 11, 13, 15, 17, 19, 21, 23
		*/
		switch(setc->sc_paramID)
		{
		    case IIAPI_CP_FLOAT4_STYLE:			/* 11 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
		  	    setc->sc_paramValue =( II_CHAR * )
				ait_malloc( MAX_VAR_LEN *
					    sizeof ( II_CHAR ) );	
			else
		  	    setc->sc_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy( symbolStr_assgn( curVarAsgn[parmArrInd],
				      		 CP_FLOAT4_STYLE_TBL, 
						 MAX_CP_FLOAT4_STYLE ),
			        ( char * )setc->sc_paramValue );

			return;
			    
		    case IIAPI_CP_NUMERIC_TREATMENT:		/* 13 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
			    setc->sc_paramValue =( II_CHAR * )ait_malloc
				( MAX_VAR_LEN * sizeof ( II_CHAR ) );	
			else
			    setc->sc_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy( symbolStr_assgn( curVarAsgn[parmArrInd],
						 CP_NUMERIC_TREATMENT_TBL, 
						 MAX_CP_NUMERIC_TREATMENT ),
			        ( char * )setc->sc_paramValue );

			return;
			    
		    case IIAPI_CP_MONEY_LORT:			/* 15 */
			setc->sc_paramValue =
			    ( II_LONG * )ait_malloc( sizeof ( II_LONG ) );	
			*( ( II_LONG * )setc->sc_paramValue ) 
			    = symbolLong_assgn( curVarAsgn[parmArrInd],
					        CP_MONEY_LORT_TBL, 
						MAX_CP_MONEY_LORT );
			    
			return;
			    
		    case IIAPI_CP_MATH_EXCP:			/* 17 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
			    setc->sc_paramValue = ( II_CHAR * )
				ait_malloc( MAX_VAR_LEN *
					    sizeof ( II_CHAR) );	
			else
			    setc->sc_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy(symbolStr_assgn( curVarAsgn[parmArrInd],
				      	        CP_MATH_EXCP_TBL, 
						MAX_CP_MATH_EXCP ),
			       ( char * )setc->sc_paramValue );

			return;
		    
		    case IIAPI_CP_DATE_FORMAT:			/* 19 */
			setc->sc_paramValue =
			    ( II_LONG * )ait_malloc( sizeof ( II_LONG ) );	
			*( ( II_LONG * )setc->sc_paramValue) 
			    = symbolLong_assgn( curVarAsgn[parmArrInd],
					        CP_DATE_FORMAT_TBL, 
						MAX_CP_DATE_FORMAT );
		    
			return;
		    
		    case IIAPI_CP_SECONDARY_INX:		/* 21 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
			     setc->sc_paramValue = ( II_CHAR * )
				ait_malloc( MAX_VAR_LEN *
					    sizeof ( II_CHAR) );	
			else
			     setc->sc_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy( symbolStr_assgn( curVarAsgn[parmArrInd],
				      		 CP_SECONDARY_INX_TBL, 
						 MAX_CP_SECONDARY_INX),
			        ( char * )setc->sc_paramValue );

			return;
		    
		    case IIAPI_CP_SERVER_TYPE:			/* 21 */
			setc->sc_paramValue =
			    ( II_LONG * )ait_malloc( sizeof ( II_LONG ) );	
			*( ( II_LONG * )setc->sc_paramValue ) 
			    = symbolLong_assgn( curVarAsgn[parmArrInd],
						CP_SERVER_TYPE_TBL, 
						MAX_CP_SERVER_TYPE );
		    
			return;
		    
		    default:
			AIT_TRACE( AIT_TR_ERROR )
			    ( "[line %d]: undefined sc_paramID.\n",
			      cur_line_num );
		    
			SIprintf	
			    ( "[line %d]: undefined sc_paramID.\n",
			      cur_line_num );
		    
			ait_exit();
		}
		    
		break;
 
	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	
		    
		SIprintf
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	
		    
		ait_exit();
    }	/* switch */
}

		    
		    
/*
** Name: var_assgn_setEnvParam() - IIAPI_SETENVPRMPARM variable
**				       binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**    19-Aug-98 (rajus01)
**	Created.
*/

/* 25 */
FUNC_EXTERN VOID
var_assgn_setEnvParam
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
   IIAPI_SETENVPRMPARM	*sete;

   sete = ( IIAPI_SETENVPRMPARM	* )curCallPtr->parmBlockPtr;

   switch( parmIndex )
   {
	case	_se_envHandle:			/* 0 */
		sete->se_envHandle = hndl_assgn( curVarAsgn[parmArrInd] );

		return;

	case	_se_paramID:			/* 1 */
		sete->se_paramID = symbolLong_assgn( curVarAsgn[parmArrInd],
				    		     se_paramID_TBL, 
						     MAX_se_paramID );
		return;

	case	_se_paramValue:			/* 2 */
		/*
		** se_paramID == 1..10, 22, 23, 24
		*/
		if ( IS_EPRMID_LONG )
		{
		    sete->se_paramValue = ( II_LONG * )
			ait_malloc( sizeof ( II_LONG ) );

		    *( ( II_LONG * )sete->se_paramValue )
			= symbolLong_assgn( curVarAsgn[parmArrInd],
					    EP_SYMINT_TBL,
					    MAX_EP_SYMINT
					  );

		    return;
	   	}

		/*
		** se_paramID == 12, 14, 16, 18, 20, 21
		*/
		if ( IS_EPRMID_CHAR )
		{
		    if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
		    	sete->se_paramValue = ( II_CHAR * )
			    ait_malloc (
				STlength( curVarAsgn[parmArrInd].right_side )
				+ 1 );
		    else
		    	sete->se_paramValue =
			    ( II_CHAR * ) ait_malloc ( MAX_STR_LEN );

		    STcopy(
			    symbolStr_assgn( curVarAsgn[parmArrInd],
					     EP_SYMSTR_TBL,
					     MAX_EP_SYMSTR
					   ),
			    ( char * )sete->se_paramValue 
			  );

		    return;
		}

		/*
		** se_paramID == 25, 26, 27
		*/
		if ( IS_EPRMID_FUNC )
		{
		    sete->se_paramValue = (II_PTR) ait_malloc( sizeof (II_PTR) );
	    	    funcptr_assgn( curVarAsgn[parmArrInd],
				   EP_FUNC_TBL, MAX_EP_FUNC,
				   (II_PTR) &(sete->se_paramValue) );
		    return;

	        }
		/*
		** se_paramID == 11, 13, 15, 17, 19
		*/
		switch(sete->se_paramID)
		{
		    case IIAPI_EP_FLOAT4_STYLE:			/* 11 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
		  	    sete->se_paramValue =( II_CHAR * )
				ait_malloc( MAX_VAR_LEN *
					    sizeof ( II_CHAR ) );	
			else
		  	    sete->se_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy( symbolStr_assgn( curVarAsgn[parmArrInd],
				      		 EP_FLOAT4_STYLE_TBL, 
						 MAX_EP_FLOAT4_STYLE ),
			        ( char * )sete->se_paramValue );

			return;
			    
		    case IIAPI_EP_NUMERIC_TREATMENT:		/* 13 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
			    sete->se_paramValue =( II_CHAR * )ait_malloc
				( MAX_VAR_LEN * sizeof ( II_CHAR ) );	
			else
			    sete->se_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy( symbolStr_assgn( curVarAsgn[parmArrInd],
						 EP_NUMERIC_TREATMENT_TBL, 
						 MAX_EP_NUMERIC_TREATMENT ),
			        ( char * )sete->se_paramValue );

			return;
			    
		    case IIAPI_EP_MONEY_LORT:			/* 15 */
			sete->se_paramValue =
			    ( II_LONG * )ait_malloc( sizeof ( II_LONG ) );	
			*( ( II_LONG * )sete->se_paramValue ) 
			    = symbolLong_assgn( curVarAsgn[parmArrInd],
					        EP_MONEY_LORT_TBL, 
						MAX_EP_MONEY_LORT );
			    
			return;
			    
		    case IIAPI_EP_MATH_EXCP:			/* 17 */
			if ( curVarAsgn[parmArrInd].right_side[0] == '\"' )
			    sete->se_paramValue = ( II_CHAR * )
				ait_malloc( MAX_VAR_LEN *
					    sizeof ( II_CHAR) );	
			else
			    sete->se_paramValue =
				( II_CHAR * )ait_malloc( MAX_STR_LEN );

			STcopy(symbolStr_assgn( curVarAsgn[parmArrInd],
				      	        EP_MATH_EXCP_TBL, 
						MAX_EP_MATH_EXCP ),
			       ( char * )sete->se_paramValue );

			return;
		    
		    case IIAPI_EP_DATE_FORMAT:			/* 19 */
			sete->se_paramValue =
			    ( II_LONG * )ait_malloc( sizeof ( II_LONG ) );	
			*( ( II_LONG * )sete->se_paramValue) 
			    = symbolLong_assgn( curVarAsgn[parmArrInd],
					        EP_DATE_FORMAT_TBL, 
						MAX_EP_DATE_FORMAT );
		    
			return;
		    
		    default:
			AIT_TRACE( AIT_TR_ERROR )
			    ( "[line %d]: undefined se_paramID.\n",
			      cur_line_num );
		    
			SIprintf	
			    ( "[line %d]: undefined se_paramID.\n",
			      cur_line_num );
		    
			ait_exit();
		}
		    
		break;

	case	_se_status:
	    sete->se_status = symbolLong_assgn( curVarAsgn[parmArrInd],
					 	gp_status_TBL,
					 	MAX_gp_status );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: se_status is output parm.\n",
		  cur_line_num );

 
	default:
		AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	
		    
		SIprintf
		    ( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
		      funTBL[curCallPtr->funNum].parmName[parmIndex] );	
		    
		ait_exit();
    }	/* switch */
}

		    
		    

/*
** Name: var_assgn_setDescriptor() - IIAPI_SETDESCRPARM variable
** 				     binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	13-Mar-95 (feige)
		    

/*
** Name: var_assgn_setDescriptor() - IIAPI_SETDESCRPARM variable
** 				     binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	13-Mar-95 (feige)
**	    Correct typo for sd_descriptor assignment.
**	    
*/

/* 26 */
FUNC_EXTERN VOID
var_assgn_setDescriptor
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_SETDESCRPARM		*setd;
    struct _descriptorList	*nodeptr;
    CALLTRACKLIST		*ptr;
    IIAPI_DESCRIPTOR		*dscrptPtr, *srcPtr, *curDescr;
    char			*index;
    i4				i, j, fieldNum;

    setd = ( IIAPI_SETDESCRPARM	* )curCallPtr->parmBlockPtr;
    
    switch( parmIndex )
    {
	case	_sd_stmtHandle:
	    setd->sd_stmtHandle = hndl_assgn( curVarAsgn[parmArrInd] );
    
	    return;

	case	_sd_descriptorCount:
	    setd->sd_descriptorCount = long_assgn( curVarAsgn[parmArrInd] );
    
	    return;

	case	_sd_descriptor:
	    if ( setd->sd_descriptorCount <= 0 )
	    	return;

	    if ( STequal( curVarAsgn[parmArrInd].right_side, "NULL" ) )
	    {
		setd->sd_descriptor = NULL;
    
		return;
	    }

	    setd->sd_descriptor 
		= ( IIAPI_DESCRIPTOR * )
		    ait_malloc ( setd->sd_descriptorCount *
				 sizeof ( IIAPI_DESCRIPTOR ) );
	    
	    if ( STequal( curVarAsgn[parmArrInd].right_side, "_DESCRIPTOR" ) )
	    {
		if ( descriptorCount < 1 )
		{
	    	    AIT_TRACE( AIT_TR_ERROR )
		    ( "[line %d]: Invalid descriptorCount '%d'.\n",
		  		cur_line_num, descriptorCount );
		    ait_exit();
		}

	    	nodeptr = descriptorList[0];
	    	dscrptPtr = setd->sd_descriptor;

	    	for ( i = 1; i <= setd->sd_descriptorCount; i++ )
	    	{
		    if ( nodeptr == NULL )
		    	return;

		    for ( j = 0; j < 8; j++ )
		    {
		    	if ( STequal( nodeptr->descrptRec[j].left_side,
				      "_NULL" ) )
			    break;

		    	descriptor_assgn( nodeptr->descrptRec[j],
				     	  dscrptPtr );
		    }

		    nodeptr = nodeptr->next;
		    dscrptPtr++;	
	    	}	

	    	return;
	    }

	/* 
	** Copy another descriptor to sd_descriptor
	*/
	index = STchr( curVarAsgn[parmArrInd].right_side, '.' );

	if ( index == NULL )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is incorrect variable of type IIAPI_DESCRIPTOR.\n", 
		  cur_line_num,
		  curVarAsgn[parmArrInd].right_side );

	    SIprintf 
		( "[line %d]: %s is incorrect variable of type IIAPI_DESCRIPTOR.\n", 
		  cur_line_num,
		  curVarAsgn[parmArrInd].right_side );

	    ait_exit();
	}

	ptr = find_fun_num ( curVarAsgn[parmArrInd].right_side );
	fieldNum = find_field_num( curVarAsgn[parmArrInd].right_side,
				   ptr->funNum );

	srcPtr = descrval_assgn ( ptr, fieldNum );

	dscrptPtr = setd->sd_descriptor;
	curDescr = srcPtr;

	for ( i = 1; i <= setd->sd_descriptorCount; i++ )
	{
	   if ( curDescr == NULL )
		break;

	   dscrptPtr->ds_dataType = curDescr->ds_dataType;
	   dscrptPtr->ds_nullable = curDescr->ds_nullable;
	   dscrptPtr->ds_length = curDescr->ds_length;
	   dscrptPtr->ds_precision = curDescr->ds_precision;
	   dscrptPtr->ds_scale = curDescr->ds_scale;
	   dscrptPtr->ds_columnType = curDescr->ds_columnType;
	   dscrptPtr->ds_columnName = ( II_CHAR * )
	       ait_malloc( STlength( curDescr->ds_columnName ) + 1 );
	   STcopy( curDescr->ds_columnName, dscrptPtr->ds_columnName );

	   dscrptPtr++;
	   curDescr++;
	}

	return;

	default:
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
		  cur_line_num,
	      	  funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    SIprintf( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
	      	      funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    ait_exit();
    }	/* switch */
}






/*
** Name: var_assgn_terminate() - IIAPI_TERMPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 27 */
FUNC_EXTERN VOID
var_assgn_terminate
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_TERMPARM	*term;

    term = ( IIAPI_TERMPARM	* )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_tm_status:
	    term->tm_status = symbolLong_assgn( curVarAsgn[parmArrInd],
					 	gp_status_TBL,
					 	MAX_gp_status );

	    AIT_TRACE( AIT_TR_WARN )
	     	( "[line %d]: Warning: tm_status is output parm.\n",
		  cur_line_num );

	default:
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
		  cur_line_num,
	      	  funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    SIprintf( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
	      	      funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    ait_exit();
    }
}






/*
** Name: var_assgn_wait() - IIAPI_WAITPARM variable binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

/* 28 */
FUNC_EXTERN VOID
var_assgn_wait
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
}

/* 29 */
FUNC_EXTERN VOID
var_assgn_abort
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_ABORTPARM		*abrt;

    abrt = ( IIAPI_ABORTPARM * )curCallPtr->parmBlockPtr;

    switch( parmIndex )
    {
	case	_ab_connHandle:
	    abrt->ab_connHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

        default:
            AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                );
	    SIprintf
	        ( "[line %d]: unrecognized field name %s.\n",
                  cur_line_num,
                  funTBL[curCallPtr->funNum].parmName[parmIndex]
                 );
            ait_exit();
   }
}

/*
** Name: var_assgn_formatData() - IIAPI_FORMATPARM variable
**				       binding function
**
** Description:
**	This function checks the semantics of field assignment, allocates 
**	buffer for the left_hand variable, parses the right_hand value 
**	and binds the value into the buffer. Exit whenever error is found.
**
** Inputs:
**	parmIndex  - the array index into funTBL[_IIapi_cancel]
**	parmArrInd - the array index into curVarAsgn indicatind the current 
**		     number of field within the scope of current variable 
**		     assignment in the input script.
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**    21-Aug-98 (rajus01)
**	Created.
**    11-Sep-98 (rajus01)
**	Resolved the core dump problem during descriptor assignment for
**	IIapi_formatData().
**	
*/

/* 30 */
FUNC_EXTERN VOID
var_assgn_frmtData
( 
    i4  parmIndex, 
    i4  parmArrInd 
)
{
    IIAPI_FORMATPARM		*frmt;
    IIAPI_DESCRIPTOR		*dscrptPtr;
    i4				j, fieldNum;
    IIAPI_DATAVALUE		*datavalPtr;
    struct _dataValList		*nodeptr;
    struct _descriptorList	*nodeDptr;
    CALLTRACKLIST 		*ptr;

    frmt = ( IIAPI_FORMATPARM * )curCallPtr->parmBlockPtr;
    
            AIT_TRACE( AIT_TR_ERROR )
		( "var_assgn_frmtData.\n" );
    switch( parmIndex )
    {
	case	_fd_envHandle:
	    frmt->fd_envHandle = hndl_assgn( curVarAsgn[parmArrInd] );
	    return;

	case	_fd_srcDesc:

	    if ( descriptorCount < 1 )
	    {
	    	AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: Invalid descriptorCount '%d'.\n",
		  		cur_line_num, descriptorCount );
		ait_exit();
	    }

	    nodeDptr = descriptorList[0];
	    dscrptPtr = &frmt->fd_srcDesc;

	    for ( j = 0; j < 8; j++ )
	    {
	        if ( STequal( nodeDptr->descrptRec[j].left_side, "_NULL" ) )
		    break;

		descriptor_assgn( nodeDptr->descrptRec[j], dscrptPtr );
	    }

	    return;

	case	_fd_dstDesc:

	    if ( descriptorCount < 2 )
	    {
	    	AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: Invalid descriptorCount '%d'.\n",
		  		cur_line_num, descriptorCount );
		ait_exit();
	    }

	    nodeDptr = descriptorList[1];
	    dscrptPtr = &frmt->fd_dstDesc;

	    for ( j = 0; j < 8; j++ )
	    {
	        if ( STequal( nodeDptr->descrptRec[j].left_side, "_NULL" ) )
		    break;

		descriptor_assgn( nodeDptr->descrptRec[j], dscrptPtr );
	    }

	    return;

	case _fd_srcValue:

	    nodeptr = dataValList;
	    datavalPtr = &frmt->fd_srcValue;
		
	    if( frmt->fd_srcValue.dv_length == 0 )
	        dataval_assgn( nodeptr->valBuf, datavalPtr );

	    free_datavalList();
	
	    return;

	case	_fd_dstValue:

	    ptr = find_fun_num ( curVarAsgn[parmArrInd].right_side );
	    fieldNum = find_field_num( curVarAsgn[parmArrInd].right_side,
				       ptr->funNum );
	    curCallPtr->descriptor = descrval_assgn( ptr, fieldNum );
	    dscrptPtr = curCallPtr->descriptor; 
	    frmt->fd_dstValue.dv_value = ( II_PTR) 
					   ait_malloc( dscrptPtr->ds_length );
	    return;

	default:
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: unrecognized field name %s.\n",
		  cur_line_num,
	      	  funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    SIprintf( "[line %d]: unrecognized field name %s.\n",
		      cur_line_num,
	      	      funTBL[curCallPtr->funNum].parmName[parmIndex] );

	    ait_exit();
    }	/* switch */
}


/*
** Name: hndl_assgn() - II_PTR value assigning function
**
** Description:
**	This function assigns the II_PTR value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static II_PTR
hndl_assgn
(  
    struct _varRec curVar 
)
{
    II_PTR	hndl;
    CALLTRACKLIST *ptr;
    char	*index;
    i4		fieldNum;

    if ( STequal( curVar.right_side, "NULL" ) )
	return	 ( NULL );

    index = STchr( curVar.right_side, '.');

    if ( index == NULL )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: %s is not a handle.\n",
	      cur_line_num,
	      curVar.right_side );

	SIprintf( "[line %d]: %s is not a handle.\n",
		  cur_line_num,
		  curVar.right_side );

 	ait_exit();
    }

    ptr = find_fun_num ( curVar.right_side );
    fieldNum = find_field_num( curVar.right_side, ptr->funNum );
    hndl = ptrval_assgn( ptr, fieldNum );

    return ( hndl );
}






/*
** Name: symbolLong_assgn() - long value searching function
** Description:
**	This function converts a symbol defined in iiapi.h to its long value by 
**      looking up the table tbl, parses the right_hand value directly if the
**      left_hand symbol could not be found in the table.
**
** Inputs:
**	curVar    - current variable pair of left_hand name string
**		    and right_hand
**	            value string
**      tbl    	  - the table to look up
**      sizeofTbl - the integer size of table tbl
**
** Outputs:
**	None.
**
** returns:
**	long value
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static II_LONG
symbolLong_assgn
( 
    struct _varRec 	curVar, 
    struct _strlongtbl 	*tbl, 
    i4  		sizeofTbl 
)
{
    II_LONG	val;
    i4		i;

    for ( i = 0; 
	  i < sizeofTbl && 
	  ! STequal( curVar.right_side, tbl[i].symbol );
	  i++ );
    
    if ( i != sizeofTbl )
	return	( tbl[i].longVal );

    val = long_assgn ( curVar );

    return( val );
    
}






/*
** Name: symbolStr_assgn() - string value searching function
** Description:
**	This function converts a symbol defined in iiapi.h to its string 
**	value by looking up the table tbl, parses the right_hand value 
**	directly if the left_hand symbol could not be found in the table.
**
** Inputs:
**	curVar    - current variable pair of left_hand name string and 
**		    right_hand value string
**      tbl    	  - the table to look up
**      sizeofTbl - the integer size of table tbl
**
** Outputs:
**	None.
**
** returns:
**	string value
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	10-May-95 (feige)
**	    Fixed bug.
**	    
*/

static char  *
symbolStr_assgn
( 
    struct _varRec	curVar, 
    struct _strstrtbl 	*tbl, 
    i4  		sizeofTbl 
)
{
    i4		i;
    char 	*str;

    if ( curVar.right_side[0] != '"' )
    {
    	for ( i = 0; 
	      i < sizeofTbl &&
	      ! STequal( curVar.right_side, tbl[i].symbol );
	      i++ );

    	if ( i != sizeofTbl )
    	    return	( tbl[i].strVal );
    }
    else
    {
	for ( i = 0; 
	      i < sizeofTbl &&
	      ! STequal( &(curVar.right_side[1]), tbl[i].strVal );
	      i++ );

	if ( i != sizeofTbl )
	    return	( tbl[i].strVal );
    }

    str = ( II_CHAR * ) ait_malloc ( MAX_STR_LEN );
    str_assgn( curVar, str );

    return ( str );
}






/*
** Name: str_assgn() - II_CHAR *  value assigning function
**
** Description:
**	This function assigns the II_CHAR *  value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static VOID	
str_assgn
( 
    struct _varRec 	curVar, 
    char 		*str 
)
{
    CALLTRACKLIST	*ptr;
    i4			fieldNum;
    char		*index;

    if ( STequal( curVar.right_side, "NULL" ) )
    {
	str = NULL;

	return;
    }

    if ( curVar.right_side[0] == '"' )
    {
        MEcopy(&(curVar.right_side[1]),STlength(&(curVar.right_side[1])),str);
    	return;
    };

    index = STchr( curVar.right_side, '.' );

    if ( index != NULL )
    {
       	ptr = find_fun_num(curVar.right_side );
       	fieldNum = find_field_num( curVar.right_side, ptr->funNum );
       	STcopy( strval_assgn ( ptr, fieldNum ), str );

	return;
    }

    AIT_TRACE( AIT_TR_ERROR )
	( "[line %d]: %s expects a string as value.\n",
	  cur_line_num, curVar.left_side);

    SIprintf( "[line %d]: %s expects a string as value.\n",
	      cur_line_num, curVar.left_side);

    ait_exit();
}






/*
** Name: bool_assgn() - II_BOOL value assigning function
**
** Description:
**	This function assigns the II_BOOL value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static II_BOOL
bool_assgn
( 
    struct _varRec curVar 
)
{
    CALLTRACKLIST	*ptr;
    i4			fieldNum;
    II_BOOL		val;
    char		*index;

    if (
	 STequal( curVar.right_side, "TRUE" ) ||
	 STequal( curVar.right_side, "1" )
       )
	return	( TRUE );

    if (
	 STequal( curVar.right_side, "FALSE" ) ||
	 STequal( curVar.right_side, "0" )
       )
	return	( FALSE );

    index = STchr( curVar.right_side, '.' );

    if ( index != NULL )
    {
	ptr = find_fun_num( curVar.right_side );
	fieldNum = find_field_num( curVar.right_side, ptr->funNum );
	val = boolval_assgn(ptr, fieldNum );
        
	return	( val );
    }

    AIT_TRACE( AIT_TR_ERROR )
	( "[line %d]: incorrect value %s.\n",
	  cur_line_num, curVar.right_side );

    SIprintf( "[line %d]: incorrect value %s.\n",
	      cur_line_num, curVar.right_side );

    ait_exit();
}






/*
** Name: long_assgn() - II_LONG value assigning function
**
** Description:
**	This function assigns the II_LONG value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	30-Apr-2003 (bonro01)
**	    Modified for 64-bit platforms where 'long' integer is 8 bytes.
**	14-Mar-03 (gordy)
**	    Changed type to quite compiler warning.
**	18-Oct-04 (gordy)
**	    II_LONG is always 4 bytes, never 8 bytes.
*/

static II_LONG
long_assgn
(  
    struct _varRec curVar 
)
{
    STATUS	status;
    i4		i;
    char	*index;

    status = CVal( curVar.right_side, &i );

    if ( status == OK )
	return ( ( II_LONG )i );

    index = STchr( curVar.right_side, '.' );
    if ( index != NULL )
    {
	i = bind_longvar( curVar.right_side );

	return ( ( II_LONG )i );
    }

    AIT_TRACE( AIT_TR_ERROR )
	( "[line %d]: incorrect value %s.\n",
	  cur_line_num, curVar.right_side );

    SIprintf( "[line %d]: incorrect value %s.\n",
	     cur_line_num, curVar.right_side );

    ait_exit();
}






/*
** Name: int_assgn() - II_INT value assigning function
**
** Description:
**	This function assigns the II_INT value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static II_INT
int_assgn
( 
    struct _varRec curVar 
)
{
    STATUS	status;
    i4		i;
    char	*index;

    status = CVan( curVar.right_side, &i );

    if ( status == OK )
	return	( ( II_INT)i );

    index = STchr( curVar.right_side, '.' );

    if ( index != NULL )
    {
	i = (i4) bind_longvar( curVar.right_side );

	return ( ( II_INT)i );
    }
     
    AIT_TRACE( AIT_TR_ERROR )
	( "[line %d]: incorrect value %s.\n",
	  cur_line_num, curVar.right_side );

    SIprintf( "[line %d]: incorrect value %s.\n",
	      cur_line_num, curVar.right_side );

    ait_exit();
}






/*
** Name: float_assgn() - f8 value assigning function
**
** Description:
**	This function assigns the f8 value represented by the 
**	right_hand string of curVar to the buffer represented by the 
**	left_hand variable name.
**
** Inputs:
**	curVar - current variable pair whose left_hand is the variable 
**		 name string and right_hand is the value string
**
** Outputs:
** 	None.
** 
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static f8
float_assgn
( 
    struct _varRec curVar 
)
{
    STATUS	status;
    f8		*i;

    i = ( f8 * )ait_malloc( sizeof( f8 ) );

    status = CVaf( curVar.right_side, '.', i );

    if ( status == CV_UNDERFLOW )
	AIT_TRACE( AIT_TR_ERROR )( "underflow\n" );

    if ( status == CV_OVERFLOW )
	AIT_TRACE( AIT_TR_ERROR )( "overflow\n" );

    if ( status == CV_SYNTAX )
	AIT_TRACE( AIT_TR_ERROR )( "syntax" );

    if ( status != OK )
    {
    	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: incorrect value %s.\n",
	      cur_line_num, curVar.right_side );

    	SIprintf( "[line %d]: incorrect value %s.\n",
		  cur_line_num, curVar.right_side );

	ait_exit();
    }

    return ( *i );
}






/*
** Name: descriptor_assgn() - IIAPI_DESCRIPTOR field parsing function
**
** Description:
**	This function parses the left_hand descriptor field name and 
**	right_hand descriptor field value of curDscrpt into descriptor 
**	buffer.
**
** Inputs:
**	curDscrpt - variable pair whose left_hand is a descriptor field 
**		    name in the format of ds_xxxx and right_hand is the 
**		    string representing the value of this field
**
** Outputs:
**	descrptPtr - descriptor buffer
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	27-Jul-95 (feige)
**	    Support syntax ds_length = sizeof( $shell_variable );
**	    
*/

static VOID
descriptor_assgn
( 
    struct _varRec 	curDscrpt, 
    IIAPI_DESCRIPTOR 	*dscrptPtr 
)
{
    char	*r_side;

    if ( STequal( curDscrpt.left_side, "ds_dataType") )
    {
	dscrptPtr->ds_dataType = ( II_INT)symbolLong_assgn( curDscrpt,
							    DT_ID_TBL,
							    MAX_DT_ID );

	return;
    }

    if ( STequal( curDscrpt.left_side, "ds_nullable" ) )
    {
	dscrptPtr->ds_nullable = bool_assgn( curDscrpt );

	return;
     }

    if ( STequal( curDscrpt.left_side, "ds_length" ) )
    {
 	if ( curDscrpt.right_side[0] != '"' )
	    dscrptPtr->ds_length = long_assgn( curDscrpt );
	else
	    dscrptPtr->ds_length
		= STlength( ( char * )&curDscrpt.right_side[1] );

	return;
    }
    
    if ( STequal( curDscrpt.left_side, "ds_precision" ) )
    {
	dscrptPtr->ds_precision = long_assgn( curDscrpt );

	return;
    }
    
    if ( STequal( curDscrpt.left_side, "ds_scale" ) )
    {
	dscrptPtr->ds_scale = long_assgn( curDscrpt );

	return;
    }
    
    if ( STequal( curDscrpt.left_side, "ds_columnType" ) )
    {
	dscrptPtr->ds_columnType = symbolLong_assgn( curDscrpt,
						     ds_columnType_TBL,
						     MAX_ds_columnType );

	return;
    }

    if ( STequal( curDscrpt.left_side, "ds_columnName" ) )
    {
	if ( curDscrpt.right_side[0] == '\"' )
	    dscrptPtr->ds_columnName = ( II_CHAR * )
		ait_malloc( STlength( curDscrpt.right_side ) + 1 );
	else
	    dscrptPtr->ds_columnName = ( II_CHAR * )ait_malloc( MAX_STR_LEN );

	str_assgn( curDscrpt,	dscrptPtr->ds_columnName );

    	return;
    }

    AIT_TRACE( AIT_TR_ERROR )
     	( "[line %d]: unrecognized field %s of IIAPI_DESCRIPTOR.\n",
	  cur_line_num, curDscrpt.left_side );

    SIprintf
        ( "[line %d]: unrecognized field %s of IIAPI_DESCRIPTOR.\n",
	  cur_line_num, curDscrpt.left_side );

    ait_exit();
}






/*
** Name: datavalue_assgn() - IIAPI_DATAVALUE field parsing function
**
** Description:
**	This function parses the left_hand datavalue field name and 
**	right_hand string of field value of curDscrpt into descriptor 
**	buffer.
**
** Inputs:
**	curDataval - variable pair whose left_hand is a datavalue 
**		     datatype name and right_hand is the string representing 
**		     the value
**
** Outputs:
**	datavalPtr - descriptor buffer
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	10-May-95 (feige)
**	    Updated TINYINT, SMALLINT, DECIMAL and MONEY assignment.
**	    
*/

static VOID
dataval_assgn
( 
    struct _varRec 	curDataval, 
    IIAPI_DATAVALUE 	*datavalPtr 
)
{
    IIAPI_CONVERTPARM	cv;
    i4			i;
    i4  		len;
    i4			prec = 0;
    i4			scale = 0;
    char		*vptr;

    /*
    ** NULL value
    */

    if ( STequal( curDataval.left_side, "NULL" ) )
    {
	datavalPtr->dv_null = TRUE;

	return;
    }
    
    /*
    ** TINYINT value
    */
    if ( STequal( curDataval.left_side, "TINYINT") )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = TINYINT_SIZE;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( TINYINT_SIZE );
	*( II_INT1 * )datavalPtr->dv_value = ( II_INT1 )int_assgn( curDataval );

      	return;
    }	
    
    /*
    ** SMALLINT value
    */
    if ( STequal( curDataval.left_side, "SMALLINT" ) )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = SMALLINT_SIZE;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( SMALLINT_SIZE );
	*( II_INT2 * )datavalPtr->dv_value = ( II_INT2 )int_assgn( curDataval );

	return;
    }

    /*
    ** INTEGER value
    */
    if ( STequal( curDataval.left_side, "INTEGER" ) )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = INTEGER_SIZE;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( INTEGER_SIZE );
	*( i4 * )datavalPtr->dv_value = int_assgn( curDataval );

	return;
    }

    /*
    ** REAL value
    */
    if ( STequal( curDataval.left_side, "REAL" ) )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = REAL_SIZE;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( sizeof( f4 ) );
	*( f4 * )datavalPtr->dv_value = (f4)float_assgn( curDataval );

	return;
    }

    /*
    ** FLOAT value
    */
    if ( STequal( curDataval.left_side, "FLOAT" ) )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = FLOAT_SIZE;
	datavalPtr->dv_value = ( II_PTR ) ait_malloc( sizeof( f8 ) );
	*( f8 * )datavalPtr->dv_value = float_assgn( curDataval );

	return;
    }

    /*
    ** CHAR value
    */
    if ( STequal( curDataval.left_side, "CHAR" ) )
    {
	datavalPtr->dv_null = FALSE;
	/* 
	** remember the 1st char in right_side is " 
	*/
	if ( curDataval.right_side[0] == '\"' )
	    len = STlength( curDataval.right_side ) - 1;
	else
	    len = MAX_STR_LEN;

	datavalPtr->dv_length = len;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( len );
	str_assgn( curDataval, ( char * )datavalPtr->dv_value );

	return;
     }

     /*
     ** DECIMAL value
     */
     if ( STequal( curDataval.left_side, "DECIMAL" ) )
     {
	cv.cv_srcDesc.ds_dataType = IIAPI_CHA_TYPE;
	cv.cv_srcDesc.ds_nullable = FALSE;
	cv.cv_srcDesc.ds_length = STlength( curDataval.right_side );
	cv.cv_srcDesc.ds_precision = 0;
	cv.cv_srcDesc.ds_scale = 0;
	cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_srcDesc.ds_columnName = NULL;

	cv.cv_srcValue.dv_null = FALSE;
	cv.cv_srcValue.dv_length = cv.cv_srcDesc.ds_length;
	cv.cv_srcValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_srcValue.dv_length );

	MEcopy( ( PTR )curDataval.right_side,
		cv.cv_srcDesc.ds_length,
		( PTR )cv.cv_srcValue.dv_value
	      );

	vptr = curDataval.right_side;

	while ( *vptr != EOS )
	    if ( ( *vptr >= '0' && *vptr <= '9' ) ||
		 *vptr == '.' )
		break;
	    else
		vptr++;

	if ( *vptr == EOS )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side );

	    SIprintf
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side );

	    ait_exit();
	}

	while ( *vptr >= '0' && *vptr <= '9' && ++prec && ++vptr );

	if ( *vptr == '.'  )
	    while ( ++vptr && 
		    *vptr >= '0' && *vptr <= '9' && 
		    ++prec && ++scale );

	if ( *vptr != EOS )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side );

	    SIprintf
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side[1] );

	    ait_exit();
	}

	len = DB_PREC_TO_LEN_MACRO( prec );

	cv.cv_dstDesc.ds_dataType = IIAPI_DEC_TYPE;
	cv.cv_dstDesc.ds_nullable = FALSE;
	cv.cv_dstDesc.ds_length = len;
	cv.cv_dstDesc.ds_precision = prec;
	cv.cv_dstDesc.ds_scale = scale;
	cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_dstDesc.ds_columnName = NULL;

	cv.cv_dstValue.dv_null = FALSE;
	cv.cv_dstValue.dv_length = cv.cv_dstDesc.ds_length;
	cv.cv_dstValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_dstValue.dv_length );

	IIapi_convertData( &cv );

	if ( cv.cv_status != IIAPI_ST_SUCCESS )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side );

	    SIprintf
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, curDataval.right_side );

	    ait_exit();
	}

	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = cv.cv_dstValue.dv_length;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( datavalPtr->dv_length );
	MEcopy( cv.cv_dstValue.dv_value, 
		datavalPtr->dv_length, 
		datavalPtr->dv_value );	

	return;
     }

    /*
    ** DATE value
    */
    if ( STequal(curDataval.left_side, "DATE" ) )
    {
	/*
	** Remember the very 1st char is '"' to
	** indicate current value is a string.
	*/
	if ( curDataval.right_side[0] != '"' )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: DATE expects a string as value\n",
		  cur_line_num );

	    SIprintf
		( "[line %d]: DATE expects a string as value\n",
		  cur_line_num );

	    ait_exit();
	}
	cv.cv_srcDesc.ds_dataType = IIAPI_CHA_TYPE;
	cv.cv_srcDesc.ds_nullable = FALSE;
	cv.cv_srcDesc.ds_length = STlength( curDataval.right_side ) - 1;
	cv.cv_srcDesc.ds_precision = 0;
	cv.cv_srcDesc.ds_scale = 0;
	cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_srcDesc.ds_columnName = NULL;

	cv.cv_srcValue.dv_null = FALSE;
	cv.cv_srcValue.dv_length = cv.cv_srcDesc.ds_length;
	cv.cv_srcValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_srcValue.dv_length );
	MEcopy( curDataval.right_side + 1,
		cv.cv_srcDesc.ds_length,
		cv.cv_srcValue.dv_value
	      );

	cv.cv_dstDesc.ds_dataType = IIAPI_DTE_TYPE;
	cv.cv_dstDesc.ds_length = 12;
	cv.cv_dstDesc.ds_precision = 0;
	cv.cv_dstDesc.ds_scale = 0;
	cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_dstDesc.ds_columnName = NULL;

	cv.cv_dstValue.dv_null = FALSE;
	cv.cv_dstValue.dv_length = cv.cv_dstDesc.ds_length;
	cv.cv_dstValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_dstValue.dv_length );

	IIapi_convertData( &cv );

	if ( cv.cv_status != IIAPI_ST_SUCCESS )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, ( char * )( curDataval.right_side + 1 ) );

	    SIprintf
		( "[line %d]: %s is not a legal DECIMAL value.\n",
		  cur_line_num, &( curDataval.right_side[1] ) );

	    ait_exit();
	}

	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = cv.cv_dstValue.dv_length;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( datavalPtr->dv_length );
	MEcopy( cv.cv_dstValue.dv_value, 
		datavalPtr->dv_length, 
		datavalPtr->dv_value );	

	return;
    }
    
    /*
    ** MONEY value
    */
    if ( STequal(curDataval.left_side, "MONEY" ) )
    {

	cv.cv_srcDesc.ds_dataType = IIAPI_CHA_TYPE;
	cv.cv_srcDesc.ds_nullable = FALSE;
	cv.cv_srcDesc.ds_length = STlength( curDataval.right_side);
	cv.cv_srcDesc.ds_precision = 0;
	cv.cv_srcDesc.ds_scale = 0;
	cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_srcDesc.ds_columnName = NULL;

	cv.cv_srcValue.dv_null = FALSE;
	cv.cv_srcValue.dv_length = cv.cv_srcDesc.ds_length;
	cv.cv_srcValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_srcValue.dv_length );
	MEcopy( curDataval.right_side,
		cv.cv_srcDesc.ds_length,
		cv.cv_srcValue.dv_value
	      );

	cv.cv_dstDesc.ds_dataType = IIAPI_MNY_TYPE;
	cv.cv_dstDesc.ds_nullable = FALSE;
	cv.cv_dstDesc.ds_length = 8;
	cv.cv_dstDesc.ds_precision = 0;
	cv.cv_dstDesc.ds_scale = 0;
	cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
	cv.cv_dstDesc.ds_columnName = NULL;

	cv.cv_dstValue.dv_null = FALSE;
	cv.cv_dstValue.dv_length = cv.cv_dstDesc.ds_length;
	cv.cv_dstValue.dv_value = ( II_PTR )
	    ait_malloc( cv.cv_dstValue.dv_length );

	IIapi_convertData( &cv );

	if ( cv.cv_status != IIAPI_ST_SUCCESS )
	{
	    AIT_TRACE( AIT_TR_ERROR )
		( "[line %d]: %s is not a legal MONEY value.\n",
		  cur_line_num, curDataval.right_side );

	    SIprintf
		( "[line %d]: %s is not a legal MONEY value.\n",
		  cur_line_num, curDataval.right_side );

	    ait_exit();
	}

	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = cv.cv_dstValue.dv_length;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( datavalPtr->dv_length );
	MEcopy( cv.cv_dstValue.dv_value, 
		datavalPtr->dv_length, 
		datavalPtr->dv_value );	

	return;
    }

    /*
    ** VARCHAR value
    */
    if ( STequal( curDataval.left_side, "VARCHAR" ) )
    {
	char	*str;

	datavalPtr->dv_null = FALSE;
	if ( curDataval.right_side[0] == '\"' )
	    len = STlength( curDataval.right_side ) - 1;
	else
	    len = MAX_STR_LEN;
	datavalPtr->dv_length = len + 2;
	datavalPtr->dv_value = ( II_PTR )ait_malloc( datavalPtr->dv_length );

	str = ( char * )ait_malloc( len + 1 );
	str_assgn( curDataval, str );
	*( ( II_INT2 * )datavalPtr->dv_value ) = ( II_INT2 )len;
	MEcopy( str, len, ( char * )datavalPtr->dv_value + 2 );
	MEfree( str );

	return;
     }

    if ( STequal( curDataval.left_side, "HANDLE" ) )
    {
	datavalPtr->dv_null = FALSE;
	datavalPtr->dv_length = sizeof ( II_PTR );
 	datavalPtr->dv_value = ( II_PTR )ait_malloc( sizeof( II_PTR ) );
	*( II_PTR * )datavalPtr->dv_value = hndl_assgn( curDataval );

	return;
    }
        

    /*  LONGVARCHAR ? */
    
    AIT_TRACE( AIT_TR_ERROR )
     	( "[line %d]: unrecognized datatype %s of IIAPI_DATAVALUE.\n",
	  cur_line_num, curDataval.left_side );
    
    SIprintf
     	( "[line %d]: unrecognized datatype %s of IIAPI_DATAVALUE.\n",
	  cur_line_num, curDataval.left_side );
    
    ait_exit();
}






/*
** Name: bind_longvar() - long variable referencing function
**
** Description:
**	This function resolves the long value of a variable of name 
**	varname in the format of xxxx.xxxx.
** 
** Inputs:
**	varname - variable name
** 
** Outputs:
**	None.
**
** returns:
**	value of type II_LONG
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	20-Mar-95 (feige)
**	    Add return ( val ).
**	    
*/

static II_LONG
bind_longvar 
( 
    char *varname 
)
{
    CALLTRACKLIST	*ptr;
    i4			fieldNum;
    II_LONG		val;

    ptr = find_fun_num( varname );
    fieldNum = find_field_num( varname, ptr->funNum );
    val = longval_assgn( ptr, fieldNum );

    return ( val );
}






/*
** Name: find_fun_num() - function number searching function
**
** Description:
**	This function searches the callTrackList for the node whose 
**	variable name is the same as that to the left of dot-sign(.) 
**	in varname.
**
** Inputs:
**	varname	- variable name whose legal format is xxxx.xxxx
**
** Outputs:
**	None.
**
** returns:
**	a node on callTrackList with the matched variable name
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static CALLTRACKLIST *
find_fun_num
( 
    char *varname 
)
{
    CALLTRACKLIST	*callptr, *foundnode = NULL;
    char		*index, *var;
    i4			len;

    index = STchr( varname, '.' );

    if ( index == NULL )
	return ( NULL );

    len = index - varname + 1;
    var = ( char * ) ait_malloc ( len );

    MEcopy( varname, (len-1), var );  
    *(var+len-1) = EOS;

    callptr = callTrackList;

    while ( callptr != NULL )
    {
	if ( STequal( callptr->parmBlockName, var ) )
	{
	    foundnode = callptr;
	}

	callptr = callptr->next;
    }

    if ( foundnode == NULL )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: unrecognized variable %s in %s.\n",
	      cur_line_num,
	      var,
	      varname );

	SIprintf( "[line %d]: unrecognized variable %s in %s.\n",
		  cur_line_num,
		  var,
		  varname );

	ait_exit();
     }

    return ( foundnode );
}






/*
** Name: find_field_num() - variable field number searching function
**
** Description:
**	This function searches for the field number in funTBL[funNum] 
**	whose filed name is the same as the substring to the right of 
**	the dod-sign(.) in varname.
**
** Inputs:
**	varName - the variable name whose legal format is xxxx.xxxx
**	funNum  - the function number which is also the index into funTBL
**
** Outputs:
**	None.
**
** returns:
**	the field number which is also the index into funTBL[funNum]
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static i4
find_field_num
( 
    char 	*varname, 
    i4  	funNum 
)
{
    char	*index;
    i4		i;

    index = STchr( varname, '.' );

    if ( index == NULL )
	return ( -1 );

    index++;

    for ( i = 0; 
	  i < MAX_FIELD_NUM &&
	  ! STequal( funTBL[funNum].parmName[i], index ); 
	  i++ );

    if ( i == MAX_FIELD_NUM )
    {
	AIT_TRACE( AIT_TR_ERROR )
	    ( "[line %d]: illegal field %s in %s.\n",
	      cur_line_num, index, varname );

	SIprintf( "[line %d]: illegal field %s in %s.\n",
		  cur_line_num, index, varname );

  	ait_exit();
    }

    return ( i );
}






/*
** Name: longval_assgn() - long value searching function
**
** Description:
**	This function resolves the II_LONG value for a field indicated 
**	by fieldNum on a callTrackList node pointed by ptr, exits 
**	if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of II_LONG which is the value of the field 
**	indicated by ptr and fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	28-Apr-95 (feige)
**	    Removed ce_eventTime and ce_eventValueCount
**	    of IIapi_catchEvent from long valur assignment.
**	26-Feb-96 (feige)
**	    Supported gm_copyMap.cp_dbmsCount which is in the format
**	    of var.cp_dbmsCount, where var is of type IIAPI_GETCOPYMAPPARM.
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
*/

static II_LONG
longval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_connect:
	    switch( fieldNum )
	    {
		case	_co_timeout:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_timeout );

		case	_co_sizeAdvise:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_sizeAdvise );

		case	_co_apiLevel:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_apiLevel );

		case	_co_type:
		    return
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_type );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_getColumns:
	    switch( fieldNum )
	    {
		case	_gc_rowCount:
		    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_rowCount);

		case	_gc_columnCount:
		    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_columnCount );

		case	_gc_rowsReturned:
		    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_rowsReturned );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_getCopyMap:
            switch( fieldNum )
            {
		case 	_gm_copyMap_cp_dbmsCount:
		    return
			( ( ( IIAPI_GETCOPYMAPPARM * )
			    ptr->parmBlockPtr)->gm_copyMap.cp_dbmsCount );

		default:
                    print_err( ptr->parmBlockName,
                               funTBL[ptr->funNum].parmName[fieldNum],
                               "II_INT or II_LONG" );

                    ait_exit();
            }

	case	_IIapi_getDescriptor:
	    switch( fieldNum )
	    {
		case	_gd_descriptorCount:
		    return 
			( ( ( IIAPI_GETDESCRPARM * )
			    ptr->parmBlockPtr)->gd_descriptorCount );
		
		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_getQueryInfo:
	    switch( fieldNum )
	    {
		case	_gq_flags:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_flags );

		case	_gq_mask:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_mask );

		case	_gq_rowCount:
		    return 
		    	( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_rowCount );

		case	_gq_procedureReturn:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_procedureReturn );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_getEvent:
	    switch( fieldNum )
	    {
		case	_gv_timeout:
		   return 
			( ( ( IIAPI_GETEVENTPARM * )
			    ptr->parmBlockPtr)->gv_timeout );
	  
		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_initialize:
	    switch( fieldNum )
	    {
		case	_in_timeout:
		   return 
			( ( ( IIAPI_INITPARM * )
			    ptr->parmBlockPtr)->in_timeout );
	  
		case	_in_version:
		   return 
			( ( ( IIAPI_INITPARM * )
			    ptr->parmBlockPtr)->in_version );
	  
		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_putColumns:
	    switch( fieldNum )
	    {
		case	_pc_columnCount:
		    return 
			( ( ( IIAPI_PUTCOLPARM * )
			    ptr->parmBlockPtr)->pc_columnCount );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_putParms:
	    switch( fieldNum )
	    {
		case	_pp_parmCount:
		    return 
			( ( ( IIAPI_PUTPARMPARM * )
			    ptr->parmBlockPtr)->pp_parmCount );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_query:
	    switch( fieldNum )
	    {
		case	_qy_queryType:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_queryType );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_QUERYTYPE" );

		    ait_exit();
	    }

	case	_IIapi_setConnectParam:
	    switch( fieldNum )
	    {
		case	_sc_paramID:
		    return 
		    	( ( ( IIAPI_SETCONPRMPARM * )
			    ptr->parmBlockPtr)->sc_paramID );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_setEnvParam:
	    switch( fieldNum )
	    {
		case	_se_paramID:
		    return 
		    	( ( ( IIAPI_SETENVPRMPARM * )
			    ptr->parmBlockPtr)->se_paramID );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	case	_IIapi_setDescriptor:
	    switch( fieldNum )
	    {
		case	_sd_descriptorCount:
		    return 
			( ( ( IIAPI_SETDESCRPARM * )
			    ptr->parmBlockPtr)->sd_descriptorCount );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_INT or II_LONG" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       funTBL[ptr->funNum].parmName[fieldNum],
		       "II_INT or II_LONG" );

	    ait_exit();
    }
}






/*
** Name: ptrval_assgn() - II_PTR value searching function
**
** Description:
**	This function resolves the II_PTR value for a field indicated 
**	by fieldNum on a callTrackList node pointed by ptr, exits 
**	if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of II_PTR which is the value of the field 
**	indicated by ptr and fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	 9-Jul-96 (gordy)
**	    Added IIapi_autocommit().
**	16-Jul-96 (gordy)
**	    Added IIapi_getEvent().
**	19-Aug-98 (rajus01)
**	    Added IIapi_abort(), IIapi_formatData().
*/

static II_PTR
ptrval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_abort:
	    switch( fieldNum )
	    {
		case	_ab_connHandle:
		    return 
			( ( ( IIAPI_ABORTPARM * )
			    ptr->parmBlockPtr)->ab_connHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }
	case	_IIapi_autocommit:
	    switch( fieldNum )
	    {
		case	_ac_connHandle:
		    return 
			( ( ( IIAPI_AUTOPARM * )
			    ptr->parmBlockPtr)->ac_connHandle );

		case	_ac_tranHandle:
		    return 
			( ( ( IIAPI_AUTOPARM * )
			    ptr->parmBlockPtr)->ac_tranHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_cancel:
	    switch( fieldNum )
	    {
		case	_cn_stmtHandle:
		    return 
			( ( ( IIAPI_CANCELPARM * )
			    ptr->parmBlockPtr)->cn_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_catchEvent:
	    switch( fieldNum )
	    {
		case	_ce_connHandle:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_connHandle );

		case	_ce_eventHandle:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_eventHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_formatData:
	    switch( fieldNum )
	    {

		case	_fd_envHandle:
		    return 
			( ( ( IIAPI_FORMATPARM * )
			    ptr->parmBlockPtr)->fd_envHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_close:
	    switch( fieldNum )
	    {
		case	_cl_stmtHandle:
		    return 
			( ( ( IIAPI_CLOSEPARM * )
			    ptr->parmBlockPtr)->cl_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_commit:
	    switch( fieldNum )
	    {
		case	_cm_tranHandle:
		    return 
			( ( ( IIAPI_COMMITPARM * )
			    ptr->parmBlockPtr)->cm_tranHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_connect:
	    switch( fieldNum )
	    {
		case	_co_connHandle:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_connHandle );

		case	_co_tranHandle:
		    return 
			( ( ( IIAPI_CONNPARM * )
			     ptr->parmBlockPtr)->co_tranHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_disconnect:
	    switch( fieldNum )
	    {
		case	_dc_connHandle:
		    return	
			( ( ( IIAPI_DISCONNPARM * )
			    ptr->parmBlockPtr)->dc_connHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_getColumns:
	    switch( fieldNum )
	    {
		case	_gc_stmtHandle:
		    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_getCopyMap:
	    switch( fieldNum )
	    {
		case	_gm_stmtHandle:
		    return 
			( ( ( IIAPI_GETCOPYMAPPARM * )
			    ptr->parmBlockPtr)->gm_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_getDescriptor:
	    switch( fieldNum )
	    {
		case	_gd_stmtHandle:
		    return 
			( ( ( IIAPI_GETDESCRPARM * )
			    ptr->parmBlockPtr)->gd_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_getQueryInfo:
	    switch( fieldNum )
	    {
		case	_gq_stmtHandle:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_stmtHandle );

		case	_gq_procedureHandle:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_procedureHandle );

		case	_gq_repeatQueryHandle:
		    return 
			( ( ( IIAPI_GETQINFOPARM * )
			   ptr->parmBlockPtr)->gq_repeatQueryHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_getEvent:
	    switch( fieldNum )
	    {
		case	_gv_connHandle:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_connHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_initialize:
	    switch( fieldNum )
	    {
		case	_in_envHandle:
		    return 
			( ( ( IIAPI_INITPARM * )
			    ptr->parmBlockPtr)->in_envHandle );
		default:
		    print_err( ptr->parmBlockName,
			      funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_prepareCommit:
	    switch( fieldNum )
	    {
		case	_pr_tranHandle:
		    return 
			( ( ( IIAPI_PREPCMTPARM * )
			    ptr->parmBlockPtr)->pr_tranHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_putColumns:
	    switch( fieldNum )
	    {
		case	_pc_stmtHandle:
		    return 
			( ( ( IIAPI_PUTCOLPARM * )
			    ptr->parmBlockPtr)->pc_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_putParms:
	    switch( fieldNum )
	    {
		case	_pp_stmtHandle:
		    return 
			( ( ( IIAPI_PUTPARMPARM * )
			    ptr->parmBlockPtr)->pp_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_query:
	    switch( fieldNum )
	    {
		case	_qy_connHandle:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_connHandle );

		case	_qy_tranHandle:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_tranHandle );

		case	_qy_stmtHandle:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			      funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_registerXID:
	    switch( fieldNum )
	    {
		case	_rg_tranIdHandle:
		    return 
			( ( ( IIAPI_REGXIDPARM * )
			    ptr->parmBlockPtr)->rg_tranIdHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_releaseEnv:
	    switch( fieldNum )
	    {
		case	_re_envHandle:
		    return 
			( ( ( IIAPI_RELENVPARM * )
			    ptr->parmBlockPtr)->re_envHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_releaseXID:
	    switch( fieldNum )
	    {
		case	_rl_tranIdHandle:
		    return 
			( ( ( IIAPI_RELXIDPARM * )
			    ptr->parmBlockPtr)->rl_tranIdHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_rollback:
	    switch( fieldNum )
	    {
		case	_rb_tranHandle:
		    return 
			( ( ( IIAPI_ROLLBACKPARM * )
			    ptr->parmBlockPtr)->rb_tranHandle );

		case	_rb_savePointHandle:
		    return ( ( ( IIAPI_ROLLBACKPARM * )
			ptr->parmBlockPtr)->rb_savePointHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_savePoint:
	    switch( fieldNum )
	    {
		case	_sp_tranHandle:
		    return 
			( ( ( IIAPI_SAVEPTPARM * )
			    ptr->parmBlockPtr)->sp_tranHandle );

		case	_sp_savePointHandle:
		    return 
			( ( ( IIAPI_SAVEPTPARM * )
			    ptr->parmBlockPtr)->sp_savePointHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_setConnectParam:
	    switch( fieldNum )
	    {
		case	_sc_connHandle:
		    return 
			( ( ( IIAPI_SETCONPRMPARM * )
			    ptr->parmBlockPtr)->sc_connHandle );

		case	_sc_paramValue:
		    return 
			( ( ( IIAPI_SETCONPRMPARM * )
			    ptr->parmBlockPtr)->sc_paramValue );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_setEnvParam:
	    switch( fieldNum )
	    {
		case	_se_envHandle:
		    return 
			( ( ( IIAPI_SETENVPRMPARM * )
			    ptr->parmBlockPtr)->se_envHandle );

		case	_se_paramValue:
		    return 
			( ( ( IIAPI_SETENVPRMPARM * )
			    ptr->parmBlockPtr)->se_paramValue );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	case	_IIapi_setDescriptor:
	    switch( fieldNum )
	    {
		case	_sd_stmtHandle:
		    return 
			( ( ( IIAPI_SETDESCRPARM * )
			    ptr->parmBlockPtr)->sd_stmtHandle );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_PTR" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       funTBL[ptr->funNum].parmName[fieldNum],
		       "II_PTR" );

	    ait_exit();
    }
}






/*
** Name: strval_assgn() - II_CHAR * value searching function
**
** Description:
**	This function resolves the II_PTR value for a field indicated 
**	by fieldNum on a callTrackList node pointed by ptr, exits 
**	if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of II_CHAR * which is the value of the field 
**	indicated by ptr and fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static II_CHAR	*
strval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_catchEvent:
	    switch( fieldNum )
	    {
		case	_ce_selectEventName:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_selectEventName );

		case	_ce_selectEventOwner:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_selectEventOwner );

		case	_ce_eventName:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_eventName );

		case	_ce_eventOwner:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_eventOwner );

		case	_ce_eventDB:
		    return 
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_eventDB );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_CHAR *" );

		    ait_exit();
	    }

	case	_IIapi_connect:
	    switch( fieldNum )
	    {
		case	_co_target:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_target );

		case	_co_username:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_username );

		case	_co_password:
		    return 
			( ( ( IIAPI_CONNPARM * )
			    ptr->parmBlockPtr)->co_password );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_CHAR *" );

		    ait_exit();
	    }

	case	_IIapi_query:
	    switch( fieldNum )
	    {
		case	_qy_queryText:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_queryText );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_CHAR *" );

		    ait_exit();
	    }

	case	_IIapi_savePoint:
	    switch( fieldNum )
	    {
		case	_sp_savePoint:
		    return 
			( ( ( IIAPI_SAVEPTPARM * )
			    ptr->parmBlockPtr)->sp_savePoint );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_CHAR *" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       	funTBL[ptr->funNum].parmName[fieldNum],
		       "II_CHAR	*" );

	    ait_exit();
    }
}






/*
** Name: boolval_assgn() - II_BOOL value searching function
**
** Description:
**	This function resolves the II_PTR value for a field indicated 
**	by fieldNum on a callTrackList node pointed by ptr, exits 
**	if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of II_BOOL which is the value of the field 
**	indicated by ptr and fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	28-Apr-95 (feige)
**	    Added ce_eventInfoAvail for IIAPI_CATCHEVENTPARM. 
*/

static II_BOOL
boolval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_catchEvent:
	    switch( fieldNum )
	    {
		case	_ce_eventInfoAvail:
		    return
			( ( ( IIAPI_CATCHEVENTPARM * )
			    ptr->parmBlockPtr)->ce_eventInfoAvail );
                default:
                    print_err( ptr->parmBlockName,
                               funTBL[ptr->funNum].parmName[fieldNum],
                               "II_BOOL" );

                    ait_exit();
	    }


	case	_IIapi_getColumns:
	    switch( fieldNum )
	    {
	    	case	_gc_moreSegments:
	    	    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_moreSegments );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_BOOL" );

		    ait_exit();
	    }

	case	_IIapi_getQueryInfo:
	    switch( fieldNum )
	    {
		case	_gq_readonly:
		    return	
			( ( ( IIAPI_GETQINFOPARM * )
			    ptr->parmBlockPtr)->gq_readonly );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_BOOL" );

		    ait_exit();
	    }

	case	_IIapi_putColumns:
	    switch( fieldNum )
	    {
		case	_pc_moreSegments:
		    return 
			( ( ( IIAPI_PUTCOLPARM * )
			    ptr->parmBlockPtr)->pc_moreSegments );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_BOOL" );

		    ait_exit();
	    }

	case	_IIapi_putParms:
	    switch( fieldNum )
	    {
		case	_pp_moreSegments:
		    return 
			( ( ( IIAPI_PUTPARMPARM * )
			    ptr->parmBlockPtr)->pp_moreSegments );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_BOOL" );

		    ait_exit();
	    }

	case	_IIapi_query:
	    switch( fieldNum )
	    {
		case	_qy_parameters:
		    return 
			( ( ( IIAPI_QUERYPARM * )
			    ptr->parmBlockPtr)->qy_parameters );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "II_BOOL" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       funTBL[ptr->funNum].parmName[fieldNum],
		       "II_BOOL" );

	    ait_exit();
    }
}






/*
** Name: datavalueval_assgn() - IIAPI_DATAVALUE value searching function
**
** Description:
**	This function resolves the IIAPI_DATAVALUE value for a field 
**	indicated by fieldNum on a callTrackList node pointed by ptr, 
**	exits if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of IIAPI_DATAVALUE which is the value of the field 
**	indicated by ptr fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static IIAPI_DATAVALUE	*
datavalueval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_getColumns:
	    switch( fieldNum )
	    {
		case	_gc_columnData:
		    return 
			( ( ( IIAPI_GETCOLPARM * )
			    ptr->parmBlockPtr)->gc_columnData );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DATAVALUE" );

		    ait_exit();
	    }

	case	_IIapi_putColumns:
	    switch( fieldNum )
	    {
		case	_pc_columnData:
		    return 
			( ( ( IIAPI_PUTCOLPARM * )
			    ptr->parmBlockPtr)->pc_columnData );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DATAVALUE" );

		    ait_exit();
	    }
	case	_IIapi_formatData:

	    switch( fieldNum )
	    {
		case	_fd_dstValue:
		{
		    IIAPI_FORMATPARM *frmt = 
		     ( IIAPI_FORMATPARM * ) ptr->parmBlockPtr;

		    return ( & ( frmt->fd_dstValue ) );
		}

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DATAVALUE" );

		    ait_exit();
	    }


	case	_IIapi_putParms:
	    switch( fieldNum )
	    {
		case	_pp_parmData:
		    return 
			( ( ( IIAPI_PUTPARMPARM * )
			    ptr->parmBlockPtr)->pp_parmData );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DATAVALUE" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       funTBL[ptr->funNum].parmName[fieldNum],
		       "IIAPI_DATAVALUE" );

	    ait_exit();
    }
}






/*
** Name: descrval_assgn() - IIAPI_DESCRIPTOR value searching function
**
** Description:
**	This function resolves the IIAPI_DESCRIPTOR value for a field 
**	indicated by fieldNum on a callTrackList node pointed by ptr, 
**	exits if not found.
**
** Inputs:
**	ptr      - a pointer which points to a node on callTrackList
**	fieldNum - the field number
**
** Outputs:
**	None.
**
** returns:
**	value of IIAPI_DESCRIPTOR which is the value of the field 
**	indicated by ptr fieldNum
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	26-Feb-96 (feige)
**	    Supported gm_copyMap.cp_dbmsDescr which is in the format
**	    of var.cp_dbmsCount, where var is of type IIAPI_GETCOPYMAPPARM.
**	    
*/

static IIAPI_DESCRIPTOR	*
descrval_assgn( CALLTRACKLIST *ptr, i4  fieldNum )
{
    switch( ptr->funNum )
    {
	case	_IIapi_getCopyMap:
	    switch( fieldNum )
	    {
		case	_gm_copyMap_cp_dbmsDescr:
		    return
			( ( ( IIAPI_GETCOPYMAPPARM * )
			    ptr->parmBlockPtr)->gm_copyMap.cp_dbmsDescr );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DESCRIPTOR" );

		    ait_exit();
	    }

	case	_IIapi_getDescriptor:
	    switch( fieldNum )
	    {
		case	_gd_descriptor:
		    return 
			( ( ( IIAPI_GETDESCRPARM * )
			    ptr->parmBlockPtr)->gd_descriptor );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DESCRIPTOR" );

		    ait_exit();
	    }

	case	_IIapi_setDescriptor:
	    switch( fieldNum )
	    {
		case	_sd_descriptor:
		    return 
			( ( ( IIAPI_SETDESCRPARM * )
			    ptr->parmBlockPtr)->sd_descriptor );

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DESCRIPTOR" );

		    ait_exit();
	    }

	case	_IIapi_formatData:

	    switch( fieldNum )
	    {
		case	_fd_dstDesc:
		{
		    IIAPI_FORMATPARM *frmt = 
		     ( IIAPI_FORMATPARM * ) ptr->parmBlockPtr;

		    return ( & ( frmt->fd_dstDesc ) );
		}

		default:
		    print_err( ptr->parmBlockName,
			       funTBL[ptr->funNum].parmName[fieldNum],
			       "IIAPI_DESCRIPTOR" );

		    ait_exit();
	    }

	default:
	    print_err( ptr->parmBlockName,
		       funTBL[ptr->funNum].parmName[fieldNum],
		       "IIAPI_DESCRIPTOR" );

	    ait_exit();
    }
}






/*
** Name: print_err() - error printing function
**
** Description:
** 	This function print error message of a variable is not of the 
**	correct type.
**
** Inputs:
**	varname   - variable name
**	fieldname - field name
**	typename  - type name
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
** 	    Created.
**	    
*/

static VOID
print_err
( 
    char  	*varname, 
    char 	*fieldname, 
    char 	*typename 
)
{
    AIT_TRACE( AIT_TR_ERROR )
	( "[line %d]: %s.%s is not of type %s.\n",
	  cur_line_num,
	  varname,
	  fieldname,
	  typename
	 );

    SIprintf
	( "[line %d]: %s.%s is not of type %s.\n",
    	  cur_line_num,
      	  varname,
      	  fieldname,
	  typename
	);

    return;
}

static II_VOID
funcptr_assgn
(
    struct _varRec      curVar,
    struct _strfunctbl   *tbl,
    i4                  sizeofTbl,
    II_PTR		**func
)
{
    i4  	i;

    for ( i = 0;
	  i < sizeofTbl && ! STequal( curVar.right_side, tbl[i].symbol );
	  i++ );

    if ( i != sizeofTbl )
    {
	*func = (II_PTR) tbl[i].func ;
	return;
    }

    AIT_TRACE( AIT_TR_ERROR )
    ( "[line %d]: Expects one of the following:\\\n", cur_line_num );
    AIT_TRACE( AIT_TR_ERROR )
    ( "APItest_event_func | APItest_prompt_func | APItest_trace_func\n" );

    SIprintf( "[line %d]: Expects one of the following.\\\n", cur_line_num );
    SIprintf( "APItest_event_func | APItest_prompt_func | APItest_trace_func\n" );

    ait_exit();
}


/*
** Name: formatData_srcVal() - get 'fd_srcValue' for IIapi_formatData().
**
** Description:
**	This input buffer to the function carries the string of
**	format 'xxxx.yyyyyyy;'. It truncates the semicolon and
**	uses this string to search the callTrackList for the node
**	whose variable name is the same as the xxxx  and uses the
**	matched node to get the fieldname yyyyyyy and assigns the
**	data value to the current variable.
**
** Inputs:
**	line_buf  - string whose value is of the format 'xxxxxx.yyyyyyy;'
**
** Outputs:
**	None.
**
** returns:
**	None.
**
** History:
**    11-Feb-99 (rajus01)
**	Created.
*/
VOID
formatData_srcVal( char * line_buf )
{
    CALLTRACKLIST             *ptr;
    i4                        fieldnum, len;
    IIAPI_DATAVALUE           *val;
    IIAPI_FORMATPARM          *frmt;

    frmt = ( IIAPI_FORMATPARM * )curCallPtr->parmBlockPtr;

    len = STlength(line_buf );
    line_buf[ len - 1] = EOS;

    ptr = find_fun_num ( line_buf );
    fieldnum = find_field_num( line_buf, ptr->funNum );
    val = datavalueval_assgn( ptr, fieldnum );

    if( val && val->dv_length )
    {
       frmt->fd_srcValue.dv_null   = val->dv_null; 
       frmt->fd_srcValue.dv_length = val->dv_length;
       frmt->fd_srcValue.dv_value  = ( II_PTR )ait_malloc( val->dv_length );
       MEcopy( ( PTR )val->dv_value, val->dv_length,
					( PTR )frmt->fd_srcValue.dv_value );
    }
    else
    {
	AIT_TRACE( AIT_TR_ERROR )
	( "formatData_srcVal: Unable to obtain data value.\n" );

        SIprintf( "formatData_srcVal: Unable to obtain data value.\n" );

	ait_exit();
    }
}

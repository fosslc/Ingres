/*
** Copyright (c) 2004 Ingres Corporation
*/





# include	<compat.h>
# include	<lo.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<iiapi.h>
# include	"aitfunc.h"
# include	"aitdef.h"
# include	"aitmisc.h"
# include	"aitproc.h"
# include	"aitparse.h"





/*
** Name:	aitmisc.	- AIT miscellanous functions
**
** Description:
**	This file defines the common functions shared by modules 
**	internal of API tester.
**
**	ait_malloc		Allocate memory.
**	ait_openfile		Open file
**	ait_exit		Exit the API tester
**	free_callTrackList	Free the memory of callTrackList
**	free_dscrpList		Free the memory of descriptorList
**	free_datavalList	Free the memory of dataValList
**	ait_initTrace		Initialize the tracing
**	ait_termTrace		Shutdown tracing
**	ait_isTrace		Check tracing level
**	ait_nopDisplay		Do nothing when tracing disabled
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	27-Mar-95 (gordy)
**	    Reworked global variables.
**	31-Mar-95 (gordy)
**	    Converted output to use messaging function
**	    in control block.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/






/*
** Name: ait_malloc - Allocate memory
**
** Description:
**      This function allocates a buffer and ait_exits if the allocation
**      fails.
**
** Inputs:
**      size      size of the buffer to be allocated.
**
** Outputs:
**	None.
**
** Returns:
**	a pointer of type II_PTR pointing to a bufferof size size.	
**
** History:
**	21-Feb-95 (feige)
**          Created.
**	31-Mar-95 (gordy)
**	    Converted output to use messaging function
**	    in control block.
*/

FUNC_EXTERN II_PTR
ait_malloc( i4  size )
{
    PTR		retValue;
    STATUS	status;
    char	str[ 64 ];

    retValue = MEreqmem( 0, size, TRUE, &status );

    if ( status != OK )
    {
	STprintf( str, "ERROR: can't allocate memory of size %d\n", size );
        AIT_TRACE( AIT_TR_ERROR )( str );
	(*ait_cb.msg)( str );

	ait_exit();
    }
    
    return( retValue );
}






/*
** Name: ait_openfile() - file openning function	
**
** Description:
**	This function performs file openning operation, ait_exit if failed.
**
** Inputs:
** 	filename	- file name
**	mode		- openning mode ("r", "w", etc.)
**
** Outputs:
**	fptrptr		- pointer to FILE pointer
**
** Returns:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
**	31-Mar-95 (gordy)
**	    Converted output to use messaging function
**	    in control block.
*/

FUNC_EXTERN bool
ait_openfile( char *filename, char *mode, FILE **fptrptr )
{
    STATUS      open_file_status;
    LOCATION    open_file_loc;
    char	name[ MAX_LOC + 1 ];
    char	str[ 256 ];

    STcopy( filename, name );
    open_file_status = LOfroms( PATH & FILENAME, name, &open_file_loc );

    if ( open_file_status != OK )
    {
        STprintf( str, "Illegal filename: %s\n", filename );
        AIT_TRACE( AIT_TR_ERROR )( str );
        (*ait_cb.msg)( str );

	return( FALSE );
    }

    open_file_status =  SIfopen( &open_file_loc, mode, 
    				 SI_TXT, FILE_REC_LEN, fptrptr );

    if ( open_file_status != OK )
    {
        STprintf( str, "Fail to open file %s\n", name );
        AIT_TRACE( AIT_TR_ERROR )( str );
        (*ait_cb.msg)( str );

	return( FALSE );
    }

    if ( SIisopen( *fptrptr ) != TRUE )
    {
	STprintf( str, "File %s is not opened.\n", name );
	AIT_TRACE( AIT_TR_ERROR )( str );
	(*ait_cb.msg)( str );

	return( FALSE );
    }

    return( TRUE );
}






/*
** Name: ait_closefile() - file closing function	
**
** Description:
**	This function performs file closing operation.
**
** Inputs:
**	fptr		FILE pointer
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

FUNC_EXTERN VOID
ait_closefile
( 
    FILE *fptr 
)
{
    if ( fptr  &&  SIisopen( fptr ) ) 
	SIclose( fptr );

    return;
}






/*
** Name: ait_exit() - shutdown API tester	
**
** Description:
**	This function frees internal buffers before shutdown.
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

FUNC_EXTERN VOID
ait_exit( )
{
    if ( inputf )   
    {
        ait_closefile( inputf );
	inputf = NULL;
    }

    if ( outputf )  
    {
        ait_closefile( outputf );
	outputf = NULL;
    }

    free_calltrackList( );
    free_dscrpList( );
    free_datavalList( );
    
    exit( 1 );
}






/*
** Name: free_callTrackList() - memory free function	
**
** Description:
**	This function frees the the linked list callTrackList upon exiting 
**	API tester.
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

FUNC_EXTERN VOID
free_calltrackList( )
{
    CALLTRACKLIST *cptr1, *cptr2;

    cptr1 = callTrackList;

    while ( cptr1 != NULL )
    {
	cptr2 = cptr1->next;
	MEfree( cptr1->parmBlockName );
	MEfree( cptr1->parmBlockPtr );
	MEfree( (PTR)cptr1 );
	cptr1 = cptr2;
    }

    callTrackList = NULL;
    
    return;
}






/*
** Name: free_dscrpList() - memory free function	
**
** Description:
**	This function frees the the linked list dscrpList upon exiting 
**	API tester.
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
**	    Free array of descriptors. 
*/

FUNC_EXTERN VOID
free_dscrpList( )
{
    struct _descriptorList      *node1, *node2;
    i4				i;

    for( i = 0; i < descriptorCount; i++ )
    {
	for( node1 = descriptorList[i]; node1; node1 = node2 )
	{
	    node2 = node1->next;
	    MEfree( (PTR)node1 );
	}

	descriptorList[i] = NULL;
    }

    descriptorCount = 0;

    return;
}






/*
** Name: free_datavalList() - memory free function	
**
** Description:
**	This function frees the the linked list dataValList upon exiting 
**	API tester.
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

FUNC_EXTERN VOID
free_datavalList( )
{
    struct _dataValList      *node1, *node2;

    node1 = dataValList;

    while ( node1 != NULL )
    {
        node2 = node1->next;
	MEfree( node1->valBuf.left_side );
	MEfree( node1->valBuf.right_side );
        MEfree( (PTR)node1 );
        node1 = node2;
    }

    dataValList = NULL;

    return;
}






/*
** Name: ait_initTrace() - Initialize tracing
**
** Description:
**	This function initializes the tracing capability in API tester.
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
**	31-Mar-95 (gordy)
**	    Use TR file interface rather than terminal interface.
*/

FUNC_EXTERN VOID
ait_initTrace( char *tracefile )
{
    CL_ERR_DESC	err_code;
    
    TRset_file( TR_F_OPEN, tracefile, STlength( tracefile ), &err_code );	

    return;
}






/*
** Name: ait_termTrace() - Terminate tracing
**
** Description:
**	This function turns off the tracing capability in API tester.
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
**	31-Mar-95 (gordy)
**	    Use TR file interface rather than terminal interface.
*/

FUNC_EXTERN VOID
ait_termTrace( VOID )
{
    CL_ERR_DESC	err_code;

    TRset_file( TR_F_CLOSE, NULL, 0, &err_code );
    traceLevel = 0;

    return;
}






/*
** Name: ait_isTrace - Is trace level set
**
** Description:
**	This function returns TRUE if the specified trae level is set, 
**	FALSE otherwise.
**
** Inputs:
**	currentTrace	current tracing level
**
** Outputs:
**	None.
**
** History:
**	21-Feb-95 (feige)
**	    Created.
*/

FUNC_EXTERN bool
ait_isTrace( i4  currentTrace )
{
    return( currentTrace <= traceLevel );
}



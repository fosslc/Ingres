/*
** Copyright (c) 2004 Ingres Corporation
*/


#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>				 
#include    <lo.h>
#include    <nm.h>
#include    <si.h>

#include    <iicommon.h>
#include    <gcu.h>


/*
**
**  Name: gcucset.c
**
**  Description:
**	Provides routines for reading the GCF character set definition
**	files.
**
**	gcu_read_cset		Read gcccset.nam.
**    
**  History:
**	26-Dec-02 (gordy)
**	    Extracted from GCC.
**	27-Jul-2010 (frima01) Bug 124137
**	    Evaluate return code of NMloc to avoid using corrupt pointers.
*/



/*
** Name: gcu_read_cset
**
** Description:
**	Reads the II_SYSTEM/ingres/files/charsets/gcccset.nam file, 
**	which maps between character set names and their u_i2 ids.  
**	The format of this file is:
**
**		# comments
**		<name> <id>
**
**	where <name> is the (case insensitive) name of the character set,
**	and id is a u_i2 in hex.
**
**	Each entry is returned via a callback routine.
**
**		bool callback( char *name, u_i4 id );
**
**	The callback routine should return TRUE to continue to the next
**	entry and FALSE to stop processing.
**
**	Returns an indication of the final processing state.  Zero is
**	returned when the entire file has been processed successfully.
**	A positive value indicates an error in the file syntax and
**	provides the line number of the error.  A negative value is
**	returned when processing is stopped because the callback
**	routine returned FALSE.
**
**	This routine succeeds if the gcccset.nam file doesn't exist, to 
**	provide for backward compatibility.
**
** Input:
**	callback	Function pointer to callback routine.
**
** Output:
**	None.
**
** Returns:
**	i4		Processing state:
**			  < 0	Processing terminated by callback.
**			  = 0	Entire file processed successfully.
**			  > 0	Line number of syntax error.
**
** History:
**	26-Dec-02 (gordy)
**	    Extracted from GCC.
*/

FUNC_EXTERN i4
gcu_read_cset( bool (*callback)(char *, u_i4) )
{
    LOCATION		file_loc;
    FILE		*file_ptr;
    char		linebuf[ 256 ];
    i4			lineno = 0;		/* line number for errors */

    /*
    ** Open gcccset.nam in $II_SYSTEM/files/charsets.
    */
    if (NMloc( FILES, 0, (char *)NULL, &file_loc ) != OK )
	return (1); /* return 1 as number of line on which error occurred */
    LOfaddpath( &file_loc, "charsets", &file_loc );
    LOfstfile( "gcccset.nam", &file_loc );

    if ( SIopen( &file_loc, "r", &file_ptr ) != OK )  return( 0 );

    /*
    ** Process each line until EOF.
    */
    while( SIgetrec( linebuf, sizeof( linebuf ), file_ptr ) != ENDFILE )
    {
	char		*name, *ptr;
	u_i4		char_id;
	char		*p = linebuf;

	lineno++;
	if ( linebuf[0] == '#' )  continue;	/* Skip comment line */
	while( CMwhite( p ) )  p++;		/* Find start of name */
	for( name = p; *p && !CMwhite( p ); p++ ) ;	/* Scan name */
	if ( p == name )  continue;		/* Skip blank line */
	*p++ = '\0';				/* Terminate name */
	while( CMwhite( p ) )  p++;		/* Find start of ID */
	for( ptr = p; CMhex( p ); p++ ) ;	/* Scan ID */
	*p++ = '\0';				/* Terminate ID */

	if ( CVahxl( ptr, &char_id ) != OK )
	{
	    /*
	    ** Syntax error in file.
	    */
	    SIclose( file_ptr );
	    return( lineno );
	}

	/*
	** Pass entry to callback routine.
	*/
	if ( ! (*callback)( name, char_id ) )
	{
	    /*
	    ** Termination requested.
	    */
	    SIclose( file_ptr );
	    return( -1 );
	}
    }

    /*
    ** We be done!
    */
    SIclose( file_ptr );
    return( 0 );
}


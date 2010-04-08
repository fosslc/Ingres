/*
** Copyright (c) 2004 Ingres Corporation 
*/
# include       <compat.h>
# include       <si.h>
# include       <st.h>
# include       <me.h>
# include       <ex.h>
# include       <er.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <pc.h>
# include       <lo.h>
# include       <erst.h>
# include 	<simacro.h>

/*
**  Name: iireadresp.c -- Contains functions for reading value from a response file.
**
**  Functions
**	
**	  main() - Main enterance for the read_response utility. 	
** 
**  To Do:
**
**	Error reporting should be based more like this
**		 ERROR( ERget( S_ST0518_no_definition ) ); 
**       ERROR( "MEreqmem() failed." );
**
**  History:
**      18-oct-2001 (gupsh01)
**              Created.
**	30-nov-2001 (devjo01)
**		Add support for "<default>" tag.
*/

/*
PROGRAM =       iiread_response
**
NEEDLIBS =      UTILLIB COMPATLIB MALLOCLIB
**
UNDEFS =        II_copyright
**
DEST =          utility
*/ 

#define MAXWIDTH 255

/*
** Name: read_response_value - Read a parameter value from the response file.
**		 main entry point for the read_response_value code.
**
**		usage:
**		iiread_response parameter_name response_file
**		
**		It is assumed that the user has permissions to open the response file.
**
**		 
*/
main(i4 argc, char *argv[])
{	
    LOCATION    file_location;
    char        loc_string[MAX_LOC + 1];
    char        *buff;
    char	*inparam = argv[1];
    FILE        *responseFile=NULL;
    i4		size;
    char	*param_name = 0;
    char	*param_value = 0;
    char        *value;
    i4		val_length = 0;
    i4		name_length = 0;
    STATUS	status = OK;
    char  	equals[] = "=";
	
    /* Use ingres allocator for memory allocation */
    MEadvise ( ME_INGRES_ALLOC );

    /* scan the input parameters for legal input values */
    if (argc < 3)
    {
	PRINT( "Error: incorrect arguments supplied. \n" ); 
	ERROR( "Usage: iiread_response parameter response_file" );
    }
	
    STlcopy(argv[2], loc_string, sizeof(loc_string));
    LOfroms(PATH & FILENAME, loc_string, &file_location);

    /* Open the response file for read */
    if ( SIfopen( &file_location, ERx("r"), SI_TXT, 
		SI_MAX_TXT_REC, &responseFile ) != OK )
	F_ERROR( "ERROR: Unable to open file %s\n", argv[2] );
  
    /* initialize the buffer first */
    size = MAXWIDTH;

    buff = (char *)MEreqmem(0, size, TRUE, NULL );
    if (!buff)
	ERROR( "ERROR: MEreqmem() failed." );
	
    while ((status = SIgetrec(buff, size, responseFile)) == OK)
    {
	i4 buflen = STlength(buff);
	if (*(buff + buflen - 1) != '\n')
	{
	    /* 
	    ** It is possible that we did not get the whole line in 
	    ** the first pass so keep reading before we process.
	    ** We have determined that we need a bigger buffer limit
	    ** thus we are going to increase size of buffer.
	    ** we have also increased the limit of size we are going 
	    ** read now.
	    */

	    char *temp = 0;
	    size += MAXWIDTH;
	    temp = (char *)MEreqmem(0, size, TRUE, NULL );
            if (!temp)
	    {
        	PRINT( "ERROR: MEreqmem() failed. \n" );
		status = FAIL;
		break;
            }

	    /* copy the data to the larger buffer */
	    MEcopy(buff, STlength(buff), temp);
	    MEfree(buff);
	    buff = temp;
	    continue;
	}

	/* screen the parameter based on first character */
	if (*buff != *inparam) 
	    continue;
	else if ((value = STindex (buff, equals, 0)) != NULL)
	{
	    val_length =  STlength(value) - 1;
	    name_length = STlength(buff) - val_length - 1;
		
	    /* get the name parameter */
	    param_name = (char *)MEreqmem(0, name_length, TRUE, NULL);

	    if (!param_name)
            {  
		PRINT ( "ERROR: MEreqmem() failed. \n" );
		status = FAIL;
	    }
	    MEcopy(buff, name_length, param_name); 
	    STzapblank (param_name, param_name); 
	    STzapblank (inparam, inparam);

	    if (STcompare (param_name, inparam) == 0)
	    {
	        /* get the value for the parameter */
		param_value = (char *)MEreqmem(0, val_length, TRUE, NULL);
		if (!param_value)
		{
		     PRINT ( "ERROR: MEreqmem() failed. \n" );
                     status = FAIL;
		     break;	
		}
		MEcopy(value + 1, val_length - 1, param_value);
		status = OK;
		break;
	    }				
	}
	else 
	{
	    PRINT("ERROR: Illegal record in response file \n");
	    status = FAIL;
	    break;
	}		
    }

    if ( status != OK && status != ENDFILE)
    {
	PRINT("ERROR: Cannot get record from the file \n");
	status = FAIL;		
    }
    else 
	status = OK;

    /* 
    ** Return the response_param if found.
    ** This is a stand alone utility so we will 
    ** just print the results and cleanup.
    **
    ** If value is the default tag, then behave as if
    ** no value was provided.
    */

    if (param_value && STcompare (param_value, "<default>")) 
    {
	F_PRINT( "%s\n", param_value );
    }
	
    /* cleanup */
    if (param_name) 
	MEfree(param_name);

    if (param_value) 
	MEfree(param_value);

    if (buff) 
	MEfree(buff);
	
    SIclose(responseFile);

    if ( status == FAIL )
	PCexit(FAIL);
    else
	PCexit(OK);
}

 

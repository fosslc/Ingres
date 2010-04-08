/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<si.h>


/**
** Name:	ugappfil.c - Append one file to another
**
** Description:
**	IIUGafAppendFile	Append one file to another
**
** History:
**	3/16/90 (Mike S) Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*{
** Name:	IIUGafAppendFile        Append one file to another
**
** Description:
**	Append a file to a base file, with an optional string separating
**	the two.  The appended file is unchanged.  If the file to be appended
**	doesn't exist, no action is taken, and success is returned.  If the 
**	base file doesn't exist, the result is the same as if it existed but 
**	was empty: the result is the separator followed by the appended file.
**
** Inputs:
**	base		LOCATION *	the base file
**	to_append	LOCATION *	the file to append
**	separator	char *		a string to separate the two 
**					(may be NULL)
**
** Outputs:
**	none	
**
**	Returns:
**		STATUS		OK if all went well
**		other		if there was an error opening or closing
**				the base file, or appending the other
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	3/16/90	(Mike S)	Initial version
*/
STATUS
IIUGafAppendFile(base, to_append, separator)
LOCATION *base;
LOCATION *to_append;
char    *separator;
{
        FILE *base_file; 
        LOINFORMATION loinf;         
        i4  loflags = 0;         
        STATUS status;         
         
        /* Check that to_append file exists */ 
        if (LOinfo(to_append, &loflags, &loinf) != OK) 
                return OK; 
                 
        /* Open base file */ 
         status = SIfopen(base, ERx("a"), SI_TXT, (i4)0, &base_file);
         if (status != OK)
                return status;  
         
        /* Add separator */ 
        if (separator != NULL && *separator != EOS) 
                SIfprintf(base_file, ERx("%s"), separator); 
          
        /* Append to_append file */          
        if ((status =  SIcat(to_append, base_file)) != OK)
        {        
                _VOID_ SIclose(base_file); 
                return (status);
        }        
         
        return (SIclose(base_file)); 
}  

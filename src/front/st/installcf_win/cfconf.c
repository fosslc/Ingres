/*
** Copyright (C) 2000 Ingres Corporation
*/

/*
** Name: cfconf.c
**
** Description:
**      File contains the functions for manipulating the extension file.
**
** History:
**    11-Feb-2000 (fanra01)
**          Created.
*/

# include <compat.h>

# include "cfinst.h"

static char readbuf[MAX_READ_LINE + 1];

/*
** Name: CFBuildList
**
** Description:
**      Build a list from an array of character strings.
**
** Inputs:
**      section         pointer to array of character strings.
**
** Outputs:
**      begin           head of the list
**      end             tail of the list
**
** Returns:
**      OK              success
**      FAIL            failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFBuildList( char* section[], PRL* begin )
{
    STATUS  status = OK;
    PRL     newline;
    PRL     end;
    char**  sectline = section;
    i4      i;

    /*
    ** Assume the section is NULL terminated
    */
    for (i=0; (sectline && sectline[i]); i++)
    {
        /*
        ** Allocate a structure for the new line
        */
        HALLOC( newline, RL, 1, &status );
        if (newline != NULL)
        {
            /*
            ** Duplicate the line and add to end of list
            */
            newline->text = STRALLOC( sectline[i] );
            if (*begin == NULL)
            {
                *begin = newline;
                newline->prev = newline;
            }
            else
            {
                end = (*begin)->prev;
                end->next = newline;
                newline->prev = end;
                (*begin)->prev = newline;
            }
        }
    }
    return (status);
}

/*
**  Name: CFCreateConf
**
**  Description:
**      Create a configuration file.
**
**  Inputs:
**      path            path to configuration file.
**      filename        filename of configuration file.
**      begin           Starting line of the confiuration
**
**  Outputs:
**      loc             location structure updated.
**
**  Returns:
**      OK              success
**      FAIL            failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFCreateConf( char* path, char* filename, LOCATION* loc, PRL begin )
{
    STATUS  status;
    char*   locpath;
    FILE*   fptr;
    PRL     entry = begin;
    i4      count;

    /*
    ** Get the location path
    */
    LOtos( loc, &locpath );
    STRCOPY(path, locpath );
    if ((status = LOfroms( PATH, locpath, loc )) == OK)
    {
        LOfstfile( filename, loc );
        if ((status = SIfopen( loc, "w", SI_TXT, 0, &fptr )) == OK)
        {
            while ((status == OK) && (entry != NULL))
            {
                status = SIwrite( STRLENGTH(entry->text), entry->text, &count,
                    fptr );
                entry = entry->next;
            }
            SIclose( fptr );
        }
        else
        {
            PRINTF( "Error: %d - Unable to open file %s in %s.\n", status,
                filename, path );
        }
    }
    return (status);
}

/*
**  Name: CFReadConf
**
**  Description:
**      Read the configuration file a line at a time and build a list.
**
**  Inputs:
**      path            path of file to read
**      filename        filename of file to read
**
**  Outputs:
**      loc             Location to update with path and filename
**      begin           Starting line of the confiuration
**      end             Last line of the configuration
**
**  Returns:
**      OK      success
**      FAIL    failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFReadConf( char* path, char* filename, LOCATION* loc, PRL* begin )
{
    STATUS  status;
    char*   locpath;
    FILE*   fptr;
    PRL     newline;
    PRL     end;

    /*
    ** Get the location path
    */
    LOtos( loc, &locpath );
    STRCOPY( path, locpath );
    if ((status = LOfroms( PATH, locpath, loc )) == OK)
    {
        LOfstfile( filename, loc );
        if ((status = SIfopen( loc, "r", SI_TXT, 0, &fptr )) == OK)
        {
            do
            {
                status = SIgetrec( readbuf, SI_MAX_TXT_REC, fptr );
                if (status != ENDFILE)
                {
                    HALLOC( newline, RL, 1, &status );
                    if (newline != NULL)
                    {
                        newline->text = STRALLOC( readbuf );
                        if (*begin == NULL)
                        {
                            *begin = newline;
                            newline->prev = newline;
                        }
                        else
                        {
                            end = (*begin)->prev;
                            end->next = newline;
                            newline->prev = end;
                            (*begin)->prev = newline;
                        }
                    }
                }
            }while (status == OK);
            status = ((status == ENDFILE) || (status == OK)) ? OK : status;
            SIclose( fptr );
        }
        else
        {
            PRINTF( "Error: %d - Unable to open file %s in %s.\n", status,
                filename, path );
        }
    }
    return (status);
}

/*
**  Name: CFWriteConf
**
**  Description:
**      Write out the configuration file.
**
**  Inputs:
**      loc             location to write configuration
**      begin           starting line of configuration
**
**  Outputs:
**      none
**
**  Returns:
**      OK      success
**      FAIL    failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFWriteConf( LOCATION* loc, PRL begin )
{
    STATUS  status = OK;
    PRL     entry = begin;
    FILE*   fptr;
    i4      count;

    if ((status=SIfopen( loc, "w", SI_TXT, 0, &fptr )) == OK)
    {
        while (entry != NULL)
        {
            status = SIwrite( STRLENGTH( entry->text ), entry->text, &count,
                fptr );
            entry = entry->next;
        }
        SIclose( fptr );
    }
    else
    {
        char*   tmp = NULL;

        LOtos( loc, &tmp );
        PRINTF( "Error: %d - Unable to open file %s.\n", status, tmp );
    }
    return (status);
}

/*
**  Name: CFFreeConf
**
**  Description:
**      Free all the memory used to update the configuration.
**
** Inputs:
**      begin               Starting line of the configuration
**
** Outputs:
**      none
**
** Returns:
**      OK      success
**      FAIL    failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFFreeConf( PRL begin )
{
    STATUS  status = FAIL;
    PRL     entry;

    while (begin != NULL)
    {
        entry = begin;
        begin = entry->next;
        if ((status = HFREE( entry->text )) == OK)
        {
            status = HFREE( (PTR)entry );
        }
    }
    return (status);
}

/*
**  Name: CFBackupConf
**
**  Description:
**      Save the current configuration as a .bak file.
**
** Inputs:
**      loc                 Current location
**
** Outputs:
**      none
**
** Returns:
**      OK      success
**      FAIL    failure
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
STATUS
CFBackupConf( LOCATION* loc, PRL begin )
{
    STATUS status;
    char buffer[MAX_LOC + 1];
    char device[MAX_LOC + 1];
    char path[MAX_LOC + 1];
    char filename[MAX_LOC + 1];
    char extension[MAX_LOC + 1];
    char version[MAX_LOC + 1];
    LOCATION backup;

    LOcopy( loc, buffer, &backup );
    LOdetail( loc, device, path, filename, extension, version );
    LOcompose( device, path, filename, ERx( "bak" ), version, &backup );
    status = CFWriteConf( &backup, begin );
    return (status);
}

/*
**  Name: CFScanConf
**
**  Description:
**      Line-by-line scan of the configuration for search string.
**
** Inputs:
**      search              Search string
**      begin               Starting line of configuration
**      termblank           enables/disable terminate on blank line.
**
** Outputs:
**      none
**
** Returns:
**      pointer containing line
**      NULL    if not found
**
** History:
**      11-Feb-2000 (fanra01)
**          Created.
*/
PRL
CFScanConf( char* search, PRL begin, bool termblank )
{
    PRL entry = begin;

    while (entry != NULL)
    {
        if ((termblank == TRUE) &&
            ((*entry->text == '\0') || (*entry->text == '\n') ||
            (*entry->text ==  ';')))
        {
            return (NULL);
        }
        else
        {
            if (STRSTRINDEX( entry->text, search ) != NULL)
            {
                break;
            }
            entry = entry->next;
        }
    }
    return (entry);
}


# include       <compat.h>
# include       <gl.h>
# include       <st.h>
# include       <clconfig.h>
# include       <systypes.h>
# include       <ex.h>
# include       <lo.h>
# include       "lolocal.h"
# include       <diracc.h>
# include       <errno.h>


/*LOwcard
**
**      LO wildcard operations.
**
**      Defines procedures:
**              LOwcard         -       Start wildcard operation
**              LOwnext         -       Get next file
**              LOwend          -       End of wildcard operation
**
*/

# define WILD ERx("*")

/*{
** Name:        LOwcard         -       Start wildcard search
**
** Description:
**      Do a wildcard search of a directory.
**
** Inputs:
**      loc     LOCATION *      NODE and PATH to search.
**      fprefix char *          filename to look for; NULL to wildcard.
**      fsuffix char *          filetype to look for; NULL to wildcard.
**      version char *          version to look for; NULL to wildcard.
**
** Outputs:
**      loc     LOCATION *      First file in list
**      lc      LO_DIR_CONTEXT * Directory context for search
**
**      Returns:
**              STATUS  OK or failure.
**
**      Exceptions:
**              none.
**
** Side Effects:
**              None.
** History:
**      10/89   (Mike S)        Initial version.
**
**      6-mar-91 (fraser)
**          Rewritten for PMFE
**	13-may-91 (seg)
**		Added OS/2 code, shifted some invariant code around
**      18-Jul-95 (fanra01)
**              Added a FindNextFile call since a FindFirstFile in a directory
**              will return the current '.' and parent directories '..'.
**              the path is searched until a valid directory or file name is
**              found or nothing is found.
**	22-feb-1996 (canor01)
**		Clean up comments.
*/

STATUS
LOwcard(
LOCATION	*loc,
char		*fprefix,
char		*fsuffix,
char		*version,
LO_DIR_CONTEXT	*lc)
{
    char *  p;
    char    find_path[MAX_LOC];
    long count = 1;
    STATUS  status;

    if(!lc || !loc)
    	return FAIL;

    strcpy(find_path, loc->path);

    p = find_path + strlen(find_path);
    if (*(p - 1) != BACKSLASH)
    {
        *p++ = BACKSLASH;
	     *p   = EOS;
    }

    strcpy(lc->path, find_path);    /* Save for future use */

    if (fprefix == NULL)
    {
        *p++ = STAR;
    }
    else
    {
        strcpy(p, fprefix);
        p += strlen(fprefix);
    }

    *p++ = PERIOD;
    if (fsuffix == NULL)
    {
        *p++ = STAR;
	     *p = EOS;
    }
    else
    {
        strcpy(p, fsuffix);
    }

    if((lc->hdir = FindFirstFile(find_path, &lc->find_buffer)) == \
            INVALID_HANDLE_VALUE)
    {
        return(ENDFILE);
    }

    /* ignore . and .. directories (assumes that they will come
    ** at the start of the list and thus the ignore can be done
    ** here, and does not have to be done in LOwnext)
    */
    
    while (
            (strcmp(lc->find_buffer.cFileName,".") == 0) ||
            (strcmp(lc->find_buffer.cFileName,"..") == 0)
          )
    {
       if(FindNextFile(lc->hdir, &lc->find_buffer) == FALSE)
           return(ENDFILE);
    }

    strcpy(lc->buffer, lc->path);
    strcat(lc->buffer, lc->find_buffer.cFileName);

    status = LOfroms(PATH & FILENAME, lc->buffer, loc);
    return(status);
}

/*{
** Name:        LOwnext         -       Get next wildcard
**
** Description:
**      Get next file from wildcard search in a directory opened by LOwcard.
**
** Inputs:
**      lc      LO_DIR_CONTEXT * search context block containing
**		open directory pointer + pattern match information
**
** Outputs:
**      loc     LOCATION       * next location from search
**
**      Returns:
**              OK	if file found
**		ENDFILE if no more files
**		FAIL	other failure
**
**      Exceptions:
**              none
**
** Side Effects:
**
** History:
*/

STATUS
LOwnext(
LO_DIR_CONTEXT *lc,
LOCATION       *loc)
{
    STATUS status;
    long count = 1;

    if(!lc || !loc)
    	return FAIL;
    if(FindNextFile(lc->hdir, &lc->find_buffer) == FALSE)
        return(ENDFILE);
    strcpy(lc->buffer, lc->path);
    strcat(lc->buffer, lc->find_buffer.cFileName);

    status = LOfroms(PATH & FILENAME, lc->buffer, loc);
    return(status);
}

/*{
** Name:        LOwend  - End wildcard search
**
** Description:
**              Return system resources after a wildcard search is over.
**              This should always be called to clean up.
**
** Inputs:
**      lc      LO_DIR_CONTEXT *        Context structure
**
** Outputs:
**      none
**
**      Returns:
**              OK
**		FAIL
**
**      Exceptions:
**              none
**
** Side Effects:
**      System resources (memory and possibly an I/O channel) are returned.
*/

STATUS
LOwend(
LO_DIR_CONTEXT *lc)
{
    if(!lc)
    	return FAIL;
    FindClose(lc->hdir);
    return(OK);
}

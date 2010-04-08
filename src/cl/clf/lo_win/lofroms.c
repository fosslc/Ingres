/*
** Copyright (c) 1983 Ingres Corporation
*/
#include	<compat.h>
#include	<cm.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<st.h>


/*LOfroms.c
**	Convert a string to a LOCATION.
**	It accepts a ptr to an already allocated location and 
**	and allocated buffer where location string is stored.
**	This buffer will always be used by the location to store string.
**	Returns BAD_SYN if string is syntactically incorrect.
**	The Null string gives an empty LOCATION.
**	A flag must be sent telling LOfroms whether the string is a:
**		NODE,PATH,FILENAME,NODE & PATH, NODE & PATH & FILENAME,
**		NODE & FILENAME, PATH & FILENAME
**
**		History
**			03/09/83 -- (mmm)
**				written
**			04/08/85 -- (dreyfus) If a NODE is specified in the
**				flag arguement, ignore it. That is,
**				treat NODE & PATH like PATH.
**			Tue Nov 11 19:03:08 PST 1986 (daveb)
**				Make "//" in filenames legal.  Required for apollo.
**			02/23/89 (daveb, seng)
**				rename LO.h to lolocal.h
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>";
**		Do loc checks correctly without redundancy;
**		Clean up formatting to 80 columns.
**	
**	19-may-95 (emmag)
**	    Branched for NT.
**	06-may-1996 (canor01)
**	    Cleaned up compiler warnings.
**	17-dec-1997 (canor01)
**	    Allow UNC filenames.
**  23-Jun-2005 (fanra01)
**      Sir 114805
**      Add test and treatment of UNC path string as device.
*/


STATUS
LOfroms(flag,string,loc)
LOCTYPE			flag;
register char		*string;
register LOCATION	*loc;
{
    register i2	strsize;	/* strsize is the index to buffer
    				** it always gives count of current
    				** characters scanned
    				*/
    i2		count;
    STATUS		LOreadfname();
    STATUS		LOclear();
    char*   uncpath;
    char*   p;

    strsize = 0;

    /* check to see if passed arguments are non null */
    if ((string == NULL) || (loc == NULL))
    {
            if( loc != NULL )
    		LOclear(loc);
    	return(LO_NULL_ARG);
    }

    /* INIT */
    loc->node = NULL;
    loc->path = NULL;
    loc->fname = NULL;
    loc->desc = flag;
    loc->fp = NULL;
    loc->string = string;

    if( *string == EOS)
    {
            /* a null string returns an empty location */
            LOclear(loc);
            return(OK);
    }

    switch (flag) {

      case NODE:		/* NODE */

	    LOclear(loc);
	    return (FAIL);
	    break;

      case (NODE & PATH):	/* treat like PATH */

      case PATH:		/* PATH */

	    /* init loc fields */
	    loc->path = string;

        if (CMcmpnocase( string + CMbytecnt(string), COLONSTRING ) == 0)
        {
            CMbyteinc( strsize, string );
            CMnext(string);
            CMbyteinc( strsize, string );
            CMnext(string);
        }
        /*
        ** Scan the path string for the start of a UNC path.
        */
        if (((p = STstrindex( loc->path, UNCSTRING, 0, FALSE )) != NULL) &&
            (p == loc->path))
        {
            /*
            ** A path that starts with two slash '\' characters is assumed
            ** to be a UNC path of the format
            ** \\machine\nameddirectory\path. The resulting components
            ** are \\machine\nameddirectory and \path.
            **
            ** Bump the character count and the pointer to skip the UNC
            ** start characters.
            */
            strsize += (sizeof(UNCSTRING) - 1);
            p += (sizeof(UNCSTRING) - 1);
            /*
            ** Scan the path string for the second backslash character as the
            ** start of the directory list, skipping the machine and the
            ** named directory.
            */
            if (((uncpath = STindex( p, SLASHSTRING, 0 )) != NULL) &&
                ((uncpath = STindex( uncpath + CMbytecnt(uncpath),
                    SLASHSTRING, 0 )) != NULL))
            {
                strsize += uncpath - loc->path;
                /*
                ** Set string to the start of the directory list
                */
                string = uncpath;
            }
	    }

	    while (*string != EOS)
	    {
		count = 0;
		if (LOreadfname(string,&count) == OK)
		{
		    /* if a legal name in the path was scanned */
		    /* then append the name to the path */
		    string = string + count;
		    strsize = strsize + count;
		}
		else
		{
		    /* bad syntax encountered by LOreadfname */
		    LOclear(loc);
		    return(LO_FR_BAD_SYN);
		}
	    }

	    /* a legal path has been read from the string */
	    if ((*(string - 1) == SLASH) && (strsize > 1)
                && (*(string - 2) != COLON))
	    {
		/* paths other than root never end in a slash */
		string--;
		*(string) = EOS;
	    }

	    /* the file name will be appended here */
	    /* string is now a pointer to the null terminator of path name */
	    loc->fname = string;
	    break;

      case (NODE & PATH & FILENAME):

	    /* don't do anything for now with nodes */
	    /* treat like PATH & FILENAME */

      case (PATH & FILENAME):	  /* PATH & FILENAME */

	    loc->path = string;

        if (*(string + 1) == COLON)
        {
            /* if there is a device name scan the device name */
            string += 2;
            strsize += 2;
        }


        if (*string == '/')
        {
            LOclear(loc);
            return(LO_FR_BAD_SYN);
        }

        if (*string == SLASH)
        {
            /* if path begins at root scan SLASH */
            string++;
            strsize++;
            if (*string == SLASH)
            {
                /* UNC filename */
                string++;
                strsize++;
                if (((uncpath = STindex( string, SLASHSTRING, 0 )) != NULL) &&
                    ((uncpath = STindex( uncpath+1, SLASHSTRING, 0 )) != NULL))
                {
                    strsize += uncpath - string;
                    string = uncpath;
                }
            }
        }

	    if (*string == EOS)
	    {
		/* root is not a legal path and filename */
		LOclear(loc);
		return(LO_FR_BAD_SYN);
	    }

	    while (*string != EOS)
	    {
		count = 0;
		if (LOreadfname(string,&count) == OK)
		{
                    if (string[count] == EOS)
           	    {
                        /* an entire path has been scanned */
                        /* store path name */
                        if (string == loc->string)
                        {
                            /* this is really a filename with a  null path */
                            loc->path = NULL;
                        }

                        /* look at the filename */
                        if (*(string + count - 1) == SLASH)
                        {
                            /* filenames don't end in SLASH */
                            LOclear(loc);
                            return (LO_FR_BAD_SYN);
                        }
                        else
                        {
                            /* string is a pointer into the buffer */
                            /* string points to the beginning of file name */
                            loc->fname = string;
                            /* allow normal exit from  while loop */
                            string = string + count;
                        }
                    }
                    else
                    {
                        /* a legal name in the path was scanned */
                        /* append the name to the path */
                        string = string + count;
                        strsize = strsize + count;
                    }
                }
                else
                {
                    /* bad syntax encountered by LOreadfname */
                    LOclear(loc);
                    return (LO_FR_BAD_SYN);
                }
            }
            break;

      case (NODE & FILENAME):

	    /* don't do anything with nodes for now */
	    /* treat like FILENAME */

      case FILENAME:	      /* JUST FNAME */

	    loc->fname = string;

            if (*string == SLASH || *(string + 1) == COLON)
	    {
		/* fname never begins with a slash */
		LOclear(loc);
		return(LO_FR_BAD_SYN);
	    }
	    if (*string == EOS)
	    {
		/* this is a null filename */

		LOclear(loc);
		return(LO_FR_BAD_SYN);
	    }
	    if (LOreadfname(string,&count) == OK)
	    {
		/* if there is a legal fname in string */
		/* then store in buf */
		if (*(string + count - 1) == SLASH)
		{
		    /* loc->fname never ends with a slash */
		    LOclear(loc);
		    return(LO_FR_BAD_SYN);
		}
                if (string[count] != EOS)
                {
                    /* error - string contained more than a fname */
                    LOclear(loc);
                    return (LO_FR_BAD_SYN);
                }
	    }
	    else
	    {
		/* LOreadfname returned BAD_SYN */

		LOclear(loc);
		return(LO_FR_BAD_SYN);
	    }
	    /* a legal fname has been read from the string */

	    break;
    } // End of switch

    return(OK);
}
/*LOreadfname
**	input:	string-	a pointer to part of a location written in string form
**		count-	a pointer to an int which is used to pass number of 
**			characters in next name read from path string.
**
**	action:	LOreadfname reads from a string into "name" until either a "/",
**		EOS, or LO_NM_LEN is exceeded.  NOTE: this reflects knowledge
**		that LO_EXT_LEN is 0, ie. that extensions are included
**		in the length of a file name.
**
**	return: count-  upon return count is the number of characters in next
**			name.
**		string-	If successful string will point to as far down in the 
**			pathname as was read.
**		LOreadfname-
**			the procedure returns OK if successful (ie. a name was
**			read) or nonzero if failed (ie. name too long)
*/
STATUS
LOreadfname(string, count)
register char	*string;
register i2	*count;
{
	*count = 0;

# ifdef NO_NO_NO

	/* "//" is legal in a path, and required on apollo (daveb) */

	if (*string == SLASH)
	{
		/* this catches a null path name part (ie. /.../...//.../...) */
		return(LO_FR_BAD_SYN);
	}
# endif

	
	while ((*string != EOS) && (*string != SLASH || !count))
	{
		(*count)++;
		if (*count > LO_NM_LEN)
		{
			return(LO_FR_BAD_SYN);
		}
		string++;

	}

	if (*string == SLASH)
	{
		(*count)++;
		string++;
	}
	return(OK);

}

/*LOclear
**
**	Return a null location.
**
*/
STATUS
LOclear(loc)
LOCATION	*loc;
{
	/* initialize location to null */
	loc->node = NULL;
	loc->path = NULL;
	loc->fname = NULL;
	loc->desc = 0;
	return(OK);
}

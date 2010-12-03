/*
** Copyright (c) 2006, 2009 Ingres Corporation 
*/

# include "ingpwutil.h"
# include <string.h>
# include <unistd.h>
# include <stdlib.h>
# include <signal.h>
# include <ctype.h>
# include  <errno.h>

/*
** Name: ingpwutil.c
**
** Description: This file contains utility functions used by ingvalidpam
**	        and ingvalidpw progarms to provide user authentication 
**		in Ingres.	
** History:
**      16-May-2008 (rajus01) SIR 120420, SD issue 116370
**          Moved from ingvalidpw.x as these utility functions are common to 
**	    both ingvalidpw and ingvalidpam programs. Also did some code
**	    clean up around debugging.
**	17-Jul-2008 (bonro01)
**	    Compile fails on Solaris.
**	    All variable definitions must come before executable code.
**	28-Aug-09 (gordy)
**	    Password may be terminated with EOS as well as ' '.
**	09-Mar-2010 (hanje04)
**	    Don't define errno, locally.
**	29-Nov-2010 (frima01) SIR 124685
**	    Add int argument to handler() to match signal.h.
*/

int	p[2];

/*
** Name: mklogfile
**
** Description: open log file pointed to by II_INGVALIDPW_LOG
**		File is opened to check for accesiblility.
** Input:
**	ptr	-	full pathname of file to open
** Output: 
**      fp	- 	File ptr to the debug file	
** Returns:
**	E_BADLOG -   if error	
**	E_OK 	 -   upon successful completion.	
** History:
**	14-jan-1998(angusm)
**	    Created
**	20-feb-1998(angusm)
**	    Further changes after comments on security issues from AUSCERT
**	30-Jun-2008 (rajus01)
**	    Return error code upon error instead of exit.
*/
int
mklogfile( FILE **fp, char *ptr)
{
    char c;
    int status = E_BADLOG;

    *fp = NULL;
    if (pipe(p) < 0)
	return(status);
    (void)signal(SIGPIPE, handler);
    switch (fork())
    {
	case -1:
	   return(status);
	case 0: /* child process */
	{
	    close(p[1]);
	    if (setgid(getgid()) < 0 )
	       return(status);
	    if (setuid(getuid()) < 0)
	       return(status);
	    *fp = fopen(ptr, "a");
	    if( *fp == NULL )
	       return(status);
	    else
            {	
	        while (read(p[0], &c, 1) == 1)
	            fputc(c, *fp);
	        fclose(*fp);
	    }

    	    return(E_OK);
	}
	default: /* parent process */
	{
	    close(p[0]); 
	    *fp = fdopen(p[1], "a");
	    setvbuf(*fp, (char *)0, _IOLBF, BUFSIZ);
	    if(*fp == NULL)
	       return(status);
	}
    }

    return(E_OK);
}

/*
** Name: iscan
** Description: Extract username/password from input buffer in
**	        safer manner than 'sscanf'.
** History:
**	20-feb-1998(angusm)
**	Created.
**	28-Aug-09 (gordy)
**	    Password may be terminated with EOS as well as ' '.
*/
int
iscan(char *input, char *username, char *password)
{
	int	i, lim, buflim;
	char	*tptr = '\0', *tptr1 = '\0';

	tptr=input;
	tptr1=username;
	lim = USR_LEN; 
	buflim = INBUFLEN;

	for (i=0; i < lim; i++)
	{
		if (i == buflim)
		    return(E_BADINPUT);
		if ( (*tptr == 0) || (isspace(*tptr)) )
		    break;
		*tptr1++ = *tptr++;
	}	
	if ( (i == lim) && !(isspace(*tptr)) )
	    return(E_BADINPUT);
	*tptr1 = '\0';

	tptr1 = password;
	lim = PWD_LEN;
	buflim = INBUFLEN-i;
	*tptr++;
	for (i=0; i < lim; i++)
	{
		if (i == buflim)
			return(E_BADINPUT);
		if ( (*tptr == 0) || (isspace(*tptr)) )
		    break;
		*tptr1++ = *tptr++;
	}	
	if ( (i == lim) && (*tptr != 0) && !(isspace(*tptr)) )
	    return(E_BADINPUT);
	*tptr1 = '\0';
        return(E_OK);
}

void handler(int arg)
{
    exit(E_BADLOG);
}

/*
** Name: getUserAndPasswd
**
** Description: Reads username and passwd from the input.
** input: 
**	fp	- 	ptr to debug file
**	user    - 	username
** 	passwd  -	user password
** Output: 
**	user    - 	username
** 	passwd  -	user password
** Returns:
**	E_OK	    -	if no error
**	error code  - 	otherwise	
** History:
**	23-Jun-2008(rajus01)
**	    Created.
*/
int getUserAndPasswd( char *user, char *passwd, FILE *fp )
{
   char    input[INBUFLEN];
   int     cnt;

   if ( (cnt = read(0,input, sizeof(input)) ) < 0 )
   {
      INGAUTH_DEBUG_MACRO(fp,1,"ingpwutil: read failed errno %d\n",errno);
      return(E_NOUSER);
   }
   if ( iscan(input, user, passwd) != E_OK )
       return(E_BADINPUT);
   if (strlen(user) == 0)
       INGAUTH_DEBUG_MACRO(fp,0,"Error: strlen(user) = 0\n");
   if (strlen(passwd) == 0)
       INGAUTH_DEBUG_MACRO(fp,0,"error: strlen(passwd) = 0\n");
   return ( E_OK  );
}

/*
** Name: getDebugFile
**
** Description: Read the debug file name from the environment.
** input: 
** 	fp  - File ptr to the debug file	
** Output: 
** 	fp  - File ptr to the debug file	
** Returns:
**	E_OK	    - if II_INGVALIDPW_LOG is not set.	
** History:
**	30-Jun-2008(rajus01)
**	    Created.
*/
int getDebugFile( FILE **fp )
{
    char    *ptr = '\0',*getenv();
    int status = E_OK;
    ptr = getenv("II_INGVALIDPW_LOG");
    if (ptr && *ptr)
	status = mklogfile(fp, ptr);
    return (status);
}

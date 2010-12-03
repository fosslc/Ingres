/*
**Copyright (c) 2008, 2009 Ingres Corporation
*/

# include <stdio.h>

/*
** Name: ingpwutil.h - The common definitions for Ingvalidpw and Ingvalidpam 
**		       programs. 
**
** History:
**      17-May-2008 (Usha) SIR 120420, SD issue 116370
**          Extracted from ingvalidpw.x.
**	28-Aug-09 (gordy)
**	    Increase max length of usernames and passwords to 256.
**	    Increase line length to 1024.
**	29-Nov-2010 (frima01) SIR 124685
**	    Added prototypes for getUserAndPasswd and getDebugFile.
*/

/*
** error return codes
*/

#define E_OK            0
#define E_NOUSER        1
#define E_NOCRYPT       2
#define E_NOMATCH       3
#define E_NOTROOT       4
#define E_BADLOG        5
#define E_BADINPUT      6
#define E_EXPIRED       7
#define E_LOCKED        8
#define E_INIT      	9
#define E_AUTH      	10

#define INBUFLEN        1024
#define USR_LEN         256
#define PWD_LEN         256

extern int      iscan(char *input, char *username, char *password);
extern int      mklogfile( FILE **fp, char *ptr);
extern void	handler(int arg);

extern int getUserAndPasswd(
	char *user,
	char *passwd,
	FILE *fp);
extern int getDebugFile(
	FILE **fp);
/*
** Debug macro tha accepts variable number of arguments.
**
** fp    - The debug file pointer.
** close - flag indicating if the file needs to be closed.
** ...   - list of arguments.
*/
#define INGAUTH_DEBUG_MACRO(fp, close, ...) \
	if( fp != NULL ) { fprintf(fp, __VA_ARGS__); \
	if( close == 1 ) fclose(fp); }

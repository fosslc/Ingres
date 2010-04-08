#include        <compat.h>
#include        <si.h>
#include        <ex.h>
#include        <lo.h>
#include        <nm.h>
#include        <idm.h>
#include        "silocal.h"


/* SIgetrec
**
**      Get next input record
**
**      Get the next record from stream, previously opened for reading.
**      SIgetrec reads n-1 characters, or up to a full record, whichever
**      comes first, into buf.  The last char is followed by a newline.
**      FAIL is returned on error, else OK.  ON unix, a record on a file
**      is a string of bytes terminated by a newline.  ON systems with for-
**      matted files, a record may be defined differently.
**
** History:
**
**	24-sep-95 (tutto01)
**	    Modified to work with revamped TE facility and front ends.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	19-mar-1996 (canor01)
**	    Remove thread-local storage definitions.
**      23-aug-1996 (wilan06)
**          Remove superfluous include of wn.h
**	15-sep-1997 (donc)
**	    Call OpenROAD-specific SIgetrec if needed.
**	26-feb-1998 (somsa01)
**	    If fgets() fails, distinguish from an ENDFILE condition and
**	    an actual error condition on stdin.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	30-Aug-2008 (drivi01)
**	    In efforts to port to Visual Studio 2008 compiler, 
**	    Updated stdin_true definition.
*/

#define	SIZEOFSTDIN	260

struct  _SISTDIN
{
        short   sw;
        char *  ptr;
        char *  prompt;
};

# define SISTDIN struct _SISTDIN
# define stdin_true  (__iob_func())


#define	MAXWIDTH	79
/* the following globals and statics are used only by front-ends */
HWND  hWndParent = NULL;
char *SIprompt   = NULL;
char  SIgetrecDefault[SIZEOFSTDIN] = "";

VOID    EX_ccc_Control_C_Clear(void);
void	SIloadPrvDB(HWND hComboDB, HWND hComboNode);
void	SIloadNodeCBox(HWND hCombo);
void	SIloadDBCBox(HWND hCombo);
void	SIsaveDBCBox(char *SIdbLast);

LRESULT CALLBACK  SIDbgWndProc (HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK  StdinDlgProc (HWND, UINT, WPARAM, LPARAM);
LRESULT (CALLBACK *lpfnStdinDlgProc)() = 0;

static i2       SIf3BufLen = 0;
static char     SIf3Buf[SIZEOFSTDIN] = "";

static i2       SIinPrmtLen = 0;
static char     SIinPrompt[MAXWIDTH+1] = "";
static char	SIdbPrompt[40+1] = "";
static i2	SIdbInUse = 0;
static char	SIdbLast[32 + 1] = "";
static char	SInodeLast[16 + 1] = "";

static char *   lpszStdin = NULL;

#undef SIgetrec

STATUS
SIgetrec(
char *  buf,    /* read record into buf and null terminate */
i4      n,      /*(max number of characters - 1) to be read */
FILE *  stream) /* get reacord from file stream */
{
        char *  return_ch;

	if( GetOpenRoadStyle())
	{
	    return(SIORgetrec( buf, n, stream ));
	    
	}
	else
	{
            if (stream == NULL)
            {
		if (SIget_charmode()) 
		{
			return_ch = fgets(buf, n, stdin_true);
			if (return_ch == NULL)
			    return( (STATUS) (ferror(stdin_true) == 0 ?
				    ENDFILE : FAIL));
			else
			    return(OK);
		}

            }
            else
	    {
                return_ch = fgets(buf, n, (FILE *)stream);
	    }
	}
	if (return_ch == NULL)
	    return( (STATUS) (ferror(stdin_true) == 0 ? ENDFILE : FAIL));
	else
	    return(OK);
}


static	WPARAM	wPosNode = 0;
static	WPARAM	wPosDB   = 0;

void
SIloadPrvDB(
HWND hComboNode,
HWND hComboDB)
{
	LOCATION loc1, loc2;
	char	locbuf[MAX_LOC+1];
	char	szBuffer[0x80];
	char *	lpszBuffer = &szBuffer[0];
	char *	p;
	FILE *	fp;

	NMloc(FILES, FILENAME, "DATABASE.LAS", &loc1);
	LOcopy(&loc1, locbuf, &loc2);
	if (SIfopen(&loc2, "r", SI_TXT, 0, &fp) == OK)
	{
	    SIgetrec(szBuffer, 0x79, fp);
	    for (p = szBuffer-1; *++p; )
	    {
		if (*p == ':')
		{
		    *p = '\0';
		    strcpy(SInodeLast, lpszBuffer);
		    wPosNode = (WPARAM)(SendMessage(hComboNode, CB_ADDSTRING, 
					NULL, (LPARAM) lpszBuffer));

		    /* use item data to store the actual table value */

		    SendMessage(hComboNode, CB_SETITEMDATA, wPosNode,
					(LPARAM) 200);
		    SendMessage(hComboNode, CB_SETCURSEL, wPosNode, NULL);
		    lpszBuffer = p + 2;
		    break;
		}
	    }
	    strcpy(SIdbLast, lpszBuffer);
	    wPosDB = (WPARAM)(SendMessage(hComboDB, CB_ADDSTRING, 
					NULL, (LPARAM) lpszBuffer));

	    /* use item data to store the actual table value */

	    SendMessage(hComboDB, CB_SETITEMDATA, (WPARAM) wPosDB,
					(LPARAM) 200);
	    SendMessage(hComboDB, CB_SETCURSEL, (WPARAM)wPosDB, NULL);
	    SIclose(fp);
	}
}

void
SIloadNodeCBox(
HWND hCombo)
{
	LOCATION loc1, loc2;
	char	locbuf[MAX_LOC+1];
	WPARAM	wPosition;
	char	szBuffer[0x80];
	char *	lpszBuffer = &szBuffer[4];
	INT	i = 201;
	i4     	count;
	FILE *	fp;

	NMloc(FILES, FILENAME, "NODE.NET", &loc1);
	LOcopy(&loc1, locbuf, &loc2);
	if (SIfopen(&loc2, "r", SI_BIN, 0, &fp) == OK)
	{
	    while (SIread(fp, 0x80, &count, szBuffer) == OK)
	    {
		if (strcmp(SInodeLast, lpszBuffer) == 0)
		    SendMessage(hCombo, CB_DELETESTRING, wPosNode, NULL);
		wPosition = (WPARAM)(SendMessage(hCombo, CB_ADDSTRING, NULL,
					(LPARAM) lpszBuffer));

		/* use item data to store the actual table value */

		SendMessage(hCombo, CB_SETITEMDATA, (WPARAM) wPosition,
					(LPARAM) i++);
		if (strcmp(SInodeLast, lpszBuffer) == 0)
		    SendMessage(hCombo, CB_SETCURSEL, (WPARAM)wPosition, NULL);
	    }
	    SIclose(fp);
	}
}

#define	NUMTOSAVE	6
#define	MAXDBNAME	40
char	SIlastDatabases[MAXDBNAME * NUMTOSAVE] = { 0 };

void
SIloadDBCBox(
HWND hCombo)
{
	LOCATION loc1, loc2;
	char	locbuf[MAX_LOC+1];
	WPARAM	wPosition;
	char	szBuffer[MAXDBNAME];
	char *	lpszBuffer = szBuffer;
	INT	i = 201;
	INT	n = 0;
	FILE *	fp;

	NMloc(FILES, FILENAME, "DATABASE.LST", &loc1);
	LOcopy(&loc1, locbuf, &loc2);
	if (SIfopen(&loc2, "r", SI_TXT, 0, &fp) == OK)
	{
	    while (SIgetrec(szBuffer, MAXDBNAME - 1, fp) == OK)
	    {
		szBuffer[lstrlen(szBuffer) - 1] = '\0';
		if (n < NUMTOSAVE)
		{
		    lstrcpy(&SIlastDatabases[n * MAXDBNAME], szBuffer);
		    n++;
		}
		if (hCombo)
		{
		    if (strcmp(SIdbLast, lpszBuffer) == 0)
		       SendMessage(hCombo, CB_DELETESTRING, wPosDB, NULL);
		    wPosition = (WPARAM)(SendMessage(hCombo, CB_ADDSTRING, NULL,
					(LPARAM) lpszBuffer));

		    /* use item data to store the actual table value */

		    SendMessage(hCombo, CB_SETITEMDATA, (WPARAM) wPosition,
					(LPARAM) i++);

		    if (strcmp(SIdbLast, lpszBuffer) == 0)
		       SendMessage(hCombo,CB_SETCURSEL,(WPARAM)wPosition,NULL);
		}
	    }
	    SIclose(fp);
	}
}

void
SIsaveDBCBox(
char *	last)
{
	LOCATION loc1, loc2;
	char	locbuf[MAX_LOC+1];
	INT	n;
	FILE *	fp;

	if (SIlastDatabases[0] == '\0')
	    SIloadDBCBox(NULL);
	NMloc(FILES, FILENAME, "DATABASE.LST", &loc1);
	LOcopy(&loc1, locbuf, &loc2);
	if (SIfopen(&loc2, "w", SI_TXT, 0, &fp) == OK)
	{
	    SIfprintf(fp, "%s\n", last);
	    for (n = -1; ++n < NUMTOSAVE; )
	    {
		if (SIlastDatabases[n * MAXDBNAME]
		 && (lstrcmp(last, &SIlastDatabases[n * MAXDBNAME])))
		    SIfprintf(fp, "%s\n", &SIlastDatabases[n * MAXDBNAME]);
	    }
	    SIclose(fp);
	}
}

char *
get_SIgetrecDefault()
{
	return (SIgetrecDefault);
}

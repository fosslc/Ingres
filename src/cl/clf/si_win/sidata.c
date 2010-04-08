/*
**      SI Data
**
**  Description
**      File added to hold all GLOBALDEFs in SI facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	06-dec-1996 (donc)
**	    Added definitions for new and improved Trace Window support.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**
*/
#include    <compat.h>

/*
**  Data from sipinit
*/

GLOBALDEF HWND hWndStdout  = NULL;
GLOBALDEF HWND hWndDummy  = NULL;
GLOBALDEF i4   SIlogit = 1;
GLOBALDEF BOOL SIchar_mode = TRUE;
GLOBALDEF char	TraceWindowTitle [48] = "Ingres History";
GLOBALDEF char	TraceWindowClass [] = "IITRACE";
GLOBALDEF HANDLE SIhInst = 0;
GLOBALDEF int SIwantDialogBox = 0;
GLOBALDEF FILE * W4glErrorLog = NULL;
GLOBALDEF bool need2del_font = FALSE;
GLOBALDEF HFONT tracewin_font = 0;
GLOBALDEF char printbuf[65536] = "\0";
GLOBALDEF char linebuf[65536] = "\0";

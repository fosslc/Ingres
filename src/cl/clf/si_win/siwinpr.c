/*
** File: siwinpr.c
**
** Description:
**  A temporary workaround for the fact that Microsoft in their infinite
**  wisdom decided not to support an ANSII terminal driver.
**
** History:
**     ??-???-?? (????)
**         Original.
*/

#include <windows.h>
#include <siwinpr.h>

typedef int INT;

HWND hWndApp=NULL;
char szClass[] = WCLASS;
char szTitle[] = WTITLE;
INT nCmdShow;

HWND
IIpinit(INT show)
{
	nCmdShow = show;	/* in case we need to restart */
	if (!hWndApp) {
		if (!(hWndApp = FindWindow(szClass, szTitle))) {
			/*
			** If IIPRINTF has not been started we need to 
			** initialize buffers and WinExec() it.
			*/
			TbufAlloc();
			WinExec("IIPRINTF", nCmdShow);
		}
	}
	return (hWndApp);
}

void
IIprintf(char *str)
{
    char sbuf[512], tbuf[512], *tp=sbuf;
    BOOL iconic;

	if (!hWndApp) 
		hWndApp = IIpinit(nCmdShow);

	strcpy (sbuf, str);

	while (tp=strchr(tp,0x0a))
	{
		*tp++=0x0d;
		strcpy(tbuf,tp);
		*tp++=0x0a;
		strcpy(tp,tbuf);
	}

/*
** If iconized mark text so can be restored when Window is maximized.
*/
	if (!hWndApp || IsIconic(hWndApp)) {
		TbufMark();
		iconic = TRUE;
	}
	else 
		iconic = FALSE;

/*
** Save the string. Keep track of whether the window is iconized so we know
** when we have to restore what has been printed.
*/
    TbufSave(str);

    if (!iconic) {
/*
** Display string in the edit control.
*/
        SendDlgItemMessage(hWndApp,ID_EDIT,EM_SETSEL,0xFFFFFFFFL,0xFFFFFFFFL);
        SendDlgItemMessage(hWndApp,ID_EDIT,EM_REPLACESEL,0,(LONG)(LPSTR)sbuf);
    }
}

/*
** buffer management variables
*/
#define TBUF_SIZE	(DWORD)0xffff
char *TbufBegin=NULL;
char *TbufNext=NULL;
unsigned long TbufCount=0;
char *TbufIconized=NULL;

void
TbufAlloc(void)
{
	HANDLE hmem;
	if (!TbufBegin) {
		hmem = GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE, TBUF_SIZE);
		TbufBegin = TbufNext = GlobalLock(hmem);
	}
}

void
TbufSave(char *str)
{
	INT len;
	len = strlen(str);
	TbufCount += len;
	if (TbufCount > TBUF_SIZE) {
		TbufAdjust();
	}
	strcpy(TbufNext, str);
	TbufNext += len;
}

void
TbufAdjust(void)
{
	/* IMPLEMENT LATER */
}

void
TbufMark(void)
{
	if (!TbufIconized)
		TbufIconized = TbufNext;
}

void
TbufUpdate(void)
{
	if (TbufIconized) {
		IIprintf(TbufIconized);
		TbufIconized = NULL;
	}
}

/*
**	Copyright (c) 1983 Ingres Corporation
*/
#include	<compat.h>
#include	<si.h>
#include	"silocal.h"

/* SIORgetrec
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
**
**	History
**		03/09/83 -- (mmm)
**			written
**		2/28/85 -- (dreyfus) return ENDFILE on EOF. Fail on error
**			OK otherwise.
**      01-sep-94 (leighb)
**          Moved the SIgetrec() DialogBox here from siprintf.c
**		02-sep-94 (leighb)
**			Added 'Suspend' to systembox to allow scrolling of trace window;
**			Added 'Ditto' to systembox to copy previous entry.
**			Make sure DialogBox is never placed offscreen.
**			Remove 'Suspend'; change 'Ditto' to 'Load Prev Input' and make
**			conditional based on whether there is any previous input.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

/* static char  *Sccsid = "@(#)SIgetrec.c       3.2  4/11/84"; */


/*      (valerier)
**
** For Windows 3.0 standard input/output
*/
int		APIENTRY	StdinDlgProc (HWND, WORD, WORD, LONG);

GLOBALDEF	char	szStdin[81] = {0};
GLOBALDEF	char	SIf3Buf[81] = {0};

GLOBALREF	HWND	hWndStdout;
GLOBALREF	HANDLE	SIhInst;
GLOBALREF	int		SIwantDialogBox;

STATICDEF	int		maxwidth = 0;

STATUS
SIORgetrec(
char *  buf,    /* read record into buf and null terminate */
i4      n,      /*(max number of characters - 1) to be read */
FILE *  stream) /* get reacord from file stream */
{
STATICDEF	i4  	stdinEOF = 0;
	        char *  return_ch = NULL;

        if (SIterminal(stream))
        {
                if (hWndStdout)
                {
                	maxwidth = n;
					while (DialogBox(SIhInst,"STDIN",hWndStdout,StdinDlgProc))
					{
						MSG msg;

						for (SIwantDialogBox = 0; SIwantDialogBox == 0; )
						{
							GetMessage(&msg,hWndStdout,(UINT)NULL,(UINT)NULL);
							TranslateMessage(&msg);
						    DispatchMessage(&msg);
						}
					}
					if (szStdin[0])
                		strcpy(SIf3Buf, szStdin);
                	strcpy(buf, szStdin);
                	strcat(buf, "\n");
                	SIprintf("%s",buf);
                	return_ch = szStdin;
                }
				else
				{
					char *	p;
					i4  	i;
					i4  	ch;
					
					if (stdinEOF)
					{
						stdinEOF = 0;
					}
					else
					{
						for (i = n, return_ch = p = buf; --i; )
						{
							if ((ch = getchar()) == EOF)
							{
								stdinEOF = 1;
								break;
							}
							else
							{
								*p++ = ch;
								if (ch == '\n')
									break;
							}
						}
						*p = '\0';
					}
				}
        }
        else
        {
#ifdef  SILOGOPENCLOSE
        if (SIlogit && (SInumopenfiles > SITRIGGERNUM))
			SIprintf("%08x\tfgets()\t%d bytes\n", stream, n);
#endif
                return_ch = fgets(buf, n, stream);
        }

        return( (STATUS) (return_ch == NULL ? ENDFILE : OK));
}

#define IDM_COPY	10
#define IDM_SUSPEND	11

int APIENTRY 
StdinDlgProc(hDlg, iMessage, wParam, lParam)
HWND    hDlg;
WORD    iMessage;
WORD    wParam;
LONG    lParam;
{
	switch (iMessage)
	{
	    case WM_INITDIALOG:
		{

			WINDOWPLACEMENT	wndpl;
			int		y;
			int		yMax;
			HMENU	hSysMenu = GetSystemMenu(hDlg, FALSE);

			InsertMenu(hSysMenu, 
			   0, 
			   (SIf3Buf[0]) ? MF_BYPOSITION | MF_STRING
			                : MF_BYPOSITION | MF_STRING | MF_GRAYED,
			   IDM_COPY,
			   "&Load Prev Input");
#ifdef	SUSPEND_STDIN
			InsertMenu(hSysMenu, 
			   1, 
			   MF_BYPOSITION | MF_STRING, 
			   IDM_SUSPEND,
			   "&Suspend");
#endif
			InsertMenu(hSysMenu,
			   2,
			   MF_BYPOSITION | MF_SEPARATOR,
			   0,
			   0);

			SetDlgItemText(hDlg, 101, SIgetBufQLine());
			SetFocus(GetDlgItem(hDlg, 100));

			if (GetWindowPlacement(hWndStdout, &wndpl) == TRUE)
			{
				yMax = GetSystemMetrics(SM_CYSCREEN) 
				     - GetSystemMetrics(SM_CYCAPTION);
				y = min(wndpl.rcNormalPosition.bottom, yMax);
				if (GetWindowPlacement(hDlg, &wndpl) == TRUE)
				  MoveWindow(hDlg, wndpl.rcNormalPosition.left, y, 
					wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left,
					wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top,
											FALSE);
			}

		}
		return 1;

	    case WM_COMMAND:
	    {
			switch (LOWORD(wParam))
			{
				case IDOK:
					GetDlgItemText(hDlg, 100, szStdin, maxwidth);
					EndDialog(hDlg, 0);
					return 1;
			}
		}
		break;

	    case WM_SYSCOMMAND:
	    {
	    	STATICDEF	int		iMessBox = 0;
			WORD		wID;		  /* item, control, or accelerator ID */
			WORD		wNotifyCode;  /* 1=accelerator, 2=menu, n=control ID */
			HWND		hwndCtl;	  /* handle of control */

			wID = LOWORD(wParam);
			wNotifyCode = HIWORD(wParam);
			hwndCtl = (HWND)lParam;

			switch (wID)
			{
				case IDM_COPY:
					if (SIf3Buf[0])
					{
					    SetDlgItemText(hDlg, 100, SIf3Buf);
					    SendDlgItemMessage(hDlg, 100, WM_KEYDOWN, VK_END, 1L);
					}	
					return 1;

#ifdef	SUSPEND_STDIN
				case IDM_SUSPEND:
					if (iMessBox == 0)
					{
						iMessBox = 1;
						MessageBox(hDlg, 
								   "Press OK fully enable the Trace Window", 
								   "DoubleClick the Trace Window to restore the Input DialogBox.", 
								   MB_OK | MB_ICONINFORMATION);
					}
					szStdin[0] = '\0';
					EndDialog(hDlg, 1);
					return 1;
#endif
			}
	    }
	}
	return 0;
}

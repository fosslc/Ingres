/*
**
**  Name: AddFileName.cpp
**
**  Description:
**	This file contains the necessary routines to add a file to the file
**	liet listbox.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "AddFileName.h"
#include <cderr.h>
#include <dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

INT LocationSelect(HWND hDlg, CString *InputFile);
VOID ProcessCDError(DWORD dwErrorCode, HWND hWnd);

/*
** AddFileName dialog
*/
AddFileName::AddFileName(CWnd* pParent /*=NULL*/)
	: CDialog(AddFileName::IDD, pParent)
{
    //{{AFX_DATA_INIT(AddFileName)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void AddFileName::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(AddFileName)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AddFileName, CDialog)
    //{{AFX_MSG_MAP(AddFileName)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** AddFileName message handlers
*/
void
AddFileName::OnOK() 
{
    GetDlgItemText(IDC_EDIT_FILENAME, DestFile);
    if (GetFileAttributes(DestFile) == -1)
    {
	Message.LoadString(IDS_NO_FILEEXIST);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
    else
	CDialog::OnOK();
}

void
AddFileName::OnButtonBrowse() 
{
    if (LocationSelect(m_hWnd, &DestFile))
	SetDlgItemText(IDC_EDIT_FILENAME, DestFile);
}

INT
LocationSelect(HWND hDlg, CString *InputFile)
{
    DWORD	    status;
    CHAR	    FileFilter[] = "All Files (*.*)\0*.*\0\0";
    CFileDialog	    FileDlg(TRUE);

    FileDlg.m_ofn.lpstrFilter	 = FileFilter;
    FileDlg.m_ofn.Flags		|= (OFN_READONLY | OFN_FILEMUSTEXIST);
    FileDlg.m_ofn.lpstrTitle	 = "DP";

    if (FileDlg.DoModal() != IDOK)
    {
	status = CommDlgExtendedError();

	if (status)
	    ProcessCDError(status, hDlg);

	return FALSE;
    }
    else
    {
	*InputFile = FileDlg.GetPathName();
    }

    return TRUE;
}

/*
**  Name: ProcessCDError
**
**  Description:
**	Procedure for processing errors returned from opening common dialog
**	boxes (e.g., GetOpenFileName()).
*/
VOID
ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    WORD	wStringID;
    CString	buf, Message;

    switch(dwErrorCode)
    {
	case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
	case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
	case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
	case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
	case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
	case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
	case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
	case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
	case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
	case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
	case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
	case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
	case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
	case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
	case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
	case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
	case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
	case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
	case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
	case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
	case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
	case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
	case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
	case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
	case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
	case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
	case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

	case 0:   /* User may have hit CANCEL or we got a *very* random error */
	    return;

	default:
	    wStringID=IDS_UNKNOWNERROR;
    }

    buf.LoadString(wStringID);
    Message.LoadString(IDS_TITLE);
    MessageBox(hWnd, buf, Message, MB_APPLMODAL | MB_OK);
    return;
}

BOOL AddFileName::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    Message.LoadString(IDS_FILENAME_HEADER);
    SetDlgItemText(IDC_FILENAME_HEADER, Message);
    Message.LoadString(IDS_BUTTON_BROWSE);
    SetDlgItemText(IDC_BUTTON_BROWSE, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);
    Message.LoadString(IDS_CANCEL);
    SetDlgItemText(IDCANCEL, Message);
    
    return TRUE;
}

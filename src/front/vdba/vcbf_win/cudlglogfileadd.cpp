/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
** CuDlgLogFileAdd.cpp : implementation file
**
** History:
**	22-nov-98 (cucjo01)
**	    Created
**	    Allows user to add directories to log partitions
**	15-mar-99 (cucjo01)
**	    Added support for MAINWIN including forward/backslash handling
**	24-jan-2000 (hanch04)
**	    Updated for UNIX build using mainwin, added FILENAME_SEPARATOR
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
**  30-Apr-2002 (noifr01)
**      (bug 107622) fixed incorrect test in the OnOK() method
**  21-Nov-2003 (uk$so01)
**     SIR #99596, Remove the class CChooseDir and use the Shell function
**     SHBrowseForFolder.
** 17-Dec-2003 (schph01)
**    SIR #111462, Put string into resource files
*/


#include "stdafx.h"
#include "vcbf.h"
#include "CuDlgLogFileAdd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef MAINWIN
#define FILENAME_SEPARATOR _T('\\')
#define FILENAME_SEPARATOR_STR _T("\\")
#endif

/////////////////////////////////////////////////////////////////////////////
// CCuDlgLogFileAdd dialog

CCuDlgLogFileAdd::CCuDlgLogFileAdd(CWnd* pParent /*=NULL*/)
	: CDialog(CCuDlgLogFileAdd::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCuDlgLogFileAdd)
	m_LogFileName = _T("");
	//}}AFX_DATA_INIT
}


void CCuDlgLogFileAdd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCuDlgLogFileAdd)
	DDX_Text(pDX, IDC_LOG_FILENAME, m_LogFileName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCuDlgLogFileAdd, CDialog)
	//{{AFX_MSG_MAP(CCuDlgLogFileAdd)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCuDlgLogFileAdd message handlers
int CALLBACK mycallback( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData )
{
	TCHAR tchszBuffer [_MAX_PATH];
	if (uMsg == BFFM_INITIALIZED)
	{
		BROWSEINFO* pBrs = (BROWSEINFO*)lpData;
		::SendMessage( hwnd, BFFM_SETSELECTION ,(WPARAM)TRUE, (LPARAM)pBrs->pszDisplayName);
	}
	else
	if (uMsg == BFFM_SELCHANGED)
	{
		ITEMIDLIST* browseList = (ITEMIDLIST*)lParam;
		if(::SHGetPathFromIDList(browseList, tchszBuffer))
			::SendMessage( hwnd, BFFM_SETSTATUSTEXT  ,(WPARAM)TRUE, (LPARAM)tchszBuffer);
		else
			::SendMessage( hwnd, BFFM_SETSTATUSTEXT  ,(WPARAM)TRUE, (LPARAM)0);
	}
	return 0;
}


void CCuDlgLogFileAdd::OnBrowse() 
{
	CString strText = _T("");
#if !defined (MAINWIN)
	strText =  _T("C:");
	strText += FILENAME_SEPARATOR;
#endif
	CString strCaption;
	BOOL bOK = FALSE;
	TCHAR tchszBuffer [_MAX_PATH];
	if (strText.GetLength() > _MAX_PATH)
		lstrcpyn (tchszBuffer, strText, _MAX_PATH);
	else
		lstrcpy (tchszBuffer, strText);
	
	LPMALLOC pIMalloc;
	if (!SUCCEEDED(::SHGetMalloc(&pIMalloc)))
	{
		TRACE0("SHGetMalloc failed.\n");
		return;
	}
	//
	// Initialize a BROWSEINFO structure,
	strCaption.LoadString(IDS_CHOOSE_FOLDER);
	BROWSEINFO brInfo;
	::ZeroMemory(&brInfo, sizeof(brInfo));
	brInfo.hwndOwner = m_hWnd;
	brInfo.pidlRoot  = NULL;
	brInfo.pszDisplayName = tchszBuffer;
	brInfo.lpszTitle = (LPCTSTR)strCaption;
	brInfo.lpfn = &mycallback;
	brInfo.lParam = (LPARAM)&brInfo;

	brInfo.ulFlags = BIF_DONTGOBELOWDOMAIN | BIF_STATUSTEXT;
	
	//
	// Only want folders (no printers, etc.)
	brInfo.ulFlags |= BIF_RETURNONLYFSDIRS;
	//
	// Display the browser.
	ITEMIDLIST* browseList = NULL;
	browseList = ::SHBrowseForFolder(&brInfo);
	//
	// if the user selected a folder . . .
	if (browseList)
	{
		//
		// Convert the item ID to a pathname,
		if(::SHGetPathFromIDList(browseList, tchszBuffer))
		{
			TRACE1("You chose==>%s\n",tchszBuffer);
			bOK = TRUE;
		}
		//
		// Free the PIDL
		pIMalloc->Free(browseList);
	}
	else
	{
		bOK = FALSE;
	}
	//
	// Cleanup and release the stuff we used
	pIMalloc->Release();
	if (bOK)
	{
		CEdit* pEdit = (CEdit *) GetDlgItem(IDC_LOG_FILENAME);
		if (pEdit)
			pEdit->SetWindowText(tchszBuffer);
	}
}

void CCuDlgLogFileAdd::OnOK() 
{
	// TODO: Add extra validation here
    CEdit* pEdit = (CEdit *) GetDlgItem(IDC_LOG_FILENAME);
    if (pEdit)
    { CString strFileName;
      pEdit->GetWindowText(strFileName);
#ifdef UNIX
      if ( (strFileName.GetAt(0) != (FILENAME_SEPARATOR)) )
#else
    int ich0  = 0;
    int ich1 = (int)_tclen((const TCHAR*)strFileName + ich0); // ANSI -> 1
    int ich2 = (int)_tclen((const TCHAR*)strFileName + ich1); // ANSI -> 1
    if ( (strFileName.GetAt(ich1) != (_T(':'))) || (strFileName.GetAt(ich1+ich2) != (FILENAME_SEPARATOR)) )
#endif
      { MessageBeep(MB_ICONEXCLAMATION);
        AfxMessageBox(IDS_PATH_FOR_TRANSAC_LOG);
        return;
      }
    }

    CDialog::OnOK();
}

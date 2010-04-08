/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : xscript.cpp , Implementation File
** Project  : Ingres Journal Analyser
** Author   : Sotheavut UK (uk$so01)
** Purpose  : Implementation of Redo Dialog Box
**
** History:
**	28-Mar-1999 (uk$so01)
**	    created
**	14-May-2001 (uk$so01)
**	    SIR #101169 (additional changes)
**	    Help management and put string into the resource files
**	21-May-2001 (uk$so01)
**	    SIR #101169 (additional changes)
**	    Help management.
**	05-Sep-2003 (uk$so01)
**	    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**	21-Nov-2003 (uk$so01)
**	    SIR #99596, Remove the class CuStaticResizeMark and the bitmap
**	    sizemark.bmp.bmp
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge() to CFile::Write(), as WriteHuge() is
**	    obsolete and in WIN32 programming Write() can also write more
**	    than 64K-1 bytes of data.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "xscript.h"
#include "ijadmlll.h"
#include "choosedb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static int Find (const CString& strText, TCHAR chFind, int nStart =0, BOOL bIgnoreInsideBeginEnd = TRUE);

//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
// Test the sub-string started at <i> if it matchs the string <lpszWord>
// The sub-string, if it matchs <lpszWord>, must be terminated by <' '> or <;> or end of line.
static BOOL NextWord (const CString& strText, int index, LPCTSTR lpszWord)
{
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	int nLen  = strText.GetLength();
	int nWLen = _tcslen (lpszWord);
	if (index > 0)
	{
		TCHAR* tchStringCurr = tchString + index;
		TCHAR* tchStringPrev = _tcsdec (tchString, tchStringCurr);
		TCHAR chBack = tchStringPrev? *tchStringPrev: _T('\0');
		switch (chBack)
		{
		case _T(' '):
		case _T(';'):
		case _T('\t'):
		case 0x0D:
		case 0x0A:
			break;
		default:
			return FALSE;
		}
	}

	if ((index + nWLen) <= nLen)
	{
		CString strExtract = strText.Mid (index, nWLen);
		if (strExtract.CompareNoCase (lpszWord) == 0)
		{
			if ((index + nWLen) == nLen )
				return TRUE;
			else
			{
				TCHAR tchszC = strText.GetAt (index + nWLen);
				switch (tchszC)
				{
				case _T(' '):
				case _T(';'):
				case _T('\t'):
				case 0x0D:
				case 0x0A:
					return TRUE;
				default:
					break;
				}
			}
		}
	}
	return FALSE;
}


//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
static void MarkCommentStart (const CString& strText, int i, int& nCommentStart)
{
	int nBytes = sizeof (TCHAR);
	if ((i+nBytes) >= strText.GetLength())
		return;
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	TCHAR tchsz1 = *(tchString + i);
	tchString = _tcsinc (tchString + i);
	TCHAR tchsz2 = *(tchString);

	if (tchsz1 == _T('/') && tchsz2 == _T('*'))
		nCommentStart = 1;
	else
	if (tchsz1 == _T('-') && tchsz2 == _T('-'))
		nCommentStart = 2;
}

//
// Caller must ensure that the position i is always the position of a charactter
// ie leading byte.
static void MarkCommentEnd (const CString& strText, int i, int& nCommentEnd)
{
	int nBytes = sizeof (TCHAR);
	TCHAR* tchString = (TCHAR*)(LPCTSTR)strText;
	if ((i+nBytes) < strText.GetLength())
	{
		TCHAR tchsz1 = *(tchString + i);
		tchString = _tcsinc (tchString + i);
		TCHAR tchsz2 = *(tchString);
		if (tchsz1 == _T('*') && tchsz2 == _T('/'))
			nCommentEnd = 1;
		if (tchsz1 == 0x0D)
			nCommentEnd = 2;
	}
	else
	if (i < strText.GetLength())
	{
		TCHAR tchsz1 = *(tchString + i);
		if (tchsz1 == _T('\n'))
			nCommentEnd = 2;
	}
}


static int Find (const CString& strText, TCHAR chFind, int nStart, BOOL bIgnoreInsideBeginEnd)
{
	int   nBeginCount = 0;
	int   nCommentStart = 0; // 1: '/*'and 2: '--'.
	int   nCommentEnd   = 0; // 1: '*/'and 2: '\n'.
	BOOL  bSQuote = FALSE; 
	BOOL  bDQuote = FALSE;
	BOOL  bBegin  = FALSE;
	BOOL  bDeclare= FALSE;
	TCHAR cChar;
	ASSERT (chFind != _T('\"'));
	ASSERT (chFind != _T('\''));

	if (strText.IsEmpty())
		return -1;
	int i, nLen = strText.GetLength();
	
	for ( i=nStart; i<nLen; i+= _tclen((const TCHAR *)strText + i) )
	{
		if (nCommentStart > 0)
		{
			MarkCommentEnd (strText, i, nCommentEnd);
			if ((nCommentStart == 1 && nCommentEnd == 1) || (nCommentStart == 2 && nCommentEnd == 2))
			{
				//
				// DBCS, i += 1 will not work:
				i+= _tclen((const TCHAR *)strText + i);
				nCommentStart = 0;
				nCommentEnd   = 0;
			}
			continue;
		}
		else
		{
			//
			// The comment inside the double or single quote is considered 
			// as normal string:
			if (!(bDQuote || bSQuote))
			{
				MarkCommentStart (strText, i, nCommentStart);
				if (nCommentStart > 0)
				{
					//
					// DBCS, i += 1 will not work:
					i+= _tclen((const TCHAR *)strText + i);
					continue;
				}
			}
		}

		cChar = strText.GetAt (i);
		if (cChar == _T('\"') && !bSQuote)
		{
			bDQuote = !bDQuote;
			continue;
		}
		if (cChar == _T('\'') && !bDQuote)
		{
			//
			// If the next character is a '\'' (It means double the quote: '' to appear 
			// the quote inside the literal string)
			int iNext = i + _tclen((const TCHAR *)strText + i);
			if (iNext < nLen && strText.GetAt (iNext) != _T('\''))
			{
				bSQuote = !bSQuote;
				continue;
			}
			else
			{
				//
				// Simple Quote has been doubled:
				// Ignore the current and the next quote:
				
				//
				// DBCS, i += 1 will not work:
				i+= _tclen((const TCHAR *)strText + i);
			}
			continue;
		}
		if (bSQuote || bDQuote)
			continue; // Ignore the character inside the simple or double quote.

		if (bIgnoreInsideBeginEnd)
		{
			if (!bDeclare && (cChar == _T('D') || cChar == _T('d')))
			{
				if (NextWord (strText, i, _T("DECLARE")))
				{
					//
					// The 'bDeclare' will be set to false only if a 'begin' is found
					// some where far after the 'declare', otherwise the erronous syntax will
					// be generated !!!.
					bDeclare = TRUE;
					i += _tcslen (_T("DECLAR")); // i += strlen (_T("DECLARE")) -1;
					continue;
				}
			}

			if (!bBegin && (cChar == _T('B') || cChar == _T('b')))
			{
				if (NextWord (strText, i, _T("BEGIN")))
				{
					bBegin = TRUE;
					bDeclare = FALSE;
					nBeginCount++;
					i += _tcslen (_T("BEGI")); // i += strlen (_T("BEGIN")) -1;
					continue;
				}
			}
			
			if (bBegin)
			{
				//
				// Nesting of Begin ?
				if (cChar == _T('B') || cChar == _T('b'))
				{
					if (NextWord (strText, i, _T("BEGIN")))
					{
						nBeginCount++;
						i += _tcslen (_T("BEGI")); // i += strlen (_T("BEGIN")) -1;
						continue;
					}
				}
			
				if (cChar == _T('E') || cChar == _T('e'))
				{
					if (NextWord (strText, i, _T("END")))
					{
						nBeginCount--;
						if (nBeginCount == 0)
							bBegin = FALSE;
						i += _tcslen (_T("EN")); // i += strlen (_T("END")) -1;
					}
				}
				//
				// Ignore the character after the word <BEGIN>, ie do not
				// find the ';' inside the begin ... end.
				continue;
			}

			if (bDeclare)
			{
				//
				// Ignore the character after the word <DECLARE>, ie do not
				// find the ';' inside the declare ... begin.
				continue;
			}
		}
		if (cChar == chFind)
			return i;
	}
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// CxScript dialog


CxScript::CxScript(CWnd* pParent /*=NULL*/)
	: CDialog(CxScript::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxScript)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_strTitle = _T("");
	m_bReadOnly = FALSE;
	m_bCall4Redo = FALSE;
	m_strNode = _T("");
	m_strDatabase = _T("");
	m_bErrorOnAffectedRowCount = FALSE;
	m_nHelpID = 0;
}


void CxScript::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxScript)
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonSave);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckChangeScript);
	DDX_Control(pDX, IDCANCEL, m_cButtonCancel);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonRun);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxScript, CDialog)
	//{{AFX_MSG_MAP(CxScript)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CHECK1, OnCheckChangeScript)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonSave)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonRun)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxScript message handlers

BOOL CxScript::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE); // Set big icon
	SetIcon(m_hIcon, FALSE);// Set small icon
	
	ResizeControls();
	if (!m_strTitle.IsEmpty())
		SetWindowText (m_strTitle);
	if (m_bReadOnly)
	{
		m_cButtonRun.EnableWindow (FALSE);
		m_cCheckChangeScript.EnableWindow (FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxScript::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}

void CxScript::ResizeControls()
{
	if (!IsWindow (m_cButtonCancel.m_hWnd))
		return;
	CRect r, rDlg;
	GetClientRect (rDlg);

	//
	// Resize the buttons:
	m_cButtonCancel.GetWindowRect (r);
	ScreenToClient (r);
	int w = r.Width();
	r.right  = rDlg.right - 10;
	r.left = r.right - w;
	m_cButtonCancel.MoveWindow (r);

	m_cButtonSave.GetWindowRect (r);
	ScreenToClient (r);
	w = r.Width();
	r.right  = rDlg.right - 10;
	r.left = r.right - w;
	m_cButtonSave.MoveWindow (r);

	m_cButtonRun.GetWindowRect (r);
	ScreenToClient (r);
	w = r.Width();
	r.right  = rDlg.right - 10;
	r.left = r.right - w;
	m_cButtonRun.MoveWindow (r);

	//
	// Resize the buttons:
	int x = r.left;
	m_cEdit1.GetWindowRect (r);
	ScreenToClient(r);
	r.bottom = rDlg.bottom - r.left;
	r.right  = x - 10;
	m_cEdit1.MoveWindow (r);
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CxScript::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
void CxScript::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

void CxScript::OnCheckChangeScript() 
{
	if (m_cCheckChangeScript.GetCheck() == 1)
		m_cEdit1.SetReadOnly (FALSE);
	else
		m_cEdit1.SetReadOnly (TRUE);
}

void CxScript::OnButtonSave() 
{
	try
	{
		UpdateData (TRUE);
		CString strFileName = _T("");
		CString strFilter = _T("SQL Script Files (*.sql)|*.sql|");
		UINT nFlag = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		CFileDialog dlgFile(FALSE, _T("sql"), NULL, nFlag, (LPCTSTR)strFilter);
		int nRes = dlgFile.DoModal();
		if (nRes == IDOK )
		{
			strFileName = dlgFile.GetPathName();
			CFile file (strFileName, CFile::modeCreate | CFile::modeWrite);
			file.Write((const void*)(LPCTSTR)m_strEdit1, m_strEdit1.GetLength());
		}
	}
	catch (CFileException* e)
	{
		TCHAR tchszBuffer[256];
		e->GetErrorMessage(tchszBuffer, 255);
		AfxMessageBox (tchszBuffer, MB_OK|MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		TRACE0 ("Raise exception at CxScript::OnButtonSave() \n");
	}
}

void CxScript::OnButtonRun() 
{
	BOOL bExecCommit = FALSE;
	try
	{
		UpdateData (TRUE);
		CString strMsg;
		CString strFormat;
		int nAnswer = IDYES;
		if (m_bCall4Redo) {
			//
			// Execute the Script on node %s, database %s ?
			strFormat.LoadString(IDS_EXECUTE_SCRIPT_ON);
			strMsg.Format(strFormat, (LPCTSTR)m_strNode,(LPCTSTR)m_strDatabase);
			nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNOCANCEL);
		}
		else {
			//
			// Are you sure you want to execute the 'external script' (on node: %s, database: %s), 
			// rather than using the 'recover now' feature?
			strFormat.LoadString(IDS_EXECUTE_EXTERNALSCRIPT_ON);
			strMsg.Format(strFormat, (LPCTSTR)m_strNode,(LPCTSTR)m_strDatabase);
			nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
			if (nAnswer == IDNO)
				nAnswer=IDCANCEL;
		}
		if (nAnswer == IDNO)
		{
			CxDlgChooseDatabase dlgNodeDb;
			dlgNodeDb.m_strNode = m_strNode;
			dlgNodeDb.m_strDatabase = m_strDatabase;

			if (dlgNodeDb.DoModal() == IDOK)
			{
				m_strNode = dlgNodeDb.m_strNode;
				m_strDatabase = dlgNodeDb.m_strDatabase;
			}
			else
				return;
		}
		else
		if (nAnswer == IDCANCEL)
			return;

		//
		// Prepare the list of statements:
		int pos = -1;
		CStringList listStr;
		CString strText = m_strEdit1;
		CString strStatement;
		pos = Find (strText, _T(';'));

		while (pos != -1 || !strText.IsEmpty())
		{
			if (pos != -1)
			{
				strStatement   = strText.Left (pos);
				strText        = strText.Mid (pos+1);
			}
			else
			{
				strStatement = strText;
				strStatement.TrimLeft();
				strStatement.TrimRight();
				strText = _T("");
			}
			pos = Find (strText, _T(';'));
			strStatement.TrimLeft ();
			strStatement.TrimRight ();

			if (!strStatement.IsEmpty())
			{
				listStr.AddTail(strStatement);
			}
		}

		//
		// Execute the statements:
		CaLowlevelAddAlterDrop execparam;
		CaTemporarySession session(m_strNode, m_strDatabase);
		execparam.NeedSession (FALSE); // No not open new session
		POSITION p = listStr.GetHeadPosition();
		while (p != NULL)
		{
			strStatement = listStr.GetNext (p);
			execparam.SetStatement (strStatement);
			execparam.ExecuteStatement(NULL);
			strStatement.MakeLower();
			if (strStatement.Find(_T("commit")) == 0)
				bExecCommit = TRUE;

			if (strStatement.Find(_T("insert")) == 0 || strStatement.Find(_T("update")) == 0 || strStatement.Find(_T("delete")) == 0)
			{
				CString strMsg = _T("");
				CString strCaller;
				if (m_bCall4Redo)
					strCaller.LoadString (IDS_REDO);
				else
					strCaller.LoadString (IDS_RECOVER);

				int nAffectedRow = execparam.GetAffectedRows();
				if (nAffectedRow == 0)
				{
					AfxFormatString2 (
						strMsg, 
						IDS_INSERTxUPDATE_NO_AFFECTROW_WARNING, 
						(LPCTSTR)strCaller, 
						(LPCTSTR)strStatement);
				}
				else
				if (m_bErrorOnAffectedRowCount && nAffectedRow > 1)
				{
					AfxFormatString2 (
						strMsg, 
						IDS_INSERTxUPDATE_AFFECTROWS_WARNING, 
						(LPCTSTR)strCaller, 
						(LPCTSTR)strStatement);
				}

				if (!strMsg.IsEmpty()) {
					if (bExecCommit) {
						CString strMsg2;
						if (m_bCall4Redo) {
							// 
							// \n\nBefore the failure, some "redo" transactions had been committed (because you have asked 
							// to redo more than one transaction, and to impersonate the initial users of the transactions).\n
							// You will need to cancel these manually (typically through IJA)
							strMsg2.LoadString (IDS_FAILURE_ON_RUN_1);
						}
						else {
							//
							// \n\nBefore the failure, some "recover" transactions had been committed (because you have asked 
							// to recover more than one transaction, and to impersonate the initial users of the transactions).\n
							// You will need to cancel these manually (typically through IJA)
							strMsg2.LoadString (IDS_FAILURE_ON_RUN_2);
						}
						strMsg +=strMsg2;
					}
					AfxMessageBox(strMsg,MB_OK|MB_ICONSTOP);
					return;
				}
			}
		}

		//
		// Release and commit session:
		session.Release();
		if (m_bCall4Redo)
		{
			// 
			// The 'Redo' script has been executed successfully.
			strMsg.LoadString(IDS_RUN_REDO_SCRIPT_SUCCESSFULLY);
		}
		else
		{
			//
			// The 'Recover' script has been executed successfully.
			strMsg.LoadString(IDS_RUN_RECOVER_SCRIPT_SUCCESSFULLY);
		}
		AfxMessageBox (strMsg);
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_OK|MB_ICONEXCLAMATION);
		//
		// If an error has occurred and a commit statement has been
		// executed, then just issue a WARNING !
		if (bExecCommit)
		{
			CString strMsg;
			strMsg.LoadString(IDS_RUNSCRIPT_COMMIT_WARNING);
			AfxMessageBox(strMsg, MB_OK|MB_ICONEXCLAMATION);
		}
	}
	catch (...)
	{
		TRACE0 ("Raise exception at CxScript::OnButtonRun() \n");
	}
}

BOOL CxScript::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (m_nHelpID > 0)
	{
		APP_HelpInfo (m_hWnd, m_nHelpID);
	}

	return TRUE;
}

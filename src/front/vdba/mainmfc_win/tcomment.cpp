/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : tcomment.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**
**  Author   : Schalk Philippe
**
**  Purpose  : Dialog and Class used for manage the comments on tables and
**             Views and columns
**
** History:
** 24-Oct-2000 (schph01)
**    SIR 102821 Created
** 24-Jun-2002 (hanje04)
**    Cast all CStrings being passed to other functions or methods using %s
**    with (LPCTSTR) and made calls more readable on UNIX.
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "tcomment.h"
#include "dgerrh.h"     // MessageWithHistoryButton

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlCommentInsertValue class

CuEditableListCtrlCommentInsertValue::CuEditableListCtrlCommentInsertValue()
{
}

CuEditableListCtrlCommentInsertValue::~CuEditableListCtrlCommentInsertValue()
{
}

BEGIN_MESSAGE_MAP(CuEditableListCtrlCommentInsertValue, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlCommentInsertValue)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK, OnEditDlgOK)
END_MESSAGE_MAP()


LONG CuEditableListCtrlCommentInsertValue::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
	{
		SetFocus();
		return 0L;
	}
	try
	{
		SetItemText (iItem, iSubItem, s);
		GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)(LPCTSTR)m_EditDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		//CString strMsg = _T("Cannot change edit text.");
		AfxMessageBox (IDS_E_CHANGE_EDIT);
	}

	SetFocus();
	return 0L;

}

void CuEditableListCtrlCommentInsertValue::SetLimitEditText()
{
	CEdit* cEditTemp = EDIT_GetEditCtrl();
	if (cEditTemp && IsWindow(cEditTemp->m_hWnd))
		cEditTemp->LimitText(MAX_LENGTH_COMMENT);
}


/////////////////////////////////////////////////////////////////////////////
// CxDlgObjectComment dialog


CxDlgObjectComment::CxDlgObjectComment(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgObjectComment::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgObjectComment)
	//}}AFX_DATA_INIT
}


void CxDlgObjectComment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgObjectComment)
	DDX_Control(pDX, IDC_COLUMNS_COMMENT, m_cListCtrl);
	DDX_Control(pDX, IDC_TABLE_COMMENT, m_cEditTableComment);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CxDlgObjectComment, CDialog)
	//{{AFX_MSG_MAP(CxDlgObjectComment)
	ON_WM_DESTROY()
	ON_EN_SETFOCUS(IDC_TABLE_COMMENT, OnSetfocusTableComment)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgObjectComment message handlers
BOOL CxDlgObjectComment::OnInitDialog()
{
	CDialog::OnInitDialog();
	LPUCHAR parent [MAXPLEVEL];
	TCHAR buffer   [MAXOBJECTNAME];
	CString csFormatTitle,csCaption;

	//
	// Make up title:
	GetWindowText(csFormatTitle);
	parent [0] = (LPUCHAR)(LPTSTR)(LPCTSTR)m_csDBName;
	parent [1] = NULL;
	GetExactDisplayString (m_csObjectName, m_csObjectOwner, m_iCurObjType, parent, buffer);

	csCaption.Format(csFormatTitle,(LPTSTR)GetVirtNodeName(m_nNodeHandle),m_csDBName,buffer);
	SetWindowText(csCaption);

	m_ImageList.Create(1, 18, TRUE, 1, 0);
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetFullRowSelected (TRUE);

	// TODO: Add extra initialization here
	LV_COLUMN lvcolumn;
	TCHAR     szColHeader [2][16];
	lstrcpy(szColHeader [0],VDBA_MfcResourceString(IDS_TC_COLUMN));          // _T("Column")
	lstrcpy(szColHeader [1],VDBA_MfcResourceString(IDS_TC_COLUMN_COMMENT));  // _T("Comment")
	int       i;
	int       hWidth [2] = {130, 180};
	CRect r;
	// calculate the width of column "comment"
	m_cListCtrl.GetClientRect(r);
	hWidth [1] = r.right-hWidth [0];
	//
	// Set the number of columns: 2
	// List of columns of Base Object (table or view):
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<2; i++)
	{
		lvcolumn.fmt = LVCFMT_LEFT;
		lvcolumn.pszText = szColHeader[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = hWidth[i];
		m_cListCtrl.InsertColumn (i, &lvcolumn);
	}

	if (!InitializeObjectColumns() || !m_ListColumn)
	{
		EndDialog(FALSE);
		return TRUE;
	}

	if (!FillListColumnWithComment( ))
	{
		EndDialog(FALSE);
		return TRUE;
	}

	LPOBJECTLIST  ls;
	LPCOMMENTCOLUMN lpCol;
	int nCount = 0;
	CString csComment;
	ls = m_ListColumn;
	while (ls)
	{
		lpCol=(LPCOMMENTCOLUMN)ls->lpObject;
		m_cListCtrl.InsertItem  (nCount, (LPCTSTR)lpCol->szColumnName);
		if (lpCol->lpszComment)
			m_cListCtrl.SetItemText  (nCount, 1,(LPCTSTR)lpCol->lpszComment);
		nCount++;
		ls=(LPOBJECTLIST)ls->lpNext;
	}

	m_cListCtrl.SetLimitEditText();
	
	m_cEditTableComment.LimitText (MAX_LENGTH_COMMENT);
	m_cEditTableComment.SetWindowText(m_csCommentObject);
	m_bEditUnselected = TRUE;

	PushHelpId(IDD_XOBJECT_COMMENT);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgObjectComment::OnOK()
{
	CString csTempo,csNameCol,csSyntaxObject,csVNodeName,csSQLSyntax;
	LPCOMMENTCOLUMN lpCommentCol;
	LPSTRINGLIST lpStrList = NULL;
	BOOL bGenerate;
	int i,iret,nCount = m_cListCtrl.GetItemCount();
	csVNodeName = (LPTSTR)GetVirtNodeName(m_nNodeHandle);
	csSyntaxObject.Empty();

	m_cListCtrl.HideProperty(TRUE);

	m_cEditTableComment.GetWindowText(m_csCommentObject);

	if (!m_tcObjComment && !m_csCommentObject.IsEmpty()) // new comment on general object
		GenerateSyntaxComment4Object(csSyntaxObject);
	else if (m_tcObjComment && (x_strcmp(m_csCommentObject,m_tcObjComment)!=0) ) // comment exist 
		GenerateSyntaxComment4Object(csSyntaxObject);

	for (i=0; i<nCount; i++)
	{
		csSQLSyntax.Empty();
		bGenerate = FALSE;
		csNameCol = m_cListCtrl.GetItemText (i,0);
		csTempo   = m_cListCtrl.GetItemText (i,1);

		lpCommentCol = FindCurrentColumn(csNameCol);
		ASSERT(lpCommentCol != NULL);

		if (lpCommentCol->lpszComment)
		{
			if (x_strcmp(lpCommentCol->lpszComment,(LPTSTR)(LPCTSTR)csTempo)!=0)
				bGenerate = TRUE;
		}
		else
		{
			if (!csTempo.IsEmpty())
				bGenerate = TRUE;
		}
		if (bGenerate)
		{
			csSQLSyntax.Format("comment on column %s.%s.%s is '%s'",
			QuoteIfNeeded((LPTSTR)(LPCTSTR)m_csObjectOwner), 
			QuoteIfNeeded((LPTSTR)(LPCTSTR)m_csObjectName), 
			QuoteIfNeeded((LPTSTR)lpCommentCol->szColumnName), 
			(LPCTSTR)csTempo);
			lpStrList = StringList_Add  (lpStrList, (LPTSTR)(LPCTSTR)csSQLSyntax);
		}
	}

	// execute the SQL statements
	iret =  VDBA20xGenCommentObject ( (LPSTR)(LPCTSTR)csVNodeName,(LPSTR)(LPCTSTR)m_csDBName,
	                                  (LPSTR)(LPCTSTR)csSyntaxObject, lpStrList);

	if (lpStrList)
		lpStrList = StringList_Done (lpStrList);

	if (iret != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_COMMENT_ON_FAILED, RES_ERR);
		return;
	}

	CDialog::OnOK();
}


BOOL CxDlgObjectComment::InitializeObjectColumns()
{
	TCHAR tblNameWithOwner[MAXOBJECTNAME];
	int ires, nLevel = 2;
	LPUCHAR aparents [MAXPLEVEL];
	TCHAR   tchszBuf       [MAXOBJECTNAME];
	TCHAR   tchszOwner   [MAXOBJECTNAME];
	TCHAR   tchszBufComplim[MAXOBJECTNAME];
	LPTSTR  lpszOwner = NULL;
	BOOL    bSystem = FALSE;

	if (m_iCurObjType != OT_TABLE && m_iCurObjType != OT_VIEW)
		return FALSE;

	StringWithOwner((LPUCHAR)Quote4DisplayIfNeeded((LPTSTR)(LPCTSTR)m_csObjectName),
	                (LPUCHAR)(LPTSTR)(LPCTSTR) m_csObjectOwner, (LPUCHAR)tblNameWithOwner);

	aparents[0] = (LPUCHAR)(LPCTSTR)m_csDBName;
	aparents[1] = (LPUCHAR)tblNameWithOwner;
	aparents[2] = NULL;
	nLevel = 2;
	tchszBuf[0] = 0;
	tchszOwner[0] = 0;
	tchszBufComplim[0] = 0;
	LPOBJECTLIST lpColTemp;
	LPCOMMENTCOLUMN lpCommentCol;

	ires =  DOMGetFirstObject( m_nNodeHandle,
	                          (m_iCurObjType == OT_TABLE)? OT_COLUMN: OT_VIEWCOLUMN,
	                           nLevel,
	                           aparents,
	                           bSystem,
	                           NULL,
	                          (LPUCHAR)tchszBuf,
	                          (LPUCHAR)tchszOwner,
	                          (LPUCHAR)tchszBufComplim);
	if (ires != RES_SUCCESS && ires != RES_ENDOFDATA)
	{
		CString csMsg;
		csMsg.LoadString(IDS_E_COMMENTS_FAILED);
		MessageWithHistoryButton(m_hWnd,csMsg);
		return FALSE;
	}

	while (ires == RES_SUCCESS)
	{
		lpColTemp = AddListObjectTail(&m_ListColumn, sizeof(COMMENTCOLUMN));
		if (!lpColTemp)
		{
			// Need to free previously allocated objects.
			ErrorMessage ( (UINT)IDS_E_CANNOT_ALLOCATE_MEMORY, RES_ERR);
			FreeObjectList(m_ListColumn);
			m_ListColumn = NULL;
			return FALSE;
		}
		lpCommentCol = (LPCOMMENTCOLUMN)lpColTemp->lpObject;
		lstrcpy((LPTSTR)lpCommentCol->szColumnName,tchszBuf);
		lpCommentCol->lpszComment = NULL;
		tchszBuf[0] = 0;
		tchszOwner[0] = 0;
		tchszBufComplim[0] = 0;
		ires = DOMGetNextObject ((LPUCHAR)tchszBuf, (LPUCHAR)tchszOwner, (LPUCHAR)tchszBufComplim);
	}
	return TRUE;
}

void CxDlgObjectComment::ObjectColumn_Done ()
{
	LPOBJECTLIST ls = m_ListColumn;
	LPCOMMENTCOLUMN lpCommentCol;

	while (ls)
	{
		lpCommentCol = (LPCOMMENTCOLUMN)ls->lpObject;
		if (lpCommentCol->lpszComment)
			ESL_FreeMem (lpCommentCol->lpszComment);
		ls = (LPOBJECTLIST)ls->lpNext;
	}
}

void CxDlgObjectComment::OnDestroy() 
{
	CDialog::OnDestroy();
	
	ObjectColumn_Done();
	FreeObjectList(m_ListColumn);
	m_ListColumn = NULL;
	if (m_tcObjComment)
		ESL_FreeMem(m_tcObjComment);
	m_ImageList.DeleteImageList();
	PopHelpId();
}

LPCOMMENTCOLUMN CxDlgObjectComment::FindCurrentColumn(CString& csColName )
{
	LPOBJECTLIST ls = m_ListColumn;
	LPCOMMENTCOLUMN lpCommentCol;

	while (ls)
	{
		lpCommentCol = (LPCOMMENTCOLUMN)ls->lpObject;
		if (x_strcmp((LPTSTR)lpCommentCol->szColumnName,(LPTSTR)(LPCTSTR)csColName)==0)
			return (LPCOMMENTCOLUMN)ls->lpObject;
		ls = (LPOBJECTLIST)ls->lpNext;
	}
	return NULL;
}

BOOL CxDlgObjectComment::FillListColumnWithComment( void )
{
	//
	// Get Session
	int hdl, nRes = RES_ERR ;
	TCHAR tcDBName      [MAXOBJECTNAME];
	TCHAR tcObjectName  [MAXOBJECTNAME];
	TCHAR tcObjectOwner [MAXOBJECTNAME];
	CString strSess;
	m_tcObjComment = NULL;
	CString strVNodeName = (LPTSTR)GetVirtNodeName(m_nNodeHandle);

	x_strcpy(tcDBName,m_csDBName);
	x_strcpy(tcObjectName, m_csObjectName );
	x_strcpy(tcObjectOwner,m_csObjectOwner);

	strSess.Format (_T("%s::%s"), 
	(LPTSTR)(LPCTSTR)strVNodeName, 
	(LPTSTR)(LPCTSTR)m_csDBName);
	nRes = Getsession ((LPUCHAR)(LPCTSTR)strSess, SESSION_TYPE_CACHEREADLOCK, &hdl);
	if (nRes != RES_SUCCESS)
	{
		TRACE0 (_T("CxDlgObjectComment::FillListColumnWithComment(): Fail to Get the session\n"));
		return FALSE;
	}

	nRes = SQLGetComment( (LPTSTR)(LPCTSTR)strVNodeName, tcDBName, tcObjectName, tcObjectOwner,
	                      &m_tcObjComment, m_ListColumn);

	ReleaseSession (hdl, RELEASE_COMMIT);

	if (nRes != RES_SUCCESS)
	{
		CString csMsg;
		csMsg.LoadString(IDS_E_COMMENTS_FAILED);
		MessageWithHistoryButton(m_hWnd,csMsg);
		return FALSE;
	}

	if (m_tcObjComment)
		SetCommentObject(m_tcObjComment);

	return TRUE;
}


void CxDlgObjectComment::GenerateSyntaxComment4Object (CString &csText)
{
	//
	// Comment on Object
	if (m_iCurObjType == OT_TABLE || m_iCurObjType == OT_VIEW)
	{
		csText.Format("comment on table %s.%s is '%s'", 
		(LPTSTR)QuoteIfNeeded(m_csObjectOwner), 
		(LPTSTR)QuoteIfNeeded(m_csObjectName), 
		(LPCTSTR)m_csCommentObject);
	}
}



void CxDlgObjectComment::OnSetfocusTableComment() 
{
	if (m_bEditUnselected)
	{
		m_cEditTableComment.SetSel(-1,0,FALSE);
		m_bEditUnselected = FALSE;
	}
}

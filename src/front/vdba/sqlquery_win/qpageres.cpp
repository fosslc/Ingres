/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qpageres.cpp, Implementation file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**
**    Purpose  : It contains a list of rows resulting from the SELECT statement.
**
** History:
**
** 18-Apr-2001 (noifr01)
**    (bug 104505) a numeric column was displayed as left-aligned (instead of
**    right-aligned)if it was the first displayed column
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate Ingres Import Assistant.
** 07-Jun-2001 (noifr01)
**    (sir 104881) left aligned remaining non-numeric columns that were still right-aligned
** 18-Jun-2001 (uk$so01)
**    BUG #104799, Deadlock on modify structure when there exist an opened 
**    cursor on the table.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
** 26-Feb-2003 (uk$so01)
**    SIR #106648, conform to the change of DDS VDBA split
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 11-Apr-2005 (komve01)
**    SIR #114284/Issue#14055289, Enabled the contents of the Results List Control
**	  to be copied to Clipboard.
** 20-Apr-2005 (komve01)
**    SIR #114284/Issue#14055289, The propagation from main seems to have missed.
**    the definition of the newly added handler OnKeyDownResultsList.
**    13-Sep-2005 (gupsh01, uk$so01)
**	In function ExcuteSelectStatement, added code to manage releasing
**	session on closing cursor. If sqlquery.ocx is invoked from the vdba dom
**	right panel then set to release the session on close.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qframe.h"
#include "qpageres.h"

//
// The define below shows the number of fetched rows below the list control:
#define FETCH_ROWS_INFO 

#ifndef VK_C
#define		VK_C	0x43
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CuDlgSqlQueryPageResult::SetQueryRowParam(CaExecParamQueryRows* pParam)
{
	m_ListCtrl.SetQueryRowParam(pParam);
}


void CuDlgSqlQueryPageResult::AboutFecthing (int nRow, BOOL bEnd)
{
	TCHAR tchszNumber [16];
	wsprintf (tchszNumber, _T("%d"), nRow);
	CString strInfo;
	if (bEnd)
		AfxFormatString1 (strInfo, IDS_TOTAL_FETCHED_ROW, tchszNumber);
	else
		AfxFormatString1 (strInfo, IDS_ALREADY_FETCHED_ROW, tchszNumber);
	m_cStaticTotalFetchedRows.SetWindowText (strInfo);
}


CuDlgSqlQueryPageResult::CuDlgSqlQueryPageResult(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgSqlQueryPageResult::IDD, pParent)
{
	m_bStandAlone = FALSE;

	//{{AFX_DATA_INIT(CuDlgSqlQueryPageResult)
	m_strEdit1 = _T("");
	//}}AFX_DATA_INIT
	m_strDatabase = _T("");
	m_nStart = 0L;
	m_nEnd   = 0L;
	m_bShowStatement = FALSE;
	m_bBrokenCursor = FALSE;
	m_bAllRows      = FALSE;
	m_pQueryRowParam = NULL;

	m_bLoaded = FALSE;
	m_nBitmap = 0;
}


void CuDlgSqlQueryPageResult::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgSqlQueryPageResult)
	DDX_Control(pDX, IDC_STATIC1, m_cStat1);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_STATIC2, m_cStat2);
	DDX_Control(pDX, IDC_EDIT2, m_cEdit2);
	DDX_Text(pDX, IDC_EDIT2, m_strDatabase);
	DDX_Control(pDX, IDC_STATIC3, m_cStaticTotalFetchedRows);
}


BEGIN_MESSAGE_MAP(CuDlgSqlQueryPageResult, CDialog)
	//{{AFX_MSG_MAP(CuDlgSqlQueryPageResult)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY(NM_OUTOFMEMORY, IDC_LIST1, OnOutofmemoryList1)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickMfcList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_QUERY_PAGE_SHOWHEADER, OnDisplayHeader)
	ON_MESSAGE (WM_SQL_QUERY_PAGE_HIGHLIGHT,  OnHighlightStatement)
	ON_MESSAGE (WM_SQL_STATEMENT_SHOWRESULT,  OnDisplayResult)
	ON_MESSAGE (WM_SQL_GETPAGE_DATA,          OnGetData)
	ON_MESSAGE (WM_SQL_CLOSE_CURSOR,          OnCloseCursor)
	ON_MESSAGE (WM_SQL_TAB_BITMAP,            OnGetTabBitmap)
	ON_MESSAGE (WMUSRMSG_SQL_FETCH,           OnFetchRows)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,      OnChangeSetting)
	ON_MESSAGE (WMUSRMSG_GETFONT,             OnGetSqlFont)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, OnKeyDownResultsList)
END_MESSAGE_MAP()

LONG CuDlgSqlQueryPageResult::OnGetTabBitmap (WPARAM wParam, LPARAM lParam)
{
	CaExecParamQueryRows* pExecParam = m_ListCtrl.GetQueryRowParam();
	if (pExecParam)
	{
		if (m_bBrokenCursor)
			return (LONG)BM_SELECT_BROKEN;
		else
		if (pExecParam->IsMoreResult())
			return (LONG)BM_SELECT_OPEN;
		else
		if (m_bAllRows)
			return (LONG)BM_SELECT;
		else
			return (LONG)BM_SELECT_CLOSE;
	}
	else
	if (m_bLoaded)
	{
		return (LONG)m_nBitmap;
	}

	return (LONG)0;
}

LONG CuDlgSqlQueryPageResult::OnCloseCursor (WPARAM wParam, LPARAM lParam)
{
	CaExecParamQueryRows* pExecParam = m_ListCtrl.GetQueryRowParam();
	if (!m_bLoaded)
	{
		ASSERT(pExecParam);
	}
	if (pExecParam && pExecParam->IsUsedCursor() && pExecParam->IsMoreResult())
	{
		m_bBrokenCursor = TRUE;
		if (!m_bStandAlone)
		{
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
			if (pFrame)
				pFrame->GetDlgSqlQueryResult()->UpdateTabBitmaps();
		}
		pExecParam->DoInterrupt(); // Close Cursor
	}
	return (LONG)0;
}

LONG CuDlgSqlQueryPageResult::OnDisplayHeader (WPARAM wParam, LPARAM lParam)
{
	m_bShowStatement = TRUE;
	CRect r;
	GetClientRect (r);
	OnSize (SIZE_RESTORED, r.Width(), r.Height());
	return 0L;
}

LONG CuDlgSqlQueryPageResult::OnHighlightStatement (WPARAM wParam, LPARAM lParam)
{
	if (m_bStandAlone)
	{
		//
		// Stand alone mode, its not concerned in Frame/View/Doc,
		// so do not use WM_SQL_QUERY_PAGE_HIGHLIGHT
		ASSERT (FALSE);
		return 0;
	}

	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return 0;
	CvSqlQueryRichEditView* pView = pFrame->GetRichEditView();
	if (!pView)
		return 0;
	CdSqlQueryRichEditDoc* pDoc =(CdSqlQueryRichEditDoc*)pView->GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return 0L;

	pView->SetColor (-1,  0, pDoc->m_crColorText);
	if (m_bShowStatement)
		return 0L;
	pView->SetColor (m_nStart, m_nEnd, pDoc->m_crColorSelect);
	return 0L;
}


//
// This funtion is originated from sqlact.c
// modified to used CListCtrl
LONG CuDlgSqlQueryPageResult::OnDisplayResult (WPARAM wParam, LPARAM lParam)
{
	CWaitCursor waitCursor;
	try
	{
		long nRes = (long)m_ListCtrl.DisplayResult();
		if (m_bStandAlone)
		{
			Invalidate();
			SetForegroundWindow();
		}

		return nRes;
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		BrokenCursor();
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
		BrokenCursor();
	}
	catch (...)
	{
		AfxMessageBox (_T("CuDlgSqlQueryPageResult::OnDisplayResult: Unknown error")); 
		BrokenCursor();
	}
	return 0;
}


void CuDlgSqlQueryPageResult::BrokenCursor()
{
	m_bBrokenCursor = TRUE;
	if (m_pQueryRowParam)
	{
		m_pQueryRowParam->DoInterrupt();
	}
	if (!m_bStandAlone)
	{
		CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
		if (pFrame)
			pFrame->GetDlgSqlQueryResult()->UpdateTabBitmaps();
	}
}


LONG CuDlgSqlQueryPageResult::OnGetData (WPARAM wParam, LPARAM lParam)
{
	LVCOLUMN lvcolumn;
	TCHAR    tchszHeader [64];
	int i, nCount = m_ListCtrl.GetItemCount();
	int j, nColumn = 0;
	CaQuerySelectPageData* pData    = NULL;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH;
	lvcolumn.pszText    = tchszHeader;
	lvcolumn.cchTextMax = sizeof (tchszHeader);

	try
	{
		CStringList* pRowList = NULL;
		CString strItem;
		CaQueryResultRowsHeader* pHeader;
		pData    = new CaQuerySelectPageData();
		pData->m_nID            = IDD;
		pData->m_bShowStatement = m_bShowStatement;
		pData->m_strStatement   = m_strEdit1;
		pData->m_nStart         = m_nStart;
		pData->m_nEnd           = m_nEnd;
		pData->m_strDatabase    = m_strDatabase;
		pData->m_nTabImage      = (int)OnGetTabBitmap(0, 0);
		pData->m_nTopIndex      = m_ListCtrl.GetTopIndex();

		if (pData->m_nTabImage  == BM_SELECT_OPEN)
			pData->m_nTabImage  = BM_SELECT_BROKEN;
		//
		// Information about fetched rows:
		m_cStaticTotalFetchedRows.GetWindowText (pData->m_strFetchInfo);
		//
		// For each column
		for (i=0; m_ListCtrl.GetColumn(i, &lvcolumn); i++)
		{
			nColumn++;
			pHeader = new CaQueryResultRowsHeader ((LPCTSTR)lvcolumn.pszText, lvcolumn.cx, lvcolumn.fmt);
			pData->m_listRows.m_listHeaders.AddTail (pHeader);
		}
		//
		// For each row
		for (i=0; i<nCount; i++)
		{
			// store item data
			int itemData = (int)m_ListCtrl.GetItemData(i);
			ASSERT (itemData >= 0 && itemData <= 2);
			pData->m_listRows.m_aItemData.Add(itemData);

			pRowList = new CStringList(nColumn);
			//
			// For each column of a row.
			for (j=0; j<nColumn; j++)
			{
				strItem = m_ListCtrl.GetItemText (i, j);
				pRowList->AddTail (strItem);
			}
			pData->m_listRows.m_listAdjacent.AddTail (pRowList);
			if (LVIS_SELECTED & m_ListCtrl.GetItemState (i, LVIS_SELECTED))
			{
				pData->m_nSelectedItem = i;
			}
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
		return (LONG)NULL;
	}
	catch (...)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_GETDATA_FOR_SAVE, MB_ICONEXCLAMATION|MB_OK);
		if (pData)
			delete pData;
		return (LONG)NULL;
	}
	
	return (LONG)pData;
}


void CuDlgSqlQueryPageResult::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgSqlQueryPageResult::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (!m_bShowStatement)
	{
		m_cEdit1.ShowWindow(SW_HIDE);
		m_cStat1.ShowWindow(SW_HIDE);
		m_cEdit2.ShowWindow(SW_HIDE);
		m_cStat2.ShowWindow(SW_HIDE);
	}
	VERIFY (m_ListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LONG style = GetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE);
	style |= LVS_SHOWSELALWAYS;
	SetWindowLong (m_ListCtrl.m_hWnd, GWL_STYLE, style);
	m_ImageList.Create(1, 12, TRUE, 1, 0);
	//
	// Mask this to use the regular text height:
	m_ListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_ListCtrl.SetFullRowSelected (TRUE);
	m_ListCtrl.SetShowRowNumber(FALSE);

	CString& strNMA = m_ListCtrl.GetNoMoreAvailableString();
	CString& strNA  = m_ListCtrl.GetNotAvailableString();
	CString& strTitle  = m_ListCtrl.GetTitle();
	strNMA.LoadString (IDS_MSG_NO_FIRST_TUPLES);
	strNA.LoadString  (IDS_NOT_AVAILABLE);
	strTitle.LoadString(IDS_TITLE_FETCHING_ROW);
	if (!m_bStandAlone)
	{
		CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
		CdSqlQueryRichEditDoc* pDoc = (CdSqlQueryRichEditDoc*)pFrame->GetActiveDocument();
		SendMessage (WMUSRMSG_CHANGE_SETTING, 0, (LPARAM)pDoc);
	}
	AboutFecthing (0, TRUE);
	ResizeControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgSqlQueryPageResult::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}



void CuDlgSqlQueryPageResult::NotifyLoad (CaQuerySelectPageData* pData)
{
	LVCOLUMN lvcolumn;
	int nCount, nColumn = 0;
	CaQueryResultRowsHeader* pHeader = NULL;
	CStringList* pStringList = NULL;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_TEXT|LVCF_SUBITEM|LVCF_WIDTH;
	
	ASSERT (pData);
	if (!pData)
		return;
	m_bLoaded = TRUE;
	m_nBitmap = pData->m_nTabImage;
	m_bShowStatement = pData->m_bShowStatement;
	m_nStart         = pData->m_nStart;
	m_nEnd           = pData->m_nEnd;
	m_strEdit1       = pData->m_strStatement;
	m_strDatabase    = pData->m_strDatabase;
	UpdateData (FALSE);
	ResizeControls();

	POSITION p, pos = pData->m_listRows.m_listHeaders.GetHeadPosition();
	while (pos != NULL)
	{
		pHeader = pData->m_listRows.m_listHeaders.GetNext(pos);
		lvcolumn.pszText = (LPTSTR)(LPCTSTR)pHeader->m_strHeader;
		lvcolumn.fmt     = pHeader->m_nAlign;
		lvcolumn.cx      = pHeader->m_nWidth;
		m_ListCtrl.InsertColumn(nColumn, &lvcolumn);
		nColumn++;
	}
	pos = pData->m_listRows.m_listAdjacent.GetHeadPosition();
	//
	// For each row.
	while (pos != NULL)
	{
		pStringList = pData->m_listRows.m_listAdjacent.GetNext (pos);
		if (!pStringList)
			continue;
		p = pStringList->GetHeadPosition();
		nColumn = 0;
		nCount = m_ListCtrl.GetItemCount();
		//
		// For each column of a row
		while (p != NULL)
		{
			CString& strItem = pStringList->GetNext (p);
			if (nColumn == 0)
				m_ListCtrl.InsertItem  (nCount, (LPCTSTR)strItem);
			else
				m_ListCtrl.SetItemText (nCount, nColumn, (LPCTSTR)strItem);
			nColumn++;
		}

		// restore item data
		m_ListCtrl.SetItemData(nCount, (DWORD)pData->m_listRows.m_aItemData[nCount]);
	}
	if (pData->m_nSelectedItem != -1)
		m_ListCtrl.SetItemState (pData->m_nSelectedItem, LVIS_SELECTED, LVIS_SELECTED);

	m_ListCtrl.EnsureVisible (0, TRUE);
	CRect rs;
	m_ListCtrl.GetItemRect (0, rs, LVIR_BOUNDS);
	CSize size (0, pData->m_nTopIndex*rs.Height());
	m_ListCtrl.Scroll (size);
	m_cStaticTotalFetchedRows.SetWindowText (pData->m_strFetchInfo);
}

void CuDlgSqlQueryPageResult::ResizeControls()
{
	int nHeight = 12;
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect rDlg, r, rInfo, rList;
	GetClientRect (rDlg);
	if (m_bShowStatement)
	{
		m_cEdit1.ShowWindow(SW_SHOW);
		m_cStat1.ShowWindow(SW_SHOW);
		m_cEdit2.ShowWindow(SW_SHOW);
		m_cStat2.ShowWindow(SW_SHOW);
	}
	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right;
	m_cEdit1.MoveWindow (r);

	r.top    = m_bShowStatement? r.bottom + 4: rDlg.top;
	r.left   = rDlg.left;
	r.bottom = rDlg.bottom;
	m_ListCtrl.MoveWindow (r);
#if !defined (FETCH_ROWS_INFO)
	m_cStaticTotalFetchedRows.ShowWindow (SW_HIDE);
	m_ListCtrl.MoveWindow (r);
#else
	m_cStaticTotalFetchedRows.GetWindowRect (rInfo);
	ScreenToClient (rInfo);
	nHeight = rInfo.Height();
	rInfo.bottom = rDlg.bottom -1;
	rInfo.top    = rInfo.bottom - nHeight;
	rInfo.left   = rDlg.left;
	rInfo.right  = rDlg.right;
	rList = r;
	rList.bottom = rInfo.top - 2;
	m_ListCtrl.MoveWindow (rList);
	m_cStaticTotalFetchedRows.MoveWindow (rInfo);
#endif
}

void CuDlgSqlQueryPageResult::OnDestroy() 
{
	CDialog::OnDestroy();
}

void CuDlgSqlQueryPageResult::InfoTrace (int nFetchRows)
{
	if (m_bStandAlone)
		return;
	CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
	if (pFrame)
	{
		CdSqlQueryRichEditDoc* pDoc = pFrame->GetSqlDocument();
		CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
		if (pDlgResult && pDoc->IsUsedTracePage())
		{
			CuDlgSqlQueryPageRaw* pRawPage = pDlgResult->GetRawPage();
			ASSERT (pRawPage);

			CString strNew;
			CString strFormat;
			strFormat.LoadString (IDS_F_ROWS);
			strNew.Format (strFormat, nFetchRows);
			pDlgResult->DisplayTraceLine (strNew, NULL);
		}
	}
}




LONG CuDlgSqlQueryPageResult::OnFetchRows (WPARAM wParam, LPARAM lParam)
{
	int nStatus = (int)wParam;
	switch (nStatus)
	{
	case CaExecParamQueryRows::FETCH_NORMAL_ENDING:
		// LPARAM = (Total number of rows fetched)
		m_bAllRows = TRUE;
		AboutFecthing ((int)lParam, TRUE);
		InfoTrace ((int)lParam);
		if (!m_bStandAlone)
		{
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
			if (pFrame)
			{
				CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
				if (pDlgResult)
					pDlgResult->UpdateTabBitmaps();
			}
		}
		break;
	case CaExecParamQueryRows::FETCH_REACH_LIMIT:
		AboutFecthing ((int)lParam, FALSE);
		break;
	case CaExecParamQueryRows::FETCH_ERROR:
		BrokenCursor();
		break;
	case CaExecParamQueryRows::FETCH_TRACEBEGIN:
		if (!m_bStandAlone)
		{
			CaExecParamQueryRows* pQueryRowParam = (CaExecParamQueryRows*)lParam;
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
			if (pFrame && pQueryRowParam)
			{
				CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
				if (pDlgResult)
				{
					CuDlgSqlQueryPageRaw* pRawPage = pDlgResult->GetRawPage();
					ASSERT (pRawPage);
					CString strCursorName = pQueryRowParam->GetCursorName();
					if (!strCursorName.IsEmpty())
					{
						CString strLastCallerID = pRawPage->GetLastCallerID();
						if (pRawPage && !strLastCallerID.IsEmpty() && strCursorName.CompareNoCase (strLastCallerID) != 0)
						{
							CString strNew;
							strNew.Format (_T("%s, <continued>"), (LPCTSTR)m_strEdit1);
							pDlgResult->DisplayTraceLine (strNew, NULL);
						}
					}
				}
			}
		}
		break;
	case CaExecParamQueryRows::FETCH_TRACEINFO:
		if (!m_bStandAlone)
		{
			CaExecParamQueryRows* pQueryRowParam = (CaExecParamQueryRows*)lParam;
			CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)GetParentFrame();
			if (pFrame && pQueryRowParam)
			{
				CuDlgSqlQueryResult* pDlgResult = pFrame->GetDlgSqlQueryResult();
				if (pDlgResult)
				{
					CuDlgSqlQueryPageRaw* pRawPage = pDlgResult->GetRawPage();
					ASSERT (pRawPage);
					if (pRawPage)
					{
						CString strText = (LPCTSTR)(LPTSTR)lParam;
						pDlgResult->DisplayTraceLine (strText, NULL);
					}
				}
			}
		}
		break;

	default:
		break;
	}
	return 0;
}

void CuDlgSqlQueryPageResult::OnOutofmemoryList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	theApp.OutOfMemoryMessage();
	BrokenCursor();
	*pResult = 0;
}


void CuDlgSqlQueryPageResult::OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LPTSTR lpItemData = NULL;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	CWaitCursor doWaitCursor;

	//
	// If rows are not fetched at one time, we cannot sort !!!
	if (!m_bAllRows)
		return;
	if (pNMListView->iSubItem == -1)
		return;
	m_ListCtrl.SortColumn(pNMListView->iSubItem);
}


long CuDlgSqlQueryPageResult::OnChangeSetting(WPARAM wParam, LPARAM lParam)
{
	int nWhat = (int)wParam;
	CaSqlQueryProperty* pProperty = (CaSqlQueryProperty*)lParam;
	if (!pProperty)
		return 0;

	BOOL bDoSomthing = FALSE;
	if (nWhat & SQLMASK_FONT)
	{
		HFONT fFont = (HFONT)pProperty->GetFont();
		m_ListCtrl.SetFont (CFont::FromHandle(fFont));
	}
	if (nWhat & SQLMASK_SHOWGRID) 
	{
		BOOL bGrid = (BOOL)lParam;
		m_ListCtrl.SetLineSeperator (pProperty->IsGrid());
		bDoSomthing = TRUE;
	}
	if (nWhat & SQLMASK_FETCHBLOCK)
	{
		if (pProperty->GetSelectMode() == 0)
		{
			//
			// Old behaviour:
			// If the cursor is still opened, use the cursor to fetch all the remaining
			// rows, at the same time the animate dialog is running and property dialog
			// has not dismissed and if user click on OK again then the re-entrance occured.
			OnCloseCursor(0, 0);
			//
			// NOTE: Re-entrance problem:
			//       When user click on the OK of property dialog, the action
			//       is taken immediately before the property dialog is close.
			//       if the action is in a secondary thread (as animated dialog)
			//       then user will be able to click again the OK button, and the 
			//       re-entrance problem occured.
			//       So the m_ListCtrl.SendMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)1); 
			//       is removed, and OnCloseCursor(0, 0); is added.
			//m_ListCtrl.PostMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)1);
		}
	}
	if (nWhat & SQLMASK_FLOAT4)
	{
		m_ListCtrl.PostMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)2, (LPARAM)pProperty->IsF4Exponential());
	}
	if (nWhat & SQLMASK_FLOAT8)
	{
		m_ListCtrl.PostMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)3, (LPARAM)pProperty->IsF8Exponential());
	}
	if (bDoSomthing)
		m_ListCtrl.Invalidate();
	return 0;
}


LONG CuDlgSqlQueryPageResult::OnGetSqlFont (WPARAM wParam, LPARAM lParam)
{
	HFONT hf = (HFONT)m_ListCtrl.SendMessage(WM_GETFONT);
	return (LONG)hf;
}


void CuDlgSqlQueryPageResult::ExcuteSelectStatement (CaConnectInfo* pConnectInfo, LPCTSTR lpszStatement, CaSqlQueryProperty* pProperty)
{
	CString strMsgBoxTitle;
	CString strInterruptFetching;
	CString strFetchInfo;
	strMsgBoxTitle.LoadString(AFX_IDS_APP_TITLE);
	strInterruptFetching.LoadString(IDS_MSG_INTERRUPT_FETCHING);
	strFetchInfo.LoadString (IDS_ROWS_FETCHED);
	CString strStatement = lpszStatement;
	if (pConnectInfo->GetNode().IsEmpty() && pConnectInfo->GetDatabase().IsEmpty() && strStatement.IsEmpty())
		return;

	//
	// Open & activate session:
	// Check if we need the new session:
	CaSession* pCurrentSession = NULL;
	CaSessionManager& sessionMgr = theApp.GetSessionManager();
	CaSession session;
	session.SetFlag (pConnectInfo->GetFlag());
	session.SetIndependent(FALSE);
	session.SetNode(pConnectInfo->GetNode());
	session.SetDatabase(pConnectInfo->GetDatabase());
	session.SetServerClass(pConnectInfo->GetServerClass());
	session.SetUser(pConnectInfo->GetUser());
	session.SetOptions(pConnectInfo->GetOptions());
	session.SetDescription(sessionMgr.GetDescription());

	SETLOCKMODE lockmode;
	memset (&lockmode, 0, sizeof(lockmode));
	lockmode.nReadLock = pProperty->IsReadLock()? LM_SHARED: LM_NOLOCK;
	pCurrentSession = sessionMgr.GetSession(&session);
	if (pCurrentSession)
	{
		pCurrentSession->Activate();
		pCurrentSession->Commit();
		pCurrentSession->SetAutoCommit(pProperty->IsAutoCommit());
		pCurrentSession->SetLockMode(&lockmode);
		pCurrentSession->Commit();
	}

	int nRowBlock = (pProperty->GetSelectMode() == 1)? pProperty->GetSelectBlock(): 0;
	BOOL bUseCursor = (pProperty->GetSelectMode() == 1);
	CaCursor* pCursor = NULL;
	if (bUseCursor)
	{
		pCursor = new CaCursor (theApp.GetCursorSequence(), lpszStatement, pCurrentSession->GetVersion());
		CaCursorInfo& fetchInfo = pCursor->GetCursorInfo();
		TCHAR tchszF4Exp = pProperty->IsF4Exponential()? _T('e'): _T('n');
		TCHAR tchszF8Exp = pProperty->IsF8Exponential()? _T('e'): _T('n');
		fetchInfo.SetFloat4Format(pProperty->GetF4Width(), pProperty->GetF4Decimal(), tchszF4Exp);
		fetchInfo.SetFloat8Format(pProperty->GetF8Width(), pProperty->GetF8Decimal(), tchszF8Exp);
        CString strSessionDescription = theApp.GetSessionManager().GetDescription();
		pCursor->SetReleaseSessionOnClose(TRUE);
	}

	//
	// Create  Query Row Param, and initialize its parameters
	CaExecParamQueryRows* pQueryRowParam = new CaExecParamQueryRows(bUseCursor);
	CaFloatFormat& fetchInfo = pQueryRowParam->GetFloatFormat();
	TCHAR tchszF4Exp = pProperty->IsF4Exponential()? _T('e'): _T('n');
	TCHAR tchszF8Exp = pProperty->IsF8Exponential()? _T('e'): _T('n');
	fetchInfo.SetFloat4Format(pProperty->GetF4Width(), pProperty->GetF4Decimal(), tchszF4Exp);
	fetchInfo.SetFloat8Format(pProperty->GetF8Width(), pProperty->GetF8Decimal(), tchszF8Exp);
	pQueryRowParam->SetSession (pCurrentSession);
	pQueryRowParam->SetShowRawPage (FALSE);
	pQueryRowParam->SetRowLimit (nRowBlock);
	pQueryRowParam->SetStrings(strMsgBoxTitle, strInterruptFetching, strFetchInfo);

	pQueryRowParam->SetStatement(lpszStatement);
	pQueryRowParam->SetCursor (pCursor);
	SetQueryRowParam(pQueryRowParam);

	// 
	// Construct list headers:
	pQueryRowParam->ConstructListHeader();

	UINT nMask = SQLMASK_SHOWGRID|SQLMASK_FONT;
	SendMessage(WMUSRMSG_CHANGE_SETTING, (WPARAM)nMask, (LPARAM)pProperty);
	PostMessage (WM_SQL_STATEMENT_SHOWRESULT, 0, (LPARAM)0);
}
void CuDlgSqlQueryPageResult::OnKeyDownResultsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
    if( GetKeyState( VK_CONTROL ) )
	{
        if( pLVKeyDow->wVKey == VK_C )
        {
			CString strSelectedValue;
			strSelectedValue.Empty();
			CString strTemp;
			int nLoop = 0;
			//int nColumn = m_ListCtrl.GetSelectedColumn();
			int nCount = m_ListCtrl.GetItemCount();
			while(nLoop < nCount)
			{
				//Browse through all the items and if it is selected, capture the text
				if(m_ListCtrl.GetItemState(nLoop,LVIS_SELECTED) == LVIS_SELECTED)
				{

					//Incase of a multiple selection, append a new line to the 
					//total string.
					if(!strSelectedValue.IsEmpty())
						strSelectedValue += "\r\n";

					//Get all the sub items and their text and lets append it
					//into one string. Since we really do not know which item
					//the user is interested in.
					//This is because, the listctrl has an entire Row selection
					//enabled.
					int nColumns = m_ListCtrl.GetHeaderCtrl()->GetItemCount();
					for(int nSubItem=0;nSubItem<nColumns;nSubItem++)
					{
						if(nSubItem > 0)
							strSelectedValue += "\t";
						strSelectedValue += m_ListCtrl.GetItemText(nLoop,nSubItem);
					}
				}
				nLoop++;
			}
			//Set the ClipBoard with the Selected Text
			if(OpenClipboard())
			{
				HGLOBAL clipbuffer;
				char* buffer;

				EmptyClipboard(); // Empty whatever's already there

				clipbuffer = GlobalAlloc(GMEM_DDESHARE, strSelectedValue.GetLength()+1);
				buffer = (char*)GlobalLock(clipbuffer);
				strcpy(buffer, LPCSTR(strSelectedValue));
				GlobalUnlock(clipbuffer);

				SetClipboardData(CF_TEXT, clipbuffer); // Set the data

				CloseClipboard(); // Close the clipboard - must and should
			}

            return;
        }
    }

	*pResult = 0;
}

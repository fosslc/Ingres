/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lsscroll.cpp, implementation file
**    Project  : Ingres Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Owner draw LISTCTRL to have a custom V Scroll
**
** History:
**
** 23-aug-1999 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 21-Jul-2000 (uk$so01)
**    BUG #102152. Extend the Vertical scroll range limit to be greater than 32767 
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 03-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "calsctrl.h"
#include "lsscroll.h"
#include "ivmsgdml.h"
#include "colorind.h"
#include "wmusrmsg.h"
#include "findinfo.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CuListCtrlVScrollEvent

CuListCtrlVScrollEvent::CuListCtrlVScrollEvent()
{
	m_bDisableGetItems = FALSE;
	m_bOwnerDraw = TRUE;
	m_pCustomVScroll = NULL;
	m_bControlMaxEvent = TRUE;
	m_bDrawInfo = FALSE;
	//
	// Management of a large number of events in the list control:
	m_bScrollManagement = FALSE;
}


CuListCtrlVScrollEvent::~CuListCtrlVScrollEvent()
{
}

void CuListCtrlVScrollEvent::AddEvent (CaLoggedEvent* pEvent, int nIndex)
{
	int nImage = -1;
	int nCount = (nIndex == -1)? GetItemCount(): nIndex;
	BOOL bRead = pEvent->IsRead();

	if (bRead)
		nImage = pEvent->IsAlert()? IM_EVENT_R_ALERT: IM_EVENT_R_NORMAL;
	else
		nImage = pEvent->IsAlert()? IM_EVENT_N_ALERT: IM_EVENT_N_NORMAL;

	nIndex = InsertItem  (nCount, _T(""), nImage);
	if (nIndex != -1)
	{
		SetItemText (nIndex, 1, pEvent->GetDateLocale());
		SetItemText (nIndex, 2, pEvent->m_strComponentType);
		SetItemText (nIndex, 3, pEvent->m_strComponentName);
		SetItemText (nIndex, 4, pEvent->m_strInstance);
		SetItemText (nIndex, 5, pEvent->m_strText);
		SetItemData (nIndex, (DWORD_PTR)pEvent);
	}
	pEvent->SetEventClass (CaLoggedEvent::EVENT_EXIST);
}

void CuListCtrlVScrollEvent::InitializeItems(int nHeader)
{
	int nHeaderCount = 0;
	LV_COLUMN lvc;
	memset (&lvc, 0, sizeof(lvc));
	lvc.mask = LVCF_FMT;
	if (nHeader != -1)
	{
		while (GetColumn(nHeaderCount, &lvc))
			nHeaderCount++;

		if (nHeaderCount != nHeader)
		{
			ASSERT (FALSE);
			return;
		}
	}
	ASSERT (m_pCustomVScroll);
	if (!m_pCustomVScroll)
		return;

	// This function causes to flash all apps in the whole desktop:
	// LockWindowUpdate();
	m_bControlMaxEvent = FALSE;
	DeleteAllItems();

	int nCount = 0;
	int nIndex = m_pCustomVScroll->GetScrollPosExtend();
	int nPage  = GetCountPerPage();
	CaLoggedEvent* e = NULL;
	BOOL bContinue = m_bScrollManagement? (nCount < nPage): TRUE;

	while (bContinue && (nIndex + nCount) < m_arrayEvent.GetSize())
	{
		e = (CaLoggedEvent*)m_arrayEvent.GetAt(nIndex + nCount);
		AddEvent (e);
		nCount++;

		bContinue = m_bScrollManagement? (nCount < nPage): TRUE;
		if (!bContinue)
			break;
	}

	m_bControlMaxEvent = TRUE;
	// This function causes to flash all apps in the whole desktop:
	// UnlockWindowUpdate();
}


BOOL CuListCtrlVScrollEvent::MoveAt(int nPos)
{
	CWaitCursor doWaitCursor;
	CaLoggedEvent* e = NULL;
	BOOL bReturn = FALSE;
	ASSERT (m_pCustomVScroll);
	if (!m_pCustomVScroll)
		return bReturn;

	m_bControlMaxEvent = FALSE;
	
	// This function causes to flash all apps in the whole desktop:
	// LockWindowUpdate();
	DeleteAllItems();

	INT_PTR nEvent = m_arrayEvent.GetSize();
	int i, nCount = 0, nCountPerPage = GetCountPerPage();
	for (i = nPos; i<nEvent; i++)
	{
		e = (CaLoggedEvent*)m_arrayEvent.GetAt (i);
		AddEvent (e);
		nCount++;
		if (m_bScrollManagement && nCount == nCountPerPage)
			break;
	}

	// This function causes to flash all apps in the whole desktop:
	// UnlockWindowUpdate();
	m_bControlMaxEvent = TRUE;
	bReturn = TRUE;

	return bReturn;
}



BOOL CuListCtrlVScrollEvent::NextItems()
{
	CWaitCursor doWaitCursor;
	CaLoggedEvent* e = NULL;
	ASSERT (m_pCustomVScroll);
	if (!m_pCustomVScroll)
		return FALSE;

	int nIndex = m_pCustomVScroll->GetScrollPosExtend();
	m_bControlMaxEvent = FALSE;

	// One line forward.
	//
	// Check if we can move forward:
	if (nIndex + GetCountPerPage() >= m_arrayEvent.GetSize())
	{
		m_bDisableGetItems = FALSE;
		return FALSE;
	}
	DeleteItem (0);

	//
	// Add one line:
	e = (CaLoggedEvent*)m_arrayEvent.GetAt(nIndex + GetCountPerPage());
	AddEvent (e);
	m_bControlMaxEvent = TRUE;

	return TRUE;
}

BOOL CuListCtrlVScrollEvent::BackItems()
{
	CWaitCursor doWaitCursor;
	CaLoggedEvent* e = NULL;
	ASSERT (m_pCustomVScroll);
	if (!m_pCustomVScroll)
		return FALSE;

	int nCount = 0;
	int nIndex = m_pCustomVScroll->GetScrollPosExtend();
	m_bControlMaxEvent = FALSE;

	// One line backward.
	//
	// Check if we can move back:
	nCount = GetItemCount();
	if ((nIndex - 1) < 0 || nCount == 0)
	{
		m_bDisableGetItems = FALSE;
		return FALSE;
	}

	DeleteItem (nCount-1);
	//
	// Add one line:
	e = (CaLoggedEvent*)m_arrayEvent.GetAt (nIndex - 1);
	AddEvent (e, 0);
	m_bControlMaxEvent = TRUE;

	return TRUE;
}


BEGIN_MESSAGE_MAP(CuListCtrlVScrollEvent, CuListCtrl)
	//{{AFX_MSG_MAP(CuListCtrlVScrollEvent)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_WM_PAINT()
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CuListCtrlVScrollEvent message handlers

void CuListCtrlVScrollEvent::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CuListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}


void CuListCtrlVScrollEvent::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CuListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CuListCtrlVScrollEvent::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (!m_bScrollManagement)
	{
		CuListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}
	if (!m_pCustomVScroll)
	{
		CuListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	int nMin, nMax, newPos;
	int nIndex = m_pCustomVScroll->GetScrollPosExtend();
	int nItemPerpage = GetCountPerPage();
	int nSelected = GetNextItem(-1, LVNI_SELECTED);
	m_pCustomVScroll->GetScrollRange(&nMin, &nMax);
	
	switch (nChar)
	{
	case VK_DOWN:
		if (m_bDisableGetItems)
			break;
		if (nIndex >= m_pCustomVScroll->GetRealMaxScroll())
			break;
		if (nSelected == -1 || (nSelected < GetItemCount()-1))
			break;
		m_bDisableGetItems = TRUE;
		newPos = nIndex + 1;
		if (NextItems())
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	case VK_NEXT:
		if (m_bDisableGetItems)
			break;
		if (nIndex >= m_pCustomVScroll->GetRealMaxScroll())
			break;
		m_bDisableGetItems = TRUE;
		newPos = nIndex + nItemPerpage;
		if (newPos > nMax)
			newPos = nMax;
		if (MoveAt (newPos))
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	case VK_UP:
		if (m_bDisableGetItems)
			break;
		if (nIndex <= nMin)
			break;
		if (nSelected == -1 || nSelected > GetTopIndex())
			break;
		m_bDisableGetItems = TRUE;
		newPos = nIndex - 1;
		if (BackItems())
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	case VK_PRIOR:
		if (m_bDisableGetItems)
			break;
		if (nIndex <= nMin)
			break;
		m_bDisableGetItems = TRUE;
		newPos = nIndex - nItemPerpage;
		if (newPos < nMin)
			newPos = nMin;
		if (MoveAt (newPos))
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	case VK_HOME:
		if (m_bDisableGetItems)
			break;
		if (nIndex <= nMin)
			break;
		m_bDisableGetItems = TRUE;
		newPos = 0;
		if (MoveAt (newPos))
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	case VK_END:
		if (m_bDisableGetItems)
			break;
		if (nIndex >= nMax && nMax <= m_pCustomVScroll->GetRealMaxScroll())
			break;
		m_bDisableGetItems = TRUE;
		newPos = nMax;
		if (MoveAt (newPos))
		{
			m_pCustomVScroll->SetScrollPos (newPos, TRUE, newPos);
			m_bDisableGetItems = FALSE;
		}
		break;

	default:
		break;
	}
	
	CuListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CuListCtrlVScrollEvent::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CuListCtrl::OnKeyUp(nChar, nRepCnt, nFlags);
	if (!m_bScrollManagement)
		return;

	m_bDisableGetItems = FALSE;
}


void CuListCtrlVScrollEvent::OnSize(UINT nType, int cx, int cy) 
{
	CuListCtrl::OnSize(nType, cx, cy);
}


//
// This called is consequenced from the removing event from the cache:
void CuListCtrlVScrollEvent::RemoveEvent (CaLoggedEvent* pEvent)
{
	INT_PTR nCount = m_arrayEvent.GetSize();
	for (int i = 0; i<nCount; i++)
	{
		CaLoggedEvent* e0 = (CaLoggedEvent*)m_arrayEvent[i];
		if (e0->GetIdentifier() == pEvent->GetIdentifier())
		{
			m_arrayEvent.RemoveAt(i);
			break;
		}
	}

	//
	// Remove event from the list control:
	if (GetItemCount() > 0)
	{
		LV_FINDINFO f;
		memset (&f, 0, sizeof(f));
		f.flags  = LVFI_PARAM;
		f.lParam = (LPARAM)pEvent;
		int nIndex = FindItem (&f);
		if (nIndex != -1)
			DeleteItem (nIndex);
	}
}


void CuListCtrlVScrollEvent::DrawItem (LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (m_bOwnerDraw)
		CuListCtrl::DrawItem(lpDrawItemStruct);
	else
		CListCtrl::DrawItem(lpDrawItemStruct);
}

void CuListCtrlVScrollEvent::OnPaint() 
{
	if (!m_bScrollManagement)
	{
		CuListCtrl::OnPaint();
	}
	else
	{
		if (!m_pCustomVScroll)
		{
			CuListCtrl::OnPaint();
			return;
		}
		m_pCustomVScroll->SetScrollRange (0, (int)(m_arrayEvent.GetSize() - GetCountPerPage()));
		if (!m_bControlMaxEvent)
		{
			CuListCtrl::OnPaint();
			return;
		}

		CScrollBar* pBarV = GetScrollBarCtrl (SB_VERT);
		if (pBarV)
			pBarV->ShowScrollBar (FALSE);
		//
		// Ensure that the total items count are always = items count per page.
		int nIndex = m_pCustomVScroll->GetScrollPosExtend();
		int nCount = GetItemCount();
		int nMin, nMax;
		m_pCustomVScroll->GetScrollRange(&nMin, &nMax);
		//
		// The control becomes smaller:
		while  (nCount > GetCountPerPage())
		{
			DeleteItem (nCount -1);
			nCount = GetItemCount();
		}

		//
		// The control becomes larger:
		nCount = GetItemCount();
		nIndex = nIndex + nCount;
		while  (nIndex < m_arrayEvent.GetSize() && nCount < GetCountPerPage())
		{
			CaLoggedEvent* e = (CaLoggedEvent*)m_arrayEvent.GetAt (nIndex);
			AddEvent (e);

			nCount = GetItemCount();
			nIndex+= 1;
		}

		CuListCtrl::OnPaint();
		if (pBarV)
			pBarV->ShowScrollBar (FALSE);
	}
}


void CuListCtrlVScrollEvent::SetListEvent(CTypedPtrList<CObList, CaLoggedEvent*>* pListEvent)
{
	CaLoggedEvent* e = NULL;
	POSITION pos = NULL;
	ASSERT (pListEvent);
	m_arrayEvent.RemoveAll();

	if (!pListEvent)
		return;
	m_arrayEvent.SetSize(pListEvent->GetCount(), 100);
	pos = pListEvent->GetHeadPosition();
	int nIndex = 0;
	while (pos != NULL)
	{
		e = pListEvent->GetNext (pos);
		m_arrayEvent.SetAtGrow (nIndex, e);
		nIndex++;
	}
}


void CuListCtrlVScrollEvent::Sort(LPARAM lParamSort, PFNEVENTCOMPARE compare)
{
	if (!m_bScrollManagement)
	{
		ASSERT (lParamSort != 0);
		if (lParamSort == 0)
			return;
		SortItems(compare, lParamSort);
	}
	else
	{
		ASSERT (m_pCustomVScroll);
		if (!m_pCustomVScroll)
			return;
		int nPos =  m_pCustomVScroll->GetScrollPosExtend();
		MoveAt(nPos);
	}
}


void CuListCtrlVScrollEvent::DrawThumbTrackInfo (UINT nPos, BOOL bErase)
{
	if (bErase && !m_bDrawInfo)
		return;
	if (!m_pCustomVScroll)
		return;
	CClientDC dc (this);
	CFont font;
	if (UxCreateFont (font, _T("Courier New"), 16))
	{
		CRect r;
		CString strMsg;
		int nMin, nMax;
		m_pCustomVScroll->GetScrollRange (&nMin, &nMax);
		double d1 = nPos, d2 = nMax;
		double dp = (d1/d2)*100.0;
		CString strScrollTo;
		if (!strScrollTo.LoadString (IDS_MSG_SCROLL_TO))
			strScrollTo = _T("Scroll To %d%%");

		strMsg.Format (strScrollTo, (UINT)dp);
		if (!m_bDrawInfo)
		{
			GetCursorPos(&m_pointInfo);
			ScreenToClient (&m_pointInfo);
			m_pointInfo.x -= 40;

			if (dp > 90.0)
			{
				m_pointInfo.y -= 20;
			}
		}

		GetClientRect (r);
		CFont* pOldFond = dc.SelectObject (&font);
		CSize txSize= dc.GetTextExtent ((LPCTSTR)_T("8888888888888888"));
	
		r.top    = m_pointInfo.y;
		r.left   = m_pointInfo.x - txSize.cx - 10;
		r.right  = m_pointInfo.x;
		r.bottom = r.top + txSize.cy + 4;

		COLORREF oldTextBkColor = dc.SetBkColor (RGB(244, 245, 200));
		CBrush br (RGB(244, 245, 200));
		dc.FillRect(r, &br);
		if (bErase)
		{
			Invalidate();
			m_bDrawInfo = FALSE;
			return;
		}
		dc.DrawText (strMsg, r, DT_CENTER|DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
		dc.SetBkColor (oldTextBkColor);
		dc.SelectObject (pOldFond);
		
		m_bDrawInfo = TRUE;
	}
}

LONG CuListCtrlVScrollEvent::FindText(WPARAM wParam, LPARAM lParam)
{
	CaFindInformation* pFindInfo = (CaFindInformation*)lParam;
	ASSERT (pFindInfo);
	if (!pFindInfo)
		return 0L;

	CString strItem;
	CString strWhat = pFindInfo->GetWhat();
	CaLoggedEvent* pEvent = NULL;
	int i, nStart;
	INT_PTR nCount;
	int nCol = 0;
	//
	// Normal List Control behaviour:
	if (!m_bScrollManagement)
	{
		nCount = GetItemCount();
		if (nCount == 0)
			return FIND_RESULT_NOTFOUND;
		if (pFindInfo->GetListWindow() != this || pFindInfo->GetListWindow() == NULL)
		{
			pFindInfo->SetListWindow(this);
			pFindInfo->SetListPos (0);
		}
		if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
			strWhat.MakeUpper();
		
		nStart = pFindInfo->GetListPos();
		for (i=pFindInfo->GetListPos (); i<nCount; i++)
		{
			pEvent = (CaLoggedEvent*)GetItemData(i);
			pFindInfo->SetListPos (i);
			if (!pEvent)
				continue;

			nCol = 0;
			while (nCol < 6)
			{
				switch (nCol)
				{
				case 0:
					nCol++;
					continue;
					break;
				case 1:
					strItem = pEvent->GetDateLocale();
					break;
				case 2:
					strItem = pEvent->m_strComponentType;
					break;
					break;
				case 3:
					strItem = pEvent->m_strComponentName;
					break;
					break;
				case 4:
					strItem = pEvent->m_strInstance;
					break;
				case 5:
					strItem = pEvent->m_strText;
					break;
				default:
					ASSERT (FALSE);
					break;
				}

				if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
					strItem.MakeUpper();

				int nBeginFind = 0, nLen = strItem.GetLength();
				int nFound = -1;

				if (!(pFindInfo->GetFlag() & FR_WHOLEWORD))
				{
					nFound = strItem.Find (strWhat);
					/*
					if (nFound != -1)
					{
						pFindInfo->SetListPos (i+1);
						EnsureVisible(i, FALSE);
						SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

						return FIND_RESULT_OK;
					}
					*/
				}
				else
				{
					char* chszSep = (char*)(LPCTSTR)theApp.m_strWordSeparator; // Seperator of whole word.
					char* token = NULL;
					// 
					// Establish string and get the first token:
					token = strtok ((char*)(LPCTSTR)strItem, chszSep);
					while (token != NULL )
					{
						//
						// While there are tokens in "strResult"
						if (strWhat.Compare (token) == 0)
						{
							nFound = 0;
							pFindInfo->SetListPos (i+1);
							/*
							EnsureVisible(i, FALSE);
							SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

							return FIND_RESULT_OK;
							*/
						}

						token = strtok(NULL, chszSep);
					}
				}

				if (nFound != -1)
				{
					pFindInfo->SetListPos (i+1);
					EnsureVisible(i, FALSE);
					SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

					return FIND_RESULT_OK;
				}

				//
				// Next column:
				nCol++;
			}
		}

		if (nCount > 0 && nStart > 0)
			return FIND_RESULT_END;
	}
	//
	// Manage the large number of items in the list control,
	// All items are not in the list control but in memory and the list control
	// display only a part of them:
	else
	{
		//
		// The m_arrayEvent contains the events (sort order ..): 
		nCount = m_arrayEvent.GetSize();
		if (nCount == 0)
			return FIND_RESULT_NOTFOUND;

		if (pFindInfo->GetListWindow() != this || pFindInfo->GetListWindow() == NULL)
		{
			pFindInfo->SetListWindow(this);
			pFindInfo->SetListPos (0);
		}
		if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
			strWhat.MakeUpper();
		
		nStart = pFindInfo->GetListPos();
		for (i=pFindInfo->GetListPos (); i<nCount; i++)
		{
			pEvent = (CaLoggedEvent*)m_arrayEvent.GetAt(i);
			pFindInfo->SetListPos (i);
			if (!pEvent)
				continue;

			nCol = 0;
			while (nCol < 6)
			{
				switch (nCol)
				{
				case 0:
					nCol++;
					continue;
					break;
				case 1:
					strItem = pEvent->GetDateLocale();
					break;
				case 2:
					strItem = pEvent->m_strComponentType;
					break;
					break;
				case 3:
					strItem = pEvent->m_strComponentName;
					break;
					break;
				case 4:
					strItem = pEvent->m_strInstance;
					break;
				case 5:
					strItem = pEvent->m_strText;
					break;
				default:
					ASSERT (FALSE);
					break;
				}

				if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
					strItem.MakeUpper();

				int nBeginFind = 0, nLen = strItem.GetLength();
				int nFound = -1;

				if (!(pFindInfo->GetFlag() & FR_WHOLEWORD))
				{
					nFound = strItem.Find (strWhat);
					//
					// The 'i' index is the arrayEvent 0-based index, not
					// the 0-based index in the list control, find the equivalent 0-based
					// index in the list control and select it !!!!
					if (nFound != -1)
					{
						pFindInfo->SetListPos (i+1);
					}
				}
				else
				{
					char* chszSep = (char*)(LPCTSTR)theApp.m_strWordSeparator; // Seperator of whole word.
					char* token = NULL;
					// 
					// Establish string and get the first token:
					token = strtok ((char*)(LPCTSTR)strItem, chszSep);
					while (token != NULL )
					{
						//
						// While there are tokens in "strResult"
						if (strWhat.Compare (token) == 0)
						{
							pFindInfo->SetListPos (i+1);
							nFound = 0;
						}

						token = strtok(NULL, chszSep);
					}
				}

				if (nFound != -1)
				{
					//
					// Check to see if item is in the list control:
					LV_FINDINFO lvfindinfo;
					memset (&lvfindinfo, 0, sizeof (LV_FINDINFO));
					lvfindinfo.flags = LVFI_PARAM;
					lvfindinfo.lParam= (LPARAM)pEvent;
					int nExist = FindItem (&lvfindinfo);
					if (nExist != -1)
					{
						EnsureVisible(nExist, FALSE);
						SetItemState (nExist, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						return FIND_RESULT_OK;
					}
					else
					{
						m_bDisableGetItems = TRUE;
						if (MoveAt (i))
						{
							m_pCustomVScroll->SetScrollPos (i, TRUE, i);
						}
						m_bDisableGetItems = FALSE;
						nExist = FindItem (&lvfindinfo);
						//
						// MoveAt must bring the item into the list control !
						ASSERT (nExist != -1);
						if (nExist != -1)
						{
							EnsureVisible(nExist, FALSE);
							SetItemState (nExist, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
							return FIND_RESULT_OK;
						}
					}
				}

				//
				// Next column:
				nCol++;
			}
		}

		if (nCount > 0 && nStart > 0)
			return FIND_RESULT_END;
	}

	return FIND_RESULT_NOTFOUND;

	return 0L;
}

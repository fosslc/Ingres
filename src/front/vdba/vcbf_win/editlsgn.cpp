/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : editlsgn.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**    Purpose  : Special Owner draw List Control for the Generic Page
**               See the CONCEPT.DOC for more detail
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
**/

#include "stdafx.h"
#include "vcbf.h"
#include "editlsgn.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGeneric

CuEditableListCtrlGeneric::CuEditableListCtrlGeneric()
	:CuEditableListCtrl()
{
	m_bCache = FALSE;
}

CuEditableListCtrlGeneric::~CuEditableListCtrlGeneric()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlGeneric, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlGeneric)
	ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,        OnEditDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,            OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL,  OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,      OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL,    OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,        OnEditSpinDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGeneric message handlers
	
LONG CuEditableListCtrlGeneric::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;

	try
	{
		GENERICLINEINFO* pData = (GENERICLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
			if (GetGenericForCache())
			{
				//
				// Special care of cache management.
				VCBFllCachePaneOnEdit (pData, (LPTSTR)(LPCTSTR)s);
			}
			else
			{
				VCBFllGenPaneOnEdit(pData, (LPTSTR)(LPCTSTR)s);
			}
			ASSERT (strOldName == pData->szname);
			VCBF_GenericSetItem (this, pData, iItem);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGeneric::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlGeneric::OnEditDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

LONG CuEditableListCtrlGeneric::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();

	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	
	try
	{
		GENERICLINEINFO* pData = (GENERICLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
			if (GetGenericForCache())
			{
				//
				// Special care of cache management.
				VCBFllCachePaneOnEdit (pData, (LPTSTR)(LPCTSTR)s);
			}
			else
			{
				VCBFllGenPaneOnEdit(pData, (LPTSTR)(LPCTSTR)s);
			}
			ASSERT (strOldName == pData->szname);
			VCBF_GenericSetItem (this, pData, iItem);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGeneric::OnEditNumberDlgOK has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}

	return 0L;
}

LONG CuEditableListCtrlGeneric::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}



LONG CuEditableListCtrlGeneric::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	int iMin, iMax;
	CString s = EDITSPIN_GetText();
	if (s.IsEmpty())
	{
		EDITSPIN_GetRange (iMin, iMax);
		s.Format (_T("%d"), iMin);
	}
	EDITSPIN_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;

	try
	{
		GENERICLINEINFO* pData = (GENERICLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
			if (GetGenericForCache())
			{
				//
				// Special care of cache management.
				VCBFllCachePaneOnEdit (pData, (LPTSTR)(LPCTSTR)s);
			}
			else
			{
				VCBFllGenPaneOnEdit(pData, (LPTSTR)(LPCTSTR)s);
			}		ASSERT (strOldName == pData->szname);
			VCBF_GenericSetItem (this, pData, iItem);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGeneric::OnEditSpinDlgOK has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
	return 0L;
}

LONG CuEditableListCtrlGeneric::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}


void CuEditableListCtrlGeneric::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	TRACE1 ("CuEditableListCtrlGeneric::OnCheckChange (%d)\n", (int)bCheck);
	CString strOnOff = bCheck? "ON": "OFF";
	GENERICLINEINFO* pData = (GENERICLINEINFO*)GetItemData (iItem);

	try
	{
		if (pData)
		{
			CString strOldName = (LPCTSTR)pData->szname;
			if (GetGenericForCache())
			{
				//
				// Special care of cache management.
				VCBFllCachePaneOnEdit (pData, (LPTSTR)(LPCTSTR)strOnOff);
			}
			else
			{
				VCBFllGenPaneOnEdit(pData, (LPTSTR)(LPCTSTR)strOnOff);
			}
			ASSERT (strOldName == pData->szname);
			VCBF_GenericSetItem (this, pData, iItem);
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGeneric::OnCheckChange has caught exception: %s\n", e.m_strReason);
		CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
		pMain->CloseApplication (FALSE);
	}
	catch (CMemoryException* e)
	{
		VCBF_OutOfMemoryMessage ();
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Other error occured ...\n");
	}
}


void CuEditableListCtrlGeneric::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber = 1;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	if (rect.PtInRect (point))
		CuListCtrl::OnRButtonDown(nFlags, point);
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}




void CuEditableListCtrlGeneric::OnDestroy() 
{
	CuEditableListCtrl::OnDestroy();
}



void CuEditableListCtrlGeneric::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   index, iColumnNumber;
	int   iItem   = -1, iSubItem = -1;
	int   iNumMin = 0;
	int   iNumMax = 400;
	CRect rect, rCell;
	UINT  flags;
	
	index = HitTest (point, &flags);
	if (index < 0)
		return;
	GetItemRect (index, rect, LVIR_BOUNDS);
	if (!GetCellRect (rect, point, rCell, iColumnNumber))
		return;
	
	rCell.top    -= 2;
	rCell.bottom += 2;
	HideProperty();
	if (iColumnNumber != 1)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlGeneric::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}



void CuEditableListCtrlGeneric::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	GENERICLINEINFO* lData = (GENERICLINEINFO*)GetItemData(iItem);
	if (!lData)
		return;
	if (iSubItem != 1)
		return;
	switch (lData->iunittype)
	{
	case GEN_LINE_TYPE_BOOL:
		break;
	case GEN_LINE_TYPE_NUM :
		SetEditNumber (iItem, iSubItem, rcCell);
		break;
	case GEN_LINE_TYPE_SPIN:
		SetEditSpin (iItem, iSubItem, rcCell);
		break;
	case GEN_LINE_TYPE_STRING:
		SetEditText   (iItem, iSubItem, rcCell);
		break;
	default:
		SetEditText   (iItem, iSubItem, rcCell);
		break;
	}
}




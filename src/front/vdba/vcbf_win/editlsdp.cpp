/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : editlsdp.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut 
**    Purpose  : Special Owner draw List Control for the Generic Derived Page 
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
#include "editlsdp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGenericDerived

CuEditableListCtrlGenericDerived::CuEditableListCtrlGenericDerived()
	:CuEditableListCtrlGeneric()
{
	m_bCache = FALSE;
}

CuEditableListCtrlGenericDerived::~CuEditableListCtrlGenericDerived()
{
}



BEGIN_MESSAGE_MAP(CuEditableListCtrlGenericDerived, CuEditableListCtrlGeneric)
	//{{AFX_MSG_MAP(CuEditableListCtrlGenericDerived)
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_CANCEL,		OnEditDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,			OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL,	OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK,		OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL,	OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK,		OnEditSpinDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlGenericDerived message handlers
	
LONG CuEditableListCtrlGenericDerived::OnEditDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDIT_GetText();
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;

	try
	{
		DERIVEDLINEINFO* pData = (DERIVEDLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
		
			BOOL bOk = VCBBllOnEditDependent(pData, (LPTSTR)(LPCTSTR)s);
			ASSERT (strOldName == pData->szname);
			VCBF_GenericDerivedAddItem (this, (LPCTSTR)strOldName, GetGenericForCache());
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGenericDerived::OnEditDlgOK has caught exception: %s\n", e.m_strReason);
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


LONG CuEditableListCtrlGenericDerived::OnEditDlgCancel (UINT wParam, LONG lParam)
{
    return 0L;
}

LONG CuEditableListCtrlGenericDerived::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	CString s = EDITNUMBER_GetText();
	
	EDITNUMBER_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0L;
	try
	{
		DERIVEDLINEINFO* pData = (DERIVEDLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
			BOOL bOk = VCBBllOnEditDependent(pData, (LPTSTR)(LPCTSTR)s);
			ASSERT (strOldName == pData->szname);
			VCBF_GenericDerivedAddItem (this, (LPCTSTR)strOldName, GetGenericForCache());
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGenericDerived::OnEditNumberDlgOK has caught exception: %s\n", e.m_strReason);
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

LONG CuEditableListCtrlGenericDerived::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}


LONG CuEditableListCtrlGenericDerived::OnEditSpinDlgOK (UINT wParam, LONG lParam)
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
		DERIVEDLINEINFO* pData = (DERIVEDLINEINFO*)GetItemData (iItem);
		if (pData)
		{
			CString strOldName = pData->szname;
			BOOL bOk = VCBBllOnEditDependent(pData, (LPTSTR)(LPCTSTR)s);
			ASSERT (strOldName == pData->szname);
			VCBF_GenericDerivedAddItem (this, (LPCTSTR)strOldName, GetGenericForCache());
		}
	}
	catch (CeVcbfException e)
	{
		//
		// Catch critical error 
		TRACE1 ("CuEditableListCtrlGenericDerived::OnEditSpinDlgOK has caught exception: %s\n", e.m_strReason);
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

LONG CuEditableListCtrlGenericDerived::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	return 0L;
}

//
// Two cases:
// 1) Check change on the value column (1)
// 2) Check change on the protected column (3)
void CuEditableListCtrlGenericDerived::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	TRACE1 ("CuEditableListCtrlGeneric::OnCheckChange (%d)\n", (int)bCheck);
	CString strOnOff = bCheck? "ON": "OFF";
	DERIVEDLINEINFO* pData = (DERIVEDLINEINFO*)GetItemData (iItem);
	if (pData)
	{
		BOOL bOk = FALSE;
		CString strOldName = pData->szname;
		switch (iSubItem)
		{
		case 1:
			bOk = VCBBllOnEditDependent(pData,   (LPTSTR)(LPCTSTR)strOnOff);
			break;
		case 3:
			strOnOff = bCheck? "yes": "no";
			bOk = VCBBllOnDependentProtectChange (pData, (LPTSTR)(LPCTSTR)strOnOff);
			break;
		default:
			break;
		}

		ASSERT (strOldName == pData->szname);
		VCBF_GenericDerivedAddItem (this, (LPCTSTR)strOldName, GetGenericForCache());
	}
}


void CuEditableListCtrlGenericDerived::OnRButtonDown(UINT nFlags, CPoint point) 
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
	if (iColumnNumber != 1 && iColumnNumber != 3)
		return;
	EditValue (index, iColumnNumber, rCell);
}



void CuEditableListCtrlGenericDerived::PreSubclassWindow() 
{
	CuEditableListCtrlGeneric::PreSubclassWindow();
}




void CuEditableListCtrlGenericDerived::OnLButtonDblClk(UINT nFlags, CPoint point) 
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
	if (iColumnNumber != 1 && iColumnNumber != 3)
		return;
	EditValue (index, iColumnNumber, rCell);
}

void CuEditableListCtrlGenericDerived::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	//
	// Just swallow the message
	// CuListCtrl::OnRButtonDblClk(nFlags, point);
}


void CuEditableListCtrlGenericDerived::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	DERIVEDLINEINFO* lData = (DERIVEDLINEINFO*)GetItemData(iItem);
	if (!lData)
		return;
	if (iSubItem != 1 || iSubItem == 3)
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
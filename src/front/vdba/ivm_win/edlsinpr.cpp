/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : edlsinpr.cpp, Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Editable List control to provide to Change the parameter of
**               Installation Branch (System and User)
**
**  History:
** 17-Dec-1998 (uk$so01)
**    Created
** 24-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 21-Nov-2003 (uk$so01)
**    SIR #99596, Cleanup
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "edlsinpr.h"
#include "wmusrmsg.h"
#include "compdata.h"
#include <shlobj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuEditableListCtrlInstallationParameter::CuEditableListCtrlInstallationParameter()
{
	m_bAllowEditing = TRUE;
}

CuEditableListCtrlInstallationParameter::~CuEditableListCtrlInstallationParameter()
{
}


BEGIN_MESSAGE_MAP(CuEditableListCtrlInstallationParameter, CuEditableListCtrl)
	//{{AFX_MSG_MAP(CuEditableListCtrlInstallationParameter)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_LAYOUTEDITDLG_OK,  OnEditDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITDLG_EXPR_ASSISTANT,  OnEditAssistant)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_CANCEL, OnEditNumberDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITNUMBERDLG_OK, OnEditNumberDlgOK)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_CANCEL, OnEditSpinDlgCancel)
	ON_MESSAGE (WM_LAYOUTEDITSPINDLG_OK, OnEditSpinDlgOK)
	ON_MESSAGE (WM_LAYOUTCOMBODLG_OK, OnComboDlgOK)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuEditableListCtrlInstallationParameter message handlers


void CuEditableListCtrlInstallationParameter::OnLButtonDblClk(UINT nFlags, CPoint point) 
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
	EditValue (index, iColumnNumber, rCell);
}


void CuEditableListCtrlInstallationParameter::OnRButtonDown(UINT nFlags, CPoint point) 
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
	EditValue (index, iColumnNumber, rCell);
}


LONG CuEditableListCtrlInstallationParameter::OnEditDlgOK (UINT wParam, LONG lParam)
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
		s.TrimLeft();
		s.TrimRight();
		if (s.CompareNoCase(theApp.m_strVariableNotSet) == 0)
		{
			SetFocus();
			return 0L;
		}

		CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
		if (pData)
			pData->SetValue(s);
		DisplayParameter (iItem);
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTEDITDLG_OK, (WPARAM)(LPCTSTR)m_EditDlgOriginalText, (LPARAM)(LPCTSTR)s);
	}
	catch (...)
	{
		AfxMessageBox (
			_T("System error(raise exception):\nCuEditableListCtrlInstallationParameter::OnEditDlgOK(), Cannot change edit text."));
	}
	SetFocus();
	return 0L;
}

void CuEditableListCtrlInstallationParameter::EditValue (int iItem, int iSubItem, CRect rcCell)
{
	if (!m_bAllowEditing)
		return;
	if (iSubItem < 1)
		return;
	CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
	if (!pData)
		return;

	if (pData->IsReadOnly())
		return;
	ParameterType nType = pData->GetType();

	CComboBox* pCombo = NULL;
	long lStyle = 0;
	int nMin, nMax;
	//
	// If this booean is TRUE, then the edit control has a button ">>" that
	// the user can click on it to open the Choose Directory Dialog Box:
	BOOL bUseAssistant = FALSE;
	EDIT_SetExpressionDialog(bUseAssistant);
	switch (iSubItem)
	{
	case C_PARAMETER:
		break;
	case C_VALUE:
		switch (nType)
		{
		case P_TEXT:
			bUseAssistant = FALSE;
			EDIT_SetExpressionDialog(bUseAssistant);
			SetEditText (iItem, iSubItem, rcCell);
			break;
		case P_NUMBER:
			SetEditNumber(iItem, iSubItem, rcCell);
			break;
		case P_NUMBER_SPIN:
			pData->GetMinMax(nMin, nMax);
			SetEditSpin (iItem, iSubItem, rcCell, nMin, nMax);
			break;
		case P_COMBO:
			pCombo = COMBO_GetComboBox();
			ASSERT (pCombo);
			if (!pCombo)
				break;
			if (IsWindow (pCombo->m_hWnd))
			{
				lStyle = ::GetWindowLong (pCombo->m_hWnd, GWL_STYLE);
				if (lStyle & CBS_DROPDOWNLIST)
					lStyle &= ~CBS_DROPDOWNLIST;
				if (!(lStyle & CBS_DROPDOWN))
					lStyle |= CBS_DROPDOWN;
				::SetWindowLong(pCombo->m_hWnd, GWL_STYLE, lStyle);
				pCombo->UpdateWindow();
			}
			//
			// We must intialize the combobox for the item data 'pData' with some data:
			pCombo->ResetContent();
			pCombo->AddString (_T("Test-1"));
			pCombo->AddString (_T("Test-2"));
			pCombo->AddString (_T("Test-3"));
			pCombo->AddString (_T("Test-4"));
			SetComboBox (iItem, iSubItem, rcCell, _T("Test-3"));
			break;
		case P_PATH:
		case P_PATH_FILE:
			bUseAssistant = TRUE;
			EDIT_SetExpressionDialog(bUseAssistant);
			SetEditText (iItem, iSubItem, rcCell);
			break;
		default:
			break;
		}
	default:
		break;
	}
}


void CuEditableListCtrlInstallationParameter::OnCheckChange (int iItem, int iSubItem, BOOL bCheck)
{
	try
	{
		CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
		ASSERT (pData);
		if (!pData)
			return;
		switch (iSubItem)
		{
		case C_PARAMETER:
		case C_DESCRIPTION:
			break;
		case C_VALUE:
			break;
		default:
			return;
		}
		DisplayParameter (iItem);
		//
		// Inform the parent that something changes:
		GetParent()->SendMessage (WM_LAYOUTCHECKBOX_CHECKCHANGE, 0, 0);
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuEditableListCtrlInstallationParameter::OnCheckChange(), Cannot change check box."));
	}
}


void CuEditableListCtrlInstallationParameter::DisplayParameter(int nItem)
{
	CaEnvironmentParameter* pData = NULL;
	int i, nCount = (nItem == -1)? GetItemCount(): nItem +1;
	int nStart = (nItem == -1)? 0: nItem;

	for (i= nStart; i<nCount; i++)
	{
		pData = (CaEnvironmentParameter*)GetItemData (i);
		SetItemText (i, C_PARAMETER,  pData->GetName());
		SetItemText (i, C_VALUE,  pData->GetValue());
		SetItemText (i, C_DESCRIPTION,  pData->GetDescription());
	}
}

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


//
// Assistant for Choosing the path:
LONG CuEditableListCtrlInstallationParameter::OnEditAssistant (WPARAM wParam, LPARAM lParam)
{
	CString strText = (LPCTSTR)lParam;
	TCHAR tchszBuffer [_MAX_PATH];
	if (strText.GetLength() > _MAX_PATH)
		lstrcpyn (tchszBuffer, strText, _MAX_PATH);
	else
		lstrcpy (tchszBuffer, strText);
	int nRes = IDCANCEL;

	CEdit* edit1 = (CEdit*)EDIT_GetEditCtrl();
	if (!edit1)
		return 0;
	if (!IsWindow (edit1->m_hWnd))
		return 0;

	int iItem, iSubItem;
	EDIT_GetEditItem(iItem, iSubItem);
	if (iItem < 0)
		return 0;
	CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
	if (!pData)
		return 0;

	if (pData->GetType() == P_PATH)
	{
		LPMALLOC pIMalloc;
		if (!SUCCEEDED(::SHGetMalloc(&pIMalloc)))
		{
			TRACE0("SHGetMalloc failed.\n");
			return 0;
		}
		//
		// Initialize a BROWSEINFO structure,
		CString strCaption;
		BROWSEINFO brInfo;
		::ZeroMemory(&brInfo, sizeof(brInfo));
		strCaption.LoadString(IDS_CHOOSE_FOLDER);
		brInfo.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
		brInfo.pidlRoot = NULL;
		brInfo.pszDisplayName = tchszBuffer;
		brInfo.lpszTitle = (LPCTSTR)strCaption;
		brInfo.lpfn = &mycallback;
		brInfo.lParam = (LPARAM)(&brInfo);

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
				nRes = IDOK;
			}
			//
			// Free the PIDL
			pIMalloc->Free(browseList);
		}
		else
		{
			nRes = IDCANCEL;
		}
		//
		// Cleanup and release the stuff we used
		pIMalloc->Release();
	}
	else
	if (pData->GetType() == P_PATH_FILE)
	{
		OPENFILENAME ofn;
		TCHAR tchszFilter[_MAX_PATH];

		tchszBuffer[0] = _T('\0');
		tchszFilter[0] = _T('\0');
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner   = m_hWnd;
		ofn.lpstrFilter = tchszFilter;
		ofn.lpstrFile   = tchszBuffer;
		ofn.nMaxFile    = sizeof(tchszBuffer);
		ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
		if (!GetOpenFileName(&ofn))
		{
			tchszBuffer[0] = _T('\0');
			nRes = IDCANCEL;
		}
		else
			nRes = IDOK;
	}

	if (nRes != IDOK)
	{
		if (edit1 && IsWindow (edit1->m_hWnd))
			edit1->SetFocus();
		return 0;
	}

	if (edit1 && IsWindow (edit1->m_hWnd))
	{
		edit1->ReplaceSel (tchszBuffer);
		edit1->SetFocus();
	}
	return 0;
}


LONG CuEditableListCtrlInstallationParameter::OnEditNumberDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	EDITNUMBER_GetEditItem(iItem, iSubItem);
	CEdit*  edit1 = (CEdit*)EDITNUMBER_GetEditCtrl();

	CString s;
	EDITNUMBER_GetText (s);
	s.TrimLeft();
	s.TrimRight();

	if (iItem < 0 || s.IsEmpty())
		return 0;
	
	int i, nLen = s.GetLength();
	for (i=0; i<nLen; i += (int)_tclen((const TCHAR*)s + i))
	{
		if (!_istdigit(s.GetAt(i)))
		{
			SetFocus();
			return 0L;
		}
	}

	if (s.CompareNoCase(theApp.m_strVariableNotSet) == 0)
	{
		SetFocus();
		return 0L;
	}

	CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
	if (pData)
		pData->SetValue(s);
	DisplayParameter (iItem);
	//
	// Inform the parent that something changes:
	GetParent()->SendMessage (WM_LAYOUTEDITNUMBERDLG_OK, (WPARAM)(LPCTSTR)m_EditNumberDlgOriginalText, (LPARAM)(LPCTSTR)s);
	return 0;
}

LONG CuEditableListCtrlInstallationParameter::OnEditNumberDlgCancel (UINT wParam, LONG lParam)
{
	TRACE0 ("TODO : CuEditableListCtrlInstallationParameter::OnEditNumberDlgCancel\n");
	return 0;
}


LONG CuEditableListCtrlInstallationParameter::OnEditSpinDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	int nMin, nMax;
	EDITSPIN_GetEditItem(iItem, iSubItem);
	EDITSPIN_GetRange (nMin, nMax);
	CEdit*  edit1 = (CEdit*)EDITSPIN_GetEditCtrl();

	CString s;
	EDITSPIN_GetText (s);
	s.TrimLeft();
	s.TrimRight();

	if (iItem < 0 || s.IsEmpty())
		return 0;
	
	int iNumber = _ttoi ((LPCTSTR)s);
	if (iNumber > nMax)
	{
		s.Format (_T("%d"), nMax);
		if (edit1 && IsWindow (edit1->m_hWnd))
			edit1->SetWindowText (s);
		MessageBeep (0xFFFFFFFF);
		return 0;
	}
	if (iNumber < nMin)
	{
		s.Format (_T("%d"), nMin);
		if (edit1 && IsWindow (edit1->m_hWnd))
			edit1->SetWindowText (s);
		MessageBeep (0xFFFFFFFF);
		return 0;
	}
	
	if (s.CompareNoCase(theApp.m_strVariableNotSet) == 0)
	{
		SetFocus();
		return 0L;
	}

	CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
	if (pData)
		pData->SetValue(s);
	DisplayParameter (iItem);
	//
	// Inform the parent that something changes:
	GetParent()->SendMessage (WM_LAYOUTEDITSPINDLG_OK, (WPARAM)(LPCTSTR)m_EditSpinDlgOriginalText, (LPARAM)(LPCTSTR)s);
	return 0;
}

LONG CuEditableListCtrlInstallationParameter::OnEditSpinDlgCancel (UINT wParam, LONG lParam)
{
	TRACE0 ("TODO : CuEditableListCtrlInstallationParameter::OnEditSpinDlgCancel\n");
	return 0;
}


LONG CuEditableListCtrlInstallationParameter::OnComboDlgOK (UINT wParam, LONG lParam)
{
	int iItem, iSubItem;
	COMBO_GetEditItem(iItem, iSubItem);
	CString s = COMBO_GetText();
	s.TrimLeft();
	s.TrimRight();

	if (iItem < 0 || s.IsEmpty())
		return 0;

	if (s.CompareNoCase(theApp.m_strVariableNotSet) == 0)
	{
		SetFocus();
		return 0L;
	}

	CaEnvironmentParameter* pData = (CaEnvironmentParameter*)GetItemData(iItem);
	if (pData)
		pData->SetValue(s);
	DisplayParameter (iItem);
	//
	// Inform the parent that something changes:
	GetParent()->SendMessage (WM_LAYOUTCOMBODLG_OK, (WPARAM)(LPCTSTR)m_ComboDlgOriginalText, (LPARAM)(LPCTSTR)s);

	return 0L;
}


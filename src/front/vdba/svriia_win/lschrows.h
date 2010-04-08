/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : lschrows.h: interface for the CuListCtrlDouble class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The control that has 3 controls, header control, and 2 CListCtrl.
**               The first list control has no scrolling management.
**
** History:
**
** 03-Jan-2001 (uk$so01)
**    Created
** 09-Jul-2001 (hanje04)
**	Added class defn for OnEditNumberDlgCancel.
**/

#if !defined(AFX_LSCHROWS_H__37E0BE7E_E15D_11D4_873A_00C04F1F754A__INCLUDED_)
#define AFX_LSCHROWS_H__37E0BE7E_E15D_11D4_873A_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "editlsct.h"
#include "lsctlhug.h"

//
// CLASS CaHeaderRowItemData
// ************************************************************************************************
class CuListCtrlDoubleUpper;
class CaHeaderRowItemData: public CObject
{
public:
	CaHeaderRowItemData(){}
	virtual ~CaHeaderRowItemData(){}
	virtual void Display (CuListCtrlDoubleUpper* pListCtrl, int i){ASSERT (FALSE);};
	virtual void EditValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell){ASSERT (FALSE);}
	virtual BOOL UpdateValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue)
	{
		ASSERT (FALSE);
		return TRUE;
	}

};


//
// CLASS CuListCtrlDoubleUpper
// ************************************************************************************************
class CuListCtrlDoubleUpper: public CuEditableListCtrl
{
public:
	CuListCtrlDoubleUpper():CuEditableListCtrl(){}
	~CuListCtrlDoubleUpper(){}

	void EditValue (int iItem, int iSubItem, CRect rcCell);
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam) { return OnEditDlgOK(wParam, lParam);}
	virtual LONG ManageEditNumberDlgOK (UINT wParam, LONG lParam){ return OnEditNumberDlgOK(wParam, lParam);}
	virtual LONG ManageComboDlgOK (UINT wParam, LONG lParam){return OnComboDlgOK(wParam, lParam);}

	//
	// If nItem != -1 then display the specific item, otherwise display
	// all items.
	void DisplayItem(int nItem = -1);

	void SetAllowEditing(BOOL bAllow = TRUE){m_bAllowEditing = bAllow;}
	BOOL IsAllowEditing(){return m_bAllowEditing;}
	//
	// Generated message map functions
protected:
	BOOL m_bAllowEditing;

	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnComboDlgOK (UINT wParam, LONG lParam);
	afx_msg LONG OnEditNumberDlgCancel     (UINT wParam, LONG lParam)
	{
		return CuEditableListCtrl::OnEditNumberDlgCancel(wParam, lParam);
	}

	DECLARE_MESSAGE_MAP()
};


//
// CLASS CuListCtrlDoubleLower
// ************************************************************************************************
class CuListCtrlDouble;
class CuListCtrlDoubleLower : public CuListCtrlHugeChildList
{
public:
	CuListCtrlDoubleLower();
	void SetOwner (CuListCtrlDouble* pParent){m_pParent = pParent;}
	void SetHeader(CHeaderCtrl* pHeader){m_pHeader = pHeader;}
	void SetHeaderRows(CuListCtrl* pHeaderRows){m_pHeaderRows = pHeaderRows;}

	void SetCurrentScrollPos(int nCurPos){m_nCurrentScrollPos = nCurPos;}

	//
	// Implementation
public:
	virtual ~CuListCtrlDoubleLower();
protected:
	CuListCtrlDouble* m_pParent;
	CHeaderCtrl* m_pHeader;
	CuListCtrl* m_pHeaderRows;
	int m_nCurrentScrollPos;

protected:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
};


//
// CLASS CuListCtrlDouble
// ************************************************************************************************
class CuListCtrlDouble : public CuListCtrlHuge
{
	DECLARE_DYNCREATE(CuListCtrlDouble)
public:
	CuListCtrlDouble();
	void SetFixedRowInfo(int nRow){m_nRowInfo = nRow;}
	int  GetFixedRowInfo(){return m_nRowInfo;}
	void Cleanup();
	//
	// When you add/remove the elements into/from the array 'm_arrayPtr' then you
	// should call this function manage the scrolling (hide/show scrollbar).
	virtual void ResizeToFit();
	virtual void InitializeHeader(LSCTRLHEADERPARAMS* pArrayHeader, int nSize);

	CuListCtrlDoubleUpper& GetListCtrlInfo(){return m_listCtrlHeader;}
	CuListCtrl* GetListCtrl(){return m_pListCtrl;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuListCtrlDouble)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

	//
	// Implementation

public:
	virtual ~CuListCtrlDouble();

protected:
	UINT IDC_HEADER;
	UINT IDC_HEADERROWS;
	UINT IDC_LISTCTRL;
	UINT IDC_STATICDUMMY;

	CHeaderCtrl m_header;
	CuListCtrlDoubleUpper  m_listCtrlHeader;
	CStatic m_cStaticDummy;
	CImageList m_ImageList;

	int m_nHeaderHeight; // The height of header control
	int m_nRowInfo;      // The number of fixed rows of the first list control
	CRect m_rcItem1;     // Item rect of list control 1.
	CScrollBar m_horzScrollBar;
	int m_nCurrentScrollPos;

	void ResizeControl (CRect rcClient);
	// Generated message map functions
protected:
	//{{AFX_MSG(CuListCtrlDouble)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LSCHROWS_H__37E0BE7E_E15D_11D4_873A_00C04F1F754A__INCLUDED_)

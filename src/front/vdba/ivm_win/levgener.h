/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : levgener.h , Header File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Generic Logevent Page
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
**
**/

#if !defined(AFX_LEVGENER_H__5D002663_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)
#define AFX_LEVGENER_H__5D002663_94F4_11D2_A2B7_00609725DDAF__INCLUDED_
#include "calsctrl.h"
#include "lsscroll.h"
#include "uvscroll.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CaLoggedEvent;
class CuDlgLogEventGeneric : public CDialog
{
public:
	CuDlgLogEventGeneric(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}
	void UpdateListEvents(CaPageInformation* pPageInfo);
	void ResizeControls();

	// Dialog Data
	//{{AFX_DATA(CuDlgLogEventGeneric)
	enum { IDD = IDD_LOGEVENT_GENERIC };
	CuVerticalScroll	m_cVerticalScroll;
	CEdit	m_cEdit3;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgLogEventGeneric)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
private:
	BOOL  m_bInitialSize;
	CSize m_sizeInitial;
	BOOL m_bAsc;
	int  m_nSortColumn;
	CTypedPtrList<CObList, CaLoggedEvent*> m_listLoggedEvent;

protected:
	CuListCtrlVScrollEvent m_cListCtrl;
	CImageList m_ImageList;

	void AddEvent (CaLoggedEvent* pEvent);
	void SortByInsert(LPARAM l);
	void ShowMessageDescriptionFrame(int nSel);

	// Generated message map functions
	//{{AFX_MSG(CuDlgLogEventGeneric)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnRemoveEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNewEvents (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnRepaint (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFindAccepted (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEVGENER_H__5D002663_94F4_11D2_A2B7_00609725DDAF__INCLUDED_)

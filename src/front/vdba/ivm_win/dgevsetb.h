/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dgevsetb.h , Header File
**    Project  : Ingres II / Visual Manager 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Modeless Dialog of the bottom pane of Event Setting Frame 
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#if !defined(AFX_DGEVSETB_H__E85F2D44_1373_11D3_A2EE_00609725DDAF__INCLUDED_)
#define AFX_DGEVSETB_H__E85F2D44_1373_11D3_A2EE_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "lsctrevt.h"
#include "calsctrl.h"
#include "uvscroll.h"

class CuDlgEventSettingBottom : public CDialog
{
public:
	CuDlgEventSettingBottom(CWnd* pParent = NULL);
	void OnOK() {return;}
	void OnCancel() {return;}
	CListCtrl* GetListCtrl();
	void ClassifyMessages(long lCat, long lCode, BOOL bActualMessage);
	void ClassifyIngresMessages(long lCat, long lCode);

	void AddEventToGroupByList(CaLoggedEvent* pEv);
	void AddEventToFullDescList(CaLoggedEvent* pEv);
	void AddEventToIngresMessage(CaLoggedEvent* pEv);
	void ShowMessageDescriptionFrame(NMLISTVIEW* pNMListView, CuListCtrlLoggedEvent* pCtrl, BOOL bCreate, BOOL bUpdate=FALSE);


	// Dialog Data
	//{{AFX_DATA(CuDlgEventSettingBottom)
	enum { IDD = IDD_EVENT_SETTING_BOTTOM };
	CButton	m_cButtonFind;
	CuVerticalScroll	m_cVerticalScroll;
	CuListCtrlLoggedEvent	m_cListCtrlIngres;
	CuListCtrlLoggedEvent	m_cListCtrlGroupBy;
	CComboBox	m_cComboIngresCategory;
	CButton	m_cGroupByMessageID;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgEventSettingBottom)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuListCtrlLoggedEvent m_cListCtrlFullDesc;
	CImageList m_ImageList;
	int m_nCurrentGroup;
	SORTPARAMS m_sortListCtrl;
	SORTPARAMS m_sortListCtrl2;
	SORTPARAMS m_sortListCtrl3;

	BOOL m_bActualMessageAll;
	BOOL m_bActualMessageGroup;
	//
	// For the scroll management of full description events:
	CTypedPtrList<CObList, CaLoggedEvent*> m_listLoggedEvent;

	void AddEvent (CaLoggedEvent* pEvent, int nCtrl = 1);
	void ResizeControls();
	void FillIngresCategory();
	void EnableControls();
	void EnableButtonFind();
	CaLoggedEvent* ExistMessage (CaLoggedEvent* pEv, int& nIndex);
	void SortByInsert(LPARAM l);
	void HandleActualMessage();

	// Generated message map functions
	//{{AFX_MSG(CuDlgEventSettingBottom)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRadioActualMessage();
	afx_msg void OnRadioIngresCategory();
	afx_msg void OnSelchangeComboIngresCategory();
	afx_msg void OnGroupByMessageID();
	afx_msg void OnDestroy();
	afx_msg void OnColumnclickList3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonFind();
	afx_msg void OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkList3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateIcon (WPARAM w, LPARAM l);
	afx_msg LONG OnFind (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGEVSETB_H__E85F2D44_1373_11D3_A2EE_00609725DDAF__INCLUDED_)

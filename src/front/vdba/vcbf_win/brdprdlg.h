/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : brdprdlg.h, Header File 
**    Project  : OpenIngres Configuration Manager 
**    Author   : UK Sotheavut
**    Purpose  : Protocol Page of Bridge
** 
** History:
**
** xx-Sep-2001 (uk$so01)
**    Created
** 16-Oct-2001 (uk$so01)
**    BUG #106053, Enable/Disable buttons 
**
*/

#if !defined(AFX_BRDPRDLG_H__66106CA1_3E27_11D1_A202_00609725DDAF__INCLUDED_)
#define AFX_BRDPRDLG_H__66106CA1_3E27_11D1_A202_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// brdprdlg.h : header file
//

#include "editlsdp.h"


class CuEditableListCtrlBridge : public CuEditableListCtrl
{
public:
	CuEditableListCtrlBridge();
	void EditValue (int iItem, int iSubItem, CRect rcCell);

	// Overrides
	//
	virtual ~CuEditableListCtrlBridge();
	virtual LONG ManageEditDlgOK (UINT wParam, LONG lParam)        {return OnEditDlgOK(wParam, lParam);}      
	virtual void OnCheckChange (int iItem, int iSubItem, BOOL bCheck);
	void ResetProtocol (int iItem, BRIDGEPROTLINEINFO* pData);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuEditableListCtrlBridge)
	protected:
	//}}AFX_VIRTUAL

	// Generated message map functions
protected:

	//{{AFX_MSG(CuEditableListCtrlBridge)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	afx_msg LONG OnEditDlgOK (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeProtocol dialog

class CuDlgBridgeProtocol : public CDialog
{
public:
	CuDlgBridgeProtocol(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgBridgeProtocol)
	enum { IDD = IDD_BRIDGE_PAGE_PROTOCOL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgBridgeProtocol)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlBridge m_ListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CuDlgBridgeProtocol)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnButton1   (UINT wParam, LONG lParam);     // Edit Value	
	afx_msg LONG OnButton2   (UINT wParam, LONG lParam);     // Default
	afx_msg LONG OnButton3   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton4   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnButton5   (UINT wParam, LONG lParam);     // Not Used
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BRDPRDLG_H__66106CA1_3E27_11D1_A202_00609725DDAF__INCLUDED_)

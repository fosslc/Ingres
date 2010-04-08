/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dasdprdlg.h, Header File 
**    Project  : VCBF 
**    Author   : UK Sotheavut
**    Purpose  : Protocol Page of DAS Server
** 
** History:
**
** xx-Sep-2001 (uk$so01)
**  10-Mar-2004: (uk$so01) 
**     created
**     SIR #110013 of DAS Server (VCBF should handle DAS Server)
*/

#if !defined(_DASDPRDLG_HEADER)
#define _DASDPRDLG_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// brdprdlg.h : header file
//

//#include "editlsdp.h"
#include  "editlnet.h"  // custom list control for net

/*
class CuEditableListCtrlDAS : public CuEditableListCtrl
{
public:
	CuEditableListCtrlDAS();
	void EditValue (int iItem, int iSubItem, CRect rcCell);

	// Overrides
	//
	virtual ~CuEditableListCtrlDAS();
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
*/


/////////////////////////////////////////////////////////////////////////////
// CuDlgBridgeProtocol dialog

class CuDlgDASVRProtocol : public CDialog
{
public:
	CuDlgDASVRProtocol(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;}
	void OnCancel() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDASVRProtocol)
	enum { IDD = IDD_DASVR_PAGE_PROTOCOL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDASVRProtocol)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CImageList m_ImageList;
	CImageList m_ImageListCheck;
	CuEditableListCtrlNet m_ListCtrl;

	BOOL AddProtocol(LPNETPROTLINEINFO lpProtocol);
	void ResetList();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDASVRProtocol)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnValidateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEditListenAddress (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
};

#endif // !defined(_DASDPRDLG_HEADER)

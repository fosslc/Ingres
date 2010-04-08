/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cleftdlg.h, header file 
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut
**               Blatte Emmanuel (Custom implementations)
**    Purpose  : Modeless dialog displayed at the left-pane of the page Configure 
**
** History:
**
** 21-Sep-1997: (uk$so01)
**    Created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
** 16-Oct-2001 (uk$so01)
**    BUG #106053, Free the unused memory 
**/

/////////////////////////////////////////////////////////////////////////////
// CConfLeftDlg dialog
#ifndef CLEFTDLG_HEADER
#define CLEFTDLG_HEADER
#include "editlsco.h"
#include "cbfitem.h"
#include "ll.h"

class CConfLeftDlg : public CDialog
{
public:
	void OnMainFrameClose();
	CConfLeftDlg(CWnd* pParent = NULL);   // standard constructor
	void OnOK() {return;};
	void OnCancel() {return;};
	CToolTipCtrl* m_gpToolTip;

	// Dialog Data
	//{{AFX_DATA(CConfLeftDlg)
	enum { IDD = IDD_CONFIG_LEFT };
	CTreeCtrl	m_tree_ctrl;
	CuEditableListCtrlComponent m_clistctrl;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfLeftDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList m_Imagelist1;
	CImageList m_Imagelist2;
	CToolTipCtrl m_ToolTip;

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void OnContextMenu(CWnd*, CPoint point);


	void GetMaxInternaleName (LPCTSTR lpszTemplateName, long& lnCount);
	// Generated message map functions
	//{{AFX_MSG(CConfLeftDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditName();
	afx_msg void OnEditCount();
	afx_msg void OnDuplicate();
	afx_msg void OnDelete();
	afx_msg void OnItemexpandedTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclickTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclickTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginlabeleditTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditTree1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnComponentExit  (UINT wParam, LONG lParam);
	afx_msg LONG OnComponentEnter (UINT wParam, LONG lParam);
	afx_msg LONG OnUpdateData     (UINT wParam, LONG lParam);
	afx_msg LONG OnValidateData   (UINT wParam, LONG lParam);
	DECLARE_MESSAGE_MAP()
private:
	CuCbfListViewItem* CreateItemFromComponentInfo(LPCOMPONENTINFO lpcomp, int &imageIndex);
	BOOL AddSystemComponent(LPCOMPONENTINFO lpcomp);
	HTREEITEM AddItemToTree( LPCTSTR lpszItem, HTREEITEM hParent, int bDir, int iIconIndex, CuCbfListViewItem *pItem );
	HTREEITEM ClickWithinTree();
	HTREEITEM FindInTree(HTREEITEM hRootItem, LPCTSTR szFindText);
	void GetMaxName(LPCTSTR lpszTemplateName, long& lnMaxCount, HTREEITEM hti);
	void OnMenuDuplicate(HTREEITEM hDupItem);
	void OnMenuDelete();
	void OnMenuProperties(BOOL bCount=FALSE);

};
#endif //#ifndef CLEFTDLG_HEADER

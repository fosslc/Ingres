/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgindx.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating indices.             .                  //
****************************************************************************************/
#if !defined(AFX_XDLGINDX_H__43C05611_5301_11D2_A2A3_00609725DDAF__INCLUDED_)
#define AFX_XDLGINDX_H__43C05611_5301_11D2_A2A3_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calsctrl.h"
#include "uchklbox.h"
#include "extccall.h"
#include "dmlindex.h"




class CxDlgCreateIndex : public CDialog
{
public:
	CxDlgCreateIndex(CWnd* pParent = NULL);
	void SetCreateParam (DMLCREATESTRUCT* pParam);

	// Dialog Data
	//{{AFX_DATA(CxDlgCreateIndex)
	enum { IDD = IDD_XCREATE_INDEX };
	CButton	m_cButtonOK;
	CButton	m_cCheckPersistence;
	CEdit	m_cEditFillfactor;
	CEdit	m_cEditExtend;
	CEdit	m_cEditAllocation;
	CButton	m_cCheckCompressKey;
	CButton	m_cCheckCompressData;
	CEdit	m_cEditIndexName;
	CSpinButtonCtrl	m_cSpinFillfactor;
	CStatic	m_cStaticMinY;
	CStatic	m_cStaticMinX;
	CStatic	m_cStaticMaxY;
	CStatic	m_cStaticMaxX;
	CComboBox	m_cComboStructure;
	CComboBox	m_cComboPageSize;
	CEdit	m_cEditMinY;
	CEdit	m_cEditMinX;
	CEdit	m_cEditMaxX;
	CEdit	m_cEditMaxY;
	CEdit	m_cEditNonleaffill;
	CEdit	m_cEditLeaffill;
	CEdit	m_cEditMaxPage;
	CEdit	m_cEditMinPage;
	CSpinButtonCtrl	m_cSpinNonleaffill;
	CSpinButtonCtrl	m_cSpinLeaffill;
	CButton	m_cStaticRange;
	CStatic	m_cStaticNonleaffill;
	CStatic	m_cStaticLeaffill;
	CStatic	m_cStaticMaxPage;
	CStatic	m_cStaticMinPage;
	BOOL	m_bCompressData;
	BOOL	m_bCompressKey;
	BOOL	m_bPersistence;
	int		m_nPageSize;
	int		m_nStructure;
	CString	m_strIndexName;
	CString	m_strMinPage;
	CString	m_strMaxPage;
	CString	m_strLeaffill;
	CString	m_strNonleaffill;
	CString	m_strAllocation;
	CString	m_strExtend;
	CString	m_strFillfactor;
	CString	m_strMaxX;
	CString	m_strMaxY;
	CString	m_strMinX;
	CString	m_strMinY;
	//}}AFX_DATA

	CString m_strDatabase;
	CString m_strTable;
	CString m_strTableOwner;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgCreateIndex)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


	//
	// Implementation
protected:
	DMLCREATESTRUCT* m_pParam;
	BOOL m_bOnNewIndexName;
	int  m_nNameSeq;
	
	int m_nMinPageMin;
	int m_nMinPageMax;
	int m_nMaxPageMin;
	int m_nMaxPageMax;
	int m_nLeaffillMin;
	int m_nLeaffillMax;
	int m_nNonleaffillMin;
	int m_nNonleaffillMax;
	int m_nAllocationMin;
	int m_nAllocationMax;
	int m_nExtendMin;
	int m_nExtendMax;
	int m_nFillfactorMin;
	int m_nFillfactorMax;

	
	CString m_strIIDatabase;

	CImageList m_ImageHeight;
	CImageList m_ImageCheck;
	CuListCtrl m_cListCtrlBaseTableColumn;
	CuListCtrl m_cListCtrlIndexColumn;
	CuListCtrl m_cListIndexName;
	CuCheckListBox m_cListLocation;
	CBitmapButton m_cButtonUp;
	CBitmapButton m_cButtonDown;

	CTypedPtrList<CObList, CaIndexStructure*> m_listStructure;
	CTypedPtrList<CObList, CaIndex*> m_listIndex;


	BOOL InitializeBaseTableColumns();
	BOOL InitializeLocations();
	void InitPageSize (LONG lPage_size);
	CaIndex* GetCurrentIndex(int& nSel);
	BOOL GenerateSyntax(CString& strStatement);
	//
	// Test if the parameters for creating indices are valid:
	BOOL CheckValidate();
	//
	// Enable/Disable Button OK:
	void EnableDisableButtonOK();
	//
	// Data from pIndex to Dialog controls:
	void SetIndexData (CaIndex* pIndex);
	//
	// Data from Dialog controls to pIndex:
	void GetIndexData (CaIndex* pIndex);
	CString  GetIndexName();
	void SelchangeComboIndexStructure(BOOL bSetDefaultValue = TRUE);

	// Generated message map functions
	//{{AFX_MSG(CxDlgCreateIndex)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	virtual void OnOK();
	afx_msg void OnChangeEditIndexName();
	afx_msg void OnButtonNameNew();
	afx_msg void OnButtonNameDelete();
	afx_msg void OnButtonColumnAdd();
	afx_msg void OnButtonColumnRemove();
	afx_msg void OnButtonColumnUp();
	afx_msg void OnButtonColumnDown();
	afx_msg void OnDblclkListBasetableColumns(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListIndexColumns(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboIndexStructure();
	afx_msg void OnItemchangedListIndexName(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboIndexPagesize();
	afx_msg void OnKillfocusEditIndexAllocation();
	afx_msg void OnKillfocusEditIndexExtend();
	afx_msg void OnKillfocusEditIndexFillfactor();
	afx_msg void OnKillfocusEditIndexLeaffill();
	afx_msg void OnKillfocusEditIndexMaxpage();
	afx_msg void OnKillfocusEditIndexMinpage();
	afx_msg void OnKillfocusEditIndexNonleaffill();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGINDX_H__43C05611_5301_11D2_A2A3_00609725DDAF__INCLUDED_)

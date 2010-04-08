/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgidxop.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                       .                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for the Constraint Enhancement (Index Option)        //
****************************************************************************************/
#if !defined(AFX_XDGIDXOP_H__A4ADB044_594A_11D2_A2A5_00609725DDAF__INCLUDED_)
#define AFX_XDGIDXOP_H__A4ADB044_594A_11D2_A2A5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "uchklbox.h"
#include "dmlindex.h"


class CxDlgIndexOption : public CDialog
{
public:
	CxDlgIndexOption(CWnd* pParent = NULL);
	void SetAlter(){m_bAlter = TRUE;}
	void SetTableParams(LPVOID pTable){m_pTable = pTable;}

	// Dialog Data
	//{{AFX_DATA(CxDlgIndexOption)
	enum { IDD = IDD_XINDEX_OPTION };
	CStatic	m_cStaticIndex;
	CComboBox	m_cComboIndexNameDDList;
	CStatic	m_cStaticIndexName;
	CButton	m_cCheckProperty;
	CComboBox	m_cComboIndex;
	CStatic	m_cStaticLocation;
	CStatic	m_cStaticExtend;
	CStatic	m_cStaticAllocation;
	CStatic	m_cStaticStructure;
	CStatic	m_cStaticFillfactor;
	CButton	m_cButtonOK;
	CComboBox	m_cComboIndexName;
	CEdit	m_cEditFillfactor;
	CEdit	m_cEditExtend;
	CEdit	m_cEditAllocation;
	CEdit	m_cEditNonleaffill;
	CEdit	m_cEditLeaffill;
	CEdit	m_cEditMaxPage;
	CEdit	m_cEditMinPage;
	CSpinButtonCtrl	m_cSpinNonleaffill;
	CSpinButtonCtrl	m_cSpinLeaffill;
	CSpinButtonCtrl	m_cSpinFillfactor;
	CComboBox	m_cComboStructure;
	int		m_nStructure;
	CString	m_strMinPage;
	CString	m_strMaxPage;
	CString	m_strLeaffill;
	CString	m_strNonleaffill;
	CString	m_strAllocation;
	CString	m_strExtend;
	CString	m_strFillfactor;
	CStatic	m_cStaticNonleaffill;
	CStatic	m_cStaticLeaffill;
	CStatic	m_cStaticMaxPage;
	CStatic	m_cStaticMinPage;
	int		m_nComboIndex;
	//}}AFX_DATA
	BOOL m_bDefineProperty;
	int  m_nGenerateMode;

	LPVOID m_pParam;
	CString m_strIndexName;
	CString m_strDatabase;

	//
	// Table that contains this index option (Alter):
	CString m_strTable;
	CString m_strTableOwner;

	CString m_strTitle;
	CString m_strStructure;
	CStringList m_listLocation;
	BOOL    m_bFromCreateTable;

	// m_nCallFor->
	// (0: primary key. 1: unique key. 2: foreign key)
	int m_nCallFor; 

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIndexOption)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

//
// Implementation
protected:
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
	BOOL m_bAlter;

	CuCheckListBox m_cListLocation;
	CTypedPtrList<CObList, CaIndexStructure*> m_listStructure;
	LPVOID m_pTable; // Must be casted to LPTABLEPARAMS

	BOOL InitializeLocations();
	void ShowControls (int nShow);
	void SetDisplayMode(BOOL bShared=FALSE);
	void InitializeComboIndex();
	BOOL QueryExistingIndexes();
	BOOL InitializeSharedIndexes();
	void DisplayIndexInfo (LPVOID lpvIndex, BOOL bShared = FALSE);
	void EnableOKButton();
	void SharedIndexChange (int nMember, LPCTSTR lpszValue);
	void IndexTructurePrepareControls(int nStructure);

	// Generated message map functions
	//{{AFX_MSG(CxDlgIndexOption)
	virtual void OnOK();
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboIndexTructure();
	afx_msg void OnKillfocusEditIndexAllocation();
	afx_msg void OnKillfocusEditIndexExtend();
	afx_msg void OnKillfocusEditIndexFillfactor();
	afx_msg void OnKillfocusEditIndexLeaffill();
	afx_msg void OnKillfocusEditIndexMaxpage();
	afx_msg void OnKillfocusEditIndexMinpage();
	afx_msg void OnKillfocusEditIndexNonleaffill();
	afx_msg void OnEditchangeComboIndexname();
	afx_msg void OnSelchangeComboIndexname();
	afx_msg void OnSelchangeComboIndex();
	afx_msg void OnCheckDefineProperty();
	afx_msg void OnSelchangeComboIndexNameDDList();
	//}}AFX_MSG
	afx_msg void OnCheckChangeLocation();
	DECLARE_MESSAGE_MAP()
};

void INDEXOPTION_STRUCT2IDX (CxDlgIndexOption* pDest,   LPVOID lpSource);
void INDEXOPTION_IDX2STRUCT (CxDlgIndexOption* pSource, LPVOID lpDest);

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGIDXOP_H__A4ADB044_594A_11D2_A2A5_00609725DDAF__INCLUDED_)

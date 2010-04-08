#if !defined(AFX_ICEBUNITFP_H__516E09E7_5CEF_11D2_9727_00609725DBF9__INCLUDED_)
#define AFX_ICEBUNITFP_H__516E09E7_5CEF_11D2_9727_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// icebunitfp.h : header file
//
extern "C" 
{
#include "ice.h"
}
/////////////////////////////////////////////////////////////////////////////
// CxDlgIceBunitFacetPage dialog

class CxDlgIceBunitFacetPage : public CDialog
{
// Construction
public:
	LPICEBUSUNITDOCDATA m_StructDoc;
	CxDlgIceBunitFacetPage(CWnd* pParent = NULL);   // standard constructor

	CString m_csVirtNodeName;
	int     m_nNodeHandle;
	// Dialog Data
	//{{AFX_DATA(CxDlgIceBunitFacetPage)
	enum { IDD = IDD_ICE_BUSINESS_FACET_PAGE };
	CStatic	m_ctrlstReloadFile;
	CButton	m_ctrlReloadFrom;
	CButton	m_ctrlbtPreCache;
	CButton	m_ctrlbtRepository;
	CButton	m_ctrlcbStaticCache;
	CStatic	m_ctrlctLocation;
	CComboBox	m_ctrlcbLocation;
	CEdit	m_ctrledFileNameExt;
	CEdit	m_ctrledFileName;
	CButton	m_ctrlcbBrowseFile;
	CButton	m_ctrlOK;
	CStatic	m_ctrlstSeparator;
	CStatic	m_ctrlstPath;
	CStatic	m_ctrlstFileName;
	CEdit	m_ctrledDocName;
	CEdit	m_ctrledExt;
	CEdit	m_ctrledPathName;
	BOOL	m_bAccess;
	int		m_nRepository;
	int		m_nPreCache;
	CString	m_csLocation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgIceBunitFacetPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableDisableControl();
	void FillStructureWithControl(LPICEBUSUNITDOCDATA IbuDdta);
	void EnableDisableOK();
	void fillIceLocationData();

	// Generated message map functions
	//{{AFX_MSG(CxDlgIceBunitFacetPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowseFile();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnStRepository();
	afx_msg void OnStFileSystem();
	afx_msg void OnChangeIceDocName();
	afx_msg void OnChangeIceEditFilename();
	afx_msg void OnChangeIceFilenameExt();
	afx_msg void OnSelchangeIceLocation();
	afx_msg void OnChangePath();
	afx_msg void OnPermCache();
	afx_msg void OnPreCache();
	afx_msg void OnSessionCache();
	afx_msg void OnCheckreloadfile();
	afx_msg void OnChangeIceDocExt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICEBUNITFP_H__516E09E7_5CEF_11D2_9727_00609725DBF9__INCLUDED_)

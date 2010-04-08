/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xmaphost.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Popup dialog for mapping host functionality
**
** History:
**
** 11-Oct-2002 (uk$so01)
**    SIR #109221, created
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined(AFX_XMAPHOST_H__B09E5335_DABB_11D6_87EB_00C04F1F754A__INCLUDED_)
#define AFX_XMAPHOST_H__B09E5335_DABB_11D6_87EB_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsctrl.h"
#include "vcdadml.h"


class CxDlgHostMapping : public CDialog
{
public:
	CxDlgHostMapping(CWnd* pParent = NULL);
	void SetSnapshotCaptions(LPCTSTR lpSnapshot1, LPCTSTR lpSnapshot2)
	{
		m_strSnapshot1 = lpSnapshot1;
		m_strSnapshot2 = lpSnapshot2;
	}
	void SetParams(CStringList& strList1Host, CStringList& strList2Host, CTypedPtrList< CObList, CaMappedHost* >& listMappedHost)
	{
		m_pstrList1Host = &strList1Host;
		m_pstrList2Host = &strList2Host;
		m_plistMappedHost = &listMappedHost;
	}

	// Dialog Data
	//{{AFX_DATA(CxDlgHostMapping)
	enum { IDD = IDD_XMAPHOST };
	CStatic	m_cStaticH2;
	CStatic	m_cStaticH1;
	CButton	m_cButtonRemove;
	CButton	m_cButtonMap;
	CListBox	m_cList2Host;
	CListBox	m_cList1Host;
	//}}AFX_DATA
	CuListCtrl m_cListCtrl;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgHostMapping)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CStringList* m_pstrList1Host;
	CStringList* m_pstrList2Host;
	CTypedPtrList< CObList, CaMappedHost* >* m_plistMappedHost;
	CString m_strSnapshot1;
	CString m_strSnapshot2;

	// Generated message map functions
	//{{AFX_MSG(CxDlgHostMapping)
	afx_msg void OnSelchangeList1();
	afx_msg void OnSelchangeList2();
	afx_msg void OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonMap();
	afx_msg void OnButtonRemove();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XMAPHOST_H__B09E5335_DABB_11D6_87EB_00C04F1F754A__INCLUDED_)

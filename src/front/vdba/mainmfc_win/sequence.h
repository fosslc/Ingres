#if !defined(AFX_SEQUENCE_H__98BAD1E3_5B78_11D7_B6FF_00C04F1790C3__INCLUDED_)
#define AFX_SEQUENCE_H__98BAD1E3_5B78_11D7_B6FF_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// sequence.h : header file
//
extern "C" {
#include "dbaset.h"
}

/////////////////////////////////////////////////////////////////////////////
// CxDlgDBSequence dialog

class CxDlgDBSequence : public CDialog
{
// Construction
public:
	CxDlgDBSequence(CWnd* pParent = NULL);   // standard constructor
	LPSEQUENCEPARAMS m_StructSequenceInfo;
	SEQUENCEPARAMS   m_StructSeqBeforeChange;
// Dialog Data
	//{{AFX_DATA(CxDlgDBSequence)
	enum { IDD = IDD_CREATEDB_SEQUENCE };
	CButton	m_ctrlOK;
	CEdit	m_ctrlEditCache;
	CButton	m_ctrlCache;
	CEdit	m_ctrlEditName;
	CStatic	m_ctrlStaticPrecision;
	CStatic	m_ctrlStartWith;
	CEdit	m_ctrlEditDecimal;
	CString	m_SequenceName;
	BOOL	m_bCycle;
	BOOL	m_bCache;
	CString	m_MaxValue;
	CString	m_MinValue;
	CString	m_csStartWith;
	CString	m_csCacheSize;
	CString	m_csIncrementBy;
	CString	m_csDecimalPrecision;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgDBSequence)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL Copy2SequenceParam(LPSEQUENCEPARAMS lpSeqSrcParam, LPSEQUENCEPARAMS lpSeqDestParam);
	void InitClassMember();
	void FillStructureWithCtrlInfo();
	BOOL AlterSequence();

	// Generated message map functions
	//{{AFX_MSG(CxDlgDBSequence)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnRadioDecimal();
	afx_msg void OnRadioInteger();
	afx_msg void OnCheckCache();
	afx_msg void OnChangeEditSequenceName();
	afx_msg void OnUpdateEditSequenceName();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEQUENCE_H__98BAD1E3_5B78_11D7_B6FF_00C04F1790C3__INCLUDED_)

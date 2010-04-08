#ifndef DOMPROP_INDEX_HEADER
#define DOMPROP_INDEX_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPusr.h : header file
//

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIndex form view

class CuDlgDomPropIndex : public CFormView
{
private:
  BOOL m_bMustInitialize;

protected:
	CuDlgDomPropIndex();
  DECLARE_DYNCREATE(CuDlgDomPropIndex)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIndex)
	enum { IDD = IDD_DOMPROP_INDEX };
	CStatic	m_stPersist;
	CListBox	m_lbLocations;
	CListCtrl	m_clistColumns;
	CEdit	m_edPgSize;
	CEdit	m_edUnique;
	CEdit	m_edParent;
	CEdit	m_edIdxSchema;
	CEdit	m_edFillFact;
	CEdit	m_edExtend;
	CEdit	m_edAllocatedPages;
	CEdit	m_edAlloc;
	CStatic	m_stStruct4;
	CStatic	m_stStruct3;
	CStatic	m_stStruct2;
	CStatic	m_stCompress;
	CStatic	m_stStruct1;
	CEdit	m_edStruct4;
	CEdit	m_edStruct3;
	CEdit	m_edStruct2;
	CEdit	m_edStruct1;
	CEdit	m_edStruct;
	CButton	m_chkPersist;
	CButton	m_chkCompress;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIndex)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIndex m_Data;

// Implementation
protected:
    virtual ~CuDlgDomPropIndex();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIndex)
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // DOMPROP_INDEX_HEADER

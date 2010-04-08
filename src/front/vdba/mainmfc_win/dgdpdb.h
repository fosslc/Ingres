#ifndef DOMPROP_DB_HEADER
#define DOMPROP_DB_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DgDPInt.h : header file
//

#include "domseri.h"
#include "uchklbox.h"
#include "afxwin.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropDb Form View

class CuDlgDomPropDb : public CFormView
{
protected:
  CuDlgDomPropDb();
  DECLARE_DYNCREATE(CuDlgDomPropDb)

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropDb)
	enum { IDD = IDD_DOMPROP_DB };
	CStatic	m_ctrlStaticUnicode;
	CStatic	m_ctrlStaticUnicode2;
	CButton	m_ctrlUnicode;
	CButton    m_ctrlUnicode2;
	CEdit	m_edLocChk;
	CEdit	m_edOwner;
	CEdit	m_edLocSrt;
	CEdit	m_edLocJnl;
	CEdit	m_edLocDmp;
	CEdit	m_edLocDb;
	CButton	m_ReadOnly;
	//}}AFX_DATA

  // list of installed Front End Catalogs
	CuCheckListBox	m_CheckFrontEndList;

  // Alternate locations list control
  CuListCtrlCheckmarks  m_cListAlternateLocs;
  CImageList            m_ImageList;
  CImageList            m_ImageCheck;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropDb)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataDb m_Data;

  // Mandatory for controls subclass good management whatever the pane is created or loaded
  BOOL  m_bNeedSubclass;
  void SubclassControls();
  void AddAltLoc (CuAltLoc* pAltLoc);

// Implementation
protected:
  virtual ~CuDlgDomPropDb();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropDb)
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_StatCtrlCatPageSize;
	CEdit m_ctrlEditCatPageSize;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // DOMPROP_DB_HEADER

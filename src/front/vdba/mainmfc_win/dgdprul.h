#ifndef DOMPROP_RULE_HEADER
#define DOMPROP_RULE_HEADER

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropRule

class CuDlgDomPropRule : public CDialog
{
public:
    CuDlgDomPropRule(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropRule)
	enum { IDD = IDD_DOMPROP_RULE };
	CEdit	m_edText;
	CEdit	m_edProc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropRule)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataRule m_Data;

// Implementation
protected:
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropRule)
    virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_RULE_HEADER

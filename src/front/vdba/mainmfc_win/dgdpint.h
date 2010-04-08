#ifndef DOMPROP_INTEGRITY_HEADER
#define DOMPROP_INTEGRITY_HEADER

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropIntegrity 

class CuDlgDomPropIntegrity : public CDialog
{
public:
    CuDlgDomPropIntegrity(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropIntegrity)
	enum { IDD = IDD_DOMPROP_INTEG };
	CEdit	m_edText;
	CEdit	m_edNumber;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropIntegrity)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIntegrity m_Data;

// Implementation
protected:
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropIntegrity)
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_INTEGRITY_HEADER

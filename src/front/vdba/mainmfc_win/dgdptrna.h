#ifndef DOMPROP_TABLE_ROWSNA_HEADER
#define DOMPROP_TABLE_ROWSNA_HEADER

#include "domseri.h"

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableRowsNA

class CuDlgDomPropTableRowsNA : public CDialog
{
public:
    CuDlgDomPropTableRowsNA(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

public:
// Dialog Data
	//{{AFX_DATA(CuDlgDomPropTableRowsNA)
	enum { IDD = IDD_DOMPROP_TABLE_ROWSNA };
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropTableRowsNA)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataTableRowsNA m_Data;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropTableRowsNA)
    virtual BOOL OnInitDialog();
	//}}AFX_MSG

  afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
  afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_TABLE_ROWSNA_HEADER

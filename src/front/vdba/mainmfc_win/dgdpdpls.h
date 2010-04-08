#ifndef DOMPROP_PROC_L_SOURCE_HEADER
#define DOMPROP_PROC_L_SOURCE_HEADER

#include "domseri.h"

class CuDlgDomPropProcLinkSource : public CFormView
{
protected:
    CuDlgDomPropProcLinkSource();
    DECLARE_DYNCREATE(CuDlgDomPropProcLinkSource)

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropProcLinkSource)
	enum { IDD = IDD_DOMPROP_PROC_L_SOURCE };
	CEdit	m_edLdbObjOwner;
	CEdit	m_edLdbNode;
	CEdit	m_edLdbObjName;
	CEdit	m_edLdbDbmsType;
	CEdit	m_edLdbDatabase;
	//}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropProcLinkSource)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataProcLinkSource m_Data;

    // Implementation
protected:
  virtual ~CuDlgDomPropProcLinkSource();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropProcLinkSource)
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

#endif  // DOMPROP_PROC_L_SOURCE_HEADER

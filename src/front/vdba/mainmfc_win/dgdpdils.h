#ifndef DOMPROP_INDEX_L_SOURCE_HEADER
#define DOMPROP_INDEX_L_SOURCE_HEADER

#include "domseri.h"

class CuDlgDomPropIndexLinkSource : public CFormView
{
protected:
    CuDlgDomPropIndexLinkSource();
    DECLARE_DYNCREATE(CuDlgDomPropIndexLinkSource)

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropIndexLinkSource)
	enum { IDD = IDD_DOMPROP_INDEX_L_SOURCE };
	CEdit	m_edLdbObjOwner;
	CEdit	m_edLdbNode;
	CEdit	m_edLdbObjName;
	CEdit	m_edLdbDbmsType;
	CEdit	m_edLdbDatabase;
	//}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropIndexLinkSource)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIndexLinkSource m_Data;

    // Implementation
protected:
  virtual ~CuDlgDomPropIndexLinkSource();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropIndexLinkSource)
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

#endif  // DOMPROP_INDEX_L_SOURCE_HEADER

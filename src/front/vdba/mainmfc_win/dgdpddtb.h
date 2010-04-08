#ifndef DOMPROP_DDB_TBL_HEADER
#define DOMPROP_DDB_TBL_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"

class CuDlgDomPropDDbTbl : public CDialog
{
public:
    CuDlgDomPropDDbTbl(CWnd* pParent = NULL);  
    void AddTbl (CuNameWithOwner *pDbTbl);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropDDbTbl)
    enum { IDD = IDD_DOMPROP_DDB_TBLS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CImageList m_StarImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropDDbTbl)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataDbTbl m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropDDbTbl)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_DDB_TBL_HEADER

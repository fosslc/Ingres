#ifndef DOMPROP_REPL_CONN_HEADER
#define DOMPROP_REPL_CONN_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"
#include "domilist.h"

class CuDlgDomPropReplicConn : public CDialog
{
public:
    CuDlgDomPropReplicConn(CWnd* pParent = NULL);  
    void AddConn (CuNameWithNumber *pReplicConn);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropReplicConn)
    enum { IDD = IDD_DOMPROP_REPL_CONN };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropReplicConn)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataReplicConn m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropReplicConn)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnColumnclickMfcList1(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_REPL_CONN_HEADER

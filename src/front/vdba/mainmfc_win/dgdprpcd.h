#ifndef DOMPROP_REPL_CDDS_HEADER
#define DOMPROP_REPL_CDDS_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"
#include "domilist.h"

class CuDlgDomPropReplicCdds : public CDialog
{
public:
    CuDlgDomPropReplicCdds(CWnd* pParent = NULL);  
    void AddCdds ( LPCTSTR name);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropReplicCdds)
    enum { IDD = IDD_DOMPROP_REPL_CDDS };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropReplicCdds)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataReplicCdds m_Data;

    // Implementation
protected:
   void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropReplicCdds)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    //}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_REPL_CDDS_HEADER

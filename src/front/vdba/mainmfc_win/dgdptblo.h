#ifndef DOMPROP_TABLE_LOC_HEADER
#define DOMPROP_TABLE_LOC_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"
#include "domilist.h"

class CuDlgDomPropTblLoc : public CDialog
{
public:
    CuDlgDomPropTblLoc(CWnd* pParent = NULL);  
    void AddTblLoc (CuNameWithOwner *pTblLoc);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropTblLoc)
    enum { IDD = IDD_DOMPROP_TABLE_LOC };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList        m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropTblLoc)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataTblLoc m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropTblLoc)
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

#endif  // DOMPROP_TABLE_LOC_HEADER

#ifndef DOMPROP_ICE_SEC_PROFILES_HEADER
#define DOMPROP_ICE_SEC_PROFILES_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri3.h"
#include "domilist.h"  // CuDomImageList

class CuDlgDomPropIceSecProfiles : public CDialog
{
public:
    CuDlgDomPropIceSecProfiles(CWnd* pParent = NULL);  
    void AddProfile (CuNameOnly *pIceSecProfiles);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropIceSecProfiles)
    enum { IDD = IDD_DOMPROP_ICE_SEC_PROFILES };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropIceSecProfiles)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataIceSecProfiles m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropIceSecProfiles)
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

#endif  // DOMPROP_ICE_SEC_PROFILES_HEADER

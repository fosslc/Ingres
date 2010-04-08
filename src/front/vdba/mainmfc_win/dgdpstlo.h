#ifndef DOMPROP_ST_LOCATION_HEADER
#define DOMPROP_ST_LOCATION_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri2.h"
#include "domilist.h"  // CuDomImageList

class CuDlgDomPropStaticLocation : public CDialog
{
public:
    CuDlgDomPropStaticLocation(CWnd* pParent = NULL);  
    void AddLocation (CuNameOnly *pLocation);
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropStaticLocation)
    enum { IDD = IDD_DOMPROP_ST_LOCATION };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrl m_cListCtrl;
    CuDomImageList m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropStaticLocation)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();

private:
  CuDomPropDataStaticLocation m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropStaticLocation)
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

#endif  // DOMPROP_ST_LOCATION_HEADER

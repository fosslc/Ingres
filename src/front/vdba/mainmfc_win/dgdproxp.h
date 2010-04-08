#ifndef DOMPROP_ROLE_XGRANTED_PROC_HEADER
#define DOMPROP_ROLE_XGRANTED_PROC_HEADER

#include "domseri2.h"
#include "domilist.h"

class CuDlgDomPropRoleXGrantedProc : public CDialog
{
public:
    CuDlgDomPropRoleXGrantedProc(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropRoleXGrantedProc)
    enum { IDD = IDD_DOMPROP_ROLE_XGRANTED_PROC };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrlCheckmarks  m_cListCtrl;
    CImageList            m_ImageCheck;
    CuDomImageList        m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropRoleXGrantedProc)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddRoleXGrantedProc (CuGrantedProc* pGrantedProc);

private:
  CuDomPropDataRoleXGrantedProc m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropRoleXGrantedProc)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnColumnClickList1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_ROLE_XGRANTED_PROC_HEADER

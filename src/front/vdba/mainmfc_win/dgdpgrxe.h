#ifndef DOMPROP_GROUP_XGRANTED_EVENT_HEADER
#define DOMPROP_GROUP_XGRANTED_EVENT_HEADER

#include "domseri2.h"
#include "domilist.h"

class CuDlgDomPropGroupXGrantedEvent : public CDialog
{
public:
    CuDlgDomPropGroupXGrantedEvent(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropGroupXGrantedEvent)
    enum { IDD = IDD_DOMPROP_GROUP_XGRANTED_DBEV };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA
    CuListCtrlCheckmarks  m_cListCtrl;
    CImageList            m_ImageCheck;
    CuDomImageList        m_ImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropGroupXGrantedEvent)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddGroupXGrantedEvent (CuGrantedEvent* pGrantedEvent);

private:
  CuDomPropDataGroupXGrantedEvent m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropGroupXGrantedEvent)
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

#endif  // DOMPROP_GROUP_XGRANTED_EVENT_HEADER

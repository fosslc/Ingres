/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgVNData.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Connection Data)     //
//               The Advanced Concept of Virtual Node.                                 //
****************************************************************************************/
#ifndef DGVNDATA_HEADER
#define DGVNDATA_HEADER

class CxDlgVirtualNodeData : public CDialog
{
public:
    CxDlgVirtualNodeData(CWnd* pParent = NULL);   // standard constructor
    void SetAlter() {m_bAlter = TRUE;}
    // Dialog Data
    //{{AFX_DATA(CxDlgVirtualNodeData)
    enum { IDD = IDD_XVNODEDATA };
    CButton    m_cOK;
    BOOL       m_bPrivate;
    CString    m_strVNodeName;
    CString    m_strRemoteAddress;
    CString    m_strListenAddress;
    CString    m_strProtocol;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CxDlgVirtualNodeData)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    BOOL m_bAlter;
    BOOL m_bDataAltered;

    // Generated message map functions
    //{{AFX_MSG(CxDlgVirtualNodeData)
    afx_msg void OnCheckPrivate();
    afx_msg void OnSelchangeProtocol();
    afx_msg void OnChangeIPAddress();
    afx_msg void OnChangeListenPort();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    CString m_strOldVNodeName;       
    CString m_strOldRemoteAddress;   
    CString m_strOldListenAddress;   
    BOOL m_bOldPrivate;           
    CString m_strOldProtocol;        

    void EnableOK ();
};
#endif

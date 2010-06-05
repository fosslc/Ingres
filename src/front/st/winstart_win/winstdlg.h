/*
**  Copyright (C) 1997, 2000 Ingres Corporation
*/

/*
**
**  Name: winstdlg.h
**
**  History:
**  01-dec-2000 (penga03)
**      Destory the semaphore when exit winstart.
**	25-Oct-2006 (drivi01)
**		Added a new variable m_bCloseDialog.  When this
**		variable is set to True, dialog will close automatically
**		once the operation is completed.
**	25-Jul-2007 (drivi01)
**		Add a variable to track timer event.
**	06-Apr-2010 (drivi01)
**		Update OnTimer to take UINT_PTR as a parameter as UINT_PTR
**		will be unsigned int on x86 and unsigned int64 on x64.
**		This will cleanup warnings.
**
*/ 

#include "cwinbat.h"
#include "whiteedt.h"

/////////////////////////////////////////////////////////////////////////////
// CWinstartDlg dialog

class CWinstartDlg : public CDialog
{
// Construction
public:

	enum DIALOG_STATE { STATE_CHECKING, STATE_WAITINGTOSTART, STATE_STARTING, 
						STATE_WAITINGTOSTOP, STATE_STOPPING, STATE_FAILED };

	CWinstartDlg(CWnd* pParent = NULL);	// standard constructor
	void SetMessage(WINBATRC wbrc, CWinBat *pcWinBat, CString *lpcsMessage = NULL),
		 SetState(DIALOG_STATE st);
	DWORD ThreadControl(void), CheckActiveInstallation(void);
    void StartIngresAsService();
    void StopIngresAsService();
    BOOL ClientServiceExists();
    BOOL ServerServiceExists();
    BOOL IngresAlreadyRunning();
    BOOL IngresRunningAsService();
    BOOL IngresRunningAsClientService();
    void GetNTErrorText(int code, CString &strErrText);

    CString m_csCmdLine;
    BOOL m_bStartFlag;
    BOOL m_bStopFlag;
    BOOL m_bParamFlag;
    CString m_strParam;

// Dialog Data
	//{{AFX_DATA(CWinstartDlg)
	enum { IDD = IDD_WINSTART_DIALOG };
	CButton	m_cbStartAsClient;
	CButton	m_cbStartAsService;
	CStatic	m_txStatusText;
	CButton	m_bnStart;
	CStatic	m_icIcon;
	CStatic	m_txMessage;
	CButton	m_bnCancel;
	CWhiteEdit m_ceLog;
	CButton	m_cbKill;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinstartDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	BOOL  	m_bCheckingActive, m_bIngStartCompleted, m_bRunning;
	DWORD	m_dwIDThread;
	CWinBat m_cWinBat[3], *m_pcWinBat;
	CString m_csBuffer;
	DWORD 	m_dwBytesRead, m_rc;
	BOOL	m_bCancelRequested;
	UINT	m_iIcon; 
	UINT	m_iAvi; 
	BOOL    m_iAviRun;
	BOOL    m_bFirstTime;
	BOOL	m_bCloseDialog;
    CString m_II_SYSTEM;
    CString m_II_INSTALLATION;
	CRITICAL_SECTION m_cs;
	UINT	m_timerEvent;

	friend DWORD WBThreadControl(LPVOID pParam), 
				 WBCheckActiveInstallation(LPVOID pParam);

	enum { MAXSTRINGSIZE = 512, BUFSIZE = 4096, MINDATALEN = 8 };


	// Generated message map functions
	//{{AFX_MSG(CWinstartDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnCancel();
	afx_msg void OnStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnStartAsService();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
extern HANDLE hSemaphore;

#pragma once


// CConfigType dialog

class ConfigType : public CPropertyPage
{
	DECLARE_DYNAMIC(ConfigType)

public:
	ConfigType();
	virtual ~ConfigType();

// Dialog Data
	//{{AFX_DATA(ConfigType)
	enum { IDD = IDD_CONFIGTYPE };
	CFont m_font_bold;
	CButton bi_radio;
	CButton txn_radio;
	CButton cm_radio;
	CButton default_radio;
	//}}AFX_DATA

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWizardBack();

	DECLARE_MESSAGE_MAP()
};

#pragma once
/*
** Copyright (c) 2006 Ingres Corporation
*/

/*
**
** Name: Summary.h - header file for Summary.cpp
**
** History:
**	16-Nov-2006 (drivi01)
**	    Created.
**	03-May-2007 (drivi01)
**	    Added two strings to the summary, one gets
**	    activated when user databases were upgraded
**	    and one when user didn't choose to upgrade
**	    databases.  This is for upgrade scenario only.
*/

class Summary : public CPropertyPage
{
	DECLARE_DYNAMIC(Summary)

public:
	Summary();
	virtual ~Summary();

// Dialog Data
	enum { IDD = IDD_SUMMARY };
	CButton m_launchpad;
	CStatic m_title1;
	CStatic m_title2;
	CStatic m_titleError;
	CStatic m_successText;
	CStatic m_errorText;
	CStatic m_msgblock;
	CStatic m_sum_path;
	CStatic m_sum_id;
	CStatic m_udbupgraded;
	CStatic m_udbupgraded2;
	C256bmp m_image1;
	C256bmp m_image2;
	CButton m_info;
	CFont	m_font_bold;
	CFont   m_font_bold_small;
	CRichEditCtrl	m_rtf;



protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	DECLARE_MESSAGE_MAP()


public:
	afx_msg BOOL OnWizardFinish();
	afx_msg void OnBnClickedButtonInfo();
};

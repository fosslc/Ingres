/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: Dependencies.h - Header for Resource Dependencies property page 
** 
** History:
**	29-apr-2004 (wonst02)
**	    Created.
** 	29-jul-2004 (wonst02)
** 		Add GetDependencies().
*/

#pragma once
#include "256bmp.h"
#include "afxcmn.h"
#include "afxwin.h"


// WcDependencies dialog

class WcDependencies : public CPropertyPage
{
	DECLARE_DYNAMIC(WcDependencies)

public:
	WcDependencies();   // standard constructor
	virtual ~WcDependencies();

// Dialog Data
	enum { IDD = IDD_DEPENDENCYPROP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnSetActive();
	afx_msg void OnBnClickedAddButton();
	afx_msg void OnBnClickedRemoveButton();
	C256bmp m_image;
	CListCtrl m_ResourceListCtrl;
	CListCtrl m_DependencyListCtrl;
	CButton m_AddButtonCtrl;
	CButton m_RemoveButtonCtrl;
	virtual BOOL OnInitDialog();
	afx_msg void OnNMDblclkResourceList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkDependencyList(NMHDR *pNMHDR, LRESULT *pResult);
	virtual LRESULT OnWizardNext();
private:
	// Get and display the list of Windows Cluster resources.
	void SetList(void);
	void GetDependencies(void); // Get current Ingres resource dependencies
};

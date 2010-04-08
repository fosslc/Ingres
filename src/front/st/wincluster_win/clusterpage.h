/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
**  History:
**	29-apr-2004 (wonst02)
**	    Created.
** 	29-jul-2004 (wonst02)
** 	    Display the cluster group name and resource name for the Ingres service.
**	02-dec-2004 (wonst02) Bug 113522
**	    Add Ingres service password and confirm password.
*/

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "256bmp.h"


// WcClusterPage dialog

class WcClusterPage : public CPropertyPage
{
	DECLARE_DYNAMIC(WcClusterPage)

public:
	WcClusterPage();
	virtual ~WcClusterPage();

// Dialog Data
	enum { IDD = IDD_CLUSTERPROP };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnSetActive();
	CEdit m_ClusterNameCtrl;
	CString m_ClusterName;
	virtual BOOL OnInitDialog();
	virtual LRESULT OnWizardNext();

	// Cluster group name
	CComboBox m_GroupNameCtrl;
	CString m_GroupName;
	// Ingres resource name
	CComboBox m_ResourceNameCtrl;
	CString m_ResourceName;
	// Ingres service password and confirm password
	CEdit   m_ServicePasswordCtrl;
	CString m_ServicePassword;
	CEdit   m_ConfirmPasswordCtrl;
	CString m_ConfirmPassword;
	C256bmp m_image;
private:
	// Get the resource type for the named cluster resource.
	LPWSTR GetResourceType(HRESOURCE hRes);
	LPVOID GetPrivProperties(HRESOURCE hRes, HNODE hNode, LPDWORD lpcbDataSize);
	DWORD GetPrivPropValue(HRESOURCE hRes, LPCWSTR lpszPropName, LPWSTR* lpszPropValue);
	DWORD WcClusterPage::GetResourceState(HRESOURCE hRes, LPWSTR* lpszGroupName);
};
